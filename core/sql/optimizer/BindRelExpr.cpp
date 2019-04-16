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
* File:		BindRelExpr.C
* Description:  Relational expressions (both physical and logical operators)
*               Methods related to the SQL binder
*
* Created:      5/17/94
* Language:     C++
*
*
*
*	It is the secret sympathy,
*	The silver link, the silken tie,
*	Which heart to heart, and mind to mind,
*	In body and in soul can bind.
*		-- Sir Walter Scott,
*		   "The Lay of the Last Minstrel"
*
******************************************************************************
*/


#define   SQLPARSERGLOBALS_FLAGS   // must precede all #include's
#define   SQLPARSERGLOBALS_NADEFAULTS

#include "Platform.h"
#include "NAWinNT.h"


#include "Sqlcomp.h"
#include "AllItemExpr.h"
#include "AllRelExpr.h"
#include "BindWA.h"
#include "ComOperators.h"
#include "ComTransInfo.h"
#include "ComLocationNames.h"
#include "ControlDB.h"
#include "Debug.h"
#include "ex_error.h"
#include "GroupAttr.h"
#include "ParNameLocList.h"
#include "parser.h"
#include "Rel3GL.h"
#include "RelDCL.h"
#include "RelPackedRows.h"
#include "RelSequence.h"
#include "ShowSchema.h"           // GetControlDefaults class
#include "StmtDDLAddConstraintCheck.h"
#include "StmtDDLCreateView.h"
#include "ElemDDLColRefArray.h"
#include "ElemDDLSaltOptions.h"
#include "TrafDDLdesc.h"
#include "UdrErrors.h"
#include "SequenceGeneratorAttributes.h"

#include "wstr.h"
#include "Inlining.h"
#include "Triggers.h"
#include "TriggerDB.h"
#include "MVInfo.h"
#include "Refresh.h"
#include "ChangesTable.h"
#include "MvRefreshBuilder.h"
#include "OptHints.h"
#include "CmpStatement.h"
#include "OptimizerSimulator.h"
#include "charinfo.h"
#include "UdfDllInteraction.h"
#include "SqlParserGlobals.h"      // must be last #include
#include "ItmFlowControlFunction.h"
#include "ComSchemaName.h" // for ComSchemaName
#include "ItemSample.h"
#include "NAExecTrans.h"
#include "HDFSHook.h"
#include "CmpSeabaseDDL.h"
#include "ComUser.h"
#include "ComSqlId.h"
#include "PrivMgrCommands.h"
#include "PrivMgrComponentPrivileges.h"
#include "PrivMgrDefs.h"
#include "PrivMgrMD.h"

  #define SLASH_C '/'

NAWchar *SQLTEXTW();

// -----------------------------------------------------------------------
// external declarations
// -----------------------------------------------------------------------

// 



// -----------------------------------------------------------------------
// static functions
// -----------------------------------------------------------------------
#ifdef NDEBUG
  THREAD_P NABoolean GU_DEBUG = FALSE;
#else
  THREAD_P NABoolean GU_DEBUG;
#endif

static void GU_DEBUG_Display(BindWA *bindWA, GenericUpdate *gu,
                             const char *text,
                             RelExpr *reDown = NULL,
                             NABoolean preEndl = FALSE,
                             NABoolean postEndl = FALSE)
{
#ifndef NDEBUG
  if (!GU_DEBUG)
    return;

  if (preEndl) cerr << endl;
  cerr << "---" << endl;

  if (gu->getTableDesc()) {
    NAString tmp;
    ValueIdList vtmp(gu->getTableDesc()->getColumnList());
    vtmp.unparse(tmp);
    cerr << gu->getUpdTableNameText() << " this>td(" << text << ") "
      << gu->getTableDesc()->getCorrNameObj().getExposedNameAsAnsiString()
      << " " << tmp << endl;
  }

  RETDesc *rd = gu->getRETDesc();
  if (rd) {
    cerr << gu->getUpdTableNameText() << " this>grd(" << text << ") " << flush;
    rd->display();
  }
  if (reDown) RETDesc::displayDown(reDown);

  if (bindWA->getCurrentScope()->getRETDesc() &&
      bindWA->getCurrentScope()->getRETDesc() != rd) {
    cerr << gu->getUpdTableNameText() << " bwa>cs>grd(" << text << ") " <<flush;
    bindWA->getCurrentScope()->getRETDesc()->display();
  }

  if (postEndl) cerr << endl;
#endif
} // GU_DEBUG_Display()

static RETDesc *bindRowValues(BindWA *bindWA,
                              ItemExpr *exprTree,
                              ValueIdList &vidList,
                              RelExpr *parent,
                              NABoolean inTrueRoot)
{
  // Before we convert the row value expressions into a ValueIdList, save the
  // original value expression root nodes in an ItemExprList.
  //
  ItemExprList exprList(exprTree, bindWA->wHeap());
  //
  // Bind the row value expressions and create a ValueIdList.
  //
  exprTree->convertToValueIdList(vidList, bindWA, ITM_ITEM_LIST, parent);
  if (bindWA->errStatus()) return NULL;

  // Set up context flags.
  // We are in a subquery if the previous scope's flag is set, note.
  //
  BindScope *currScope   = bindWA->getCurrentScope();
  BindScope *prevScope   = bindWA->getPreviousScope(currScope);
  NABoolean inSelectList = currScope->context()->inSelectList();
  NABoolean inInsert     = currScope->context()->inInsert();
  NABoolean inSubquery   = FALSE;
  if (prevScope)
    inSubquery = prevScope->context()->inSubquery();

  // See if UDF_SUBQ_IN_AGGS_AND_GBYS is enabled. It is enabled if the
  // default is ON, or if the default is SYSTEM and ALLOW_UDF is ON.
  NABoolean udfSubqInAggGrby_Enabled = FALSE;
  DefaultToken udfSubqTok = CmpCommon::getDefault(UDF_SUBQ_IN_AGGS_AND_GBYS);
  if ((udfSubqTok == DF_ON) ||
      (udfSubqTok == DF_SYSTEM))
    udfSubqInAggGrby_Enabled = TRUE;
  
  // See if ALLOW_MULTIDEGREE_SUBQ_IN_SELECTLIST is enabled. It is
  // enabled if the default is ON, or if the default is SYSTEM and
  // ALLOW_UDF is ON.
  NABoolean allowMultiDegSubqInSelect_Enabled = FALSE;
  DefaultToken allowMultiDegreeTok =
    CmpCommon::getDefault(ALLOW_MULTIDEGREE_SUBQ_IN_SELECTLIST);
  if ((allowMultiDegreeTok == DF_ON) ||
      (allowMultiDegreeTok == DF_SYSTEM))
    allowMultiDegSubqInSelect_Enabled = TRUE;
  
  //
  // Create the result table.
  // If a row value expression is not a column reference and does not have
  // a rename AS clause, the column is an unnamed expression.
  //
  RETDesc *resultTable = new (bindWA->wHeap()) RETDesc(bindWA);
  CollIndex j = 0;
  for (CollIndex i = 0; i < exprList.entries(); i++, j++) 
  {
    ItemExpr *itemExpr = (ItemExpr *) exprList[i];
    ValueId valId = itemExpr->getValueId();
    ValueId boundValId = vidList[j];
    CMPASSERT(boundValId != NULL_VALUE_ID);

    if (inSelectList && inTrueRoot &&
        (boundValId.getType().getTypeQualifier() == NA_UNKNOWN_TYPE)&&
        (boundValId.getItemExpr()->getOperatorType() == ITM_CONSTANT))
    {
      ConstValue * constItemExpr = (ConstValue*) boundValId.getItemExpr();
      if (constItemExpr->isNull())
        boundValId.coerceType(NA_NUMERIC_TYPE) ;
    }

    switch (itemExpr->getOperatorType()) 
    {
      case ITM_REFERENCE: {
        ColReference *colRef = (ColReference *) itemExpr;
        const ColRefName &colRefName = colRef->getColRefNameObj();
        CMPASSERT(valId != NULL_VALUE_ID || colRefName.isStar());

        if (colRefName.isStar()) {
          const ColumnDescList *star = colRef->getStarExpansion();
          CMPASSERT(star != NULL);
          const ColumnDescList &starExpansion = *star;
          CMPASSERT(starExpansion.entries() > 0);   // ColRef::bind chked this alrdy
          CMPASSERT(inSelectList);

          resultTable->addColumns(bindWA, starExpansion);

          j += starExpansion.entries() - 1;
        }               // isStar
        else {
          // Do another xcnm lookup so the column we add to our resultTable
          // will have its CorrName object correct
          // (e.g., in "SELECT TL.B,* FROM TA TL,TA TR ORDER BY B;"
          // colref TL.B will resolve to TL.B, not CAT.SCH.TL.B)
          // and its heading (Genesis 10-980126-5495).
          BindScope *bindScope;
          ColumnNameMap *xcnmEntry = bindWA->findColumn(colRefName, bindScope);

          if (NOT xcnmEntry)   // ## I don't recall when this case occurs...
            resultTable->addColumn(bindWA,
                                   colRefName,
                                   boundValId,
                                   colRef->getTargetColumnClass());
          else
            resultTable->addColumn(bindWA,
                                   xcnmEntry->getColRefNameObj(),
                                   boundValId,
                                   colRef->getTargetColumnClass(), // MV --
                                   xcnmEntry->getColumnDesc()->getHeading());
        }
        break;
      }
      case ITM_RENAME_COL: 
      {
        RenameCol *renameCol = (RenameCol *) itemExpr;
        const ColRefName &colRefName = *renameCol->getNewColRefName();
        CMPASSERT(NOT colRefName.isStar());

        const char * heading = NULL;

        // if this rename was for a BLOB/CLOB column from JDBC, return
        // the heading of the child base column. This is needed for JDBC
        // as it uses the heading to figure out if the column is a LOB
        // column.
        if (CmpCommon::getDefault(JDBC_PROCESS) == DF_ON)
          {
            ItemExpr * childExpr = itemExpr->child(0)->castToItemExpr();
            if (childExpr->getOperatorType() == ITM_BASECOLUMN)
              {
                heading = ((BaseColumn *)childExpr)->getNAColumn()->getHeading();

                if (heading)
                  {
                    if ((strcmp(heading, "JDBC_BLOB_COLUMN -") != 0) &&
                        (strcmp(heading, "JDBC_CLOB_COLUMN -") != 0))
                      heading = NULL;
                  }
              }
          }

        // No heading is passed here (whole point of SQL derived-column is rename)
        // unless it is a jdbc blob/clob heading.
        resultTable->addColumn(bindWA,
                               colRefName,
                               boundValId,
                               renameCol->getTargetColumnClass(),
                               heading);
        break;
      }
      case ITM_ROW_SUBQUERY: 
      case ITM_USER_DEF_FUNCTION: {

        // Deal with multi Valued User Defined Functions or Subqueries with
        // degree > 1.
        //
        // In order to have the correct degree during the bind phase,
        // since we don't have all the information until after the transform
        // phase, we need to put entries into the RETDesc early.
        //
        // Say you have a query like this:
        //    select mvf(a,b) from t1;
        // and assume mvf outputs 2 values.
        //
        // at bind time, the select list will only have 1 entry in it, namely
        // the ITM_USER_DEF_FUNCTION.
        // Since we do degree checking at bind time, we need to know now that
        // mvf() actually produces 2 values.
        //
        // So what we do here, is that we substitute the original 
        // ITM_USER_DEF_FUNCTION with ValueIdProxies. One for each output of
        // the original function. The selectList of the RelRoot as well as the
        // retDESC are updated with the additional elements.
        //
        // Similarly if we have a subquery like this:
        //
        //    select (select max(a),max(b) from t2), a from t1;
        //
        // we will wrap the subquery in a ValeIdProxy representing the 
        // subquery from a transformation point of view, but representing
        // max(a) from an output point of view. A second ValueIdProxy will be
        // added for max(b), so the select list of the outer query would look 
        // like this:
        // 
        //     [ ValueIdProxy(Subq:max(a)), ValueIdProxy(Subq:max(b)), a ]
        //
        // instead of just 
        //    
        //     [ Subq, a ]
        //
        // like we are used to.
        //
        // At transform time the valueIdProxies, will disappear and we will
        // transform the UDF/Subquery carried inside the valueIdProxy
        // marked to be transformed. Some might hang around until Normalization.
        // Only the ValueIdProxy representing the first output will be marked
        // to be transformed, so we only transform the UDF/Subquery once.
        //
        // Similarly, we update the outer query's retDESC.


        NABoolean isSubquery = 
                           (itemExpr->getOperatorType() == ITM_ROW_SUBQUERY) ? 
                           TRUE  : FALSE;

        NAColumnArray outCols;
        ValueIdList outColVids;
        CollIndex currIndex = j;

        if (isSubquery)
        {
           Subquery * subq = (Subquery *) itemExpr;
   
           const RETDesc *retDesc = subq->getSubquery()->getRETDesc();

           if( retDesc )
           {
              retDesc->getColumnList()->getValueIdList(outColVids);
           }
        }
        else
        {
           UDFunction * udf = (UDFunction *) itemExpr;
           CMPASSERT(udf->getRoutineDesc());
           const RoutineDesc *rDesc = udf->getRoutineDesc();

          
           // Get the outputs of this UDF, these are as defined in metadata
           // including names etc.
           outCols = rDesc->getEffectiveNARoutine()->getOutParams();
           outColVids = rDesc->getOutputColumnList();
        }
  
        if ( (outColVids.entries() == 1) || 
             ( isSubquery  &&
               (!allowMultiDegSubqInSelect_Enabled)
             ))
        {
           // Do exactly what we used to do if the degree is 1.
           // or we have disallowed subqueries of degree > 1.

           if (isSubquery)
           {
              // ## Here we ought to manufacture a unique name per Ansi 7.9 SR 9c.
              ColRefName colRefName;
              resultTable->addColumn(bindWA, colRefName, boundValId);
           }
           else
           {
              NAColumn *col = outCols[0];
              const char *  heading = col->getHeading();
              ColRefName colRefName( col->getColName());
              ColumnClass colClass( col->getColumnClass());

              resultTable->addColumn(bindWA, 
                                     colRefName, 
                                     boundValId,
                                     colClass,
                                     heading);
           }
           break;
        }

        // Wrap all the outputs with a ValueIdProxy
        // so that we can deal with multiple outputs
        // If we didn't have a RETDesc or a RoutineDesc, outColVids
        // will be empty and we don't do anything.

        // Also we do not need to worry about recursing through the 
        // RETDesc entries as the call to convertToValueIdList() above
        // did that already.
        for (CollIndex idx = 0; idx < outColVids.entries(); idx++)
        {

           NAColumn *col;

           NABoolean isRealOrRenameColumn = 
                 (outColVids[idx].getItemExpr()->getOperatorType() == 
                  ITM_BASECOLUMN) || 
                 (outColVids[idx].getItemExpr()->getOperatorType() == 
                  ITM_RENAME_COL) ||
                  !isSubquery     ? TRUE : FALSE;

           if (isSubquery)
           {
              col = ((NAColumn *) outColVids[idx].getItemExpr());
           }
           else
           {
              col = ((NAColumn *) outCols[idx]);
           }

           const char *  heading  = isRealOrRenameColumn ?  
                                                col->getHeading() : "";
           ColRefName colRefName( isRealOrRenameColumn ? 
                                                col->getColName() : "");
           ColumnClass colClass( isRealOrRenameColumn ? 
                                           col->getColumnClass() : USER_COLUMN);


           // We are wrapping the MVF/Subquery and its additional outputs
           // with a ValueIdProxy. This way we don't end up flattening or
           // expanding the outputs of the MVF multiple times.

           // The valueId of the RoutineParam corresponding to the 
           // metadata column is used for the output valueId.

           // So if you had a query like this:
           //
           // select swap2(a,b) from t1;
           //
           // and swap2() returns 2 outputs (basically the inputs swapped)
           //
           // The new select list for the query would be:
           //
           // 1: ValueIdProxy with the derivedNode being swap2, and output
           //    valueId containing the first output parameter of swap2.
           //    Also the transformDerivedFrom flag would be set
           // 2: ValueIdProxy with the derivedNode being swap2, and output
           //    valueId containing the second output parameter of swap2.
           //
           // These ValueIdProxy nodes will go away at transform time..

           ValueIdProxy *proxyOutput = new (CmpCommon::statementHeap())
                               ValueIdProxy( boundValId, 
                                             outColVids[idx],
                                             idx);


           // The type of the proxy is the same as the output valueId associated
           // with it.
           proxyOutput = (ValueIdProxy *) proxyOutput->bindNode(bindWA);

           if (bindWA->errStatus()) return NULL;

           // Make sure we transform the MVF
           if (idx == 0) proxyOutput->setTransformChild(TRUE);


           if (!isSubquery || isRealOrRenameColumn) 
           {
              resultTable->addColumn(bindWA, 
                                     colRefName, 
                                     proxyOutput->getValueId(), 
                                     colClass,
                                     heading);
   
           }
           else
           {
              resultTable->addColumn(bindWA, colRefName, 
                                     proxyOutput->getValueId());
           }

           if (idx == 0) 
           {
             vidList.removeAt(currIndex); // we need to delete the old valueId
           }
           else
             j++; // The first entry simply replaces the original
           
           // Update the list with the new value.
           // insertAt has the nice feature that it will push 
           // the residual elements to the right, so we do not need to 
           // manage the valueIds we haven't processed yet as long as we
           // update the index (j++ above) correctly.
           vidList.insertAt(currIndex++,proxyOutput->getValueId());
        }

        break;
      }

      default: 
      {
        // ## Here we ought to manufacture a unique name per Ansi 7.9 SR 9c.
        ColRefName colRefName;
        resultTable->addColumn(bindWA, colRefName, boundValId);
        break;
      }
    } // switch
  } // for

  // need this for static cursor declaration
  cmpCurrentContext->saveRetrievedCols_ = resultTable->getDegree();

  // Before we can return the result table, we need to check for the possible
  // syntax error below, in which we can't use the definition of "inSubquery"
  // that we calculate above.  Our example case is, if we're directly below
  // a GroupByAgg, then we need to look at the scope *before* the GroupByAgg
  // to determine if we satisfy the error condition below.  This is a problem
  // with how our plan trees don't sync completely with SQL syntax.
  // Here's the error case (Genesis 10-980518-0765):
  //
  //   >> select (select distinct 1,2 from T1 t) from T1;
  //
  // First of all, yes, it's a really stupid query.  Oh well!  :-)
  //
  // It's pretty clear that the "1,2" is part of a "select list inside the
  // subquery of a select list."  However, the parser creates a GroupByAgg
  // for the distinct keyword (sigh), which means that we have an
  // additional scope between the scope of the SQLRecord (1,2) and the
  // scope of the "TRUE" parent, the inner-select.  This additional scope
  // is for the GroupByAgg.  So in the case of a GroupByAgg (and possibly
  // another case will arise later ...?), we need to look at the
  // GroupByAgg's parent to determine if we satisfy this error condition.
  //
  // To recap: To handle this one (stupid) case we've added a ton of
  // comments and code here and in GroupByAgg::bindNode(), plus created
  // the new functions/members BindWA::getSubqueryScope(), and
  // BindContext::lookAboveToDecideSubquery_/().  Wonderful!
  //
  if (prevScope) {

    BindScope *subQScope = bindWA->getSubqueryScope(currScope);
    //
    // subQScope should be non-NULL when prevScope is non-NULL
    //
    CMPASSERT(subQScope);

    NABoolean inSubqueryInSelectList = subQScope->context()->inSubquery() &&
                                       subQScope->context()->inSelectList();

    NABoolean inSubqueryInGroupByClause = subQScope->context()->inSubquery() &&
                   subQScope->context()->inGroupByClause() && 
                   (CmpCommon::getDefault(UDF_SUBQ_IN_AGGS_AND_GBYS) == DF_ON);

    //10-060602-6930 Begin
    //Added a check to not enter this condition when we are in bindView scope
    if (inSelectList && 
        (inSubqueryInSelectList || 
        inSubqueryInGroupByClause) && 
        !bindWA->inViewExpansion()) {
      //10-060602-6930 End
      // We now can check for the syntax error that we've done so much work
      // above (and in GroupByAgg::bindNode(), BindWA.h & BindWA.cpp)
      // to detect:

      if ((j > 1) && 
          (!allowMultiDegSubqInSelect_Enabled) ) {
        // 4019 The select list of a subquery in a select list must be scalar
        *CmpCommon::diags() << DgSqlCode(-4019);
        bindWA->setErrStatus();
        return NULL;
      }
    }
  } // prevScope

  return resultTable;
} // bindRowValues()

// Bind a constraint (MP Check Constraint).
// Returns NULL if error in constraint *OR* we can safely ignore the constraint
// (e.g., a NOT NULL NONDROPPABLE constraint); caller must check bindWA errsts.
//
static ItemExpr* bindCheckConstraint(
  BindWA *bindWA,
  CheckConstraint *constraint,
  const NATable *naTable,
  NABoolean catmanCollectUsages = FALSE,
  ItemExpr *viewCheckPred = NULL)
{
  ItemExpr *constraintPred = NULL;
  if (viewCheckPred) {
    // view WITH CHECK OPTION: the view's where-clause was already parsed
    // in bindView
    CMPASSERT(constraint->getConstraintText().isNull());   // sanity check
    constraintPred = viewCheckPred;
  }
  else {

    Parser parser(bindWA->currentCmpContext());
    constraintPred = parser.getItemExprTree(constraint->getConstraintText().data(),
                                            constraint->getConstraintText().length(),
                                            CharInfo::UTF8 // ComGetNameInterfaceCharSet()
                                            );

  }
  if (constraintPred) {
    ParNameLocList *saveNameLocList = bindWA->getNameLocListPtr();
    if (!catmanCollectUsages ||
        !bindWA->getUsageParseNodePtr() ||
        bindWA->getUsageParseNodePtr()->getOperatorType() == DDL_CREATE_VIEW)
      bindWA->setNameLocListPtr(NULL);

    CMPASSERT(!bindWA->getCurrentScope()->context()->inCheckConstraint());
    bindWA->getCurrentScope()->context()->inCheckConstraint() = constraint;

    constraintPred->bindNode(bindWA);
    bindWA->setNameLocListPtr(saveNameLocList);
    bindWA->getCurrentScope()->context()->inCheckConstraint() = NULL;

    if (bindWA->errStatus()) {
      delete constraintPred;
      constraintPred = NULL;
    }
  }
  // A NOT NULL constraint on a single column which never allows nulls
  // (has no null indicator bytes)
  // -- i.e., the common case of a column declared NOT NULL NONDROPPABLE --
  // does not need to be separately enforced as a constraint, because
  // Executor will raise a numeric-overflow error if someone tries to
  // put a NULL into such a column.
  //
  // So we don't need to put this constraint into the list, but we do need
  // to save its name, for run-time error diags.
  //
  // ##To be done:
  // ## GenRelUpdate DP2Insert/Update: for each col in newRecExpr(),
  // ##   if getNotNullViolationCode(), then
  // ##     save the SqlCode and the getNotNullConstraintName()...asAnsiString()
  // ##     and some column identifier (pos or offset) in some per-query struct
  // ## Executor: if error 8411, if truly a NULL violation, look up that column
  // ##   in the nnconstraint struct and populate diags with the info there.
  //
  if (constraintPred) {
    ItemExprList nncols(bindWA->wHeap());
    constraintPred->getColumnsIfThisIsISNOTNULL(nncols);
    for (CollIndex i = 0; i < nncols.entries(); i++) {
      NAColumn *nacol = nncols[i]->getValueId().getNAColumn();
      if (!nacol->getType()->supportsSQLnullPhysical()) {
        nacol->setNotNullNondroppable(constraint);
          //
          // DO *NOT* do:   delete constraintPred;
          // -- it deletes a whole tree of stuff referenced elsewhere!
          //
        constraintPred = NULL;
      } else {
        // Leaving the column's type's supportsSQLnullPhysical() as is (TRUE),
        // set its supportsSQLnullLogical() to FALSE,
        // for the Transform phase.
        nacol->mutateType()->setNullable(TRUE/*supports physical nulls*/,
                                         FALSE/*but not logical nulls */);
      }
    }
  }
  else {
    *CmpCommon::diags() << DgSqlCode(-4025)
      << DgConstraintName(ToAnsiIdentifier(constraint->getConstraintName().getObjectName()))
      << DgTableName(naTable->getTableName().getQualifiedNameAsAnsiString());
    bindWA->setErrStatus();
  }
  return constraintPred;
} // bindCheckConstraint()

static ItemExpr *intersectColumns(const RETDesc &leftTable,
                                  const RETDesc &rightTable,
                                  BindWA* bindWA)
{
  ItemExpr *predicate = NULL;
  for (CollIndex i = 0; i < leftTable.getDegree(); i++) {
    ItemExpr *leftExpr  = leftTable.getValueId(i).getItemExpr();
    ItemExpr *rightExpr = rightTable.getValueId(i).getItemExpr();
    BiRelat *compare = new (bindWA->wHeap())
      BiRelat(ITM_EQUAL, leftExpr, rightExpr, TRUE);
    if (predicate)
      predicate = new (bindWA->wHeap()) BiLogic(ITM_AND, predicate, compare);
    else
      predicate = compare;
  }
  // Binding this predicate must be done in caller's context/scope, not here...
  return predicate;
} // intersectColumns()

static ItemExpr *joinCommonColumns(const RelExpr *const leftRelExpr,
                                   const RelExpr *const rightRelExpr,
                                   BindWA* bindWA)
{
  const RETDesc &leftTable = *leftRelExpr->getRETDesc();
  const RETDesc &rightTable = *rightRelExpr->getRETDesc();
  //
  // Find the common column names between two tables and create a predicate
  // that joins the columns.  For example, if tables T1 and T2 have common
  // column names A and B, return the predicate T1.A = T2.A AND T1.B = T2.B.
  // The checking for ambiguous common columns will be done when they are
  // are coalesced for the output list.
  //
  ItemExpr *predicate = NULL;
  for (CollIndex i = 0; i < leftTable.getDegree(); i++) {
    ColRefName simpleColRefName(leftTable.getColRefNameObj(i).getColName()); //
    if (NOT simpleColRefName.isEmpty()) {                                    //
      ColumnNameMap *commonCol = rightTable.findColumn(simpleColRefName);    //
      if (commonCol) {                                                       //
        ItemExpr *leftExpr  = leftTable.getValueId(i).getItemExpr();
        ItemExpr *rightExpr = commonCol->getValueId().getItemExpr();         //

        bindWA->markAsReferencedColumn(leftExpr);

        bindWA->markAsReferencedColumn(rightExpr);

        BiRelat *compare = new (bindWA->wHeap())
        BiRelat(ITM_EQUAL, leftExpr, rightExpr);
        if (predicate)
          predicate = new(bindWA->wHeap()) BiLogic(ITM_AND, predicate, compare);
        else
          predicate = compare;
      }
    }
  }
  // Binding this predicate is being done in caller, Join::bindNode()
  return predicate;
} // joinCommonColumns()

// Functions findNonCommonColumns() and coalesceCommonColumns()
//
// These create the column descriptors for the result of a natural join.
// A natural join is equivalent to
//
// SELECT SLCC, SLT1, SLT2 FROM T1, T2
//
// where SLCC represents the list of coalesced common columns of T1 and T2,
//       SLT1 represents the list of non-common columns of T1, and
//       SLT2 represents the list of non-common columns of T2.
//
// A coalesced common column C is equivalent to
//
// COALESCE (T1.C, T2.C) AS C  -- i.e. there is no table name; CorrName is ""
//
// where COALESCE (T1.C, T2.C) is equivalent to
//
// CASE WHEN T1.C IS NOT NULL THEN T1.C ELSE T2.C END
//
// Function findNonCommonColumns(), on the first call, coalesces common
// columns into the resultTable, and collects non-common columns.
// On the second call it continues to collect non-common columns.
//
// Function coalesceCommonColumns() adds SLCC, SLT1, SLT2 to the
// resultTable in the proper order.
//
static void findNonCommonColumns(BindWA *bindWA,
                                 OperatorTypeEnum joinType,
                                 const RETDesc &sourceTable,
                                 const RETDesc &targetTable,
                                 RETDesc &resultTable,
                                 ColumnDescList &nonCommonCols)
{
  // Used for ANSI 6.4 SR 3aii below.
  CorrName implemDependCorr(bindWA->fabricateUniqueName(), TRUE);
  //
  for (CollIndex i = 0; i < sourceTable.getDegree(); i++) {
    const ColRefName &sourceColRefName = sourceTable.getColRefNameObj(i);
    ValueId sourceId = sourceTable.getValueId(i);
    ColRefName simpleColRefName(sourceColRefName.getColName());
    //
    // If a column is an unnamed expression, it is a non-common column.
    //
    if (simpleColRefName.isEmpty())
      nonCommonCols.insert(new (bindWA->wHeap())
               ColumnDesc(sourceColRefName, sourceId, NULL, bindWA->wHeap()));
    else {
      ColumnNameMap *commonCol = targetTable.findColumn(simpleColRefName);
      //
      // If the named column does not have a corresponding column in the
      // target table, it is a non-common column.
      //
      if (NOT commonCol)
        nonCommonCols.insert(new (bindWA->wHeap())
                ColumnDesc(sourceColRefName, sourceId, NULL, bindWA->wHeap()));
      //
      // If the target table has more than one corresponding column, error.
      //
      else if (commonCol->isDuplicate()) {
        NAString fmtdList(bindWA->wHeap());
        LIST(TableNameMap*) xtnmList(bindWA->wHeap());
        targetTable.getTableList(xtnmList, &fmtdList);   // Tables in the RETDesc

        *CmpCommon::diags() << DgSqlCode(-4004)
          << DgColumnName(simpleColRefName.getColName())
          << DgTableName(commonCol->getColRefNameObj().getCorrNameObj().
                                    getExposedNameAsAnsiString())
          << DgString0(fmtdList)
          << DgString1(bindWA->getDefaultSchema().getSchemaNameAsAnsiString());

        bindWA->setErrStatus();
        return;
      }
      else if (joinType != ITM_NO_OP) {
        //
        // Coalesce the common columns and add them to the result table.
        //
        ValueId resultId;
        switch(joinType) {
        case REL_JOIN:
        case REL_LEFT_JOIN:
          resultId = sourceId;
          break;
        case REL_RIGHT_JOIN:
          resultId = commonCol->getValueId();
          break;
        default: {
          ItemExpr *sourceExpr = sourceId.getItemExpr();
          ItemExpr *targetExpr = commonCol->getValueId().getItemExpr();
          UnLogic *test = new (bindWA->wHeap())
          UnLogic(ITM_IS_NULL, sourceExpr);
          ItemExpr *coalesce = new (bindWA->wHeap())
                                 Case(NULL, new (bindWA->wHeap())
                                   IfThenElse(test,
                                              targetExpr,
                                              sourceExpr));
          coalesce = coalesce->bindNode(bindWA)->castToItemExpr();
          if (bindWA->errStatus()) {
            delete test;
            delete coalesce;
            return;
          }
          resultId = coalesce->getValueId();
          break;
        } // default case (braces required since vars are initialized here)
        } // switch
        //
        // ANSI 6.4 SR 3aii:
        // We've fabricated a unique implementation-dependent CorrName
        // outside the loop; the common columns have this basically
        // invisible CorrName, the point of which seems to be that
        //   select * from
        //       ta natural join tb
        //     join			-- not natural!
        //      (ta tx natural join tb ty)
        //     on 1=1;
        // should not generate an ambiguous column reference error
        // from the star-expansion.  So according to ANSI,
        // the two natural joins produce, respectively,
        //   fab1.slcc, ta.slt1, tb.slt2
        //   fab2.slcc, tx.slt1, ty.slt2
        // so the join produces
        //   fab1.slcc, ta.slt1, tb.slt2, fab2.slcc, tx.slt1, ty.slt2
        // i.e. the two SLCC's are unambiguous.
        //
        ColRefName implemDepend(simpleColRefName.getColName(),implemDependCorr);
        resultTable.addColumn(bindWA, implemDepend, resultId);
      } // coalesce SLCC into resultTable
    } // named column
  } // for
} // findNonCommonColumns()

// Comments for this function can be found above the preceding function.
static void coalesceCommonColumns(BindWA *bindWA,
                                  OperatorTypeEnum joinType,
                                  const RETDesc &leftTable,
                                  const RETDesc &rightTable,
                                  RETDesc &resultTable)
{
  ColumnDescList nonCommonCols(bindWA->wHeap());
  // non-common columns of the left table
  //
  // Coalesce the common column names of the left and right tables and add
  // them to the result table.
  // Collect the non-common column names from the left.
  //
  findNonCommonColumns(bindWA,
    joinType,
    leftTable,
    rightTable,
    resultTable,
    nonCommonCols);
  if (bindWA->errStatus()) return;
  //
  // Collect the non-common column names from the right.
  //
  RETDesc irrelevantOnThisCall;
  findNonCommonColumns(bindWA,
    ITM_NO_OP,      // do not add SLCC to resultTable
    rightTable,
    leftTable,
    irrelevantOnThisCall,
    nonCommonCols);
  if (bindWA->errStatus()) return;
  //
  // Add the non-common columns from the left and right to the result table.
  //
  resultTable.addColumns(bindWA, nonCommonCols);
  nonCommonCols.clearAndDestroy();
  //
  // Add the system columns from the left and right to the result table.
  //
  resultTable.addColumns(bindWA, *leftTable.getSystemColumnList(), SYSTEM_COLUMN);
  resultTable.addColumns(bindWA, *rightTable.getSystemColumnList(), SYSTEM_COLUMN);
} // coalesceCommonColumns()

// For Catalog Manager, this function:
// 1) Fixes up the name location list to help with computing of the view text,
//    check constraint search condition text, etc.
// 2) Collects the table (base table, view, etc.) usages information for
//    view definitions, check constraint definitions, etc.
//
// ** Some of this could be implemented, perhaps more simply,
// ** using BindWA::viewCount() and BindWA::tableViewUsageList().
//
static void BindUtil_CollectTableUsageInfo(BindWA *bindWA,
                                           const CorrName& corrName)
{
  // Task (1)
  //
  ParNameLocList *pNameLocList = bindWA->getNameLocListPtr();
  if (pNameLocList)
  {
    ParNameLoc * pNameLoc
      = pNameLocList->getNameLocPtr(corrName.getNamePosition());
    if (pNameLoc)
    {
      if (NOT pNameLoc->getExpandedName(FALSE).isNull())
        CMPASSERT(pNameLoc->getExpandedName() ==
          corrName.getQualifiedNameObj().getQualifiedNameAsAnsiString());
      pNameLoc->setExpandedName(
                corrName.getQualifiedNameObj().getQualifiedNameAsAnsiString());
    }
    //
    // Task (2)
    //
    ExprNode *pUsageParseNode = bindWA->getUsageParseNodePtr();
    if (pUsageParseNode)
    {
      if (pUsageParseNode->getOperatorType() == DDL_CREATE_VIEW)
      {
        StmtDDLCreateView &cvpn = *pUsageParseNode->castToElemDDLNode()
          ->castToStmtDDLCreateView();
        ParTableUsageList &vtul = cvpn.getViewUsages().getViewTableUsageList();
        vtul.insert(corrName.getExtendedQualNameObj());
      }
      else if (pUsageParseNode->getOperatorType()
               == DDL_ALTER_TABLE_ADD_CONSTRAINT_CHECK)
      {
        StmtDDLAddConstraintCheck &node = *pUsageParseNode->castToElemDDLNode()
           ->castToStmtDDLAddConstraintCheck();
        ParTableUsageList &tul = node.getTableUsageList();
        tul.insert(corrName.getQualifiedNameObj());
      }
    }
  } // if (pNameLocList)

} // BindUtil_CollectTableUsageInfo()

void castComputedColumnsToAnsiTypes(BindWA *bindWA,
                                    RETDesc *rd,
                                    ValueIdList &compExpr)
{
  if (! rd)
    return;

  const ColumnDescList &cols = *rd->getColumnList();
  CollIndex i = cols.entries();
  CMPASSERT(i == compExpr.entries());

  while (i--) {
    ColumnDesc *col = cols[i];

    if (col->getValueId().getType().getTypeQualifier() == NA_ROWSET_TYPE) {
      return;
    }
    NAType *naType = &(NAType&)col->getValueId().getType();
    //
    // Note: the unsupported and DATETIME cases are mutually exclusive with the LARGEDEC case below.
    //
    if (!naType->isSupportedType()) {
      // Unsupported types are displayed as strings of '#' to their display length
      ItemExpr *theRepeat =
        new (bindWA->wHeap()) Repeat(new (bindWA->wHeap()) SystemLiteral("#"),
                                       new (bindWA->wHeap()) SystemLiteral(
                                            naType->getDisplayLength(
                                                 naType->getFSDatatype(),
                                                 0,
                                                 naType->getPrecision(),
                                                 naType->getScale(),
                                                 0)));
      theRepeat = theRepeat->bindNode(bindWA);
      col->setValueId(theRepeat->getValueId());
      compExpr[i] = theRepeat->getValueId();
    }
    else if ((CmpCommon::getDefault(MODE_SPECIAL_1) == DF_ON) &&
             (NOT bindWA->inViewDefinition()) &&
             (NOT bindWA->inMVDefinition()) &&
             (NOT bindWA->inCTAS()) &&
             (naType->getTypeQualifier()== NA_DATETIME_TYPE &&
              ((const DatetimeType *)naType)->getSubtype() == 
              DatetimeType::SUBTYPE_SQLDate) &&
             (! CmpCommon::context()->getSqlmxRegress()) &&
             (strcmp(ActiveSchemaDB()->getDefaults().getValue(OUTPUT_DATE_FORMAT),
               "ANSI") != 0))
      { // Special1 DATE, return as YY/MM/DD
        ItemExpr * newChild =
          new (bindWA->wHeap())
          Format(col->getValueId().getItemExpr(), "YY/MM/DD", FALSE);
        newChild = newChild->bindNode(bindWA);
        col->setValueId(newChild->getValueId());
        compExpr[i] = newChild->getValueId();
      }

    if ((naType->getFSDatatype() == REC_BIN64_UNSIGNED) &&
        (CmpCommon::getDefault(TRAF_LARGEINT_UNSIGNED_IO) == DF_OFF) &&
        (NOT bindWA->inCTAS()) &&
        (NOT bindWA->inViewDefinition()))
      {
        NumericType *nTyp = (NumericType *)naType;
        
        ItemExpr * cast = new (bindWA->wHeap())
          Cast(col->getValueId().getItemExpr(),
               new (bindWA->wHeap())
               SQLBigNum(bindWA->wHeap(), MAX_HARDWARE_SUPPORTED_UNSIGNED_NUMERIC_PRECISION,
                         nTyp->getScale(),
                         FALSE,
                         FALSE,
                         naType->supportsSQLnull()
                         ));
        
        cast = cast->bindNode(bindWA);
        if (bindWA->errStatus()) 
          return;
        col->setValueId(cast->getValueId());
        compExpr[i] = cast->getValueId();

        naType = (NAType*)&cast->getValueId().getType();
      }

    if ((naType->getFSDatatype() == REC_BOOLEAN) &&
        (CmpCommon::getDefault(TRAF_BOOLEAN_IO) == DF_OFF) &&
        (NOT bindWA->inCTAS()) &&
        (NOT bindWA->inViewDefinition()))
      {
        NumericType *nTyp = (NumericType *)naType;
        
        ItemExpr * cast = new (bindWA->wHeap())
          Cast(col->getValueId().getItemExpr(),
               new (bindWA->wHeap())
               SQLChar(bindWA->wHeap(), SQL_BOOLEAN_DISPLAY_SIZE, naType->supportsSQLnull()));
        
        cast = cast->bindNode(bindWA);
        if (bindWA->errStatus()) 
          return;
        col->setValueId(cast->getValueId());
        compExpr[i] = cast->getValueId();
        
        naType = (NAType*)&cast->getValueId().getType();
      }
    
    if ((DFS2REC::isBinaryString(naType->getFSDatatype())) &&
        (CmpCommon::getDefault(TRAF_BINARY_OUTPUT) == DF_OFF) &&
        (NOT bindWA->inCTAS()) &&
        (NOT bindWA->inViewDefinition()))
      {
        ItemExpr * cast = NULL;
        cast = new (bindWA->wHeap()) 
          ConvertHex(ITM_CONVERTTOHEX, col->getValueId().getItemExpr());

        cast = cast->bindNode(bindWA);
        if (bindWA->errStatus()) 
          return;
        col->setValueId(cast->getValueId());
        compExpr[i] = cast->getValueId();
        
        naType = (NAType*)&cast->getValueId().getType();
      }
    
    // if OFF, return tinyint as smallint.
    // This is needed until all callers/drivers have full support to
    // handle IO of tinyint datatypes.
    if (((naType->getFSDatatype() == REC_BIN8_SIGNED) ||
         (naType->getFSDatatype() == REC_BIN8_UNSIGNED)) &&
        (NOT bindWA->inCTAS()) &&
        (NOT bindWA->inViewDefinition()) &&
        ((CmpCommon::getDefault(TRAF_TINYINT_SUPPORT) == DF_OFF) ||
         (CmpCommon::getDefault(TRAF_TINYINT_RETURN_VALUES) == DF_OFF)))
      {
        NumericType *srcNum = (NumericType*)naType; 
        NumericType * newType;
        if (srcNum->getScale() == 0)
          newType = new (bindWA->wHeap())
            SQLSmall(bindWA->wHeap(), NOT srcNum->isUnsigned(),
                     naType->supportsSQLnull());
        else
          newType = new (bindWA->wHeap())
            SQLNumeric(bindWA->wHeap(), sizeof(short), srcNum->getPrecision(), 
                       srcNum->getScale(),
                       NOT srcNum->isUnsigned(), 
                       naType->supportsSQLnull());

        ItemExpr * cast = new (bindWA->wHeap())
          Cast(col->getValueId().getItemExpr(), newType);

        cast = cast->bindNode(bindWA);
        if (bindWA->errStatus()) 
          return;
        col->setValueId(cast->getValueId());
        compExpr[i] = cast->getValueId();
      }
    else if (naType->getTypeQualifier() == NA_NUMERIC_TYPE && 
             !((NumericType &)col->getValueId().getType()).binaryPrecision()) {
      NumericType *nTyp = (NumericType *)naType;
      
      ItemExpr * ie = col->getValueId().getItemExpr();
      NAType *newTyp = NULL;
      Lng32 newPrec;
      Lng32 newScale;
      Lng32 oflow = -1;
      Lng32 bignumOflow = -1;
      NABoolean bignumIO = FALSE;
      if (CmpCommon::getDefault(BIGNUM_IO) == DF_ON)
        bignumIO = TRUE; // explicitely set to ON
      else if (CmpCommon::getDefault(BIGNUM_IO) == DF_OFF)
        bignumIO = FALSE; // explicitely set to OFF
      else if (CmpCommon::getDefault(BIGNUM_IO) == DF_SYSTEM)
        {
          if ((nTyp->isBigNum()) &&
              (((SQLBigNum*)nTyp)->isARealBigNum()))
            bignumIO = TRUE;
        }

      if (CmpCommon::getDefaultNumeric(MAX_NUMERIC_PRECISION_ALLOWED) ==
          MAX_HARDWARE_SUPPORTED_SIGNED_NUMERIC_PRECISION)
        bignumIO = FALSE;

      if (bignumIO)
        bignumOflow = nTyp->getPrecision() - 
          (Lng32)CmpCommon::getDefaultNumeric(MAX_NUMERIC_PRECISION_ALLOWED);
      else
        {
          if (nTyp->isSigned())
            oflow = nTyp->getPrecision() - MAX_HARDWARE_SUPPORTED_SIGNED_NUMERIC_PRECISION;
          else
            oflow = nTyp->getPrecision() - MAX_HARDWARE_SUPPORTED_UNSIGNED_NUMERIC_PRECISION;
        }

      if ((bignumOflow > 0) || (oflow > 0))
        {
          if (bignumOflow > 0) {
            newPrec = 
              (Lng32)CmpCommon::getDefaultNumeric(MAX_NUMERIC_PRECISION_ALLOWED);
            Lng32 orgMagnitude = nTyp->getPrecision() - nTyp->getScale();
        
            // set the newScale
            // IF there is overflow in magnitude set the scale to 0.
            // ELSE set the accomodate the magnitude part and truncate the scale
            newScale = (orgMagnitude >= newPrec) ? 0 : newPrec - orgMagnitude ;

            if (newScale > newPrec)
              {
                *CmpCommon::diags() << DgSqlCode(-3015) 
                                    << DgInt0(newScale) << DgInt1(newPrec);

                bindWA->setErrStatus();
                return;
              }

            newTyp = new (bindWA->wHeap())
              SQLBigNum(bindWA->wHeap(), newPrec,
                        newScale,
                        ((SQLBigNum &)col->getValueId().getType()).isARealBigNum(),
                        nTyp->isSigned(),
                        nTyp->supportsSQLnull());
          }
          else if (oflow > 0) {
            // If it's not a computed expr, but a column w/ a legal type, re-loop
            if (col->getValueId().getNAColumn(TRUE/*don't assert*/)) {
              //CMPASSERT(!nTyp->isInternalType());
              //continue;
            }
            
            OperatorTypeEnum op = ie->origOpType();
            CMPASSERT(op != NO_OPERATOR_TYPE &&      // Init'd correctly?
                      op != ITM_RENAME_COL &&      // Expect these to have
                      op != ITM_REFERENCE);      // been bound, vanished.
            
            ItemExpr *ie2 = ie;
	    
            while (op == ITM_INSTANTIATE_NULL)
            {
              ie2 = ie2->child(0).getPtr();
              op = ie2->origOpType();
            }

            // ANSI 6.5 SR 7 - 9:  aggregates must be exact if column is exact.
            newPrec  = MAX_NUMERIC_PRECISION;
            Lng32 orgMagnitude = (nTyp->getMagnitude() + 9) / 10;
            // set the newScale
            // IF there is overflow in magnitude set the scale to 0.
            // ELSE set the accomodate the magnitude part and truncate the scale
            newScale = (orgMagnitude >= newPrec) ? 0 : newPrec - orgMagnitude ;
            
            // Based on the CQD set the scale to MIN value.
            // CQD specifies the MIN scale that has to be preserved in case
            // of overflow.
            NADefaults &defs = ActiveSchemaDB()->getDefaults();
            Lng32 minScale = defs.getAsLong(PRESERVE_MIN_SCALE);
            newScale = MAXOF(minScale, newScale);
            
            if (op == ITM_SUM || op == ITM_AVG) {
              // AVG = DIVIDE( SUM(), COUNT() )
              ItemExpr *tmp = (op == ITM_SUM) ?
                ie2 : ie2->child(0).getPtr();

              //
              // Now that we support OLAP functions, this may be
              // a pointer to an ITM_NOTCOVERED node.  If so, we
              // need to check its child(0) node rather than 
              // the ITM_NOTCOVERED node.
              //
              if (tmp->getOperatorType() == ITM_NOTCOVERED )
                  tmp = (Aggregate *)(ItemExpr *)tmp->child(0);

              CMPASSERT(tmp->isAnAggregate());
              Aggregate *sum = (Aggregate *)tmp;
              ItemExpr *arg = (sum->getOriginalChild()) ?
                sum->getOriginalChild()  : sum->child(0).getPtr();
              if (arg->getValueId() == NULL_VALUE_ID)
                arg = sum->child(0).getPtr();
              CMPASSERT(arg->getValueId() != NULL_VALUE_ID);
              Lng32 needScale = arg->getValueId().getType().getScale();
              if (needScale > newPrec)
                needScale = newPrec;
              if (newScale < needScale || op == ITM_SUM) // ANSI 6.5 SR 9 b + c
                newScale = needScale;
            }
            
            if (newScale == 0)
              newTyp = new (bindWA->wHeap())
                         SQLLargeInt(bindWA->wHeap(), TRUE, // hardware only supports signed
                                     nTyp->supportsSQLnull());
            else
              newTyp = new (bindWA->wHeap())
                         SQLNumeric(bindWA->wHeap(), sizeof(Int64),
                                    newPrec,
                                    newScale,
                                    nTyp->isSigned(),
                                    nTyp->supportsSQLnull());
            
          } // overflow
          
          ItemExpr *cast = new (bindWA->wHeap())
                             Cast(ie, newTyp, ITM_CAST, TRUE/*checkForTrunc*/);
          cast = cast->bindNode(bindWA);
          if (bindWA->errStatus()) return;
          
          if (!col->getColRefNameObj().getColName().isNull()) {
            // We get here via CREATE VIEW v AS SELECT (expr op expr) AS nam ...;
            
            // ColumnDesc::setValueId() makes the RETDesc's XCNM inconsistent --
            // but this is ok because name lookup over this XCNM doesn't happen
            // after the point we've gotten to here --
            // a) if caller is StmtDDLCreateView::bindNode via RelRoot::bindNode,
            //    there's no further lookup at all;
            // b) if caller is bindView(), then thanks to the way RenameTable
            //    and RETDesc work, the inconsistent XCNM is not consulted
            //    so we don't have to worry about this issue ... (for now anyhow!)
          }
          col->setValueId(cast->getValueId());
          compExpr[i] = cast->getValueId();
        } // overflow (bignum or regular)
    }   // numeric
  }   // loop over cols in RETDesc
}   // castComputedColumnsToAnsiTypes()

TrafDesc *generateSpecialDesc(const CorrName& corrName)
{
  TrafDesc * desc = NULL;

  if (corrName.getSpecialType() == ExtendedQualName::VIRTUAL_TABLE)
    {
      if (corrName.getQualifiedNameObj().getObjectName() == ExplainFunc::getVirtualTableNameStr())
        {
          ExplainFunc ef;
          desc = ef.createVirtualTableDesc();
        }
      else if (corrName.getQualifiedNameObj().getObjectName() == StatisticsFunc::getVirtualTableNameStr())
        {
          StatisticsFunc sf;
          desc = sf.createVirtualTableDesc();
        }
      else if (corrName.getQualifiedNameObj().getObjectName() == ExeUtilRegionStats::getVirtualTableNameStr())
        {
          ExeUtilRegionStats eudss;
          desc = eudss.createVirtualTableDesc();
        }
      else if (corrName.getQualifiedNameObj().getObjectName() == ExeUtilRegionStats::getVirtualTableClusterViewNameStr())
        {
          ExeUtilRegionStats eudss(TRUE);
          desc = eudss.createVirtualTableDesc();
        }
      else if (HiveMDaccessFunc::isHiveMD(corrName.getQualifiedNameObj().getObjectName()))
        {
          NAString mdType = 
            HiveMDaccessFunc::getMDType(corrName.getQualifiedNameObj().getObjectName());

          HiveMDaccessFunc hivemd(&mdType);
          desc = hivemd.createVirtualTableDesc();
        }
    }

  return desc;
} // generateSpecialDesc()


// -----------------------------------------------------------------------
// member functions for class BindWA
// -----------------------------------------------------------------------
NARoutine *BindWA::getNARoutine ( const QualifiedName &name )
{
  NARoutineDBKey key(name, wHeap());
  NARoutine * naRoutine = getSchemaDB()->getNARoutineDB()->get(this, &key);
  if (!naRoutine)
  {
     TrafDesc *udfMetadata = NULL;
     CmpSeabaseDDL cmpSBD(STMTHEAP);
     udfMetadata =  cmpSBD.getSeabaseRoutineDesc(
				       name.getCatalogName(),
				       name.getSchemaName(),
				       name.getObjectName());
     if (!udfMetadata)
       return NULL;

     NAHeap *routineHeap;
     if (getSchemaDB()->getNARoutineDB()->cachingMetaData()) 
     {
       const Lng32 size = 16 * 1024;  // The initial size
       routineHeap = new CTXTHEAP NAHeap("NARoutine Heap", (NAHeap *)CTXTHEAP, 
                                         size);
     }
     else 
       routineHeap=CmpCommon::statementHeap(); 

     Int32 errors=0;
     naRoutine = new (routineHeap)
       NARoutine(name,
                 udfMetadata, 
                 this,
                 errors,
                 routineHeap);
     if ( NULL == naRoutine || errors != 0)
     {
       setErrStatus();
       return NULL;
     }
     
     // Add NARoutine to the NARoutineDB cache.
     if (getSchemaDB()->getNARoutineDB()->cachingMetaData()) 
       getSchemaDB()->getNARoutineDB()->put(naRoutine);
  }
  return naRoutine;
}

NATable *BindWA::getNATable(CorrName& corrName,
                            NABoolean catmanCollectTableUsages, // default TRUE
                            TrafDesc *inTableDescStruct)     // default NULL
{
  BindWA *bindWA = this;   // for coding convenience

  NATable * table = NULL;
  // Search in volatile schema first. If not found, search in regular cat/sch.
  NABoolean volatileTableFound = FALSE;
  NAString userName;
  if ((CmpCommon::context()->sqlSession()->volatileSchemaInUse()) &&
      (! inTableDescStruct) &&
      (corrName.getSpecialType() != ExtendedQualName::VIRTUAL_TABLE) &&
      (corrName.getSpecialType() != ExtendedQualName::HBMAP_TABLE))
    {
      CorrName newCorrName = 
        CmpCommon::context()->sqlSession()->getVolatileCorrName
          (corrName);
      if (bindWA->errStatus())
        return NULL;

      //get NATable from cache
      table = bindWA->getSchemaDB()->getNATableDB()->
        get(newCorrName, bindWA, inTableDescStruct);
      if (!table)
        {
          // now search in regular cat/sch.
          // clear diags area.
          CmpCommon::diags()->clear();
          bindWA->resetErrStatus();
        }
      else
        {
          NABoolean isValid = 
            CmpCommon::context()->sqlSession()->validateVolatileCorrName
            (corrName);

          // if this table is found in volatile schema, then
          // make sure it is a volatile table.
          if ((isValid) &&
              (NOT table->isVolatileTable()))
            {
              *CmpCommon::diags() << DgSqlCode(-4190) <<
                                     DgTableName(table->getTableName().
                                       getQualifiedNameAsAnsiString(TRUE));
              
              bindWA->setErrStatus();
              return NULL;
            }

          if (isValid)
            {
              newCorrName.setIsVolatile(TRUE);
              corrName = newCorrName;
            }
          else
            {
              // table was found in the volatile schema but it is
              // not a valid volatile name.
              // Look for it in regular schema.
              table = NULL;

              CmpCommon::diags()->clear();
              bindWA->resetErrStatus();

              // remember that volatile table was found so we
              // can generate a better error message later.
              volatileTableFound = TRUE;
            }
        }
    }

  if (! table)
    {
      // Expand the table (base table, view, etc.) name with
      // the default catalog and schema parts if the specified
      // table name does not include these parts.
      // This method will also first apply any prototype value (from a host var)
      // into the corrName's qualifiedName.
      //

      NABoolean catNameSpecified =
              (NOT corrName.getQualifiedNameObj().getCatalogName().isNull());
      NABoolean schNameSpecified =
              (NOT corrName.getQualifiedNameObj().getSchemaName().isNull());

     // try PUBLIC SCHEMA only when no schema was specified
      // and CQD PUBLIC_SCHEMA_NAME is specified
      NAString publicSchema = "";
      CmpCommon::getDefault(PUBLIC_SCHEMA_NAME, publicSchema, FALSE);
      ComSchemaName pubSchema(publicSchema);
      NAString pubSchemaIntName = "";
      if ( !schNameSpecified && !pubSchema.getSchemaNamePart().isEmpty() )
      {
        pubSchemaIntName = pubSchema.getSchemaNamePart().getInternalName();
      }

      corrName.applyDefaults(bindWA, bindWA->getDefaultSchema());
      if (bindWA->errStatus())
        return NULL;      // prototype value parse error

      // cannot use hbase map schema as table name
      if ((corrName.getQualifiedNameObj().getSchemaName() == HBASE_EXT_MAP_SCHEMA) &&
          (! Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)) &&
          (! Get_SqlParser_Flags(ALLOW_SPECIALTABLETYPE)))
         {
          *CmpCommon::diags() << DgSqlCode(-4261)
                              << DgSchemaName(corrName.getQualifiedNameObj().getSchemaName());
          
          bindWA->setErrStatus();
          return NULL;
        }

      // if this is an HBase mapped table and schema name is not specified, 
      // then set schema to hbase map schema.
      if ((corrName.getSpecialType() == ExtendedQualName::HBMAP_TABLE) &&
          (NOT schNameSpecified))
        {
          corrName.getQualifiedNameObj().setSchemaName(HBASE_EXT_MAP_SCHEMA);
        }
 
      // override schema
      if ( ( overrideSchemaEnabled() )
           // not volatile table
        && ( ! volatileTableFound ) 
        )
      {
        doOverrideSchema(corrName);
      }

      // if DEFAULT_SCHEMA_ACCESS_ONLY, can only access default and public schemas
      if (corrName.getSpecialType()==ExtendedQualName::NORMAL_TABLE) 
                                    // NORMAL_TABLE also covers synonym, view and MV
      {
        if (violateAccessDefaultSchemaOnly(corrName.getQualifiedNameObj()))
          return NULL;
      }

      // make sure that schema name is not a VOLATILE SCHEMA
      if ((! bindWA->inDDL()) ||
          ((bindWA->inViewDefinition()) ||
           (bindWA->inMVDefinition())))
      {
        // for Histogram, support to use VOLATILE SCHEMA
        // or else, don't support
        if (!corrName.getQualifiedNameObj().isHistogramTable() && 
            !CmpCommon::context()->sqlSession()->validateVolatileQualifiedSchemaName
            (corrName.getQualifiedNameObj()))
        {
          bindWA->setErrStatus();
          return NULL;
        }
      }

      // if specified name is an HBASE name, see if a mapped table exists
      if ((corrName.getQualifiedNameObj().getCatalogName() == HBASE_SYSTEM_CATALOG) &&
          ((corrName.getQualifiedNameObj().getSchemaName() == HBASE_MAP_SCHEMA) ||
           (corrName.getQualifiedNameObj().getSchemaName() == HBASE_SYSTEM_SCHEMA)))
        {
          corrName.getQualifiedNameObj().setCatalogName(TRAFODION_SYSCAT_LIT);
          corrName.getQualifiedNameObj().setSchemaName(HBASE_EXT_MAP_SCHEMA);
        }
      
      //get NATable (from cache or from metadata)
      table = bindWA->getSchemaDB()->getNATableDB()->
                                     get(corrName, bindWA, inTableDescStruct);

      //try the public schema if not found
      if ( !table && !pubSchemaIntName.isNull() )
      {
        CorrName pCorrName(corrName);
        pCorrName.getQualifiedNameObj().setSchemaName(pubSchemaIntName);
        if ( !pubSchema.getCatalogNamePart().isEmpty() )
        {
          pCorrName.getQualifiedNameObj().setCatalogName(
            pubSchema.getCatalogNamePart().getInternalName());
        }

        bindWA->resetErrStatus();
        table = bindWA->getSchemaDB()->getNATableDB()->
                                       get(pCorrName, bindWA, inTableDescStruct);
        if ( !bindWA->errStatus() && table )
        { // if found in public schema, do not show previous error
          // and replace corrName
          CmpCommon::diags()->clear();
          corrName.getQualifiedNameObj().setCatalogName(
            pCorrName.getQualifiedNameObj().getCatalogName());
          corrName.getQualifiedNameObj().setSchemaName(
            pCorrName.getQualifiedNameObj().getSchemaName());
        }
      }

      // if sch name was not specified and table is not found in default schema, then
      // look for it in HBase mapped schema.
      if ( !table && ! schNameSpecified)
        {
          CorrName pCorrName(corrName);
          pCorrName.getQualifiedNameObj().setSchemaName(HBASE_EXT_MAP_SCHEMA);
          
          bindWA->resetErrStatus();
          Lng32 diagsMark = CmpCommon::diags()->mark();
          table = bindWA->getSchemaDB()->getNATableDB()->
            get(pCorrName, bindWA, inTableDescStruct);
          if ( !bindWA->errStatus() && table )
            { // if found in mapped schema, do not show previous error
              // and replace corrName
              CmpCommon::diags()->clear();
              corrName.getQualifiedNameObj().setCatalogName(
                   pCorrName.getQualifiedNameObj().getCatalogName());
              corrName.getQualifiedNameObj().setSchemaName(
                   pCorrName.getQualifiedNameObj().getSchemaName());
            }
          else
            {
              // discard the errors from failed map table name lookup and only return
              // the previous error.
              CmpCommon::diags()->rewind(diagsMark);
            }
         }

      // move to here, after public schema try because BindUtil_CollectTableUsageInfo
      // saves table info for mv definition, etc.
      // Conditionally (usually) do stuff for Catalog Manager (static func above).

      if (catmanCollectTableUsages)
        if (corrName.getSpecialType() != ExtendedQualName::TRIGTEMP_TABLE)
          {
            if ((bindWA->inViewDefinition()) &&
                (corrName.isHbaseMap()))
              {
                // this is an internal hbase mapped name.
                // Use the original name to store in view definition.
                CorrName origName(corrName);
                origName.getQualifiedNameObj().setCatalogName(HBASE_SYSTEM_CATALOG);
                origName.getQualifiedNameObj().setSchemaName(HBASE_MAP_SCHEMA);
                BindUtil_CollectTableUsageInfo(bindWA, origName); 
              }
            else
              BindUtil_CollectTableUsageInfo(bindWA, corrName); 
          }

      if (!table)
        {
          if (volatileTableFound) 
            {
              if ((CmpCommon::diags()->mainSQLCODE() == -1003) &&
                  (NOT catNameSpecified))
                {
                  // the name is in true USER_NAME.VOL_TAB_NAME form
                  // where the USER_NAME doesn't match current name.
                  // Clear errors and return an appropriate message.
                  CmpCommon::diags()->clear();
                  CmpCommon::context()->sqlSession()->validateVolatileCorrName
                    (corrName);
                  bindWA->setErrStatus();
                }
            }
          
          return NULL;
        }
    }

  // if a volatile table is found, make sure that volatile schema is in
  // use and volatile tables are allowed.
  if ((table) && (table->isVolatileTable()))
    {
      // set volatile table indication in table's tablename
      ((QualifiedName&)(table->getTableName())).setIsVolatile(TRUE);
    }
      
  // For now, don't allow access through the Trafodion external name created for
  // native HIVE or HBASE objects unless the allowExternalTables flag is set.  
  // allowExternalTables is set for drop table and SHOWDDL statements.  
  // TDB - may want to merge the Trafodion version with the native version.
  if ((table) && 
      (table->isTrafExternalTable() && 
       (NOT table->getTableName().isHbaseMappedName()) &&
       (! bindWA->allowExternalTables())))    
    {
      *CmpCommon::diags() << DgSqlCode(-4258)
			  << DgTableName(table->getTableName().getQualifiedNameAsAnsiString());
      
      bindWA->setErrStatus();
      return NULL;
    }
  
  // If the table is an external table and has an associated native table, 
  // check to see if the external table structure still matches the native table.
  // If not, return an error
  if ((table) && table->isTrafExternalTable() &&
      (NOT table->getTableName().isHbaseMappedName()))
    {
      NAString adjustedName =ComConvertTrafNameToNativeName 
           (table->getTableName().getCatalogName(),
            table->getTableName().getUnqualifiedSchemaNameAsAnsiString(),
            table->getTableName().getUnqualifiedObjectNameAsAnsiString()); 
        
      // Get a description of the associated Trafodion table
      Int32 numNameParts = 3;
      QualifiedName adjustedQualName(adjustedName,numNameParts,STMTHEAP, bindWA);
      CorrName externalCorrName(adjustedQualName, STMTHEAP);
      NATable *nativeNATable = bindWA->getSchemaDB()->getNATableDB()->
                                  get(externalCorrName, bindWA, inTableDescStruct);
      if ((bindWA->externalTableDrop()) &&
          (bindWA->errStatus()))
        {
          return NULL;
        }
      
      // Compare native and external table definitions.
      // -- If this call is to drop external table, skip comparison.
      // -- Otherwise compare that number of columns is the same.
      // -- Compare type for corresponding columns. But if external table 
      //    was created with explicit col attrs, then skip type check for cols.
      // 
      // TBD - return what mismatches
      NABoolean compError = FALSE;
      if (nativeNATable &&
          (NOT bindWA->externalTableDrop()))
        { 
          if (table->getNAColumnArray().entries() != 
              nativeNATable->getNAColumnArray().entries())
            compError = TRUE;
          if ((NOT compError) &&
              (NOT table->hiveExtColAttrs()) &&
              (NOT table->hiveExtKeyAttrs()))
            {
              if (NOT (table->getNAColumnArray() == nativeNATable->getNAColumnArray()))
                compError = TRUE;
            }
          if (compError)
            {
              *CmpCommon::diags() << DgSqlCode(-3078)
                                  << DgString0(adjustedName)
                                  << DgTableName(table->getTableName().getQualifiedNameAsAnsiString());
              bindWA->setErrStatus();
              nativeNATable->setRemoveFromCacheBNC(TRUE);
              return NULL;
            }
        }
    }
  
  HostVar *proto = corrName.getPrototype();
  if (proto && proto->isPrototypeValid())
    corrName.getPrototype()->bindNode(bindWA);

  // This test is not "inAnyConstraint()" because we DO want to increment
  // the count for View With Check Option constraints.
  if (!getCurrentScope()->context()->inTableCheckConstraint() &&
      !getCurrentScope()->context()->inRIConstraint())
    table->incrReferenceCount();

  if (table)
    OSIM_captureTableOrView(table);

  return table;
} // BindWA::getNATable()

NATable *BindWA::getNATableInternal(
     CorrName& corrName,
     NABoolean catmanCollectTableUsages, // default TRUE
     TrafDesc *inTableDescStruct,     // default NULL
     NABoolean extTableDrop)
{
  ULng32 savedParserFlags = Get_SqlParser_Flags (0xFFFFFFFF);
  Set_SqlParser_Flags(ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME);
  Set_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL);
  
  setAllowExternalTables(TRUE);
  if (extTableDrop)
    setExternalTableDrop(TRUE);

  NATable * nat = 
    getNATable(corrName, catmanCollectTableUsages, inTableDescStruct);

  // Restore parser flags settings to what they originally were
  Assign_SqlParser_Flags (savedParserFlags);
  setAllowExternalTables(FALSE);
  setExternalTableDrop(FALSE);

  return nat;
}

static TableDesc *createTableDesc2(BindWA *bindWA,
                                   const NATable *naTable,
                                   CorrName &corrName, Hint *hint)
{
  // Allocate a base table descriptor.
  //
  TableDesc *tdesc = new (bindWA->wHeap()) TableDesc(bindWA, naTable, corrName);

  // Insert the table name into the XTNM.
  //
  bindWA->getCurrentScope()->getXTNM()->insertNames(bindWA, corrName);
  if (bindWA->errStatus()) return NULL;

  // For each NAColumn, allocate a BaseColumn, bind the BaseColumn, and
  // add the ValueId to the TableDesc.
  //
  CollIndex i = 0;
  for (i = 0; i < naTable->getColumnCount(); i++) {
    BaseColumn *baseCol = new (bindWA->wHeap()) BaseColumn(tdesc, i);
    baseCol->bindNode(bindWA);
    if (bindWA->errStatus()) 
      return NULL;
    ValueId valId = baseCol->getValueId();
    tdesc->addToColumnList(valId);
  }

  // set primary key for this table
  tdesc->setPrimaryKeyColumns();

  // For each index, create an IndexDesc.
  //

  NAString indexChoice;
  NADefaults &defs = ActiveSchemaDB()->getDefaults();
  defs.getValue(HIDE_INDEXES,indexChoice);

  for (i = 0; i < naTable->getIndexList().entries(); i++)
  {
      NAFileSet *nfs=naTable->getIndexList()[i];
      
      IndexDesc *idesc = new (bindWA->wHeap())
        IndexDesc(tdesc, nfs, bindWA->currentCmpContext());

      if (naTable->getClusteringIndex()->getFileSetName() ==
          idesc->getIndexName()) {
        tdesc->setClusteringIndex(idesc);
        idesc->markAsClusteringIndex();
      }
      
      if(indexChoice.compareTo("NONE") ==0
         OR indexChoice.compareTo("VERTICAL") ==0
         OR (indexChoice.compareTo("KEYINDEXES") ==0 AND
         tdesc->isKeyIndex(idesc))
         OR naTable->getClusteringIndex()->getFileSetName() ==
            nfs->getFileSetName())
      {
          tdesc->addIndex(idesc);
          // implementation of optimizer hints
          if (hint AND hint->hasIndexHint
              (idesc->getNAFileSet()->getExtFileSetName()))
          {
            tdesc->addHintIndex(idesc);
          }
      if (idesc->isUniqueIndex() )
        tdesc->addUniqueIndex(idesc);
      }
      else
      {
        delete idesc;
      }

  }

  if (hint AND hint->indexCnt() > tdesc->getHintIndexes().entries() AND
      CmpCommon::getDefault(INDEX_HINT_WARNINGS) != DF_OFF)
    {
      // emit a warning that we didn't process all the index hints,
      // there is probably a spelling mistake
      *CmpCommon::diags() << DgSqlCode(4371)
                          << DgInt0(tdesc->getHintIndexes().entries())
                          << DgInt1(hint->indexCnt())
                          << DgTableName(naTable->getTableName().
                                         getQualifiedNameAsAnsiString());
    }

  // For each vertical partition, create an IndexDesc.
  // Add this VP to the list of VPs for the TableDesc.
 for (i = 0; i < naTable->getVerticalPartitionList().entries(); i++) {
    if(indexChoice.compareTo("NONE") ==0
       OR indexChoice.compareTo("INDEXES")==0
       OR indexChoice.compareTo("KEYINDEXES")==0)
      {
        IndexDesc *idesc = new (bindWA->wHeap())
            IndexDesc(tdesc, naTable->getVerticalPartitionList()[i],
                     bindWA->currentCmpContext());
        tdesc->addVerticalPartition(idesc);
      }
  }

  // Allocate a RETDesc, attach it to the BindScope.
  //
  bindWA->getCurrentScope()->setRETDesc(new (bindWA->wHeap())
    RETDesc(bindWA, tdesc));

  // Do not include tables-referenced-in-a-constraint (when/if we allow them)
  // in the view-contains-table list; if we did include them, then
  // TableViewUsageList::getViewsOnTable() would give wrong results
  // for where it's used to prevent the Halloween problem.
  //
  // If we end up needing this extra info, I advise either a separate list,
  // or a new field in TableViewUsage indicating usage type (containment
  // versus reference), enhancing method getViewsOnTable() accordingly.
  //
  if (!bindWA->getCurrentScope()->context()->inAnyConstraint())
    bindWA->tableViewUsageList().insert(new (bindWA->wHeap())
      TableViewUsage(
           tdesc->getCorrNameObj().getQualifiedNameObj(),
           tdesc->getCorrNameObj().getSpecialType(),
           naTable->getViewText() != NULL,
           bindWA->viewCount()));

  return tdesc;

} // static createTableDesc2()

TableDesc *BindWA::createTableDesc(const NATable *naTable,
                                   CorrName &corrName,
                                   NABoolean catmanCollectUsages, Hint *hint)
{
  BindWA *bindWA = this;  // for coding convenience

  TableDesc *tdesc = createTableDesc2(bindWA, naTable, corrName, hint);
  if (bindWA->errStatus()) return NULL;

  // Now bind any table check constraints and attach them to our new tdesc.
  // These constraints must be processed for UPDATE and INSERT.
  // DELETEs must clear them; see Delete::bindNode.
  //
  // For SELECTs, NOT NULL constraints are marked on the NAColumn::allowsNulls
  // allowing more elaborate Transformations.  For SELECTs, other types of
  // constraints are not currently used, but could be in future,
  // to optimize by providing additional predicate/selectivity info.
  //
  // ## We ought to write some regression test cases like
  //	INSERT INTO T (SELECT * FROM S)	-- T's constraints yes, S irrelevant
  //	INSERT INTO T VALUES ((SELECT A FROM S WHERE..),..)
  //	INSERT INTO V3 ...	-- underlying basetbl's constrts yes
  //				-- V3 atop VA atop T: let the views be
  //				-- WITH CHECK OPTION, then viewpred-constrt yes
  //
  const CheckConstraintList &ccl = naTable->getCheckConstraints();
  if (ccl.entries()) {

    // Table check constraint text is stored in the metadata tables
    // with the underlying table/view name (e.g. "CHECK (C.S.T.COL > 0)"),
    // whereas any correlation name in a query
    // (e.g. "SELECT * FROM C.S.T FOO WHERE COL < 10")
    // is irrelevant to the persistent constraint text --
    // when binding the check constraint, we want to find column C.S.T.COL,
    // while the TableDesc/RETDesc just built only exposes the column
    // under names COL and FOO.COL.
    //
    // So, if we have a correlation name, we must:
    // - rename our TableDesc (rename FOO to C.S.T)
    // - create a temporary table name scope for C.S.T that will hide FOO
    // - construct a temporary RETDesc with names COL, T.COL, S.T.COL, C.S.T.COL
    //   but the same ValueId's they had before
    //
    // Then we bind the constraints using that RETDesc for name lookups.
    //
    // Then for the non-empty correlation, reset/undo the temporary stuff.

    RETDesc *savedRETDesc = NULL;
    NABoolean corrNameIsNonEmpty = !corrName.getCorrNameAsString().isNull();
    CorrName synonymReferenceCorrName; 

    if(naTable->getIsSynonymTranslationDone()){
      QualifiedName baseQualifiedName(naTable->getSynonymReferenceName(),3);
      synonymReferenceCorrName=baseQualifiedName; 
    }
   
    if ((corrNameIsNonEmpty) || (naTable->getIsSynonymTranslationDone())) {
      
      CorrName baseCorrName;
      baseCorrName = (naTable->getIsSynonymTranslationDone()) ? synonymReferenceCorrName : naTable->getTableName();

      tdesc->setCorrName(baseCorrName);

      bindWA->getCurrentScope()->xtnmStack()->createXTNM();
      bindWA->getCurrentScope()->getXTNM()->insertNames(bindWA, baseCorrName);
      if (bindWA->errStatus()) return NULL;

      savedRETDesc = bindWA->getCurrentScope()->getRETDesc();
      bindWA->getCurrentScope()->setRETDesc(new (bindWA->wHeap())
      RETDesc(bindWA, tdesc));
      if (bindWA->errStatus()) return NULL;
    }

    for (CollIndex i = 0; i < ccl.entries(); i++) {
      ItemExpr *constraintPred =
        bindCheckConstraint(bindWA, ccl[i], naTable, catmanCollectUsages);
      if (constraintPred)
        tdesc->addCheckConstraint(bindWA, naTable, ccl[i], constraintPred);
      else if (bindWA->errStatus())
        break;
    }

    if ((corrNameIsNonEmpty) || (naTable->getIsSynonymTranslationDone())){  // reset temporaries
      tdesc->setCorrName(corrName);
      delete bindWA->getCurrentScope()->getRETDesc();
      bindWA->getCurrentScope()->setRETDesc(savedRETDesc);
      bindWA->getCurrentScope()->xtnmStack()->removeXTNM();
    }
  } // check constraint processing required

  // if the table contains computed columns, bind the expressions to compute the columns
  for (CollIndex c = 0; c < naTable->getColumnCount(); c++) {
    NAColumn *nac = tdesc->getNATable()->getNAColumnArray()[c];

    if (nac->isComputedColumn()) {
        ItemExpr *computedColumnExpr = NULL;
        Parser parser(bindWA->currentCmpContext());

        // parse the text stored in the NAColumn
        computedColumnExpr = parser.getItemExprTree(
             nac->getComputedColumnExprString(),
             str_len(nac->getComputedColumnExprString()),
             CharInfo::UTF8);

        if (computedColumnExpr) {
          ParNameLocList *saveNameLocList = bindWA->getNameLocListPtr();
          bindWA->setNameLocListPtr(NULL);
          bindWA->getCurrentScope()->context()->inComputedColumnExpr() = TRUE;
          
          computedColumnExpr = computedColumnExpr->bindNode(bindWA);

          bindWA->setNameLocListPtr(saveNameLocList);
          bindWA->getCurrentScope()->context()->inComputedColumnExpr() = FALSE;
         
          if (bindWA->errStatus()) {
            delete computedColumnExpr;
            computedColumnExpr = NULL;
            return NULL;
          }
          else {
            // Store the expression tree in the base column
            ((BaseColumn *) tdesc->getColumnList()[c].getItemExpr())->
              setComputedColumnExpr(computedColumnExpr->getValueId());
          }
        }
    }
  }

  return tdesc;

} // BindWA::createTableDesc()


// QSTUFF - helper for BindWA::bindView.
static void propagateDeleteAndStream(RelExpr *re, GroupAttributes *ga)
{
  if (ga->isEmbeddedUpdateOrDelete())
    re->getGroupAttr()->setEmbeddedIUD(
      ga->getEmbeddedIUD());

  if (ga->isStream())
    re->getGroupAttr()->setStream(TRUE);

  if (ga->isSkipInitialScan())
    re->getGroupAttr()->setSkipInitialScan(TRUE);

  Int32 arity = re->getArity();
  for (Int32 i = 0; i < arity; i++) {
    if (re->child(i))
      propagateDeleteAndStream(re->child(i), ga);
  }
}

RelExpr *BindWA::bindView(const CorrName &viewName,
                          const NATable *naTable,
                          const StmtLevelAccessOptions &accessOptions,
                          ItemExpr *predicate,
                          GroupAttributes *groupAttrs,
                          NABoolean catmanCollectUsages)
{
  BindWA *bindWA = this;   // for coding convenience

  CMPASSERT(viewName.getQualifiedNameObj() == naTable->getTableName());

  NABoolean inViewExpansion = bindWA->setInViewExpansion(TRUE);   // QSTUFF

  // If this is a native hive view, then temporarily change the default schema to
  // that of viewName. This will make sure that all unqualified objects in view
  // text are expanded in this schema. 
  NABoolean defSchWasChanged = FALSE;
  SchemaName savedSch(bindWA->getDefaultSchema());
  if (viewName.isHive())
    {
      SchemaName s(viewName.getQualifiedNameObj().getSchemaName(),
                   viewName.getQualifiedNameObj().getCatalogName());
      bindWA->setDefaultSchema(s);
      defSchWasChanged = TRUE;
    }

  // set a flag for overrride_schema
  //if (overrideSchemaEnabled())
    bindWA->getCurrentScope()->setInViewExpansion(TRUE);

  if (!bindWA->getCurrentScope()->context()->inAnyConstraint())
    bindWA->tableViewUsageList().insert(new (bindWA->wHeap())
      TableViewUsage(
        viewName.getQualifiedNameObj(),
        viewName.getSpecialType(),
        TRUE/*isView*/,
        bindWA->viewCount()));

  // save the current parserflags setting
  ULng32 savedParserFlags = Get_SqlParser_Flags (0xFFFFFFFF);

  // allow funny characters in the tablenames used in the select list.
  // This enables views to be created on 'internal' secret table
  // so they could be accessed.
  // At view creation time, the caller still need to set this
  // parserflag from the sql interface(mxci, etc) otherwise the view
  // creation will fail. Since parserflags can only be set by super
  // users, the view with special tablenames could only have been created
  // by a super user.
  Set_SqlParser_Flags(ALLOW_FUNNY_IDENTIFIER);

  // Parse the view text.
  //
  // isolation level and order by are allowed in create view, if
  // the corresponding cqds are set.
  // These cqds are only valid during 'create view' time. Once the views
  // are created, we don't need to look at them.
  // During view expansion when we reach this method, turn the cqds on if
  // they are not already on, so parser doesn't return an error.
  // Reset them back, if they were set here.
  NABoolean allowIsolationLevelWasSet = FALSE;
  NABoolean allowOrderByWasSet = FALSE;
  if (CmpCommon::getDefault(ALLOW_ISOLATION_LEVEL_IN_CREATE_VIEW) == DF_OFF)
    {
      allowIsolationLevelWasSet = TRUE;

      NAString op("ON");
      ActiveSchemaDB()->getDefaults().validateAndInsert
        ("ALLOW_ISOLATION_LEVEL_IN_CREATE_VIEW", op, FALSE);
    }
  if (CmpCommon::getDefault(ALLOW_ORDER_BY_IN_CREATE_VIEW) == DF_OFF)
    {
      allowOrderByWasSet = TRUE;

      NAString op("ON");
      ActiveSchemaDB()->getDefaults().validateAndInsert
        ("ALLOW_ORDER_BY_IN_CREATE_VIEW", op, FALSE);
    }

  Parser parser(bindWA->currentCmpContext());
  parser.hiveDDLInfo_->disableDDLcheck_ = TRUE;
  ExprNode *viewTree = parser.parseDML(naTable->getViewText(),
                                       naTable->getViewLen(),
                                       naTable->getViewTextCharSet());

  // Restore parser flags settings to what they originally were
  Set_SqlParser_Flags (savedParserFlags);

  if (allowIsolationLevelWasSet)
    {
      NAString op("OFF");
      ActiveSchemaDB()->getDefaults().validateAndInsert
        ("ALLOW_ISOLATION_LEVEL_IN_CREATE_VIEW", op, FALSE);
    }
  if (allowOrderByWasSet)
    {
      NAString op("OFF");
      ActiveSchemaDB()->getDefaults().validateAndInsert
        ("ALLOW_ORDER_BY_IN_CREATE_VIEW", op, FALSE);
    }

  if (NOT viewTree) {
    bindWA->setErrStatus();

    if (defSchWasChanged)
      bindWA->setDefaultSchema(savedSch);

    return NULL;
  }

  // Remove the StmtQuery node.
  // Clear the root flag in the RelRoot node since this not the topmost
  // RelRoot in the query tree.
  //
  CMPASSERT(viewTree->getOperatorType() == STM_QUERY);
  RelExpr *queryTree = viewTree->castToStatementExpr()->getQueryExpression();
  CMPASSERT(queryTree->getOperatorType() == REL_ROOT);
  ((RelRoot *)queryTree)->setRootFlag(FALSE);

  CMPASSERT(queryTree->getChild(0)->getOperatorType() == REL_DDL);
  StmtDDLCreateView *createViewTree = ((DDLExpr *)(queryTree->getChild(0)))->
    getDDLNode()->castToStmtDDLNode()->castToStmtDDLCreateView();
  CMPASSERT(createViewTree);
  queryTree = createViewTree->getQueryExpression();
  CMPASSERT(queryTree->getOperatorType() == REL_ROOT);
  ((RelRoot *)queryTree)->setRootFlag(FALSE);

  RelRoot *viewRoot = (RelRoot *)queryTree;  // save for add'l binding below
  ParNameLocList *saveNameLocList = bindWA->getNameLocListPtr();

  // This was put here for Genesis 10-980217-0467.
  // Now with the fix for 10-980408-5149, we even more strongly need to bypass
  // or ignore any accessOpts from the view, for a consistent access model.

  if ((CmpCommon::getDefault(ALLOW_ISOLATION_LEVEL_IN_CREATE_VIEW) == DF_OFF) ||
      (viewRoot->accessOptions().accessType() == TransMode::ACCESS_TYPE_NOT_SPECIFIED_))
    {
      // if cqd is set and view options were explicitely specified,
      // then do not overwrite it with accessOptions.
      viewRoot->accessOptions() = accessOptions;
    }

  // Set the WCO context (Genesis 10-971112-7028 + 10-990518-8420):
  //   If this view is WITH CHECK OPTION, then all views below it acquire
  //   check-option-ness, per Ansi 11.19 GR 9-11a
  //   (we implement only CASCADED -- see further notes later on in this func);
  //   if some view above this one is WCO, then this view effectively is too,
  //   regardless of its getViewCheck() value.
  // Genesis 10-990518-8420 fix in particular:
  //   with-check-option views of the form
  //     SELECT..FROM(SELECT..WHERE p1)REN WHERE p2
  //   were emitting a bind error on pred p1, and ignoring pred p2!
  //
  NABoolean topmostViewWithCheckOption = FALSE;
  if (naTable->getViewCheck() &&
      bindWA->getCurrentScope()->context()->inUpdateOrInsert() &&
      !bindWA->inViewWithCheckOption()) {
    topmostViewWithCheckOption = TRUE;
    bindWA->inViewWithCheckOption() = naTable;
  }

  // QSTUFF
  // Give the new query tree the pubsub group attrs before
  // binding, so that binder checks are applied to the new tree.
  if ((groupAttrs) &&
      (groupAttrs->isEmbeddedUpdateOrDelete() || groupAttrs->isStream()))
    propagateDeleteAndStream(queryTree,groupAttrs);

  // ************ THE FIRST OF TWO BINDNODE'S ************
  // Bind the basic queryTree first (before Rename), for stoi_ security stuff.
  // Cascade the WCO-ness down to RelExpr::bindSelf which captures predicates.
  // On this bind, unconditionally we never collect usages.
  //
  bindWA->viewCount()++;

  bindWA->setNameLocListPtr(NULL);      // do not collect usages for catman

  queryTree = queryTree->bindNode(bindWA);

  if (bindWA->errStatus())
    {
      if (defSchWasChanged)
        bindWA->setDefaultSchema(savedSch);
      
      return NULL;
    }

  bindWA->setNameLocListPtr(saveNameLocList);

  bindWA->viewCount()--;

  // if RelRoot has an order by, insert a Logical Sort node below it
  // and move the order by expr from view root to this sort node.
  // The view root node is eliminated during transformation/normalization
  // and the sortlogical node provides a place to 'hold' the order by expr.
  // During transformation, this sort key is moved from the sortlogical node
  // to the root node of the query, if there is no explicit order by 
  // specified as part of the query.
  // SortLogical node is a shortlived node and is eliminated during
  // the normalization phase.
  if (viewRoot->hasOrderBy())
    {
      RelExpr * sortNode = new (bindWA->wHeap())
                             SortLogical(queryTree->child(0)->castToRelExpr(),
                                         viewRoot->reqdOrder(),
                                         bindWA->wHeap());
      sortNode = sortNode->bindNode(bindWA);
      if (bindWA->errStatus())
        return NULL;

      viewRoot->removeOrderByTree();
      viewRoot->reqdOrder().clear();

      viewRoot->setChild(0, sortNode);
    }

  // Insert a RenameTable node above the view tree.
  //
  const NAColumnArray &columns = naTable->getNAColumnArray();
  ItemExpr *columnList = new (bindWA->wHeap())
    RenameCol(NULL, new (bindWA->wHeap())
      ColRefName(columns[0]->getColName(), bindWA->wHeap()));
  //
  CollIndex i = 1;
  for (i = 1; i < naTable->getColumnCount(); i++)
    columnList = new (bindWA->wHeap())
      ItemList(columnList, new (bindWA->wHeap())
        RenameCol(NULL, new (bindWA->wHeap())
          ColRefName(columns[i]->getColName(), bindWA->wHeap())));
  //

  queryTree = new (bindWA->wHeap())
    RenameTable(TRUE/*copy tableName as is*/,
                queryTree->castToRelExpr(),
                viewName,
                columnList,
                bindWA->wHeap(),
                TRUE/*isView*/);
  if (predicate) queryTree->addSelPredTree(predicate);
  ((RenameTable *) queryTree)->setViewNATable(naTable);

  // this query used this view
  appendViewName
    (viewName.getQualifiedNameObj().getQualifiedNameAsAnsiString().data());

  // set a flag for overrride_schema
  // with the call to bindNode below, only the Rename node will be bound.
  // Since the view has already been expanded we reset the viewExpansion flag here.
  //if (overrideSchemaEnabled())
    bindWA->getCurrentScope()->setInViewExpansion(inViewExpansion);

  // ************ THE SECOND OF TWO BINDNODE'S ************
  // Bind the view tree whose top is this new RenameTable.
  // If we are the topmost WCO, then do NOT cascade the incoming predicate!
  // Collect usages only if CatMan caller requested it.
  //
  if (topmostViewWithCheckOption) bindWA->inViewWithCheckOption() = NULL;
  if (!catmanCollectUsages) bindWA->setNameLocListPtr(NULL);
  queryTree = queryTree->bindNode(bindWA);
  bindWA->setNameLocListPtr(saveNameLocList);
  if (bindWA->errStatus()) return NULL;

  ((RenameTable *) queryTree)->setViewNATable(NULL);

  // Genesis 10-980126-5495:
  // Now that we have the RenameTable's RETDesc, set its view column headings.
  // We know that the NATable and the RenameTable column lists are in lockstep.
  //
  const ColumnDescList &columnsRET = *queryTree->getRETDesc()->getColumnList();
  CMPASSERT(columns.entries() == naTable->getColumnCount() &&
            columns.entries() == columnsRET.entries());
  for (i = 0; i < naTable->getColumnCount(); i++)
  {
    columnsRET[i]->setHeading(columns[i]->getHeading());
  }


  // If it's a view that is WITH CHECK OPTION, and this is an UPDATE/INSERT,
  // bind/transform/normalize the view predicate and place it as a constraint
  // on the base table's TableDesc.  This is equivalent to the default kind
  // of check clause, WITH CASCADED CHECK OPTION, which is all we need provide
  // up through Intermediate-Level SQL'92.
  //
  // (ANSI says that all CHECK OPTION views must be updatable (11.19 SR12)
  // which means it references exactly one updatable view or, at bottom,
  // exactly one base table (7.9 SR12).
  // MP guarantees that all CHECK OPTION views must be protection views, and
  // all pviews reference exactly one base table.)
  //
  // Notice that since (Genesis 10-990518-8420) we now bind and collect the
  // view preds in bindSelf -- i.e. pushed down below here --
  // only this topmost WCO can set up the constraint(s).
  // Thus we have lost the nice, but not mandated by Ansi, ability to specify
  // which cascaded-down-to view causes which exact pred violation --
  // i.e. error EXE_CHECK_OPTION_VIOLATION_CASCADED (8104)
  // no longer appears, only EXE_CHECK_OPTION_VIOLATION (8105).

  if (topmostViewWithCheckOption) {

    CheckConstraint *constraint = NULL;
    ItemExpr *viewCheckPred = NULL;

    if (bindWA->predsOfViewWithCheckOption().entries()) {
      constraint = new (bindWA->wHeap())
      CheckConstraint(viewName.getQualifiedNameObj(),       // this view name
                      naTable->getTableName(),       // no parsing needed
                      bindWA->wHeap());
      viewCheckPred = bindWA->predsOfViewWithCheckOption().rebuildExprTree();
    }

    // if at least one predicate exists in the view or what underlies it
    if (constraint) {

      RelExpr *underlyingTableOrView = viewRoot->child(0);
      RETDesc *saveRETDesc = bindWA->getCurrentScope()->getRETDesc();
      RETDesc *underlyingRETDesc = underlyingTableOrView->getRETDesc();
      bindWA->getCurrentScope()->setRETDesc(underlyingRETDesc);
      CMPASSERT(underlyingTableOrView);
      CMPASSERT(underlyingTableOrView->getOperatorType() == REL_RENAME_TABLE ||
                underlyingTableOrView->getOperatorType() == REL_SCAN);

      ItemExpr *constraintPred =
                 bindCheckConstraint(bindWA,
                                     constraint,
                                     naTable,
                                     catmanCollectUsages,
                                     viewCheckPred);
      if (constraintPred)
                 queryTree->getScanNode()->getTableDesc()->addCheckConstraint(
                                                             bindWA,
                                                             naTable,    // topmost WCO view
                                                             constraint,    // this view name
                                                             constraintPred);

      bindWA->getCurrentScope()->setRETDesc(saveRETDesc);

    }   // at least one predicate exists

    bindWA->inViewWithCheckOption() = NULL;
    bindWA->predsOfViewWithCheckOption().clear();

  }   // topmost WCO view

  // QSTUFF
  bindWA->setInViewExpansion(inViewExpansion);
  bindWA->getUpdateToScanValueIds().clear();
  // QSTUFF

  if (defSchWasChanged)
    bindWA->setDefaultSchema(savedSch);

  return queryTree;
} // BindWA::bindView()

// -----------------------------------------------------------------------
// member functions for class RelExpr
// -----------------------------------------------------------------------

void RelExpr::bindChildren(BindWA *bindWA)
{
  // Increment the trigger recursion counter.
  if (getInliningInfo().isTriggerRoot())
    getInliningInfo().getTriggerObject()->incRecursionCounter();

  // TSJ's flow their data from left child to right child;
  // some can also share binding scope column info from left to right.
  Int32 arity = getArity();
  for (Int32 i = 0; i < arity; i++) {
    if (child(i)) {

      // If doing a non-first child and the operator is
      // NOT one in which values/names can flow from one scope
      // the sibling scope, then we must clear the current RETDesc
      // (so as to disallow the illegal query in the Binder internals document,
      // section 1.5.3, also in TEST028).
      //
      if (i && !getOperator().match(REL_ANY_TSJ))
        bindWA->getCurrentScope()->setRETDesc(NULL);

      child(i) = child(i)->bindNode(bindWA);
      if (bindWA->errStatus()) return;
    }
  }

  synthPropForBindChecks();   // QSTUFF

  // Decrement the trigger recursion counter.
  if (getInliningInfo().isTriggerRoot())
    getInliningInfo().getTriggerObject()->decRecursionCounter();

} // RelExpr::bindChildren()

void RelExpr::synthPropForBindChecks()   // QSTUFF
{
  // synthesis of delete and stream properties to
  // allow for binder checks. We assume that all
  // operators are rejected when binding the respective node
  // -- except UNIONS -- in which more than one child has
  // has any of those attributes. If both attributes are
  // specified both must be specified for the same
  // result-set/base table.

  for (Int32 j = 0; j < getArity(); j++) {

     if (child(j)) {

        if (child(j)->getGroupAttr()->isStream())
        {
          getGroupAttr()->setStream(TRUE);

          if (child(j)->getGroupAttr()->isSkipInitialScan())
            getGroupAttr()->setSkipInitialScan(TRUE);
        }

        if (child(j)->getGroupAttr()->isEmbeddedUpdateOrDelete() ||
            child(j)->getGroupAttr()->isEmbeddedInsert())
          getGroupAttr()->setEmbeddedIUD(
               child(j)->getGroupAttr()->getEmbeddedIUD());
 
        if (child(j)->getGroupAttr()->reorderNeeded())
            getGroupAttr()->setReorderNeeded(TRUE);
     }
  }
}

RelExpr *RelExpr::bindSelf(BindWA *bindWA)
{
  // create the group attributes
  //
  if (NOT getGroupAttr())
    setGroupAttr(new (bindWA->wHeap()) GroupAttributes);

  //
  // Detach the item expression tree for the predicate, bind it, convert it to
  // a ValueIdSet, and attach it to the RelExpr node.
  //
  ItemExpr *predTree = removeSelPredTree();
  if (predTree) {
    bindWA->getCurrentScope()->context()->inWhereClause() = TRUE;
    predTree->convertToValueIdSet(selectionPred(), bindWA, ITM_AND);
    bindWA->getCurrentScope()->context()->inWhereClause() = FALSE;

    if (bindWA->errStatus()) return this;

    // If this is an embedded insert, then subquery predicates are not
    // allowed.  
    // For example:  To handle this query and issue an error stating
    //               subqueries are not allowed in embedded inserts
    // 
    //  select a from (insert into t901t01 values(22,22,222))t(a,b,c)
    //  where t.a IN (select m from t901t03 where t901t03.m = 77);

    if (getGroupAttr()->isEmbeddedInsert())
    {
       if (!selectionPred().isEmpty() && selectionPred().containsSubquery())
       {
         *CmpCommon::diags() << DgSqlCode(-4337);
         bindWA->setErrStatus();
         return this;
       }
    }  

    // Genesis 10-990518-8420.
    if (bindWA->inViewWithCheckOption())
      bindWA->predsOfViewWithCheckOption() += selectionPred();
  }

  // ++MV
  // Bind the uniqueColumnsTree expression.
  //
  ItemExpr *uniqueColumnsTree = removeUniqueColumnsTree();
  if (uniqueColumnsTree)
  {
    uniqueColumnsTree->
      convertToValueIdSet(getUniqueColumns(), bindWA, ITM_ITEM_LIST);
    if (bindWA->errStatus()) return this;
  }
  // --MV

  // set flag here if an Insert/Update/Delete operation is below this node   
  if( bindWA->isBindingIUD() )
  {
    setSeenIUD();
  }

  //
  // This mechanism is used to set InliningInfo flags on an entire subtree.
  getInliningInfo().setFlags(bindWA->getInliningInfoFlagsToSetRecursivly());

  //
  // Add the values in the Outer References Set as the input values
  // that must be supplied to this RelExpr.
  //
  getGroupAttr()->addCharacteristicInputs(bindWA->getCurrentScope()->getOuterRefs());
  markAsBound();
  return this;
} // RelExpr::bindSelf()

RelExpr *RelExpr::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  bindChildren(bindWA);
  if (bindWA->errStatus())
    return this;
  return bindSelf(bindWA);
}

RETDesc *RelExpr::getRETDesc() const
{
  if (RETDesc_)
    return RETDesc_;
  if ((getArity() == 1) && (child(0)))
    return child(0)->getRETDesc();
  else
    return NULL;
}

// When there is a view atop a view atop a ... atop a single base table,
// this will follow the chain of RenameTable-RelRoot-... down till it finds
// the bottom, the single base table's Scan node.
//
// This method does check to ensure exactly one single base table.
//
Scan *RelExpr::getScanNode(NABoolean assertExactlyOneScanNode) const
{
  RelExpr *result = (RelExpr *)this;   // cast away constness, big whoop

  while (result) {
    if ((result->getOperatorType() == REL_SCAN) ||
	(result->getOperatorType() == REL_HBASE_ACCESS))
      break;
    if (result->getArity() > 1) {
      if (assertExactlyOneScanNode)
      { 
        CMPASSERT(result->getArity() <= 1);
      }
      else return NULL;
    }
    result = result->child(0);
  }

  if (assertExactlyOneScanNode) { CMPASSERT(result); }
  return (Scan *)result;
}


Scan *RelExpr::getLeftmostScanNode() const
{
  RelExpr *result = (RelExpr *)this;   // cast away constness, big whoop

  while (result) {
    if (result->getOperatorType() == REL_SCAN) break;
    result = result->child(0);
  }

  return (Scan *)result;
}



Join * RelExpr::getLeftJoinChild() const
{
  RelExpr *result = (RelExpr *)this;
  
  while(result)
    {
      if (result->getOperatorType() == REL_LEFT_JOIN) 
        break;
      result = result->child(0);
    }
  return (Join *)result;
}

RelSequence* RelExpr::getOlapChild() const
{
  RelExpr *result = (RelExpr *)this;
  
  while(result)
    {
      if (result->getOperatorType() == REL_SEQUENCE) 
        break;
      result = result->child(0);
    }
  return (RelSequence *)result;
}

// QSTUFF
// We use this method for finding the scan node of an updatable view.
// This may either be a base table scan or a RenameTable node inserted
// by a previous index expansion.
RelExpr *RelExpr::getViewScanNode(NABoolean isTopLevelUpdateInView) const
{
  RelExpr *result = (RelExpr *)this;    // cast away constness, big whoop

  while (result) {
    if (result->getOperatorType() == REL_SCAN) break;

    if (result->getOperatorType() == REL_RENAME_TABLE &&
        ((RenameTable *)result)->isView()) break;

    result = result->child(0);
  }

  return result;
}

// -----------------------------------------------------------------------
// getFirstIUDNode
//
// Return the first node that is an insert, update, or delete.
// Only search down left side from the starting point (currentNode)
//
// If an IUD node is not found, return NULL
// -----------------------------------------------------------------------
GenericUpdate * Join::getFirstIUDNode(RelExpr *currentNode)
{
  while(currentNode)
  {
    if( currentNode->getOperator().match(REL_ANY_GEN_UPDATE))
    {      
      break;
    }        
    currentNode = currentNode->child(0);
  }
  return (GenericUpdate*)currentNode;
}

// -----------------------------------------------------------------------
// member functions for class Join
//
// When we implement "JOIN USING (column list)", we need to:		##
// - disallow both NATURAL and USING in the same query (syntax err in Parser?)
// - ensure that the named USING cols are indeed common cols
// - coalesce common cols for USING just as we do for NATURAL,
//   including ensuring that common cols are marked as referenced
//   (as done in joinCommonColumns)
// -----------------------------------------------------------------------

RelExpr *Join::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // Do not support for general NEO users.
  if ( (getOperatorType() == REL_FULL_JOIN) &&
       (CmpCommon::getDefault(COMP_BOOL_192) == DF_ON) ) {
    
    RelExpr *leftJoin = this;
    leftJoin->setOperatorType(REL_LEFT_JOIN);
    
    Join *antiJoin = static_cast<Join *>(leftJoin->copyTree(bindWA->wHeap()));
    antiJoin->setOperatorType(REL_RIGHT_JOIN);

    NAString leftName("ALJ", bindWA->wHeap());

    // Make it unique.  
    //  
    leftName += bindWA->fabricateUniqueName();

    RelExpr *rename = new (bindWA->wHeap())
      RenameTable(antiJoin, leftName);

    RelExpr *unionAll = new (bindWA->wHeap()) Union(leftJoin, rename);

    unionAll->bindNode(bindWA);
    if (bindWA->errStatus()) return this;

    ItemExpr *nullCheck = antiJoin->addNullInstIndicatorVar(bindWA).getItemExpr();

    CMPASSERT(nullCheck);
    
    ItemExpr *filter = new (bindWA->wHeap())
      UnLogic(ITM_IS_NULL, nullCheck );
    
    filter->bindNode(bindWA);
    if (bindWA->errStatus()) return this;
    
    // Add filter to Join
    //
    antiJoin->selectionPred() += filter->getValueId();

    return unionAll;
  }

  Join *saveInJ = bindWA->getCurrentScope()->context()->inJoin();
  bindWA->getCurrentScope()->context()->inJoin() = this;
  NABoolean savedPrivSetting = FALSE;

  // Bind the child nodes.
  //
  bindChildren(bindWA);
  if (bindWA->errStatus()) return this;

  // MV logging push-down
  if( getInliningInfo().isDrivingMvLogInsert() )
  {
    GenericUpdate *rightSideIUD = getFirstIUDNode(this->child(1));

    if( NULL != rightSideIUD )
    {
      TableDesc *tdesc = rightSideIUD->getTableDesc();

      CMPASSERT(tdesc);

      const NATable *table = tdesc->getNATable();

      // only for MV logs
      if( ExtendedQualName::IUD_LOG_TABLE == table->getSpecialType() )
      {
        updateTableDesc_ = tdesc;
        updateSelectValueIdMap_ = new (bindWA->wHeap())
                             ValueIdMap(rightSideIUD->updateToSelectMap());
      }
    }
  }


  // Controlled availability of Full Outer Join support 
  // The COMP_BOOL_199 must be removed when full outer join
  // becomes general availability.
  
  // Full outer joins are not currently supported.
  // But can enabled by setting COMP_BOOL_199 to ON.

  if ((getOperatorType() == REL_FULL_JOIN &&
       (CmpCommon::getDefault(COMP_BOOL_199) == DF_OFF))
      ||  //OR
      (getOperatorType() == REL_UNION_JOIN )){
    // 3022 Feature not yet supported
    *CmpCommon::diags() << DgSqlCode(-3022)
                        << DgString0(
                                    (getOperatorType() == REL_FULL_JOIN) ?
                                    "FULL OUTER JOIN" : "UNION JOIN");
    bindWA->setErrStatus();
    return this;
  }
  
  //
  // Bind the ON clause of the join.
  //
  RelExpr *leftRelExpr  = child(0).getPtr();
  RelExpr *rightRelExpr = child(1).getPtr();
  RETDesc *leftTable    = child(0)->getRETDesc();
  RETDesc *rightTable   = child(1)->getRETDesc();

  ItemExpr *joinPredx;
  if (isNaturalJoin()) {
    // since the common column references need fetch histograms, the where
    // flag is set here so that when we call markAsReferencedColumn()
    // in the joinCommoncolumns() method it would set the common
    // columns as refenced by looking a the inWhereCaluse_ flag.
    NABoolean orig = bindWA->getCurrentScope()->context()->inWhereClause();
    bindWA->getCurrentScope()->context()->inWhereClause() = TRUE;
    joinPredx = joinCommonColumns(leftRelExpr, rightRelExpr, bindWA);
    bindWA->getCurrentScope()->context()->inWhereClause() = orig;
  }
  else
    joinPredx = removeJoinPredTree();

  if (joinPredx) {
    ItemExpr *saveInJP = bindWA->getCurrentScope()->context()->inJoinPred();
    bindWA->getCurrentScope()->context()->inJoinPred() = joinPredx;
    RETDesc preJoinResult;
    preJoinResult.addColumns(bindWA, *leftTable);
    preJoinResult.addColumns(bindWA, *rightTable);
    bindWA->getCurrentScope()->setRETDesc(&preJoinResult);
    joinPredx->convertToValueIdSet(joinPred(), bindWA, ITM_AND);
    bindWA->getCurrentScope()->context()->inJoinPred() = saveInJP;
    if (bindWA->errStatus()) return this;
  }
  //
  // Create the output list.
  // The TRUE's in the nullInstantiate() force a Cast expression to be set up,
  // as required by the Normalizer.
  //
  NABoolean newTables = TRUE;
  ValueIdList &nullOutputList = nullInstantiatedOutput();
  ValueIdList &nullOutputForRightJoinList = nullInstantiatedForRightJoinOutput();

  switch(getOperatorType()) {
  case REL_LEFT_JOIN:
    leftTable  = new (bindWA->wHeap()) RETDesc(bindWA, *leftTable);
    rightTable = rightTable->nullInstantiate(bindWA, TRUE, nullOutputList);
    break;
  case REL_RIGHT_JOIN:
    leftTable  = leftTable->nullInstantiate(bindWA,  TRUE, nullOutputList);
    rightTable = new (bindWA->wHeap()) RETDesc(bindWA, *rightTable);
    break;
  case REL_FULL_JOIN:
  case REL_UNION_JOIN:
    {
    leftTable  = leftTable->nullInstantiate(bindWA,  TRUE, nullOutputForRightJoinList);
    
    rightTable = rightTable->nullInstantiate(bindWA, TRUE, nullOutputList);

    
    // comp_bool_198 = 'on' enables FullOuter transformation
    // inner, left or right
    if (CmpCommon::getDefault(COMP_BOOL_198) == DF_OFF) //don't enable FOJ transformation
      {
      ItemExpr * instNull = NULL;
      CollIndex index = 0; 
      
      // disable the FOJ Transformation.
      for (index = 0; index < nullInstantiatedOutput().entries(); index++)
        {
          instNull = nullInstantiatedOutput()[index].getItemExpr();
          CMPASSERT(instNull->getOperatorType() == ITM_INSTANTIATE_NULL);
          ((InstantiateNull *)instNull)->NoCheckforLeftToInnerJoin = TRUE;
        } // endfor
      
      instNull = NULL;
      
      for (index = 0; 
        index < nullInstantiatedForRightJoinOutput().entries(); index++)
        {
          instNull = nullInstantiatedForRightJoinOutput()[index].getItemExpr();
          CMPASSERT(instNull->getOperatorType() == ITM_INSTANTIATE_NULL);
          ((InstantiateNull *)instNull)->NoCheckforLeftToInnerJoin = TRUE;
        } // endfor
      } // env "ENABLE_FOJ_TRANSFORMATION"
    break;
    }
  case REL_JOIN:
  default:
    newTables = FALSE;
    break;
  }
  RETDesc *resultTable = new (bindWA->wHeap()) RETDesc(bindWA);

  Int32 rowSet = (child(0)->getOperatorType() == REL_RENAME_TABLE) &&
               (child(0)->child(0)->getOperatorType() == REL_UNPACKROWS) &&
               (child(1)->getOperatorType() == REL_ROOT);

  if (NOT isNaturalJoin()) {
    if ((!rowSet) &&
        (getOperatorType() != REL_TSJ_FLOW)) {
      resultTable->addColumns(bindWA, *leftTable);
    }
    // ++MV  -- bug fixing for semi-joins
    if (!isSemiJoin())
    {
      resultTable->addColumns(bindWA, *rightTable);
    }
    // --MV  -- bug fixing for semi-joins
  } else {
    coalesceCommonColumns(bindWA,
                          getOperatorType(),
                          *leftTable,
                          *rightTable,
                          *resultTable);
    if (bindWA->errStatus()) return this;
  }
  setRETDesc(resultTable);
  bindWA->getCurrentScope()->setRETDesc(resultTable);

  // QSTUFF
  NAString fmtdList(bindWA->wHeap());
  LIST(TableNameMap*) xtnmList(bindWA->wHeap());

  bindWA->getTablesInScope(xtnmList, &fmtdList);

  if ((child(0)->getGroupAttr()->isStream()) &&
      (child(1)->getGroupAttr()->isStream())){
    bindWA->getTablesInScope(xtnmList, &fmtdList);
    *CmpCommon::diags() << DgSqlCode(-4158)
      << DgString0(fmtdList);
    bindWA->setErrStatus();
    return this;
  }

  // Disallowing joins for EMBEDDED...INSERT
  // 
  if (getGroupAttr()->isEmbeddedInsert() && 
      !isTSJForWrite()   // the tsjForWrite flag is set for
                         // those joins which are created by
                         // the Binder during inlining (eg. IndexMaintanence)
                         // Here we only want to disable user specified joins
                         // and not joins introduced as part of inlining.
      ){
      *CmpCommon::diags() << DgSqlCode(-4336)
        << DgString0(fmtdList)
        << DgString1(getGroupAttr()->getOperationWithinGroup());
      bindWA->setErrStatus();
      return this;
    }


  if ( ((child(0)->getGroupAttr()->isEmbeddedUpdateOrDelete()) &&
        (child(1)->getGroupAttr()->isEmbeddedUpdateOrDelete())) ||
       ((child(0)->getGroupAttr()->isEmbeddedInsert()) &&
        (child(1)->getGroupAttr()->isEmbeddedInsert())) ||
       (bindWA->isEmbeddedIUDStatement()) ) {
    NAString type0,type1;
    if (child(0)->getGroupAttr()->isEmbeddedUpdate())
      type0 = "UPDATE";
    else
    {
      if (child(0)->getGroupAttr()->isEmbeddedInsert())
        type0 = "INSERT";
      else
        type0 = "DELETE";
    }
    if (child(1)->getGroupAttr()->isEmbeddedUpdate())
      type1 = "UPDATE";
    else
    {
      if (child(1)->getGroupAttr()->isEmbeddedInsert())
        type1 = "INSERT";
      else
        type1 = "DELETE";
    }
    *CmpCommon::diags() << DgSqlCode(-4175)
      << DgString0(fmtdList)
      << DgString1(type0)
      << DgString2(type1);
    bindWA->setErrStatus();
    return this;
  }

  if ((child(0)->getGroupAttr()->isEmbeddedUpdateOrDelete() ||
       child(0)->getGroupAttr()->isStream()) &&
      (child(1)->getGroupAttr()->isEmbeddedUpdateOrDelete() ||
       child(1)->getGroupAttr()->isStream())){
    *CmpCommon::diags() << DgSqlCode(-4176)
      << DgString0(fmtdList)
      << (getGroupAttr()->isEmbeddedUpdate() ?
          DgString1("UPDATE"):DgString1("DELETE"));
    bindWA->setErrStatus();
    return this;
  }

  if (getOperatorType() == REL_LEFT_JOIN){
    if (child(1)->getGroupAttr()->isEmbeddedUpdateOrDelete()){
      *CmpCommon::diags() << DgSqlCode(-4156)
        << DgString0(fmtdList)
        << (child(1)->getGroupAttr()->isEmbeddedUpdate() ?
            DgString1("UPDATE"):DgString1("DELETE"));
      bindWA->setErrStatus();
      return this;
    }
    if (child(1)->getGroupAttr()->isStream()){
      *CmpCommon::diags() << DgSqlCode(-4157)
        << DgString0(fmtdList);
      bindWA->setErrStatus();
      return this;
    }
  }

  if (getOperatorType() == REL_RIGHT_JOIN){
    if (child(0)->getGroupAttr()->isEmbeddedUpdateOrDelete()){
      *CmpCommon::diags() << DgSqlCode(-4164)
        << DgString0(fmtdList)
        << (child(0)->getGroupAttr()->isEmbeddedUpdate() ?
            DgString1("UPDATE"):DgString1("DELETE"));
      bindWA->setErrStatus();
      return this;
    }
    if (child(0)->getGroupAttr()->isStream()){
      *CmpCommon::diags() << DgSqlCode(-4165)
        << DgString0(fmtdList);
      bindWA->setErrStatus();
      return this;
    }
  }

  // we need to move stream and nested updates to the
  // left to ensure correct execution. This causes the statement
  // to be rejected if the user specified join_order_by_user and
  // the query must be reordered

  if (child(1)->getGroupAttr()->isStream() ||
    child(1)->getGroupAttr()->isEmbeddedUpdateOrDelete()){
    getGroupAttr()->setReorderNeeded(TRUE);
  }
  // QSTUFF

  if (newTables) {
    delete leftTable;
    delete rightTable;
  }

  bindWA->getCurrentScope()->context()->inJoin() = saveInJ;

  if (getOperatorType() == REL_TSJ){
    //Using rowsets in a predicate with embedded update/delete results
    //in a NestedJoin subtree after Normalization.This NestedJoin subtree
    //has embedded update/delete as the right child, which is not allowed
    //during optimization. Here we try to disallow this usage at Binding
    //when a REL_TSJ subtree has rowsets as the left child and embedded
    //update/delete as the right child. An error message[4123] is signaled.
    if (rowSet && getGroupAttr()->isEmbeddedUpdateOrDelete()){
      *CmpCommon::diags() << DgSqlCode(-4213);
      bindWA->setErrStatus();
      return this;
    }
  }

  // transfer rowsetRowCountArraySize from HostArrayWA to this node.
  if (bindWA->getHostArraysArea() && isRowsetIterator())
    setRowsetRowCountArraySize(bindWA->getHostArraysArea()->getRowsetRowCountArraySize());

  // Bind the base class.
  //
  return bindSelf(bindWA);
} // Join::bindNode()

ValueId Join::addNullInstIndicatorVar(BindWA *bindWA,
                                      ItemExpr *indicatorVal)
{
  // Add an indicator variable that can tell us whether
  // a left join found a match in the right child table
  // or not. The returned ValueId will have the value 1
  // if a match was found, and NULL if no match was found.

  ItemExpr *cval = indicatorVal;

  if (!cval)
    cval = new (bindWA->wHeap()) SystemLiteral(1);
  cval = cval->bindNode(bindWA);
  if (bindWA->errStatus())
    return NULL_VALUE_ID;

  // Null instantiate the value.
  ValueId niCval = cval->getValueId().nullInstantiate(bindWA, TRUE);

  // Add it to the RETDesc of the Join.
  ColRefName cvalName("", bindWA->wHeap());
  getRETDesc()->addColumn(bindWA, cvalName , niCval, USER_COLUMN);

  // Add it to the list of null instantiated outputs.
  nullInstantiatedOutput().insert(niCval);

  return niCval;
}

//++MV
// This function builds the BalueIdMap that is used for translating the required
// sort key to the right child sort key and backwards
void Join::BuildRightChildMapForLeftJoin()
{
  ValueIdMap &map = rightChildMapForLeftJoin();

  for (CollIndex j = 0; j < nullInstantiatedOutput().entries(); j++)
  {
    ValueId instNullId, rightChildId;

    instNullId = nullInstantiatedOutput_[j];

    assert(instNullId.getItemExpr()->getOperatorType() == ITM_INSTANTIATE_NULL);

    // Access the operand of the InstantiateNull
    rightChildId = (((InstantiateNull *)(instNullId.getItemExpr()))->getExpr()->getValueId());

    map.addMapEntry(instNullId, rightChildId);
  }
}
//--MV

//++MV
// This function builds the ValueIdMap that is used for translating the 
// required
// sort key to the left  child sort key and backwards
void Join::BuildLeftChildMapForRightJoin()
{
  ValueIdMap &map = leftChildMapForRightJoin();

  for (CollIndex j = 0; j < nullInstantiatedForRightJoinOutput().entries(); j++)
  {
    ValueId instNullId, leftChildId;

    instNullId = nullInstantiatedForRightJoinOutput_[j];

    assert(instNullId.getItemExpr()->getOperatorType() == ITM_INSTANTIATE_NULL);

    // Access the operand of the InstantiateNull
    leftChildId = (((InstantiateNull *)(instNullId.getItemExpr()))->getExpr()->getValueId());

    map.addMapEntry(instNullId, leftChildId);
  }
}
//--MV

// -----------------------------------------------------------------------
// member functions for class Intersect
// -----------------------------------------------------------------------

RelExpr *Intersect::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // Bind the child nodes.
  //
  bindChildren(bindWA);
  if (bindWA->errStatus()) return this;

  // Check that there are an equal number of select items on both sides.
  //
  const RETDesc &leftTable = *child(0)->getRETDesc();
  const RETDesc &rightTable = *child(1)->getRETDesc();
  if (leftTable.getDegree() != rightTable.getDegree()) {
    // 4014 The operands of an intersect must be of equal degree.
    *CmpCommon::diags() << DgSqlCode(-4014);
    bindWA->setErrStatus();
    return this;
  }

  // Join the columns of both sides. 
  //
  if(CmpCommon::getDefault(MODE_SPECIAL_4) != DF_ON)
  {
    *CmpCommon::diags() << DgSqlCode(-3022)    // ## INTERSECT not yet supported , not fully tested
        << DgString0("INTERSECT");             // ##
    bindWA->setErrStatus();                    // ##
    if (bindWA->errStatus()) return NULL;      // ##
  }
  //
  ItemExpr *predicate = intersectColumns(leftTable, rightTable, bindWA);
  RelExpr *join = new (bindWA->wHeap())
    Join(child(0)->castToRelExpr(),
         child(1)->castToRelExpr(),
         REL_JOIN,
         predicate);

  // Bind the join.
  //
  join = join->bindNode(bindWA)->castToRelExpr();
  if (bindWA->errStatus()) return join;

  // Change the output of the join to just the left side.
  //
  delete join->getRETDesc();
  join->setRETDesc(new (bindWA->wHeap()) RETDesc(bindWA, leftTable));
  bindWA->getCurrentScope()->setRETDesc(join->getRETDesc());

  // QSTUFF
  NAString fmtdList1(bindWA->wHeap());
  LIST(TableNameMap*) xtnmList1(bindWA->wHeap());
  NAString fmtdList2(bindWA->wHeap());
  LIST(TableNameMap*) xtnmList2(bindWA->wHeap());

  leftTable.getTableList(xtnmList1, &fmtdList1);
  rightTable.getTableList(xtnmList2, &fmtdList2);

  if (child(0)->getGroupAttr()->isStream() &&
      child(1)->getGroupAttr()->isStream()){
    *CmpCommon::diags() << DgSqlCode(-4159)
                << DgString0(fmtdList1) << DgString1(fmtdList2);
    bindWA->setErrStatus();
    return this;
  }

  // Needs to be removed when supporting get_next for INTERSECT
  if (getGroupAttr()->isEmbeddedUpdateOrDelete()) {
    *CmpCommon::diags() << DgSqlCode(-4160)
                << DgString0(fmtdList1)
                << DgString1(fmtdList2)
                << (child(0)->getGroupAttr()->isEmbeddedUpdate() ?
                             DgString2("UPDATE"):DgString2("DELETE"))
                << (child(1)->getGroupAttr()->isEmbeddedUpdate() ?
                             DgString3("UPDATE"):DgString3("DELETE"));
    bindWA->setErrStatus();
    return this;
  }
  // QSTUFF

  return join;
} // Intersect::bindNode()

// -----------------------------------------------------------------------
// member functions for class Except 
// -----------------------------------------------------------------------

RelExpr *Except::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // Bind the child nodes.
  //
  bindChildren(bindWA);
  if (bindWA->errStatus()) return this;

  // Check that there are an equal number of select items on both sides.
  //
  const RETDesc &leftTable = *child(0)->getRETDesc();
  const RETDesc &rightTable = *child(1)->getRETDesc();
  if (leftTable.getDegree() != rightTable.getDegree()) {
    // 4014 The operands of an intersect must be of equal degree.
    *CmpCommon::diags() << DgSqlCode(-4014);
    bindWA->setErrStatus();
    return this;
  }

  // Join the columns of both sides. 
  //
  if(CmpCommon::getDefault(MODE_SPECIAL_4) != DF_ON)
  {
    *CmpCommon::diags() << DgSqlCode(-3022)    // ## EXCEPT not yet supported: not fully tested
        << DgString0("EXCEPT");             // ##
    bindWA->setErrStatus();                    // ##
    if (bindWA->errStatus()) return NULL;      // ##
  }
  //
  ItemExpr *predicate = intersectColumns(leftTable, rightTable, bindWA);
  RelExpr *join = new (bindWA->wHeap())
    Join(child(0)->castToRelExpr(),
         child(1)->castToRelExpr(),
         REL_ANTI_SEMIJOIN,
         predicate);

  // Bind the join.
  //
  join = join->bindNode(bindWA)->castToRelExpr();
  if (bindWA->errStatus()) return join;

  // Change the output of the join to just the left side.
  //
  delete join->getRETDesc();
  join->setRETDesc(new (bindWA->wHeap()) RETDesc(bindWA, leftTable));
  bindWA->getCurrentScope()->setRETDesc(join->getRETDesc());

  // QSTUFF
  NAString fmtdList1(bindWA->wHeap());
  LIST(TableNameMap*) xtnmList1(bindWA->wHeap());
  NAString fmtdList2(bindWA->wHeap());
  LIST(TableNameMap*) xtnmList2(bindWA->wHeap());

  leftTable.getTableList(xtnmList1, &fmtdList1);
  rightTable.getTableList(xtnmList2, &fmtdList2);

  if (child(0)->getGroupAttr()->isStream() &&
      child(1)->getGroupAttr()->isStream()){
    *CmpCommon::diags() << DgSqlCode(-4159)
                << DgString0(fmtdList1) << DgString1(fmtdList2);
    bindWA->setErrStatus();
    return this;
  }

  // Needs to be removed when supporting get_next for EXCEPT
  if (getGroupAttr()->isEmbeddedUpdateOrDelete()) {
    *CmpCommon::diags() << DgSqlCode(-4160)
                << DgString0(fmtdList1)
                << DgString1(fmtdList2)
                << (child(0)->getGroupAttr()->isEmbeddedUpdate() ?
                             DgString2("UPDATE"):DgString2("DELETE"))
                << (child(1)->getGroupAttr()->isEmbeddedUpdate() ?
                             DgString3("UPDATE"):DgString3("DELETE"));
    bindWA->setErrStatus();
    return this;
  }
  // QSTUFF

  return join;
} // Excpet::bindNode()

// -----------------------------------------------------------------------
// member functions for class Union
// -----------------------------------------------------------------------

RelExpr *Union::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  //
  // Bind the conditional expression.
  //
  ItemExpr *condExprTree = removeCondExprTree();
  if (condExprTree)
  {
    condExprTree->convertToValueIdList(condExpr(), bindWA, ITM_ITEM_LIST);
    if (bindWA->errStatus()) {
      return NULL;
    }
  }

  //
  // Bind the triggered action exception expression.
  //
  ItemExpr *trigExprTree = removeTrigExceptExprTree();
  if (trigExprTree)
  {
    // the assumption in the binder (in Union::addValueIdUnion) is that 
    // unionMap_ count is always less than or equal to one but triggers 
    // code might increment this number during binding because of 
    // recursive triggers or triggers that are used more than once 
    // in the statement. This check fixes the unionMap_ for triggers. 
    if ((unionMap_ != NULL) && (unionMap_->count_ > 1))
    {
       unionMap_->count_--;
       unionMap_ = new (CmpCommon::statementHeap()) UnionMap;
    }

    trigExprTree->convertToValueIdList(trigExceptExpr(), bindWA, ITM_ITEM_LIST);
    if (bindWA->errStatus()) {
      return NULL;
    }
  }

  AssignmentStArea *assignArea = NULL;
  // We store a pointer to this Union node in the assignment statements area.
  // This is needed for compound statements project, in particular when we have
  // assignment statements within an IF statement
  if (getUnionForIF()) {
    assignArea = bindWA->getAssignmentStArea();
    setPreviousIF(assignArea->getCurrentIF());
    assignArea->setCurrentIF(this);
  }

  //
  // Bind the child nodes.
  //
  bindWA->getCurrentScope()->context()->inUnion() = TRUE;

  currentChild() = 0;
  child(0) = child(0)->bindNode(bindWA);

  if (bindWA->errStatus()) return this;

  // If we have assignment statements of compound statements, we need to get rid
  // of the value ids generated while binding the first child. Also, we create a
  // list of the value ids of the variables that are on the left side of a SET
  // statement
  if (getUnionForIF() && leftList() && assignArea) {
    assignArea->removeLastValueIds(leftList(), this);
  }

  if (getCondUnary()) {
    CollIndex leftDegree = child(0)->getRETDesc()->getDegree();

    ItemExpr *tupleExpr = new (bindWA->wHeap()) ConstValue();

    for (CollIndex i=0; i+1<leftDegree; i++) {
      ItemExpr *con = new (bindWA->wHeap()) ConstValue();

      ItemList *list = new (bindWA->wHeap()) ItemList(con, tupleExpr);

      tupleExpr = list;
    }

    RelExpr *tuple = new (bindWA->wHeap()) Tuple(tupleExpr);

    // create the selection predicate (1=0) for the Tuple node
    ItemExpr *predicate = new (bindWA->wHeap())
                            BiRelat(ITM_EQUAL,
                                  new (bindWA->wHeap()) ConstValue(1),
                                  new (bindWA->wHeap()) ConstValue(0));
    tuple->addSelPredTree(predicate);

    RelExpr *tupleRoot = new (bindWA->wHeap()) RelRoot(tuple);

    setChild (1, tupleRoot);
  }

  if (child(1)) {
    if (!(child(1)->getOperator().match(REL_ANY_TSJ))) {
      bindWA->getCurrentScope()->setRETDesc(NULL);
    }

    currentChild() = 1;
    child(1) = child(1)->bindNode(bindWA);
    if (bindWA->errStatus()) return this;

    // If we have assignment statements of compound statements,
    // we need to get rid of the value ids generated while binding
    // the second child
    if (getUnionForIF() && rightList() && assignArea) {
      assignArea->removeLastValueIds(rightList(), this);
    }
  }

  // check for & warn against UNIONs that have inconsistent access/lock modes.
  // flag "select * from t1 union select * from t2 for <access> mode"
  // with a warning that t1 and t2 may have inconsistent access/lock modes.
  checkAccessLockModes();

  //Copies the leftlist and rightlist this conditional union to the appropriate list of the
  //conditional union node pointed to by the previousIF argument.
  Union * previousIF = getPreviousIF();
  if (previousIF && getUnionForIF()) {
     copyLeftRightListsToPreviousIF(previousIF, bindWA);
  }

  synthPropForBindChecks();
                                              // QSTUFF

  bindWA->getCurrentScope()->context()->inUnion() = FALSE;

  //
  // Check that there are an equal number of select items on both sides.
  //
  const RETDesc &leftTable = *child(0)->getRETDesc();
  const RETDesc &rightTable = *child(1)->getRETDesc();
  RETDesc *resultTable = NULL;

  RelRoot * root = bindWA->getTopRoot() ;
  if (root) {
     if (getGroupAttr()->isStream() && root->hasOrderBy()){
       NAString fmtdList1(bindWA->wHeap());
       LIST(TableNameMap*) xtnmList1(bindWA->wHeap());
       NAString fmtdList2(bindWA->wHeap());
       LIST(TableNameMap*) xtnmList2(bindWA->wHeap());
       leftTable.getTableList(xtnmList1, &fmtdList1);
       rightTable.getTableList(xtnmList2, &fmtdList2);
       *CmpCommon::diags() << DgSqlCode(-4166)
                           << DgString0(fmtdList1)
                           << DgString1(fmtdList2) ;
       bindWA->setErrStatus();
       return this;
     }
  }

  if (leftTable.getDegree() != rightTable.getDegree()) {

#ifndef NDEBUG
    dumpChildrensRETDescs(leftTable, rightTable);
#endif

    if ( (!getUnionForIF()) &&
         (!getCondUnary()) //for triggers
       ) {
      // 4126 The row-value-ctors of a VALUES must be of equal degree.
      // 4066 The operands of a union must be of equal degree.
      // This is not necessary if we are in an assignment stmt.
      Lng32 sqlcode = bindWA->getCurrentScope()->context()->inTupleList() ?
                                                            -4126 : -4066;
      *CmpCommon::diags() << DgSqlCode(sqlcode);
      bindWA->setErrStatus();
      return this;
    }
  }

  //
  // For each select item on both sides, create a ValueIdUnion and insert its
  // ValueId into the select list for the union.
  //

  // We check to see if there were assignments on either side
  if  ( !getUnionForIF() ) {
    resultTable = new (bindWA->wHeap()) RETDesc(bindWA);
    for (CollIndex i = 0; i < leftTable.getDegree(); i++) {
      ValueIdUnion *vidUnion = new (bindWA->wHeap())
      ValueIdUnion(leftTable.getValueId(i),
                   rightTable.getValueId(i),
                   NULL_VALUE_ID,
                   getUnionFlags());
      vidUnion->setIsTrueUnion(TRUE);
      vidUnion->bindNode(bindWA);
      if (bindWA->errStatus()) {
        delete vidUnion;
        delete resultTable;
        return this;
      }
      ValueId valId = vidUnion->getValueId();
      addValueIdUnion(valId, bindWA->wHeap());
      resultTable->addColumn(bindWA, leftTable.getColRefNameObj(i), valId);
    }
  }
  else {
    // Case in which we have asignment statements below this node.
    // We have to carefuly match the valueids in the IF and ELSE part.
    // For instance, if SET :a = ... occurs in both branches or only in one.
    if (getUnionForIF() && assignArea) {
      resultTable = createReturnTable(assignArea, bindWA);
    }
  }


  setRETDesc(resultTable);
  bindWA->getCurrentScope()->setRETDesc(resultTable);
  //
  // Bind the base class.
  //

  // We are done binding this node. The current IF node is now the closest
  // IF node that is also an ancestor of this node
  if (getUnionForIF() && assignArea) {
    assignArea->setCurrentIF(getPreviousIF());
  }

  // QSTUFF
  // this is not a hard restriction. Once the get_next protocol supports unions
  // similar to the split-top operator, this check can be removed.
  if (getGroupAttr()->isEmbeddedUpdateOrDelete() ||
      (getGroupAttr()->isEmbeddedInsert() && !isSystemGenerated_) ||
      (bindWA->isEmbeddedIUDStatement())) {
    if (getUnionForIF()) {
      *CmpCommon::diags() << DgSqlCode(-4210);
      bindWA->setErrStatus();
      return this;
    }

  NAString fmtdList1(bindWA->wHeap());
    LIST(TableNameMap*) xtnmList1(bindWA->wHeap());
  NAString fmtdList2(bindWA->wHeap());
    LIST(TableNameMap*) xtnmList2(bindWA->wHeap());

    leftTable.getTableList(xtnmList1, &fmtdList1);
    rightTable.getTableList(xtnmList2, &fmtdList2);

    // Fix for Solution 10-070117-1834. 
    // Error Message for -4161 - assumes that both sides
    // of the UNION is an embedded operation. For a
    // query such as, 
    // select * from (delete from t709t1)as x union all (select * from t709t1)
    // the right side of the UNION is not an embedded operation.
    // Hence, changing the text for 4161 to a more generic one so
    // that all cases are covered in this one text message.


    *CmpCommon::diags() << DgSqlCode(-4161)
      << DgString0(fmtdList1)
      << DgString1(fmtdList2);

    bindWA->setErrStatus();
    return this;
  }
  // QSTUFF


  // ++MV
  // Bind the alternateRightChildOrderExprTree expression.
  //
  ItemExpr *alternateRightChildOrderExprTree = removeAlternateRightChildOrderExprTree();
  if (alternateRightChildOrderExprTree)
  {
    alternateRightChildOrderExprTree->
      convertToValueIdList(alternateRightChildOrderExpr(), bindWA, ITM_ITEM_LIST);
    if (bindWA->errStatus()) {
      return NULL;
    }
  }
  // --MV

  // Bind the base class.
  //
  RelExpr *boundExpr = bindSelf(bindWA);
  if (bindWA->errStatus()) {
    delete resultTable;
    return boundExpr;
  }
  return boundExpr;
} // Union::bindNode()

// check for & warn against UNIONs that have inconsistent access/lock modes
void Union::checkAccessLockModes()
{
  Scan *left = child(0)->getAnyScanNode();
  Scan *right = child(1)->getAnyScanNode(); 
  
  if (!left || !right) return; // no-op.

  // UNION is user-specified as opposed to system-generated (eg, by
  // triggers/RI in GenericUpdate::inlinePipelineActions, etc)
  if (isSystemGenerated_) {
    return;
  }
  
  Lng32 lockFlagSession = CmpCommon::transMode()->getDP2LockFlags().getValue();
  StmtLevelAccessOptions optionsLeft = left->accessOptions();
  StmtLevelAccessOptions optionsRight = right->accessOptions();

  Lng32 lockFlagLeft = lockFlagSession;
  Lng32 lockFlagRight = lockFlagSession;

  if (optionsLeft.userSpecified()) {
    lockFlagLeft = optionsLeft.getDP2LockFlags().getValue();
  }
  if (optionsRight.userSpecified()) {
    lockFlagRight = optionsRight.getDP2LockFlags().getValue();
  }
  if (lockFlagLeft != lockFlagRight) {
    *CmpCommon::diags()
      << DgSqlCode(3192)
      << DgString0(left->getTableName().getQualifiedNameAsString())
      << DgString1(right->getTableName().getQualifiedNameAsString());
  }
} // Union::checkAccessLockModes()

void Union::copyLeftRightListsToPreviousIF(Union * previousIF, BindWA * bindWA)
{
  AssignmentStHostVars *thisLeftList =  leftList();
  AssignmentStHostVars *thisRightList = rightList();

  // If the previous IF node does not have a left list, we copy the left and right
  // lists to that left list
  if (previousIF->currentChild() == 0 && !(previousIF->leftList())) {
      AssignmentStHostVars *leftListOfPreviousIF = previousIF->getCurrentList(bindWA);
      // Copy the leftList of this node to the left list of the previous IF
      leftListOfPreviousIF->addAllToListInIF(thisLeftList) ;
      // Copy the rightList of this node to the left list of the previous IF
      leftListOfPreviousIF->addAllToListInIF(thisRightList) ;
  }

  // If the previous IF node does not have a right list, we copy the left and right
  // lists to that left list
  if (previousIF->currentChild() == 1 && !(previousIF->rightList())) {
      AssignmentStHostVars *rightListOfPreviousIF = previousIF->getCurrentList(bindWA);
      // Copy the leftList of this node to the right list of the previous IF
      rightListOfPreviousIF->addAllToListInIF(thisLeftList) ;
      // Copy the rightList of this node to the right list of the previous IF
      rightListOfPreviousIF->addAllToListInIF(thisRightList) ;
  }
} // Union::copyLeftRightListsToPreviousIF

// -----------------------------------------------------------------------
// MV --
// A debugging method for dumping the columns in the RETDesc of both
// children when they do not match.
void Union::dumpChildrensRETDescs(const RETDesc& leftTable,
                                  const RETDesc& rightTable)
{
// turn this code on when you need it by changing the #if below
#if 0   
  // -- MVs. Debugging code !!!!! TBD
  fprintf(stdout, " #    Left                                Right\n");
  CollIndex maxIndex, minIndex;
  NABoolean leftIsBigger;
  if (leftTable.getDegree() > rightTable.getDegree())
  {
    maxIndex = leftTable.getDegree();
    minIndex = rightTable.getDegree();
    leftIsBigger = TRUE;
  }
  else
  {
    maxIndex = rightTable.getDegree();
    minIndex = leftTable.getDegree();
    leftIsBigger = FALSE;
  }

  for (CollIndex i=0; i<minIndex; i++)
  {
    ColumnDesc *leftColDesc  = leftTable.getColumnList()->at(i);
    ColumnDesc *rightColDesc = rightTable.getColumnList()->at(i);
    NAString leftCol (leftColDesc->getColRefNameObj().getColRefAsString());
    NAString rightCol(rightColDesc->getColRefNameObj().getColRefAsString());
    fprintf(stdout, " %3d  %-55s %-55s \n",
            i, leftCol.data(), rightCol.data());
  }

  if (leftIsBigger)
  {
    for (CollIndex j=minIndex; j<maxIndex; j++)
    {
      ColumnDesc *leftColDesc  = leftTable.getColumnList()->at(j);
      NAString leftCol(leftColDesc->getColRefNameObj().getColRefAsString());
      fprintf(stdout, " %3d  %-35s\n",
              j, leftCol.data());
    }
  }
  else
  {
    for (CollIndex k=minIndex; k<maxIndex; k++)
    {
      ColumnDesc *rightColDesc = rightTable.getColumnList()->at(k);
      NAString rightCol(rightColDesc->getColRefNameObj().getColRefAsString());
      fprintf(stdout, " %3d                                      %-35s \n",
              k, rightCol.data());
    }
  }
#endif
}


// ----------------------------------------------------------------------
// static helper functions for classes RelRoot and GroupByAgg
// ----------------------------------------------------------------------

static NABoolean containsGenericUpdate(const RelExpr *re)
{
  if (re->getOperator().match(REL_ANY_GEN_UPDATE)) return TRUE;
  for (Int32 i = 0; i < re->getArity(); ++i ) {
    if (re->child(i) && containsGenericUpdate(re->child(i))) return TRUE;
  }
  return FALSE;
}

static NABoolean containsUpdateOrDelete(const RelExpr *re)
{
  if (re->getOperator().match(REL_ANY_UPDATE_DELETE))
    return TRUE;
  for (Int32 i = 0; i < re->getArity(); ++i ) {
    if (re->child(i) && containsUpdateOrDelete(re->child(i)))
      return TRUE;
  }
  return FALSE;
}

// QSTUFF

static GenericUpdate *getGenericUpdate(RelExpr *re)
{
  if (re) {
    if (re->getOperatorType() == REL_UNARY_UPDATE ||
        re->getOperatorType() == REL_UNARY_DELETE)
      return (GenericUpdate *)re;

    for (Int32 i = 0; i < re->getArity(); ++i) {    // check all children (both sides)
      GenericUpdate *gu = getGenericUpdate(re->child(i));
      if (gu) return gu;
    }
  }
  return NULL;
}

static NABoolean checkUnresolvedAggregates(BindWA *bindWA)
{
  const ValueIdSet &aggs = bindWA->getCurrentScope()->getUnresolvedAggregates();
  if (aggs.isEmpty()) return FALSE;         // no error

  NAString unparsed(bindWA->wHeap());
  for (ValueId vid = aggs.init(); aggs.next(vid); aggs.advance(vid)) {

    const ItemExpr *ie = vid.getItemExpr();
    CMPASSERT(ie->isAnAggregate());
    Aggregate *agg = (Aggregate *)ie;

  // Don't display COUNT() part of SUM()/COUNTxxx(), our implementation of AVG()
  // Display only the COUNT_NONULL() our implementation of VARIANCE and STDDEV
  // This is to avoid printing the aggregate functions more than once.

    if((agg->origOpType() != ITM_AVG || agg->getOperatorType() == ITM_SUM) &&
       (!(agg->origOpType() == ITM_STDDEV || agg->origOpType() == ITM_VARIANCE)
       || agg->getOperatorType() == ITM_COUNT_NONULL)){

      unparsed += ", ";
      if (agg->origOpType() == ITM_COUNT_STAR__ORIGINALLY)
        unparsed += "COUNT(*)";
      else
        agg->unparse(unparsed, DEFAULT_PHASE, USER_FORMAT_DELUXE);
    }
  }
  unparsed.remove(0,2);              // remove initial ", "

  // 4015 Aggregate functions placed incorrectly.
  *CmpCommon::diags() << DgSqlCode(-4015) << DgString0(unparsed);
  bindWA->setErrStatus();
  return TRUE;
} // checkUnresolvedAggregates()

// ----------------------------------------------------------------------
// member functions for class RelRoot
// ----------------------------------------------------------------------


static NABoolean isRenamedColInSelList(BindWA * bindWA, ItemExpr * col, 
                                       ItemExprList &origSelectList,
                                       CollIndex &indx,
                                       RETDesc * childRETDesc)
{
  if (col->getOperatorType() != ITM_REFERENCE)
    return FALSE;

  ColReference * havingColReference = (ColReference*)col;
  
  CollIndex j = 0;

  NABoolean found = FALSE;
  while (j < origSelectList.entries())
    {
      ItemExpr * selectListEntry = origSelectList[j];
      
      if (selectListEntry->getOperatorType() == ITM_RENAME_COL)
        {
          const ColRefName &selectListColRefName = 
            *((RenameCol *)selectListEntry)->getNewColRefName();
          
          if (havingColReference->getColRefNameObj() == selectListColRefName)
            {
              if (found)
                {
                  // multiple entries with the same name. Error.
                  *CmpCommon::diags() << DgSqlCode(-4195) 
                                      << DgString0(selectListColRefName.getColName());
                  bindWA->setErrStatus();
                  return FALSE;
                }

              ColumnNameMap *baseColExpr = NULL;
              if (childRETDesc)
                baseColExpr = childRETDesc->findColumn(selectListColRefName);
              if ( NOT baseColExpr)
              { 
                found = TRUE;
                indx = j;
              }
            }
        } // rename col
      
      j++;
    } // while
  return found;
}

static short replaceRenamedColInHavingWithSelIndex(
                                     BindWA * bindWA,
                                     ItemExpr * expr, 
                                     ItemExprList &origSelectList,
                                     NABoolean &replaced,
                                     NABoolean &notAllowedWithSelIndexInHaving,
                                     RETDesc * childRETDesc)
{
  if (((expr->getOperatorType() >= ITM_ROW_SUBQUERY) &&
       (expr->getOperatorType() <= ITM_GREATER_EQ_ANY)) ||
      ((expr->getOperatorType() >= ITM_AVG) &&
       (expr->getOperatorType() <= ITM_VARIANCE)) ||
      ((expr->getOperatorType() >= ITM_DIFF1) &&
       (expr->getOperatorType() <= ITM_NOT_THIS)))
    {
      notAllowedWithSelIndexInHaving = TRUE;
      return 0;
    }

  for (Int32 i = 0; i < expr->getArity(); i++)
    {
      CollIndex j = 0;
      if (isRenamedColInSelList(bindWA, expr->child(i), origSelectList, 
                                j, childRETDesc))
        {
          SelIndex * selIndex = new(bindWA->wHeap()) SelIndex(j+1);
          expr->setChild(i, selIndex);
          replaced = TRUE;
        }
      else if (bindWA->errStatus())
        return -1;
      else if (replaceRenamedColInHavingWithSelIndex(
                              bindWA, expr->child(i), origSelectList, replaced,
                              notAllowedWithSelIndexInHaving, childRETDesc))
        return -1;
    }
  return 0;
}

static short setValueIdForRenamedColsInHaving(BindWA * bindWA,
                                              ItemExpr * expr, 
                                              ValueIdList &compExpr)
{
  if (((expr->getOperatorType() >= ITM_ROW_SUBQUERY) &&
       (expr->getOperatorType() <= ITM_GREATER_EQ_ANY)) ||
      ((expr->getOperatorType() >= ITM_AVG) &&
       (expr->getOperatorType() <= ITM_VARIANCE)) ||
      ((expr->getOperatorType() >= ITM_DIFF1) &&
       (expr->getOperatorType() <= ITM_NOT_THIS)))
    {
      return 0;
    }

  for (Int32 i = 0; i < expr->getArity(); i++)
    {
      if (expr->child(i)->getOperatorType() == ITM_SEL_INDEX)
        {
          SelIndex * si = (SelIndex*)expr->child(i)->castToItemExpr();
          si->setValueId(compExpr[si->getSelIndex()-1]);
        }
      else
        setValueIdForRenamedColsInHaving(bindWA, expr->child(i), compExpr);
    }
  return 0;
}

// Method to update the selIndecies after we have gone through a
// selectList expansion due to MVFs or Subqueries with degree > 1
// used to update the orderByTree
// 
// Returns a list of SelIndecies that were updated.
static void fixUpSelectIndecies(ItemExpr * expr, ValueIdSet &updatedIndecies,
                                CollIndex idx, CollIndex offset)
{

  if (expr == NULL ) return;

  for (Int32 i = 0; i < expr->getArity(); i++)
    {
      // Only update ones that we haven't already done.
      if ((expr->child(i)->getOperatorType() == ITM_SEL_INDEX) && 
           !updatedIndecies.contains(expr->child(i)->getValueId()))
        {
          SelIndex * si = (SelIndex*)expr->child(i)->castToItemExpr();
          if (si->getSelIndex() > idx)
          {
            si->setSelIndex(si->getSelIndex() + offset);
            updatedIndecies += si->getValueId();
          }
        }
      else
        fixUpSelectIndecies(expr->child(i), updatedIndecies, idx, offset);
    }

    // Now check myself..
    // Only update ones that we haven't already done.
    if ((expr->getOperatorType() == ITM_SEL_INDEX) && 
         !updatedIndecies.contains(expr->getValueId()))
      {
        SelIndex * si = (SelIndex*)expr->castToItemExpr();
        if (si->getSelIndex() > idx)
        {
          si->setSelIndex(si->getSelIndex() + offset);
          updatedIndecies += si->getValueId();
        }
      }
}

// Method to update the selIndecies after we have gone through a
// selectList expansion due to MVFs or Subqueries with degree > 1
// used to update the GroupByList
// 
// Returns a list of SelIndecies that were updated.
static void fixUpSelectIndeciesInSet(ValueIdSet & expr, 
                                ValueIdSet &updatedIndecies,
                                CollIndex idx, 
                                CollIndex offset)
{

  for (ValueId vid = expr.init(); expr.next(vid); expr.advance(vid))
    {
      // Only update ones that we haven't already done.
      if (((ItemExpr *)vid.getItemExpr())->getOperatorType() == ITM_SEL_INDEX &&
          !updatedIndecies.contains(vid))
        {
          SelIndex * si = (SelIndex*) vid.getItemExpr();
          if (si->getSelIndex() > idx)
          {
            si->setSelIndex(si->getSelIndex() + offset);
            updatedIndecies += si->getValueId();
          }
        }
    }
}

RelRoot * RelRoot::transformOrderByWithExpr(BindWA *bindWA)
{
  NABoolean specialMode = (CmpCommon::getDefault(GROUP_OR_ORDER_BY_EXPR) == DF_ON);
  if (NOT specialMode)
    return this;
  ItemExprList origSelectList(bindWA->wHeap());
  ItemExprList origOrderByList(bindWA->wHeap());
  
  CollIndex origSelectListCount ;
  if ((getCompExprTree() == NULL) &&
      (child(0)->getOperatorType() != REL_GROUPBY))
    {
      return this;
    }
  
  ItemExpr *orderByTree = getOrderByTree();
  if (!orderByTree) 
    return this;
  
  if (orderByTree)
    {
      origOrderByList.insertTree(orderByTree);
    }
  
  if (getCompExprTree())
    origSelectList.insertTree(getCompExprTree());
  else if (child(0)->getOperatorType() == REL_GROUPBY)
    {
      // this is the case:  select distinct <expr> from t order by <expr>
      GroupByAgg * grby = (GroupByAgg *)(child(0)->castToRelExpr());
      if (grby->child(0) && grby->child(0)->getOperatorType() == REL_ROOT)
        {
          RelRoot * selRoot = (RelRoot*)grby->child(0)->castToRelExpr();
          if (selRoot->getCompExprTree())
            origSelectList.insertTree(selRoot->getCompExprTree());
        }
    }
  
  Lng32 selListCount = origSelectList.entries();

  // if there is an expression in the order by list and this expression matches
  // a select list expression, then replace it with the index of that select list item.
  ItemExprList newOrderByList((Lng32)origOrderByList.entries(), bindWA->wHeap());
  NABoolean orderByExprFound = FALSE;
  for (Lng32 i = 0; i < origOrderByList.entries(); i++)
    {
      ItemExpr * currOrderByItemExpr = origOrderByList[i];

      NABoolean isDesc = FALSE;
      if (currOrderByItemExpr->getOperatorType() == ITM_INVERSE)
        {
          currOrderByItemExpr = currOrderByItemExpr->child(0)->castToItemExpr();
          isDesc = TRUE;
        }

      if (NOT ((currOrderByItemExpr->getOperatorType() == ITM_SEL_INDEX) ||
               (currOrderByItemExpr->getOperatorType() == ITM_REFERENCE) ||
               (currOrderByItemExpr->getOperatorType() == ITM_CONSTANT)))
        {
          NABoolean found = FALSE;
          Lng32 selListIndex = 0;
          ItemExpr * selItem = NULL;
          ItemExpr * renameColEntry = NULL;
          while ((NOT found) && (selListIndex < selListCount))
            {
              selItem = origSelectList[selListIndex];
              
              if (selItem->getOperatorType() == ITM_RENAME_COL)
                {
                  renameColEntry = selItem;
                  selItem = selItem->child(0);
                }
              
              found = currOrderByItemExpr->duplicateMatch(*selItem);
              if (NOT found)
                selListIndex++;
            }
          
          if (NOT found)
            {
              *CmpCommon::diags() << DgSqlCode(-4197) 
                                  << DgString0("ORDER BY");
              bindWA->setErrStatus();
              return NULL;
            }

          selItem->setInOrderByOrdinal(TRUE);
          currOrderByItemExpr = new(bindWA->wHeap()) SelIndex(selListIndex+1);
          if (isDesc)
            {
              currOrderByItemExpr = new(bindWA->wHeap()) InverseOrder(currOrderByItemExpr);
            }

          orderByExprFound = TRUE;
        } // if order by expr
      
      newOrderByList.insert(currOrderByItemExpr);
    }
  
  if ((orderByExprFound) &&
      (newOrderByList.entries() > 0))
    {
      removeOrderByTree();
      addOrderByTree(newOrderByList.convertToItemExpr());
    }

  return this;
}

//////////////////////////////////////////////////////////////////////
// GROUPING functions returns a 1 or 0 depending on whether a null
// value was moved as a rollup group or not.
//
// GROUPING_ID(a,b,c) returns a value corresponding to the bit vector 
// where each bit entry represents the GROUPING result for the argument
// of GROUPING_ID function.
//
// For ex: GROUPING_ID(a,b,c) will have 3 bit entries, 
// and is equivalent to:
//   GROUPING(a)*4 + GROUPING(b)*2 + GROUPING(c)*1
//////////////////////////////////////////////////////////////////////
ItemExpr * RelRoot::processGroupingID(ItemExpr * ie, BindWA *bindWA)
{
  if (ie->getOperatorType() != ITM_GROUPING_ID)
    return ie;

  ItemExpr * groupingIdExpr = NULL;

  ItemExprList childExprList(bindWA->wHeap());
  childExprList.insertTree(ie->child(0)->castToItemExpr());
  
  Int64 multiplier = (Int64)pow(2, (childExprList.entries()-1));
  SQLLargeInt * li = 
    new(bindWA->wHeap()) SQLLargeInt(bindWA->wHeap(), FALSE, FALSE); // +ve value, no nulls
  for (CollIndex i = 0; i < (CollIndex)childExprList.entries(); i++)
    {
      ItemExpr * currChildIE = 
        ((ItemExpr *) childExprList[i])->castToItemExpr();
      
      ItemExpr * groupingClause =
        new(bindWA->wHeap()) Aggregate(ITM_GROUPING, currChildIE, FALSE);
      
      ItemExpr * multiplierClause = new(bindWA->wHeap()) 
        ConstValue(li, (void*)&multiplier, sizeof(Int64));
      ItemExpr * groupingExpr = new(bindWA->wHeap()) 
        BiArith(ITM_TIMES, groupingClause, multiplierClause);
      
      if (i == 0)
        {
          groupingIdExpr = groupingExpr;
        }
      else
        {
          groupingIdExpr = new(bindWA->wHeap())
            BiArith(ITM_PLUS, groupingIdExpr, groupingExpr);
        }
      
      multiplier = multiplier / 2;
    }
  
  groupingIdExpr = new(bindWA->wHeap()) Cast(groupingIdExpr, li);

  return groupingIdExpr;
}

///////////////////////////////////////////////////////////////////////////
//
// This methods performs the following in this order:
//
// If groupby name refers to a renamed col name in the select list,
// replace group by entry with ordinal position of that sel list entry.
//
// If groupby ordinal exceeds the number of select list elements, 
// return error.
//
// If groupby ordinal referes to a '*', return error.
// 
// If groupby ordinal refers to a column(ITM_REFERENCE) or a renamed 
// col name(ITM_RENAME_COL) whose child is a column(ITM_REFERENCE),
// replace ordinal with actual col name.
//
// If there are ordinals in group by list, mark RelRoot indicating
// phase2 transformation is needed.
//
// Mark all select list item exprs which are referened as an ordinal to
// indicate that groupby check to validate grouping columns is not needed
// for the subtree rooted below that select list item.
//
///////////////////////////////////////////////////////////////////////////
RelRoot * RelRoot::transformGroupByWithOrdinalPhase1(BindWA *bindWA)
{
  NABoolean specialMode =
    (CmpCommon::getDefault(MODE_SPECIAL_1) == DF_ON);

  // make sure child of root is a groupby node.or a sequence node 
  // whose child is a group by node
  if (child(0)->getOperatorType() != REL_GROUPBY && 
      (child(0)->getOperatorType() != REL_SEQUENCE || 
       (child(0)->child(0) && child(0)->child(0)->getOperatorType()!=REL_GROUPBY)))
    return this;

  NABoolean compExprTreeIsNull = FALSE;
  CollIndex origSelectListCount ;
  if (getCompExprTree() == NULL)
  {
    compExprTreeIsNull = TRUE;
    origSelectListCount = 0;
   // return this;
  }

  GroupByAgg * grby;

  if (child(0)->getOperatorType() == REL_GROUPBY)
  {  
    grby = (GroupByAgg *)(child(0)->castToRelExpr());
  }
  else
  {// sequence node above group by
    grby = (GroupByAgg *)(child(0)->child(0)->castToRelExpr());
  }

  DCMPASSERT(grby != NULL);

  if ((NOT specialMode) &&
      (grby->getGroupExprTree() == NULL))
    return this;

  ItemExpr * groupExprTree  = grby->getGroupExprTree();
  ItemExprList origSelectList(bindWA->wHeap());
  ItemExprList origGrbyList(bindWA->wHeap());

  if (groupExprTree)
  {
    origGrbyList.insertTree(groupExprTree);
  }

  if (NOT compExprTreeIsNull)
  {
    // expand GROUPING_ID in terms of GROUPING aggregates
    if (grby->isRollup())
      {
        NABoolean groupingIDfound = FALSE;

        ItemExprList selList(getCompExprTree(), bindWA->wHeap());
        ItemExprList newSelList(bindWA->wHeap());
        for (CollIndex ii = 0; ii < selList.entries(); ii++)
          {
            ItemExpr * ie = selList[ii];
            if (ie->getOperatorType() == ITM_GROUPING_ID)
              {
                ItemExpr * newIE = processGroupingID(ie, bindWA);
                if (bindWA->errStatus())
                  return this;
                
                groupingIDfound = TRUE;
                newSelList.insert(newIE);
              }
            else
              newSelList.insert(ie);
          } // for

        if (groupingIDfound)
          {
            ItemExpr * newCompExprTree = newSelList.convertToItemExpr();
            removeCompExprTree();
            addCompExprTree(newCompExprTree);
          }
       }

    origSelectList.insertTree(getCompExprTree());
    origSelectListCount = origSelectList.entries();
  }

  ItemExprList newGroupByList((Lng32)origGrbyList.entries(), bindWA->wHeap());
  
  NABoolean foundSelIndex = FALSE;

  NABoolean lookForRenamedCols = TRUE;
  if ((CmpCommon::getDefault(GROUP_OR_ORDER_BY_EXPR) == DF_OFF) &&
      (NOT specialMode))
    lookForRenamedCols = FALSE;

  NABoolean lookForExprInGroupByClause = TRUE;
  if (CmpCommon::getDefault(COMP_BOOL_92) == DF_ON)
    lookForExprInGroupByClause = FALSE;

  // See if UDF_SUBQ_IN_AGGS_AND_GBYS is enabled. It is enabled if the
  // default is ON, or if the default is SYSTEM and ALLOW_UDF is ON.
  NABoolean udfSubqInAggGrby_Enabled = FALSE;
  DefaultToken udfSubqTok = CmpCommon::getDefault(UDF_SUBQ_IN_AGGS_AND_GBYS);
  if ((udfSubqTok == DF_ON) ||
      (udfSubqTok == DF_SYSTEM))
    udfSubqInAggGrby_Enabled = TRUE;

  // This list will store duplicate expression specified in  select list and 
  // GroupBy clause. It helps with specifying select Index  as well as 
  // mark InGroupByOrdinal flag correctly (Gen Sol:10-100129-7836)
  NAList<CollIndex> listOfExpressions(CmpCommon::statementHeap());

  for (CollIndex i = 0; (i < (CollIndex) origGrbyList.entries());i++)
    {
      ItemExpr * currGroupByItemExpr = 
        ((ItemExpr *) origGrbyList[i])->castToItemExpr();
      ItemExpr * newGroupByItemExpr = NULL;

      NABoolean selIndexError = FALSE;
      Int64 selIndex = -1;
      if (currGroupByItemExpr->getOperatorType() == ITM_CONSTANT)
        {
          ConstValue * cv = (ConstValue*)currGroupByItemExpr;
          if ((cv->canGetExactNumericValue()) &&
              (cv->getType()->getScale() == 0))
            {
              selIndex = cv->getExactNumericValue();
              if ((selIndex >= 0) && (selIndex < MAX_COMSINT32))
                {
                  if (selIndex == 0 || selIndex > origSelectListCount)
                    {
                      // remember that this select index is in error.
                      // Look for this constant in the select list.
                      // If it is not found, then this const will be
                      // treated as a select index and an error will 
                      // returned. If it is found in the select list,
                      // then it will be treated as a group by expression.
                      selIndexError = TRUE;
                    }
                  else
                    currGroupByItemExpr = 
                      new(bindWA->wHeap()) SelIndex((Lng32)selIndex);
                }
            }
        }

      NABoolean found = FALSE;
      if ((currGroupByItemExpr->getOperatorType() != ITM_REFERENCE) &&
          (currGroupByItemExpr->getOperatorType() != ITM_SEL_INDEX) &&
          (lookForExprInGroupByClause))
        {
           Int32 selListIndex = -1, lastMatch = -1;
          CollIndex j = 0;
          while ((NOT found) && (j < origSelectListCount))
            {
              ItemExpr * selectListEntry = origSelectList[j];
              
              if ((selectListEntry->getOperatorType() != ITM_REFERENCE) &&
                  ((selectListEntry->getOperatorType() != ITM_RENAME_COL) ||
                  ((selectListEntry->child(0)) && 
                  (selectListEntry->child(0)->getOperatorType() != ITM_REFERENCE))))
                {
                  ItemExpr * renameColEntry = NULL;
                  if (selectListEntry->getOperatorType() == ITM_RENAME_COL)
                    {
                      renameColEntry = selectListEntry;
                      selectListEntry = selectListEntry->child(0);
                    }
                  
                  found = 
                    currGroupByItemExpr->duplicateMatch(*selectListEntry);
                  if (found)
                    {
		      lastMatch = j;
		      if(!listOfExpressions.contains(j))
		      {
			selListIndex = j;
			listOfExpressions.insert(j);
			
                        selectListEntry->setInGroupByOrdinal(TRUE);
                        selectListEntry->setIsGroupByExpr(TRUE);

			if (renameColEntry)
			  renameColEntry->setInGroupByOrdinal(TRUE);
		      }
		      else
			found = FALSE;
                    }
                }
              j++;
            } // while

	    if(lastMatch != -1)
	    {
	      found = TRUE;

	      if(selListIndex == -1)
		selListIndex = lastMatch;

	      if (bindWA->inViewDefinition())
		currGroupByItemExpr = 
		  new(bindWA->wHeap()) SelIndex(selListIndex+1, 
						currGroupByItemExpr);
	      else
		currGroupByItemExpr = new(bindWA->wHeap()) SelIndex(selListIndex+1);
	    }
        } // expr in group by clause
      
      if ((NOT found) &&
          (selIndexError) &&
          (selIndex > 0))
        {
          // this const was not found in the select list and it was
          // not a valid select index.
          // Return an error.
          *CmpCommon::diags() << DgSqlCode(-4007) 
                              << DgInt0((Lng32)selIndex)
                              << DgInt1((Lng32)origSelectList.entries());
          bindWA->setErrStatus();
          return NULL;
        }
      if (compExprTreeIsNull)
        return this;

      if (currGroupByItemExpr->getOperatorType() == ITM_SEL_INDEX)
        {
          SelIndex * si = (SelIndex*)currGroupByItemExpr;
          if (si->getSelIndex() > origSelectList.entries())
            {
              *CmpCommon::diags() << DgSqlCode(-4007) 
                                  << DgInt0((Lng32)si->getSelIndex()) 
                                  << DgInt1((Lng32)origSelectList.entries());
              bindWA->setErrStatus();
              return NULL;
            }

          ItemExpr * selectListEntry = origSelectList[si->getSelIndex()-1];
          if ((selectListEntry->getOperatorType() == ITM_RENAME_COL) &&
              (selectListEntry->child(0)->getOperatorType() == ITM_REFERENCE))
            {
              // make a copy of this entry's child
              newGroupByItemExpr = 
                selectListEntry->child(0)->
                  castToItemExpr()->copyTopNode(NULL, bindWA->wHeap());
            }
          else if (selectListEntry->getOperatorType() == ITM_REFERENCE)
            {
              if (((ColReference*)selectListEntry)-> getColRefNameObj().isStar())
                {
                  *CmpCommon::diags() << DgSqlCode(-4185) ;
                  bindWA->setErrStatus();
                  return NULL;
                }

              // make a copy of this entry
              newGroupByItemExpr = 
                selectListEntry->copyTopNode(NULL, bindWA->wHeap());
            }
          else
            {
              selectListEntry->setInGroupByOrdinal(TRUE);
              newGroupByItemExpr = currGroupByItemExpr;
            }

          foundSelIndex = TRUE;
        } // group by ordinal
      else if (currGroupByItemExpr->getOperatorType() == ITM_REFERENCE)
        {
           ColReference * groupByColReference = 
                                 (ColReference*)currGroupByItemExpr;
           
          // find out if this ColReference name is a renamed col in the
          // select list.
          if (lookForRenamedCols && 
              groupByColReference->getCorrNameObj().getQualifiedNameObj().getObjectName().length() == 0)
            {

              NABoolean renamedColsInSelectList = FALSE;
              CollIndex j = 0;
              NABoolean found = FALSE;
              while (j < origSelectList.entries())
                {
                  ItemExpr * selectListEntry = origSelectList[j];

                  if (selectListEntry->getOperatorType() == ITM_RENAME_COL)
                    {
                      renamedColsInSelectList = TRUE;

                      const ColRefName &selectListColRefName = 
                        *((RenameCol *)selectListEntry)->getNewColRefName();
                      
                      if (groupByColReference->getColRefNameObj().getColName()
                          == selectListColRefName.getColName())
                        {
                          if (found)
                            {
                              // multiple entries with the same name. Error.
                              *CmpCommon::diags() << DgSqlCode(-4195) 
                                << DgString0(selectListColRefName.getColName());
                              bindWA->setErrStatus();
                              return NULL;
                            }

                          foundSelIndex = TRUE;

                          selectListEntry->setInGroupByOrdinal(TRUE);

                          newGroupByItemExpr = 
                            new(bindWA->wHeap()) SelIndex(j+1);
                          ((SelIndex *) newGroupByItemExpr)->
                            setRenamedColNameInGrbyClause(TRUE);


                          found = TRUE;
                        }
                    } // rename col

                  j++;
                } // while

              if ((NOT renamedColsInSelectList) &&
                  (j == origSelectList.entries()))
                lookForRenamedCols = FALSE;
            } // lookForRenamedCols

          if (! newGroupByItemExpr)
            newGroupByItemExpr = currGroupByItemExpr;
        } // else foundSelIndex
      else if ((currGroupByItemExpr->getOperatorType() == ITM_USER_DEF_FUNCTION) &&
               (udfSubqInAggGrby_Enabled))
            newGroupByItemExpr = currGroupByItemExpr;
      else if ((currGroupByItemExpr->getOperatorType() == ITM_ROW_SUBQUERY) &&
               (udfSubqInAggGrby_Enabled))
            newGroupByItemExpr = currGroupByItemExpr;
      else
        {
          *CmpCommon::diags() << DgSqlCode(-4197) 
                              << DgString0("GROUP BY");
          bindWA->setErrStatus();
          return NULL;
        }

      newGroupByList.insert(newGroupByItemExpr);

    } // for
  
  if ((foundSelIndex) &&
      (newGroupByList.entries() > 0))
    {
      grby->removeGroupExprTree();
      grby->addGroupExprTree(newGroupByList.convertToItemExpr());
    }

  grby->setParentRootSelectList(getCompExprTree());

  // if order by and group by are specified, check to see that
  // all columns specified in the order by clause are also present
  // in the group by clause.
  allOrderByRefsInGby_ = FALSE;
  if (
      (getOrderByTree()) &&
      (grby->getGroupExprTree() != NULL))
    {
      ItemExpr *orderByTree = getOrderByTree();

      ItemExprList orderByList(orderByTree, bindWA->wHeap());
      ItemExprList groupByList(grby->getGroupExprTree(), bindWA->wHeap());

      allOrderByRefsInGby_ = TRUE;
      for (CollIndex ii = 0; ii < orderByList.entries(); ii++)
        {
          ItemExpr * colRef = orderByList[ii];
          if (colRef->getOperatorType() == ITM_INVERSE)
            colRef = colRef->child(0)->castToItemExpr();
          if (colRef && colRef->getOperatorType() == ITM_REFERENCE)
            {
              ColReference * obyColRef = (ColReference*)colRef;
              
              NABoolean found = FALSE;
              for (CollIndex j = 0; j < groupByList.entries(); j++)
                {
                  ItemExpr * gbyExpr = groupByList[j];
                  if (gbyExpr->getOperatorType() == ITM_REFERENCE)
                    {
                      ColReference * gbyColRef = (ColReference*)gbyExpr;
                      if (obyColRef->getColRefNameObj().getColName() ==
                          gbyColRef->getColRefNameObj().getColName())
                        {
                          found = TRUE;
                          break;
                        }
                    } // if
                } // for

              if (NOT found)
                {
                  allOrderByRefsInGby_ = FALSE;
                  break;
                }
            } // if
        } // for
    } // if

  return this;
}

RelRoot * RelRoot::transformGroupByWithOrdinalPhase2(BindWA *bindWA)
{
  NABoolean specialMode =
    (CmpCommon::getDefault(MODE_SPECIAL_1) == DF_ON);

  // make sure child of root is a groupby node or a sequence node 
  // whose child is a group by node
  if (child(0)->getOperatorType() != REL_GROUPBY && 
       (child(0)->getOperatorType() != REL_SEQUENCE || 
         (child(0)->child(0) && child(0)->child(0)->getOperatorType()!=REL_GROUPBY)))
    return this;

  GroupByAgg * grby;
  RelSequence * seqNode=NULL;

  if (child(0)->getOperatorType() == REL_GROUPBY )
  {
    grby=(GroupByAgg *)(child(0)->castToRelExpr());
  }
  else
  {//sequence node above group by
    grby=(GroupByAgg *)(child(0)->child(0)->castToRelExpr());
    seqNode=(RelSequence *)(child(0)->castToRelExpr());
  }

  DCMPASSERT(grby != NULL);

  if (grby->isRollup())
    {
      if (grby->groupExpr().entries() != grby->rollupGroupExprList().entries())
        {
          *CmpCommon::diags() << DgSqlCode(-4384)
                              << DgString0("Cannot have duplicate entries.");
          
          bindWA->setErrStatus();
          return NULL;
        }

      for (ValueId valId = grby->aggregateExpr().init();
           grby->aggregateExpr().next(valId);
           grby->aggregateExpr().advance(valId))
        {
          ItemExpr * ae = valId.getItemExpr();

          // right now, only support groupby rollup on min/max/sum/avg/count
          if (NOT ((ae->getOperatorType() == ITM_MIN) ||
                   (ae->getOperatorType() == ITM_MAX) ||
                   (ae->getOperatorType() == ITM_SUM) ||
                   (ae->getOperatorType() == ITM_AVG) ||
                   (ae->getOperatorType() == ITM_COUNT) ||
                   (ae->getOperatorType() == ITM_COUNT_NONULL) ||
                   (ae->getOperatorType() == ITM_GROUPING)))
            {
              *CmpCommon::diags() << DgSqlCode(-4384)
                                  << DgString0("Unsupported rollup aggregate function.");
              
              bindWA->setErrStatus();
              return NULL;
            }
          
          // right now, only support groupby rollup on non-distinct aggrs
          Aggregate * ag = (Aggregate*)ae;
          if (ag->isDistinct())
            {
              *CmpCommon::diags() << DgSqlCode(-4384)
                                  << DgString0("Distinct rollup aggregates not supported.");
              
              bindWA->setErrStatus();
              return NULL;
            }

          // if grouping aggr, find the rollup group it corresponds to.
          if (ae->getOperatorType() == ITM_GROUPING)
            {
              NABoolean found = FALSE;
              ItemExpr * aggrChild = ae->child(0);
              int i = 0;
              while ((NOT found) and (i < grby->rollupGroupExprList().entries()))
                {
                  ValueId vid =  grby->rollupGroupExprList()[i];
                  if (vid.getItemExpr()->getOperatorType() == ITM_SEL_INDEX)
                    {
                      SelIndex * si = (SelIndex*)vid.getItemExpr();
                      vid = compExpr()[si->getSelIndex()-1];
                    }
                  found =  aggrChild->duplicateMatch(*vid.getItemExpr());
                  if (found)
                    ag->setRollupGroupIndex(i);
                  i++;
                } // while

              if (NOT found)
                {
                  // must find it.
                  *CmpCommon::diags() << DgSqlCode(-4384)
                                      << DgString0("GROUPING function can only be specified on a GROUP BY ROLLUP entry.");
                  
                  bindWA->setErrStatus();
                  return NULL;
                }
            }
        } // for
    }
  else
    {
      // not groupby rollup
      for (ValueId valId = grby->aggregateExpr().init();
           grby->aggregateExpr().next(valId);
           grby->aggregateExpr().advance(valId))
        {
          ItemExpr * ae = valId.getItemExpr();

          // grouping can only be specified with 'groupby rollup' clause
          if (ae->getOperatorType() == ITM_GROUPING)
            {
              *CmpCommon::diags() << DgSqlCode(-3242)
                                  << DgString0("GROUPING function can only be specified with GROUP BY ROLLUP clause.");
              
              bindWA->setErrStatus();
              return NULL;
            }
 
        } // for
    }

  ValueIdSet &groupExpr = grby->groupExpr();
  // copy of groupExpr used to identify the changed
  // value ids
  ValueIdSet groupExprCpy(grby->groupExpr());

  // When we encounter subqueries or MVFs in the select list 
  // these gets expanded at bind time, and so the select index have to
  // be offset with the expansion number since the sel_index number 
  // reflects the select list at parse time.
  for (ValueId vid = groupExpr.init(); 
       groupExpr.next(vid);
       groupExpr.advance(vid))
    {
      if (vid.getItemExpr()->getOperatorType() == ITM_SEL_INDEX)
        {
          CollIndex selIndexExpansionOffset = 0;
          SelIndex * si = (SelIndex*)(vid.getItemExpr());
          ValueId grpById = 
                      compExpr()[si->getSelIndex() -1];
          si->setValueId(grpById);
          if (child(0)->getOperatorType() != REL_SEQUENCE)
          {
            groupExprCpy.remove(vid);
            groupExprCpy.insert(grpById);
            if (grby->isRollup())
              {
                CollIndex idx = grby->rollupGroupExprList().index(vid);
                grby->rollupGroupExprList()[idx] = grpById;
              }
          }
          else
          {  //sequence
            CMPASSERT(seqNode);
            const ValueIdSet seqCols = ((const RelSequence*)seqNode)->sequencedColumns();

            ItemExpr * ie = grpById.getItemExpr();
            ItemExpr::removeNotCoveredFromExprTree(ie,seqCols);

            //ie = ie->copyTree(bindWA->wHeap());
            //ie = ie->bindNode(bindWA);
            if (bindWA->errStatus())
	      return NULL;

            groupExprCpy.remove(vid);
            groupExprCpy.insert(ie->getValueId());

            ie = new (bindWA->wHeap()) NotCovered(ie);
            ie->synthTypeAndValueId();

            compExpr()[si->getSelIndex()-1] = ie->getValueId();
            seqNode->addSequencedColumn(ie->getValueId());

          }

          switch (grpById.getItemExpr()->getOperatorType())
          {
            case ITM_VALUEID_PROXY:
            {
              ValueId derivedId = 
                    (( ValueIdProxy *)(grpById.getItemExpr()))->isDerivedFrom();

              // If this is not the ValueIdProxy that represents the MVF or Subq
              // skip the expansion.
              if ((( ValueIdProxy *)(grpById.getItemExpr()))->
                     needToTransformChild() != TRUE) break;

              ValueIdList outputs;

              switch (derivedId.getItemExpr()->getOperatorType())
              {
                case ITM_USER_DEF_FUNCTION:
                {
                  // When we reference a UDF in the groupBy clause,
                  // if the UDF is a MVF(has multiple outputs), we need to add 
                  // the other elements from the MVF's outputs.
                
                  // These elements have already been expanded into the 
                  // select list, so all we need to do is to add them to the
                  // groupby expression.
   
                  // By default, we associate the valueId of the MVF with 
                  // its first output, so we just need to copy the rest of the 
                  // outputs.
                  UDFunction *udf = (UDFunction *) derivedId.getItemExpr();
                  const RoutineDesc *rDesc = udf->getRoutineDesc();
                  outputs =  rDesc->getOutputColumnList();
                  break;
                }

                case ITM_ROW_SUBQUERY:
                {
                  // When we reference a subquery in the groupBy clause,
                  // if the subquery has a degree > 1, we need to add the other
                  // elements from the subquery's select list.
                  Subquery *subq = (Subquery *) derivedId.getItemExpr();
                  RelRoot *subqRoot = (RelRoot *) subq->getSubquery();
                  outputs =  subqRoot->compExpr();
                  break;
                }
                default:
                  CMPASSERT(0); // we don't support anything else
              }


              // Add in the other outputs from the MVF/Subquery
              for (CollIndex i=1; i < outputs.entries();  i++)
              {
                selIndexExpansionOffset ++;
                groupExprCpy.insert(outputs[i]);
              }

              // Need to check the groupBy and orderBy lists
              // for selIndexes with an index greater than this one, 
              // If we find one, bump its index into the select list by
              // the expansion.
     
              ValueIdSet fixedUpIndecies;

              fixUpSelectIndeciesInSet(grby->groupExpr(),fixedUpIndecies,
                                       si->getSelIndex(), 
                                       selIndexExpansionOffset);

              fixUpSelectIndecies(getOrderByTree(), fixedUpIndecies,
                                  si->getSelIndex(), 
                                  selIndexExpansionOffset);
            break;
            }
          }

          // Now that we have swapped the vid list from grouping
          // expression to the corresponding one from select list
          // go thru each expression, collect the base columns
          // and mark each column as referenced for histogram.
          // Since this is only for group by, we will get only single
          // interval histograms - 10-081015-6557
          ValueIdSet columns;
          grpById.getItemExpr()->findAll(ITM_BASECOLUMN, columns, TRUE, TRUE);
          for (ValueId id = columns.init(); 
                       columns.next(id); 
                       columns.advance(id))
          {
            NAColumn *nacol = id.getNAColumn();
            if (nacol->isReferencedForHistogram())
              continue;
            nacol->setReferencedForSingleIntHist();
          }
       } // found Sel Index
    }

  ValueId valId;
  if (grby->isRollup())
    {
      for (CollIndex i = 0; i < grby->rollupGroupExprList().entries(); i++)
        {
          valId = grby->rollupGroupExprList()[i];
          
          if (NOT valId.getType().supportsSQLnull())
            {
              *CmpCommon::diags() << DgSqlCode(-4384)
                                  << DgString0("Grouped columns must be nullable.");
              bindWA->setErrStatus();
              return NULL;
            }
        }
    }

  //looking for extra order requirement, currently, aggregate function PIVOT_GROUP will need extra order 
  //so loop through the aggregation expression and check if there is PIVOT_GROUP and it needs explicit order
  //if found, populate the extraOrderExpr for the GroupAggBy
  //so later optimizer can add correct sort key 
  ValueIdSet &groupAggExpr = grby->aggregateExpr();

  for (ValueId vid = groupAggExpr.init(); 
       groupAggExpr.next(vid);
       groupAggExpr.advance(vid))
    {
      if (vid.getItemExpr()->getOperatorType() == ITM_PIVOT_GROUP)
      {
        if( ((PivotGroup*)vid.getItemExpr())->orderBy() ) 
        {
          grby->setExtraGrpOrderby(((PivotGroup*)vid.getItemExpr())->getOrderbyItemExpr());
          ValueIdList tmpList;
          grby->getExtraGrpOrderby()->convertToValueIdList(tmpList, bindWA, ITM_ITEM_LIST);
          grby->setExtraOrderExpr(tmpList);
        }
      }
    }
  // recreate the groupExpr expression after updating the value ids
  grby->setGroupExpr (groupExprCpy);

  if ((grby->selPredTree()) &&
      (grby->selIndexInHaving()))
    {
      setValueIdForRenamedColsInHaving(bindWA, grby->selPredTree(),
                                       compExpr());

      BindScope *currScope = bindWA->getCurrentScope();
      
      ItemExpr *havingPred = grby->removeSelPredTree();
      currScope->context()->inHavingClause() = TRUE;
      havingPred->convertToValueIdSet(grby->selectionPred(), 
                                      bindWA, ITM_AND);
      currScope->context()->inHavingClause() = FALSE;
      if (bindWA->errStatus()) 
        return this;
    }

  if (orderByTree_ && seqNode && grby)
  {
    ItemExprList origOrderByList(bindWA->wHeap());

    origOrderByList.insertTree(orderByTree_);
    
    ItemExprList newOrderByList((Lng32)origOrderByList.entries(), bindWA->wHeap());

    for (CollIndex i = 0; (i < (CollIndex) origOrderByList.entries());i++)
    {
      ItemExpr * currOrderByItemExpr = 
                      ((ItemExpr *) origOrderByList[i])->castToItemExpr();
      ItemExpr * newOrderByItemExpr = currOrderByItemExpr;
      
      if (currOrderByItemExpr->getOperatorType() == ITM_SEL_INDEX)
      {
         SelIndex * si = (SelIndex*)(currOrderByItemExpr);
         if (compExpr()[si->getSelIndex()-1].getItemExpr()->getOperatorType() != ITM_BASECOLUMN)
         {
           newOrderByItemExpr = compExpr()[si->getSelIndex()-1].getItemExpr();
         }
      }
      newOrderByList.insert(newOrderByItemExpr);
    }
    orderByTree_ = newOrderByList.convertToItemExpr();
  }

  return this;
}

void RelRoot::transformTDPartitionOrdinals(BindWA *bindWA)
{
  if(!getHasTDFunctions()) 
    return ;

  if (getCompExprTree() == NULL)
    return ;

  BindScope *currScope = bindWA->getCurrentScope();

  RelExpr * realChildNode = NULL;
 
  if (child(0)->getOperatorType() == REL_FIRST_N)
  {
    realChildNode = child(0)->child(0);
  }
  else
  {
    realChildNode = child(0);
  }
  
  if(realChildNode->getOperatorType() != REL_SEQUENCE )
  {
    return;
  }

  RelSequence * seqNode = (RelSequence *)realChildNode;

  if (!seqNode->getPartitionBy())
  {
    return;
  }
  
  ItemExpr * partitionBy  = seqNode->getPartitionBy()->copyTree(bindWA->wHeap());

  ItemExprList origSelectList(getCompExprTree(), bindWA->wHeap());

  ItemExprList origPartitionByList(bindWA->wHeap());
  if (partitionBy)
  {
      origPartitionByList.insertTree(partitionBy);
  }

  for (CollIndex i = 0; (i < (CollIndex) origPartitionByList.entries());i++)
  {
    ItemExpr * currPartitionByItemExpr = 
      ((ItemExpr *) origPartitionByList[i])->castToItemExpr();    

    NABoolean selIndexError = FALSE;
    Int64 selIndex = -1;
    if (currPartitionByItemExpr->getOperatorType() == ITM_CONSTANT)
    {
      ConstValue * cv = (ConstValue*)currPartitionByItemExpr;
      if ((cv->canGetExactNumericValue()) &&
          (cv->getType()->getScale() == 0))
      {
        selIndex = cv->getExactNumericValue();

        if (selIndex <= 0 || selIndex > origSelectList.entries())
        { //index in error -- produce error message
          //in TD mode group by <constant> -- constant is purely positional  
          //selIndexError = TRUE;
          *CmpCommon::diags() << DgSqlCode(-4366);
          bindWA->setErrStatus();
          return;
        }
        else
        {
          origPartitionByList.usedEntry( i )= 
            origSelectList.usedEntry((CollIndex)selIndex-1)->copyTree(bindWA->wHeap());
        }
      }
    }
  }
  seqNode->setPartitionBy(origPartitionByList.convertToItemExpr());
}
// resolveAggregates - 
// If aggregate functions have been found in the select list, then
// either attach the aggregate functions to the existing GroupBy below
// this RelRoot, or if there is no GroupBy create a GroupBy with an
// empty groupby list (scalar) and attach the aggregate functions to
// this GroupBy.
//
void RelRoot::resolveAggregates(BindWA *bindWA) 
{
  BindScope *currScope = bindWA->getCurrentScope();

  if (NOT currScope->getUnresolvedAggregates().isEmpty()) {
    if (getHasTDFunctions())
    { //Using rank function and aggregate functions in the same scope is not supported. 
      *CmpCommon::diags() << DgSqlCode(-4365);
      bindWA->setErrStatus();
      return;    
    }

    RelExpr *sequence = currScope->getSequenceNode();

    // The aggregates were used without a GROUP BY or HAVING
    // clause, i.e. an implicit aggregation is performed
    // (with a NULL result for an empty input table).
    NABoolean implicitGrouping = (child(0)->getOperatorType() != REL_GROUPBY);
    
    if(getHasOlapFunctions()) {
      implicitGrouping = (sequence->child(0)->getOperatorType() != REL_GROUPBY);
    }
    GroupByAgg *groupByAgg = NULL;
    if (implicitGrouping) {
      RelExpr * realChildNode = NULL;

      // if my child is a FIRST_N node, then add the GroupByAgg below it.
      // Otherwise, add the GroupByAgg below me.
      if (child(0)->getOperatorType() == REL_FIRST_N)
      {
        realChildNode = child(0)->child(0);
      }
      else
        realChildNode = child(0);

      if(getHasOlapFunctions()) {

        realChildNode = sequence->child(0);
      }

      groupByAgg =
        new (bindWA->wHeap()) GroupByAgg(realChildNode,REL_GROUPBY);
      realChildNode->setBlockStmt(isinBlockStmt());

      if(getHasOlapFunctions())
        sequence->setChild(0, groupByAgg);
      else if (child(0)->getOperatorType() == REL_FIRST_N)
        child(0)->setChild(0, groupByAgg);
      else
        setChild(0, groupByAgg);

      groupByAgg->setBlockStmt(isinBlockStmt());
    }
    else {
      if(getHasOlapFunctions()) {
        groupByAgg = (GroupByAgg *)sequence->child(0).getPtr();
      } else {
        groupByAgg = (GroupByAgg *)child(0).getPtr();
      }
    }

    NAString colName(bindWA->wHeap());
    Lng32 sqlCode = 0;
    ValueId valId = NULL_VALUE_ID;

    if (currScope->context()->unaggColRefInSelectList()) {
      sqlCode = -4021;
      valId = currScope->context()->unaggColRefInSelectList()->getValueId();
    }
    else if (implicitGrouping) {
      // Genesis 10-000414-9410:  "SELECT SUM(A),* FROM T; --no GROUP BY"
      // cannot be flagged with err 4012 in ColReference::bindNode
      // because table not marked "grouped" yet.
      //
      const ColumnDescList &cols = *currScope->getRETDesc()->getColumnList();
      CollIndex i, n = cols.entries();
      for (i=0; i<n; i++) {
        const ColumnDesc *col = cols[i];
        if (!col->isGrouped())
          if (col->getColRefNameObj().isStar() ||
              col->getValueId().getNAColumn(TRUE/*okIfNotColumn*/)) {
            sqlCode = -4012;
            valId = col->getValueId();
            colName = col->getColRefNameObj().getColRefAsAnsiString();
            break;
          }
      }
    }

    // Table has no GROUP BY (so no grouping columns exist at all)
    // but is grouped by dint of a column reference within an aggregate,
    // making any unaggregated column references illegal, by ANSI 7.9 SR 7.
    if (sqlCode) {

      if (colName.isNull()) {
        const NAColumn *nacol = valId.getNAColumn(TRUE/*okIfNotColumn*/);
        if (nacol)
          colName = nacol->getFullColRefNameAsAnsiString();
        else
          colName = "_unnamed_column_";
      }

      // 4012 Col ref must be grouping or aggregated -- no star ref allowed!
      // 4021 The select list contains a non-grouping non-aggregated column.
      *CmpCommon::diags() << DgSqlCode(sqlCode) << DgColumnName(colName);
      bindWA->setErrStatus();
      return;
    }

    // Move the unresolved aggregates into the groupby node and bind
    // (simply returns if "groupByAgg" isn't new).
    groupByAgg->aggregateExpr() += currScope->getUnresolvedAggregates();
    currScope->getUnresolvedAggregates().clear();
    groupByAgg->bindNode(bindWA);
  }
}

// resolveSequenceFunctions -
// Add the unresolvedSequenceFunctions to the Sequence node for this
// scope.  If there are sequence functions, but no sequence node, it
// is an error.  Also if there is a sequence node, but no sequence
// functions, it is an error.
//
// 
void RelRoot::resolveSequenceFunctions(BindWA *bindWA)
{
  BindScope *currScope = bindWA->getCurrentScope();

  // If we have a Sequence Node associated with the RelRoot node,
  //
  RelSequence *sequenceNode = (RelSequence *)currScope->getSequenceNode();
  currScope->getSequenceNode() = NULL;

  if (sequenceNode) {
    if (getHasTDFunctions() && sequenceNode->child(0)->getOperatorType() == REL_GROUPBY) 
    { //Using rank function and group by clause in the same scope is not supported.
      *CmpCommon::diags() << DgSqlCode(-4366);
      bindWA->setErrStatus();
      return;
    }

    CMPASSERT(sequenceNode->getOperatorType() == REL_SEQUENCE);

    // Do not allow sequence functions or OLAP Window functions
    // with Embedded Updates.
    //
    if (getGroupAttr()->isEmbeddedUpdateOrDelete()){
      *CmpCommon::diags() << DgSqlCode(-4202)
        << (getGroupAttr()->isEmbeddedUpdate() ?
            DgString0("UPDATE"):DgString0("DELETE"));
      bindWA->setErrStatus();
      return;
      }

    // If there are some sequence functions that have not been attached
    // to the Sequence node, do so now.  These were found when binding
    // the select list.
    //
    sequenceNode->
      addUnResolvedSeqFunctions(currScope->getUnresolvedSequenceFunctions(),
                                bindWA);

    currScope->getUnresolvedSequenceFunctions().clear();
    currScope->getAllSequenceFunctions().clear();
    if (bindWA->errStatus()) return;

    // Make sure the sequence function has some work to do.
    // The cast is needed since the compiler will attempt to pick the
    // protected (writable) version of 'sequenceFunctions()'.  (Is this
    // a compiler bug)
    //
    if ((((const RelSequence *)sequenceNode)->sequenceFunctions().isEmpty() )
        && 
        ( !getHasOlapFunctions() &&
          ((const RelSequence *)sequenceNode)->requiredOrder().entries() != 0 )) {
      // Can't have a sequence by clause without
      // sequence functions.
      //
      *CmpCommon::diags() << DgSqlCode(-4111);
      bindWA->setErrStatus();
      return;
    }
  } else if (! currScope->getUnresolvedSequenceFunctions().isEmpty()) {

    // Can't have sequence functions without a
    // sequence by clause.
    // First, loop through the list of functions.
    //
    ValueIdSet &unresolved = currScope->getUnresolvedSequenceFunctions();
    NAString unparsed(bindWA->wHeap());
    for (ValueId vid = unresolved.init(); unresolved.next(vid); unresolved.advance(vid)) {
     ItemExpr *ie = vid.getItemExpr();
     CMPASSERT(ie->isASequenceFunction());
     unparsed += ", ";
     ie->unparse(unparsed, DEFAULT_PHASE, USER_FORMAT_DELUXE);
    }
    unparsed.remove(0,2);     // remove initial ", "

    *CmpCommon::diags() << DgSqlCode(-4110) << DgString0(unparsed);
    bindWA->setErrStatus();
    return;
  }
}

// if a where pred is specified on an immediate child scan or rename node,
// and it contains an 'and'ed rownum() predicate of the form:
//    rownum < val, or rownum <= val, or rownum = val
// then get the val and make it the firstN value.
// Also, remove this predicate from selPredTree.
void RelRoot::processRownum(BindWA * bindWA)
{
  NABoolean specialMode = (CmpCommon::getDefault(MODE_SPECIAL_4) == DF_ON);
  if (NOT specialMode)
    return;

  if (! child(0))
    return;

  if ((child(0)->getOperatorType() != REL_SCAN) &&
      (child(0)->getOperatorType() != REL_RENAME_TABLE))
    return;

  if (! child(0)->selPredTree())
    return;
  
  ItemExpr * wherePred = child(0)->selPredTree();
  ItemExprList iel(wherePred, bindWA->wHeap(), ITM_AND, FALSE, FALSE);

  NABoolean found = FALSE;
  for (Lng32 i = 0; ((NOT found) && (i < iel.entries())); i++)
    {
      ItemExpr * ie = iel[i];
      if (ie->getArity() != 2)
        continue; 

      if (NOT ((ie->getOperatorType() == ITM_LESS) ||
               (ie->getOperatorType() == ITM_EQUAL) ||
               (ie->getOperatorType() == ITM_LESS_EQ)))
        continue;
      
      ItemExpr * child0 = ie->child(0)->castToItemExpr();
      ItemExpr * child1 = ie->child(1)->castToItemExpr();

      if (NOT ((child0->getOperatorType() == ITM_REFERENCE) &&
               (child1->getOperatorType() == ITM_CONSTANT)))
        continue;
               
      ColReference * col = (ColReference*)child0;
      ColRefName &colRefName = col->getColRefNameObj();
      CorrName &cn = col->getCorrNameObj();
      
      const NAString &catName = cn.getQualifiedNameObj().getCatalogName();	       
      const NAString &schName = cn.getQualifiedNameObj().getSchemaName();		
      const NAString &objName = cn.getQualifiedNameObj().getObjectName();		
      const NAString &colName = colRefName.getColName();
      
      if (NOT ((catName.isNull()) &&
               (schName.isNull()) &&                
               (objName.isNull()) &&
               (colName == "ROWNUM")))
        continue;

      ConstValue * cv = (ConstValue*)child1;
      if (NOT cv->canGetExactNumericValue())
        continue;

      Int64 val = cv->getExactNumericValue();
      if (val < 0)
        continue;

      if ((ie->getOperatorType() == ITM_EQUAL) &&
          (val != 1))
        continue;

      if ((ie->getOperatorType() == ITM_LESS) &&
          (val > 0))
        val--;

      setFirstNRows(val);

      // remove this pred from the list
      iel.removeAt(i);

      found = TRUE;
    }

  if (found)
    {
      // convert the list back to selection pred.
      ItemExpr * ie = iel.convertToItemExpr();
      child(0)->removeSelPredTree();
      child(0)->addSelPredTree(ie);
    }

  return;
}

RelExpr *RelRoot::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // if this is a subquery with [first n] + ORDER BY, we have to rewrite it
  if (!isTrueRoot() &&  // if it is a subquery
      hasOrderBy() &&   // and there is an order by
      ((getFirstNRows() != -1) || (getFirstNRowsParam())) && // and there is [first/last/any n]
      (CmpCommon::getDefault(COMP_BOOL_114) == DF_ON))  // and this fix is turned on
    {
      if ((getFirstNRows() >= 0) || (getFirstNRowsParam()))
        {
          if (needFirstSortedRows())
            {
              // [first n] + ORDER BY case, need to rewrite the subquery
              return rewriteFirstNOrderBySubquery(bindWA);
            }
          else
            {
              // [any n] + ORDER BY case: We can ignore the ORDER BY, since the
              // user said any n rows will do
              removeOrderByTree();
            }
        }
      else
        {
          // [last 0] or [last 1] case
          
          // There are two possible approaches here. We could change the 
          // [last n] to [first n], and reverse the sense of the ORDER BY
          // (that is, adding Inverse nodes or removing them, depending).
          // But [last n] is usually used for performance testing purposes;
          // the idea is to fetch all the data but not return it. This 
          // makes sense at the top level, but not so much at the subquery
          // level. And inverting the sort order and transforming to [first n]
          // would defeat this performance testing purpose. So, for now
          // we will simply forbid this case.
          *CmpCommon::diags() << DgSqlCode(-4484);
          bindWA->setErrStatus();
          return this;
        }
    }

  if (isTrueRoot()) 
    {
      // if this is simple scalar aggregate on a seabase table
      //  (of the form:  select count(*), sum(a) from t; )
      // then transform it so it could be evaluated using hbase co-processor.
      if ((CmpCommon::getDefault(HBASE_COPROCESSORS) == DF_ON) &&
	  (child(0) && child(0)->getOperatorType() == REL_SCAN))
	{
          Scan * scan = (Scan*)child(0)->castToRelExpr();

	  if ((getCompExprTree()) &&
              (NOT hasOrderBy()) &&
              (! getSelPredTree()) &&
              (! scan->getSelPredTree()) &&
	      (scan->selectionPred().isEmpty()) &&
              ((scan->getTableName().getSpecialType() == ExtendedQualName::NORMAL_TABLE) ||
	       (scan->getTableName().getSpecialType() == ExtendedQualName::INDEX_TABLE)) &&
              !scan->getTableName().isPartitionNameSpecified() &&
              !scan->getTableName().isPartitionRangeSpecified() &&
              (NOT bindWA->inViewDefinition()))
	    {
              ItemExprList selList(bindWA->wHeap());
              selList.insertTree(getCompExprTree());
	      // for now, only count(*) can be co-proc'd
              if ((selList.entries() == 1) &&
                  (selList[0]->getOperatorType() == ITM_COUNT) &&
		  (selList[0]->origOpType() == ITM_COUNT_STAR__ORIGINALLY))
                {
                  CorrName cn(scan->getTableName());
                  NATable *naTable = bindWA->getNATable(cn);
		  if (bindWA->errStatus()) 
		    return this;

		  if (((naTable->getObjectType() == COM_BASE_TABLE_OBJECT) ||
		       (naTable->getObjectType() == COM_INDEX_OBJECT)) &&
		      (((naTable->isSeabaseTable()) &&
                        (NOT naTable->isHbaseMapTable())) ||
                       ((naTable->isHiveTable()) &&
                        (NOT naTable->isView()) &&
                        (naTable->getClusteringIndex()) &&
                        (naTable->getClusteringIndex()->getHHDFSTableStats()->isOrcFile()))))
		    {
		      Aggregate * agg = 
			new(bindWA->wHeap()) Aggregate(ITM_COUNT,
						       new (bindWA->wHeap()) SystemLiteral(1),
						       FALSE /*i.e. not distinct*/,
						       ITM_COUNT_STAR__ORIGINALLY,
						       '!');
		      agg->bindNode(bindWA);
		      if (bindWA->errStatus())
			{	  
			  return this;
			}

		      ValueIdSet aggrSet;
		      aggrSet.insert(agg->getValueId());
		      ExeUtilExpr * eue = NULL;
                      
                      if (naTable->isSeabaseTable())
                        eue = 
                          new(CmpCommon::statementHeap())
                          ExeUtilHbaseCoProcAggr(cn,
                                                 aggrSet);
                      else
                        eue = 
                          new(CmpCommon::statementHeap())
                          ExeUtilOrcFastAggr(cn,
                                             aggrSet);
		      
		      eue->bindNode(bindWA);
		      if (bindWA->errStatus())
			{	  
			  return this;
			}
		      
		      setChild(0, eue);

                      removeCompExprTree();
                      addCompExprTree(agg);

		    } // if seabaseTable
		} // count aggr
	    }
	} // coproc on


      if (child(0) && 
	  ((child(0)->getOperatorType() == REL_INSERT) ||
	   (child(0)->getOperatorType() == REL_UNARY_INSERT) ||
	   (child(0)->getOperatorType() == REL_LEAF_INSERT)))
	{
	  Insert * ins = (Insert*)child(0)->castToRelExpr();
	  if (ins->isNoRollback())
          {
            if ((CmpCommon::getDefault(AQR_WNR) 
                 != DF_OFF) &&
                (CmpCommon::getDefault(AQR_WNR_INSERT_CLEANUP)
                 != DF_OFF))
              ins->enableAqrWnrEmpty() = TRUE;
          }
          if (CmpCommon::transMode()->anyNoRollback())
          {
            // tbd - may need to integrate these two.
            if ((CmpCommon::getDefault(AQR_WNR)
                != DF_OFF) &&
                (CmpCommon::getDefault(AQR_WNR_INSERT_CLEANUP)
                != DF_OFF))
              ins->enableAqrWnrEmpty() = TRUE;
          }
	}
      
      // if lob is being extracted as chunks of string, then only one
      // such expression could be specified in the select list.
      // If this is the case, then insert ExeUtilLobExtract operator.
      // This operator reads lob contents and returns them to caller as
      // multiple rows.
      // This lobextract function could only be used in the outermost select
      // list and must be converted at this point.
      // It is not evaluated on its own.
      if (getCompExprTree())
	{
	  ItemExprList selList(bindWA->wHeap());
	  selList.insertTree(getCompExprTree());
	  if ((selList.entries() == 1) &&
	      (selList[0]->getOperatorType() == ITM_LOBEXTRACT))
	    {
	      LOBextract * lef = (LOBextract*)selList[0];
	      
	      ExeUtilLobExtract * le =
		new (PARSERHEAP()) ExeUtilLobExtract
		(lef, ExeUtilLobExtract::TO_STRING_,
		 0, 0, lef->getTgtSize(), 0,
		 NULL, NULL, NULL, child(0), PARSERHEAP());
	      le->setHandleInStringFormat(FALSE);
	      setChild(0, le);
	    }
	}

      processRownum(bindWA);
      
    } // isTrueRoot

  if (getHasTDFunctions()) 
  {
    transformTDPartitionOrdinals(bindWA);
    if (bindWA->errStatus()) return NULL;
  }

  RelRoot * returnedRoot = 
    transformGroupByWithOrdinalPhase1(bindWA);
  if (! returnedRoot)
    return NULL;

  returnedRoot = 
    transformOrderByWithExpr(bindWA);
  if (! returnedRoot)
    return NULL;

  if (bindWA->getCurrentScope()->context()->inTableCheckConstraint()) {
    // See ANSI 11.9 Leveling Rule 1a (Intermediate Sql).
    // 4089 A check constraint cannot contain a subquery.
    *CmpCommon::diags() << DgSqlCode(-4089)
      << DgConstraintName(
            bindWA->getCurrentScope()->context()->inCheckConstraint()->
            getConstraintName().getQualifiedNameAsAnsiString());
    bindWA->setErrStatus();
  }

  if (isTrueRoot()) 
    bindWA->setTopRoot(this);

  bindWA->setBindTrueRoot(isTrueRoot());

  if (!bindWA->getAssignmentStArea()) {
    bindWA->getAssignmentStArea() =
      new (bindWA->wHeap()) AssignmentStArea(bindWA);
    bindWA->getAssignmentStArea()->getAssignmentStHostVars() =
      new (bindWA->wHeap()) AssignmentStHostVars(bindWA);
  }


  // If there are one or more output rowset variables, then we introduce
  // a RowsetInto node below this Root node. The RowsetInto node will
  // create a Pack node later on when it is binded, so that we can
  // insert values into the rowset output variables.
  // We don't do this transformation if we are inside a compound statement.
  //
  if (isTrueRoot() && assignmentStTree()) {
    ItemExpr *outputVar = getOutputVarTree();
    if (outputVar) {

      CMPASSERT(outputVar->getChild(0)->getOperatorType() == ITM_HOSTVAR);
      HostVar *hostVar = (HostVar *) outputVar->getChild(0);

      if (hostVar->getType()->getTypeQualifier() == NA_ROWSET_TYPE) {

        ItemExpr *outputVar = removeOutputVarTree();
        assignmentStTree() = NULL;

        // Get the output size expression. It may be a constant or a variable.
        ItemExpr * sizeExpr = getHostArraysArea()->outputSize();

        // set the SelectIntoRowsets flag
        getHostArraysArea()->setHasSelectIntoRowsets(TRUE);

        // Create INTO node. Its child is the current root
        RelExpr *intoNode =
          new (bindWA->wHeap()) RowsetInto(this, outputVar, sizeExpr);

        //If case of first N with ORDER BY generator introduces the FIRST N
        //operator.  For rowsets FIRST N node need to be introduced below the
        //PACK node and not below the top root. So set first N rows for INTO
        //node and not the top root.
        if (hasOrderBy()) {
          intoNode->setFirstNRows(getFirstNRows());
          setFirstNRows(-1);
        }

        // Create a new root node that will go above the RowsetInto node
        setRootFlag(FALSE);
        RelRoot *newRoot  = new (bindWA->wHeap()) RelRoot(intoNode);
        newRoot->setRootFlag(TRUE);
        // copy the display flag from this true Root to the new root.
        // newRoot->setDisplayTree(getDisplayTree());
        newRoot->setDisplayTree(TRUE);
        newRoot->addInputVarTree(removeInputVarTree());
        newRoot->outputVarCnt() = outputVarCnt();

        NABoolean defaultSortedRows = newRoot->needFirstSortedRows();
        //Int64 defaultFirstNRows = newRoot->getFirstNRows();
        newRoot->needFirstSortedRows() = needFirstSortedRows();
        //newRoot->setFirstNRows(getFirstNRows());
        needFirstSortedRows() = defaultSortedRows;
        //        setFirstNRows(defaultFirstNRows);

        newRoot->rollbackOnError() = rollbackOnError();

        // migrate hostArraysArea to newroot, and tell bindWA about it
        newRoot->setHostArraysArea(getHostArraysArea());
        bindWA->setHostArraysArea(getHostArraysArea());
        setSubRoot(FALSE);      // old root is no longer the root
        newRoot->setSubRoot(TRUE);   // newRoot becomes the root

        return newRoot->bindNode(bindWA);
      }
    }
  }

  if (assignmentStTree() && child(0)->getOperatorType() != REL_ROWSET_INTO) {
    AssignmentStHostVars *ptr =
      new (bindWA->wHeap()) AssignmentStHostVars(bindWA);

    if (ptr->containsRowsets(assignmentStTree())) {
      ItemExpr *outputSizeExpr = NULL;

      // The user may have used the ROWSET FOR OUTPUT SIZE construct
      // set the SelectIntoRowsets flag.
      if (getHostArraysArea()) {
        outputSizeExpr  = getHostArraysArea()->outputSize();
        getHostArraysArea()->setHasSelectIntoRowsets(TRUE);
      }

      // Create RowsetInto node. Its child is the current root
      RelExpr *intoNode = new (bindWA->wHeap())
                           RowsetInto(this, assignmentStTree(), outputSizeExpr);

      //If case of first N with ORDER BY generator introduces the FIRST N
      //operator.  For rowsets FIRST N node need to be introduced below the
      //PACK node and not below the top root. So set first N rows for INTO
      //node and not the top root.
      if (hasOrderBy()) {
        intoNode->setFirstNRows(getFirstNRows());
        setFirstNRows(-1);
      }

      RelRoot *newRoot  = new (bindWA->wHeap()) RelRoot(*this);

      newRoot->child(0) = intoNode;
      newRoot->removeCompExprTree();
      setRootFlag(FALSE);
      removeInputVarTree();
      assignmentStTree() = NULL;

      return newRoot->bindNode(bindWA);
    }
  }

  // Create a new scope.
  //
  if (!isDontOpenNewScope())  // -- Triggers.
  {
    bindWA->initNewScope();

    // MV --
    if(TRUE == hasMvBindContext())
    {
      // Copy the MvBindContext object from the RelRoot node to the
      // current BindContext.
      bindWA->markScopeWithMvBindContext(getMvBindContext());
    }

    if (getInliningInfo().isTriggerRoot())
    {
      CMPASSERT(getInliningInfo().getTriggerObject() != NULL);
      bindWA->getCurrentScope()->context()->triggerObj() =
        getInliningInfo().getTriggerObject()->getCreateTriggerNode();
    }

    if (getInliningInfo().isActionOfRI())
      bindWA->getCurrentScope()->context()->inRIConstraint() = TRUE;
  }

  // Save whether the user specified SQL/MP-style access options in the query
  // (this is always true for the LOCK stmt, which we must maximize).
  //
  if (child(0)->getOperatorType() == REL_LOCK) {
    accessOptions().updateAccessOptions(
      TransMode::ILtoAT(TransMode::READ_COMMITTED_),
      ((RelLock *)child(0).getPtr())->getLockMode());
    accessOptions().updateAccessOptions(
      TransMode::ILtoAT(CmpCommon::transMode()->getIsolationLevel()));
  }

  // QSTUFF:  the updateOrDelete flag is set to ensure that scans done as 
  // part of a generic update cause an exclusive lock to be set to ensure
  // a consistent completion of the following update or delete.
  if (containsUpdateOrDelete(this))
  {
    accessOptions().setUpdateOrDelete(TRUE);
  }
  else if (isTrueRoot())
  {
    // if the query does not contain any Generic Update nodes, mark it
    // as read only query. In that case, we have freedom not to include
    // some indexes in the indexes list.
    bindWA->setReadOnlyQuery();
  }


  // This block of code used to be in RelRoot::propagateAccessOptions() which
  // used to be called from here. We've since replaced this old 'push' call
  // with the 'pull' of BindWA->findUserSpecifiedAccessOption() calls from
  // RelRoot, Scan, and GenericUpdate.
  // QSTUFF
  // We decided to stick with READ COMMITTED as the default access
  //  (even for streams).  However, if we change our mind again, this is
  //  the place to do it.
  // if (getGroupAttr()->isStream() &&
  //   (accessOptions().accessType() == ACCESS_TYPE_NOT_SPECIFIED_))
  //	  accessOptions().accessType() = SKIP_CONFLICT_;

  // Set the flag to indicate to DP2 that this executes an
  //  embedded update or delete.
  if (getGroupAttr()->isEmbeddedUpdateOrDelete())
    accessOptions().setUpdateOrDelete(TRUE);
  // QSTUFF

  if (accessOptions().userSpecified())
    bindWA->getCurrentScope()->context()->setStmtLevelAccessOptions(accessOptions());

  if (isSubRoot() && getHostArraysArea())
    getHostArraysArea()->setRoot(this);

  if (isTrueRoot()) {

    // If this were false, then SynthType's ValueDesc::create()
    // would use a DIFFERENT SchemaDB than BindItemExpr's createValueDesc()
    // -- wrong!  Assert this only once per query.
    CMPASSERT(ActiveSchemaDB() == bindWA->getSchemaDB());

    // set the upDateCurrentOf_ attribute for the root if possible
    if (child(0)->getOperatorType() == REL_UNARY_UPDATE ||
      child(0)->getOperatorType() == REL_UNARY_DELETE) {
      GenericUpdate *gu = (GenericUpdate *)child(0)->castToRelExpr();
      if (gu->updateCurrentOf()) {
        updateCurrentOf()  = gu->updateCurrentOf();
        currOfCursorName() = gu->currOfCursorName();
      }
    }

    // If we are processing a rowset,
    // then the child operator is a REL_TSJ.
    // If this is the case, and the operation is
    // an update or delete, we need to search
    // further to deterine its correct child
    // operator type. 
    
    // Otherwise, the child operator type is correct.
 
    if (bindWA->getHostArraysArea() && 
        bindWA->getHostArraysArea()->hasHostArraysInWhereClause() && 
        bindWA->getHostArraysArea()->hasInputRowsetsInSelectPredicate() == HostArraysWA::NO_  &&
        NOT bindWA->getHostArraysArea()->hasHostArraysInTuple())
        // ensure that we don't flag rowset selects or insert selects with rowsets in the predicate
    {
      if (bindWA->getHostArraysArea()->hasHostArraysInSetClause())  // includes rowset merge statements too
        childOperType() = REL_UNARY_UPDATE;
      else
        childOperType() = REL_UNARY_DELETE;
    }
    else
      childOperType() = child(0)->getOperator();

    // see if we can potentially optimize the buffer sizes for
    // oltp queries. Done for update/delete/insert-values/select-unique.

    // if scan, olt opt is possible.
    if (childOperType() == REL_SCAN)
      oltOptInfo().setOltOpt(TRUE);

    /*
    // For Denali release 1, compound statements are restricted
    // to yield at most one row; so olt opt is possible for CS.
    // If a compound statement is not pushed down to DP2, then
    // OLT optimization will be turned off in generator.
    //
    //       Turn it off for Compound statement as insertion with tuple list
    //       is possible in a CS.
    */
    else if (childOperType() == REL_COMPOUND_STMT)
      oltOptInfo().setOltOpt(TRUE);

    // if INSERT...VALUES, olt opt is possible.
    else if ((childOperType() == REL_UNARY_INSERT) &&
             (NOT child(0)->child(0) ||
             child(0)->child(0)->getOperatorType() == REL_TUPLE))
      oltOptInfo().setOltOpt(TRUE);

  } // isTrueRoot

  else if (checkFirstNRowsNotAllowed(bindWA)) {
    *CmpCommon::diags() << DgSqlCode(-4102);
    bindWA->setErrStatus();
    return NULL;
  }

  BindScope *currScope = bindWA->getCurrentScope();

  // -- MVs
  // Check for the Refresh node before binding, because after binding it
  // will be gone.
  if (child(0)->getOperatorType() == REL_REFRESH)
    setRootOfInternalRefresh();

  // set the currect host area in bindWA for non-root stmt.
  // fix 10-031106-4430 (RG: mxcmp failed to compile INSERT
  //                         statement with rowsets within IF statement)
  HostArraysWA *tempWA = NULL;
  if ( NOT isTrueRoot() && getHostArraysArea() )
  {
     tempWA = bindWA->getHostArraysArea();
     bindWA->setHostArraysArea(getHostArraysArea());
  }

  bindWA->setBindTrueRoot(FALSE);

  // Bind the children here to determine if we need to rollback on error
  // for embedded update/delete's.
  //
  bindChildren(bindWA);

  if ( tempWA )
  {
    // Restore previous environment
    bindWA->setHostArraysArea(tempWA);
  }

  if (bindWA->errStatus()) 
    return NULL;

  // For SPJ, store the spOutParams_ from the bindWA in RelRoot,
  // We need it at codegen
  if ( bindWA->getSpOutParams ().entries ())
    spOutParams_ = &( bindWA->getSpOutParams ());

  if (isTrueRoot()) {

    if (child(0) && child(0)->getGroupAttr()->isEmbeddedUpdateOrDelete()) {
      // Olt optimization is now supported for embedded updates/deletes (pub/sub
      // thingy) for now.
      oltOptInfo().setOltOpt(TRUE);
      if (getFirstNRows() != -1) {
        // [FIRST/ANY n] syntax cannot be used with an embedded update or embedded delete.
      *CmpCommon::diags() << DgSqlCode(-4216);
      bindWA->setErrStatus();
      return NULL;
      }
    }

    // If updateCurrentOf_ not set yet
    // Check the tree for a GenericUpdate RelExpr (anywhere in the tree)
    // so we can set the root node accordingly.
    GenericUpdate *gu = getGenericUpdate(this);
    if (!updateCurrentOf() && gu && gu->updateCurrentOf()) {
      updateCurrentOf()  = gu->updateCurrentOf();
      currOfCursorName() = gu->currOfCursorName();
    }

    // if standalone update/delete(no update where current of),
    // olt opt is possible.
    if (((childOperType() == REL_UNARY_UPDATE) ||
        (childOperType() == REL_UNARY_DELETE)) &&
        (NOT updateCurrentOf()))
      oltOptInfo().setOltOpt(TRUE);

    // If transaction statement (begin/commit/rollback/set xn,
    // olt opt is possible.
    if (childOperType() == REL_TRANSACTION)
      oltOptInfo().setOltOpt(TRUE);

    // Set indication whether transaction need to be aborted on error
    // during an IUD query.
    // Rollback will be done for a query that contains
    // rowsets, or an insert which is
    // not an 'insert...values' with a single value.
    //
    // There are more cases when a transaction will be rolled back on
    // an IUD error. These are set in GenericUpdate::preCodeGen,
    // and DP2(IUD)::preCodeGen.
    // These include embedded update or delete, stream access, non-unique
    // update or delete... See ::preCodeGen methods for details.
    rollbackOnError() = FALSE;
    if (childOperType().match(REL_ANY_GEN_UPDATE))
      {
        if (bindWA->getHostArraysArea() &&
            bindWA->getHostArraysArea()->done()) // rowsets
          rollbackOnError() = TRUE;
        else if ((childOperType() == REL_UNARY_INSERT) &&
                 (child(0)->child(0) &&
                  child(0)->child(0)->getOperatorType() != REL_TUPLE))
          rollbackOnError() = TRUE;
      }
      if (bindWA->getHostArraysArea() &&
          bindWA->getHostArraysArea()->getTolerateNonFatalError())
      {
        setTolerateNonFatalError(RelExpr::NOT_ATOMIC_);
      }
  }

  CMPASSERT(currScope == bindWA->getCurrentScope());   // sanity check

  // do not do olt qry optimization, if rowsets are present.
  if (bindWA->getHostArraysArea() && bindWA->getHostArraysArea()->done())
    {
      oltOptInfo().setOltOpt(FALSE);

      if (bindWA->getHostArraysArea()->getTolerateNonFatalError()) {
        // we also cannot do  dp2 level olt optimization if this is a non-atomic rowset insert
        oltOptInfo().setOltEidOpt(FALSE);
      }
      else {
        // but can do dp2 level olt optimization if this is "regular" rowset insert
        oltOptInfo().setOltEidOpt(TRUE); 
      }
    }

  // If unresolved aggregate functions have been found in the children of the
  // root node, that would mean that we are referencing aggregates before
  // the groupby operation is performed

  if (checkUnresolvedAggregates(bindWA)) return this;

  // A RelRoot does not have a select list for SQL update, delete, insert
  // statements as well as when the query contains an SQL union. If a
  // select list is absent, assign the select list of its child to it.
  // This will propagate the selection lists of the children of the
  // union up to the root.
  //
  // Detach the item expression tree for the select list and bind it.
  //
  ItemExpr *compExprTree = removeCompExprTree();
  if (NOT compExprTree) {
        // -- for RI and Triggers
    if (isEmptySelectList())
      setRETDesc(new (bindWA->wHeap()) RETDesc(bindWA));
    else {
      if (child(0)) {
        setRETDesc(child(0)->getRETDesc());
        getRETDesc()->getValueIdList(compExpr());
      }
    }
  }
  else {
    CMPASSERT(!currScope->context()->inSelectList());
    currScope->context()->inSelectList() = TRUE;

    // QSTUFF
    // in case we are binding an embedded generic update within a view
    // we have to rename column references using OLD or NEW as
    // table names since we adopted the RETDesc/TableDesc of the
    // scan node or the view scan node, i.e. the RenameTable node
    // at the root of an expanded view.

    if (bindWA->renameToScanTable()){

      ColReference * cr = NULL;
      ItemExpr * itm = compExprTree;
      NABoolean done = FALSE;

      const CorrName corr =
        (getViewScanNode()->getOperatorType() == REL_RENAME_TABLE) ?
        ((RenameTable *)getViewScanNode())->getTableName() :
      ((Scan *)getViewScanNode())->getTableDesc()->getCorrNameObj();

      while (NOT done){

        if (itm->getOperatorType() == ITM_ITEM_LIST){
          cr = (ColReference *) itm->getChild(0);
          itm = itm->getChild(1)->castToItemExpr();
          }
        else {
          cr = (ColReference *) itm;
          done = TRUE;
          }

        cr->getCorrNameObj().getQualifiedNameObj().
          setObjectName(corr.getQualifiedNameObj().getObjectName());

        }
      }
    // QSTUFF

    RelRoot *viewQueryRoot = NULL;
    StmtDDLCreateView *pCreateView = NULL;
    if (bindWA->inViewDefinition()) {
      pCreateView = bindWA->getCreateViewParseNode();
      if (pCreateView->getQueryExpression() == this) {
        viewQueryRoot = this;
        CMPASSERT(isTrueRoot());
        pCreateView->setCurViewColNum((CollIndex)0);
      }
    }

    // charset inference
    compExprTree->setResolveIncompleteTypeStatus(TRUE);

    HostArraysWA * arrayWA = bindWA->getHostArraysArea() ;
    if (arrayWA && arrayWA->hasHostArraysInTuple()) {
      CollIndex counterRowVals = 0;
      CMPASSERT(!bindWA->getCurrentScope()->context()->counterForRowValues());
      bindWA->getCurrentScope()->context()->counterForRowValues() = &counterRowVals;
      
      // If this query (scope) contains OLAP Window functions, then add
      // a Sequence Operator just below the Root node.  Also, if aggregates
      // exist, resolve them now.
      //
      setRETDesc(bindRowValues(bindWA, compExprTree, compExpr(), this, isTrueRoot()));
      bindWA->getCurrentScope()->context()->counterForRowValues() = NULL;
    }
    else {
      setRETDesc(bindRowValues(bindWA, compExprTree, compExpr(), viewQueryRoot, isTrueRoot()));
    }
    if (bindWA->errStatus()) return NULL;

    if (viewQueryRoot) pCreateView->resetCurViewColNum();

    currScope->context()->inSelectList() = FALSE;
  }

  // MVs --
  if (bindWA->isPropagateOpAndSyskeyColumns()        &&
      child(0)->getOperatorType()!=REL_GROUPBY       &&
      child(0)->getOperatorType()!=REL_AGGREGATE     &&
      currScope->getUnresolvedAggregates().isEmpty() &&
      !isEmptySelectList()                           &&
      !isTrueRoot())
    getRETDesc()->propagateOpAndSyskeyColumns(bindWA, TRUE);

  CMPASSERT(currScope == bindWA->getCurrentScope());   // sanity check
  currScope->setRETDesc(getRETDesc());
  bindWA->setRenameToScanTable(FALSE);         // QSTUFF

  // Genesis 10-980106-2038 + 10-990202-1098.
  //
  if (isTrueRoot()) {
    castComputedColumnsToAnsiTypes(bindWA, getRETDesc(), compExpr());
    if (bindWA->errStatus()) return NULL;
  }

  // Genesis 10-970822-2581.  See finalize() in SqlParser.y.
  //
  // If we are in a compound statement (an IF's UNION), do not issue an error.
  //
  // Added condition for CALL StoredProcedures
  // If we invoke a CALL statement, the #out params do not match the
  // # columns, we make that check in the CallSP::bindNode, so ignore it
  // for now.
  if (isTrueRoot() &&
      (child(0)) &&
      (child(0)->getOperatorType() != REL_CALLSP &&
      (child(0)->getOperatorType() != REL_COMPOUND_STMT &&
      (child(0)->getOperatorType() != REL_TUPLE &&
       (Int32)getRETDesc()->getDegree() != 0))) &&
     (child(0)->getOperatorType() != REL_UNION ||
     (!((Union *) (RelExpr *) child(0))->getUnionForIF())) &&
      outputVarCntValid() &&
      outputVarCnt() != (Int32)getRETDesc()->getDegree() &&
     (outputVarCnt() ||
      CmpCommon::context()->GetMode() != STMT_DYNAMIC)) {
      // 4093 The number of output parameters ($0) must equal the number of cols
      // 4094 The number of output host vars  ($0) must equal the number of cols
      Lng32 sqlcode = (CmpCommon::context()->GetMode() == STMT_DYNAMIC) ?
      -4093 : -4094;
      *CmpCommon::diags() << DgSqlCode(sqlcode)
      << DgInt0(outputVarCnt()) << DgInt1(getRETDesc()->getDegree());
      bindWA->setErrStatus();
      return NULL;
    }

  ItemExpr *inputVarTree = removeInputVarTree();
  if (inputVarTree) {
    inputVarTree->convertToValueIdList(inputVars(), bindWA, ITM_ITEM_LIST);
    if (bindWA->errStatus()) return NULL;

    // If DYNAMIC SQL compilation, then
    // remove from the input var list (list of HostVars and DynamicParams)
    // any env vars that were found to have a equivalence value which is
    // valid (parseable) for the context it appears in
    // (i.e., we've already bound the env var name's dynamic value,
    // so we no longer need the env var name at all).
    // Right now, this means that in sqlci you can say
    //	 set envvar xyz cat.sch.tbl;
    //	 select * from $xyz;
    //
    if (CmpCommon::context()->GetMode() == STMT_DYNAMIC) {
      for (CollIndex i = inputVars().entries(); i--; ) {
        HostVar *hostVar = (HostVar *)inputVars()[i].getItemExpr();
        if (hostVar->getOperatorType() == ITM_HOSTVAR &&
            hostVar->isPrototypeValid() &&
            (hostVar->isEnvVar() ||
             hostVar->isDefine()))
          inputVars().removeAt(i);
      }
    } // STMT_DYNAMIC
  } // inputVarTree

  // add to the inputVars, any user functions that are to be treated
  // like input values, that is, evaluated once and used therafter.
  // Do not insert duplicate value ids.
  for (CollIndex i = 0; i < bindWA->inputFunction().entries(); i++ ) {
    if (NOT inputVars().contains(bindWA->inputFunction()[i]))
      inputVars().insert(bindWA->inputFunction()[i]);
  }

  // If aggregate functions have been found in the select list, then
  // create a groupby node with an empty groupby list, if the child is not
  // already a groupby node.
  //
  resolveAggregates(bindWA);
  if (bindWA->errStatus()) return NULL;

  // Add the unresolvedSequenceFunctions to the Sequence node for this
  // scope.  If there are sequence functions, but no sequence node, it
  // is an error.  Also if there is a sequence node, but no sequence
  // functions, it is an error.
  // If OLAP Window functions exist for this scope, they will have been
  // translated into sequence functions by this point and so will be added
  // to the Sequence node here.
  //
  resolveSequenceFunctions(bindWA);
  if (bindWA->errStatus()) return NULL;

  BindScope *prevScope   = bindWA->getPreviousScope(currScope);
  NABoolean inRowSubquery   = FALSE;
  if (prevScope)
    inRowSubquery = prevScope->context()->inRowSubquery();

  NABoolean groupByAggNodeAdded = FALSE;
  if (inRowSubquery && (CmpCommon::getDefault(COMP_BOOL_137) == DF_OFF))
    // force adding one row aggregates in the [last 0] case
    groupByAggNodeAdded = addOneRowAggregates(bindWA, 
                            getFirstNRows() == -2 /* [last 0] case */);

  returnedRoot = 
    transformGroupByWithOrdinalPhase2(bindWA);
  if (! returnedRoot)
    return NULL;

  ItemExpr *orderByTree = removeOrderByTree();
  if (orderByTree) {
    //
    // Tandem extension to ANSI (done only if source table is not grouped!):
    // Allow the ORDER BY clause to reference columns in the source table even
    // if the columns are not referenced in the select list.  Treat the extra
    // columns as *system* columns so that they can be referenced by name
    // (ORDER BY name) but not by position in select list (ORDER BY n).
    // Thus, select-list columns have precedence, as they should since ANSI
    // allows only them in ORDER BY to begin with!
    //
    // Add all source columns to system column list of temporary orderBy;
    // remove select-list columns from this system column list;
    // insert select-list columns into the *user* column list
    // (these must be in separate loops to set up the orderBy XCNM correctly!).
    // Then bind the temporary (convert to ValueId list), reset the RETDesc.
    //
    bindWA->getCurrentScope()->context()->inOrderBy() = TRUE;
    CollIndex i;
    RETDesc orderBy;
    const RETDesc &select = *getRETDesc();
    const RETDesc &source = *child(0)->getRETDesc();

    // if the source is grouped, then the ORDER BY columns must be in
    // the select list.  So, don't add any other columns that aren't
    // in the select list...
    if (source.isGrouped()) {
      orderBy.setGroupedFlag();
      //10-031125-1549 -begin
      //Since we are processing a groupby we should
      //certainly have some node below it. Futher if
      //that node is a REL_ROOT we will certainly have
      //a child. So this rather unusual call sequence
      //is safe. We are actually looking for a Pattern
      //like REL_GROUPBY(REL_ROOT(*)) introduced to handle
      //Distint qualifier.
      //for example if we have a query like
      //select distinct j as jcol from t1 order by j;
      //the tree will look like
      //REL_ROOT(REL_GROUPBY(REL_ROOT(REL_SCAN(t1))))
      //In this is a NON-ANSI query. To support queries like this
      //we need to expose "J" as a system column. To do that we need
      //to get hold of the RetDesc of the node below the REL_ROOT
      //(not the actual REL_ROOT).
      RETDesc *src = NULL;
      if(child(0)->child(0)&&
         child(0)->child(0)->getOperatorType() == REL_ROOT)
      {
         src = child(0)->child(0)->child(0)->getRETDesc();
      }
      else
      {
        src = child(0)->getRETDesc();
      }

      const ColumnDescList &sysColList = *src->getSystemColumnList();
      const ColumnDescList &usrColList = *src->getColumnList();
      ValueId vid;

      for(i = 0; i < select.getDegree(); i++) {
        vid = select.getValueId(i);
        for(CollIndex j = 0; j < sysColList.entries(); j++){
          if( vid == sysColList[j]->getValueId()){
               orderBy.addColumn(bindWA, sysColList[j]->getColRefNameObj()
                                 , sysColList[j]->getValueId()
                                 , SYSTEM_COLUMN);
          }
        }
        for(CollIndex k = 0; k < usrColList.entries(); k++){
          if(vid ==  usrColList[k]->getValueId()){
               orderBy.addColumn(bindWA, usrColList[k]->getColRefNameObj()
                                 , usrColList[k]->getValueId()
                                 , SYSTEM_COLUMN);
              }
        }
      }
      //10-031125-1549 -end

      NABoolean specialMode = TRUE;
      

      // In specialMode, we want to support order by on columns
      // which are not explicitely specified in the select list.
      // Ex: select a+1 from t group by a order by a;
      // Find all the column references in the orderByTree which are
      // also in the group by list but are not explicitely specified
      // in the select list.
      // This code path is for cases when both GROUP BY and ORDER BY are
      // specified.
      // If order by is specified without the group by, then that case
      // is already covered in the 'else' portion.
      if ((specialMode) &&
          (child(0)->getOperatorType() == REL_GROUPBY) &&
          (allOrderByRefsInGby_)) // already validated that all order by cols
                                  // are also in group by clause
        {
          ItemExprList orderByList(orderByTree, bindWA->wHeap());
          GroupByAgg * grby=(GroupByAgg *)(child(0)->castToRelExpr());
          for (CollIndex ii = 0; ii < orderByList.entries(); ii++)
            {
              ItemExpr * colRef = orderByList[ii];
              if (colRef->getOperatorType() == ITM_INVERSE)
                colRef = colRef->child(0)->castToItemExpr();
              if (colRef && colRef->getOperatorType() == ITM_REFERENCE)
                {
                  ColReference * obyColRef = (ColReference*)colRef;

                  for (CollIndex k = 0; k < usrColList.entries(); k++)
                    {
                      if (obyColRef->getColRefNameObj().getColName() ==
                          usrColList[k]->getColRefNameObj().getColName())
                        {
                          orderBy.delColumn(bindWA,
                                            usrColList[k]->getColRefNameObj(),
                                            SYSTEM_COLUMN);

                          orderBy.addColumn(bindWA, 
                                            usrColList[k]->getColRefNameObj(),
                                            usrColList[k]->getValueId(),
                                            SYSTEM_COLUMN);
                          break;
                        } // if
                    } // for
                  
                } // if
            } // for
        }
      
      for (i = 0; i < select.getDegree(); i++)
        orderBy.delColumn(bindWA, select.getColRefNameObj(i), SYSTEM_COLUMN);
    }
    else {
      // add the potential ORDER BY columns... omitting the ones that will
      // in the select list anyway.
      orderBy.addColumns(bindWA, *source.getColumnList(),       SYSTEM_COLUMN);
      orderBy.addColumns(bindWA, *source.getSystemColumnList(), SYSTEM_COLUMN);

      for (i = 0; i < select.getDegree(); i++)
        orderBy.delColumn(bindWA, select.getColRefNameObj(i),   SYSTEM_COLUMN);
    }

    for (i = 0; i < select.getDegree(); i++)
      orderBy.addColumn(bindWA, select.getColRefNameObj(i),
                                select.getValueId(i),           USER_COLUMN);

    bindWA->getCurrentScope()->setRETDesc(&orderBy);

    // fix for defect 10-010522-2978
    // If we need to move this OrderBy to the RelRoot above this one...
    //   move it to the rowsetReqdOrder_ of that RelRoot, otherwise keep
    //   it at this level... in the current RelRoot's reqdOrder_

    ValueIdList & pRRO = getParentForRowsetReqdOrder() ?
                         getParentForRowsetReqdOrder()->rowsetReqdOrder_ :
                         reqdOrder();

      // Replace any selIndexies in the orderByTree with what it refers to
      // before we expand it.
      // This is done so that we can deal with subqueries with degree > 1
      // and MVFs.

    ItemExpr *sPtr = orderByTree, *ePtr = orderByTree;
    
    Int32 childIdx = 0;
    NABoolean onlyOneEntry(TRUE);

    CollIndex selListCount = compExpr().entries();

    while (sPtr != NULL) 
    {
      
      if (sPtr->getOperatorType() == ITM_ITEM_LIST)
      {
        ePtr = sPtr;
        sPtr = ePtr->child(0);
        childIdx = 0;
        onlyOneEntry = FALSE;
      }
      if (sPtr->getOperatorType() == ITM_SEL_INDEX)
      {
        SelIndex * si = (SelIndex*)(sPtr);

        CollIndex selIndex = si->getSelIndex();
        if(selIndex == 0 || selIndex > selListCount)
        {
          *CmpCommon::diags() << DgSqlCode(-4007) 
                              << DgInt0((Lng32)si->getSelIndex()) 
                              << DgInt1(selListCount);
           bindWA->setErrStatus();
           return NULL;
        }

        ValueId orderById = compExpr()[si->getSelIndex()-1];
        if (ePtr->getOperatorType() == ITM_ITEM_LIST)
          ePtr->child(childIdx) = orderById.getItemExpr();
        else
          ePtr = orderById.getItemExpr();

        orderById.getItemExpr()->setInOrderByOrdinal(TRUE);
      }
      if ((ePtr->getArity() == 2) && ePtr->child(1) != NULL && 
          ePtr->child(1)->getOperatorType() != ITM_ITEM_LIST &&
          childIdx != 1)
        childIdx = 1;
      else
        childIdx = 0;
      sPtr = (childIdx == 1) ? ePtr->child(1) : NULL;
    }

    if (onlyOneEntry)
      orderByTree =  ePtr;

      // If we had any ordinal expressions expand them in case there
      // are any UDFs or subquery of degree > 1.
      // Also expand any directly referenced UDFs and subqueries of degree > 1.
    ItemExprList origOrderByList(orderByTree, bindWA->wHeap());

    origOrderByList.convertToItemExpr()->
      convertToValueIdList(pRRO, bindWA, ITM_ITEM_LIST);
    
    // end fix for defect 10-010522-2978

    if (bindWA->errStatus()) 
      return NULL;

    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    bindWA->getCurrentScope()->context()->inOrderBy() = FALSE;
  }

  // validate that select list doesn't contain any expressions that cannot be
  // grouped or ordered.
  for (Lng32 selIndex = 0; selIndex < compExpr().entries(); selIndex++)
    {
      ItemExpr * ie = compExpr()[selIndex].getItemExpr();
      if ((ie->inGroupByOrdinal()) || (ie->inOrderByOrdinal()))
        {
          if (NOT ie->canBeUsedInGBorOB(TRUE))
            {
              return NULL;
            }
        }
    }

  if (hasPartitionBy())
  {
    ItemExpr *partByTree = removePartitionByTree();
    partByTree->convertToValueIdSet(partArrangement_, bindWA, ITM_ITEM_LIST);
    if (bindWA->errStatus()) return NULL;
  }

  // fix for defect 10-010522-2978
  // If we're the upper level RelRoot, we must check to see if we have
  // any entries that need to be added to reqdOrder() and add them if
  // there are any...

  if ( rowsetReqdOrder_.entries() ) {
     // We never expect for reqdOrder to contain any entries.  But
     // if it ever does, we want to be able to take a look at this
     // code again to decide whether we should be appending to the
     // reqdOrder list.  Currently the code is written to append to
     // the end of the reqdOrder list, which is likely to be the correct
     // behavior even if there are entries in reqdOrder; we just think
     // that someone should have the chance to rethink this in the event
     // there are entries in reqdOrder and so we're making it fail here
     // to allow/force someone to make the decision.
     CMPASSERT(reqdOrder().entries() == 0);

     // note: NAList<ValueIdList>::insert(const NAList<ValueIdList> &)
     //       actually does an append to the END of the list (not an
     //       insert at the head or after the current position).
     reqdOrder().insert( rowsetReqdOrder_ );
  }

  // end fix for defect 10-010522-2978

  // Bind the update column specification of a cursor declaration.
  // Don't remove the tree: leave it for possible error 4118 in NormRelExpr.
  if (updateColTree_) {
    updateColTree_->convertToValueIdList(updateCol(), bindWA, ITM_ITEM_LIST);
    if (bindWA->errStatus()) {
      if (CmpCommon::diags()->contains(-4001))
        *CmpCommon::diags() << DgSqlCode(-4117);
      return NULL;
    }

    if (getGroupAttr()->isEmbeddedDelete()) {   // QSTUFF
      *CmpCommon::diags() << DgSqlCode(-4169);
      bindWA->setErrStatus() ;
      return NULL;
    }
  }


  // check whether a CONTROL QUERY SHAPE statement is in effect.
  // Do not do if this is a control query statement.
  if (ActiveControlDB()->getRequiredShape()) {
    OperatorTypeEnum op = child(0)->getOperatorType();
    if (!child(0)->isAControlStatement() &&
        op != REL_DESCRIBE &&
        op != REL_EXPLAIN &&
        op != REL_DDL &&
        op != REL_LOCK &&
        op != REL_UNLOCK &&
        op != REL_SET_TIMEOUT &&  
        op != REL_STATISTICS &&
        op != REL_TRANSACTION &&
        op != REL_EXE_UTIL)
      reqdShape_ = ActiveControlDB()->getRequiredShape()->getShape();
  }

  // If this is a parallel extract producer query:
  // * the number of requested streams must be greater than one and
  //   not more than the number of configured CPUs
  // * force a shape with an ESP exchange node immediately below
  //   the root
  ComUInt32 numExtractStreams = getNumExtractStreams();
  if (numExtractStreams_ > 0)
  {
    // Check the number of requested streams
    NADefaults &defs = bindWA->getSchemaDB()->getDefaults();

    NABoolean fakeEnv = FALSE;
    ComUInt32 numConfiguredESPs = defs.getTotalNumOfESPsInCluster(fakeEnv);
    
    if ((numExtractStreams == 1) || (numExtractStreams > numConfiguredESPs))
    {
      *CmpCommon::diags() << DgSqlCode(-4119)
                          << DgInt0((Lng32) numConfiguredESPs);
      bindWA->setErrStatus();
      return NULL;
    }

    // Force the shape. There are three cases to consider:
    // a. there is no required shape in the ControlDB
    // b. there is a required shape and it is acceptable for this
    //    parallel extract.
    // c. there is a required shape and it is not acceptable.

    if (reqdShape_ == NULL)
    {
      // Case a.
      // Manufacture an esp_exchange(cut,N) shape
      reqdShape_ = new (bindWA->wHeap())
        ExchangeForceWildCard(new (bindWA->wHeap()) CutOp(0),
                              ExchangeForceWildCard::FORCED_ESP_EXCHANGE,
                              ExchangeForceWildCard::ANY_LOGPART,
                              (Lng32) numExtractStreams_);
    }
    else
    {
      NABoolean reqdShapeIsOK = FALSE;
      if (reqdShape_->getOperatorType() == REL_FORCE_EXCHANGE)
      {
        ExchangeForceWildCard *exch = (ExchangeForceWildCard *) reqdShape_;
        ExchangeForceWildCard::forcedExchEnum whichType = exch->getWhich();
        Lng32 howMany = exch->getHowMany();
        if (whichType == ExchangeForceWildCard::FORCED_ESP_EXCHANGE &&
            howMany == (Lng32) numExtractStreams_)
        {
          reqdShapeIsOK = TRUE;
        }
      }

      if (reqdShapeIsOK)
      {
        // Case b.
        // Do nothing
      }
      else
      {
        // Case c.
        // Add an esp_exchange to the top of the required shape
        RelExpr *child = reqdShape_;
        reqdShape_ = new (bindWA->wHeap())
          ExchangeForceWildCard(child,
                                ExchangeForceWildCard::FORCED_ESP_EXCHANGE,
                                ExchangeForceWildCard::ANY_LOGPART,
                                (Lng32) numExtractStreams_);
      }

    } // if (reqdShape_ == NULL) else ...
  } // if (numExtractStreams_ > 0)

  // Bind the base class.
  //
  RelExpr *boundExpr = bindSelf(bindWA);
  if (bindWA->errStatus()) return boundExpr;

  // If we have dynamic rowsets, we want to replace
  // dynamic parameters with available inputs.
  if (isTrueRoot() && bindWA->hasDynamicRowsetsInQuery()) {
    ValueIdSet inputs = getGroupAttr()->getCharacteristicInputs();
    UInt32 j = 0;
    // this for loop is over the list of available inputs. We are replacing array
    // parameters with hostvars introduced during HostArraysWA::processArrayHostVar
    // The hostvars introduced in that method are contained in the inputs() list.
    for (ValueId id = inputs.init(); inputs.next(id); inputs.advance(id)) {
      if (id.getItemExpr()->getOperatorType() == ITM_DYN_PARAM) {
        continue;
      }
      // We are assuming here that the hostvars introduced are in the same order as
      // the parameter arrays in inputVars(), i.e. (hv_A, hv_B) corresponds to
      // (?,?,?(as A), ?(as B))
      while (j < inputVars().entries()) {
        ItemExpr *ie = inputVars()[j].getItemExpr() ;
        OperatorTypeEnum ieType = ie->getOperatorType() ;
        if (( ieType != ITM_DYN_PARAM) ||
            (((DynamicParam *) ie)->getRowsetSize() == 0))
             {
          // if an ie is not a dynamicParam or it is a scalar dynamic Param do not remove
          // it from inputVars_. From embedded SQL it is possible to have scalar and array
          // dynamic params in the same statement. This is not possible from ODBC.
               j++;
             }
        else
          break ;
      }
      if (j < inputVars().entries()) {
        inputVars().removeAt(j);
        inputVars().insertAt(j, id);
        j++;
      }
    }
  }

  // RelRoot::codeGen() and Statement::execute() use TOPMOST root's accessOpts.
  //
  if (bindWA->getCurrentScope()->context()->stmtLevelAccessOptions())
    if (!accessOptions().userSpecified()) // seems redundant
      accessOptions() = *bindWA->getCurrentScope()->context()->stmtLevelAccessOptions();

  // Update operations currently require SERIALIZABLE (== MP REPEATABLE_)
  // locking level -- the QSTUFF-enabled DP2 now does this, supporting a true
  // READ_COMMITTED that is STABLE rather than merely CLEAN.
  if (!containsGenericUpdate(this)) {
    // Genesis 10-990114-6293:
    // This flag tells RelRoot::codeGen to set a flagbit in the root-tdb which
    // cli/Statement::execute + compareTransModes() will look at --
    //   if set, then this "read-write" stmt will be allowed to execute
    //   in a run-time transmode of read-only W/O HAVING TO BE RECOMPILED.
    readOnlyTransIsOK() = TRUE;
  }

  if (isTrueRoot()) {
    if (updateCurrentOf()) {
      // add child genericupdate's primary key hostvars to pkeyList.
      // The getLeftmostScanNode() method will return the leftmost Scan node
      // as the original scan node may have moved due to the IM tree.
      pkeyList().insert(child(0)->castToRelExpr()->getLeftmostScanNode()->pkeyHvarList());
    }

    for(Int32 st=0; st < (Int32)bindWA->getStoiList().entries(); st++)
    {
      if(bindWA->getStoiList()[st]->getStoi()->isView())
        viewStoiList_.insert(bindWA->getStoiList()[st]);
    }

    if(bindWA->inDDL())
      ddlStoiList_.insert(bindWA->getStoiList());

    // populate the list of all the routines open information of this query
    stoiUdrList_.insert(bindWA->getUdrStoiList());

    // populate the list of all the UDF information of this query
    udfList_.insert(bindWA->getUDFList());

    // check privileges
    if (!checkPrivileges(bindWA))
    {
      bindWA->setErrStatus();
      return NULL;
    }

    // store the trigger's list in the root
    if (bindWA->getTriggersList())
    {
      triggersList_ =
        new (bindWA->wHeap()) LIST(ComTimestamp)
                (bindWA->wHeap(), bindWA->getTriggersList()->entries());
      triggersList_->insert(*(bindWA->getTriggersList()));

      // Don't allow OLT optimization when triggers are involved.
      oltOptInfo().setOltOpt(FALSE);
    }


    // store the uninitialized mv list if there are any
    // entries    
    if( bindWA->getUninitializedMvList() )
    {    
        uninitializedMvList_ = new (bindWA->wHeap()) UninitializedMvNameList 
            (bindWA->wHeap(), bindWA->getUninitializedMvList()->entries());
        uninitializedMvList_->insert( *(bindWA->getUninitializedMvList()) );             
    } 
    
    
    DBG( if (getenv("TVUSG_DEBUG")) bindWA->tableViewUsageList().display(); )
  } // isTrueRoot

  // Don't allow OLT optimization when ON STATEMENT MV refresh is involved.
  if (bindWA->isBindingOnStatementMv())
    oltOptInfo().setOltOpt(FALSE);

  // disable esp parallelism for merge statements.
  // See class RelRoot for details about this.
  if ((isTrueRoot()) &&
      (bindWA->isMergeStatement()))
    {
      setDisableESPParallelism(TRUE);
    }

  // Remove the current scope.
  //
  if (!isDontOpenNewScope())  // -- Triggers
    bindWA->removeCurrentScope();

  // In case we have a query of the form
  //	SET <host var list> = <select statement>
  // we must update the value ids of the host variables in that list.
  // See Assignment Statement Internal Spec (a project of Compound Statements).
  if (assignmentStTree() &&
      bindWA->getAssignmentStArea() &&
      bindWA->getAssignmentStArea()->getAssignmentStHostVars() &&
     !bindWA->getAssignmentStArea()->getAssignmentStHostVars()->
                              updateValueIds(compExpr(), assignmentStTree())) {
    bindWA->setErrStatus();
    return NULL;
  }

  if (getPredExprTree())
    {
      CMPASSERT(isTrueRoot());

      ItemExpr * ie = removePredExprTree();
      ie = ie->bindNode(bindWA);
      if (bindWA->errStatus())
        return NULL;
      
      addPredExprTree(ie);
    }

  if (getFirstNRowsParam())
    {
      firstNRowsParam_ = firstNRowsParam_->bindNode(bindWA);
      if (bindWA->errStatus())
        return this;

      const SQLInt si(FALSE, FALSE);
      ValueId vid = firstNRowsParam_->castToItemExpr()->getValueId();
      vid.coerceType(si, NA_NUMERIC_TYPE);

      if (vid.getType().getTypeQualifier() != NA_NUMERIC_TYPE)
        {
          // 4045 must be numeric.
          *CmpCommon::diags() << DgSqlCode(-4045) << DgString0(getTextUpper());
          bindWA->setErrStatus();
          return this;
        }
    }

  if ((getFirstNRows() != -1) ||
       (getFirstNRowsParam()))
    {
      // [first/any/last N] processing

      RelExpr * nodeToInsertUnder = this;
      if (inRowSubquery)
        {
          // [first/any/last N] in a row subquery special case
          //
          // In this case, if N > 1 it is first/any N, and we can simply
          // ignore that as row subqueries already enforce an at-most-one
          // row semantic. For [first 1], [last 1], [last 0], we need to
          // add the node below any one-row aggregate group by node created
          // earlier in this method. (If it put it above that group by node,
          // that is too late; the one-row aggregate will raise an 8401 error
          // before our FirstN node has a chance to narrow the result down to
          // zero or one rows.) There is an interesting nuance with [last 0]:
          // We forced the addition of a one-row aggregate group by node
          // in that case, because [last 0] returns no rows. We might have
          // a scalar aggregate subquery which ordinarily would not require
          // a one-row aggregate group, but when [last 0] is present we want
          // to force the aggregates to become NULL. Adding a one-row 
          // aggregate group on top of the scalar aggregate, with the FirstN
          // node in between them does the trick.
          if (groupByAggNodeAdded &&
               ( (getFirstNRows() == 1) ||   // [first 1] or [any 1]
                 (getFirstNRows() == -2) ||  // [last 0]
                 (getFirstNRows() == -3) ) ) // [last 1]
            {
              nodeToInsertUnder = child(0);
              CMPASSERT(nodeToInsertUnder->getOperatorType() == REL_GROUPBY);             
            }
          else if (!groupByAggNodeAdded && (getFirstNRows() == -2))  // [last 0]
            {
              CMPASSERT(groupByAggNodeAdded);  // a GroupByAgg should have been forced
            }
          else  // a case where we can throw the [first/any/last N] away
            {
              nodeToInsertUnder = NULL;
            }
        }
          
      if (nodeToInsertUnder)
        {
          // create a firstN node to retrieve firstN rows.
          FirstN * firstn = new(bindWA->wHeap())
            FirstN(nodeToInsertUnder->child(0), getFirstNRows(), needFirstSortedRows(), getFirstNRowsParam());   

          firstn->bindNode(bindWA);
          if (bindWA->errStatus())
            return NULL;

          // Note: For ORDER BY + [first n], we want to sort the rows before 
          // picking just n of them. (We don't do this for [any n].) We might
          // be tempted to copy the orderByTree into the FirstN node at this
          // point, but this doesn't work. Instead, we copy the bound ValueIds
          // at normalize time. We have to do this in case there are expressions
          // involved in the ORDER BY and there is a DESC. The presence of the
          // Inverse node at the top of the expression tree seems to cause the
          // expressions underneath to be bound to different ValueIds, which 
          // causes coverage tests in FirstN::createContextForAChild requirements
          // generation to fail. An example of where this occurs is:
          //
          // prepare s1 from
          //   select [first 2] y, x from
          //    (select a,b + 26 from t1) as t(x,y)
          //   order by y desc;
          //
          // If we copy the ORDER BY ItemExpr tree and rebind, we get a different
          // ValueId for the expression b + 26 in the child characteristic outputs
          // than what we get for the child of Inverse in Inverse(B + 26). The
          // trick of copying the already-bound ORDER BY clause later avoids this.

          nodeToInsertUnder->setChild(0, firstn);
        }

      // reset firstN indication in the root node.
      setFirstNRows(-1);
      setFirstNRowsParam(NULL);
    }

  // if we have no user-specified access options then
  // get it from nearest enclosing scope that has one (if any)
  if (!accessOptions().userSpecified()) {
    StmtLevelAccessOptions *axOpts = bindWA->findUserSpecifiedAccessOption();
    if (axOpts) {
      accessOptions() = *axOpts;
    }
  }

  if (bindWA->getHoldableType() == SQLCLIDEV_ANSI_HOLDABLE)
  {
    if (accessOptions().accessType() != TransMode::ACCESS_TYPE_NOT_SPECIFIED_)
    {
      if (accessOptions().accessType() == TransMode::REPEATABLE_READ_ACCESS_)
      {
        *CmpCommon::diags() << DgSqlCode(-4381);
        bindWA->setErrStatus();
        return NULL;
      }
    }
    else
    {
      TransMode::IsolationLevel il=CmpCommon::transMode()->getIsolationLevel();
      if (CmpCommon::transMode()->ILtoAT(il) == TransMode::REPEATABLE_READ_ACCESS_)
      {
        *CmpCommon::diags() << DgSqlCode(-4381);
        bindWA->setErrStatus();
        return NULL;
      }
    }
  }

  // The above code is in Scan::bindNode also.
  // It would be nice to refactor this common code; someday.
             
  return boundExpr;
} // RelRoot::bindNode()


RelExpr * RelRoot::rewriteFirstNOrderBySubquery(BindWA *bindWA)
{
  // We have a subquery of the form:
  //
  // select [first n] <x> from <t> order by <y>
  //
  // Unfortunately, there are chicken-and-egg problems that prevent us
  // from pushing the ORDER BY down to the firstN node. (We have to wait
  // until normalizeNode time to insure we get the right ValueIds, but
  // the RelRoot containing the ORDER BY has already been deleted by
  // the time we get there.) To circumvent that, we rewrite the subquery
  // in this form:
  //
  // select <x>
  // from (select <x>, row_number() over(order by <y>) rn from <t>)
  // where rn <= n
  //
  // Aside: For row subqueries in the top-most select list, the 
  // chicken-and-egg problem doesn't exist because the top-most RelRoot
  // does survive until normalizeNode time.
  //
  // To perform this transformation, we take the input tree (on the left
  // below) and rewrite it to the output tree (on the right below). Since
  // this transformation is being done before binding, we move around
  // ItemExpr trees instead of ValueIds. Binding happens at the end of
  // the method, after the rewrite.
  //
  //   
  //                                       new RelRoot (newRelRoot), with copy of
  //                                        original select list
  //                                          | 
  //                                       new RenameTable (renameTable), 
  //                                         with the WHERE rn <= n
  //                                         clause attached
  //                                          |
  //     RelRoot of subquery (this)        original RelRoot (this),
  //       with original select             with ROW_NUMBER ORDER BY aggregate added
  //       list (originalSelectList)         to select list, ORDER BY removed 
  //        |                                from RelRoot 
  //        |                                 |
  //        |                              new RelSequence (sequenceNode)
  //        |                                 |
  //     Subquery tree (query)             Subquery tree (query)            


  // point to subquery tree
  RelExpr * query = child(0)->castToRelExpr();

  // retain a pointer to the present select list to put in our new RelRoot
  ItemExpr * originalSelectList = getCompExprTree();

  // create "row_number() over(order by <y>) as rn" parse tree, removing the
  // order by tree from this RelRoot in the process
  Aggregate * rowNumberOverOrderBy = 
    new (bindWA->wHeap()) Aggregate(ITM_COUNT, 
                                    new (bindWA->wHeap()) SystemLiteral(1),
                                    FALSE /*i.e. not distinct*/,
                                    ITM_COUNT_STAR__ORIGINALLY,
                                    '!');
  rowNumberOverOrderBy->setOLAPInfo(NULL, removeOrderByTree(), -INT_MAX, 0);
  NAString rowNumberColumnName = "_sys_RN_" + bindWA->fabricateUniqueName();
  ColRefName * colRefName = new (bindWA->wHeap()) ColRefName(rowNumberColumnName, bindWA->wHeap());
  RenameCol * rename = new (bindWA->wHeap())RenameCol(rowNumberOverOrderBy, colRefName);

  // add it to select list of the current RelRoot
  compExprTree_ = new (bindWA->wHeap()) ItemList(compExprTree_,rename);

  // put a RelSequence node on top of the query node, and make it the child of this
  RelSequence * sequenceNode = new (bindWA->wHeap()) RelSequence(query,NULL);
  sequenceNode->setHasOlapFunctions(TRUE);
  this->child(0) = sequenceNode;

  // put a RenameTable node on top of this
  NAString corrName = "_sys_X_" + bindWA->fabricateUniqueName();
  RenameTable * renameTable = new (bindWA->wHeap()) RenameTable(this, corrName);

  // attach the WHERE RN <= n clause to the RenameTable node
  DCMPASSERT(getFirstNRows() >= 0); // we are only called for [first n] cases
  ConstValue * n = new (bindWA->wHeap()) ConstValue(getFirstNRows(),bindWA->wHeap());
  ColReference * colReference = new (bindWA->wHeap()) ColReference(colRefName);
  BiRelat * whereClause = new (bindWA->wHeap()) BiRelat(ITM_LESS_EQ,colReference,n);     
  renameTable->addSelPredTree(whereClause);

  // create a new select <x> from (current RelRoot) where rn <= n tree
  RelRoot * newRelRoot = new (bindWA->wHeap()) RelRoot(renameTable,REL_ROOT,originalSelectList);    

  // remove the getFirstNRows of RelRoot
  setFirstNRows(-1);
      
  return newRelRoot->bindNode(bindWA);
} // RelRoot::rewriteFirstNOrderBySubquery


// Present the select list as a tree of Item Expressions
ItemExpr *RelRoot::selectList()
{
  return compExpr().rebuildExprTree(ITM_ITEM_LIST);
} // RelRoot::selectList()

// Returns current place that assignmentStTree_ points to and
// sets that pointer to NULL
ItemExpr * RelRoot::removeAssignmentStTree()
{
  ItemExpr* tempTree = assignmentStTree_;
  assignmentStTree_ = NULL;
  return tempTree;
}

bool OptSqlTableOpenInfo::checkColPriv(const PrivType privType,
                                       const PrivMgrUserPrivs *pPrivInfo)
{
  CMPASSERT (pPrivInfo);

  NATable* table = getTable();
  NAString columns = "";

  if (!isColumnPrivType(privType))
  {
    *CmpCommon::diags() << DgSqlCode(-4481)
                        << DgString0(PrivMgrUserPrivs::convertPrivTypeToLiteral(privType).c_str())
                        << DgString1(table->getTableName().getQualifiedNameAsAnsiString())
                        << DgString2(columns);
    return false;  
  }

  bool hasPriv = true;

  // initialize to something, gets set appropriately below
  LIST (Lng32) * colList = NULL ;
  switch (privType)
  {
    case INSERT_PRIV:
    {
      colList = (LIST (Lng32) *)&(getInsertColList());
      break;
    }
    case UPDATE_PRIV:
    {
      colList = (LIST (Lng32) *)&(getUpdateColList());
      break;
    }
    case SELECT_PRIV:
    {
      colList = (LIST (Lng32) *)&(getSelectColList());
      break;
    }
    default:
      CMPASSERT(FALSE); // delete has no column privileges.
  }

  bool collectColumnNames = false;
  if (pPrivInfo->hasAnyColPriv(privType))
  {
    collectColumnNames = true;
    columns += "(columns:" ; 
  }
  bool firstColumn = true;
  for(size_t i = 0; i < colList->entries(); i++)
  {
    size_t columnNumber = (*colList)[i];
    if (!(pPrivInfo->hasColPriv(privType,columnNumber)))
    {
      hasPriv = false;
      if (firstColumn && collectColumnNames)
      {
        columns += " ";
        firstColumn = false;
      }
      else 
        if (collectColumnNames)
          columns += ", ";

      if (collectColumnNames)
        columns += table->getNAColumnArray()[columnNumber]->getColName();
    }
  }

  if (collectColumnNames)
    columns += ")" ;

  // (colList->entries() == 0) ==> we have a select count(*) type query or a
  // select 1 from T type query. In other words the table needs to be accessed
  // but no column has been explicitly referenced.
  // For such queries if the user has privilege on any one column that is 
  // sufficient. collectColumnNames indicates whether the user has privilege
  // on at least one column. The following if statement applies only to selects
  // For update and insert we do not expect colList to be empty.

  if ((colList->entries() == 0)&& !collectColumnNames)
  {
    hasPriv = false;
    columns = "";
  }

  if (!hasPriv)
    *CmpCommon::diags() << DgSqlCode(-4481)
                        << DgString0(PrivMgrUserPrivs::convertPrivTypeToLiteral(privType).c_str())
                        << DgString1(table->getTableName().getQualifiedNameAsAnsiString())
                        << DgString2(columns);

  return hasPriv;
  
}


NABoolean RelRoot::checkFirstNRowsNotAllowed(BindWA *bindWA)
{
  // do not call this method on a true root.
  CMPASSERT(NOT isTrueRoot());
 
  //***************************************************************** 
  // FirstNRows >= 0 (for FirstN)
  //            == -2 For Last 0
  //            == -3 For Last 1
  // These values are set in parser; see the code SqlParser.y under
  // Non-Terminal querySpecification when fisrtN is specified
  //******************************************************************

  if ( (getFirstNRows() >= 0 || 
        getFirstNRows() == -2 ||
        getFirstNRows() == -3) &&  // this root has firstn
      (!((getInliningInfo().isEnableFirstNRows()) ||
      (getHostArraysArea() && getHostArraysArea()->getHasSelectIntoRowsets()) || //firstn is allowed with a rowset subroot
      (assignmentStTree()))))  // first n is allowed in a CS. Presence of assignmentStTree
                               // on a non true root implies presence of select into statement
                               // within a cs
        {
          // 4102 The [FIRST/ANY n] syntax can only be used in an outermost SELECT statement.
          if (CmpCommon::getDefault(ALLOW_FIRSTN_IN_SUBQUERIES) == DF_OFF)
            return TRUE;
        }
   return FALSE;
}

// ----------------------------------------------------------------------------
// Method:  checkPrivileges
//
// This method:
//   - Verifies that the user executing the query has the necessary privileges
//   - Adds security keys to RelRoot class  that need to be checked when priv
//     changes (revokes) are performed.  Security keys are part of the Query 
//     Invalidation feature.
//   - Also, removes any previously cached entries if the user has no priv
//
// Input: pointer to the binder work area
// Output:  result of the check
//   TRUE  - user has priv
//   FALSE - user does not have priv or unexpected error occurred
//
// The ComDiags area is populated with error details
// The BindWA flag setFailedForPrivileges is set to TRUE if priv check fails 
// ----------------------------------------------------------------------------
NABoolean RelRoot::checkPrivileges(BindWA* bindWA)
{
  // If internal caller and not part of explain, then return 
  if (Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)) 
    return TRUE;

  // If qiPath (used for testing) is not 0, skip root user check
  NAString qiPath = "";
  CmpCommon::getDefault(QI_PATH, qiPath, FALSE);
  if (qiPath.length() == 0 && ComUser::isRootUserID())
    return TRUE;

  // See if there is anything to check
  //  StoiList contains any tables used in the query
  //  UdrStoiList contains any routines used in the query
  //  CoProcAggrList contains any queries using the aggregate co-processor
  //  SeqValList contains any sequences
  if (bindWA->getStoiList().entries() == 0 &&
      bindWA->getUdrStoiList().entries() == 0 &&
      bindWA->getCoProcAggrList().entries() == 0 &&
      bindWA->getSeqValList().entries() == 0)
    return TRUE;

  // If authorization is not enabled, then return TRUE
  if (!CmpCommon::context()->isAuthorizationEnabled())
    return TRUE;

  ComBoolean QI_enabled = (CmpCommon::getDefault(CAT_ENABLE_QUERY_INVALIDATION) == DF_ON);
  NABoolean RemoveNATableEntryFromCache = FALSE ;

  // Have the ComSecurityKey constructor compute the hash value for the the User's ID.
  // Note: The following code doesn't care about the object's hash value or the resulting 
  // ComSecurityKey's ActionType....we just need the hash value for the User's ID.
  int64_t objectUID = 12345;
  Int32 thisUserID = ComUser::getCurrentUser();
  ComSecurityKey userKey( thisUserID , objectUID
                         , SELECT_PRIV
                         , ComSecurityKey::OBJECT_IS_OBJECT
                        );
  uint32_t userHashValue = userKey.getSubjectHashValue();


  // Set up a PrivMgrCommands class in case we need to get privilege information
  NAString privMDLoc;
  CONCAT_CATSCH(privMDLoc,CmpSeabaseDDL::getSystemCatalogStatic(),SEABASE_PRIVMGR_SCHEMA);
  PrivMgrCommands privInterface(privMDLoc.data(), CmpCommon::diags(), PrivMgr::PRIV_INITIALIZED);
  PrivStatus retcode = STATUS_GOOD;

  // ==> Check privileges for tables used in the query.
  SqlTableOpenInfo * stoi = NULL ;
  OptSqlTableOpenInfo * optStoi = NULL;
  for(Int32 i=0; i<(Int32)bindWA->getStoiList().entries(); i++)
  {
    RemoveNATableEntryFromCache = FALSE ;  // Initialize each time through loop
    optStoi = (bindWA->getStoiList())[i];
    stoi = optStoi->getStoi();
    NATable* tab = optStoi->getTable();
    ComSecurityKeySet secKeySet = tab->getSecKeySet();

    // System metadata tables do not, by default, have privileges stored in the
    // NATable structure.  Go ahead and retrieve them now. 
    PrivMgrUserPrivs *pPrivInfo = tab->getPrivInfo();
    PrivMgrUserPrivs privInfo;
    if (!pPrivInfo)
    {
      secKeySet.clear();
      CmpSeabaseDDL cmpSBD(STMTHEAP);
      if (cmpSBD.switchCompiler(CmpContextInfo::CMPCONTEXT_TYPE_META))
      {
        if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
          *CmpCommon::diags() << DgSqlCode( -4400 );
        return FALSE;
      }
      retcode = privInterface.getPrivileges( tab, thisUserID, privInfo, &secKeySet);
      cmpSBD.switchBackCompiler();

      if (retcode != STATUS_GOOD)
      {
        tab->setRemoveFromCacheBNC(TRUE);
        bindWA->setFailedForPrivileges(TRUE);
        *CmpCommon::diags() << DgSqlCode( -1034 );
        return FALSE;
      }
      pPrivInfo = &privInfo;
    }

    // Check each primary DML privilege to see if the query requires it. If 
    // so, verify that the user has the privilege
    bool insertQIKeys = (QI_enabled && secKeySet.entries() > 0);
    for (int_32 i = FIRST_DML_PRIV; i <= LAST_PRIMARY_DML_PRIV; i++)
    {
      if (stoi->getPrivAccess((PrivType)i))
      {
        if (!pPrivInfo->hasPriv((PrivType)i) && !optStoi->checkColPriv((PrivType)i, pPrivInfo))
          RemoveNATableEntryFromCache = TRUE;
        else
          if (insertQIKeys)    
            findKeyAndInsertInOutputList(secKeySet,userHashValue,(PrivType)(i), bindWA);
      }
    }

    // wait until all the primary DML privileges have been checked before
    // setting failure information
    if ( RemoveNATableEntryFromCache )
    {
       bindWA->setFailedForPrivileges( TRUE );
       tab->setRemoveFromCacheBNC(TRUE); // To be removed by CmpMain before Compilation retry
    }
  }  // for loop over tables in stoi list

  // ==> Check privileges for functions and procedures used in the query.
  NABoolean RemoveNARoutineEntryFromCache = FALSE ;
  if (bindWA->getUdrStoiList().entries())
  {
    for(Int32 i=0; i<(Int32)bindWA->getUdrStoiList().entries(); i++)
    {
      // Privilege info for the user/routine combination is stored in the 
      // NARoutine object.
      OptUdrOpenInfo *udrStoi = (bindWA->getUdrStoiList())[i];
      NARoutine* rtn = udrStoi->getNARoutine();
      PrivMgrUserPrivs *pPrivInfo = rtn->getPrivInfo();

      NABoolean insertQIKeys = FALSE;
      if (QI_enabled && (rtn->getSecKeySet().entries() > 0))
       insertQIKeys = TRUE;

      if (pPrivInfo == NULL) 
      {
        RemoveNARoutineEntryFromCache = TRUE ; 
        *CmpCommon::diags() << DgSqlCode( -1034 );
      }

      // Verify that the user has execute priv
      else
      {
        if (pPrivInfo->hasPriv(EXECUTE_PRIV))
        {
          // do this only if QI is enabled and object has security keys defined
          if ( insertQIKeys )
            findKeyAndInsertInOutputList(rtn->getSecKeySet(), userHashValue, EXECUTE_PRIV, bindWA);
        }

        // plan requires privilege but user has none, report an error
        else
        {
          RemoveNARoutineEntryFromCache = TRUE ; 
          *CmpCommon::diags() 
            << DgSqlCode( -4482 )
            << DgString0( "EXECUTE" )
            << DgString1( udrStoi->getUdrName() );
        }
      }

      if ( RemoveNARoutineEntryFromCache )
      {
        bindWA->setFailedForPrivileges(TRUE);

        // If routine exists in cache, add it to the list to remove
        NARoutineDB *pRoutineDBCache  = bindWA->getSchemaDB()->getNARoutineDB();
        NARoutineDBKey key(rtn->getSqlName(), bindWA->wHeap());
        NARoutine *cachedNARoutine = pRoutineDBCache->get(bindWA, &key);
        if (cachedNARoutine != NULL)
          pRoutineDBCache->moveRoutineToDeleteList(cachedNARoutine, &key);
      }
    }  // for loop over UDRs
  }  // end if any UDRs.

  // ==> Check privs on any CoprocAggrs used in the query.
  for (Int32 i=0; i<(Int32)bindWA->getCoProcAggrList().entries(); i++)
  {
    RemoveNATableEntryFromCache = FALSE ;  // Initialize each time through loop
    ExeUtilHbaseCoProcAggr *coProcAggr = (bindWA->getCoProcAggrList())[i];
    NATable* tab = bindWA->getSchemaDB()->getNATableDB()->
                                   get(coProcAggr->getCorrName(), bindWA, NULL);

    ComSecurityKeySet secKeySet = tab->getSecKeySet();
    Int32 numSecKeys = 0;

    // Privilege info for the user/table combination is stored in the NATable
    // object.
    PrivMgrUserPrivs* pPrivInfo = tab->getPrivInfo();
    PrivMgrUserPrivs privInfo;

    // System metadata tables do not, by default, have privileges stored in the
    // NATable structure.  Go ahead and retrieve them now. 
    if (!pPrivInfo)
    {
      secKeySet.clear();
      CmpSeabaseDDL cmpSBD(STMTHEAP);
      if (cmpSBD.switchCompiler(CmpContextInfo::CMPCONTEXT_TYPE_META))
      {
        if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
          *CmpCommon::diags() << DgSqlCode( -4400 );
        return FALSE;
      }
      retcode = privInterface.getPrivileges( tab, thisUserID, privInfo, &secKeySet);
      cmpSBD.switchBackCompiler();

      if (retcode != STATUS_GOOD)
      {
        bindWA->setFailedForPrivileges( TRUE );
        tab->setRemoveFromCacheBNC(TRUE); // To be removed by CmpMain before Compilation retry
        *CmpCommon::diags() << DgSqlCode( -1034 );
        return FALSE;
      }
      pPrivInfo = &privInfo;
    }

    // Verify that the user has select priv
    // Select priv is needed for EXPLAIN requests, so no special check is done
    NABoolean insertQIKeys = FALSE; 
    if (QI_enabled && (secKeySet.entries()) > 0)
      insertQIKeys = TRUE;
    if (pPrivInfo->hasPriv(SELECT_PRIV))
    {
      // do this only if QI is enabled and object has security keys defined
      if ( insertQIKeys )
        findKeyAndInsertInOutputList(secKeySet, userHashValue, SELECT_PRIV, bindWA );
    }

    // plan requires privilege but user has none, report an error
    else
    {
       bindWA->setFailedForPrivileges( TRUE );
       tab->setRemoveFromCacheBNC(TRUE); // To be removed by CmpMain before Compilation retry
       *CmpCommon::diags()
         << DgSqlCode( -4481 )
         << DgString0( "SELECT" )
         << DgString1( tab->getTableName().getQualifiedNameAsAnsiString() );
    }
  }  // for loop over coprocs

  // ==> Check privs on any sequence generators used in the query.
  for (Int32 i=0; i<(Int32)bindWA->getSeqValList().entries(); i++)
  {
    RemoveNATableEntryFromCache = FALSE ;  // Initialize each time through loop
    SequenceValue *seqVal = (bindWA->getSeqValList())[i];
    NATable* tab = const_cast<NATable*>(seqVal->getNATable());
    CMPASSERT(tab);
    ComSecurityKeySet secKeySet = tab->getSecKeySet();

    // get privilege information from the NATable structure
    PrivMgrUserPrivs *pPrivInfo = tab->getPrivInfo();
    PrivMgrUserPrivs privInfo;
    if (!pPrivInfo)
    {
      secKeySet.clear();
      CmpSeabaseDDL cmpSBD(STMTHEAP);
      if (cmpSBD.switchCompiler(CmpContextInfo::CMPCONTEXT_TYPE_META))
      {
        if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
          *CmpCommon::diags() << DgSqlCode( -4400 );
        return FALSE;
      }
      retcode = privInterface.getPrivileges(tab, thisUserID, privInfo, &secKeySet);
      cmpSBD.switchBackCompiler();
      if (retcode != STATUS_GOOD)
      {
        bindWA->setFailedForPrivileges(TRUE);
        tab->setRemoveFromCacheBNC(TRUE); // Not used until sequences stored in table cache
        *CmpCommon::diags() << DgSqlCode( -1034 );
        return FALSE;
      }
      pPrivInfo = &privInfo;
    }

    // Verify that the user has usage priv
    NABoolean insertQIKeys = FALSE; 
    if (QI_enabled && (secKeySet.entries()) > 0)
      insertQIKeys = TRUE;
    if (pPrivInfo->hasPriv(USAGE_PRIV))
    {
      // do this only if QI is enabled and object has security keys defined
      if ( insertQIKeys )
        findKeyAndInsertInOutputList(secKeySet, userHashValue, USAGE_PRIV, bindWA );
    }

    // plan requires privilege but user has none, report an error
    else
    {
      bindWA->setFailedForPrivileges( TRUE );
      tab->setRemoveFromCacheBNC(TRUE); // To be removed by CmpMain before Compilation retry
      *CmpCommon::diags()
        << DgSqlCode( -4491 )
        << DgString0( "USAGE" )
        << DgString1( tab->getTableName().getQualifiedNameAsAnsiString());
    }
  }  // for loop over sequences

  return !bindWA->failedForPrivileges() ;
}

// ****************************************************************************
// method: findKeyAndInsertInOutputList
//
// This method searches through the list of security keys associated with the
// object to find the best candidate to save in the plan based on the
// privilege required.  If it finds a candidate, it inserts the best candidate 
// into securityKeySet_ member of the RelRoot class.
//
// Security key types currently include:
//   COM_QI_OBJECT_<priv>:   privileges granted directly to the user
//   COM_QI_USER_GRANT_ROLE: privileges granted to the user via a role 
//   COM_QI_USER_GRANT_SPECIAL_ROLE: privileges granted to PUBLIC
//
// Keys are added as follows:
//   if a privilege has been granted to public, 
//     add UserObjectPublicKey
//       invalidation is enforced when priv is revoked from public
//   if a privilege has been granted on a column of an object to a user:
//     add UserColumnKey
//       invalidation is enforced when and column priv is revoked from the user
//   if a privilege has been granted on a column of an object to a role:
//      add to ColumnRoleKeys
//        invalidation is enforced when any priv is revoked from one of these roles
//        or when one of these roles is revoked from the user.   
//   if a privilege has been granted directly on an object to a user: 
//     add UserObjectKey 
//       invalidation is enforced when priv is revoked from the user
//   if a privilege has been granted directly on an object to a role:
//     add an entry to ObjectRoleKeys 
//       invalidation is enforced when priv is revoked from one of these roles,
//       or when one of these roles is revoked from user
//
// ****************************************************************************
void RelRoot::findKeyAndInsertInOutputList( ComSecurityKeySet KeysForTab
                                          , const uint32_t userHashValue
                                          , const PrivType which
                                          , BindWA* bindWA
                                          )
{
   // If no keys associated with object, just return
   if (KeysForTab.entries() == 0)
     return;

   ComSecurityKey * UserColumnKey = NULL;
   ComSecurityKey * RoleColumnKey = NULL;
   ComSecurityKey * UserObjectKey = NULL;
   ComSecurityKey * RoleObjectKey = NULL;
   ComSecurityKey * UserObjectPublicKey = NULL;
   
   // These may be implemented at a later time
   ComSecurityKey * UserSchemaKey = NULL; //privs granted at schema level to user

   // Get action type for UserObjectKey based on the privilege (which)
   // so if (which) is SELECT, then the objectActionType is COM_QI_OBJECT_SELECT
   ComSecurityKey  dummyKey;
   ComQIActionType columnActionType = 
                   dummyKey.convertBitmapToQIActionType ( which, ComSecurityKey::OBJECT_IS_COLUMN );
   ComQIActionType objectActionType =
                   dummyKey.convertBitmapToQIActionType ( which, ComSecurityKey::OBJECT_IS_OBJECT );

   ComSecurityKey * thisKey = NULL;

   // With column level privileges, the user may get privileges from various
   // roles.  Today, we add all roles that may hold the requested privilege.
   // If we ever fully support invalidation keys at the column level, then only 
   // roles that are required to run the query should be added. 
   // For example, 
   //  user gets select on table1: col1, col2 from role1
   //  user gets select on table1: col3, col4 from role2
   //  If the query performs a select for col1, then only changes to role1 are relevant
   //  Today, we include both role1 and role2 
   //  Therefore, if role2 is revoked from the user, invalidation is unnecessarily enforced
   ComSecurityKeySet ColumnRoleKeys(bindWA->wHeap());
   ComSecurityKeySet ObjectRoleKeys(bindWA->wHeap());
   ComSecurityKeySet SchemaRoleKeys(bindWA->wHeap());

   // NOTE: hashValueOfPublic will be the same for all keys, so we generate it only once.
   uint32_t hashValueOfPublic = ComSecurityKey::SPECIAL_OBJECT_HASH;

   // Traverse List looking for ANY appropriate ComSecurityKey 
   for ( Int32 ii = 0; ii < (Int32)(KeysForTab.entries()); ii++ )
   {
      thisKey = &(KeysForTab[ii]);
  
      // See if the key is column related
      if ( thisKey->getSecurityKeyType() == columnActionType )
      {
         if ( thisKey->getSubjectHashValue() == userHashValue )
         {
            // Found a security key for the objectActionType
            if ( ! UserColumnKey )
               UserColumnKey = thisKey;
         }
         // Found a security key for a role associated with the user
         else if (qiSubjectMatchesRole(thisKey->getSubjectHashValue()))
         {
            ColumnRoleKeys.insert(*thisKey);
         }
      }

      // See if the key is object related
      else if ( thisKey->getSecurityKeyType() == objectActionType )
      {
         if ( thisKey->getSubjectHashValue() == userHashValue )
         {
            // Found a security key for the objectActionType
            if ( ! UserObjectKey ) 
               UserObjectKey = thisKey;
         }
         // Found a security key for a role associated with the user
         else if (qiSubjectMatchesRole(thisKey->getSubjectHashValue()))
         {
            ObjectRoleKeys.insert(*thisKey);
         }
      }
     
      else if (thisKey->getSecurityKeyType() == COM_QI_USER_GRANT_SPECIAL_ROLE)
      {
         if (thisKey->getObjectHashValue() == hashValueOfPublic )
         {
            if (! UserObjectPublicKey )
               UserObjectPublicKey = thisKey;
         }
      }

      else {;} // Not right action type, just continue traversing.
   }

   // Determine best key (fewest invalidations required)

   // For now, always add column invalidation keys.  Once full integration of
   // column privileges is implemented, then this code changes
   if (UserColumnKey)
     securityKeySet_.insert(*UserColumnKey);
   else if (ColumnRoleKeys.entries() > 0)
   {
      for (int j = 0; j < ColumnRoleKeys.entries(); j++)
      {
        securityKeySet_.insert(ColumnRoleKeys[j]);

        // add a key in case the role is revoked from the user
        ComSecurityKey roleKey(userHashValue, ColumnRoleKeys[j].getSubjectHashValue());
        securityKeySet_.insert(roleKey);
      }
   }

   //   UserObjectKeys are better than ObjectRoleKeys
   if (UserObjectKey)
      securityKeySet_.insert(*UserObjectKey);

   else if (ObjectRoleKeys.entries() > 0)
   {
      for (int j = 0; j < ObjectRoleKeys.entries(); j++)
      {
        securityKeySet_.insert(ObjectRoleKeys[j]);

        // add a key in case the role is revoked from the user
        ComSecurityKey roleKey(userHashValue, ObjectRoleKeys[j].getSubjectHashValue());
        securityKeySet_.insert(roleKey);
      }
   }

   // Add public if it exists - handles revoke public from user
   if ( UserObjectPublicKey != NULL )
     securityKeySet_.insert(*UserObjectPublicKey); 
}


// -----------------------------------------------------------------------
// member functions for class GroupByAgg
// -----------------------------------------------------------------------

RelExpr *GroupByAgg::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  //
  // add any aggregate functions found in the parent node(s)
  //
  BindScope *currScope = bindWA->getCurrentScope();
  aggregateExpr_ += currScope->getUnresolvedAggregates();
  currScope->getUnresolvedAggregates().clear();
  //
  // Bind the child nodes.
  //
  currScope->context()->lookAboveToDecideSubquery() = TRUE;
  bindChildren(bindWA);
  currScope->context()->lookAboveToDecideSubquery() = FALSE;
  if (bindWA->errStatus()) return this;
  bindWA->getCurrentScope()->setRETDesc(getRETDesc());

  // QSTUFF
  NAString fmtdList(bindWA->wHeap());
  LIST(TableNameMap*) xtnmList(bindWA->wHeap());
  bindWA->getTablesInScope(xtnmList, &fmtdList);

  // can be removed when supporting aggregates on streams
  if (getGroupAttr()->isStream()){
    *CmpCommon::diags() << DgSqlCode(-4162) << DgString0(fmtdList);
    bindWA->setErrStatus();
    return this;
  }

  if ((getGroupAttr()->isEmbeddedUpdateOrDelete()) ||
      (bindWA->isEmbeddedIUDStatement())) {
    *CmpCommon::diags() << DgSqlCode(-4163) << DgString0(fmtdList)
      << (getGroupAttr()->isEmbeddedUpdate() ?
         DgString1("UPDATE"):DgString1("DELETE"));
    bindWA->setErrStatus();
    return this;
  }
  // QSTUFF

  // if unresolved aggregate functions have been found in the children of the
  // Groupby node, that would mean that we are referencing aggregates before
  // the groupby operation is performed
  //
  if (checkUnresolvedAggregates(bindWA))
    return this;
  //
  // Detach the item expression tree for the grouping column list, bind it,
  // convert it to a ValueIdSet, and attach it to the GroupByAgg node.
  //
  ItemExpr *groupExprTree = removeGroupExprTree();
  if (groupExprTree) {
    currScope->context()->inGroupByClause() = TRUE;
    groupExprTree->convertToValueIdSet(groupExpr(), bindWA, ITM_ITEM_LIST);

    if (isRollup())
      groupExprTree->convertToValueIdList(
           rollupGroupExprList(), bindWA, ITM_ITEM_LIST);

    currScope->context()->inGroupByClause() = FALSE;
    if (bindWA->errStatus()) return this;

    ValueIdList groupByList(groupExpr());
 
    for (CollIndex i = 0; i < groupByList.entries(); i++)
    {
      ValueId vid = groupByList[i];
      vid.getItemExpr()->setIsGroupByExpr(TRUE);
    }

    if ((groupExprTree != NULL) &&
        (getParentRootSelectList() != NULL))
      {
        RETDesc * childRETDesc = child(0)->getRETDesc();
        ItemExprList origSelectList(getParentRootSelectList(), bindWA->wHeap());
        
        for (CollIndex i = 0; i < groupByList.entries(); i++)
          {
            ValueId vid = groupByList[i];
            if((vid.getItemExpr()->getOperatorType() == ITM_SEL_INDEX)&&
               (((SelIndex*)(vid.getItemExpr()))->renamedColNameInGrbyClause()))
              {
                ULng32 indx = ((SelIndex*)(vid.getItemExpr()))->getSelIndex() - 1;
                if (origSelectList.entries() > indx && 
                    origSelectList[indx]->getOperatorType() == ITM_RENAME_COL) 
                  {
                    const ColRefName &selectListColRefName = 
                      *((RenameCol *)origSelectList[indx])->getNewColRefName();
                    ColumnNameMap *baseColExpr = 
                      childRETDesc->findColumn(selectListColRefName);
                    if (baseColExpr)
                      {
                        groupExpr().remove(vid);
                        groupExpr().insert(baseColExpr->getValueId());

                        if (isRollup())
                          {
                            CollIndex idx = rollupGroupExprList().index(vid);
                            rollupGroupExprList()[idx] = baseColExpr->getValueId();
                          }
 
                        baseColExpr->getColumnDesc()->setGroupedFlag();
                        origSelectList[indx]->setInGroupByOrdinal(FALSE);
                      }
                  }
              }
          }
        
        if (getSelPredTree()) 
          {
            ItemExpr * havingPred = (ItemExpr *) getSelPredTree();
            
            // see if having expr refers to any renamed col in the select list.
            // that is NOT a name exposed by child RETDesc. 
            // If it does, replace it with SelIndex.
            // For now, do this for special1 mode and only if the having
            // is a simple pred of the form:  col <op> value.
            // Later, we can extend this to all kind of having pred by 
            // traversing the having pred tree and replacing renamed cols.
            NABoolean replaced = FALSE;
            NABoolean notAllowedWithSelIndexInHaving = FALSE;
            replaceRenamedColInHavingWithSelIndex(
                 bindWA, havingPred, origSelectList, replaced,
                 notAllowedWithSelIndexInHaving,child(0)->getRETDesc());
            if (bindWA->errStatus())
              return this;
            if (replaced)
              {
                if (notAllowedWithSelIndexInHaving)
                  {
                    *CmpCommon::diags() << DgSqlCode(-4196) ;
                    bindWA->setErrStatus();
                    return this;
                  }
                
                setSelIndexInHaving(TRUE);
              }
          }
        setParentRootSelectList(NULL);
      }
    
    // Indicate that we are not in a scalar groupby.  Any aggregate
    // functions found in the select list or having clause cannot
    // evaluate to NULL unless their argument is null.
    currScope->context()->inScalarGroupBy() = FALSE;
  }
  //
  // bind the having predicates and attach the resulting value id set
  // to the node (as a selection predicate on the groupby node)
  //
  ItemExpr *havingPred = removeSelPredTree();
  if (havingPred && NOT selIndexInHaving()) 
  {
    currScope->context()->inHavingClause() = TRUE;
    havingPred->convertToValueIdSet(selectionPred(), bindWA, ITM_AND);
    currScope->context()->inHavingClause() = FALSE;
    if (bindWA->errStatus()) 
      return this;
  }

  //
  // Bind the base class.
  //
  RelExpr *boundExpr = bindSelf(bindWA);
  if (bindWA->errStatus()) return boundExpr;

  if ((havingPred) &&
    (selIndexInHaving())) 
  {
    addSelPredTree(havingPred);
  }

  //
  // Get the aggregate expressions from the list that has accumulated
  // in the current bind scope and clear the list in the bind scope --
  //   but first, if Tuple::bindNode()/checkTupleElementsAreAllScalar()
  //   created this node, add the subquery aggregate expr
  //   (Genesis 10-000221-6676).
  //
  if (aggregateExprTree_) { // only Binder, not Parser, should put anything here
    //	CMPASSERT(bindWA->getCurrentScope()->context()->inTupleList());
    CMPASSERT(aggregateExprTree_->nodeIsBound() ||
              aggregateExprTree_->child(0)->nodeIsBound());
    aggregateExprTree_ = aggregateExprTree_->bindNode(bindWA);
    if (bindWA->errStatus()) return boundExpr;
    aggregateExpr_    += aggregateExprTree_->getValueId();
    aggregateExprTree_ = NULL;
  }
  aggregateExpr_ += currScope->getUnresolvedAggregates();
  currScope->getUnresolvedAggregates().clear();
  getRETDesc()->setGroupedFlag();

  return boundExpr;
} // GroupByAgg::bindNode()

// -----------------------------------------------------------------------
// member functions for class Scan
// -----------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////
// A list of 'fabricated' hostvar representing the hostvars is generated
// that will contain the primary key values. These primary key
// values are retrieved at runtime from the cursor statement
// specified in the 'current of' clause. A predicate of the
// form 'where pkey1 = :pkey1 and pkey2 = :pkey2...' is attached
// to the selection pred of this node. The hostvar values are
// then passed in by the root node to its child and they reach
// this node at runtime where the 'where' predicate is evaluated.
////////////////////////////////////////////////////////////////////////
void Scan::bindUpdateCurrentOf(BindWA *bindWA, NABoolean updateQry)
{
  ValueIdList keyList = getTableDesc()->getClusteringIndex()->getIndexKey();
  ItemExpr * rootPtr = NULL;
  char hvName[30];
  CollIndex i = 0;
  for (i = 0; i < keyList.entries(); i++)
    {
      ValueId vid = keyList[i];

      // Fabricate a name for the i'th host variable,
      // make a hostvar,add it to pkeyHvarList.
      sprintf(hvName,"_upd_pkey_HostVar%d",i);

      HostVar *hv = new(bindWA->wHeap()) HostVar(hvName, &vid.getType(), TRUE);
      hv->bindNode(bindWA);

      pkeyHvarList().insert(hv->getValueId());

      // Build a 'pkey = pkey_hvar' predicate.
      ItemExpr * eqPred = new(bindWA->wHeap())
                            BiRelat(ITM_EQUAL, vid.getItemExpr(), hv);

      if (!rootPtr)
        rootPtr = eqPred;
      else
        rootPtr = new(bindWA->wHeap()) BiLogic(ITM_AND, rootPtr, eqPred);
    } // loop over all pkey columns

  if (updateQry)
    {
      ItemExpr * updCheckPtr = NULL;
      ValueIdList nonKeyColList;
      getTableDesc()->getClusteringIndex()->getNonKeyColumnList(nonKeyColList);
      for (i = 0; i < nonKeyColList.entries(); i++)
        {
          ValueId vid = nonKeyColList[i];

          // Fabricate a name for the i'th host variable,
          // make a hostvar,add it to pkeyHvarList.
          sprintf(hvName,"_upd_col_HostVar%d",i);

          HostVar *hv = new(bindWA->wHeap()) HostVar(hvName, &vid.getType(), TRUE);
          hv->bindNode(bindWA);

          pkeyHvarList().insert(hv->getValueId());

          // Build a 'col = col_hvar' predicate.
          ItemExpr * eqPred = new(bindWA->wHeap())
            BiRelat(ITM_EQUAL, vid.getItemExpr(), hv, TRUE);

          if (!updCheckPtr)
            updCheckPtr = eqPred;
          else
            updCheckPtr =
              new(bindWA->wHeap()) BiLogic(ITM_AND, updCheckPtr, eqPred);
        } // loop over all pkey columns

      if (updCheckPtr)
        {
          updCheckPtr = new (bindWA->wHeap())
            Case(NULL,
                 new (bindWA->wHeap())
                   IfThenElse(updCheckPtr,
                              new (bindWA->wHeap()) BoolVal(ITM_RETURN_TRUE),
                              new (bindWA->wHeap())
                                BoolVal(ITM_RETURN_TRUE,
                                        new (bindWA->wHeap())
                                          RaiseError(-(Lng32)EXE_CURSOR_UPDATE_CONFLICT))));

          rootPtr = new(bindWA->wHeap()) BiLogic(ITM_AND, rootPtr, updCheckPtr);
        }
    }

//  rootPtr->bindNode(bindWA);

  // add this new tree to the existing selection predicate
  addSelPredTree(rootPtr);

  bindSelf(bindWA);   // just in case

} // Scan::bindUpdateCurrentOf()

// Every Scan and every GenericUpdate has its own stoi,
// plus copies of some of these stoi's are copied to the BindWA
//
// The scan/gu stoi's will become ex_partn_access stoi's
//
// The stoiList copies in BindWA will have their security
// checked in the binder, in RelRoot::checkPrivileges
//
// Stoi's must exist for every table/view/MV/index.
// Stoi's that are not copied to the BindWA are those for which Ansi mandates
// that no security checking be done (e.g., indexes).
//
OptSqlTableOpenInfo *setupStoi(OptSqlTableOpenInfo *&optStoi_,
                               BindWA *bindWA,
                               const RelExpr *re,
                               const NATable *naTable,
                               const CorrName &corrName,
                               NABoolean noSecurityCheck)
{
  // Get the PHYSICAL (non-Ansi/non-delimited) filename of the table or view.
  CMPASSERT(!naTable->getViewText() || naTable->getViewFileName());
  NAString fileName( naTable->getViewText() ?
                      (NAString)naTable->getViewFileName() :
                       naTable->getClusteringIndex()->
                          getFileSetName().getQualifiedNameAsString(),
                     bindWA->wHeap());

  SqlTableOpenInfo * stoi_ = new (bindWA->wHeap()) SqlTableOpenInfo;

  optStoi_ = new(bindWA->wHeap()) OptSqlTableOpenInfo(stoi_,
                                                      corrName,
                                                      bindWA->wHeap());

  stoi_->setFileName(convertNAString(fileName, bindWA->wHeap()));

  if (naTable->getIsSynonymTranslationDone())
  {
     stoi_->setAnsiName(convertNAString(
                         naTable->getSynonymReferenceName(),
                         bindWA->wHeap())); 
  }
  else
  {
     stoi_->setAnsiName(convertNAString(
                         naTable->getTableName().getQualifiedNameAsAnsiString(),
                         bindWA->wHeap()));
  }

  if(naTable->isUMDTable() || naTable->isSMDTable()
    || naTable->isMVUMDTable() || naTable->isTrigTempTable())
  {
    stoi_->setIsMXMetadataTable(1);
  }

  if (NOT corrName.getCorrNameAsString().isNull())
    {
      NABoolean corrNameSpecified = TRUE;
      if (corrNameSpecified)
        {
          stoi_->setCorrName(convertNAString(
               corrName.getCorrNameAsString(),
               bindWA->wHeap()));
        }
    }

  // Materialized-View is considered as a regular table
  stoi_->setSpecialTable(naTable->getSpecialType() != ExtendedQualName::NORMAL_TABLE &&
    naTable->getSpecialType() != ExtendedQualName::MV_TABLE);
  stoi_->setIsView(naTable->getViewText() ? TRUE : FALSE);

  if (naTable->isHbaseTable())
    stoi_->setIsHbase(TRUE);

  stoi_->setLocationSpecified(corrName.isLocationNameSpecified() || 
    corrName.isPartitionNameSpecified() );

  stoi_->setUtilityOpen(corrName.isUtilityOpenIdSpecified());
  stoi_->setUtilityOpenId(corrName.getUtilityOpenId());

  stoi_->setIsNSAOperation(corrName.isNSAOperation());


  if (! naTable->getViewText())
    stoi_->setIsAudited(naTable->getClusteringIndex()->isAudited());

  switch (re->getOperatorType())
  {
    case REL_UNARY_INSERT:
    case REL_LEAF_INSERT:
      stoi_->setInsertAccess();
      break;
    case REL_UNARY_UPDATE:
      {
        stoi_->setUpdateAccess();
        if (((GenericUpdate*)re)->isMerge())
          stoi_->setInsertAccess();
      }
      break;
    case REL_UNARY_DELETE:
    case REL_LEAF_DELETE:
      {
        stoi_->setDeleteAccess();
        if (((GenericUpdate*)re)->isMerge())
          stoi_->setInsertAccess();
      }
      break;
    case REL_SCAN:
    case REL_LOCK:
    case REL_UNLOCK:
    case REL_HBASE_COPROC_AGGR:
      stoi_->setSelectAccess();
      break;
    case REL_EXE_UTIL:

      stoi_->setSelectAccess();
      stoi_->setInsertAccess();
      stoi_->setUpdateAccess();
      stoi_->setDeleteAccess();
      break;
    default:
      CMPASSERT(FALSE);
  }

  NABoolean validateTS = TRUE;

  if ((naTable->getClusteringIndex() &&
       naTable->getClusteringIndex()->isSystemTable()) ||
      (NOT validateTS))
    stoi_->setValidateTimestamp(FALSE);
  else
    stoi_->setValidateTimestamp(TRUE);


  // MV --
  // For INTERNAL REFRESH statements, leave only the insert on the MV itself.
  if (re->getInliningInfo().isAvoidSecurityCheck() ||
      (bindWA->isBindingMvRefresh() &&
       (!naTable->isAnMV() || !stoi_->getInsertAccess())))
  {
    return NULL;
  }

  
  // In a SCAN, only the topmost view is inserted into BindWA StoiList
  // (thus there will be no security check on underlying views/basetables,
  // as Ansi says there shouldn't).
  if (re->getOperatorType() == REL_SCAN && bindWA->viewCount())
  {
    return NULL;
  }

  // Genesis 10-980306-4309:
  // Ansi says not supposed to be any security check on referenced tables,
  // nor of course on indexes, RIs and temp tables which are not an Ansi
  // notion to begin with.
  if ((naTable->getSpecialType() == ExtendedQualName::TRIGTEMP_TABLE) ||
      (naTable->getSpecialType() == ExtendedQualName::IUD_LOG_TABLE) ||
      (naTable->getSpecialType() == ExtendedQualName::INDEX_TABLE))
  {
    return NULL;
  }

  if (noSecurityCheck)
  {
    return NULL;
  }

  if (re->getOperator().match(REL_ANY_GEN_UPDATE)&& 
      (((GenericUpdate*)re)->getUpdateCKorUniqueIndexKey()))
  {
    return NULL;
  }

  OptSqlTableOpenInfo *stoiInList = NULL;

  for (CollIndex i=0; i < bindWA->getStoiList().entries(); i++)
    if (strcmp(bindWA->getStoiList()[i]->getStoi()->fileName(), fileName) == 0) {
      stoiInList = bindWA->getStoiList()[i];
      break;
    }
  
  if (!stoiInList) {
    stoiInList =
      new(bindWA->wHeap()) OptSqlTableOpenInfo(
					       new (bindWA->wHeap()) SqlTableOpenInfo(*stoi_),
					       corrName,
					       bindWA->wHeap());
    
    stoiInList->setTable((NATable*)naTable);
    bindWA->getStoiList().insert(stoiInList);

    bindWA->hbaseColUsageInfo()->insert((QualifiedName*)&naTable->getTableName());

  } else {
    // This is conceptually equivalent to
    //	stoiInList->AccessFlags |= stoi_->AccessFlags :
    if (stoi_->getInsertAccess()) stoiInList->getStoi()->setInsertAccess();
    if (stoi_->getUpdateAccess()) stoiInList->getStoi()->setUpdateAccess();
    if (stoi_->getDeleteAccess()) stoiInList->getStoi()->setDeleteAccess();
    if (stoi_->getSelectAccess()) stoiInList->getStoi()->setSelectAccess();
  }
  
  return stoiInList;
  
} // setupStoi()

static void bindHint(Hint *hint, BindWA *bindWA)
{
  // bind the index names, make them fully qualified
  for (CollIndex x=0; hint && x<hint->indexCnt(); x++)
    {
      QualifiedName qualIxName((*hint)[x], 1, bindWA->wHeap(), bindWA);
      hint->replaceIndexHint(x, qualIxName.getQualifiedNameAsAnsiString());
    }
}

//----------------------------------------------------------------------------
RelExpr *Scan::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
      return this;
  }

  // -- Triggers
  // Is this a Scan on a temporary table inside the action of a statement trigger?
  if (getTableName().isATriggerTransitionName(bindWA))
    return buildTriggerTransitionTableView(bindWA); // Located in Inlining.cpp

  // -- MV
  // Is this a Scan on a log inside the select statement of a Materialized View?
  // If so - maybe we need to replace this Scan with some other RelExpr tree.
  // Ignore when inDDL() because the log may not exist yet.
  if (!bindWA->inDDL() &&
      getTableName().getSpecialType() == ExtendedQualName::NORMAL_TABLE)
  {
    const MvBindContext *pMvBindContext = bindWA->getClosestMvBindContext();
    if (NULL != pMvBindContext)
    {
      RelExpr *replacementTree =
        pMvBindContext->getReplacementFor(getTableName().getQualifiedNameObj());

      if (replacementTree != NULL)
      {
        // We need to replace the Scan on the base table by some other tree.
        // Make sure this tree has the same name as the Scan.
        const CorrName& baseCorrName = getTableName();
        replacementTree = new(bindWA->wHeap())
          RenameTable(TRUE, replacementTree, baseCorrName);

        // Move any selection predicates on the Scan to the tree.
        replacementTree->addSelPredTree(removeSelPredTree());

        // Bind the tree and return instead of the tree.
        return replacementTree->bindNode(bindWA);
      }
    }
  }

  bindChildren(bindWA);
  if (bindWA->errStatus()) return this;

  // Get the NATable for this object.
  //
  NATable *naTable = bindWA->getNATable(getTableName());
  if (bindWA->errStatus()) 
    return this;

  // Set up stoi.  bindWA->viewCount is altered during expanding the view.
  setupStoi(stoi_, bindWA, this, naTable, getTableName(), noSecurityCheck());

  // If the object is a view, expand the view.
  //
  if (naTable->getViewText()) {

    // Allow view on exception_table or any other special_table_name objects  
    ComBoolean specialTableFlagOn = Get_SqlParser_Flags(ALLOW_SPECIALTABLETYPE);
    if (specialTableFlagOn == FALSE)
    {
        Set_SqlParser_Flags(ALLOW_SPECIALTABLETYPE);
        SQL_EXEC_SetParserFlagsForExSqlComp_Internal(ALLOW_SPECIALTABLETYPE);
    }

    RelExpr * boundView = bindWA->bindView(getTableName(),
                            naTable,
                            accessOptions(),
                            removeSelPredTree(),
                            getGroupAttr(),
                            TRUE/*catmanCollectUsages*/);

    // QSTUFF
    // First we checked whether its a view and if so it must be updatable
    // when using it for stream access or an embedded update or delete
    if (!naTable->isUpdatable() && getGroupAttr()->isEmbeddedUpdateOrDelete()){
      *CmpCommon::diags() << DgSqlCode(-4206)
        << DgTableName(naTable->getTableName().getQualifiedNameAsAnsiString())
        << (getGroupAttr()->isEmbeddedUpdate() ?
        DgString0("UPDATE") : DgString0("DELETE"));
      bindWA->setErrStatus();

//    restore ALLOW_SPECIALTABLETYPE setting
      if (specialTableFlagOn == FALSE)
          Reset_SqlParser_Flags(ALLOW_SPECIALTABLETYPE);
      return NULL;
    }

    if (!naTable->isUpdatable() && getGroupAttr()->isStream()){
      *CmpCommon::diags() << DgSqlCode(-4151)
        << DgTableName(naTable->getTableName().getQualifiedNameAsAnsiString());
      bindWA->setErrStatus();
      if (specialTableFlagOn == FALSE) 
          Reset_SqlParser_Flags(ALLOW_SPECIALTABLETYPE);
      return NULL;
    }


    // Second we make sure the the underlying base table is key sequenced
    // in case of embedded d/u and streams
    if (boundView->getLeftmostScanNode()) {
      // this is not a "create view V(a) as values(3)" kind of a view
      const NATable * baseTable =
        boundView->getLeftmostScanNode()->getTableDesc()->getNATable();

      if (getGroupAttr()->isStream()) {
        if (!baseTable->getClusteringIndex()->isKeySequenced()) {
          *CmpCommon::diags() << DgSqlCode(-4204)
            << DgTableName(
                 baseTable->getTableName().getQualifiedNameAsAnsiString());
          bindWA->setErrStatus();
          if (specialTableFlagOn == FALSE)
            Reset_SqlParser_Flags(ALLOW_SPECIALTABLETYPE);

          return NULL;
        }
      }

      if (getGroupAttr()->isEmbeddedUpdateOrDelete()){
        if (!baseTable->getClusteringIndex()->isKeySequenced()){
          *CmpCommon::diags() << DgSqlCode(-4205)
            << DgTableName(
                 baseTable->getTableName().getQualifiedNameAsAnsiString())
            << (getGroupAttr()->isEmbeddedUpdate() ?
                DgString0("UPDATE") : DgString0("DELETE"));
          bindWA->setErrStatus();
          if (specialTableFlagOn == FALSE)
            Reset_SqlParser_Flags(ALLOW_SPECIALTABLETYPE);

          return NULL;
        }
      }
    }
    // QSTUFF

//    restore ALLOW_SPECIALTABLETYPE setting
    if (specialTableFlagOn == FALSE)
        Reset_SqlParser_Flags(ALLOW_SPECIALTABLETYPE);

    return boundView;
  }


  // -- MV
  // If this is the expanded tree pass during CREATE MV, expand the MV into
  // its SELECT tree, just like a regular view.
  // Do this only for incremental MVs, otherwise they may introduce unsupported
  // operators such as Union.
  if (naTable->isAnMV() && 
      bindWA->isExpandMvTree() &&
      naTable->getMVInfo(bindWA)->isIncremental())
  {
    CMPASSERT(bindWA->inDDL());
    return bindExpandedMaterializedView(bindWA, naTable);
  }

  // Do not allow to select from an un initialized MV
  if (naTable->isAnMV() && !bindWA->inDDL() && !bindWA->isBindingMvRefresh())
  {
    if (naTable->verifyMvIsInitializedAndAvailable(bindWA))
      return NULL;
  }

  // Allocate a TableDesc and attach it to the Scan node.
  // This call also allocates a RETDesc, attached to the BindScope,
  // which we want to attach also to the Scan.
  //
  // disable override schema for synonym
  NABoolean os = FALSE;
  if ( ( bindWA->overrideSchemaEnabled() ) 
    && ( ! naTable->getSynonymReferenceName().isNull() ) )
  { 
    os = bindWA->getToOverrideSchema();
    bindWA->setToOverrideSchema(FALSE);  
  }

  if (getHint())
    bindHint(getHint(), bindWA);

  TableDesc * tableDesc = NULL;

  if ((NOT isHbaseScan()) || (! getTableDesc()))
    {
      tableDesc = bindWA->createTableDesc(naTable, getTableName(), 
					  FALSE, getHint());
    }
  else
    tableDesc = getTableDesc();

  // restore override schema setting
  if ( ( bindWA->overrideSchemaEnabled() ) 
    && ( ! naTable->getSynonymReferenceName().isNull() ) )
    bindWA->setToOverrideSchema(os);  

  // before attaching set the selectivity hint defined by the user for this
  // table

  if (tableDesc && getHint() &&
      getTableName().getSpecialType() == ExtendedQualName::NORMAL_TABLE)
  {
    double s;
    s = getHint()->getSelectivity();
    if (0.0 <= s && s <= 1.0) {
      SelectivityHint *selHint = new (STMTHEAP) SelectivityHint();
      selHint->setScanSelectivityFactor(s);
      tableDesc->setSelectivityHint(selHint);
    }
    if (getHint()->getCardinality() >= 1.0) {
      s = getHint()->getCardinality();
      CostScalar scanCard(s);
      if((scanCard.getValue() - floor(scanCard.getValue())) > 0.00001)
        scanCard = ceil(scanCard.getValue());

      CardinalityHint *cardHint = new (STMTHEAP) CardinalityHint();
      cardHint->setScanCardinality(scanCard);
      tableDesc->setCardinalityHint(cardHint);
    }
  }

  setTableDesc(tableDesc);
  if (bindWA->errStatus()) return this;
  setRETDesc(bindWA->getCurrentScope()->getRETDesc());

  if ((CmpCommon::getDefault(ALLOW_DML_ON_NONAUDITED_TABLE) == DF_OFF) &&
      (naTable && naTable->getClusteringIndex() && !naTable->getClusteringIndex()->isAudited()))
  {
     *CmpCommon::diags() << DgSqlCode(-4211)
       << DgTableName(
           naTable->getTableName().getQualifiedNameAsAnsiString());
     bindWA->setErrStatus();
     return NULL;
  }
 
  // restricted partitions for HBase table
  if (naTable->isHbaseTable() &&
      (naTable->isPartitionNameSpecified() ||
       naTable->isPartitionRangeSpecified()))
    {
      PartitioningFunction * partFunc = naTable->getClusteringIndex()->getPartitioningFunction();

      // find the salt column and apply a predicate on the salt column.
      // For Hash2, since the partittion key columns are columns used to build
      // the _SALT_ column, we need to search all columns for the _SALT_ column.
      const NAColumnArray &ccCols = 
           (partFunc && partFunc->castToHash2PartitioningFunction())?
            naTable->getClusteringIndex()->getAllColumns()
              :
            naTable->getClusteringIndex()->getPartitioningKeyColumns();

      NABoolean saltColFound = FALSE;

      for (CollIndex i=0; i<ccCols.entries() && !saltColFound; i++)
        {
          if (ccCols[i]->isComputedColumn() &&
              ccCols[i]->getColName() == 
              ElemDDLSaltOptionsClause::getSaltSysColName())
            {
              saltColFound = TRUE;
              // create a predicate "_SALT_" = <num> or
              // "_SALT_" between <num> and <num>
              Int32 beginPartNum = partFunc->getRestrictedBeginPartNumber() - 1;
              Int32 endPartNum = partFunc->getRestrictedEndPartNumber() - 1;
              
              // fill in defaults, indicated by -1 (-2 after subtraction above)
              if (beginPartNum < 0)
                beginPartNum = 0;
              if (endPartNum < 0)
                endPartNum = partFunc->getCountOfPartitions() - 1;
              
              ItemExpr *partPred = NULL;
              ColReference *saltColRef = new(bindWA->wHeap()) ColReference(
                   new(bindWA->wHeap()) ColRefName(
                        ccCols[i]->getFullColRefName(), bindWA->wHeap()));
              
              if (beginPartNum == endPartNum)
                {
                  partPred = new(bindWA->wHeap()) BiRelat
                    (ITM_EQUAL,
                     saltColRef,
                     new(bindWA->wHeap()) ConstValue(beginPartNum,bindWA->wHeap()));
                }
              else
                {
                  partPred = new(bindWA->wHeap()) Between
                    (saltColRef,
                     new(bindWA->wHeap()) ConstValue(beginPartNum,bindWA->wHeap()),
                     new(bindWA->wHeap()) ConstValue(endPartNum,bindWA->wHeap()));
                }
              
              ItemExpr *newSelPred = removeSelPredTree();
              if (newSelPred)
                newSelPred = new(bindWA->wHeap()) BiLogic(ITM_AND,
                                                          newSelPred,
                                                          partPred);
              else
                newSelPred = partPred;
              
              // now add the partition predicates
              addSelPredTree(newSelPred->bindNode(bindWA));
            }
        }
      
      if (!saltColFound)
        {
          // not allowed to select individual partitions from HBase tables
          // unless they are salted
          char buf[20];
          snprintf(buf, 20, "%d", partFunc->getRestrictedBeginPartNumber());
          *CmpCommon::diags() << DgSqlCode(-1276)
                              << DgString0(buf)
                              << DgTableName(
                                   naTable->getTableName().getQualifiedNameAsAnsiString());
          bindWA->setErrStatus();
          return NULL;
        }
    }

   if (naTable->isHiveTable() && 
       !(naTable->getClusteringIndex()->getHHDFSTableStats()->isOrcFile() ||
	 naTable->getClusteringIndex()->getHHDFSTableStats()
	 ->isSequenceFile()) &&
       (CmpCommon::getDefaultNumeric(HDFS_IO_BUFFERSIZE_BYTES) == 0) && 
       (naTable->getRecordLength() >
	CmpCommon::getDefaultNumeric(HDFS_IO_BUFFERSIZE)*1024))
     {
       // do not raise error if buffersize is set though buffersize_bytes.
       // Typically this setting is used for testing alone.
       *CmpCommon::diags() << DgSqlCode(-4226)
			   << DgTableName(
					  naTable->getTableName().
					  getQualifiedNameAsAnsiString())
			   << DgInt0(naTable->getRecordLength());
       bindWA->setErrStatus();
       return NULL;
     }
  // Bind the base class.
  //
  RelExpr *boundExpr = bindSelf(bindWA);
  if (bindWA->errStatus()) return this;
  //
  // Assign the set of columns that belong to the table to be scanned
  // as the output values that can be produced by this scan.
  //
  getGroupAttr()->addCharacteristicOutputs(getTableDesc()->getColumnList());
  getGroupAttr()->addCharacteristicOutputs(getTableDesc()->hbaseTSList());

   // MV --
  if (getInliningInfo().isMVLoggingInlined())
    projectCurrentEpoch(bindWA);

  // QSTUFF
  // Second we make sure the the underlying base table is key sequenced in case
  // of embedded d/u and streams
  if (getGroupAttr()->isStream()){

    if (!naTable->getClusteringIndex()->isKeySequenced() ||
        naTable->hasVerticalPartitions()){
      *CmpCommon::diags() << DgSqlCode(-4204)
        << DgTableName(naTable->getTableName().getQualifiedNameAsAnsiString());
      bindWA->setErrStatus();
      return NULL;
    }
    if (!getTableDesc()->getClusteringIndex()->getNAFileSet()->isAudited()) {
      // Stream access not allowed on a non-audited table
      *CmpCommon::diags() << DgSqlCode(-4215)
        << DgTableName(
            naTable->getTableName().getQualifiedNameAsAnsiString());
      bindWA->setErrStatus();
      return NULL;
    }
  }

  if (getGroupAttr()->isEmbeddedUpdateOrDelete()){
    if (!naTable->getClusteringIndex()->isKeySequenced()
      || naTable->hasVerticalPartitions()){
      *CmpCommon::diags() << DgSqlCode(-4205)
        << DgTableName(naTable->getTableName().getQualifiedNameAsAnsiString())
        << (getGroupAttr()->isEmbeddedUpdate() ?
            DgString0("UPDATE") : DgString0("DELETE"));
      bindWA->setErrStatus();
      return NULL;
    }
  }
  // QSTUFF

  // Fix "browse access mode incorrectly starts transaction" genesis case
  // 10-021111-1080. Here's a glimpse at what may have been the original
  // intent of the old code (taken from RelExpr.h comment for the now
  // defunct RelExpr::propagateAccessOptions):
  //
  // At parse time, user can specify statement level access options.
  // (See SQL/ARK Language spec). These options are attached to the
  // RelRoot node and could be different for different Scans in the query.
  // All Scan and Update nodes under a RelRoot have the same Access
  // type and the Lock Mode.
  //
  // The problem is propagateAccessOptions did not visit all the Scans,
  // eg, it did not propagate to subquery Scans, and it did not propagate
  // to internal RelRoots. This "push" model seems harder to understand
  // and to do correctly.
  //
  // So, we go with the "pull" model. An interesting node such as a Scan,
  // GenericUpdate, RelRoot that needs a user-specified access/lock mode
  // can "pull" one from BindWA. BindWA already implements SQL scoping
  // and visibility rules. It's easier to explain also. Each table
  // reference inherits the user-specified access/lock mode of the
  // nearest SQL scope, going from the table outwards. If the entire
  // query has no user-specified access/lock mode, then it uses the
  // session-level default access/lock mode.
  //
  // if we have no user-specified access options then
  // get it from nearest enclosing scope that has one (if any)
  if (!accessOptions().userSpecified()) {
    StmtLevelAccessOptions *axOpts = bindWA->findUserSpecifiedAccessOption();
    if (axOpts) {
      accessOptions() = *axOpts;
    }
  }
  // The above code is in RelRoot::bindNode also.
  // It would be nice to refactor this common code; someday.

  // See Halloween handling code in GenericUpdate::bindNode
  if (accessOptions().userSpecified()) {
    if ( accessOptions().accessType() == TransMode::REPEATABLE_READ_ACCESS_ ||
         accessOptions().accessType() == TransMode::READ_COMMITTED_ACCESS_  ||
         accessOptions().accessType() == TransMode::READ_UNCOMMITTED_ACCESS_
      ) {
        naTable->setRefsIncompatibleDP2Halloween();
    }
  }
  else {
    TransMode::IsolationLevel il = CmpCommon::transMode()->getIsolationLevel();
    if((CmpCommon::transMode()->ILtoAT(il) == TransMode::REPEATABLE_READ_ACCESS_ ) ||
       (CmpCommon::transMode()->ILtoAT(il) == TransMode::READ_COMMITTED_ACCESS_  ) ||
       (CmpCommon::transMode()->ILtoAT(il) == TransMode::READ_UNCOMMITTED_ACCESS_     )) {
        naTable->setRefsIncompatibleDP2Halloween();
    }
  }

  const NAString * tableLockVal = 
      ActiveControlDB()->getControlTableValue(
            getTableName().getUgivenName(), "TABLELOCK");
  if (*tableLockVal == "ON")
        naTable->setRefsIncompatibleDP2Halloween();

  //Embedded update/delete queries on partitioned table
  //generates assertion when ATTEMPT_ASYNCHRONOUS_ACCESS
  //flag is OFF.This is because split operator is used.
  //Removing of split top operator causes some problems.
  //Error 66 from file system is one of them.
  //So, for now compiler will generate error if these
  //conditions occur.
  if (getGroupAttr()->isEmbeddedUpdateOrDelete() &&
       naTable->getClusteringIndex()->isPartitioned() &&
       (CmpCommon::getDefault(ATTEMPT_ASYNCHRONOUS_ACCESS) == DF_OFF)) {

     *CmpCommon::diags() << DgSqlCode(-4321)
       << DgTableName(naTable->getTableName().getQualifiedNameAsAnsiString());
     bindWA->setErrStatus();
     return NULL;
  }
  // Stream access not allowed on a multi-partioned access paths, when
  // CQD ATTEMPT_ASYNCHRONOUS_ACCESS is set to OFF.If we find 
  // that all access paths are partitioned we give an error.

   if (getGroupAttr()->isStream() &&
      (CmpCommon::getDefault(ATTEMPT_ASYNCHRONOUS_ACCESS) == DF_OFF)) {
    NABoolean atleastonenonpartitionedaccess = FALSE;
    NAFileSetList idescList = naTable->getIndexList();
 
    for(CollIndex i = 0; 
        i < idescList.entries() && !atleastonenonpartitionedaccess; i++)
       if(!(idescList[i]->isPartitioned()) )
        atleastonenonpartitionedaccess = TRUE;
 
    if (!atleastonenonpartitionedaccess) {
     *CmpCommon::diags() << DgSqlCode(-4320)
       << DgTableName(naTable->getTableName().getQualifiedNameAsAnsiString());
     bindWA->setErrStatus();
     return NULL;
    }
  }

   if (hbaseAccessOptions_)
     {
       if (hbaseAccessOptions_->isMaxVersions())
         {
           hbaseAccessOptions_->setHbaseVersions
             (
              getTableDesc()->getClusteringIndex()->getNAFileSet()->numMaxVersions()
              );
         }
     }

  return boundExpr;
} // Scan::bindNode()

//----------------------------------------------------------------------------
RelExpr *Scan::bindExpandedMaterializedView(BindWA *bindWA, NATable *naTable)
{
  CollHeap *heap = bindWA->wHeap();
  MVInfoForDML *mvInfo = naTable->getMVInfo(bindWA);
  QualifiedName mvName(mvInfo->getNameOfMV(), 3, heap, bindWA);
  CorrName mvCorrName(mvName, heap, getTableName().getCorrNameAsString());

  RelExpr *viewTree = mvInfo->buildMVSelectTree();
  viewTree = new(heap) RenameTable(TRUE, viewTree, mvCorrName);
  viewTree->addSelPredTree(removeSelPredTree());
  RelExpr *boundExpr = viewTree->bindNode(bindWA);
  if (bindWA->errStatus())
    return this;

  if (naTable->getClusteringIndex()->hasSyskey())
  {
    // In case the MV on top of this MV is an MJV, it needs the SYSKEY
    // column of this MV. Since the SYSKEY column is not projected from
    // the select list of this MV, just fake it. It's value will never be
    // used anyway - just it's existance.
    ConstValue *dummySyskey = new(heap) ConstValue(0);
    dummySyskey->changeType(new(heap) SQLLargeInt(heap));
    ItemExpr *dummySyskeyCol = dummySyskey->bindNode(bindWA);
    if (bindWA->errStatus())
      return this;

    ColRefName syskeyName("SYSKEY", mvCorrName);
    boundExpr->getRETDesc()->addColumn(bindWA,
                                       syskeyName,
                                       dummySyskeyCol->getValueId(),
                                       SYSTEM_COLUMN);
  }

  bindWA->getCurrentScope()->setRETDesc(boundExpr->getRETDesc());
  return boundExpr;
}

//----------------------------------------------------------------------------
// This Scan needs to project the CurrentEpoch column.
// Create and bind the CurrentEpoch function
void Scan::projectCurrentEpoch(BindWA *bindWA)
{
  ItemExpr *currEpoch =
    new(bindWA->wHeap()) GenericUpdateOutputFunction(ITM_CURRENTEPOCH);
  currEpoch->bindNode(bindWA);

  // Add it to the RETDesc
  ColRefName virtualColName(InliningInfo::getEpochVirtualColName());
  getRETDesc()->addColumn(bindWA, virtualColName, currEpoch->getValueId());

  // And force the generator to project it even though it is not
  // a column in the IndexDesc.
  ValueIdSet loggingCols;
  loggingCols.insert(currEpoch->getValueId());
  setExtraOutputColumns(loggingCols);
}

// -----------------------------------------------------------------------
// methods for class Tuple
// -----------------------------------------------------------------------

// Genesis 10-990226-4329 and 10-000221-6676.
static RelExpr *checkTupleElementsAreAllScalar(BindWA *bindWA, RelExpr *re)
{
  if (!re) return NULL;
  RETDesc *rd = re->getRETDesc();
  CMPASSERT(rd);

  // an empty tuple is okay (dummy for Triggers, e.g.)
  const ColumnDescList &cols = *rd->getColumnList();
  for (CollIndex i = cols.entries(); i--; ) {
    ColumnDesc *col = cols[i];
    Subquery *subq = (Subquery *)cols[i]->getValueId().getItemExpr();
    if (subq->isASubquery()) {
      if (cols.entries() > 1 && subq->getDegree() > 1) {
        // 4125 The select list of a subquery in a VALUES clause must be scalar.
        *CmpCommon::diags() << DgSqlCode(-4125);
        bindWA->setErrStatus();
        return NULL;
      }      
      else if (cols.entries() == 1) {  // if cols.entries() > 1 && subq->getDegree() > 1
          // we do not want to make the transformation velow. We want to keep the 
         // values clause, so that it cann be attached by a tsj to the subquery 
        // during transform.
        CMPASSERT(subq->isARowSubquery());
        if (CmpCommon::getDefault(COMP_BOOL_137) == DF_ON)
        {
          ValueIdList subqSelectList;
          RETDesc *subqRD = subq->getSubquery()->getRETDesc()->nullInstantiate(
            bindWA, TRUE/*forceCast for GenRelGrby*/, subqSelectList);
          subq->getSubquery()->setRETDesc(subqRD);
          ItemExpr *agg = new(bindWA->wHeap())
            Aggregate(ITM_ONE_ROW, subqSelectList.rebuildExprTree());
          RelExpr * gby = new(bindWA->wHeap())
            GroupByAgg(subq->getSubquery(), REL_GROUPBY, NULL, agg);
          NABoolean save = bindWA->getCurrentScope()->context()->inTupleList();
          bindWA->getCurrentScope()->context()->inTupleList() = TRUE;
          gby = gby->bindNode(bindWA);
          bindWA->getCurrentScope()->context()->inTupleList() = save;
          return gby;
        }
        else
        {
          return subq->getSubquery();
        }

      }
    }
  }
  return re;
}

RelExpr *Tuple::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }
  
  // Used by rowsets. We search for occurrences of arrays within this node to
  // replace them with scalar variables
  
  if (bindWA->getHostArraysArea() && !bindWA->getHostArraysArea()->done()) 
    {
      RelExpr *boundExpr = bindWA->getHostArraysArea()->modifyTupleNode(this);

      if (boundExpr)
        return checkTupleElementsAreAllScalar(bindWA, boundExpr);
    }

  bindChildren(bindWA);
  if (bindWA->errStatus()) return this;

  // Detach the item expression tree for the value list and bind it.
  // We use counterForRowValues() and pass in parent, for DEFAULT processing
  // (Ansi 7.1 SR 1).
  //
  CollIndex counterRowVals = 0;
  CMPASSERT(!bindWA->getCurrentScope()->context()->counterForRowValues());
  bindWA->getCurrentScope()->context()->counterForRowValues() = &counterRowVals;
  //
  setRETDesc(bindRowValues(bindWA, removeTupleExprTree(), tupleExpr(), this, FALSE));
  if (bindWA->errStatus()) return this;
  //
  bindWA->getCurrentScope()->context()->counterForRowValues() = NULL;

  // Do NOT set currently scoped RETDesc to this VALUES(...) RETDesc --
  // makes  "select * from t where ((values(1)),a) = (1,2);"
  // fail with error 4001 "column A not found, no named tables in scope"
  //
  //	bindWA->getCurrentScope()->setRETDesc(getRETDesc());

  // Bind the base class.
  //
  RelExpr *boundExpr = bindSelf(bindWA);
  // -- Trigger
  if (bindWA->errStatus()) return this;

  //
  //for case 10-020716-5497
  RelExpr *newExpr = checkTupleElementsAreAllScalar(bindWA, boundExpr);

  //before doing anything with newExpr make sure it is not null it can
  //be null if there is an error incheckTupleElementsAreAllScalar.

  getGroupAttr()->addCharacteristicOutputs(tupleExpr());

  return newExpr;
} // Tuple::bindNode()

// -----------------------------------------------------------------------
// methods for class TupleList
// -----------------------------------------------------------------------

RelExpr *TupleList::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  RelExpr * boundExpr = NULL;
  bindChildren(bindWA);
  if (bindWA->errStatus()) return this;

  ExprValueId eVid(tupleExprTree());
  ItemExprTreeAsList tupleList(&eVid, ITM_ITEM_LIST);
  NABoolean castTo = castToList().entries() > 0;

  if (tupleExprTree()->containsSubquery() ||
      tupleExprTree()->containsUDF() 
      #ifndef NDEBUG
      ||
      getenv("UNIONED_TUPLES")
      #endif
     ) {

    // Make a union'ed tree of all the tuples in tupleList.	##
    // This is done coz TupleList doesn't handle transformation	##
    // of subqueries in tuples correctly yet.			##
    CollIndex nTupleListEntries = (CollIndex)tupleList.entries();
    for (CollIndex i = 0; i < nTupleListEntries ; i++) {

      ItemExpr *ituple = tupleList[i]->child(0)->castToItemExpr();
      RelExpr  *rtuple = new(bindWA->wHeap()) Tuple(ituple);
      rtuple = rtuple->bindNode(bindWA);
      if (bindWA->errStatus()) return this;

      // If INSERTing a TupleList, make some Assign's (even tmp's work!)
      // to do some error-checking for MP-NCHAR-as-single-byte target columns.
      //
      // Similar code exists in
      // (a) the loop further down, (b) TupleList::codeGen()
      // and yes, it needs to be in all three places.
      //
      // NOTE:  tmpAssign MUST BE ON HEAP --
      //   Cannot be done with a stack-allocated tmpAssign
      //   because ItemExpr destructor will delete children,
      //   which we (and parent) are still referencing!
      if (castTo) {
        const ColumnDescList &itms = *rtuple->getRETDesc()->getColumnList();
        for (CollIndex j = 0; j < (CollIndex)itms.entries(); j++) {
          ValueId src = itms[j]->getValueId();

          Assign *tmpAssign = new(bindWA->wHeap())
            Assign(castToList()[j].getItemExpr(), src.getItemExpr());
          tmpAssign = (Assign *)tmpAssign->bindNode(bindWA);
          if (bindWA->errStatus()) return this;
        }
      }

      if (!boundExpr)
        boundExpr = rtuple;
      else
        boundExpr = new(bindWA->wHeap()) Union(boundExpr, rtuple);
    } // for loop over tupleList

    CMPASSERT(boundExpr);
    return boundExpr->bindNode(bindWA);

  } // containsSubquery

  // Detach the item expression tree for the value list and bind it.
  // We use counterForRowValues() and pass in parent, for DEFAULT processing
  // (Ansi 7.1 SR 1).
  //
  CollIndex counterRowVals = 0;
  CMPASSERT(!bindWA->getCurrentScope()->context()->counterForRowValues());
  bindWA->getCurrentScope()->context()->counterForRowValues() = &counterRowVals;

  // tupleExprTree() contains a list of tuples.
  // Each tuple is also a list of values (this list may contain one item).
  // Bind all values in all the tuples.
  // Check that the number of elements in each tuple is the same,
  // and that the types of corresponding elements are compatible.
  //
  numberOfTuples_ = tupleList.entries();
  CollIndex prevTupleNumEntries = NULL_COLL_INDEX;

  // A list of ValueIdUnions nodes.  Will create as many as there are
  // entries in each tuple.  The valIds from corresponding elements of
  // the tuples will be added so that each ValueIdUnion represents a
  // column of the tuple virtual table.  Used to determine the
  // union-compatible type to be used for the result type produced by
  // the tuplelist.
  // 
  ItemExprList vidUnions(bindWA->wHeap());
  ValueIdUnion *vidUnion;

  CollIndex i = 0;
  CollIndex nEntries = (CollIndex)tupleList.entries() ;
  for (i = 0; i < nEntries ; i++) {

    counterRowVals = 0;

    ValueIdList vidList;
    ItemExpr *tuple = tupleList[i]->child(0)->castToItemExpr();
    tuple->convertToValueIdList(vidList, bindWA, ITM_ITEM_LIST, this);
    if (bindWA->errStatus())
      return NULL;

    if (prevTupleNumEntries == NULL_COLL_INDEX) {
      prevTupleNumEntries = vidList.entries();
    }
    else if (prevTupleNumEntries != vidList.entries()) {
      // 4126 The row-value-ctors of a VALUES must be of equal degree.
      *CmpCommon::diags() << DgSqlCode(-4126);
      bindWA->setErrStatus();
      return NULL;
    }

    // Genesis 10-980611-7153
    if (castTo && prevTupleNumEntries != castToList().entries()) 
      break;

    for (CollIndex j = 0; j < prevTupleNumEntries; j++) {
      // If any unknown type in the tuple, coerce it to the target type.
      // Also do same MP-NCHAR magic as above.
      ValueId srcVID = vidList[j];
      if (castTo) {
        ValueId src = vidList[j];
        src.coerceType(castToList()[j].getType());

        ItemExpr *srcIE = src.getItemExpr();
        ItemExpr *tgtIE = castToList()[j].getItemExpr();
        
        // tmpAssign MUST BE ON HEAP -- see note above!
        Assign *tmpAssign = new(bindWA->wHeap()) Assign(tgtIE, srcIE);
        tmpAssign = (Assign *)tmpAssign->bindNode(bindWA);
        if (bindWA->errStatus()) 
          return this;

      }
 
      if(i == 0) {
        ValueIdList vids;

        // Create an empty ValueIdUnion.  Will create as many as there
        // are entries in each tuple.  Add the valIds from
        // corresponding elements of the tuples so that each
        // ValueIdUnion represents a column of the tuple virtual
        // table.
        //
        vidUnion = new(bindWA->wHeap()) 
          ValueIdUnion(vids, NULL_VALUE_ID);

        vidUnion->setWasDefaultClause(TRUE);

        vidUnions.insertAt(j, vidUnion);
      }

      // Add the valIds from corresponding elements of the tuples so
      // that each ValueIdUnion represents a column of the tuple
      // virtual table.
      //
      vidUnion = (ValueIdUnion *)vidUnions[j];
      vidUnion->setSource((Lng32)i, vidList[j]);
        
      if (NOT vidList[j].getItemExpr()->wasDefaultClause())
        vidUnion->setWasDefaultClause(FALSE);
    } // for loop over entries in tuple

  } // for loop over tupleList

  if (castTo && prevTupleNumEntries != castToList().entries()) 
  {

    // 4023 degree of row value constructor must equal that of target table
    *CmpCommon::diags() << DgSqlCode(-4023)
                        << DgInt0((Lng32)prevTupleNumEntries)
                        << DgInt1((Lng32)castToList().entries());
    bindWA->setErrStatus();
    return NULL;
  }

  // do INFER_CHARSET fixup
  if (!doInferCharSetFixup(bindWA, CharInfo::ISO88591, prevTupleNumEntries, 
                           tupleList.entries())) {
    return NULL;
  }

  ItemExpr * outputList = NULL;
  for (CollIndex j = 0; j < prevTupleNumEntries; j++) {

    // Get the ValueIdUnion node corresponding to this column of the
    // tuple list virtual table
    //
    vidUnion = (ValueIdUnion *)vidUnions[j];

    if (castTo) {
      // Make sure the place holder type can support all the values in
      // the tuple list and target column
      //
      vidUnion->setSource(numTuples(), castToList()[j]);

      vidUnion->setIsCastTo(TRUE);
    }

    vidUnion->bindNode(bindWA);
    if (bindWA->errStatus())
      return NULL;

    if (castTo) {
      // Check that the source and target types are compatible.
      //   Cannot be done with a stack-allocated tmpAssign
      //   because ItemExpr destructor will delete children,
      //   which we (and parent) are still referencing!
      Assign *tmpAssign = new(bindWA->wHeap())
        Assign(castToList()[j].getItemExpr(), vidUnion);

      if ( CmpCommon::getDefault(ALLOW_IMPLICIT_CHAR_CASTING) == DF_ON )
      {
        tmpAssign->tryToDoImplicitCasting(bindWA);
      }
      const NAType *targetType = tmpAssign->synthesizeType();
      if (!targetType) {
        bindWA->setErrStatus();
        return NULL;
      }
    }
  

    NAType *phType = vidUnion->getValueId().getType().newCopy(bindWA->wHeap());

    NATypeToItem *placeHolder = new(bindWA->wHeap()) NATypeToItem(phType);

    Cast * cnode;
    if (castTo)
      {
        cnode = new(bindWA->wHeap()) Cast(placeHolder, phType, ITM_CAST, TRUE);
        if (vidUnion->getValueId().getItemExpr()->wasDefaultClause())
          cnode->setWasDefaultClause(TRUE);
      }
    else
      cnode = new(bindWA->wHeap()) Cast(placeHolder, phType);
    cnode->setConstFoldingDisabled(TRUE);
    cnode->bindNode(bindWA);

    if (!outputList)
      outputList = cnode;
    else
      outputList = new(bindWA->wHeap()) ItemList(outputList, cnode);
  }

  setRETDesc(bindRowValues(bindWA, outputList, tupleExpr(), this, FALSE));

  if (bindWA->errStatus()) return this;

  bindWA->getCurrentScope()->context()->counterForRowValues() = NULL;

  // Bind the base class.
  //
  boundExpr = bindSelf(bindWA);
  if (bindWA->errStatus()) return this;

   // need to add system columns as well....?

  NABoolean inSubquery = FALSE;
  BindScope *currScope   = bindWA->getCurrentScope();
  BindScope *prevScope   = bindWA->getPreviousScope(currScope);
  if (prevScope)
    inSubquery = prevScope->context()->inSubquery();

  if (inSubquery)
  {
    // need to change tupleExpr() & make it null-instantiated as RETDesc stores
    // null instantiated columns (most probably these are constants, but not
    // necessarily)

    const ColumnDescList *viewColumns = getRETDesc()->getColumnList();

    tupleExpr().clear();
    for (CollIndex k=0; k < viewColumns->entries(); k++)
    {
      ValueId vid = (*viewColumns)[k]->getValueId();

      // Special logic in Normalizer to optimize away a LEFT JOIN is not to
      // be explored there, as this is not a LEFT JOIN
      // Genesis case: 10-010312-1675
      // If the query were to be a LEFT JOIN, we would not be here
      if (vid.getItemExpr()->getOperatorType() == ITM_INSTANTIATE_NULL)

        {
          ((InstantiateNull *)vid.getItemExpr())->NoCheckforLeftToInnerJoin
                       = TRUE;
        }

      tupleExpr().insert(vid);
    }
  }
  getGroupAttr()->addCharacteristicOutputs(tupleExpr());

  return boundExpr;
} // TupleList::bindNode()

// set vidlist = ith tuple of this tuplelist and return TRUE
RelExpr* TupleList::getTuple
(BindWA *bindWA, ValueIdList& vidList, CollIndex i)
{
  ExprValueId eVid(tupleExprTree());
  ItemExprTreeAsList tupleList(&eVid, ITM_ITEM_LIST);
  ItemExpr *tuple = tupleList[i]->child(0)->castToItemExpr();
  tuple->convertToValueIdList(vidList, bindWA, ITM_ITEM_LIST, this);
  return bindWA->errStatus() ? NULL : this;
}

// set needsFixup to TRUE iff tuplelist needs INFER_CHARSET fixup
RelExpr*
TupleList::needsCharSetFixup(BindWA *bindWA, 
                             CollIndex arity, 
                             CollIndex nTuples,
                             NAList<NABoolean> &strNeedsFixup,
                             NABoolean &needsFixup)
{
  // assume it needs no INFER_CHARSET fixup until proven otherwise
  needsFixup = FALSE;
  if (CmpCommon::wantCharSetInference()) {
    CollIndex t, x;
    for (x = 0; x < arity; x++) { // initialize
      strNeedsFixup.insert(FALSE); 
    }
    // go thru tuplelist looking for unprefixed string literals
    for (t = 0; t < nTuples; t++) {
      // get tuple
      ValueIdList tup;
      if (!getTuple(bindWA, tup, t)) {
        return NULL; // something wrong
      } 
      else {
        // go thru columns of tuple looking for unprefixed string literals
        for (x = 0; x < arity; x++) {
          if (!strNeedsFixup[x] && tup[x].inferableCharType()) {
            strNeedsFixup[x] = TRUE;
            needsFixup = TRUE;
          }
        }
      }
    }
  }
  return this; // all OK
}

// find fixable strings' inferredCharTypes
RelExpr*
TupleList::pushDownCharType(BindWA *bindWA,
                            enum CharInfo::CharSet cs,
                            NAList<const CharType*> &inferredCharType,
                            NAList<NABoolean> &strNeedsFixup,
                            CollIndex arity, 
                            CollIndex nTuples)
{
  // mimic CharType::findPushDownCharType() logic
  const CharType* dctp = CharType::desiredCharType(cs);
  NAList<const CharType*> sampleCharType(CmpCommon::statementHeap(),arity);
  NAList<Int32> total(CmpCommon::statementHeap(),arity);
  NAList<Int32> ct   (CmpCommon::statementHeap(),arity);
  CollIndex t, x;
  for (x = 0; x < arity; x++) { // initialize
    total.insert(0);
    ct.insert(0);
    sampleCharType.insert(NULL);
  }
  // go thru tuplelist looking for fixable strings' inferredCharType
  for (t = 0; t < nTuples; t++) {
    // get tuple
    ValueIdList tup;
    if (!getTuple(bindWA, tup, t)) {
      return NULL; // something wrong
    }
    else {
      // go thru tuple looking for fixable strings' inferredCharType
      for (x = 0; x < arity; x++) {
        if (strNeedsFixup[x]) {
          total[x] += 1;
          const CharType *ctp;
          if (tup[x].hasKnownCharSet(&ctp)) {
            ct[x] += 1;
            if (sampleCharType[x] == NULL) {
              sampleCharType[x] = ctp;
            }
          }
        }
      }
    }
  }
  for (x = 0; x < arity; x++) {
    if (ct[x] == total[x]) { 
      // all have known char set or none need fixup
      inferredCharType.insert(NULL); // nothing to fix
    }
    else {
      inferredCharType.insert(sampleCharType[x] ? sampleCharType[x] : dctp);
    }
  }
  return this; // all OK
}

// do INFER_CHARSET fixup
RelExpr*
TupleList::doInferCharSetFixup(BindWA *bindWA,
                               enum CharInfo::CharSet cs,
                               CollIndex arity, 
                               CollIndex nTuples)
{
  NABoolean needsFixup;
  NAList<NABoolean> strNeedsFixup(CmpCommon::statementHeap(),arity);
  RelExpr *result = needsCharSetFixup
    (bindWA, arity, nTuples, strNeedsFixup, needsFixup);
  if (!result || // something went wrong
      !needsFixup) { // no INFER_CHARSET fixup needed
    return result;
  }
  else { // some string literal needs INFER_CHARSET fixup
    NAList<const CharType*> inferredCharType(CmpCommon::statementHeap(),arity);
    if (!pushDownCharType(bindWA, cs, inferredCharType, 
                          strNeedsFixup, arity, nTuples)) {
      return NULL; // something went wrong
    }
    else {
      // go thru tuplelist fixing up literals' char sets
      CollIndex t, x;
      for (t = 0; t < nTuples; t++) {
        // get tuple
        ValueIdList tup;
        if (!getTuple(bindWA, tup, t)) {
          return NULL; // something went wrong
        }
        else {
          // go thru tuple fixing up literals' char sets
          for (x = 0; x < arity; x++) {
            if (strNeedsFixup[x] && tup[x].inferableCharType()) {
              // coerce literal to have column's inferred char set
              tup[x].coerceType(*(inferredCharType[x]), NA_CHARACTER_TYPE);
            }
          }
        }
      }
    }
  }
  return this;
}

// -----------------------------------------------------------------------
// member functions for class RenameTable
// -----------------------------------------------------------------------

RelExpr *RenameTable::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc()); // -- Triggers
    return this;
  }

  //
  // Create a new table name scope.
  //
  bindWA->getCurrentScope()->xtnmStack()->createXTNM();

  // code to enforce the specification that if an index expression is specified
  // with a rowset and the index is included in the derived table, the index
  // must be the last column of the derived column list
  if((getTableName().getCorrNameAsString() != "Rowset___") && (getArity() != 0))
    {
       if(child(0)->getOperatorType() == REL_ROWSET)
         {
           NAString indexExpr(bindWA->wHeap());
           NAString lastString("", bindWA->wHeap());
           ItemExpr *tempPtr;

           indexExpr = ((Rowset *)getChild(0))->getIndexName();
           if((indexExpr != "") && newColNamesTree_)
             {
                for (tempPtr = newColNamesTree_; tempPtr; tempPtr=tempPtr->child(1))
                  {
                     Int32 arity = tempPtr->getArity();
                     if(arity == 1)
                       {
                          lastString = ((RenameCol *)tempPtr)->getNewColRefName()->getColName();
                       }
                  }
                if(indexExpr != lastString)
                  {
                     *CmpCommon::diags() << DgSqlCode(-30012)
                              << DgString0(indexExpr)
                              << DgString1(getTableName().getCorrNameAsString());
                     bindWA->setErrStatus();
                     return NULL;
                  }
             }
         }
    }

  //
  // Bind the child nodes.
  //
  bindChildren(bindWA);
  if (bindWA->errStatus()) return this;

  //
  // Remove the table name scope.
  //
  bindWA->getCurrentScope()->xtnmStack()->removeXTNM();
  //
  // Create the result table.
  //
  RETDesc *resultTable = new (bindWA->wHeap()) RETDesc(bindWA);
  const RETDesc &sourceTable = *child(0)->getRETDesc();
  const CorrName &tableName = getTableName();
  ItemExpr *derivedColTree = removeColNameTree();
  ItemExprList derivedColList(bindWA->wHeap());
  const NAString *simpleColNameStr;
  CollIndex i;
  //
  // Check that there are an equal number of columns to values.
  //
  if (derivedColTree) {
    derivedColList.insertTree(derivedColTree);
    if (derivedColList.entries() != sourceTable.getDegree()) {
      // 4016 The number of derived columns must equal the degree of the derived table.
      *CmpCommon::diags() << DgSqlCode(-4016)
        << DgInt0(derivedColList.entries()) << DgInt1(sourceTable.getDegree());
      bindWA->setErrStatus();
      delete resultTable;
      return this;
    }
  }
  //
  // Insert the derived column names into the result table.
  // By ANSI 6.3 SR 6 (applies to explicit derived column list),
  // duplicate names are not allowed.
  // If user did not specify a derived column name list,
  // expose the select list's column names (implicit derived column list);
  // ANSI does not say that these cannot be duplicates --
  // if there's a later (i.e. in an outer scope) reference to a duplicately
  // named column, ColReference::bindNode will issue an error
  // (in particular, if all references are to constants, e.g. "count(*)",
  // then duplicates are not disallowed in the implicit derived column list!).
  //
  // When Create View DDL uses this Binder, we must enforce
  // ANSI 11.19 SR 8 + 9, clearly disallowing dups/ambigs
  // (and disallowing implem-dependent names, i.e. our unnamed '(expr)' cols!).
  //
  for (i = 0; i < sourceTable.getDegree(); i++) {
    //
    if (derivedColTree) {      // explicit derived column list
      CMPASSERT(derivedColList[i]->getOperatorType() == ITM_RENAME_COL);
      simpleColNameStr = &((RenameCol *) derivedColList[i])->
                                     getNewColRefName()->getColName();
      if (*simpleColNameStr != "") {   // named column, not an expression
        if (resultTable->findColumn(*simpleColNameStr)) {
          ColRefName errColName(*simpleColNameStr, tableName);
          // 4017 Derived column name $ColumnName was specified more than once.
          *CmpCommon::diags() << DgSqlCode(-4017)
            << DgColumnName(errColName.getColRefAsAnsiString());
          bindWA->setErrStatus();
          delete resultTable;
          return this;
        }
      }
    } else            // implicit derived column list
      simpleColNameStr = &sourceTable.getColRefNameObj(i).getColName();
    //
    ColRefName colRefName(*simpleColNameStr, tableName);
    ValueId valId = sourceTable.getValueId(i);
    resultTable->addColumn(bindWA, colRefName, valId);
  } // for-loop
  //
  // Insert system columns similarly, completely ignoring dup names.
  //
  const ColumnDescList &sysColList = *sourceTable.getSystemColumnList();
  for (i = 0; i < sysColList.entries(); i++) {
    simpleColNameStr = &sysColList[i]->getColRefNameObj().getColName();
    if (NOT resultTable->findColumn(*simpleColNameStr)) {
      ColRefName colRefName(*simpleColNameStr, tableName);
      ValueId valId = sysColList[i]->getValueId();   // (slight diff from the
      resultTable->addColumn(bindWA, colRefName, valId, SYSTEM_COLUMN);   //above)
    }
  }
  setRETDesc(resultTable);

  // MVs --
  // When binding INTERNAL REFRESH commands, the SYSKEY and @OP columns should
  // be propageted to the scope above, even when they are not specified in the
  // select list.
  if (bindWA->isPropagateOpAndSyskeyColumns())
    getRETDesc()->propagateOpAndSyskeyColumns(bindWA, FALSE);

  bindWA->getCurrentScope()->setRETDesc(resultTable);
  //
  // Insert the table name into the XTNM,
  // casting away constness on the correlation name
  // in order to have default cat+sch filled in.
  //
  bindWA->getCurrentScope()->getXTNM()->insertNames(bindWA,
                                                    (CorrName &)tableName);

  if (bindWA->errStatus()) {
    delete resultTable;
    return this;
  }

  if (getViewNATable())
  {
    const NATable * natable = getViewNATable() ;
    const ColumnDescList &columnsRET = *(resultTable->getColumnList());
    for (i = 0; i < natable->getColumnCount(); i++)
    {
      columnsRET[i]->setViewColPosition(
        ((natable->getNAColumnArray())[i])->getPosition());
      columnsRET[i]->setViewFileName((const char*)natable->getViewFileName());
    }
  }

  //
  // Bind the base class.
  //
  return bindSelf(bindWA);
} // RenameTable::bindNode()

// -----------------------------------------------------------------------
// member functions for class RenameReference
// -----------------------------------------------------------------------

// This method replaces the RETDesc of the current scope, with a new RETDesc
// that contains the columns of the transition values (OLD@ and NEW@) but
// with correlation names specified by the user in the REFERENCING clause
// of the row trigger.
void RenameReference::prepareRETDescWithTableRefs(BindWA *bindWA)
{
  CollIndex  refsToFind = getRefList().entries();
  CollIndex  refsFound = 0;
  RETDesc   *retDesc;

  // First find the NEW@ and OLD@ tables in one of the scopes.
  BindScope *scope = bindWA->getCurrentScope();
  // For each BindScope,
  while ((scope!=NULL) && (refsToFind > refsFound))
  {    // until we find all the references.
    retDesc = scope->getRETDesc();
    // Skip if an empty RETDesc
    if ((retDesc!=NULL) && !retDesc->isEmpty())
    {
      // For each reference to change
      for (CollIndex i=0; i<refsToFind; i++)
        // Find the table name in the RETDesc, and save a pointer to it's
        // column list in the TableRefName object.
        if(getRefList().at(i).lookupTableName(retDesc))
          refsFound++;
    }

    // Get the next BindScope to search.
    scope = bindWA->getPreviousScope(scope);
  } // while not done

  RETDesc *resultTable = new (bindWA->wHeap()) RETDesc(bindWA);

  // Create an empty RETDesc for the current scope.
  bindWA->getCurrentScope()->setRETDesc(resultTable);

  // For each table reference, add to the RETDesc of the current scope, the
  // columns of the columns of the referenced tables with the new referencing
  // names as correlation names.
  for (CollIndex i=0; i<refsToFind; i++)
    getRefList()[i].bindRefColumns(bindWA);
}

// The RenaneReference node renames values flowing down through it.
// It is used above a row trigger body, to implement the REFERENCING clause
// of the trigger definition - renaming the OLD and NEW transition variables
// to user specified names.
//
// This bind is top-down, so we first prepare the RETDesc, and then bind
// the children using this RETDesc.
RelExpr *RenameReference::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // Save the current RETDesc.
  RETDesc *prevRETDesc = bindWA->getCurrentScope()->getRETDesc();

  // Replace the RETDesc of the current scope with one that contains the user
  // names (MY_NEW, MY_OLD) instead of the reference names (NEW@, OLD@).
  prepareRETDescWithTableRefs(bindWA);

  // Bind the child nodes, in a new BindScope.
  // If we don't open a new scope here, the bindChildren() method will
  // overwrite the RETDesc of the current scope with NULL.
  bindWA->initNewScope();

  bindChildren(bindWA);
  if (bindWA->errStatus())
    return this;

  // Bind the base class.
  RelExpr *boundNode = bindSelf(bindWA);

  // Save this scope's outer references before removing the scope.
  const ValueIdSet myOuterRefs = bindWA->getCurrentScope()->getOuterRefs();

  setRETDesc(bindWA->getCurrentScope()->getRETDesc());

  bindWA->removeCurrentScope();
  bindWA->getCurrentScope()->setRETDesc(prevRETDesc);

  // Now merge the outer references into the previous scope.
  bindWA->getCurrentScope()->mergeOuterRefs(myOuterRefs, FALSE);

  return boundNode;
}  // RenameReference::bindNode()

// -----------------------------------------------------------------------
// member functions for class BeforeTrigger
// -----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
// Find the name and position of a column SET to by this before Trigger.
// The targetColName is an output parameter, saving the bindSetClause()
// method the work of finding the column name.
// The naTable parameter is NULL during DML. and is only used for DDL
// semantic checks.
//////////////////////////////////////////////////////////////////////////////
Lng32 BeforeTrigger::getTargetColumn(CollIndex i,  // Index of Assign expr.
                                    ColRefName* targetColName,
                                    const NATable *naTable)
{
  ItemExpr *currentAssign = setList_->at(i);
  CMPASSERT(currentAssign->getOperatorType() == ITM_ASSIGN);

  ItemExpr *targetColReference = currentAssign->child(0);
  CMPASSERT(targetColReference->getOperatorType() == ITM_REFERENCE);
  ColRefName& targetColRefName =
    ((ColReference *)targetColReference)->getColRefNameObj();

  if (targetColName != NULL)  // return the column name to the binder.
    *targetColName = targetColRefName;

  const NAString& colName = targetColRefName.getColName();

  // If called during DML binding of the BeforeTrigger node, the
  // column position will not be used, because the check for duplicate
  // SET columns was done in DDL time.
  if (naTable == NULL)
    return 0;

  // We get here from DDL binding of the BeforeTrigger node, or from
  // the Inlining code.
  NAColumn *colObj  = naTable->getNAColumnArray().getColumn(colName);

  // If colObj is NULL, it's a bad column name.
  if (colObj == NULL)
    return -1;

  return colObj->getPosition();
}

//////////////////////////////////////////////////////////////////////////////
// This method is called only during DDL (CREATE TRIGGER) of a before trigger
// with a SET clause.
// Each of the columns updated by the SET clause goes through several
// semantic checks, that cannot be done in the parser.
//////////////////////////////////////////////////////////////////////////////
void BeforeTrigger::doSetSemanticChecks(BindWA *bindWA, RETDesc *origRETDesc)
{
  UpdateColumns localCols = UpdateColumns(FALSE);
  ColRefName currentCol;
  const NATable *scanNaTable = NULL;
  NABoolean isUpdateOp=FALSE;

  Scan *scanNode = getLeftmostScanNode();
  CMPASSERT(scanNode != NULL);
  scanNaTable = scanNode->getTableDesc()->getNATable();

  CorrName oldCorr(OLDCorr);
  if (origRETDesc->getQualColumnList(oldCorr))
    isUpdateOp = TRUE;

  for (CollIndex i=0; i<setList_->entries(); i++)
  {
    // Get the name and position of the Assign target column.
    Lng32 targetColPosition = getTargetColumn(i, &currentCol, scanNaTable);

    if (!currentCol.getCorrNameObj().isATriggerTransitionName(bindWA, TRUE))
    {
      // 11017 Left hand of SET assignment must be qualified with the name of the NEW transition variable
      *CmpCommon::diags() << DgSqlCode(-11017) ; // must be NEW name
      bindWA->setErrStatus();
      return;
    }

    if (targetColPosition == -1)
    {
      // 11022 Column $0~ColumnName is not a column in table $0~TableName
      NAString tableName = scanNaTable->getTableName().getQualifiedNameAsString();
      *CmpCommon::diags() << DgSqlCode(-11022)
        << DgColumnName(currentCol.getColName())
        << DgTableName(tableName);
      bindWA->setErrStatus();
      return;
    }

    // We need to check for duplicate SET columns in DDL time only.
    if (localCols.contains(targetColPosition))
    {
      // 4022 column specified more than once
      *CmpCommon::diags() << DgSqlCode(-4022)
        << DgColumnName(currentCol.getColName());
      bindWA->setErrStatus();
      return;
    }
    localCols.addColumn(targetColPosition);

    // Is this a SET into a column that is part of the clustering key?
    // This is only allowed on Inserts, not on Updates (Deletes never get here).
    if (isUpdateOp  &&
      scanNaTable->getNAColumnArray().getColumn(targetColPosition)->isClusteringKey())
    {
      // 4033 Column $0~ColumnName is a primary or clustering key column and cannot be updated.
      *CmpCommon::diags() << DgSqlCode(-4033)
        << DgColumnName(currentCol.getColName());
      bindWA->setErrStatus();
      return;
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// This method is called for before triggers that use the SET clause.
// For each column to be set using SET MYNEW.<colname> = <setExpr> do:
// 1. Find NEW@.<colname> in origRETDesc.
// 2. Verify that there is such a column, and that the user is allowd to
//    change it.
// 3. Get the column's ItemExpr expression, and save it in passThruExpr.
// 4. Create an ItemExpr tree as follows:
//                     case
//                       |
//                  IfThenElse
//                 /     |     \
//        condition    setExpr  passThruExpr
//
//    where condition is the WHEN clause expression.
// 5. Bind this new expression in the RETDesc of the current scope.
// 6. remove NEW@.<colname> from origRETDesc, and re-insert it as the new
//    expression.
//////////////////////////////////////////////////////////////////////////////
void BeforeTrigger::bindSetClause(BindWA *bindWA, RETDesc *origRETDesc, CollHeap *heap)
{
  // Semantic checks are only needed during DDL.
  if (bindWA->inDDL())
  {
    doSetSemanticChecks(bindWA, origRETDesc);
    if (bindWA->errStatus())
      return;
  }

  CorrName newCorr(NEWCorr);
  const TableRefName *newRefName = getRefList().findTable(newCorr);
  CMPASSERT(newRefName!=NULL);
  CorrName newRef = newRefName->getTableCorr();
  ColRefName currentCol;

  // For each Assign expression in the list.
  for (CollIndex i=0; i<setList_->entries(); i++)
  {
    // Get the name and position of the Assign target column.
    Lng32 targetColPosition = getTargetColumn(i, &currentCol, NULL);

    currentCol.getCorrNameObj() = newRef;
    ItemExpr *setExpr = setList_->at(i)->child(1);
    // Find the current value of this NEW@ column.
    ColumnNameMap *currentColExpr = origRETDesc->findColumn(currentCol);
    CMPASSERT(currentColExpr != NULL); // Otherwise we would have been thrown with error 11022 - see above.
    ItemExpr *passThruExpr = currentColExpr->getValueId().getItemExpr();

    ItemExpr *colExpr = NULL;
    if (whenClause_ == NULL)
      // After we add the support for reading the trigger status from
      // the resource fork, and adding it to the condition, we should
      // never get here.
      colExpr = setExpr;
    else
    {
      IfThenElse *ifExpr = new(heap)
        IfThenElse(whenClause_, setExpr, passThruExpr);
      colExpr = new(heap) Case(NULL, ifExpr);
    }
    colExpr = colExpr->bindNode(bindWA);
    if (bindWA->errStatus())
    return;

    // Now remove and re-insert the column to the original RETDesc,
    // that will be restored at the bottom of the method.
      currentCol.getCorrNameObj() = newCorr;
    origRETDesc->delColumn(bindWA, currentCol, USER_COLUMN);
    origRETDesc->addColumn(bindWA, currentCol, colExpr->getValueId());

          // force binding of the assign here so that type incompatability is caught
          // during DDL
          if (bindWA->inDDL())
          {
               ItemExpr *currentAssign = setList_->at(i);
               CMPASSERT(currentAssign->getOperatorType() == ITM_ASSIGN);
               currentAssign->bindNode(bindWA);
          }
  }
}

//////////////////////////////////////////////////////////////////////////////
// This method is called for before triggers that use the SIGNAL clause.
// 1. Find the "virtual execId column" in origRETDesc.
// 3. Get the column's ItemExpr expression, and save it in passThruExpr.
// 4. Create an ItemExpr tree as follows:
//                     case
//                       |
//                  IfThenElse
//                 /     |       \
//              AND  passThruExpr passThruExpr
//            /    \
//    condition     RaiseError
//
//    where condition is the WHEN clause expression, and RaiseError is the
//    SIGNAL expression.
// 5. Bind this new expression in the RETDesc of the current scope.
// 6. remove "virtual execId column" from origRETDesc, and re-insert it as
//    the new expression.
//
// The value of the expression is always the passThruExpr, for type
// compatibility. since if the SIGNAL fires, the actual value returned does
// not matter. The AND will evaluate the RaiseError only if the condition
// evaluates to TRUE.
//////////////////////////////////////////////////////////////////////////////
void BeforeTrigger::bindSignalClause(BindWA *bindWA, RETDesc *origRETDesc, CollHeap *heap)
{
  if (bindWA->inDDL())
  {
    // In DDL time (CREATE TRIGGER) all we need is to bind the signal
    // expression for semantic checks.
    signal_->bindNode(bindWA);
    if (bindWA->errStatus())
    return;
  }
  else
  {
    // The SIGNAL expression is piggy-backed on the Unique ExecuteID
    // value inserted into the temp table.
    ColumnNameMap *execIdCol =
      origRETDesc->findColumn(InliningInfo::getExecIdVirtualColName());
    CMPASSERT(execIdCol != NULL);
    const ColRefName& ExecIdColName = execIdCol->getColRefNameObj();
    ItemExpr *passThruExpr = execIdCol->getValueId().getItemExpr();
    ItemExpr *whenAndSignal = NULL;

    // Case 10-040604-5021:
    // General AND logic uses "short circuiting" as follows: if the
    // left side is FALSE, evaluation of the right side is skipped, and
    // the result returned is FALSE. The following expression depends on
    // evaluation of the right side being skipped whenever the left side
    // is NOT TRUE, (i.e., FALSE or NULL). Therefore, an IS TRUE unary
    // predicate must be placed above the actual WHEN condition. Otherwise,
    // the signal will fire when the WHEN condition evaluates to NULL.
    if (whenClause_ != NULL)
    {
         if (whenClause_->getOperatorType() == ITM_AND ||
             whenClause_->getOperatorType() == ITM_OR)
         {
             ItemExpr *isTrueExpr = new (heap) UnLogic(ITM_IS_TRUE, whenClause_);
             whenAndSignal = new(heap) BiLogic(ITM_AND, isTrueExpr, signal_);
         }
         else
         {
             whenAndSignal = new(heap) BiLogic(ITM_AND, whenClause_, signal_);
         }
    }

    else
      // After we add the support for reading the trigger status from
      // the resource fork, and adding it to the condition, we should
      // never get here.
      whenAndSignal = signal_;

    // For type compatibity, the original value is used whatever the
    // WHEN clause evaluates to. However, if it evaluates to TRUE, the
    // evaluation of the signal expression will throw an SQLERROR.
    ItemExpr *condSignalExpr = new(heap)
      Case(NULL, new(heap)
        IfThenElse(whenAndSignal, passThruExpr, passThruExpr));
    condSignalExpr = condSignalExpr->bindNode(bindWA);
    if (bindWA->errStatus())
      return;

    // Now delete the original "virtual column" from the RETDesc, and
    // re-insert it with the new value.
    origRETDesc->delColumn(bindWA, ExecIdColName, USER_COLUMN);
    origRETDesc->addColumn(bindWA, ExecIdColName, condSignalExpr->getValueId());
  }
}

//////////////////////////////////////////////////////////////////////////////
// This bind is bottom-up, so we first bind the children, and then use
// and change the RETDesc they created.
//////////////////////////////////////////////////////////////////////////////
RelExpr *BeforeTrigger::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  bindChildren(bindWA);
  if (bindWA->errStatus())
    return this;

  // Now we know that we have the columns of OLD@ and NEW@ in the RETDesc
  // of the current scope. Save this scope so we can update it and restore
  // it when we are done.
  RETDesc   *origRETDesc = bindWA->getCurrentScope()->getRETDesc();
  CollHeap  *heap = bindWA->wHeap();
  CollIndex  refsToFind = getRefList().entries();

  // For each reference to change, Find the table name in the RETDesc,
  // and save a pointer to it's column list in the TableRefName object.
  CollIndex i=0;
  for (i=0; i<refsToFind; i++)
    getRefList().at(i).lookupTableName(origRETDesc);

  // Create an empty RETDesc for the current scope.
  // It will contain the names the user specified (MY_NEW, MY_OLD) for the
  // OLD@ and NEW@ transition variables, and will be used to bind this
  // node only.
  bindWA->getCurrentScope()->setRETDesc(new(heap) RETDesc(bindWA));

  // For each table reference, add to the RETDesc of the current scope,
  // the columns of the referenced tables with the new referencing names
  // as correlation names.
  for (i=0; i<refsToFind; i++)
    getRefList().at(i).bindRefColumns(bindWA);

  // First bind the condition. The ValueId will be used later (possibly
  // multiple times) so that during execution, the expression will be
  // evaluated only once.
  if (whenClause_ != NULL)
  {
    whenClause_ = whenClause_->bindNode(bindWA);
    if (bindWA->errStatus())
      return this;
  }

  // Use the bound condition to prepare the conditional expression
  // for each column modified by the trigger (SET MY_NEW.a = ...)
  if (setList_ != NULL)
    bindSetClause(bindWA, origRETDesc, heap);

  // Use the bound condition to prepare the conditional SIGNAL
  // expression, on the ExecuteId "virtual column".
  if (signal_ != NULL)
    bindSignalClause(bindWA, origRETDesc, heap);

  if (bindWA->errStatus())
    return this;

  // We don't need the RETDesc of the current scope anymore. Restore the
  // original RETDesc with the updated columns.
  bindWA->getCurrentScope()->setRETDesc(origRETDesc);
  if (parentTSJ_ != NULL)
  {
    // If this is the top most before trigger, save a copy of the RETDesc
    // for use by the transformNode() pass.
    RETDesc *savedRETDesc = new(heap) RETDesc(bindWA, *origRETDesc);
    setRETDesc(savedRETDesc);
  }
  //
  // Bind the base class.
  //
  RelExpr *boundNode = bindSelf(bindWA);

  return boundNode;
} // BeforeTrigger::bindNode()

// -----------------------------------------------------------------------
// member functions for class Insert
// -----------------------------------------------------------------------
static void bindInsertRRKey(BindWA *bindWA, Insert *insert,
                            ValueIdList &sysColList, CollIndex i)
{
  // For a KS round-robin partitioned table, the system column
  // (for now there is only one, SYSKEY) is initialized via the expression
  // "ProgDistribKey(partNum, rowPos, totalNumParts)".
  //
  const NAFileSet *fs =
    insert->getTableDesc()->getClusteringIndex()->getNAFileSet();

  // For now, round-robin partitioned tables are always stored in
  // key-sequenced files, and there is only one system column (SYSKEY)
  // which is at the beginning of the record.
  CMPASSERT(fs->isKeySequenced() && i==0);

  CollHeap *heap = bindWA->wHeap();

  // Host variables that provide access to partition number,
  // row position, and total number of partitions --
  // supplied at run-time by the executor insert node.
  //
  ItemExpr *partNum = new (heap)
    HostVar("_sys_hostVarInsertPartNum",
      new (heap) SQLInt(heap, FALSE,FALSE),   // int unsigned not null
      TRUE // is system-generated
     );
  partNum->synthTypeAndValueId();
  insert->partNumInput() = partNum->getValueId();  // for later use in codeGen

  ItemExpr *rowPos = new (heap)
    HostVar("_sys_hostVarInsertRowPos",
      new (heap) SQLInt(heap, FALSE,FALSE),  // int unsigned not null
      TRUE // is system-generated
     );
  rowPos->synthTypeAndValueId();
  insert->rowPosInput() = rowPos->getValueId();  // for later use in codeGen

  ItemExpr *totNumParts = new (heap)
    HostVar("_sys_hostVarInsertTotNumParts",
      new (heap) SQLInt(heap, FALSE,FALSE),  // int unsigned not null
      TRUE // is system-generated
     );
  totNumParts->synthTypeAndValueId();
  insert->totalNumPartsInput() = totNumParts->getValueId();   // for later use

  // Generate expression to compute a round-robin key.  Parameters to
  // ProgDistribKey are the partition number, the row position (which
  // is chosen randomly; the insert node will retry if a number is
  // selected that is already in use), and the total number of
  // partitions.
  ItemExpr *rrKey = new (heap) ProgDistribKey(partNum, rowPos, totNumParts);

  // Build and set round-robin key expression.
  Assign *assign = new (heap)
    Assign(sysColList[i].getItemExpr(), rrKey, FALSE /*not user-specified*/);
  assign->bindNode(bindWA);
  insert->rrKeyExpr() = assign->getValueId();
} // bindInsertRRKey

RelExpr *Insert::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // Set local binding flags
  setInUpdateOrInsert(bindWA, this, REL_INSERT);
  // The 8108 (unique constraint on an ID column) error must be raised
  // only for system generated IDENTITY values and not for
  // user generated ID values. We use the GenericUpdate::identityColumnUniqueIndex_
  // to indicate to the EID that 8108 should be raised in place of 8102.
  // This variable is used to indicate that there is an IDENTITY column
  // in the table for which the system is generating the value

  // This is NULL if "DEFAULT VALUES" was specified,
  // non-NULL if a query-expr child was specified:  VALUES.., TABLE.., SELECT..
  RelExpr *someNonDefaultValuesSpecified = child(0);

  // Set flag for firstN in context
  if (child(0) && child(0)->getOperatorType() == REL_ROOT) // Indicating subquery
    if (child(0)->castToRelExpr() && 
        child(0)->castToRelExpr()->getFirstNRows() >= 0)
      if (bindWA && 
          bindWA->getCurrentScope() && 
          bindWA->getCurrentScope()->context())
        bindWA->getCurrentScope()->context()->firstN() = TRUE;

  if (NOT someNonDefaultValuesSpecified) {  // "DEFAULT VALUES" specified
    // Kludge up a dummy child before binding the GenericUpdate tree
    setChild(0, new(bindWA->wHeap()) Tuple(new(bindWA->wHeap()) SystemLiteral(0)));
  }

  // Bind the GenericUpdate tree.
  //
  RETDesc *incomingRETDescForSource = bindWA->getCurrentScope()->getRETDesc();
  RelExpr *boundExpr = GenericUpdate::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return boundExpr;

  const NAFileSet* fileset = getTableDesc()->getNATable()->getClusteringIndex();
  const NAColumnArray& partKeyCols = fileset->getPartitioningKeyColumns();

  if (getTableDesc()->getNATable()->isHiveTable())
  {
    if (partKeyCols.entries() > 0)
      {
        // Insert into partitioned tables would require computing the target
        // partition directory name, something we don't support yet.
        *CmpCommon::diags() << DgSqlCode(-4222)
                            << DgString0("Insert into partitioned Hive tables");
        bindWA->setErrStatus();
        return this;
      }

    // specifying a list of column names to insert to is not yet supported
    if (insertColTree_) {
      *CmpCommon::diags() << DgSqlCode(-4223)
                          << DgString0("Target column list for insert into Hive table");
      bindWA->setErrStatus();
      return this;
    }
     
    RelExpr *feResult = FastExtract::makeFastExtractTree(
         getTableDesc(),
         child(0).getPtr(),
         getOverwriteHiveTable(),
         TRUE,  // called from within binder
         FALSE, // not a common subexpr
         bindWA);

    if (feResult)
      {
        feResult = feResult->bindNode(bindWA);
        if (bindWA->errStatus())
          return NULL;

        return feResult;
      }
    else
      return this;
  }

  if(!(getOperatorType() == REL_UNARY_INSERT &&
       (child(0)->getOperatorType() == REL_TUPLE || // VALUES (1,'b')
        child(0)->getOperatorType() == REL_TUPLE_LIST || // VALUES (1,'b'),(2,'Y')
        child(0)->getOperatorType() == REL_UNION))  &&      // VALUES with subquery
     (getOperatorType() != REL_LEAF_INSERT))
    {
      setInsertSelectQuery(TRUE);
    }



  // Prepare for any IDENTITY column checking later on
  NAString identityColumnName;
  NABoolean identityColumnGeneratedAlways = FALSE;
  
  identityColumnGeneratedAlways =
    getTableDesc()->isIdentityColumnGeneratedAlways(&identityColumnName);
    
  if ((getTableName().isVolatile()) &&
      (CmpCommon::context()->sqlSession()->volatileSchemaInUse()) &&
      (getTableName().getSpecialType() == ExtendedQualName::NORMAL_TABLE) &&
      ((ActiveSchemaDB()->getDefaults()).getAsLong(IMPLICIT_UPD_STATS_THRESHOLD) > -1) &&
      (bindWA->isInsertSelectStatement()) &&
      (NOT getTableDesc()->getNATable()->isVolatileTableMaterialized()))
    {
      if (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
        //      if (NOT Get_SqlParser_Flags(NO_IMPLICIT_VOLATILE_TABLE_UPD_STATS))
        {
          // treat this insert as a volatile load stmt.
          RelExpr * loadVolTab = 
            new (bindWA->wHeap()) 
            ExeUtilLoadVolatileTable(getTableName(),
                                     this,
                                     bindWA->wHeap());
          
          boundExpr = loadVolTab->bindNode(bindWA);
          if (bindWA->errStatus()) 
            return boundExpr;

          return boundExpr;
        }
      else
        {
          NATable * nat = (NATable*)(getTableDesc()->getNATable());
          nat->setIsVolatileTableMaterialized(TRUE);
        }
    }

  // Now we have to create the following three collections:
  //
  // - newRecExpr()
  //   An unordered set of Assign nodes of the form
  //   "col1 = value1, col2 = value2, ..." which is used by Norm/Optimizer.
  //
  // - newRecExprArray()
  //   An ordered array of Assign nodes of the same form,
  //   ordered by column position, which is used by Generator.
  //   This array must have the following properties:
  //
  //   - All columns not specified in the insert statement must be
  //     Assign'ed with their default values.
  //
  //   - If this is a key-sequenced table with a (non-RR) SYSKEY column,
  //     we must create the first entry in the newRecExprArray
  //     to be "SYSKEY_COL = 0".  This is a placeholder where the timestamp
  //     value will be moved at runtime.  Round-robin SYSKEY columns are
  //     initialized via an expression of the form "SYSKEY_COL =
  //     ProgDistribKey(..params..)".  SYSKEY columns for other table
  //     organizations are handled by the file system or disk process.
  //
  // - updateToSelectMap()
  //   A ValueIdMap that can be used to rewrite value ids of the
  //   target table in terms of the source table and vice versa.
  //   The top value ids are target value ids, the bottom value ids
  //   are those of the source.
  //

  NABoolean view = bindWA->getNATableInternal(getTableName())->getViewText() != NULL;

  ValueIdList tgtColList, userColList, sysColList, *userColListPtr;
  CollIndexList colnoList(STMTHEAP);
  CollIndex totalColCount, defaultColCount, i;

  getTableDesc()->getSystemColumnList(sysColList);

  //
  // Detach the column list and bind the columns to the target table.
  // Set up "colnoList" to map explicitly specified columns to where
  // in the ordered array we will be inserting later.
  //
  ItemExpr *columnTree = removeInsertColTree();
  CMPASSERT(NOT columnTree || someNonDefaultValuesSpecified);
  if (columnTree || (view && someNonDefaultValuesSpecified)) {
    //
    // INSERT INTO t(colx,coly,...) query-expr;
    // INSERT INTO v(cola,colb,...) query-expr;
    // INSERT INTO v query-expr;
    //  where  query-expr is VALUES..., TABLE..., or SELECT...,
    //  but not DEFAULT VALUES.
    //    userColList is the full list of columns in the target table
    //    colnoList contains, for those columns specified in tgtColList,
    //		their ordinal position in the target table user column list
    //		(i.e., not counting system columns, which can't be specified
    //		in the insert column list); e.g. '(Z,X,Y)' -> [3,1,2]
    //
    CMPASSERT(NOT columnTree ||
              columnTree->getOperatorType() == ITM_REFERENCE ||
              columnTree->getOperatorType() == ITM_ITEM_LIST);
    getTableDesc()->getUserColumnList(userColList);
    userColListPtr = &userColList;

    RETDesc *columnLkp;

    if (columnTree) {
      // bindRowValues will bind using the currently scoped RETDesc left in
      // by GenericUpdate::bindNode, which will be that of the naTableTop
      // (topmost view or table), *not* that of the base table (getTableDesc()).
      columnLkp = bindRowValues(bindWA, columnTree, tgtColList, this, FALSE);
      if (bindWA->errStatus()) return boundExpr;
    } 
    else 
    {
      columnLkp = bindWA->getCurrentScope()->getRETDesc();
      columnLkp->getColumnList()->getValueIdList(tgtColList);
    }

    if (GU_DEBUG) {
      cerr << "columnLkp " << flush;
      columnLkp->display();
    }

    for (i = 0; i < columnLkp->getDegree(); i++) {
      // Describes column in the base table:
      ValueId source = columnLkp->getValueId(i);
      const NAColumn *nacol = source.getNAColumn();

      // Gets name of the column in this (possibly view) table:
      const ColRefName colName = columnLkp->getColRefNameObj(i);
 
      // solution 10-081114-7315
      if (bindWA->inDDL() && bindWA->isInTrigger ())
      {
         if (!userColListPtr->contains(source))
         {
            // 4001 column not found
            *CmpCommon::diags() << DgSqlCode(-4001)
              << DgColumnName(colName.getColName())
              << DgString0(getTableName().getQualifiedNameObj().getQualifiedNameAsAnsiString())
              << DgString1(bindWA->getDefaultSchema().getSchemaNameAsAnsiString());
            bindWA->setErrStatus();
            delete columnLkp;
            return boundExpr;
         }
      }

      if (columnLkp->findColumn(colName)->isDuplicate()) {
        // 4022 column specified more than once
        *CmpCommon::diags() << DgSqlCode(-4022)
                            << DgColumnName(colName.getColName());
        bindWA->setErrStatus();
        delete columnLkp;
        return boundExpr;
      }

      colnoList.insert(nacol->getPosition());
      // Commented out this assert, as Assign::bindNode below emits nicer errmsg
      // CMPASSERT((long)nacol->getPosition() - (long)firstColNumOnDisk >= 0);
    }
    if (columnTree) {
      delete columnLkp;
      columnLkp = NULL;
    }
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
  }
  else {
    //
    // INSERT INTO t query-expr;
    // INSERT INTO t DEFAULT VALUES;
    // INSERT INTO v DEFAULT VALUES;
    //    userColListPtr points to tgtColList (which is the full list)
    //    userColList not used (because tgtColList already is the full list)
    //    colnoList remains empty (because tgtColList is already in order)
    //              if no system columns, set to list of user cols otherwise
    getTableDesc()->getUserColumnList(tgtColList);
    userColListPtr = &tgtColList;
    if (sysColList.entries()) {
      // set up colnoList to indicate the user columns, to help
      // binding DEFAULT clauses in DefaultSpecification::bindNode()
      for (CollIndex uc=0; uc<tgtColList.entries(); uc++) {
        colnoList.insert(tgtColList[uc].getNAColumn()->getPosition());
      }
    }
  }

  // Compute total number of columns. Note that there may be some unused
  // entries in newRecExprArray(), in the following case:
  // - For computed columns that are not stored on disk
  totalColCount = userColListPtr->entries() + sysColList.entries();
  newRecExprArray().resize(totalColCount);

  // Make sure children are bound -- GenericUpdate::bindNode defers
  // their binding to now if this is an INSERT..VALUES(..),
  // because only now do we have target column position info for
  // correct binding of INSERT..VALUES(..,DEFAULT,..)
  // in DefaultSpecification::bindNode.
  //
  // Save current RETDesc and XTNM.
  // Bind the source in terms of the original RETDesc,
  //   with target column position info available through
  //     bindWA->getCurrentScope()->context()->updateOrInsertNode()
  //   (see DefaultSpecification::bindNode, calls Insert::getColDefaultValue).
  // Restore RETDesc and XTNM.
  //
  RETDesc *currRETDesc = bindWA->getCurrentScope()->getRETDesc();
  bindWA->getCurrentScope()->setRETDesc(incomingRETDescForSource);
  bindWA->getCurrentScope()->xtnmStack()->createXTNM();
  setTargetUserColPosList(colnoList);

  // if my child is a TupleList, then all tuples are to be converted/cast
  // to the corresponding target type of the tgtColList.
  // Pass on the tgtColList to TupleList so it can generate the Cast nodes
  // with the target types during the TupleList::bindNode.
  TupleList *tl = NULL;
  if (child(0)->getOperatorType() == REL_TUPLE_LIST) {
    tl = (TupleList *)child(0)->castToRelExpr();
    tl->castToList() = tgtColList;
  }

  if (getTolerateNonFatalError() != RelExpr::UNSPECIFIED_) {
    HostArraysWA * arrayWA = bindWA->getHostArraysArea() ;
    if (arrayWA && arrayWA->hasHostArraysInTuple()) {
      if (getTolerateNonFatalError() == RelExpr::NOT_ATOMIC_)
        arrayWA->setTolerateNonFatalError(TRUE);
      else
        arrayWA->setTolerateNonFatalError(FALSE);  // Insert::tolerateNonfatalError == ATOMIC_
    }
    else if (NOT arrayWA->getRowwiseRowset()) {
      // NOT ATOMIC only for rowset inserts
      *CmpCommon::diags() << DgSqlCode(-30025) ;
      bindWA->setErrStatus();
      return boundExpr;
    }
  }

 


   
  bindChildren(bindWA);
  if (bindWA->errStatus()) return this;

  // if this is an insert into native hbase table in _ROW_ format, then
  // validate that only REL_TUPLE or REL_TUPLE_LIST is being used.
  if ((getOperatorType() == REL_UNARY_INSERT) &&
      (getTableDesc()->getNATable()->isHbaseRowTable()))
    {
      NABoolean isError = FALSE;
      if (NOT (child(0)->getOperatorType() == REL_TUPLE || // VALUES (1,'b')
               child(0)->getOperatorType() == REL_TUPLE_LIST)) // VALUES (1,'b'),(2,'Y')
        {
          isError = TRUE;
        }

      // Also make sure that inserts into column_details field of _ROW_ format
      // hbase virtual table are being done through column_create function.
      //   For ex: insert into hbase."_ROW_".hb values ('1', column_create('cf:a', '100'))
      //
      if ((NOT isError) && (child(0)->getOperatorType() == REL_TUPLE))
	{
          ValueIdList &tup = ((Tuple*)(child(0)->castToRelExpr()))->tupleExpr();
          if (tup.entries() == 2) // can only have 2 entries
            {
              ItemExpr * ie = tup[1].getItemExpr();
              if (ie && ie->getOperatorType() != ITM_HBASE_COLUMN_CREATE)
                {
                  isError = TRUE;
                }
            }
          else
            isError = TRUE;
        }

      if ((NOT isError) && (child(0)->getOperatorType() == REL_TUPLE_LIST))
	{
	  TupleList * tl = (TupleList*)(child(0)->castToRelExpr());
	  for (CollIndex x = 0; x < (UInt32)tl->numTuples(); x++)
	    {
	      ValueIdList tup;
	      if (!tl->getTuple(bindWA, tup, x)) 
		{
                  isError = TRUE;
		}

              if (NOT isError)
                {
                  if (tup.entries() == 2) // must have 2 entries
                    {
                      ItemExpr * ie = tup[1].getItemExpr();
                      if (ie->getOperatorType() != ITM_HBASE_COLUMN_CREATE)
                        {
                          isError = TRUE;
                        }
                    }
                  else
                    isError = TRUE;
                } // if
            } // for
        } // if

      if (isError)
        {
          *CmpCommon::diags() << DgSqlCode(-1429);
          bindWA->setErrStatus();
          
          return boundExpr; 
        }
    }

  // the only time that tgtColList.entries()(Insert's colList) != tl->castToList().entries()
  // (TupleList's colList) is when DEFAULTS are removed in TupleList::bindNode() for insert
  // into table with IDENTITY column, where the system generates the values
  // for it using SG (Sequence Generator).
  // See TupleList::bindNode() for detailed comments.
  // When tgtColList.entries()(Insert's col list) is not
  // equal to  tl->castToList().entries() (TupleList's column list)
  // make sure the correct colList is used during binding.
  ValueIdList newTgtColList;
  if(tl && (tgtColList.entries() != tl->castToList().entries()))
    {
    newTgtColList = tl->castToList();
    CMPASSERT(newTgtColList.entries() == (tgtColList.entries() -1));
    }
  else
    newTgtColList = tgtColList;

  setTargetUserColPosList();
  bindWA->getCurrentScope()->xtnmStack()->removeXTNM();
  bindWA->getCurrentScope()->setRETDesc(currRETDesc);
  NABoolean bulkLoadIndex = bindWA->isTrafLoadPrep() && noIMneeded() ;

  if (someNonDefaultValuesSpecified) 
    // query-expr child specified
    {

      const RETDesc &sourceTable = *child(0)->getRETDesc();
      if ((sourceTable.getDegree() != newTgtColList.entries())&& !bulkLoadIndex) {
      // 4023 degree of row value constructor must equal that of target table
      *CmpCommon::diags() << DgSqlCode(-4023)
        << DgInt0(sourceTable.getDegree()) << DgInt1(tgtColList.entries());
      bindWA->setErrStatus();
      return boundExpr;
      }

    
    OptSqlTableOpenInfo* stoiInList = NULL;
    for (CollIndex ii=0; ii < bindWA->getStoiList().entries(); ii++)
    {
      if (getOptStoi() && getOptStoi()->getStoi())
	{
	  if (strcmp((bindWA->getStoiList())[ii]->getStoi()->fileName(), 
		     getOptStoi()->getStoi()->fileName()) == 0) 
	    {
	      stoiInList = bindWA->getStoiList()[ii];
	      break;
	    }
	}
    }

    // Combine the ValueIdLists for the column list and value list into a
    // ValueIdSet (unordered) of Assign nodes and a ValueIdArray (ordered).
    // Maintain a ValueIdMap between the source and target value ids.
    CollIndex i2 = 0;
    const ColumnDescList *viewColumns = NULL;
    if (getBoundView())
      viewColumns = getBoundView()->getRETDesc()->getColumnList();

     if (bulkLoadIndex) {
       setRETDesc(child(0)->getRETDesc());
      } 



    for (i = 0; i < tgtColList.entries() && i2 < newTgtColList.entries(); i++) {
      if(tgtColList[i] != newTgtColList[i2])
        continue;

      ValueId target = tgtColList[i];
      ValueId source ;
      if (!bulkLoadIndex)
        source = sourceTable.getValueId(i2);
      else {
        ColRefName & cname = ((ColReference *)(baseColRefs()[i2]))->getColRefNameObj();
        source = sourceTable.findColumn(cname)->getValueId();
      }
      CMPASSERT(target != source);

      const NAColumn *nacol = target.getNAColumn();

      const NAType &sourceType = source.getType();
      const NAType &targetType = target.getType();

      if ( DFS2REC::isFloat(sourceType.getFSDatatype())  &&
           DFS2REC::isNumeric(targetType.getFSDatatype()) &&
           (getTableDesc()->getNATable()->getPartitioningScheme()  ==
                                                COM_HASH_V1_PARTITIONING ||
            getTableDesc()->getNATable()->getPartitioningScheme()  ==
                                                COM_HASH_V2_PARTITIONING) )
      {
        const NAColumnArray &partKeyCols = getTableDesc()->getNATable()
                         ->getClusteringIndex()->getPartitioningKeyColumns();

        for (CollIndex j=0; j < partKeyCols.entries(); j++)
        {
          if (partKeyCols[j]->getPosition() == nacol->getPosition())
          {
            ItemExpr *ie = source.getItemExpr();
            ItemExpr *cast = new (bindWA->wHeap())
                                Cast(ie, &targetType, ITM_CAST);
            cast = cast->bindNode(bindWA);
            if (bindWA->errStatus()) 
              return NULL;

            source = cast->getValueId();

          }
        }
      }

      Assign *assign = new (bindWA->wHeap())
        Assign(target.getItemExpr(), source.getItemExpr());
      assign->bindNode(bindWA);

      if(bindWA->errStatus())
	return NULL;



      if (stoiInList && !getUpdateCKorUniqueIndexKey())
      {
        if(!getBoundView())
          stoiInList->addInsertColumn(nacol->getPosition());
        else
        {
          NABoolean found = FALSE;
          for (CollIndex k=0; k < viewColumns->entries(); k++) {
            if ((*viewColumns)[k]->getValueId() == target) {
              stoiInList->addInsertColumn((Lng32) k);
              found = TRUE;
              // Updatable views cannot have any underlying basetable column
              // appear more than once, so it's safe to break out of the loop.
              break;
            }
          }  // loop k
          CMPASSERT(found);
        }
      }

      //
      // Check for automatically inserted TRANSLATE nodes.
      // Such nodes are inserted by the Implicit Casting And Translation feature.
      // If this node has a child TRANSLATE node, then that TRANSLATE node
      // is the real "source" that we must use from here on.
      //
      ItemExpr *assign_child = assign->child(1);
      if ( assign_child->getOperatorType() == ITM_CAST )
      {
         const NAType& type = assign_child->getValueId().getType();
         if ( type.getTypeQualifier() == NA_CHARACTER_TYPE )
         {
            ItemExpr *assign_grndchld = assign_child->child(0);
            if ( assign_grndchld->getOperatorType() == ITM_TRANSLATE )
            {
               source = assign_grndchld->getValueId();
               CMPASSERT(target != source);
            }
         }
      }

      const NAType *colType = nacol->getType();
      if (!colType->isSupportedType()) {
        *CmpCommon::diags() << DgSqlCode(-4027)     // 4027 table not insertable
        << DgTableName(nacol->getNATable()->getTableName().getQualifiedNameAsAnsiString());
        bindWA->setErrStatus();
      }

      if (bindWA->errStatus()) return boundExpr;
      newRecExprArray().insertAt(nacol->getPosition(), assign->getValueId());
      newRecExpr().insert(assign->getValueId());
    
      const NAType& assignSrcType = assign->getSource().getType();
      // if ( <we added some type of conversion> AND
      //      ( <tgt and src are both character> AND
      //        (<they are big and errors can occur> OR <charsets differ> OR <difference between tgt and src lengths is large>)))
      //      OR
      //      ( <we changed the basic type and we allow incompatible types> )
      //    )
      //   <then incorporate this added conversion into the updateToSelectMap>
      if ( source != assign->getSource() && 
           ((assignSrcType.getTypeQualifier() == NA_CHARACTER_TYPE &&
             sourceType.getTypeQualifier() == NA_CHARACTER_TYPE &&
             ((assign->getSource().getItemExpr()->getOperatorType() == ITM_CAST &&
               sourceType.errorsCanOccur(assignSrcType) && 
               sourceType.getNominalSize() > 
               CmpCommon::getDefaultNumeric(LOCAL_MESSAGE_BUFFER_SIZE)*1024) ||
              // Temporary code to fix QC4395 in M6. For M7, try to set source
              // to the right child of the assign after calling assign->bindNode.
              // We should then be able to eliminate this entire if statement
              // as well as the code to check for TRANSLATE nodes above.
              ((CharType &) assignSrcType).getCharSet() !=
              ((CharType &) sourceType).getCharSet() || 
              // The optimizer may ask for source data to be partitioned or sorted on original source columns
              // This is the reason we need to choose the else branch below unless we have a particular reason
              // to do otherwise. Each of the conditions in this if statement reflects one of those partcular
              // conditions. The bottomValues of updateToSelectMap will be placed in their entirety in the 
              // characteristic outputs of the source node. Outputs of the source node may be used to allocate
              // buffers at runtime and therefore we would like to keep the output as small as possible.
              // If the source cannot be partioned/sorted on a column because we have assign-getSource in the bottomValues
              // then the cost is that data will be repartitioned with an additional exchange node. If the difference in
              // length between source and assignSrc is large then the cost of repartition is less than the cost of 
              // allocating and using large buffers.
              sourceType.getNominalSize() > (assignSrcType.getNominalSize() + 
                                             (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_98))  // default value is 512
              ))
            ||
            // If we allow incompatible type assignments, also include the
            // added cast into the updateToSelectMap
            assignSrcType.getTypeQualifier() !=  sourceType.getTypeQualifier() &&
            CmpCommon::getDefault(ALLOW_INCOMPATIBLE_OPERATIONS) == DF_ON))
        {
          updateToSelectMap().addMapEntry(target,assign->getSource());
        }
      else
        {
          updateToSelectMap().addMapEntry(target,source);
        }

      i2++;
    }
  }
  setBoundView(NULL);

  // Is the table round-robin (horizontal) partitioned?
  PartitioningFunction *partFunc =
    getTableDesc()->getClusteringIndex()->getNAFileSet()->
    getPartitioningFunction();

  NABoolean isRRTable =
    partFunc && partFunc->isARoundRobinPartitioningFunction();

  // Fill in default values for any columns not explicitly specified.
  //
  if (someNonDefaultValuesSpecified)      // query-expr child specified, set system cols
    defaultColCount = totalColCount - newTgtColList.entries();
  else                  // "DEFAULT VALUES" specified
    defaultColCount = totalColCount;

  if (identityColumnGeneratedAlways)
    defaultColCount = totalColCount;

  NABoolean isAlignedRowFormat = getTableDesc()->getNATable()->isSQLMXAlignedTable();
  NABoolean omittedDefaultCols = FALSE;
  NABoolean omittedCurrentDefaultClassCols = FALSE;

  if (defaultColCount) {
    NAWchar zero_w_Str[2]; zero_w_Str[0] = L'0'; zero_w_Str[1] = L'\0';  // wide version
    CollIndex sysColIx = 0, usrColIx = 0;

    for (i = 0; i < totalColCount; i++) {

      ValueId target;
      NABoolean isASystemColumn = FALSE;
      const NAColumn *nacol = NULL;

      // find column on position i in the system or user column lists
      if (sysColIx < sysColList.entries() &&
          sysColList[sysColIx].getNAColumn()->getPosition() == i)
        {
          isASystemColumn = TRUE;
          target = sysColList[sysColIx];
        }
      else
        {
          CMPASSERT((*userColListPtr)[usrColIx].getNAColumn()->getPosition() == i);
          target = (*userColListPtr)[usrColIx];
        }
      nacol = target.getNAColumn();
      
      // if we need to add the default value, we don't have a new rec expr yet
      if (NOT newRecExprArray().used(i)) {
       
        const char* defaultValueStr = NULL;
        ItemExpr *  defaultValueExpr = NULL;
        NABoolean needToDeallocateColDefaultValueStr = FALSE;

        // Used for datetime columns with COM_CURRENT_DEFAULT.
        //
        NAType *castType = NULL;

        if (isASystemColumn) {
          if (isRRTable) {
            bindInsertRRKey(bindWA, this, sysColList, sysColIx);
            if (bindWA->errStatus()) return boundExpr;
          }

          if (nacol->isComputedColumn())
            {
              CMPASSERT(target.getItemExpr()->getOperatorType() == ITM_BASECOLUMN);
              ValueId defaultExprValId = ((BaseColumn *) target.getItemExpr())->
                getComputedColumnExpr();
              ValueIdMap updateToSelectMapCopy(updateToSelectMap());

              // Use a copy to rewrite the value, to avoid requesting additional
              // values from the child. We ask the child for all entries in this
              // map in GenericUpdate::pushdownCoveredExpr().
              updateToSelectMapCopy.rewriteValueIdDown(defaultExprValId, defaultExprValId);
              defaultValueExpr = defaultExprValId.getItemExpr();
            }
          else
            defaultValueStr = (char *)zero_w_Str;
        }
        else {            // a user column (cf. Insert::getColDefaultValue)
          CMPASSERT(NOT nacol->isComputedColumn()); // computed user cols not yet supported
          defaultValueStr = nacol->getDefaultValue();
        }
          
        if (NOT defaultValueStr && NOT defaultValueExpr) {
          // 4024 column has neither a default nor an explicit value.
          *CmpCommon::diags() << DgSqlCode(-4024) << DgColumnName(nacol->getColName());
          bindWA->setErrStatus();
          return boundExpr;
        }

        if (defaultValueStr) {
          // If the column has a default class of COM_CURRENT_DEFAULT,
          // cast the default value (which is CURRENT_TIMESTAMP) to
          // the type of the column.  Here we capture the type of the
          // column. COM_CURRENT_DEFAULT is only used for Datetime
          // columns.
          //
          if (nacol->getDefaultClass() == COM_CURRENT_DEFAULT || nacol->getDefaultClass() == COM_CURRENT_UT_DEFAULT 
             || nacol->getDefaultClass() == COM_UUID_DEFAULT) {
            castType = nacol->getType()->newCopy(bindWA->wHeap());
            omittedCurrentDefaultClassCols = TRUE;
            omittedDefaultCols = TRUE;
          }
          else if ((nacol->getDefaultClass() == COM_IDENTITY_GENERATED_ALWAYS) ||
                   (nacol->getDefaultClass() == COM_IDENTITY_GENERATED_BY_DEFAULT)) {
            setSystemGeneratesIdentityValue(TRUE);
          }
          else if (nacol->getDefaultClass() != COM_NO_DEFAULT)
            omittedDefaultCols = TRUE;

          // Bind the default value, make an Assign, etc, as above
          Parser parser(bindWA->currentCmpContext());

          // save the current parserflags setting
          ULng32 savedParserFlags = Get_SqlParser_Flags (0xFFFFFFFF);
          Set_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL);
          Set_SqlParser_Flags(ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME);
          defaultValueExpr = parser.getItemExprTree(defaultValueStr);
          CMPASSERT(defaultValueExpr);

          // Restore parser flags settings to what they originally were
          Assign_SqlParser_Flags (savedParserFlags);
	} // defaultValueStr != NULL

	Assign *assign = NULL;
	
	// If the default value string was successfully parsed,
	// Create an ASSIGN node and bind.
        //
        if (defaultValueExpr) {

          // If there is a non-NULL castType, then cast the default
          // value to the castType.  This is used in the case of
          // datetime value with COM_CURRENT_DEFAULT.  The default
          // value will be CURRENT_TIMESTAMP for all datetime types,
          // so must cast the CURRENT_TIMESTAMP to the type of the
          // column.
          //
          if(castType) {
            defaultValueExpr = new (bindWA->wHeap())
              Cast(defaultValueExpr, castType);
          }

          // system generates value for IDENTITY column.
          if (defaultValueExpr->getOperatorType() == ITM_IDENTITY &&
              (CmpCommon::getDefault(COMP_BOOL_210) == DF_ON))
            {
              // SequenceGenerator::createSequenceSubqueryExpression()
              // is called for introducing the subquery in
              // defaultValueExpr::bindNode() (IdentityVar::bindNode()).
              // We bind here to make sure the correct subquery
              // is used.
              defaultValueExpr = defaultValueExpr->bindNode(bindWA);
            }

	  if (((isUpsertLoad()) ||
               ((isUpsert()) && (getTableDesc()->getNATable()-> isSQLMXAlignedTable()))) &&
              (NOT defaultValueExpr->getOperatorType() == ITM_IDENTITY) &&
	      (NOT isASystemColumn))
	    {
	      // for 'upsert using load' construct, all values must be specified so
	      // data could be loaded using inserts.
	      // If some values are missing, then it becomes an update.
	      *CmpCommon::diags() << DgSqlCode(-4246) ;
	      bindWA->setErrStatus();

	      return boundExpr;
	    }
          assign = new (bindWA->wHeap())
            Assign(target.getItemExpr(), defaultValueExpr,
                    FALSE /*Not user Specified */);
          if ((nacol->getDefaultClass() != COM_CURRENT_DEFAULT) &&
              (nacol->getDefaultClass() != COM_CURRENT_UT_DEFAULT) &&
              (nacol->getDefaultClass() != COM_FUNCTION_DEFINED_DEFAULT) &&
              (nacol->getDefaultClass() != COM_UUID_DEFAULT) &&
              (nacol->getDefaultClass() != COM_USER_FUNCTION_DEFAULT))
             assign->setToBeSkipped(TRUE);
          assign->bindNode(bindWA);
        }

        //
        // Note: Parser or Binder errors from MP texts are possible.
        //
        if (!defaultValueExpr || bindWA->errStatus()) {
            // 7001 Error preparing default on <column> for <table>.
            *CmpCommon::diags() << DgSqlCode(-7001)
                                << DgString0(defaultValueStr)
                                << DgString1(nacol->getFullColRefNameAsAnsiString());
            bindWA->setErrStatus();
            return boundExpr;
          }

	newRecExprArray().insertAt(i, assign->getValueId());
	newRecExpr().insert(assign->getValueId());
	updateToSelectMap().addMapEntry(target,defaultValueExpr->getValueId());

	if (needToDeallocateColDefaultValueStr && defaultValueStr != NULL)
	  {
	    NADELETEBASIC((NAWchar*)defaultValueStr, bindWA->wHeap());
	    defaultValueStr = NULL;
	  }

        if (--defaultColCount == 0)
          break;  // tiny performance hack

      }   // NOT newRecExprArray().used(i)
      else 
      {
        if (nacol->getDefaultClass() == COM_IDENTITY_GENERATED_ALWAYS)
          {
            Assign * assign = (Assign*)newRecExprArray()[i].getItemExpr();
            ItemExpr * ie = assign->getSource().getItemExpr();

            // The IDENTITY column type of GENERATED ALWAYS AS IDENTITY
            // can not be used with user specified values.
            // However, if the override CQD is set, then
            // allow user specified values to be added
            // for a GENERATED ALWAYS AS IDENTITY column.
            if ((NOT ie->wasDefaultClause()) &&
                (CmpCommon::getDefault(OVERRIDE_GENERATED_IDENTITY_VALUES) == DF_OFF))
              {
                *CmpCommon::diags() << DgSqlCode(-3428)
                                    << DgString0(nacol->getColName());
                bindWA->setErrStatus();
                return boundExpr;
              }
          }
      }
      
      if (isASystemColumn)
        sysColIx++;
      else
        usrColIx++;
    }   // for i < totalColCount
  }   // defaultColCount

  // Now add the default values created as part of the Assigns above
  // to the charcteristic inputs. The user specified values are added
  // to the characteristic inputs during GenericUpdate::bindNode
  // executed earlier as part of this method.
  getGroupAttr()->addCharacteristicInputs(bindWA->
                                          getCurrentScope()->
                                          getOuterRefs());


  if (isRRTable) {
    const LIST(IndexDesc *) indexes = getTableDesc()->getIndexes();
    for(i = 0; i < indexes.entries(); i++) {
      indexes[i]->getPartitioningFunction()->setAssignPartition(TRUE);
    }
  }

 if ((getOperatorType() == REL_UNARY_INSERT) &&
     (getTableDesc()->getNATable()->hasLobColumn()) &&
     child(0)->getOperatorType() == REL_TUPLE_LIST) // VALUES (1,'b'),(2,'Y')
    {
      if (child(0)->getOperatorType() == REL_TUPLE_LIST)
	{
	  TupleList * tl = (TupleList*)(child(0)->castToRelExpr());
	  for (CollIndex x = 0; x < (UInt32)tl->numTuples(); x++)
	    {
	      ValueIdList tup;
	      if (!tl->getTuple(bindWA, tup, x)) 
		{
		  bindWA->setErrStatus();

		  return boundExpr; // something went wrong
		}
	      
	      for (CollIndex n = 0; n < tup.entries(); n++)
		{
		  ItemExpr * ie = tup[n].getItemExpr();
                  
		  if (ie->getOperatorType() == ITM_LOBINSERT)
		    {                                                          
                      // cannot have this function in a values list with
                      // multiple tuples. Use a single tuple.
                          *CmpCommon::diags() << DgSqlCode(-4483);
                          bindWA->setErrStatus();		      
                          return boundExpr; 
                        
                    }
               
                  else
                    {
                      Assign * assign = (Assign*)newRecExprArray()[n].getItemExpr();
                      ItemExpr *assign_child = NULL;
                      if (assign)
                        {
                           assign_child = assign->child(1);
                        }
                      if ( assign_child && assign_child->getOperatorType() == ITM_CAST )
                        {
                          const NAType& type = assign_child->getValueId().getType();
                          if ( type.getTypeQualifier() == NA_LOB_TYPE )
                            {
                              // cannot have this function in a values list with multiple
                              // tuples. Use a single tuple.
                              *CmpCommon::diags() << DgSqlCode(-4483);
                              bindWA->setErrStatus();		      
                              return boundExpr; 
                            }
                        }
                    }
                }
            }
        }
    }



  // It is a system generated identity value if
  // identityColumn() != NULL_VALUE_ID. The identityColumn()
  // is set two places (1) earlier in this method.
  // (2) DefaultSpecification::bindNode()

  // The IDENTITY column of type GENERATED ALWAYS AS IDENTITY
  // must be specified in the values list as (DEFAULT) or
  // must be excluded from the values list forcing the default.

  if (identityColumnGeneratedAlways &&
      NOT systemGeneratesIdentityValue())
    {
      // The IDENTITY column type of GENERATED ALWAYS AS IDENTITY
      // can not be used with user specified values.
      // However, if the override CQD is set, then
      // allow user specified values to be added
      // for a GENERATED ALWAYS AS IDENTITY column.

      if (CmpCommon::getDefault(OVERRIDE_GENERATED_IDENTITY_VALUES) == DF_OFF)
        {
          *CmpCommon::diags() << DgSqlCode(-3428)
                              << DgString0(identityColumnName.data());
           bindWA->setErrStatus();
           return boundExpr;
        }
    }

  ItemExpr *orderByTree = removeOrderByTree();
  if (orderByTree) {
    bindWA->getCurrentScope()->context()->inOrderBy() = TRUE;
    bindWA->getCurrentScope()->setRETDesc(child(0)->getRETDesc());
    orderByTree->convertToValueIdList(reqdOrder(), bindWA, ITM_ITEM_LIST);

    bindWA->getCurrentScope()->context()->inOrderBy() = FALSE;
    if (bindWA->errStatus()) return NULL;
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
  }

  setInUpdateOrInsert(bindWA);

  // Triggers --
  NABoolean insertFromValuesList =
    getOperatorType() == REL_UNARY_INSERT &&
    (child(0)->getOperatorType() == REL_TUPLE || // VALUES (1,'b')
    child(0)->getOperatorType() == REL_TUPLE_LIST || // VALUES (1,'b'),(2,'Y')
    child(0)->getOperatorType() == REL_UNION);       // VALUES with subquery

  // Insert from values that gets input from above should not use flow,
  // for performance. Cases, other than TUPLE, should be investigated.
  if (bindWA->findNextScopeWithTriggerInfo() != NULL
      && (getGroupAttr()->getCharacteristicInputs() != NULL)
      && (insertFromValuesList))
    setNoFlow(TRUE);

  if (getUpdateCKorUniqueIndexKey())
  {
    SqlTableOpenInfo * scanStoi = getLeftmostScanNode()->getOptStoi()->getStoi();
    short updateColsCount = scanStoi->getColumnListCount();
    getOptStoi()->getStoi()->setColumnListCount(updateColsCount);
    getOptStoi()->getStoi()->setColumnList(new (bindWA->wHeap()) short[updateColsCount]);
      for (short i=0; i<updateColsCount; i++)
        getOptStoi()->getStoi()->setUpdateColumn(i,scanStoi->getUpdateColumn(i));
  }
   
  if ((getIsTrafLoadPrep()) &&
      (getTableDesc()->getCheckConstraints().entries() != 0 ||
          getTableDesc()->getNATable()->getRefConstraints().entries() != 0  ))
  {
    // enabling/disabling constraints is not supported yet
    //4486--Constraints not supported with bulk load. Disable the constraints and try again.
    *CmpCommon::diags() << DgSqlCode(-4486)
                        <<  DgString0("bulk load") ;
    bindWA->setErrStatus();
    return boundExpr;
  }
  if (getIsTrafLoadPrep())
  {
    PartitioningFunction *pf = getTableDesc()->getClusteringIndex()->getPartitioningFunction();

    const NodeMap* np;
    Lng32 partns = 1;
    if ( pf && (np = pf->getNodeMap()) )
      {
        partns = np->getNumEntries();
        if(partns > 1  && CmpCommon::getDefault(ATTEMPT_ESP_PARALLELISM) == DF_OFF)
	  {
	    // 4490 - BULK LOAD into a salted table is not supported if 
	    // ESP parallelism is turned off
	    *CmpCommon::diags() << DgSqlCode(-4490);
	     bindWA->setErrStatus();
	     return boundExpr;
	  }
      }

    const NATable* naTable = getTableDesc()->getNATable();
    if (naTable->hasLobColumn())
      {
	NAColumn *nac = NULL;
	for (CollIndex c = 0; c < naTable->getColumnCount(); c++) 
	  {
	    nac = naTable->getNAColumnArray()[c];
	    if (nac->getType()->isLob())
	      break;
	  }
	*CmpCommon::diags() << DgSqlCode(-4494)
			    << DgTableName(naTable->getTableName().
					   getQualifiedNameAsAnsiString()) 
			    << DgColumnName(nac->getColName());
	bindWA->setErrStatus();
	return boundExpr;
      } // has Lob column
  } // isLoadPrep

  NABoolean toMerge = FALSE;
  if (isUpsertThatNeedsTransformation(isAlignedRowFormat, omittedDefaultCols, omittedCurrentDefaultClassCols,toMerge)) {
    if ((CmpCommon::getDefault(TRAF_UPSERT_TO_EFF_TREE) == DF_OFF) ||toMerge)	
      {
	boundExpr = xformUpsertToMerge(bindWA);  
	return boundExpr;
      }
    else 
      boundExpr = xformUpsertToEfficientTree(bindWA);
    
    
  }
  if (NOT (isMerge() || noIMneeded()))
    boundExpr = handleInlining(bindWA, boundExpr);

  // turn OFF Non-atomic Inserts for ODBC if we have detected that Inlining is needed
  // necessary warnings have been generated in handleInlining method.
  if (CmpCommon::getDefault(ODBC_PROCESS) == DF_ON) {
    if (bindWA->getHostArraysArea() &&
	(NOT bindWA->getHostArraysArea()->getRowwiseRowset()) &&
	!(bindWA->getHostArraysArea()->getTolerateNonFatalError()))
      setTolerateNonFatalError(RelExpr::UNSPECIFIED_);
  }
  
  
  // When mtsStatement_ or bulkLoadIndex is set Insert needs to return rows;
  // so potential outputs are added (note that it's not replaced) to 
  // the Insert node. Currently mtsStatement_ is set
  // for MTS queries and embedded insert queries.
  if (isMtsStatement() || bulkLoadIndex)
    {
      if(isMtsStatement())
        setRETDesc(new (bindWA->wHeap()) RETDesc(bindWA, getTableDesc()));

      bindWA->getCurrentScope()->setRETDesc(getRETDesc());

      ValueIdList outputs;
      getRETDesc()->getValueIdList(outputs, USER_AND_SYSTEM_COLUMNS);
      ValueIdSet potentialOutputs;
      getPotentialOutputValues(potentialOutputs);
      potentialOutputs.insertList(outputs); 
      setPotentialOutputValues(potentialOutputs);

      // this flag is set to indicate optimizer not to pick the
      // TupleFlow operator
      setNoFlow(TRUE);
    }

  return boundExpr;
} // Insert::bindNode()

/* Upsert into a table with an index is converted into a Merge to avoid
the problem described in Trafodion-14. An upsert may overwrite an existing row
in the base table (identical to the update when matched clause of Merge) or
it may insert a new row into the base table (identical to insert when not
matched clause of merge). If the upsert caused a row to be updated in the 
base table then the old version of the row will have to be deleted from 
indexes, and a new version inserted. Upsert is being transformed to merge
so that we can delete the old version of an updated row from the index.

Upsert is also converted into merge when TRAF_UPSERT_MODE is set to MERGE and 
there are omitted cols with default values in case of aligned format table or 
omitted current timestamp cols in case of non-aligned row format
*/
NABoolean Insert::isUpsertThatNeedsTransformation(NABoolean isAlignedRowFormat, 
                                                  NABoolean omittedDefaultCols,
                                                  NABoolean omittedCurrentDefaultClassCols,
                                                  NABoolean &toMerge) const
{
  toMerge = FALSE;
  // If the the table has an identity column in clustering key or has a syskey 
  // we dont need to do this transformation.The incoming row will always be 
  // unique. So first check if we any of the conditions are satisfied to 
  //even try the transform
  NABoolean mustTryTransform = FALSE;
  if (isUpsert() && 
      NOT ( getIsTrafLoadPrep() ||
            ( (getTableDesc()->isIdentityColumnGeneratedAlways() && 
               getTableDesc()->hasIdentityColumnInClusteringKey()))  || 
            ((getTableDesc()->getClusteringIndex()->getNAFileSet()->hasSyskey()))))
    {
      mustTryTransform = TRUE;
    }

  // Transform upsert to merge in case of special modes and
  // omitted default columns
  // Case 1 :  CQD is set to MERGE, omitted current(timestamp) default 
  //           columns with  non-aligned row format table or omitted 
  //           default columns with aligned row format tables 

  // Case 2 :  CQD is set to Optimal, for non-aligned row format with omitted 
  //           current(timestamp) columns, it is converted into merge 
  //           though it is not optimal for performance. This is done to ensure
  //           that when the CQD is set to optimal, non-aligned format would 
  //           behave like merge when any column is  omitted 
  if (isUpsert()  &&   
      mustTryTransform &&          
      ((CmpCommon::getDefault(TRAF_UPSERT_MODE) == DF_MERGE) &&     
       (((NOT isAlignedRowFormat) && omittedCurrentDefaultClassCols) ||
        (isAlignedRowFormat && omittedDefaultCols)))
      ||
      ((CmpCommon::getDefault(TRAF_UPSERT_MODE) == DF_OPTIMAL) &&
       ((NOT isAlignedRowFormat) && omittedCurrentDefaultClassCols)))
    {
      toMerge = TRUE;
      return TRUE;
    }

  // Transform upsert to efficient tree if none of the above conditions 
  // are true and the table has secondary indexes 
  if (isUpsert() &&  
      mustTryTransform &&
      (getTableDesc()->hasSecondaryIndexes()))
    {
      toMerge = FALSE;
      return TRUE;
    }
  
  return FALSE;
}

/** commenting the following method out for future work. This may be enabled 
as a further performance improvement if we can eliminate the sort node that 
gets geenrated as part of the Sequence Node. In case of no duplicates we won't
 need the Sequence node at all. 

// take an insert(src) node and transform it into
// a tuple_flow with old/new rows flowing to the IM tree.
// with a newly created input_scan
RelExpr* Insert::xformUpsertToEfficientTreeNoDup(BindWA *bindWA) 
{
  NATable *naTable = bindWA->getNATable(getTableName());
  if (bindWA->errStatus())
    return NULL;
  if ((naTable->getViewText() != NULL) && (naTable->getViewCheck()))		
    {		
      *CmpCommon::diags() << DgSqlCode(-3241) 		
			  << DgString0(" View with check option not allowed.");	    		
      bindWA->setErrStatus();		
      return NULL;		
    }

  RelExpr *topNode = this;
  // Create a new BindScope, to encompass the new nodes 
  // upsert(left_join(input_scan, tuple))
  // and any inlining nodes that will be created. Any values the upsert
  // and children will need from src will be marked as outer references in that
  // new BindScope. We assume that "src" is already bound.
  ValueIdSet currOuterRefs = bindWA->getCurrentScope()->getOuterRefs();

  CMPASSERT(child(0)->nodeIsBound());

  BindScope *upsertScope = bindWA->getCurrentScope();

  // columns of the target table
  const ValueIdList &tableCols = updateToSelectMap().getTopValues();
  const ValueIdList &sourceVals = updateToSelectMap().getBottomValues();

  // create a Join node - left join of the base table columns with the columns to be upserted.
  // columns of the target table
  CMPASSERT(child(0)->nodeIsBound());
  
  Scan * targetTableScan =
    new (bindWA->wHeap())
    Scan(CorrName(getTableDesc()->getCorrNameObj(), bindWA->wHeap()));

 
  //join predicate between source columns and target table.
  ItemExpr * keyPred = NULL;
  ItemExpr * keyPredPrev = NULL;
  BaseColumn* baseCol;
  ColReference * targetColRef;
  int predCount = 0;
  ValueIdSet newOuterRefs;
  ItemExpr * pkeyValPrev;
  ItemExpr * pkeyVals;
  for (CollIndex i = 0; i < tableCols.entries(); i++)
    {
      baseCol = (BaseColumn *)(tableCols[i].getItemExpr()) ;
      if (baseCol->getNAColumn()->isSystemColumn())
	continue;

      targetColRef = new(bindWA->wHeap()) ColReference(
						       new(bindWA->wHeap()) ColRefName(
										       baseCol->getNAColumn()->getFullColRefName(), bindWA->wHeap()));
    

      if (baseCol->getNAColumn()->isClusteringKey())
	{
	  // create a join/key predicate between source and target table,
	  // on the clustering key columns of the target table, making
	  // ColReference nodes for the target table, so that we can bind
	  // those to the new scan
	  keyPredPrev = keyPred;
	  keyPred = new (bindWA->wHeap())
	    BiRelat(ITM_EQUAL, targetColRef, 
		    sourceVals[i].getItemExpr(),
		    baseCol->getType().supportsSQLnull());
	  predCount++;
	  if (predCount > 1) 
	    {
	      keyPred = new(bindWA->wHeap()) BiLogic(ITM_AND,
						     keyPredPrev,
						     keyPred);  
	    }
	  pkeyValPrev = pkeyVals;
    
	  pkeyVals = tableCols[i].getItemExpr();
	  if (i > 0) 
	    {
	      pkeyVals = new(bindWA->wHeap()) ItemList(pkeyVals,pkeyValPrev);
      
	    }
	}
     
    }
 
  // Map the table's primary key values to the source lists key values
  ValueIdList tablePKeyVals = NULL;
  ValueIdList sourcePKeyVals = NULL;
  
  pkeyVals->convertToValueIdList(tablePKeyVals,bindWA,ITM_ITEM_LIST);
  updateToSelectMap().mapValueIdListDown(tablePKeyVals,sourcePKeyVals);
  
  Join *lj = new(bindWA->wHeap()) Join(child(0),targetTableScan,REL_LEFT_JOIN,keyPred);
  lj->doNotTransformToTSJ();	  
  lj->setTSJForWrite(TRUE);
   bindWA->getCurrentScope()->xtnmStack()->createXTNM();
  RelExpr *boundLJ = lj->bindNode(bindWA);
  if (bindWA->errStatus())
    return NULL;
  bindWA->getCurrentScope()->xtnmStack()->removeXTNM();
  setChild(0,boundLJ);
  topNode = handleInlining(bindWA,topNode);


  return topNode; 
}
*/

// take an insert(src) node and transform it into
// a tuple_flow with old/new rows flowing to the IM tree.
// with a newly created sequence node used to eliminate duplicates.
/*
               NJ
            /      \
         Sequence   NJ
        /            \  
     Left Join        IM Tree 
     /        \
    /          \
Input Tuplelist  Target Table Scan
or select list
*/
         
RelExpr* Insert::xformUpsertToEfficientTree(BindWA *bindWA) 
{
  NATable *naTable = bindWA->getNATable(getTableName());
  if (bindWA->errStatus())
    return NULL;
  if ((naTable->getViewText() != NULL) && (naTable->getViewCheck()))		
    {		
      *CmpCommon::diags() << DgSqlCode(-3241) 		
			  << DgString0(" View with check option not allowed.");	    		
      bindWA->setErrStatus();		
      return NULL;		
    }

  RelExpr *topNode = this;
 
  CMPASSERT(child(0)->nodeIsBound());

  BindScope *upsertScope = bindWA->getCurrentScope();
  // Create a new BindScope, to encompass the new nodes 
  // upsert(left_join(input_scan, tuple))
  // and any inlining nodes that will be created. Any values the upsert
  // and children will need from src will be marked as outer references in that
  // new BindScope. We assume that "src" is already bound.
  ValueIdSet currOuterRefs = bindWA->getCurrentScope()->getOuterRefs();
  // Save the current RETDesc.
  RETDesc *prevRETDesc = bindWA->getCurrentScope()->getRETDesc();

  // columns of the target table
  const ValueIdList &tableCols = updateToSelectMap().getTopValues();
  const ValueIdList &sourceVals = updateToSelectMap().getBottomValues();

  // create a Join node - left join of the base table columns with the columns to be upserted.
  // columns of the target table
  CMPASSERT(child(0)->nodeIsBound());
  
  Scan * targetTableScan =
    new (bindWA->wHeap())
    Scan(CorrName(getTableDesc()->getCorrNameObj(), bindWA->wHeap()));

   bindWA->getCurrentScope()->context()->inUpsertXform() = TRUE;
  //join predicate between source columns and target table.
  ItemExpr * keyPred = NULL;
  ItemExpr * keyPredPrev = NULL;
  BaseColumn* baseCol;
  ColReference * targetColRef;
  int predCount = 0;
  ValueIdSet newOuterRefs;
  ItemExpr * pkeyValPrev = NULL;
  ItemExpr * pkeyVals = NULL;
  for (CollIndex i = 0; i < tableCols.entries(); i++)
    {
      baseCol = (BaseColumn *)(tableCols[i].getItemExpr()) ;
      if (baseCol->getNAColumn()->isSystemColumn())
	continue;

      targetColRef = new(bindWA->wHeap()) ColReference(
           new(bindWA->wHeap()) ColRefName(
                baseCol->getNAColumn()->getFullColRefName(),
                bindWA->wHeap()));
    

      if (baseCol->getNAColumn()->isClusteringKey())
	{
	  // create a join/key predicate between source and target table,
	  // on the clustering key columns of the target table, making
	  // ColReference nodes for the target table, so that we can bind
	  // those to the new scan
	  keyPredPrev = keyPred;
	  keyPred = new (bindWA->wHeap())
	    BiRelat(ITM_EQUAL, targetColRef, 
		    sourceVals[i].getItemExpr(),
		    baseCol->getType().supportsSQLnull());
	  predCount++;
	  if (predCount > 1) 
	    {
	      keyPred = new(bindWA->wHeap()) BiLogic(ITM_AND,
						     keyPredPrev,
						     keyPred);  
	    }
          
          pkeyValPrev = pkeyVals;
    
	  pkeyVals = tableCols[i].getItemExpr();
	  
	  if (pkeyValPrev != NULL ) 
	    {
	      pkeyVals = new(bindWA->wHeap()) ItemList(pkeyVals,pkeyValPrev);
      
	    }
	}
     
    }
 
  // Map the table's primary key values to the source lists key values
  ValueIdList tablePKeyVals ;
  ValueIdList sourcePKeyVals ;
  
  pkeyVals->convertToValueIdList(tablePKeyVals,bindWA,ITM_ITEM_LIST);
  updateToSelectMap().mapValueIdListDown(tablePKeyVals,sourcePKeyVals);
  
  Join *lj = new(bindWA->wHeap()) Join(child(0),targetTableScan,REL_LEFT_JOIN,keyPred);

  bindWA->getCurrentScope()->xtnmStack()->createXTNM();

  
  RelExpr *boundLJ = lj->bindNode(bindWA);
  if (bindWA->errStatus())
    return NULL;
  bindWA->getCurrentScope()->xtnmStack()->removeXTNM();
 
  ValueId nullInstIndicator(
       lj->addNullInstIndicatorVar(
            bindWA,
            new(bindWA->wHeap()) SystemLiteral(
                 "U",
                 CharInfo::ISO88591)));
  ValueIdSet sequenceFunction ;		
 
  //Retrieve all the system and user columns of the left join output
  ValueIdList  ljOutCols = 0;
  boundLJ->getRETDesc()->getValueIdList(ljOutCols);
  //Retrieve the null instantiated part of the LJ output
  ValueIdList ljNullInstColumns = lj->nullInstantiatedOutput();
  
  //Create the olap node and use the primary key of the table as the 
  //"partition by" columns for the olap node.
  CMPASSERT(!bindWA->getCurrentScope()->getSequenceNode());
  RelSequence *seqNode = new(bindWA->wHeap()) RelSequence(boundLJ, sourcePKeyVals.rebuildExprTree(ITM_ITEM_LIST),  (ItemExpr *)NULL);
 

  // Create a LEAD Item Expr for a random value 999. 
  // Use this to eliminate rows which have a NULL for this LEAD value within 
  // a particular partition range.
  ItemExpr *leadItem, *boundLeadItem = NULL;
  ItemExpr *constLead999 = new (bindWA->wHeap()) ConstValue( 999);

  leadItem = new(bindWA->wHeap()) ItmLeadOlapFunction(constLead999,1);
  ((ItmLeadOlapFunction *)leadItem)->setIsOLAP(TRUE);
  boundLeadItem = leadItem->bindNode(bindWA);
  if (bindWA->errStatus()) return this;
  boundLeadItem->convertToValueIdSet(sequenceFunction);
  seqNode->setSequenceFunctions(sequenceFunction);
  
  // Add a selection predicate (post predicate) to check if the LEAD item is NULL
  ItemExpr *selPredOnLead = NULL;
  selPredOnLead = new (bindWA->wHeap()) UnLogic(ITM_IS_NULL,leadItem);
  selPredOnLead = selPredOnLead->bindNode(bindWA);
  if (bindWA->errStatus()) return this;
  seqNode->selectionPred() += selPredOnLead->getValueId();
  seqNode->setChild(0,boundLJ);

  RelExpr *boundSeqNode = seqNode->bindNode(bindWA);  
   
  setChild(0,boundSeqNode);

  // Fixup the newRecExpr() and newRecExprArray() to refer to the new 
  // valueIds of the new child - i.e RelSequence. Use the saved off valueIdMap
  // from the current bindScope for this.
  ValueIdSet newNewRecExpr;
  ValueIdMap notCoveredMap = bindWA->getCurrentScope()->getNcToOldMap();
  notCoveredMap.rewriteValueIdSetUp(newNewRecExpr, newRecExpr());
  newRecExpr() = newNewRecExpr;
  
  ValueIdList oldRecArrList(newRecExprArray());
  ValueIdList newRecArrList;
  notCoveredMap.rewriteValueIdListUp(newRecArrList, oldRecArrList);
  ValueIdArray newNewRecArray(newRecArrList.entries());
  
  for (CollIndex i = 0; i < newRecArrList.entries(); i++)
    {
      newNewRecArray.insertAt(i,newRecArrList.at(i));
    }
  newRecExprArray() = newNewRecArray;

  ValueId notCoveredNullInstIndicator;

  notCoveredMap.rewriteValueIdUp(notCoveredNullInstIndicator,
                                 nullInstIndicator);
  ItemExpr *nvl = new(bindWA->wHeap()) BuiltinFunction(
         ITM_NVL,
         bindWA->wHeap(),
         2,
         notCoveredNullInstIndicator.getItemExpr(),
         new(bindWA->wHeap()) SystemLiteral("I",
                                            CharInfo::ISO88591));
  nvl = nvl->bindNode(bindWA);
  setProducedMergeIUDIndicator(nvl->getValueId());

  setXformedEffUpsert(TRUE);
  bindWA->getCurrentScope()->context()->inUpsertXform() =  FALSE;
  return topNode; 
}


// take an insert(src) node and transform it into
// tsj_flow(src, merge_update(input_scan))
// with a newly created input_scan
RelExpr* Insert::xformUpsertToMerge(BindWA *bindWA) 
{
  NATable *naTable = bindWA->getNATable(getTableName());
  if (bindWA->errStatus())
    return NULL;
  if ((naTable->getViewText() != NULL) && (naTable->getViewCheck()))		
  {		
    *CmpCommon::diags() << DgSqlCode(-3241) 		
			<< DgString0(" View with check option not allowed.");	    		
    bindWA->setErrStatus();		
    return NULL;		
  }

  // columns of the target table
  const ValueIdList &tableCols = updateToSelectMap().getTopValues();
  const ValueIdList &sourceVals = updateToSelectMap().getBottomValues();

  NABoolean isAlignedRowFormat = getTableDesc()->getNATable()->isSQLMXAlignedTable();
		    
  // Create a new BindScope, to encompass the new nodes merge_update(input_scan)
  // and any inlining nodes that will be created. Any values the merge_update
  // and children will need from src will be marked as outer references in that
  // new BindScope. We assume that "src" is already bound.
  ValueIdSet currOuterRefs = bindWA->getCurrentScope()->getOuterRefs();

  CMPASSERT(child(0)->nodeIsBound());
  bindWA->initNewScope();
  BindScope *mergeScope = bindWA->getCurrentScope();

  // create a new scan of the target table, to be used in the merge
  Scan * inputScan =
    new (bindWA->wHeap())
    Scan(CorrName(getTableDesc()->getCorrNameObj(), bindWA->wHeap()));

  ItemExpr * keyPred = NULL;
  ItemExpr * keyPredPrev = NULL;
  ItemExpr * setAssign = NULL;
  ItemExpr * setAssignPrev = NULL;
  ItemExpr * insertVal = NULL;
  ItemExpr * insertValPrev = NULL;
  ItemExpr * insertCol = NULL;
  ItemExpr * insertColPrev = NULL;
  BaseColumn* baseCol;
  ColReference * targetColRef;
  int predCount = 0;
  int setCount = 0;
  ValueIdSet newOuterRefs;

  // loop over the columns of the target table
  for (CollIndex i = 0; i<tableCols.entries(); i++)
  {
    baseCol = (BaseColumn *)(tableCols[i].getItemExpr()) ;
    if (baseCol->getNAColumn()->isSystemColumn())
      continue;

    targetColRef = new(bindWA->wHeap()) ColReference(
         new(bindWA->wHeap()) ColRefName(
              baseCol->getNAColumn()->getFullColRefName(), bindWA->wHeap()));
    if (baseCol->getNAColumn()->isClusteringKey())
    {
      // create a join/key predicate between source and target table,
      // on the clustering key columns of the target table, making
      // ColReference nodes for the target table, so that we can bind
      // those to the new scan
      keyPredPrev = keyPred;
      keyPred = new (bindWA->wHeap())
        BiRelat(ITM_EQUAL, targetColRef, 
                sourceVals[i].getItemExpr(),
                baseCol->getType().supportsSQLnull());
      predCount++;
      if (predCount > 1) 
      {
         keyPred = new(bindWA->wHeap()) BiLogic(ITM_AND,
                                                keyPredPrev,
                                                keyPred);  
      }

    }
    if (sourceVals[i].getItemExpr()->getOperatorType() != ITM_CONSTANT)
      {
        newOuterRefs += sourceVals[i];
        mergeScope->addOuterRef(sourceVals[i]);
      }

    // create the INSERT (WHEN NOT MATCHED) part of the merge for this column, again
    // with a ColReference that we will then bind to the MergeUpdate target table
    insertValPrev = insertVal;
    insertColPrev = insertCol ;
    insertVal = sourceVals[i].getItemExpr();
    insertCol =  new(bindWA->wHeap()) ColReference(
                     new(bindWA->wHeap()) ColRefName(
                          baseCol->getNAColumn()->getFullColRefName(), bindWA->wHeap()));
    if (i > 0) 
    {
      insertVal = new(bindWA->wHeap()) ItemList(insertVal,insertValPrev);
      insertCol = new(bindWA->wHeap()) ItemList(insertCol,insertColPrev);
    }
  }
  inputScan->addSelPredTree(keyPred);
  for (CollIndex i = 0 ; i < newRecExprArray().entries(); i++) 
  {
      const Assign *assignExpr = (Assign *)newRecExprArray()[i].getItemExpr();
      ValueId tgtValueId = assignExpr->child(0)->castToItemExpr()->getValueId();
      NAColumn *col = tgtValueId.getNAColumn( TRUE );
      NABoolean copySetAssign = FALSE;
      if (col->isSystemColumn())
         continue;
      else
      if (! col->isClusteringKey()) 
      {
         // Create the UPDATE (WHEN MATCHED) part of the new MergeUpdate for
         // a non-key column. We need to bind in the new = old values
         // in GenericUpdate::bindNode. So skip the columns that are not user
         // specified. Note that we had a discussion on whether such a transformed
         // UPSERT shouldn't update all columns.
         //
         if (assignExpr->isUserSpecified())
             copySetAssign = TRUE;
         // If copy the Default values in case of replace mode or optiomal mode with
         // aligned row tables
         else if ((CmpCommon::getDefault(TRAF_UPSERT_MODE) == DF_REPLACE) ||
                (isAlignedRowFormat && CmpCommon::getDefault(TRAF_UPSERT_MODE) == DF_OPTIMAL))
             copySetAssign = TRUE;
         if (copySetAssign)
         { 
            setAssignPrev = setAssign;
            setAssign = (ItemExpr *)assignExpr;
            setCount++;
            if (setCount > 1) 
               setAssign = new(bindWA->wHeap()) ItemList(setAssignPrev, setAssign);
         }
     }
  }
  MergeUpdate *mu = new (bindWA->wHeap())
    MergeUpdate(CorrName(getTableDesc()->getCorrNameObj(), bindWA->wHeap()),
                NULL,
                REL_UNARY_UPDATE,
                inputScan, // USING
                setAssign, // WHEN MATCHED THEN UPDATE
                insertCol, // WHEN NOT MATCHED THEN INSERT (cols) ...
                insertVal, // ... VALUES()
                bindWA->wHeap(),
                NULL);

  mu->setXformedUpsert();
  // Use mergeScope, the scope we created here, for the MergeUpdate. We are
  // creating some expressions with outer references here in this method, so
  // we need to control the scope from here.
  mu->setNeedsBindScope(FALSE);

  RelExpr *boundMU = mu->bindNode(bindWA);

  // remove the BindScope created earlier in this method
  bindWA->removeCurrentScope();

  // Remove the outer refs from the parent scope, they are provided
  // by the left child of the TSJ_FLOW, unless they were already outer refs
  // when we started this method. The binder logic doesn't handle
  // that well, since they come from a child scope, not the current one,
  // so we help a little.
  newOuterRefs -= currOuterRefs;
  bindWA->getCurrentScope()->removeOuterRefs(newOuterRefs);

  Join * jn = new(bindWA->wHeap()) Join(child(0), boundMU, REL_TSJ_FLOW, NULL);

  jn->doNotTransformToTSJ();
  jn->setTSJForMerge(TRUE);	
  jn->setTSJForMergeWithInsert(TRUE);
  jn->setTSJForMergeUpsert(TRUE);
  jn->setTSJForWrite(TRUE);

  RelExpr *result = jn->bindNode(bindWA);
  if (bindWA->errStatus())
    return NULL;
  // Copy the userSecified and canBeSkipped attribute to mergeUpdateInsertExprArray
  ValueIdList mergeInsertExprArray = mu->mergeInsertRecExprArray();
  for (CollIndex i = 0 ; i < newRecExprArray().entries(); i++) 
  {
      const Assign *assignExpr = (Assign *)newRecExprArray()[i].getItemExpr();
      ((Assign *)mergeInsertExprArray[i].getItemExpr())->setToBeSkipped(assignExpr->canBeSkipped());
      ((Assign *)mergeInsertExprArray[i].getItemExpr())->setUserSpecified(assignExpr->isUserSpecified());
  }
 
  return result;
}

RelExpr *HBaseBulkLoadPrep::bindNode(BindWA *bindWA)
{
  //CMPASSERT((CmpCommon::getDefault(TRAF_LOAD) == DF_ON &&
   //   CmpCommon::getDefault(TRAF_LOAD_HFILE) == DF_ON));
  if (nodeIsBound())
  {
    return this;
  }

  Insert * newInsert = new (bindWA->wHeap())
                            Insert(getTableName(),
                                   NULL,
                                   REL_UNARY_INSERT,
                                   child(0)->castToRelExpr());


  newInsert->setInsertType(UPSERT_LOAD);
  newInsert->setIsTrafLoadPrep(true);
  newInsert->setCreateUstatSample(getCreateUstatSample());

  // Pass the flag to bindWA to guarantee that a range partitioning is 
  // always used for all source and target tables.
  bindWA->setIsTrafLoadPrep(TRUE);

  RelExpr *boundNewInsert = newInsert->bindNode(bindWA);

  if (bindWA->errStatus())
    return NULL;

  return boundNewInsert;



}

// This is a callback from DefaultSpecification::bindNode
// called from Insert::bindNode
// (you need to understand the latter to understand this).
//
const char *Insert::getColDefaultValue(BindWA *bindWA, CollIndex i) const
{
  CMPASSERT(canBindDefaultSpecification());

  CollIndexList &colnoList = *targetUserColPosList_;
  CollIndex pos = colnoList.entries() ? colnoList[i] : i;

  const ValueIdList &colList = getTableDesc()->getColumnList();

  if (colList.entries() <= pos) {
    // 4023 degree of row value constructor must equal that of target table
    *CmpCommon::diags() << DgSqlCode(-4023)
                        << DgInt0(++pos)
                        << DgInt1(colList.entries());
    bindWA->setErrStatus();
    return NULL;
  }

  ValueId target = colList[pos];
  const NAColumn *nacol = target.getNAColumn();
  const char* defaultValueStr = nacol->getDefaultValue();
  
  CharInfo::CharSet mapCS = CharInfo::ISO88591;
  NABoolean mapCS_hasVariableWidth = CharInfo::isVariableWidthMultiByteCharSet(mapCS);
  size_t defaultValueWcsLen = 0;
  NAWchar *defaultValueWcs = (NAWchar *) defaultValueStr;
  NABoolean ucs2StrLitPrefix = FALSE;

  if (nacol->getDefaultClass() == COM_USER_DEFINED_DEFAULT &&
      nacol->getType() &&
      nacol->getType()->getTypeQualifier() == NA_CHARACTER_TYPE &&
      ((CharType*)(nacol->getType()))->getCharSet() == CharInfo::ISO88591 &&
      mapCS_hasVariableWidth &&
      defaultValueWcs != NULL &&
      nacol->getNATable()->getObjectSchemaVersion() >= COM_VERS_2300 &&
      (defaultValueWcsLen = NAWstrlen(defaultValueWcs)) > 6 &&
      ( ( ucs2StrLitPrefix = ( NAWstrncmp(defaultValueWcs, NAWSTR("_UCS2\'"), 6) == 0 )) ||
        ( defaultValueWcsLen > 10 &&
          NAWstrncmp(defaultValueWcs, NAWSTR("_ISO88591\'"), 10) == 0 )) &&
      defaultValueWcs[defaultValueWcsLen-1] == NAWCHR('\''))
  {
    NAWcharBuf *pWcharBuf = NULL;
    if (ucs2StrLitPrefix)
    {
      // Strip the leading _UCS2 prefix.
      pWcharBuf =
        new (bindWA->wHeap()) NAWcharBuf(&defaultValueWcs[5],
                                         defaultValueWcsLen - 5,
                                         bindWA->wHeap());
    }
    else
    {
      // Keep the leading _ISO88591 prefix.
      pWcharBuf =
        new (bindWA->wHeap()) NAWcharBuf(defaultValueWcs,
                                         defaultValueWcsLen,
                                         bindWA->wHeap());
    }
    charBuf *pCharBuf = NULL; // must set this variable to NULL so the
                              // following function call will allocate
                              // space for the output literal string
    Int32 errorcode = 0;
    pCharBuf = unicodeTocset(*pWcharBuf, bindWA->wHeap(),
                             pCharBuf, mapCS, errorcode);
    // Earlier releases  treated the converted multibyte character
    // string, in ISO_MAPPING character set, as if it is a string of
    // ISO88591 characters and then convert it back to UCS-2 format;
    // i.e., for each byte in the string, insert an extra byte
    // containing the binary zero value.
    NADELETE(pWcharBuf, NAWcharBuf, bindWA->wHeap());
    pWcharBuf = NULL; // must set this variable to NULL to force the
                      // following call to allocate space for the
                      // the output literal string
    pWcharBuf = ISO88591ToUnicode(*pCharBuf, bindWA->wHeap(), pWcharBuf);
    // Prepare the converted literal string for the following CAST
    // function by setting pColDefaultValueStr to point to the string
    NAWchar *pWcs = NULL;
    if (ucs2StrLitPrefix)
    {
      pWcs = new (bindWA->wHeap()) NAWchar[10+NAWstrlen(pWcharBuf->data())];
      NAWstrcpy(pWcs, NAWSTR("_ISO88591"));
    }
    else
    {
      pWcs = new (bindWA->wHeap()) NAWchar[1+NAWstrlen(pWcharBuf->data())];
      pWcs[0] = NAWCHR('\0');
    }
    NAWstrcat(pWcs, pWcharBuf->data());
    defaultValueStr = (char *)pWcs;
    NADELETE(pWcharBuf, NAWcharBuf, bindWA->wHeap());
    NADELETE(pCharBuf, charBuf, bindWA->wHeap());
  }

  if (NOT defaultValueStr AND bindWA) {
    // 4107 column has no default so DEFAULT cannot be specified.
    *CmpCommon::diags() << DgSqlCode(-4107) << DgColumnName(nacol->getColName());
    bindWA->setErrStatus();
  }

  return defaultValueStr;
} // Insert::getColDefaultValue()


// -----------------------------------------------------------------------
// member functions for class Update
// -----------------------------------------------------------------------

RelExpr *Update::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }
  // Set flag for firstN in context 
  if (child(0) && child(0)->getOperatorType() == REL_SCAN)
    if (child(0)->castToRelExpr() && 
        ((Scan *)(child(0)->castToRelExpr()))->getFirstNRows() >= 0)
      if (bindWA && 
          bindWA->getCurrentScope() && 
          bindWA->getCurrentScope()->context())
        bindWA->getCurrentScope()->context()->firstN() = TRUE;

  setInUpdateOrInsert(bindWA, this, REL_UPDATE);

  RelExpr * boundExpr = GenericUpdate::bindNode(bindWA);
  if (bindWA->errStatus()) return NULL;
  setInUpdateOrInsert(bindWA);

  if (getTableDesc()->getNATable()->isHbaseCellTable())
    {
      *CmpCommon::diags() << DgSqlCode(-1425)
			  << DgTableName(getTableDesc()->getNATable()->getTableName().
					 getQualifiedNameAsAnsiString())
			  << DgString0("Reason: Cannot update an hbase table in CELL format. Use ROW format for this operation.");
    
      bindWA->setErrStatus();
      return this;
    }

  // QSTUFF
  if (getGroupAttr()->isStream() &&
     !getGroupAttr()->isEmbeddedUpdateOrDelete()) {
    *CmpCommon::diags() << DgSqlCode(-4173);
    bindWA->setErrStatus();
    return this;
  }
  // QSTUFF

  if (NOT bindWA->errStatus() AND
      NOT getTableDesc()->getVerticalPartitions().isEmpty())
    {
      // 4058 UPDATE query cannot be used on a vertically partitioned table.
      *CmpCommon::diags() << DgSqlCode(-4058) <<
        DgTableName(getTableDesc()->getNATable()->getTableName().
          getQualifiedNameAsAnsiString());
      bindWA->setErrStatus();
      return this;
    }

  // make sure scan done as part of an update runs in serializable mode so a
  // tsj(scan,update) implementation of a update runs as an atomic operation
  if (child(0)->getOperatorType() == REL_SCAN) {
    Scan *scanNode = (Scan*)(child(0)->castToRelExpr());
    if (!scanNode->accessOptions().userSpecified()) {
      scanNode->accessOptions().updateAccessOptions
        (TransMode::ILtoAT(TransMode::SERIALIZABLE_));
    }
  }

  // if FIRST_N is requested, insert a FirstN node.
  if ((getOperatorType() == REL_UNARY_UPDATE) &&
      (child(0)->getOperatorType() == REL_SCAN))
    {
      Scan * scanNode = (Scan *)(child(0)->castToRelExpr());
      if ((scanNode->getFirstNRows() != -1) &&
          (getGroupAttr()->isEmbeddedUpdateOrDelete()))
        {
          *CmpCommon::diags() << DgSqlCode(-4216);
          bindWA->setErrStatus();
          return NULL;
        }

      if (scanNode->getFirstNRows() >= 0)
        {
          FirstN * firstn = new(bindWA->wHeap())
            FirstN(scanNode, scanNode->getFirstNRows(), FALSE /* there's no ORDER BY on an UPDATE */, NULL);
          firstn->bindNode(bindWA);
          if (bindWA->errStatus())
            return NULL;

          setChild(0, firstn);
        }
    }

  // if rowset is used in set clause a direct rowset that is not in subquery 
  // must be present in the where clause
  if ((bindWA->getHostArraysArea()) &&
        (bindWA->getHostArraysArea()->hasHostArraysInSetClause()) &&
        (!(bindWA->getHostArraysArea()->hasHostArraysInWhereClause()))) {
      *CmpCommon::diags() << DgSqlCode(-30021) ;
      bindWA->setErrStatus();
      return this;
    }

  NABoolean transformUpdateKey = updatesClusteringKeyOrUniqueIndexKey(bindWA);
  if (bindWA->errStatus()) // error occurred in updatesCKOrUniqueIndexKey()
    return this;

  if ((transformUpdateKey) && (NOT isMerge()))
    {
      boundExpr = transformUpdatePrimaryKey(bindWA);
    }
  else
    boundExpr = handleInlining(bindWA, boundExpr);
  
  if (bindWA->errStatus()) // error occurred in transformUpdatePrimaryKey()
    return this;    // or handleInlining()
  
  return boundExpr;
} // Update::bindNode()

// -----------------------------------------------------------------------
// member functions for class MergeUpdate
// -----------------------------------------------------------------------

RelExpr *MergeUpdate::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    {
      bindWA->getCurrentScope()->setRETDesc(getRETDesc());
      return this;
    }

  if (needsBindScope_)
    bindWA->initNewScope();

  // For an xformed upsert any UDF or subquery is guaranteed to be 
  // in the using clause. Upsert will not generate a merge without using 
  // clause. ON clause, when matched SET clause and when not matched INSERT
  // clauses all use expressions from the using clause. (same vid).
  // Therefore any subquery or UDF in the using clause will flow to the
  // rest of he tree through the TSJ and will be available. Each subquery
  // will be evaluated only once, and will be evaluated prior to the merge
  if (isMerge() && child(0) && !xformedUpsert())
  {
    ItemExpr *selPred = child(0)->castToRelExpr()->selPredTree();
    if (selPred || where_)
    {
      NABoolean ONhasSubquery = (selPred && selPred->containsSubquery());
      NABoolean ONhasAggr = (selPred && selPred->containsAnAggregate());
      NABoolean whrHasSubqry = FALSE;
      if (ONhasSubquery || ONhasAggr ||
	  (where_ && ((whrHasSubqry=where_->containsSubquery()) ||
                      where_->containsAnAggregate())))
      {
	*CmpCommon::diags() << DgSqlCode(-3241) 
			    << DgString0
	  (ONhasSubquery ? "Subquery in ON clause not allowed." :
           (ONhasAggr ? "aggregate function in ON clause not allowed." :
            (whrHasSubqry ?
             "subquery in UPDATE ... WHERE clause not allowed." :
             "aggregate function in UPDATE ... WHERE clause not allowed.")));  
	bindWA->setErrStatus();
	return this;
      }
      ItemExpr *ONhasUDF = (selPred ? selPred->containsUDF() : NULL);
      ItemExpr *whereHasUDF = (where_ ? where_->containsUDF() : NULL);
      if (ONhasUDF || whereHasUDF)
      {
	*CmpCommon::diags() << DgSqlCode(-4471)
			    << DgString0
	  (((UDFunction *)(ONhasUDF ? ONhasUDF : whereHasUDF))->
	   getFunctionName().getExternalName()); 
	bindWA->setErrStatus();
	return this;
      }
    }
  }

  if (isMerge() && recExprTree() && !xformedUpsert())
  {
    if (recExprTree()->containsSubquery())
    {
      *CmpCommon::diags() << DgSqlCode(-3241) 
                          << DgString0(" Subquery in SET clause not allowed.");
      bindWA->setErrStatus();
      return this;
    }
    if (recExprTree()->containsUDF())
    {
      *CmpCommon::diags() << DgSqlCode(-4471) 
                          << DgString0(((UDFunction *)recExprTree()->containsUDF())->
                                       getFunctionName().getExternalName());    
      bindWA->setErrStatus();
      return this;
    }
  }

  // if insertValues, then this is an upsert stmt.
  if (insertValues())
    {
      if (insertValues()->containsSubquery() && !xformedUpsert())
        {
          *CmpCommon::diags() << DgSqlCode(-3241) 
                              << DgString0(" Subquery in INSERT clause not allowed.");
          bindWA->setErrStatus();
          return this;
        }
      if (insertValues()->containsUDF() && !xformedUpsert())
        {
          *CmpCommon::diags() << DgSqlCode(-4471) 
                              << DgString0(((UDFunction *)insertValues()->containsUDF())->
                                           getFunctionName().getExternalName());
          bindWA->setErrStatus();
          return this;
        }

      Tuple * tuple = new (bindWA->wHeap()) Tuple(insertValues());
      Insert * ins = new (bindWA->wHeap())
        Insert(getTableName(),
               NULL,
               REL_UNARY_INSERT,
               tuple,
               insertCols(),
               NULL);
      ins->setInsertType(Insert::SIMPLE_INSERT);
      if (isMergeUpdate())
        ins->setIsMergeUpdate(TRUE);
      else
        ins->setIsMergeDelete(TRUE);

      ins->setTableDesc(getTableDesc());
      bindWA->getCurrentScope()->xtnmStack()->createXTNM();
      ins = (Insert*)ins->bindNode(bindWA);
      if (bindWA->errStatus()) 
        return NULL;
      bindWA->getCurrentScope()->xtnmStack()->removeXTNM();

      mergeInsertRecExpr() = ins->newRecExpr();
      mergeInsertRecExprArray() = ins->newRecExprArray();
    }

  NATable *naTable = bindWA->getNATable(getTableName());
  if (bindWA->errStatus())
    return NULL;
 
  if ((naTable->getViewText() != NULL) && (naTable->getViewCheck()))		
  {		
    *CmpCommon::diags() << DgSqlCode(-3241) 		
			<< DgString0(" View with check option not allowed.");	    		
    bindWA->setErrStatus();		
    return NULL;		
  }

  if ((naTable->isHbaseCellTable()) ||
      (naTable->isHbaseRowTable()))
    {
      *CmpCommon::diags() << DgSqlCode(-3241) 
			  << DgString0("Hbase tables not supported.");
      bindWA->setErrStatus();
      return NULL;
    }

  if (naTable->isHiveTable())
    {
      *CmpCommon::diags() << DgSqlCode(-3241) 
			  << DgString0("Hive tables not supported.");
      bindWA->setErrStatus();
      return NULL;
    }

  bindWA->setMergeStatement(TRUE);  

  // Create a merge IUD indicator, a CHAR(1) CHARACTER SET ISO88591
  // NOT NULL variable, that can be used by index maintenance and
  // other operations to find out what action the WHEN clause
  // indicated, insert (I), update (U) or delete (D). This will be
  // removed in GenericUpdate::normalizeNode() if nobody asked for
  // it. The actual value will be produced by the executor work
  // method, there is no expression for it.
  if (getProducedMergeIUDIndicator() == NULL_VALUE_ID)
    {
      ItemExpr *mergeIUDIndicator = new(bindWA->wHeap()) NATypeToItem(
           new(bindWA->wHeap()) SQLChar(bindWA->wHeap(), 
                1, FALSE, FALSE, FALSE, FALSE, CharInfo::ISO88591));

      mergeIUDIndicator = mergeIUDIndicator->bindNode(bindWA);
      if (bindWA->errStatus())
        return NULL;
      setProducedMergeIUDIndicator(mergeIUDIndicator->getValueId());
    }

  RelExpr * boundExpr = Update::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  if (checkForMergeRestrictions(bindWA))
    return NULL;

  if (where_) {
    bindWA->getCurrentScope()->context()->inWhereClause() = TRUE;
    where_->convertToValueIdSet(mergeUpdatePred(), bindWA, ITM_AND);
    bindWA->getCurrentScope()->context()->inWhereClause() = FALSE;

    if (bindWA->errStatus()) return NULL;

    // any values added by where_ to Outer References Set should be
    // added to input values that must be supplied to this MergeUpdate
    getGroupAttr()->addCharacteristicInputs
      (bindWA->getCurrentScope()->getOuterRefs());
  }

  if (needsBindScope_)
    bindWA->removeCurrentScope();

  bindWA->setMergeStatement(TRUE);

  return boundExpr;
} // MergeUpdate::bindNode()

// -----------------------------------------------------------------------
// member functions for class Delete
// -----------------------------------------------------------------------

RelExpr *Delete::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }
  // Save the current scope and node for children to peruse if necessary.
  BindContext *context = bindWA->getCurrentScope()->context();
  if (context) {
    context->deleteScope() = bindWA->getCurrentScope();
    context->deleteNode()  = this;
    if (getFirstNRows() >= 0) context->firstN() = TRUE;
  }

  RelExpr * boundExpr = GenericUpdate::bindNode(bindWA);
  if (bindWA->errStatus()) return boundExpr;

  if ((csl_) &&
      (NOT getTableDesc()->getNATable()->isHbaseRowTable()))
    {
      *CmpCommon::diags() << DgSqlCode(-1425)
			  << DgTableName(getTableDesc()->getNATable()->getTableName().
					 getQualifiedNameAsAnsiString());
    
      bindWA->setErrStatus();
      return this;
    }

  if (getTableDesc()->getNATable()->isHbaseCellTable())
    {
      *CmpCommon::diags() << DgSqlCode(-1425)
			  << DgTableName(getTableDesc()->getNATable()->getTableName().
					 getQualifiedNameAsAnsiString())
			  << DgString0("Reason: Cannot delete from an hbase table in CELL format. Use ROW format for this operation.");
    
      bindWA->setErrStatus();
      return this;
    }

  // QSTUFF
  if (getGroupAttr()->isStream() &&
     !getGroupAttr()->isEmbeddedUpdateOrDelete()) {
    *CmpCommon::diags() << DgSqlCode(-4180);
    bindWA->setErrStatus();
    return this;
  }
  // QSTUFF

  // Not only are check constraints on a DELETE nonsensical,
  // but they can cause VEGReference::replaceVEGReference to assert
  // with valuesToBeBound.isEmpty (Genesis 10-980202-0718).
  //
  // in case we are binding a generic update within a generic update
  // due to view expansion we would like to ensure that all constraints
  // are checked properly for the update operation performed on the
  // underlying base table
  if (NOT (bindWA->inViewExpansion() && bindWA->inGenericUpdate())) {  // QSTUFF
    getTableDesc()->checkConstraints().clear();
    checkConstraints().clear();
  }

  if (NOT getTableDesc()->getVerticalPartitions().isEmpty())
    {
      // 4029 DELETE query cannot be used on a vertically partitioned table.
      *CmpCommon::diags() << DgSqlCode(-4029) <<
        DgTableName(getTableDesc()->getNATable()->getTableName().
                      getQualifiedNameAsAnsiString());
      bindWA->setErrStatus();
      return this;
    }

  Scan *scanNode = NULL;
  // make sure scan done as part of a delete runs in serializable mode so a
  // tsj(scan,delete) implementation of a delete runs as an atomic operation
  if (child(0)->getOperatorType() == REL_SCAN) {
    scanNode = (Scan*)(child(0)->castToRelExpr());
    if (!scanNode->accessOptions().userSpecified()) {
      scanNode->accessOptions().updateAccessOptions
        (TransMode::ILtoAT(TransMode::SERIALIZABLE_));
    }
  }
  
  BindScope *prevScope   = NULL;
  BindScope *currScope   = bindWA->getCurrentScope();
  NABoolean inUnion = FALSE;

  while (currScope && !inUnion)
  {
    BindContext *currContext = currScope->context();
    if (currContext->inUnion())
    {
      inUnion = TRUE;
    }
    prevScope = currScope;
    currScope = bindWA->getPreviousScope(prevScope);
  }

  RelRoot *root =  bindWA->getTopRoot();

  if (getFirstNRows() >= 0)  // First N Delete
  {
    CMPASSERT(getOperatorType() == REL_UNARY_DELETE);

    // First N Delete on a partitioned table. Not considered a MTS delete.
    if (getTableDesc()->getClusteringIndex()->isPartitioned())
    {

      if (root->getCompExprTree() || inUnion ) // for unions we know there is a select
      {  // outer selectnot allowed for "non-MTS" first N delete
        *CmpCommon::diags() << DgSqlCode(-4216);
        bindWA->setErrStatus();
        return this;
      }

      RelExpr * childNode = child(0)->castToRelExpr();
      FirstN * firstn = new(bindWA->wHeap())
        FirstN(childNode, getFirstNRows(), FALSE /* There's no ORDER BY on a DELETE */, NULL);
      firstn->bindNode(bindWA);
      if (bindWA->errStatus())
        return NULL;

      setChild(0, firstn);
      setFirstNRows(-1);

    }
    else
    {
      // First N delete on a single partition. This is considered a MTS Delete.
      if ((bindWA->getHostArraysArea()) &&
          ((bindWA->getHostArraysArea()->hasHostArraysInWhereClause()) ||
          (bindWA->getHostArraysArea()->getHasSelectIntoRowsets())))
      { // MTS delete not supported with rowsets
        *CmpCommon::diags() << DgSqlCode(-30037);
        bindWA->setErrStatus();
        return this;
      }

    
      if (scanNode && scanNode->getSelectionPred().containsSubquery())
      {
        // MTS Delete not supported with subquery in where clause
        *CmpCommon::diags() << DgSqlCode(-4138);

        bindWA->setErrStatus();
        return this;

      }

      if (root->hasOrderBy())
      { // mts delete not supported with order by
        *CmpCommon::diags() << DgSqlCode(-4189);
        bindWA->setErrStatus();
        return this;
      }
      if (root->getCompExprTree() || // MTS Delete has an outer select
          bindWA->isInsertSelectStatement() || // Delete inside an Insert Select statement, Soln:10-061103-0274
          inUnion )  // for unions we know there is a select
      {                                                                 
        if (root->getFirstNRows() < -1 || 
            inUnion) // for unions we wish to raise a union 
        {  // The outer select has a Last 1/0 clause      // specific error later, so set the flag now. 
          setMtsStatement(TRUE);  
        }
        else
        { // raise an error if no Last 1 clause is found.
          *CmpCommon::diags() << DgSqlCode(-4136);
          bindWA->setErrStatus();
          return this;
        }
      }
    }
  }

  // Triggers --
  
  if (NOT noIMneeded())
    boundExpr = handleInlining(bindWA, boundExpr);
  else if (hbaseOper() && (getGroupAttr()->isEmbeddedUpdateOrDelete()))
  {
     setRETDesc(new (bindWA->wHeap()) RETDesc(bindWA));

     CorrName corrOLDTable (getScanNode(TRUE)->getTableDesc()->getCorrNameObj().getQualifiedNameObj(),
             bindWA->wHeap(),"OLD");

     // expose OLD table columns
     getRETDesc()->addColumns(bindWA, *child(0)->getRETDesc(), &corrOLDTable);

     ValueIdList outputs;
     getRETDesc()->getValueIdList(outputs, USER_AND_SYSTEM_COLUMNS);
     addPotentialOutputValues(outputs);

     bindWA->getCurrentScope()->setRETDesc(getRETDesc());
  }

  if (isMtsStatement())
    bindWA->setEmbeddedIUDStatement(TRUE);

   if (getFirstNRows() > 0) 
    {
      // create a firstN node to delete FIRST N rows, if no such node was created
      // during handleInlining. Occurs when DELETE FIRST N is used on table with no
      // dependent objects. 
      FirstN * firstn = new(bindWA->wHeap())
        FirstN(boundExpr, getFirstNRows(), FALSE /* There's no ORDER BY on a DELETE */ );
      if (NOT(scanNode && scanNode->getSelectionPred().containsSubquery()))
        firstn->setCanExecuteInDp2(TRUE);
      firstn->bindNode(bindWA);
      if (bindWA->errStatus())
        return NULL;

      setFirstNRows(-1);
      boundExpr = firstn;
    }

   if (csl())
     {
       for (Lng32 i = 0; i < csl()->entries(); i++)
	 {
	   NAString * nas = (NAString*)(*csl())[i];
	   
	   bindWA->hbaseColUsageInfo()->insert
	     ((QualifiedName*)&getTableDesc()->getNATable()->getTableName(), nas);
	 }
     }

  return boundExpr;
} // Delete::bindNode()

// -----------------------------------------------------------------------
// member functions for class MergeDelete
// -----------------------------------------------------------------------

RelExpr *MergeDelete::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    {
      bindWA->getCurrentScope()->setRETDesc(getRETDesc());
      return this;
    }
  
  bindWA->initNewScope();

  if ((isMerge()) && 
      (child(0)) && 
      (child(0)->castToRelExpr()->selPredTree()))
  {
    if (child(0)->castToRelExpr()->selPredTree()->containsSubquery())
    {
      *CmpCommon::diags() << DgSqlCode(-3241) 
                          << DgString0(" Subquery in ON clause not allowed.");    
      bindWA->setErrStatus();
      return this;
    }
    if (child(0)->castToRelExpr()->selPredTree()->containsUDF())
    {
      *CmpCommon::diags() << DgSqlCode(-4471)
                          << DgString0(((UDFunction *)child(0)->
                                       castToRelExpr()->selPredTree()->
                                       containsUDF())->
                                       getFunctionName().getExternalName());
      bindWA->setErrStatus();
      return this;
    }
  }


  // if insertValues, then this is an upsert stmt.
  if (insertValues())
    {
      if (insertValues()->containsSubquery())
      {
        *CmpCommon::diags() << DgSqlCode(-3241) 
                            << DgString0(" Subquery in INSERT clause not allowed.");
        bindWA->setErrStatus();
        return this;
      }
      if (insertValues()->containsUDF())
      {
        *CmpCommon::diags() << DgSqlCode(-4471)
                            << DgString0(((UDFunction *)insertValues()->
                                          containsUDF())->
                                         getFunctionName().getExternalName());
        bindWA->setErrStatus();
        return this;
      }
      if (CmpCommon::getDefault(COMP_BOOL_175) == DF_OFF)
      {
        // MERGE DELETE + INSERT is buggy, so disallow it unless CQD is on. In
        // particular, the optimizer sometimes fails to produce a plan in phase 1.
        // JIRA TRAFODION-1509 covers completing the MERGE DELETE + INSERT feature.
	*CmpCommon::diags() << DgSqlCode(-3241)
			    << DgString0(" MERGE DELETE not allowed with INSERT.");

      }

      Tuple * tuple = new (bindWA->wHeap()) Tuple(insertValues());
      Insert * ins = new (bindWA->wHeap())
      Insert(getTableName(),
             NULL,
             REL_UNARY_INSERT,
             tuple,
             insertCols(),
             NULL);
      ins->setInsertType(Insert::SIMPLE_INSERT);
      ins->setIsMergeDelete(TRUE);

      ins->setTableDesc(getTableDesc());
      bindWA->getCurrentScope()->xtnmStack()->createXTNM();
      ins = (Insert*)ins->bindNode(bindWA);
      if (bindWA->errStatus()) 
        return NULL;
      bindWA->getCurrentScope()->xtnmStack()->removeXTNM();

      mergeInsertRecExpr() = ins->newRecExpr();
      mergeInsertRecExprArray() = ins->newRecExprArray();
    }

  NATable *naTable = bindWA->getNATable(getTableName());
  if (bindWA->errStatus())
    return NULL;
  
  if ((naTable->getViewText() != NULL) && (naTable->getViewCheck()))		
  {		
    *CmpCommon::diags() << DgSqlCode(-3241) 		
			<< DgString0(" View with check option not allowed.");	    		
    bindWA->setErrStatus();		
    return NULL;		
  }

  bindWA->setMergeStatement(TRUE);  
  RelExpr * boundExpr = Delete::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  if (checkForMergeRestrictions(bindWA))
    return NULL;

  bindWA->removeCurrentScope(); 

  bindWA->setMergeStatement(TRUE);

  return boundExpr;
} // MergeDelete::bindNode()

static const char NEWTable [] = "NEW";    // QSTUFF:  corr for embedded d/u
static const char OLDTable [] = "OLD";    // QSTUFF:  corr for embedded d/u

// QSTUFF
// this method binds both, the set clauses applied to the after
// image as well as the set clauses applied to the before image
// the new set on rollback clause allows an application to modify
// the before image.
// delete from tab set on rollback x = 1;
// update tab set x = 1 set on rollback x = 2;
void GenericUpdate::bindUpdateExpr(BindWA        *bindWA,
                                   ItemExpr      *recExpr,
                                   ItemExprList  &assignList,
                                   RelExpr       *boundView,
                                   Scan          *scanNode,
                                   SET(short)    &stoiColumnSet,
                                   NABoolean     onRollback)
{

  RETDesc *origScope = NULL;

  ValueIdSet &newRecExpr =
    (onRollback == TRUE) ? newRecBeforeExpr() : this->newRecExpr();
  ValueIdArray &newRecExprArray =
    (onRollback == TRUE) ? newRecBeforeExprArray() : this->newRecExprArray();

  if (onRollback &&
      ((!getTableDesc()->getClusteringIndex()->getNAFileSet()->isAudited()) ||
       (getTableDesc()->getNATable()->hasLobColumn()))) {
    // SET ON ROLLBACK clause is not allowed on a non-audited table
    *CmpCommon::diags() << DgSqlCode(-4214)
      << DgTableName(getTableDesc()->getNATable()->getTableName().
        getQualifiedNameAsAnsiString());
    bindWA->setErrStatus();
    return;
  }

  CollIndex     i, j;
  CollIndexList colnoList(STMTHEAP);   // map of col nums (row positions)
  CollIndex a = assignList.entries();

  const ColumnDescList *viewColumns = NULL;

  // if this is a view then get the columns of the view
  if (boundView) {
    viewColumns = boundView->getRETDesc()->getColumnList();
  }

  // if the GU has a SET ON ROLLBACK clause this method is called
  // twice: once to bind the columns in the SET clause and a second
  // time to bind the columns in the SET ON ROLLBACK clause.
  // Initially the update column list of the stoi_ is empty.
  // If this is the second call, store the update column list
  // from the first call.
  short *stoiColumnList = NULL;
  CollIndex currColumnCount = 0;
  if (currColumnCount = stoi_->getStoi()->getColumnListCount())
  {
       stoiColumnList = new (bindWA->wHeap()) short[currColumnCount];

       for (i = 0;  i < currColumnCount; i++)
              stoiColumnList[i] = stoi_->getStoi()->getUpdateColumn(i);
  }

  stoi_->getStoi()->setColumnList(new (bindWA->wHeap()) short[a + currColumnCount]);

  for (i = 0; i < a; i++) {
    CMPASSERT(assignList[i]->getOperatorType() == ITM_ASSIGN);
    assignList[i]->child(0)->bindNode(bindWA);                  // LHS
    if (bindWA->errStatus()) return;
    const NAColumn *nacol = assignList[i]->child(0).getNAColumn();
    if(getOperatorType() == REL_UNARY_UPDATE)
    {
        stoi_->getStoi()->setUpdateColumn(i, (short) nacol->getPosition());
        stoi_->getStoi()->incColumnListCount();
        stoi_->addUpdateColumn(nacol->getPosition());
    }
    const NAType *colType = nacol->getType();
    if (!colType->isSupportedType()) {
      *CmpCommon::diags() << DgSqlCode(-4028)     // 4028 table not updatatble
      << DgTableName(nacol->getNATable()->getTableName().getQualifiedNameAsAnsiString());
      bindWA->setErrStatus();
      return;
    }

    // If this is a sequence generator IDENTITY column
    // with a default type of GENERATED ALWAYS,
    // then post error -3428.  GENERATED ALWAYS
    // IDENTITY columns may not be updated.
    if(getOperatorType() == REL_UNARY_UPDATE         &&
       CmpCommon::getDefault(COMP_BOOL_210) == DF_ON &&
       nacol->isIdentityColumnAlways())
      {
        *CmpCommon::diags() << DgSqlCode(-3428)
                              << DgString0(nacol->getColName());
        bindWA->setErrStatus();
        return;
      }

    colnoList.insert(nacol->getPosition());         // save colno for next loop

    // in case its not a view we record the column position of the
    // base table, otherwise that of the view

    if (NOT boundView)
      stoiColumnSet.insert((short) nacol->getPosition());

    // if this is a view get the positions of the columns
    // within the view that are being updated.
    if (boundView) {
      ValueId vid = assignList[i]->child(0).getValueId();
      NABoolean found = FALSE;
      for (CollIndex k=0; k < viewColumns->entries(); k++) {
        if ((*viewColumns)[k]->getValueId() == vid) {
          stoiColumnSet.insert((short) k);
          found = TRUE;
          // Updatable views cannot have any underlying basetable column
          // appear more than once, so it's safe to break out of the loop.
          break;
        }
      }  // loop k
      CMPASSERT(found);
    }  // boundView
  }  // loop i<a

  // If this is the second call to this method, restore the update
  // columns bound in the first call
  if (currColumnCount)
  {
      for (i = a;  i < (currColumnCount + a); i++)
      {
          stoi_->getStoi()->setUpdateColumn(i,  stoiColumnList[i-a]);
          stoi_->addUpdateColumn(stoiColumnList[i-a]);
      }
  }

  // RHS:  Bind the right side of the Assigns such that the source expressions
  // reference the columns of the source table.
  //
  //### With a cascade of views, should this be "getRETDesc" as is,
  //### or "scanNode->getRETDesc" ?                     --?
  //### Should I set this->setRD to be the target(new)tbl at the beginning,
  //### explicitly say "scanNode..." here?              --i think not
  //

  if (GU_DEBUG) GU_DEBUG_Display(bindWA, this, "u");

  origScope = bindWA->getCurrentScope()->getRETDesc();

  // this sets the scope to the scan table for the before values
  // the previous scope was to the "UPDATE" table
  // we will reset the scope before returning in order not to introduce
  // hidden side effects but have the generic update explicitely point
  // to the scan scope

  bindWA->getCurrentScope()->setRETDesc(getRETDesc());

  //this has to be done after binding the LHS because of triggers
  //Soln :10-050110-3403 : Don't side-effect the SET on ROLLBACK list
  //when we come down to process it the next time over.So process only
  //the assignList
  ItemExpr* tempExpr = assignList.convertToItemExpr();
  tempExpr->convertToValueIdSet(newRecExpr, bindWA, ITM_ITEM_LIST);
  if (bindWA->errStatus()) return;

  if (NOT onRollback)
  {
    for (ValueId v = newRecExpr.init(); newRecExpr.next(v);
	 newRecExpr.advance(v))
      {
	CMPASSERT(v.getItemExpr()->getOperatorType() == ITM_ASSIGN);
	
        // remove all the onrollack expressions
	if (((Assign *)v.getItemExpr())->onRollback())
	  {
	    newRecExpr.remove(v);
	  }
      }
  }
  else
  {
     for (ValueId v = newRecExpr.init(); newRecExpr.next(v);
              newRecExpr.advance(v))
     {
        CMPASSERT(v.getItemExpr()->getOperatorType() == ITM_ASSIGN);

        // remove all the NON-onrollack expressions
        if ((getOperatorType() == REL_UNARY_UPDATE) &&
               !(((Assign *)v.getItemExpr())->onRollback()))
        {
            newRecExpr.remove(v);
        }
     }

     if (getOperatorType() == REL_UNARY_DELETE)
     {
        recExpr->convertToValueIdSet(this->newRecExpr(), bindWA, ITM_ITEM_LIST);
     }
  }

  // now we built the RHS
  // Now we have our colnoList map with which to build a temporary array
  // (with holes) and get the update columns ordered (eliminating dups).
  // Actually we store the ids of the bound Assign nodes corresponding
  // to the columns, of course.
  //
  CollIndex totalColCount = getTableDesc()->getColumnList().entries();
  ValueIdArray holeyArray(totalColCount);
  ValueId assignId;                                 // i'th newRecExpr valueid
  for (i = 0, assignId = newRecExpr.init();         // bizarre ValueIdSet iter
         newRecExpr.next(assignId);
         i++, newRecExpr.advance(assignId)) {
     j = colnoList[i];
     if (holeyArray.used(j)) {
       const NAColumn *nacol = holeyArray[j].getItemExpr()->child(0).getNAColumn();
       //4022 target column multiply specified
       *CmpCommon::diags() << DgSqlCode(-4022) << DgColumnName(nacol->getColName());
       bindWA->setErrStatus();
       return;
     }
     holeyArray.insertAt(j, assignId);
   }

   //
   // Now we have the holey array.  The next loop ignores unused entries
   // and copies the used entries into newRecExprArray(), with no holes.
   // It also builds a list of the columns being updated that contain
   // a column on the right side of the SET assignment expression.
   //
   // Entering this loop, i is the number of specified update columns;
   // exiting, j is.
   //
   CMPASSERT(i == a);

   // we built a map between identifical old and new columns, i.e. columns
   // which are not updated and thus identical. We insert the resulting
   // equivalence relationships e.g. old.a = new.a during transformation
   // into the respective VEGGIES this allows the optimizer to select index
   // scan for satisfying order requirements specified by an order by clause
   // on new columns, e.g.
   // select * from (update t set y = y + 1 return new.a) t order by a;
   // we cannot get the benefit of this VEG for a merge statement when IM is required
   // allowing a VEG in this case causes corruption on base table key values because
   // we use the "old" value of key column from fetchReturnedExpr, which can be junk
   // in case there is no row to update/delete, and a brand bew row is being inserted
   NABoolean mergeWithIndex = isMerge() && getTableDesc()->hasSecondaryIndexes() ;
   if ((NOT onRollback) && (NOT mergeWithIndex)){
     for (i = 0;i < totalColCount; i++){
       if (!(holeyArray.used(i))){
         oldToNewMap().addMapEntry(
           scanNode->getTableDesc()->
           getColumnList()[i].getItemExpr()->getValueId(),
           getTableDesc()->
           getColumnList()[i].getItemExpr()->getValueId());
       }
     }
   }

   // when binding a view which contains an embedded update
   // we must map update valueids to scan value ids
   // to allow for checking of access rights.
   for (i = 0; i < getTableDesc()->getColumnList().entries();i++)
     bindWA->getUpdateToScanValueIds().addMapEntry(
     getTableDesc()->getColumnList()[i].getItemExpr()->getValueId(),
     scanNode->getTableDesc()->getColumnList()[i].getItemExpr()->getValueId());

   newRecExprArray.resize(i);
   TableDesc *scanDesc = scanNode->getTableDesc();
   NABoolean rightContainsColumn = FALSE;

   for (i = j = 0; i < totalColCount; i++) {
     if (holeyArray.used(i)) {
       ValueId assignExpr = holeyArray[i];
       newRecExprArray.insertAt(j++, assignExpr);
       ItemExpr *right = assignExpr.getItemExpr()->child(1);

       // even if a column is set to a constant we mark it
       // as updated to prevent indices covering this column from
       // being used for access
       ItemExpr *left = assignExpr.getItemExpr()->child(0);

       scanDesc->addColUpdated(left->getValueId());

       if (right->containsColumn())
         rightContainsColumn = TRUE;
     }
   }

   // WITH NO ROLLBACK not supported if rightside of update 
   // contains a column expression. Also this feature is not
   // supported with the SET ON ROLLBACK feature
   if (isNoRollback() ||
       (CmpCommon::transMode()->getRollbackMode() == TransMode::NO_ROLLBACK_))
  {
    if ((rightContainsColumn && CmpCommon::getDefault(ALLOW_RISKY_UPDATE_WITH_NO_ROLLBACK) == DF_OFF) || onRollback)        
    {
      NAString warnMsg = "";
      if(rightContainsColumn)
      {
	warnMsg = "Suggestion: Set ALLOW_RISKY_UPDATE_WITH_NO_ROLLBACK CQD to ON to allow";
	if (getOperatorType() == REL_UNARY_DELETE)
	  warnMsg += " DELETE ";
	else
	  warnMsg += " UPDATE ";
	warnMsg += "command with right-hand side SET clause consisting of columns.";
      }

      if (getOperatorType() == REL_UNARY_DELETE)
	*CmpCommon::diags() << DgSqlCode(-3234) << DgString0(warnMsg);
      else
	*CmpCommon::diags() << DgSqlCode(-3233) << DgString0(warnMsg);
      bindWA->setErrStatus();
      return ;
    }
  }

   CMPASSERT(j == a);
   bindWA->getCurrentScope()->setRETDesc(origScope);
}

void getScanPreds(RelExpr *start, ValueIdSet &preds)
{
  RelExpr *result = start;

  while (result) {
    preds += result->selectionPred();

    if (result->getOperatorType() == REL_SCAN) break;
    if (result->getArity() > 1) {
      return ;
    }
    result = result->child(0);
  }

  return;
}


// Note that this is the R2 compatible way to handle Halloween problem.
// This update (only insert for now) contains a reference to the
// target in the source.  This could potentially run into the so
// called Halloween problem.  Determine if this is a case we may be
// able to handle.  The cases that we handle are:
//
//   -- The reference to the target is in a subquery
//   -- There any number of references to the target in the source
//   -- The subquery cannot be a row subquery.
//   -- The subquery must contain only one source (the reference to the target)
//   -- 
//
//  Return TRUE if this does represent a Halloween problem and the caller will
//  then issue the error message
//
//  Return FALSE is this is a case we can handle.  Set the
//  'avoidHalloweenR2' flag in the subquery and this generic Update so
//  that the optimizer will pick a plan that is Halloween safe.
//
NABoolean GenericUpdate::checkForHalloweenR2(Int32 numScansToFind)
{
  
  // If there are no scans, no problem, return okay (FALSE)
  //
  if(numScansToFind == 0) {
    return FALSE;
  }

  // Allow any number of scans

  // Do not support for general NEO users.
  if (CmpCommon::getDefault(MODE_SPECIAL_1) == DF_OFF)
    return TRUE;

  // Number of scans of the target table found so far.
  //
  Int32 numHalloweenScans = 0;

  // Get the primary source of the generic update.  We are looking for
  // the halloween scans in the predicates of this scan node
  //
  ValueIdSet preds;
  getScanPreds(this, preds);

  Subquery *subq;

  // Search the preds of this scan for subqueries.
  //
  //    ValueIdSet &preds = scanNode->selectionPred();

  for(ValueId p = preds.init(); preds.next(p); preds.advance(p)) {
    ItemExpr *pred = p.getItemExpr();

    // If this pred contains a subquery, find the scans
    //
    if(pred->containsSubquery()) {
      ValueIdSet subqPreds;
      subqPreds += pred->getValueId();

      // Search all the preds and their children
      //
      while(subqPreds.entries()) {
        ValueIdSet children;
        for(ValueId s = subqPreds.init();
            subqPreds.next(s);
            subqPreds.advance(s)) {
          ItemExpr *term = s.getItemExpr();

          // Found a subquery, now look for the scan...
          //
          if(term->isASubquery()) {
            subq = (Subquery *)term;
              
            // We don't support row subqueries, keep looking for the scan
            // in the next subquery.
            if(!subq->isARowSubquery()) {

              // Is this the subquery that has the scan of the table
              // we are updating?
              //
              Scan *halloweenScan = subq->getSubquery()->getScanNode(FALSE);
              if(halloweenScan) { 
                  
                // Is this the scan we are looking for?
                //
                if(halloweenScan->getTableDesc()->getNATable() == 
                   getTableDesc()->getNATable()) {
                  subq->setAvoidHalloweenR2(this);
                  numHalloweenScans++;
                }
              }
            }
          }
            
          // Follow all the children as well.
          //
          for(Int32 i = 0; i < term->getArity(); i++) {
            children += term->child(i)->getValueId();
          }
        }
        subqPreds = children;
      }
    }
  }

  setAvoidHalloweenR2(numScansToFind);

  // If we found and marked all the halloween scans, then return FALSE (allow).
  // We have marked the subqueries to avoid the halloween problem.  This will
  // force the optimizer to pick a plan that will be safe.
  //
  if(numHalloweenScans == numScansToFind)
    return FALSE;

  return TRUE;
}

// See ANSI 7.9 SR 12 + 6.3 SR 8 for definition of "updatable" table
// references;  in particular, note that one of the requirements for a view's
// being updatable is that ultimately underlying it (passing through a
// whole stack of views) is *exactly one* wbase table -- i.e., no joins
// allowed.
//
RelExpr * GenericUpdate::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // QSTUFF
  // we indicate that we are in a generic update. If we are
  // already in a generic update we know that this time we are
  // binding a generic update within a view.
  // however be aware of the following scenario. We currently
  // reject embedded updates and streams in the source but
  // obviously allow view with embedded updates as a target.
  // Since its already within a generic update we will only
  // return the scan node to the  insert
  //
  // insert into select ... from (update/delete ....) t;
  //
  // but not cause the update to be bound in when doing
  //
  // insert into viewWithDeleteOrUpdate values(...);
  //
  // in both cases we got an insert->update/delete->scan
  NABoolean inGenericUpdate = FALSE;

  if (getOperatorType() != REL_UNARY_INSERT)
    inGenericUpdate = bindWA->setInGenericUpdate(TRUE);

  NABoolean returnScanNode =
    (inGenericUpdate && bindWA->inViewExpansion() &&
    ( getOperatorType() == REL_UNARY_DELETE ||
      getOperatorType() == REL_UNARY_UPDATE ));

  // those group attributes should be set only by the topmost
  // generic update once we are invoked when already binding
  // another generic we reset those group attributes since we
  // already know that we will only return a scan node

  if ((returnScanNode) && (child(0))) {
    child(0)->getGroupAttr()->setStream(FALSE);
    child(0)->getGroupAttr()->setSkipInitialScan(FALSE);
    child(0)->getGroupAttr()->setEmbeddedIUD(NO_OPERATOR_TYPE);
  }

  // if we have no user-specified access options then
  // get it from nearest enclosing scope that has one (if any)
  if (!accessOptions().userSpecified()) {
    StmtLevelAccessOptions *axOpts = bindWA->findUserSpecifiedAccessOption();
    if (axOpts) {
      accessOptions() = *axOpts;
    }
  }
  // The above code is in Scan::bindNode also.
  // It would be nice to refactor this common code; someday.

  // Make sure we have the appropriate transaction mode & isolation level
  // in order to do the update.  Genesis 10-970922-3488.
  // Keep this logic in sync with Generator::verifyUpdatableTransMode()!
  Lng32 sqlcodeA = 0, sqlcodeB = 0;

  // fix case 10-040429-7402 by checking our statement level access options
  // first before declaring any error 3140/3141.
  TransMode::IsolationLevel il;
  ActiveSchemaDB()->getDefaults().getIsolationLevel
    (il,
     CmpCommon::getDefault(ISOLATION_LEVEL_FOR_UPDATES));
  verifyUpdatableTrans(&accessOptions(), CmpCommon::transMode(),
                       il,
                       sqlcodeA, sqlcodeB);

  if (sqlcodeA || sqlcodeB) {
    // 3140 The isolation level cannot be READ UNCOMMITTED.
    // 3141 The transaction access mode must be READ WRITE.
    if (sqlcodeA) *CmpCommon::diags() << DgSqlCode(sqlcodeA);
    if (sqlcodeB) *CmpCommon::diags() << DgSqlCode(sqlcodeB);
    bindWA->setErrStatus();
    return this;
  }

  Int64 transId=-1;
  if ((isNoRollback() && 
       (NOT (Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))) &&
      ((CmpCommon::transMode()->getAutoCommit() != TransMode::ON_ ) ||
       (NAExecTrans(0, transId)))) {

    // do not return an error if this is a showplan query being compiled
    // in the second arkcmp.
    const NAString * val =
      ActiveControlDB()->getControlSessionValue("SHOWPLAN");
    if (NOT ((val) && (*val == "ON")))
      {
	*CmpCommon::diags() << DgSqlCode(-3231);   // Autocommit must be ON,
	bindWA->setErrStatus(); // if No ROLLBACK is specified in IUD statement syntax
	return this;
      }
  }

  if (isNoRollback() ||
      (CmpCommon::transMode()->getRollbackMode() == TransMode::NO_ROLLBACK_))
  {
    if ((child(0)->getGroupAttr()->isStream()) || 
        (child(0)->getGroupAttr()->isEmbeddedUpdateOrDelete()) ||
        (updateCurrentOf()))
    {
      if (getOperatorType() == REL_UNARY_DELETE)
        *CmpCommon::diags() << DgSqlCode(-3234);
      else
        *CmpCommon::diags() << DgSqlCode(-3233);
      bindWA->setErrStatus();
      return this;
    }
  }

  // The SQL standard as defined in ISO/IEC JTC 1/SC 32 date: 2009-01-12
  // CD 9075-2:200x(E) published by ISO/IEC JTC 1/SC 32/WG 3
  // "Information technology -- Database languages -- SQL --
  //  Part2: Foundation (SQL/Foundation)", page 920, section 14.14,
  // page 918, section 14.13, page 900, section 14.9, page 898, section 14.8
  // does allow correlation names in update & delete statements.
  // Therefore, we delete this unnecessary restriction as part of the fix
  // for genesis solution 10-090921-4747:
  // Many places in this method assume the specified target table
  // has no correlation name -- indeed, Ansi syntax does not allow one --
  // this assert is to catch any future syntax-extensions we may do.
  //
  // E.g., see code marked
  //	##SQLMP-SYNTAX-KLUDGE##
  // in SqlParser.y + SqlParserAux.cpp,
  // which add a non-Ansi corr name to all table refs
  // when they really only should add to SELECTed tables.
  // So here, in an INSERT/UPDATE/DELETEd table,
  // we UNDO that kludge.
  //
  //if (!getTableName().getCorrNameAsString().isNull()) {
    //CMPASSERT(SqlParser_NAMETYPE == DF_NSK ||
    // HasMPLocPrefix(getTableName().getQualifiedNameObj().getCatalogName()));
    //getTableName().setCorrName("");			// UNDO that kludge!
  //}

  // Genesis 10-980831-4973
  if (((getTableName().isLocationNameSpecified() || 
        getTableName().isPartitionNameSpecified()) &&
     (!Get_SqlParser_Flags(ALLOW_SPECIALTABLETYPE))) &&
     (getOperatorType() != REL_UNARY_DELETE)) {
    *CmpCommon::diags() << DgSqlCode(-4061);   // 4061 a partn not ins/upd'able
    bindWA->setErrStatus();
    return this;
  }
  
   // -- Triggers
  // If this node is part of the action of a trigger,
  // then don't count the rows that are affected.
  if (bindWA->findNextScopeWithTriggerInfo() != NULL)
  {
    rowsAffected_ = DO_NOT_COMPUTE_ROWSAFFECTED;

    // Does the table name match the name of one of the transition tables?
    if (updatedTableName_.isATriggerTransitionName(bindWA))
    {
      // 11020 Ambiguous or illegal use of transition name $0~string0.
      *CmpCommon::diags() << DgSqlCode(-11020)
                          << DgString0(getTableName().getQualifiedNameAsString());

      bindWA->setErrStatus();
      return this;
    }
  }

  // Get the NATable for this object, and an initial ref count.
  // Set up stoi.
  //
  // We do not suppress mixed name checking in getNATable for R1
  // from here, because prototype name executes through here.  We
  // want to check prototype name.

  const NATable *naTable = bindWA->getNATable(getTableName());
  if (bindWA->errStatus()) return this;

  if (naTable && naTable->isHbaseTable())
    hbaseOper() = TRUE;

  if (naTable && naTable->isHbaseMapTable() &&
      (CmpCommon::getDefault(TRAF_HBASE_MAPPED_TABLES_IUD) == DF_OFF))
    {
      *CmpCommon::diags() << DgSqlCode(-4223)
			  << DgString0("Insert/Update/Delete on HBase mapped tables is");
      
      bindWA->setErrStatus();
      return this;
    }

  if ((CmpCommon::getDefault(ALLOW_DML_ON_NONAUDITED_TABLE) == DF_OFF) &&
      naTable && naTable->getClusteringIndex() && 
      (!naTable->getClusteringIndex()->isAudited())
      // && !bindWA->isBindingMvRefresh()  // uncomment if non-audit MVs are ever supported
     )
  {
     *CmpCommon::diags() << DgSqlCode(-4211)
       << DgTableName(
           naTable->getTableName().getQualifiedNameAsAnsiString());
     bindWA->setErrStatus();
     return NULL;
  }
  
  Int32 beforeRefcount = naTable->getReferenceCount();

  OptSqlTableOpenInfo *listedStoi
    = setupStoi(stoi_, bindWA, this, naTable, getTableName());

  if (getOperatorType() == REL_UNARY_INSERT &&
      NOT naTable->isInsertable()) {
    *CmpCommon::diags() << DgSqlCode(-4027)  // 4027 table not insertable
      << DgTableName(naTable->getTableName().getQualifiedNameAsAnsiString());
    bindWA->setErrStatus();
    return this;
  }
  if (NOT naTable->isUpdatable()) {
    *CmpCommon::diags() << DgSqlCode(-4028)  // 4028 table not updatable
      << DgTableName(naTable->getTableName().getQualifiedNameAsAnsiString());
    bindWA->setErrStatus();
    return this;
  }

  if (naTable->isVerticalPartition()) {
    // On attempt to update an individual VP, say: 4082 table not accessible
    *CmpCommon::diags() << DgSqlCode(-4082) <<
       DgTableName(naTable->getTableName().getQualifiedNameAsAnsiString());
    bindWA->setErrStatus();
    return this;
  }


  if (naTable->isAnMV())
  {
    // we currently don't allow updating (deleting) MVs in a trigger action
    if (bindWA->inDDL() && bindWA->isInTrigger ())
    {
        *CmpCommon::diags() << DgSqlCode(-11051);
        bindWA->setErrStatus();
        return this;
    }

    // This table is a materialized view. Are we allowed to change it?
    if ((getTableName().getSpecialType() != ExtendedQualName::MV_TABLE) &&
        (getTableName().getSpecialType() != ExtendedQualName::GHOST_MV_TABLE))
    {
      // The special syntax flag was not used -
      // Only on request MV allows direct DELETE operations by the user.
      MVInfoForDML *mvInfo = ((NATable *)naTable)->getMVInfo(bindWA);
      if (mvInfo->getRefreshType() == COM_ON_REQUEST &&
          getOperatorType() == REL_UNARY_DELETE)
      {
        // Set NOLOG flag.
        setNoLogOperation();
      }
      else
      {
        // Direct update is only allowed for User Maintainable MVs.
        if (mvInfo->getRefreshType() != COM_BY_USER)
        {
          // A Materialized View cannot be directly updated.
          *CmpCommon::diags() << DgSqlCode(-12074);
          bindWA->setErrStatus();
          return this;
        }
      }
    }

    // If this is not an INTERNAL REFRESH command, make sure the MV is
    // initialized and available.
    if (!bindWA->isBindingMvRefresh())
    {
      if (naTable->verifyMvIsInitializedAndAvailable(bindWA))
        return NULL;
    }
  }

  if (naTable->isAnMVMetaData() &&
      getTableName().getSpecialType() != ExtendedQualName::MVS_UMD)
  {
    if (getTableName().getPrototype() == NULL ||
        getTableName().getPrototype()->getSpecialType() != ExtendedQualName::MVS_UMD)
    {  // ERROR 12075: A Materialized View Metadata Table cannot be directly updated.
      *CmpCommon::diags() << DgSqlCode(-12075);
      bindWA->setErrStatus();
      return this;
    }
  }

  if ((naTable->isSeabaseTable()) &&
      (naTable->isSeabaseMDTable() || 
       naTable->isSeabasePrivSchemaTable()) &&
      (NOT naTable->isUserUpdatableSeabaseMDTable()) &&
      (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      // IUD on hbase metadata is only allowed for internal queries.
     *CmpCommon::diags() << DgSqlCode(-1391)
			 <<  DgString0(naTable->getTableName().getQualifiedNameAsAnsiString())
                         << DgString1("metadata");
      bindWA->setErrStatus();
      return this;
    }
  else if ((naTable->isSeabaseTable()) &&
           (naTable->getTableName().getSchemaName() == SEABASE_REPOS_SCHEMA) &&
           (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      // IUD on hbase metadata is only allowed for internal queries.
      *CmpCommon::diags() << DgSqlCode(-1391)
			 <<  DgString0(naTable->getTableName().getQualifiedNameAsAnsiString())
                         << DgString1("repository");
      bindWA->setErrStatus();
      return this;
    }
  
  if ((naTable->isHbaseTable()) &&
      (naTable->isHbaseCellTable() || naTable->isHbaseRowTable()) &&
      (CmpCommon::getDefault(HBASE_NATIVE_IUD) == DF_OFF))
    {
      *CmpCommon::diags() << DgSqlCode(-4223)
			  << DgString0("Insert/Update/Delete on native HBase tables or in CELL/ROW format is");
      
      bindWA->setErrStatus();
      return this;
     }

  NABoolean insertFromValuesList =
   (getOperatorType() == REL_UNARY_INSERT &&
    (child(0)->getOperatorType() == REL_TUPLE ||  // VALUES(1,'b')
     child(0)->getOperatorType() == REL_TUPLE_LIST ||  // VALUES(1,'b'),(2,'Y')
     child(0)->getOperatorType() == REL_UNION)) ||  // VALUES..(with subquery inside the list)
    getOperatorType() == REL_LEAF_INSERT;          // index type of inserts

   if((!insertFromValuesList) && (getOperatorType() == REL_UNARY_INSERT))
    bindWA->setInsertSelectStatement(TRUE);

  // an update/delete node is created as an update/delete with child
  // of a scan node by parser. If this is the case, then no security
  // checks are needed on child Scan node.
  if ((getOperatorType() == REL_UNARY_UPDATE ||
       getOperatorType() == REL_UNARY_DELETE) &&
      (child(0) && (child(0)->getOperatorType() == REL_SCAN))) {
    Scan * scanNode = (Scan *)(child(0)->castToRelExpr());
    scanNode->setNoSecurityCheck(TRUE);
  }

  // Setting the begin index for TableViewUsageList to zero, instead
  // of the bindWA->tableViewUsageList().entries(); Becasue
  // bindWA->tableViewUsageList().entries() sets the index to the current
  //entry in the list, which excludes previous statements executed in a CS.
  CollIndex begSrcUsgIx = 0;
  if (!insertFromValuesList) {
    //
    // Create a new table name scope for the source table (child node).
    // Bind the source.
    // Reset scope context/naming.
    //
    bindWA->getCurrentScope()->xtnmStack()->createXTNM();
    bindChildren(bindWA);
    if (bindWA->errStatus()) return this;
    bindWA->getCurrentScope()->xtnmStack()->removeXTNM();

    // QSTUFF

    // we currently don't support streams and embedded updates
    // for "insert into select from" statements.
    if (getOperatorType() == REL_UNARY_INSERT){

      if (child(0)->getGroupAttr()->isStream()){
        *CmpCommon::diags() << DgSqlCode(-4170);
        bindWA->setErrStatus();
        return this;
      }

      if (child(0)->getGroupAttr()->isEmbeddedUpdateOrDelete() ||
          child(0)->getGroupAttr()->isEmbeddedInsert()){
        *CmpCommon::diags() << DgSqlCode(-4171)
			    << DgString0(getGroupAttr()->getOperationWithinGroup());
        bindWA->setErrStatus();
        return this;
      }
    }

    // binding a generic update within a generic update
    // can only occur when binding an updatable view containing
    // an embedded delete or embedded update. We don't continue
    // binding the generic update and but return the bound scan node.
    // the scan node may be either a base table scan or a RenameTable
    // node in case we are updating a view
    // Since an embedded generic update may have referred to the OLD
    // and NEW table we set a binder flag causing the table name to
    // be changed to the name of the underlying scan table in the
    // RelRoot on top of the generic update. Since we
    // know that the normalizer has checked before allowing an update
    // on the view that not both, i.e.new and old column values have been
    // referred this is a safe operation.

    if (returnScanNode){
      // this line is a hack to get through Update::bindNode on the return
      setTableDesc(getScanNode()->getTableDesc());

      bindWA->setInGenericUpdate(inGenericUpdate);
      bindWA->setRenameToScanTable (TRUE);
      NATable *nTable = bindWA->getNATable(getTableName());

      // decr once for just getting it here
      // and again to compensate for the reference taken out
      // previously which becomes obsolete since we just return a scan node
      nTable->decrReferenceCount();
      nTable->decrReferenceCount();

      return getViewScanNode();
    }
    // QSTUFF
  }
  else {
    // else, Insert::bindNode does VALUES(...) in its Assign::bindNode loop
    // in particular, it does      VALUES(..,DEFAULT,..)
  }

  #ifndef NDEBUG
    GU_DEBUG_Display(bindWA, this, "incoming", NULL, TRUE);
  #endif

  // QSTUFF
  // in case of an insert operation we don't set it initially in order
  // to prevent that an embedded update or delete may be accidentially
  // removed from a source view. However we need it for binding the
  // target because it may be a view and its embedded updates have to
  // be removed.

  if (getOperatorType() == REL_UNARY_INSERT)
    inGenericUpdate = bindWA->setInGenericUpdate(TRUE);

  CMPASSERT(NOT(updateCurrentOf() &&
                getGroupAttr()->isEmbeddedUpdateOrDelete()));

  // this is a patch to allow for embedded updates in view definitions
  ParNameLocList * pLoc = NULL;
  if (getGroupAttr()->isEmbeddedUpdate()) {
    pLoc =  bindWA->getNameLocListPtr();
    bindWA->setNameLocListPtr(NULL);
  }
  // QSTUFF

  // Allocate a TableDesc and attach it to the node.
  //
  // Note that for Update/Delete, which always have a Scan node attached
  // (see below), we cannot reuse the Scan's TableDesc:
  // GenMapTable.C doesn't find the proper ValueIds when processing an
  // update/delete on a table with an index.
  // So we must always create a new (target) TableDesc, always a base table.
  //
  // Note that bindWA->getCurrentScope()->setRETDesc() is implicitly called:
  // 1) by createTableDesc, setting it to this new (target) base table;
  // 2) by bindView (if called), resetting it to the view's RenameTable RETDesc
  //    atop the new (target) table.
  //
  const NATable *naTableTop = naTable;
  NABoolean isView = naTable->getViewText() != NULL;
  RelExpr *boundView = NULL;      // ## delete when done with it?
  Scan *scanNode = NULL;

  if (getOperatorType() == REL_UNARY_INSERT ||
      getOperatorType() == REL_LEAF_INSERT) {
    if (isView) {        // INSERT into a VIEW:
    //
    // Expand the view definition as if it were a Scan child of the Insert
    // (like all children, must have its own table name scope).
    //
    bindWA->getCurrentScope()->xtnmStack()->createXTNM();
    boundView = bindWA->bindView(getTableName(),
                                 naTable,
                                 accessOptions(),
                                 removeSelPredTree(),
                                 getGroupAttr());
    #ifndef NDEBUG
      GU_DEBUG_Display(bindWA, this, "bv1", boundView);
    #endif
    if (bindWA->errStatus()) return this;
    scanNode = boundView->getScanNode();
    bindWA->getCurrentScope()->xtnmStack()->removeXTNM();
    }
  }
  else if (getOperatorType() == REL_UNARY_UPDATE ||
           getOperatorType() == REL_UNARY_DELETE) {
    scanNode = getScanNode();
  }

  if (updateCurrentOf()) {
    CMPASSERT(scanNode);
    scanNode->bindUpdateCurrentOf(bindWA,
                                  (getOperatorType() == REL_UNARY_UPDATE));
    if (bindWA->errStatus()) return this;
  }

  // As previous comments indicated, we're creating a TableDesc for the target,
  // the underlying base table.  Here we go and do it:

  NABoolean isScanOnDifferentTable = FALSE;
  if (isView) {
    // This binding of the view sets up the target RETDesc.
    // This is the first bindView for UPDATE and DELETE on a view,
    // and the second for INSERT into a view (yes, we *do* need to do it again).
    boundView = bindWA->bindView(getTableName(),
                                 naTable,
                                 accessOptions(),
                                 removeSelPredTree(),
                                 getGroupAttr(),
                                 TRUE);   // QSTUFF
    setTableDesc(boundView->getScanNode()->getTableDesc());
    if ((getOperatorType() == REL_INSERT)||
        (getOperatorType() == REL_UNARY_INSERT) ||
        (getOperatorType() == REL_LEAF_INSERT))
    {
      ((Insert *)this)->setBoundView(boundView);
    }
    // for triggers
    if (scanNode)
      {
        const NATable *naTableLocal = scanNode->getTableDesc()->getNATable();
        if ((naTableLocal != naTable) && (naTableLocal->getSpecialType() == ExtendedQualName::TRIGTEMP_TABLE))
          isScanOnDifferentTable = TRUE;
      }
  } else if (NOT (getUpdateCKorUniqueIndexKey() && (getOperatorType() == REL_UNARY_INSERT))) {
    // an insert that is introduced to implement a phase of update primary key already 
    // has the right tabledesc (obtained from the update that it is replacing), so 
    // do not create another tablesdesc for such an insert.
    if (scanNode)
      naTable = scanNode->getTableDesc()->getNATable();
    CorrName tempName(naTableTop->getTableName(),
                      bindWA->wHeap(),
                      "",
                      getTableName().getLocationName(),
                      getTableName().getPrototype());
    
    tempName.setUgivenName(getTableName().getUgivenName());
    
    tempName.setSpecialType(getTableName().getSpecialType());
    //    tempName.setIsVolatile(getTableName().isVolatile());
    TableDesc * naTableToptableDesc = bindWA->createTableDesc(
                                                             naTableTop,
                                                             tempName);

    if(naTableToptableDesc)
    {
      naTableToptableDesc->setSelectivityHint(NULL);
      naTableToptableDesc->setCardinalityHint(NULL);
    }

    setTableDesc(naTableToptableDesc);

    // Now naTable has the Scan's table, and naTableTop has the GU's table.
    // Rather than compare naTable pointers we now compare the extended
    // qualified name contained in them. This name is the key to an natable
    // object in NATableDB and will enable us to tell if scan's table and 
    // GU's table are the same.
    isScanOnDifferentTable = (naTable->getExtendedQualName() != 
			      naTableTop->getExtendedQualName());
  }

  if (bindWA->errStatus())
    return this;


  // QSTUFF
  // in case of a delete or update we may have to bind set clauses.
  // first we bind the left target column, second we bind the right hand side
  // we also have to separate the set on rollback clauses in a separate
  // list. The set clauses generate a newRecExpr list, the set on rollback
  // clause generate a newRecBeforeExpr list.

  // we add the old to new valueid map as it allows us to generate
  // a subset operator in the presence of order by. the compiler
  // needs to understand that the old and new valueids are identical

  // inlined trigger may update and scan different tables
  if ((getOperatorType() == REL_UNARY_DELETE) && 
    (!isScanOnDifferentTable && !getUpdateCKorUniqueIndexKey())) {
    const ValueIdList &dkeys =
      getTableDesc()->getClusteringIndex()->getClusteringKeyCols();
    const ValueIdList &skeys =
      scanNode->getTableDesc()->getClusteringIndex()->getClusteringKeyCols();
    CollIndex j = skeys.entries();
    for (CollIndex i = 0; i < j; i++) {
      oldToNewMap().addMapEntry(skeys[i].getItemExpr()->getValueId(),
                                dkeys[i].getItemExpr()->getValueId());
    }
  }

  ItemExpr *recExpr = removeNewRecExprTree();

  if (recExpr &&
      (getOperatorType() == REL_UNARY_DELETE ||
       getOperatorType() == REL_UNARY_UPDATE)) {

    ItemExprList  recList(recExpr, bindWA->wHeap());
    ItemExprList  recBeforeList(bindWA->wHeap());
    SET(short)    stoiColumnSet(bindWA->wHeap());

    // in case a delete statement has a recEpxr, set on rollback
    // clauses have been defined and need to be bound

    // as part of binding any set on rollback clause we have check
    // that no contraints are defined for the specific clauses; otherwise
    // the statement is rejected.
    // the target columns are bound to the update table, the source
    // columns are bound to the scan table

    if (getOperatorType() == REL_UNARY_DELETE){
      recBeforeList.insert(recList);
      bindUpdateExpr(bindWA,recExpr,recBeforeList,boundView,scanNode,stoiColumnSet,TRUE);
      if (bindWA->errStatus()) return this;
    }

    // in case of an update operator we have to separate the set and
    // set on rollback clauses

    if (getOperatorType() == REL_UNARY_UPDATE) {
      CMPASSERT(recList.entries());

      NABoolean leftIsList      = FALSE;
      NABoolean rightIsList     = FALSE;
      NABoolean legalSubqUdfExpr = FALSE;

      for (CollIndex i = 0;i < recList.entries(); i++){

        CMPASSERT(recList[i]->getOperatorType() == ITM_ASSIGN);

        if (recList[i]->child(0)->getOperatorType() == ITM_ITEM_LIST)
          leftIsList = TRUE;
        if (recList[i]->child(1)->getOperatorType() == ITM_ITEM_LIST)
          rightIsList = TRUE;

        if (((Assign *)recList[i])->onRollback()){

          // On rollback clause currently not allowed with update lists.
          if ((leftIsList) || (rightIsList))
            {
              *CmpCommon::diags() << DgSqlCode(-3242) 
                                  << DgString0(" ON ROLLBACK not supported with SET lists.");
              bindWA->setErrStatus();
              return this;
            }
          //          CMPASSERT((NOT leftIsList) && (NOT rightIsList))
          
          recBeforeList.insert(recList[i]);
          recList.removeAt(i);
          i--;
        }
      }
      

      if ((leftIsList) &&
          (NOT rightIsList) && 
          (recList.entries() == 1) &&
          ((recList[0]->child(1)->getOperatorType() == ITM_ROW_SUBQUERY) ||
          (recList[0]->child(1)->getOperatorType() == ITM_USER_DEF_FUNCTION)))
        {
          ItemExpr   * expr = NULL;

          // Both Subqueries and UDFs are now using the ValueIdProxy
          // to carry the each of the valueIds representing the select list
          // or UDF outputs. The transformation of the ValueIdProxy will do the
          // right thing, and we don't need setSubqInUpdateAssing() anymore.

          // Bind the subquery
          if (recList[0]->child(1)->getOperatorType() == ITM_ROW_SUBQUERY) 
          {
             RowSubquery * rs = 
               (RowSubquery*)(recList[0]->child(1)->castToItemExpr());

             // Not sure that we ever have a subquery without a REL_ROOT
             // left this additional check from the old code.
             if (rs->getSubquery()->getOperatorType() == REL_ROOT)
             {
                rs = (RowSubquery *) rs->bindNode(bindWA);
                if (bindWA->errStatus())
                  return this;

                legalSubqUdfExpr = TRUE;
                expr = (ItemExpr *) rs;
             }
          
          }
          else
          {
             UDFunction * rudf = 
               (UDFunction*)(recList[0]->child(1)->castToItemExpr());

             //  Need to bind the UDFunction to get its outputs.
             rudf = (UDFunction *) rudf->bindNode(bindWA);
             if (bindWA->errStatus())
               return this;

             legalSubqUdfExpr = TRUE;
             expr = (ItemExpr *) rudf;

          }

          // Update the recList with the bound itemExpr
          recList[0]->child(1) = expr;


          // Use the ItemExprList to flatten the Subquery or UDF
          ItemExprList *exprList = (ItemExprList *) new(bindWA->wHeap())
                                  ItemExprList(expr,bindWA->wHeap());

          // Convert the ItemExprList to a Tree
          ItemExpr * ie = exprList->convertToItemExpr();
          ie = ie->bindNode(bindWA);
          if (bindWA->errStatus())
            return this;
  
          Assign * assignNode = (Assign *)recList[0];
          assignNode->child(1) = ie;

          rightIsList = TRUE;
        }

      if ((leftIsList) || (rightIsList)) // some elements as lists
        {
          ItemExprList  newRecList(bindWA->wHeap());
          for (CollIndex i = 0; i < recList.entries(); i++)
            {
              Assign * assignNode = (Assign *)recList[i];

                // Need to bind any UDFs or Subqieries in the expression 
                // so that we know the degree before we expand the list.
              assignNode->child(0) = 
                          assignNode->child(0)->bindUDFsOrSubqueries(bindWA); 
              if (bindWA->errStatus())
                return this;

                // Need to bind any UDFs or Subqieries in the expression 
                // so that we know the degree before we expand the list.
              assignNode->child(1) = 
                          assignNode->child(1)->bindUDFsOrSubqueries(bindWA); 
              if (bindWA->errStatus())
                return this;

              ItemExprList leftList(assignNode->child(0), bindWA->wHeap());
              ItemExprList rightList(assignNode->child(1), bindWA->wHeap());
              Lng32 numLeftElements = (Lng32) leftList.entries();
              Lng32 numRightElements = (Lng32) rightList.entries();

              // See if ALLOW_SUBQ_IN_SET is enabled. It is enabled if
              // the default is ON, or if the default is SYSTEM and
              // ALLOW_UDF is ON.
              NABoolean allowSubqInSet_Enabled = FALSE;
              DefaultToken allowSubqTok =
                CmpCommon::getDefault(ALLOW_SUBQ_IN_SET);
              if ((allowSubqTok == DF_ON) ||
                  (allowSubqTok == DF_SYSTEM))
                allowSubqInSet_Enabled = TRUE;

              if (!allowSubqInSet_Enabled)
                 {
                   for (CollIndex j = 0; j < rightList.entries(); j++)
                     {
                       if (((numLeftElements > 1) ||
                           (numRightElements > 1)) &&
                           (((rightList[j]->getOperatorType() == ITM_ROW_SUBQUERY) ||
                             (rightList[j]->getOperatorType() == ITM_VALUEID_PROXY)) &&
                             (legalSubqUdfExpr == FALSE)))
                         {
                           *CmpCommon::diags() << DgSqlCode(-3242) 
                             << DgString0(" Multiple elements or multiple subqueries are not allowed in this SET clause.");    
                           bindWA->setErrStatus();
                           return this;
                         }
                     }
                 }

              if (numLeftElements != numRightElements)
                {
                  *CmpCommon::diags() << DgSqlCode(-4023)
                    << DgInt0(numRightElements) 
                    << DgInt1(numLeftElements);
                  bindWA->setErrStatus();
                  return this;
                }
              
              // create newRecList with one Assign node for each element.
              for (CollIndex k = 0; k < leftList.entries(); k++)
                {
                  ItemExpr * leftIE = leftList[k];
                  ItemExpr * rightIE = rightList[k];
                  Assign *assign = new (bindWA->wHeap())
                    Assign(leftIE, rightIE);
                  // We do not bind the above Assign as it will be done
                  // in bindUpdateExpr below. (bug #1893)
                  newRecList.insert(assign);
                }

            } // for

	  bindUpdateExpr(bindWA,recExpr,newRecList,boundView,scanNode,stoiColumnSet);
	  if (bindWA->errStatus()) 
	    return this;
	} // some elements as lists
      else
	{ // no elements as lists
	  if (recList.entries()){
	    bindUpdateExpr(bindWA,recExpr,recList,boundView,scanNode,stoiColumnSet);
	    if (bindWA->errStatus()) return this;
	  }
	}

      if (recBeforeList.entries()){
      bindUpdateExpr(bindWA,recExpr,recBeforeList,boundView,scanNode,stoiColumnSet,TRUE);
      if (bindWA->errStatus()) return this;
      }

    } // UNARY_UPDATE

  // now we record the columns updated for the SqlTableOpenInfo
  if (listedStoi) {
    listedStoi->getStoi()->setColumnListCount((short)stoiColumnSet.entries());

    short *stoiColumnList = new (bindWA->wHeap())
      short[stoiColumnSet.entries()];

    for (CollIndex i = 0; i < stoiColumnSet.entries(); i++)
    {
      stoiColumnList[i] = stoiColumnSet[i];
      listedStoi->addUpdateColumn(stoiColumnSet[i]);
    }

    listedStoi->getStoi()->setColumnList(stoiColumnList);
  }

    // the previous implementation assumed that the scope points
    // to the scan table; we don't want to disturb the code and
    // make that happen --

    #ifndef NDEBUG
      GU_DEBUG_Display(bindWA, this, "u");
    #endif

    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
  }
  // QSTUFFF

  CollIndex endSrcUsgIx = bindWA->tableViewUsageList().entries();

  if ((!isScanOnDifferentTable) &&
      (((getOperatorType() == REL_UNARY_INSERT) &&
       !insertFromValuesList && !getGroupAttr()->isEmbeddedInsert()) ||
      (getOperatorType() == REL_UNARY_UPDATE) ||
      (getOperatorType() == REL_UNARY_DELETE))){

    // Special handling of statements that could suffer the 
    // Halloween problem, e.g., "insert into t select from t"
    // or "insert into v select from t", if v references t

    DBG( if (getenv("TVUSG_DEBUG")) bindWA->tableViewUsageList().display(); )

    const NATable *naTableBase = naTable;
    const QualifiedName *viewName = NULL;
    if (isView) {
      // Currently, per Ansi rules, we can only insert through a view if
      // there is a single underlying base table without joins or unions.
      // Since we are binding the view twice for INSERTS,
      // the variable beforeRefcount for the *single* base table has to be 2.
      //
      beforeRefcount = beforeRefcount + 1;
      naTableBase = getTableDesc()->getNATable();
      viewName = &naTable->getTableName();
    }
    if ((getOperatorType() == REL_UNARY_UPDATE ||
         getOperatorType() == REL_UNARY_DELETE) &&
        (child(0)->getOperatorType() == REL_SCAN)) {
      // The table is referenced twice; once for the update/delete and
      // the second time for the scan below it.
      beforeRefcount = beforeRefcount + 1;
    }

    const QualifiedName &tableBaseName = naTableBase->getTableName();
    Int32 afterRefcount =  naTableBase->getReferenceCount();
    NABoolean isSGTableType = getTableName().getSpecialType() == ExtendedQualName::SG_TABLE;

    NAString viewFmtdList(bindWA->wHeap());
    Int32 baseSeenInSrc = 0;
    
    // The views on the table do not need to be obtained
    // if the table type is a SEQUENCE GENERATOR

    if (!isSGTableType)
      baseSeenInSrc = bindWA->tableViewUsageList().getViewsOnTable(
                               begSrcUsgIx, endSrcUsgIx,
                               bindWA->viewCount(),
                               tableBaseName,
                               getTableName().getSpecialType(),
                               viewName,
                               viewFmtdList);

    NABoolean halloween = FALSE;
    if (CmpCommon::getDefault(R2_HALLOWEEN_SUPPORT) == DF_ON) {

      if (beforeRefcount != afterRefcount) {

          // Check to see if we can support this update.
          //
          if(checkForHalloweenR2(afterRefcount - beforeRefcount)) {
            halloween = TRUE;
          }
      }
      else {
        Scan *scanSrc = getScanNode(FALSE/*no assert*/);
        if ((baseSeenInSrc > beforeRefcount) &&
            ((scanSrc && scanSrc->getTableName().isLocationNameSpecified())||
             (getTableName().isLocationNameSpecified()))) {
          halloween = TRUE;
        }
        if (Get_SqlParser_Flags(ALLOW_SPECIALTABLETYPE)) {
          if ((scanSrc && scanSrc->getTableName().isLocationNameSpecified())||
              (getTableName().isLocationNameSpecified())){
            // Do not enforce Halloween check if it is a
            // partition only operation.
            // We assume the programmer knows what he's doing
            // -- hopefully, by doing insert/update/delete
            // operations as part of Partition Management
            // (Move Partition Boundary or Split Partition or
            // Merge Partition. See TEST057 and TEST058)
            halloween = FALSE;
          }
        }
      }
      if (halloween) {
        CMPASSERT(!(isView && viewFmtdList.isNull()));
        *CmpCommon::diags() << DgSqlCode(viewFmtdList.isNull() ? -4026 : -4060)
                            << DgTableName(
                                  tableBaseName.getQualifiedNameAsAnsiString())
                            << DgString0(viewFmtdList);
        bindWA->setErrStatus();
        return this;
      }
    } 
    else {
      // Support for self-referencing updates/Halloween problem.
      if (beforeRefcount != afterRefcount) {

        setAvoidHalloween(TRUE);

        bindWA->getTopRoot()->setAvoidHalloween(TRUE);

        // Decide if access mode (default or specified) is compatible
        // with the use of DP2 locks.  If access mode was specified,
        // it is a property of the naTableBase.
        NABoolean cannotUseDP2Locks = 
             naTableBase->getRefsIncompatibleDP2Halloween();

        // Now check the transaction isolation level, which can override
        // the access mode.  Note that il was initialized above for the
        // check for an updatable trans, i.e., errors 3140 and 3141.
        if((CmpCommon::transMode()->ILtoAT(il) == TransMode::REPEATABLE_READ_ACCESS_ ) ||
           (CmpCommon::transMode()->ILtoAT(il) == TransMode::READ_COMMITTED_ACCESS_     ) ||
           (CmpCommon::transMode()->ILtoAT(il) == TransMode::READ_UNCOMMITTED_ACCESS_     )) 
           cannotUseDP2Locks = TRUE;

        // Save the result with this GenericUpdate object.  It will be 
        // used when the nextSubstitute  methods of TSJFlowRule or TSJRule 
        // call GenericUpdate::configTSJforHalloween.
        if (NOT getHalloweenCannotUseDP2Locks())
          setHalloweenCannotUseDP2Locks(cannotUseDP2Locks);

        // Keep track of which table in the query is the self-ref table.
        // This is a part of the fix for solution 10-071204-9253.
        ((NATable *)naTableBase)->setIsHalloweenTable();
      }
      else {
        Scan *scanSrc = getScanNode(FALSE/*no assert*/);
        if ((baseSeenInSrc > beforeRefcount) &&
            ((scanSrc && scanSrc->getTableName().isLocationNameSpecified())||
             (getTableName().isLocationNameSpecified()))) {
          halloween = TRUE;
        }
        if (Get_SqlParser_Flags(ALLOW_SPECIALTABLETYPE)) {
          if ((scanSrc && scanSrc->getTableName().isLocationNameSpecified())||
              (getTableName().isLocationNameSpecified())){
            // Do not enforce Halloween check if it is a
            // partition only operation.
            // We assume the programmer knows what he's doing
            // -- hopefully, by doing insert/update/delete
            // operations as part of Partition Management
            // (Move Partition Boundary or Split Partition or
            // Merge Partition. See TEST057 and TEST058)
            halloween = FALSE;
          }
        }
        if (halloween) {
          CMPASSERT(!(isView && viewFmtdList.isNull()));
          *CmpCommon::diags() << DgSqlCode(viewFmtdList.isNull() ? -4026 : -4060)
                              << DgTableName(
                                  tableBaseName.getQualifiedNameAsAnsiString())
                              << DgString0(viewFmtdList);
          bindWA->setErrStatus();
          return this;
        }
      }
    }
  }

  // Bind the base class.
  // Allocate an empty RETDesc and attach it to this node, *but* leave the
  // currently scoped RETDesc (that of naTableTop) as is, for further binding
  // in caller Insert::bindNode or LeafInsert/LeafDelete::bindNode.
  //
  RelExpr *boundExpr = bindSelf(bindWA);
  CMPASSERT(boundExpr == this);  // assumed by RETDesc/RI/IM code below
  if (bindWA->errStatus()) return boundExpr;
  setRETDesc(new (bindWA->wHeap()) RETDesc(bindWA));

  // Copy the check constraints to the private memory of the GenericUpdate.
  //
  checkConstraints() = getTableDesc()->getCheckConstraints();

  // Create a key expression for the table to be updated.
  // The code specific to the Insert node is handled in Insert::bindNode.
  //
  if (getOperatorType() == REL_UNARY_UPDATE ||
      getOperatorType() == REL_UNARY_DELETE) {

    if (getTableDesc()->getNATable()->isHiveTable())
      {
        *CmpCommon::diags() << DgSqlCode(-4223)
                            << DgString0("Update/Delete on Hive table is");
        bindWA->setErrStatus();
        return this;
      }

    // SQL syntax requires (and the parser ensures) that a direct descendant
    // (passing thru views) of an update/delete node is a scan node on the
    // same table that is being updated (note that normalizer transformations
    // may change this at a later time).
    // An exception to this rule happens when before triggers are inlined.
    // In this case, the update/delete on the subject table is driven by
    // a Scan on a temp table. The primary key columns of the subject table are
    // a subset of the primary key columns of the temp table, and using the
    // same column names, but not neccessarily in the same order.
    //
    // Update/Delete nodes require expressions in their newRecExpr that can
    // be used to form the primary key of the table to update/delete.
    //
    const NAColumnArray &keyColArray =
      getTableDesc()->getNATable()->getClusteringIndex()->getIndexKeyColumns();
    CollIndex numKeyCols = keyColArray.entries();
    const NAColumnArray &scanColArray =
    scanNode->getTableDesc()->getNATable()->getNAColumnArray();

    for (CollIndex i = 0; i < numKeyCols; i++) {
      // The scan node and the update/delete node both use the SAME table,
      // so their column names are also the same.
      //
      Lng32 colPos = keyColArray[i]->getPosition();
      ItemExpr *guCol = getTableDesc()->getColumnList()[colPos].getItemExpr();
      ItemExpr *scanCol;  // - Triggers
      if (!isScanOnDifferentTable)
        scanCol = scanNode->getTableDesc()->getColumnList()[colPos].getItemExpr();
      else
      {
        // Make sure this is a BaseColumn.
        CMPASSERT(guCol->getOperatorType() == ITM_BASECOLUMN);
        // Find the column name.
        const NAString& colName = ((BaseColumn *)guCol)->getColName();
        // Find a column with the same name, in the table from the Scan node.
        // SYSKEY is an exception since its name in the temp table is "@SYSKEY"
        ExtendedQualName::SpecialTableType tableType =
          scanNode->getTableDesc()->getCorrNameObj().getSpecialType();
        NAColumn *scanNaCol = NULL;
        if (ExtendedQualName::TRIGTEMP_TABLE == tableType && colName == "SYSKEY")
        {
          scanNaCol = scanColArray.getColumn("@SYSKEY");
        }
        else
        {
          scanNaCol = scanColArray.getColumn(colName);
        }
        CMPASSERT(scanNaCol != NULL)
        // Get the position of this column in the Scan table.
        Lng32 scanColPos = scanNaCol->getPosition();
        // Get the Scan BaseColumn.
        scanCol = scanNode->getTableDesc()->getColumnList()[scanColPos].getItemExpr();
      }
      ItemExpr *newKeyPred = new (bindWA->wHeap())
        BiRelat(ITM_EQUAL, guCol, scanCol);
      newKeyPred->bindNode(bindWA);
      beginKeyPred().insert(newKeyPred->getValueId());

      updateToSelectMap().addMapEntry(
           newKeyPred->child(0)->getValueId(),
           newKeyPred->child(1)->getValueId());

    } // loop over key columns

    // All of the indexes also require expressions that can be used to
    // form the primary key of the index to update/delete. Create these
    // item expressions here.
    // (From here to the end of the loop over indexes structurally resembles
    // GenericUpdate::imBindAllIndexes(), but has significant differences.)
    //
    // Remember the value ID's of the scan node index columns for
    // code generation time.
    //

    if ((this->getOperatorType() == REL_UNARY_UPDATE) && isScanOnDifferentTable)
    {
      setScanIndexDesc(NULL);  // for triggers
    }
    else
    {
      setScanIndexDesc(scanNode->getTableDesc()->getClusteringIndex());
    }
  } // REL_UNARY_UPDATE or REL_UNARY_DELETE


  // QSTUFF
  // we need to check whether this code is executed as part of a create view
  // ddl operation using bindWA->inDDL() and prevent indices, contraints and
  // triggers to be added as the catalog manager binding functions cannot
  // handle it right now
  // QSTUFF

  // QSTUFF hack !
  if (getGroupAttr()->isEmbeddedUpdate())
    bindWA->setNameLocListPtr(pLoc);
  bindWA->setInGenericUpdate(inGenericUpdate);
  // QSTUFF

  // set flag that we are binding an Insert/Update/Delete operation
  // Used to disable Join optimization when necessary
  bindWA->setBindingIUD();

  return boundExpr;
} // GenericUpdate::bindNode()

NABoolean GenericUpdate::checkForMergeRestrictions(BindWA *bindWA)
{
  if (!isMerge())
    return FALSE;


  ValueIdList tempVIDlist;
  getTableDesc()->getIdentityColumn(tempVIDlist);
  NAColumn *identityCol = NULL;
  if (tempVIDlist.entries() > 0)
  {
    ValueId valId = tempVIDlist[0];
    identityCol = valId.getNAColumn();
  }

  // MERGE on a table with BLOB columns is not supported
  if (getTableDesc()->getNATable()->hasLobColumn())
  {
    *CmpCommon::diags() << DgSqlCode(-3241) 
                        << DgString0(" LOB column not allowed.");
    bindWA->setErrStatus();
    return TRUE;

  }
  
  if (getTableDesc()->hasUniqueIndexes() && 
      (CmpCommon::getDefault(MERGE_WITH_UNIQUE_INDEX) == DF_OFF))
  {
    *CmpCommon::diags() << DgSqlCode(-3241) 
			<< DgString0(" unique indexes not allowed.");
    bindWA->setErrStatus();
    return TRUE;
  }
  
  if ((accessOptions().accessType() == TransMode::SKIP_CONFLICT_ACCESS_) ||
      (getGroupAttr()->isStream()) ||
      (newRecBeforeExprArray().entries() > 0)) // set on rollback
  {
    *CmpCommon::diags() << DgSqlCode(-3241)
                        << DgString0(" Stream, skip conflict or SET ON ROLLBACK not allowed.");
    bindWA->setErrStatus();
    return TRUE;
  }

  if (getGroupAttr()->isEmbeddedUpdateOrDelete())
  {
    *CmpCommon::diags() << DgSqlCode(-3241)
                        << DgString0(" Embedded update/deletes not allowed.");
    bindWA->setErrStatus();
    return TRUE;
  }

  if ((getInliningInfo().hasInlinedActions()) ||
      (getInliningInfo().isEffectiveGU()))
  {
    if (getInliningInfo().hasTriggers()) 
    {
      *CmpCommon::diags() << DgSqlCode(-3241)
                          << DgString0(" Triggers not allowed.");
      bindWA->setErrStatus();
      return TRUE;
    }
  }

  return FALSE;
}

// This class LeafInsert and its companion LeafDelete
// are currently used only by Index Maintenance,
// but we ought not make any assumptions.
// ##IM: It might be useful to add a flag such as GenericUpdate::isIndexTable_
// ##IM: and set it to TRUE in createIMNode().
//
RelExpr *LeafInsert::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  #ifndef NDEBUG
    if (GU_DEBUG) cerr << "\nLeafInsert " << getUpdTableNameText() << endl;
  #endif

  setInUpdateOrInsert(bindWA, this, REL_INSERT);

  if (getPreconditionTree()) {
    ValueIdSet pc;
    
    getPreconditionTree()->convertToValueIdSet(pc, bindWA, ITM_AND);
    if (bindWA->errStatus())
      return this;
    
    setPreconditionTree(NULL);
    setPrecondition(pc);
  }

  RelExpr *boundExpr = GenericUpdate::bindNode(bindWA);
  if (bindWA->errStatus()) return boundExpr;

  // Make newRecExprArray_ be an ordered set of assign nodes of the form
  //   "ixcol1 = basetblcol1, ixcol2 = basecol2, ..." (for Index Maintenance)

  // Note: For SQL/MP tables, ixcol0 is the keytag, and will need to be
  // handled differently from other columns.


  const ValueIdList &tgtcols = getTableDesc()->getColumnList();

  CMPASSERT(tgtcols.entries() == baseColRefs().entries());
  for (CollIndex i = 0; i < tgtcols.entries(); i++) {
    Assign *assign;
    assign = new (bindWA->wHeap())
      Assign(tgtcols[i].getItemExpr(), baseColRefs()[i], FALSE);
    assign->bindNode(bindWA);
    if (bindWA->errStatus()) return NULL;
    newRecExprArray().insertAt(i, assign->getValueId());
    newRecExpr().insert(assign->getValueId());
    updateToSelectMap().addMapEntry(assign->getTarget(), assign->getSource());
  }

  if (getReferencedMergeIUDIndicator() != NULL_VALUE_ID)
    bindWA->getCurrentScope()->addOuterRef(getReferencedMergeIUDIndicator());
  // RelExpr::bindSelf (in GenericUpdate::bindNode) has done this line, but now
  // any outer refs discovered in bindNode's in the above loop must be added.
  // For Index Maintenance, these must be exactly the set of baseColRefs vids
  // (all the target index cols are from the locally-scoped RETDesc left by
  // the GenericUpdate::bindNode), plus the merge IUD indicator, if used.
  getGroupAttr()->addCharacteristicInputs(bindWA->getCurrentScope()->getOuterRefs());

  // The NATable of getTableName() had been set to INDEX_TABLE so that
  // getNATable would search the right namespace.
  // Now we make the Optimizer treat this as a regular table, not an index
  // (in particular, don't have it choose VSBB sidetree-insert).
  //
  // The TableDesc setting may be redundant/unnecessary, but we do it
  // for completeness and safety.
  //
  // -- Triggers
  // If it is NOT an index table (like maybe a TRIGTEMP_TABLE), leave it alone
  if (getTableName().getSpecialType() == ExtendedQualName::INDEX_TABLE)
  {
    getTableName().setSpecialType(ExtendedQualName::NORMAL_TABLE);
    getTableDesc()->getCorrNameObj().setSpecialType(ExtendedQualName::NORMAL_TABLE);
  }

  setInUpdateOrInsert(bindWA);
  return boundExpr;

} // LeafInsert::bindNode()

RelExpr *LeafDelete::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  #ifndef NDEBUG
    if (GU_DEBUG) cerr << "\nLeafDelete " << getUpdTableNameText() << endl;
  #endif

  if (getPreconditionTree()) {
    ValueIdSet pc;

    getPreconditionTree()->convertToValueIdSet(pc, bindWA, ITM_AND);
    if (bindWA->errStatus())
      return this;

    setPreconditionTree(NULL);
    setPrecondition(pc);
  }

  RelExpr *boundExpr = GenericUpdate::bindNode(bindWA);
  if (bindWA->errStatus()) return boundExpr;

  //Set the beginKeyPred
  if (TriggersTempTable *tempTableObj = getTrigTemp())
    {      
     
      const ValueIdList &keycols = getTableDesc()->getClusteringIndex()->getIndexKey();
      ItemExpr *keyExpr;
  
      // Normal case - use the UniqueExecuteId builtin function.
      keyExpr = new(bindWA->wHeap()) UniqueExecuteId();
      
      ItemExpr  *tempKeyPred = new(bindWA->wHeap()) BiRelat(ITM_EQUAL, keycols[0].getItemExpr(), keyExpr);     
      tempKeyPred->bindNode(bindWA);
      if (bindWA->errStatus()) return NULL;
      beginKeyPred().insert(tempKeyPred->getValueId());
      // Create the ItemExpr for the constant UniqueIudNum 
      ItemExpr *col2 = new(bindWA->wHeap()) 
             ColReference(new(bindWA->wHeap()) ColRefName(UNIQUEIUD_COLUMN));

      // Compare it to the correct offset.
      BindWA::uniqueIudNumOffset offset = BindWA::uniqueIudNumForInsert ; 
       
      ItemExpr *iudConst = new(bindWA->wHeap()) ConstValue(bindWA->getUniqueIudNum(offset));
      ItemExpr *predIudId = new(bindWA->wHeap()) BiRelat(ITM_EQUAL, keycols[1].getItemExpr(), iudConst);
      predIudId->bindNode(bindWA);
      if (bindWA->errStatus()) return NULL;
      beginKeyPred().insert(predIudId->getValueId());
      
      
       for (CollIndex i = 2; i<keycols.entries(); i++)
         {
           ItemExpr *keyPred = NULL;
           ItemExpr *keyItemExpr = keycols[i].getItemExpr();
           ItemExpr *baseItemExpr = NULL;
           Lng32 keyColPos = keycols[i].getNAColumn()->getPosition();
           baseItemExpr = baseColRefs()[keyColPos];
           keyPred = new (bindWA->wHeap())
             BiRelat(ITM_EQUAL, keyItemExpr, baseItemExpr);
         
           keyPred->bindNode(bindWA);
           if (bindWA->errStatus()) return NULL;
           beginKeyPred().insert(keyPred->getValueId());
         }
    }
    
  else
    {
    
  
  const ValueIdList &keycols = getTableDesc()->getClusteringIndex()->getIndexKey();

  for (CollIndex i = 0; i < keycols.entries() ; i++) 
    {
    ItemExpr *keyPred = 0;
    ItemExpr *keyItemExpr = keycols[i].getItemExpr();
    Lng32 keyColPos = keycols[i].getNAColumn()->getPosition();
    
    ItemExpr *baseItemExpr = NULL;
    // For a unique index (for undo) we are passing in all the index
    // columns in baseColRefs. So we need to find the index key col 
    // position in the index col list and compare the key columns with
    // it's corresponding column in the index column list
    if (isUndoUniqueIndex())
      baseItemExpr = baseColRefs()[keyColPos];
    else
      baseItemExpr = baseColRefs()[i];
    
    keyPred = new (bindWA->wHeap())
      BiRelat(ITM_EQUAL, keyItemExpr, baseItemExpr);

    keyPred->bindNode(bindWA);
    if (bindWA->errStatus()) return NULL;
    beginKeyPred().insert(keyPred->getValueId());
  }
    }
 

  if (isUndoUniqueIndex())
    {
      setUpExecPredForUndoUniqueIndex(bindWA) ;
    }
 
  if (getTrigTemp())
    {
      setUpExecPredForUndoTempTable(bindWA);
    }

  // See LeafInsert::bindNode for comments on remainder of this method.

  if (getReferencedMergeIUDIndicator() != NULL_VALUE_ID)
    bindWA->getCurrentScope()->addOuterRef(getReferencedMergeIUDIndicator());
  getGroupAttr()->addCharacteristicInputs(bindWA->getCurrentScope()->getOuterRefs());

  getTableName().setSpecialType(ExtendedQualName::NORMAL_TABLE);
  getTableDesc()->getCorrNameObj().setSpecialType(ExtendedQualName::NORMAL_TABLE);

  return boundExpr;

} // LeafDelete::bindNode()

void LeafDelete::setUpExecPredForUndoUniqueIndex(BindWA *bindWA)
{
  // Set up the executor predicate . Used in the case of Undo to undo the
  // exact row that caused an error.Note that if we used only the key 
  // columns  to undo, we may end up undoing existing rows . 
  // This is done only for unique indexes 
  ItemExpr *execPred  = NULL;


  const ValueIdList &indexCols = getTableDesc()->getClusteringIndex()->getIndexColumns();
  for ( CollIndex i = 0; i < indexCols.entries(); i++)
    {  
      execPred = new (bindWA->wHeap())
        BiRelat(ITM_EQUAL, indexCols[i].getItemExpr(), baseColRefs()[i]);
      execPred->bindNode(bindWA);
      if (bindWA->errStatus()) return ;
        executorPred() += execPred->getValueId();
    }
  return;
}
void LeafDelete::setUpExecPredForUndoTempTable(BindWA *bindWA)
{

ItemExpr *execPred  = NULL;
 
  const ValueIdList &tempCols = getTableDesc()->getClusteringIndex()->getIndexColumns();
  for ( CollIndex i = 0; i < tempCols.entries(); i++)
    {
      NAString colName(tempCols[i].getNAColumn()->getColName());
      if (colName.data()[0] == '@' && colName.compareTo("@SYSKEY"))
        continue;
      execPred = new (bindWA->wHeap())
        BiRelat(ITM_EQUAL, tempCols[i].getItemExpr(), baseColRefs()[i]);
      execPred->bindNode(bindWA);
      if (bindWA->errStatus()) return;
      executorPred() += execPred->getValueId();
    }

  return;
}

// -----------------------------------------------------------------------
// RelRoutine
// -----------------------------------------------------------------------
RelExpr *RelRoutine::bindNode(BindWA *bindWA)
{
  CMPASSERT(0); // For the time being, all classes above implement their own.

  //
  // Allocate an RETDesc and attach it to this and the BindScope.
  // Needs to occur in later classes when we know if we are at table
  // type or not..
  // XXX setRETDesc(new (bindWA->wHeap()) RETDesc(bindWA, getTableDesc()));
  // bindWA->getCurrentScope()->setRETDesc(getRETDesc());
  //
  // Bind the base class.
  //
  RelExpr *boundExpr = bindSelf(bindWA);
  if (bindWA->errStatus()) return boundExpr;
  //
  // Assign the set of columns that belong to the virtual table
  // as the output values that can be produced by this node.
  //
  //  XXX done in later clasees 
  //  getGroupAttr()->addCharacteristicOutputs(getTableDesc()->getColumnList());
  return boundExpr;
} // RelRoutine::bindNode()

// -----------------------------------------------------------------------
// BuiltinTableValuedFunction
// will be called by 
// ExplainFunc and StatisticsFunc
//   Rely on function implementation in TableValuedFunction 
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// Explain/Statistics/HiveMD Func
// -----------------------------------------------------------------------
RelExpr *BuiltinTableValuedFunction::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  //
  // Bind the child nodes.
  //
  bindChildren(bindWA);
  if (bindWA->errStatus())
    return this;

  //
  // Check if there is already an NATable for the Explain/Statistics table.
  //
  if (getOperatorType() == REL_EXPLAIN ||
      getOperatorType() == REL_STATISTICS  ||
      getOperatorType() == REL_HIVEMD_ACCESS ||
      getOperatorType() == REL_HBASE_ACCESS)
  {
    NATable *naTable =  NULL;

    if (getOperatorType() == REL_HBASE_ACCESS)
      {
	// should not reach here
	CMPASSERT(0); 
      }
    else
      {
	CorrName corrName(getVirtualTableName());
	corrName.setSpecialType(ExtendedQualName::VIRTUAL_TABLE);
	NATable *naTable = bindWA->getSchemaDB()->getNATableDB()->
	  get(&corrName.getExtendedQualNameObj());
	
	if (NOT naTable) 
	  {
	    TrafDesc *tableDesc = createVirtualTableDesc();
	    if (tableDesc)
	      naTable = bindWA->getNATable(corrName, FALSE/*catmanUsages*/, tableDesc);
	    
	    if ( ! tableDesc || bindWA->errStatus() )
	      return this;
	  }
	
	// Allocate a TableDesc and attach it to this.
	//
	TableDesc * td = bindWA->createTableDesc(naTable, corrName);
	if (! td || bindWA->errStatus())
	  return this;
	
	setTableDesc(td);
	if (bindWA->errStatus())
	  return this;
      }

    if (getProcAllParamsTree()) 
      {
	((ItemExpr *)getProcAllParamsTree())->convertToValueIdList(getProcAllParamsVids(), bindWA, ITM_ITEM_LIST);
	if (bindWA->errStatus())
	  return this;
	
	// Clear the Tree since we now have gotten vids for all the parameters.
	setProcAllParamsTree(NULL);
	
	Lng32 sqlcode = 0;
	if (getProcAllParamsVids().entries() != numParams()) 
	  {
	    sqlcode = -4067;
	    
	    // 4067 Explain/Statistics requires two operands, of type character.
	    *CmpCommon::diags() << DgSqlCode(sqlcode) << DgString0(getTextForError());
	    bindWA->setErrStatus();
	    return NULL;
	  }
	
	// type any param arguments to fixed char since runtime explain
	// expects arguments to be fixed char.
	Lng32 len = (Lng32)CmpCommon::getDefaultNumeric(VARCHAR_PARAM_DEFAULT_SIZE);
	SQLChar c(NULL, len);
	
	for (Lng32 i = 0; i < numParams(); i++)
	  {
	    getProcAllParamsVids()[i].coerceType(c, NA_CHARACTER_TYPE);
	    if (getProcAllParamsVids()[i].getType().getTypeQualifier() != NA_CHARACTER_TYPE)
	      {
		sqlcode = -4067;
		
		// 4067 Explain/Statistics requires two operands, of type character.
		*CmpCommon::diags() << DgSqlCode(sqlcode) << DgString0(getTextForError());
		bindWA->setErrStatus();
		return NULL;
	      }
	    
	    const NAType &typ = getProcAllParamsVids()[i].getType();
	    
	    CharInfo::CharSet chld_cs = ((const CharType&)typ).getCharSet();
	    ItemExpr *ie;
	    if ( chld_cs == CharInfo::UNICODE )
	      {
		ie = new (bindWA->wHeap()) Translate(
						     getProcAllParamsVids()[i].getItemExpr(),
						     Translate::UNICODE_TO_ISO88591);
		ie = ie->bindNode(bindWA);
		getProcAllParamsVids()[i] = ie->getValueId();
	      }
	    
	    if (bindWA->errStatus())
	      return NULL;
	    
	    // For Explain and Statistics all parameters are inputs
	    getProcInputParamsVids().insert(getProcAllParamsVids());
	  } // for
      }
  } // if
  
  return TableValuedFunction::bindNode(bindWA);
    
}

// -----------------------------------------------------------------------
// TableValuedFunction
// -----------------------------------------------------------------------
RelExpr *TableValuedFunction::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  //
  // Bind the child nodes.
  //
  bindChildren(bindWA);
  if (bindWA->errStatus())
    return this;


  //
  // Allocate an RETDesc and attach it to this and the BindScope.
  //
  setRETDesc(new (bindWA->wHeap()) RETDesc(bindWA, getTableDesc()));
  bindWA->getCurrentScope()->setRETDesc(getRETDesc());
  //
  // Bind the base class.
  //
  RelExpr *boundExpr = bindSelf(bindWA);
  if (bindWA->errStatus()) return boundExpr;
  //
  // Assign the set of columns that belong to the virtual table
  // as the output values that can be produced by this node.
  //
  getGroupAttr()->addCharacteristicOutputs(getTableDesc()->getColumnList());
  return boundExpr;
} // TableValuedFunction::bindNode()


// -----------------------------------------------------------------------
// Member functions for classes Control*
// must be written allowing for a NULL BindWA to be passed in!
//
// This happens when called from the SQLC/SQLCO Preprocessor,
// which needs to bind certain "static-only" statements --
// those which evaluate to STATIC_ONLY_WITH_WORK_FOR_PREPROCESSOR --
// see ControlAbstractClass::isAStaticOnlyStatement().
// -----------------------------------------------------------------------

RelExpr * ControlAbstractClass::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) return this;

  // Early return if called by SQLC/SQLCO Preprocessor
  if (!bindWA) return this;

  // Allocate an empty RETDesc and attach it to this node and the BindScope.
  setRETDesc(new(bindWA->wHeap()) RETDesc(bindWA));
  bindWA->getCurrentScope()->setRETDesc(getRETDesc());

  return bindSelf(bindWA);
} // ControlAbstractClass::bindNode()


RelExpr * ControlQueryShape::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // remember the required shape in the control table
  if (alterArkcmpEnvNow())
    {
      if (getShape())
        ActiveControlDB()->setRequiredShape(this);
      else
        {
          // no shape passed in. Hold or Restore.
          if (holdShape())
            ActiveControlDB()->saveCurrentCQS();
          else
            ActiveControlDB()->restoreCurrentCQS();
          if (ActiveControlDB()->getRequiredShape())
            ActiveControlDB()->getRequiredShape()->holdShape() = holdShape();
        }
    }

  return ControlAbstractClass::bindNode(bindWA);
} // ControlQueryShape::bindNode()


RelExpr * ControlQueryDefault::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // Alter the current Defaults settings if this is a static CQD.
  //
  // "AffectYourself" is coming to you courtesy of the Staple Singers:
  // 'Affect yourself, na na na, na na na na, affect yourself, re re re re.'
  // It's neat to find such Binder-relevant lyrics, eh?
  //
  NABoolean affectYourself = alterArkcmpEnvNow();

  assert(!bindWA || bindWA->getSchemaDB() == ActiveSchemaDB());
  NADefaults &defs = ActiveSchemaDB()->getDefaults();
  defs.setState(NADefaults::SET_BY_CQD);

  if ( defs.isReadonlyAttribute(token_) == TRUE )
    {
      Int32 attrNum = defs.lookupAttrName(token_);
      
      if (stricmp(value_, defs.getValue(attrNum)) != 0 ) 
      {
         if (CmpCommon::getDefault(DISABLE_READ_ONLY) == DF_OFF)
         {
           if (bindWA) bindWA->setErrStatus();
           *CmpCommon::diags() << DgSqlCode(-4130) << DgString0(token_);
           return NULL;
         }
       }
    }


  if (holdOrRestoreCQD_ == 0)
    {
      if (affectYourself)
        attrEnum_ =  defs.validateAndInsert(token_, value_, reset_);
      else
        attrEnum_ = defs.validate(token_, value_, reset_);
      if (attrEnum_ < 0)
        {
          if (bindWA) bindWA->setErrStatus();
          return NULL;
        }
      
      // remember this control in the control table
      if (affectYourself)
        ActiveControlDB()->setControlDefault(this);
    }
  else if ((holdOrRestoreCQD_ > 0) && (affectYourself))
    {
      attrEnum_ = defs.holdOrRestore(token_, holdOrRestoreCQD_);
      if (attrEnum_ < 0)
        {
          if (bindWA) bindWA->setErrStatus();
          return NULL;
        }
    }
  
  return ControlAbstractClass::bindNode(bindWA);
} // ControlQueryDefault::bindNode()


RelExpr * ControlTable::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) return this;

  CMPASSERT(bindWA);  // can't handle it yet if called from SQLC Preprocessor

  // remember this control in the control table
  tableName_->applyDefaults(bindWA, bindWA->getDefaultSchema());

  NABoolean ok = alterArkcmpEnvNow() ?
                      ActiveControlDB()->setControlTableValue(this) :
                      ActiveControlDB()->validate(this);
  if (NOT ok)
    {
      if (bindWA) bindWA->setErrStatus();
      return NULL;
    }

  return ControlAbstractClass::bindNode(bindWA);
} // ControlTable::bindNode()


RelExpr * ControlSession::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) return this;

  // remember this control in the control session
  NABoolean ok = alterArkcmpEnvNow() ?
                      ActiveControlDB()->setControlSessionValue(this) :
                      ActiveControlDB()->validate(this);
  if (NOT ok)
    {
      if (bindWA) bindWA->setErrStatus();
      return NULL;
    }

  return ControlAbstractClass::bindNode(bindWA);
} // ControlSession::bindNode()


RelExpr * SetSessionDefault::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  if (getOperatorType() == REL_SET_SESSION_DEFAULT)
    {
      // trim leading and trailing spaces from token_ and value_
      // and upcase token
      token_ = token_.strip(NAString::both);
      value_ = value_.strip(NAString::both);
      token_.toUpper();

      // TBD:  perhaps add a component privilege that allows others
      //       to set parserflags
      if ((token_ == "SET_PARSERFLAGS") ||
          (token_ == "RESET_PARSERFLAGS"))
        {
          if (!ComUser::isRootUserID())
            {
              *CmpCommon::diags() << DgSqlCode(-1017);
              bindWA->setErrStatus();
              return this;
            }
        }

    }

  return ControlAbstractClass::bindNode(bindWA);
} // SetSessionDefault::bindNode()



// -----------------------------------------------------------------------
// member function for class RelSetTimeout
// -----------------------------------------------------------------------

RelExpr * RelSetTimeout::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) return this;

  // Allocate an empty RETDesc and attach it to this node and the BindScope.
  setRETDesc(new(bindWA->wHeap()) RETDesc(bindWA));
  bindWA->getCurrentScope()->setRETDesc(getRETDesc());

  if (timeoutValueExpr_) {  // bind the timeout-value expression
    timeoutValueExpr_->bindNode(bindWA);
    if (bindWA->errStatus()) return this;
  }

  if ( ! strcmp("*", userTableName_.getCorrNameAsString()) )
    isForAllTables_ = TRUE ;

  HostVar *proto = userTableName_.getPrototype() ;

  // Check for the not-supported  "SET STREAM TIMEOUT" on a specific stream
  if ( isStream_ && ! isForAllTables_ ) {
    *CmpCommon::diags() << DgSqlCode(-3187);
    bindWA->setErrStatus();
    return this;
  }

  if ( isForAllTables_ ) { /* do nothing */ }
  else if ( proto ) { // it is a HOSTVAR or DEFINE
    userTableName_.applyDefaults(bindWA, bindWA->getDefaultSchema());
    CMPASSERT ( proto->isPrototypeValid() ) ;
    userTableName_.getPrototype()->bindNode(bindWA);
  } else { // i.e., an explicit table name was specified
    // Get the NATable for this table.
    NATable *naTable = bindWA->getNATable(userTableName_, FALSE);
    if (bindWA->errStatus()) return this;  // e.g. error: table does not exist

    if ( naTable->getViewText() ) {  // can not set lock timeout on a view
      *CmpCommon::diags() << DgSqlCode(-3189);
      bindWA->setErrStatus();
      return this;
    }

    // Extract and keep the physical file name
    const NAFileSet * clstInd = naTable->getClusteringIndex() ;
    setPhysicalFileName( clstInd->getFileSetName().getQualifiedNameAsString().data() );
  }

  // Bind the base class.
  return bindSelf(bindWA);
}
// -----------------------------------------------------------------------
// member functions for class Describe
// (see sqlcomp/CmpDescribe.cpp for execution of the request)
// -----------------------------------------------------------------------

RelExpr *Describe::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // SHOWCONTROL DEFAULT "magic string"; -- see ShowSchema.h and ExSqlComp.cpp
  if (getFormat() == CONTROL_DEFAULTS_) {
    if (getDescribedTableName().getQualifiedNameObj().getObjectName() ==
        ShowSchema::ShowControlDefaultSchemaMagic())
    {
      // Return info in an error message (a warning msg doesn't cut it).
      const SchemaName &catsch = bindWA->getDefaultSchema();
      NAString cat(catsch.getCatalogNameAsAnsiString(),bindWA->wHeap());
      NAString sch(catsch.getUnqualifiedSchemaNameAsAnsiString(),bindWA->wHeap());
      //
      *CmpCommon::diags() << DgSqlCode(-ABS(ShowSchema::DiagSqlCode()))
        << DgCatalogName(cat) << DgSchemaName (sch);
      bindWA->setErrStatus();
      return this;
    }
    if (getDescribedTableName().getQualifiedNameObj().getObjectName() ==
        GetControlDefaults::GetExternalizedDefaultsMagic())
    {
      // Return info in an error message (a warning msg doesn't cut it).
      NAString cqdPairs(bindWA->wHeap());
      size_t lenN, lenV;
      char lenbufN[10], lenbufV[10];
      const char *nam, *val;
      NADefaults &defs = bindWA->getSchemaDB()->getDefaults();
      for (CollIndex i = 0; i < defs.numDefaultAttributes(); i++ ) {
        if (defs.getCurrentDefaultsAttrNameAndValue(i, nam, val, TRUE)) {
          lenN = strlen(nam);
          lenV = strlen(val);
          CMPASSERT(lenN <= 999 && lenV <= 999);  // %3d coming up next
          sprintf(lenbufN, "%3d", (UInt32)lenN);
          sprintf(lenbufV, "%3d", (UInt32)lenV);
          cqdPairs += NAString(lenbufN) + nam + lenbufV + val;
        }
      }
      *CmpCommon::diags()
        << DgSqlCode(-ABS(GetControlDefaults::DiagSqlCode()))
        << DgString0(cqdPairs);
      bindWA->setErrStatus();
      return this;
    }
  }

  // Create a descriptor for a virtual table to look like this:
  // 
  //	CREATE TABLE DESCRIBE__ (DESCRIBE__COL VARCHAR(3000) NOT NULL);
  // For SeaQuest Unicode:
  //	CREATE TABLE DESCRIBE__ (DESCRIBE__COL VARCHAR(3000 BYTES) CHARACTER SET UTF8 NOT NULL);
  //
  #define MAX_DESCRIBE_LEN 3000 // e.g., SQL/MP Views.ViewText column

  // TrafAllocateDDLdesc requires that HEAP (STMTHEAP) be used for new's!

  TrafDesc * table_desc = TrafAllocateDDLdesc(DESC_TABLE_TYPE, NULL);
  table_desc->tableDesc()->tablename = new HEAP char[strlen("DESCRIBE__")+1];
  strcpy(table_desc->tableDesc()->tablename, "DESCRIBE__");

  // see nearly identical code below for indexes file desc
  TrafDesc * files_desc = TrafAllocateDDLdesc(DESC_FILES_TYPE, NULL);
  table_desc->tableDesc()->files_desc = files_desc;

  Lng32 colnumber = 0, offset = 0;
  TrafDesc * column_desc = TrafMakeColumnDesc(
       table_desc->tableDesc()->tablename,
       "DESCRIBE__COL",
       colnumber,      // INOUT
       REC_BYTE_V_ASCII,
       MAX_DESCRIBE_LEN,
       offset,      // INOUT
       FALSE/*not null*/,
       SQLCHARSETCODE_UNKNOWN,
       NULL);
  column_desc->columnsDesc()->character_set    = CharInfo::UTF8;
  column_desc->columnsDesc()->encoding_charset = CharInfo::UTF8;

  table_desc->tableDesc()->colcount = colnumber;
  table_desc->tableDesc()->record_length = offset;

  TrafDesc * index_desc = TrafAllocateDDLdesc(DESC_INDEXES_TYPE, NULL);
  index_desc->indexesDesc()->tablename = table_desc->tableDesc()->tablename;
  index_desc->indexesDesc()->indexname = table_desc->tableDesc()->tablename;
  index_desc->indexesDesc()->keytag = 0; // primary index
  index_desc->indexesDesc()->record_length = table_desc->tableDesc()->record_length;
  index_desc->indexesDesc()->colcount = table_desc->tableDesc()->colcount;
  index_desc->indexesDesc()->blocksize = 4096; // anything > 0

  TrafDesc * i_files_desc = TrafAllocateDDLdesc(DESC_FILES_TYPE, NULL);
  index_desc->indexesDesc()->files_desc = i_files_desc;

  TrafDesc * key_desc = TrafAllocateDDLdesc(DESC_KEYS_TYPE, NULL);
  key_desc->keysDesc()->keyseqnumber = 1;
  key_desc->keysDesc()->tablecolnumber = 0;
  key_desc->keysDesc()->setDescending(FALSE);

  index_desc->indexesDesc()->keys_desc = key_desc;
  table_desc->tableDesc()->columns_desc = column_desc;
  table_desc->tableDesc()->indexes_desc = index_desc;

  //
  // Get the NATable for this object.
  //
  CorrName corrName("DESCRIBE__");
  corrName.setSpecialType(ExtendedQualName::VIRTUAL_TABLE);
  NATable *naTable = bindWA->getNATable(corrName, FALSE/*CatBind*/, table_desc);
  if (bindWA->errStatus())
    return this;
  //
  // Allocate a TableDesc (which is not the table_desc we just constructed)
  // and attach it to the Scan node.
  //
  setTableDesc(bindWA->createTableDesc(naTable, corrName));
  if (bindWA->errStatus())
    return this;
  //
  // Allocate an RETDesc and attach it to the Scan node and the BindScope.
  //
  setRETDesc(new (bindWA->wHeap()) RETDesc(bindWA, getTableDesc()));
  bindWA->getCurrentScope()->setRETDesc(getRETDesc());
  //
  // Bind the described table CorrName member, the children, and the base class.
  //

  if (! describedTableName_.getQualifiedNameObj().getObjectName().isNull())
    {
       if (getIsControl())
          describedTableName_.applyDefaults(bindWA, bindWA->getDefaultSchema());
        if (NOT getIsControl())
        {
          // do not override schema for showddl
          bindWA->setToOverrideSchema(FALSE);  
          
          // check if we need to consider public schema before 
          // describedTableName_ is qualified by getNATable
          if (describedTableName_.getQualifiedNameObj().getSchemaName().isNull())
            setToTryPublicSchema(TRUE);

          if ((getFormat() == Describe::INVOKE_) ||
              (getFormat() == Describe::SHOWDDL_) &&
              (getLabelAnsiNameSpace() == COM_TABLE_NAME) &&
              (NOT getIsSchema()))
            {
              bindWA->getNATableInternal(describedTableName_);
              if (bindWA->errStatus())
                {
                  return this;
                }
            }
          else
            describedTableName_.applyDefaults(bindWA, bindWA->getDefaultSchema());
        }
      if (pUUDFName_ NEQ NULL AND NOT pUUDFName_->getObjectName().isNull())
      {
          pUUDFName_->applyDefaults(bindWA->getDefaultSchema());
      }
    }
  
  bindChildren(bindWA);
  RelExpr *boundExpr = bindSelf(bindWA);
  if (bindWA->errStatus()) return boundExpr;
  //
  // Assign the set of columns that belong to the table to be scanned
  // as the output values that can be produced by this scan.
  //
  getGroupAttr()->addCharacteristicOutputs(getTableDesc()->getColumnList());
  return boundExpr;
} // Describe::bindNode()

// -----------------------------------------------------------------------
// member functions for class RelLock
// -----------------------------------------------------------------------

RelExpr * RelLock::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // do not do override schema for this
  bindWA->setToOverrideSchema(FALSE);

  // Get the NATable for this object.
  NATable *naTable = bindWA->getNATable(getTableName());
  if (bindWA->errStatus())
    return this;

  NABoolean isView = !!naTable->getViewText();
  if (isView && !naTable->isAnMV())
    {
      *CmpCommon::diags() << DgSqlCode(-4222)
                          << DgString0("Views");
      bindWA->setErrStatus();
      return this;
    }
  else
    {
      baseTableNameList_.insert((CorrName *)getPtrToTableName());
    }

  Int32 locSpec = 0;
  NAString tabNames(bindWA->wHeap());

  for (CollIndex i = 0; i < baseTableNameList_.entries(); i++) {
    naTable = bindWA->getNATable(*baseTableNameList_[i]);
    if (bindWA->errStatus())
      return this;

    // Genesis 10-990212-6908:
    // Ignore the user-specified correlation name --
    // use just the 3-part tblname (and any LOCATION clause, etc).
    // Then, insert only unique names into tabIds_ --
    // to prevent XTNM duplicates (errmsg 4056)
    // when multiple layered views reference the same table or corr-name.

    CorrName bt(*baseTableNameList_[i]);
    bt.setCorrName("");

    NABoolean haveTDforThisBT = FALSE;
    for (CollIndex j = 0; j < tabIds_.entries(); j++) {
      if (bt == tabIds_[j]->getCorrNameObj()) {
        haveTDforThisBT = TRUE;
        break;
      }
    }

    if (!haveTDforThisBT) {
      if (bt.isLocationNameSpecified()) locSpec++;
      tabNames += NAString(", ") +
                    bt.getQualifiedNameObj().getQualifiedNameAsAnsiString();
      tabIds_.insert(bindWA->createTableDesc(naTable, bt));
      if (bindWA->errStatus()) return this;
    }
  }

  if (tabIds_.entries() > 1) {
    CMPASSERT(locSpec == 0);
    tabNames.remove(0, 2);   // remove leading ", "
    // Warning 4124: More than one table will be locked: $0~String0.
    // (warning, so user realizes the effects of this command
    // when run on a view which joins tables...).
    *CmpCommon::diags() << DgSqlCode(+4124) << DgString0(tabNames);
  }

  if ((isView) ||
      (tabIds_.entries() > 1) ||
      (baseTableNameList_.entries() > 1) ||
      (CmpCommon::getDefault(ATTEMPT_ESP_PARALLELISM) == DF_OFF))
    {
      parallelExecution_ = FALSE;
    }

  // Allocate an empty RETDesc and attach it to this node and the BindScope.
  setRETDesc(new(bindWA->wHeap()) RETDesc(bindWA));
  bindWA->getCurrentScope()->setRETDesc(getRETDesc());

  // Bind the base class.
  return bindSelf(bindWA);
} // RelLock::bindNode()

// -----------------------------------------------------------------------
// member functions for class RelTransaction
// -----------------------------------------------------------------------

RelExpr * RelTransaction::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // Allocate an empty RETDesc and attach it to this node and the BindScope.
  setRETDesc(new(bindWA->wHeap()) RETDesc(bindWA));
  bindWA->getCurrentScope()->setRETDesc(getRETDesc());

  if (diagAreaSizeExpr_) {
    diagAreaSizeExpr_->bindNode(bindWA);
    if (bindWA->errStatus()) return this;
  }

  // "mode_" is NULL if BEGIN/COMMIT/ROLLBACK WORK, nonNULL if SET TRANSACTION.
  if (mode_) {
    if ((mode_->autoCommit() != TransMode::AC_NOT_SPECIFIED_) || 
        (mode_->getAutoBeginOn() != 0) ||
        (mode_->getAutoBeginOff() != 0))
    {
      if (NOT (mode_->isolationLevel() == TransMode::IL_NOT_SPECIFIED_ &&
               mode_->accessMode()     == TransMode::AM_NOT_SPECIFIED_))
        {
          *CmpCommon::diags() << DgSqlCode(-3242)
                              << DgString0("Isolation level or Access mode options cannot be specified along with autocommit or autobegin options.");
          bindWA->setErrStatus();
          return this;
        }
    }
    else 
    {
      // See Ansi 14.1, especially SR 4.
      // Similar code must be maintained in
      // comexe/ExControlArea::addControl() and NADefaults::validateAndInsert().

      // SET TRANSACTION w/o specifying ISOLATION LEVEL reverts TransMode to
      // the NADefaults setting of ISOLATION_LEVEL
      // (which the user should set to SERIALIZABLE if they want
      // SET TRANSACTION to be Ansi conformant).

      if (mode_->isolationLevel() == TransMode::IL_NOT_SPECIFIED_)
      {
        if (CmpCommon::getDefault(ISOLATION_LEVEL_FOR_UPDATES) == DF_NONE)
          bindWA->getSchemaDB()->getDefaults().getIsolationLevel(
            mode_->isolationLevel()); // short int
        else
          bindWA->getSchemaDB()->getDefaults().getIsolationLevel(
            mode_->isolationLevel(), // short int
            CmpCommon::getDefault(ISOLATION_LEVEL_FOR_UPDATES));
      }

      if (mode_->accessMode() == TransMode::AM_NOT_SPECIFIED_)
        mode_->updateAccessModeFromIsolationLevel(
          mode_->getIsolationLevel());   // enum

      // 3114 Transaction access mode RW is incompatible with isolation level RU
      if (mode_->accessMode() == TransMode::READ_WRITE_ &&
          mode_->isolationLevel() == TransMode::READ_UNCOMMITTED_) {
        *CmpCommon::diags() << DgSqlCode(-3114);
        bindWA->setErrStatus();
        return this;
      }

      if (mode_->rollbackMode() == TransMode::ROLLBACK_MODE_NOT_SPECIFIED_)
          mode_->rollbackMode() = TransMode::ROLLBACK_MODE_WAITED_ ;
      // 4352 - 
      if (mode_->multiCommit() == TransMode::MC_ON_)
        {
          if (mode_->invalidMultiCommitCompatibility())
            {
              *CmpCommon::diags() << DgSqlCode(-4352);
              bindWA->setErrStatus();
              return this;
            }

        }
    }
  } // SET TRANSACTION stmt

  // Bind the base class.
  return bindSelf(bindWA);
}


// Transpose::bindNode - Bind the transpose node.
// Coming into the node (from the parser) there are two ItemExpr Trees:
//
//   keyCol_: The ItemExpr contains a ColReference to the key column which
//   is added by the transpose node. This pointer ia set to NULL by bindNode.
//   If keyCol_ is NULL coming into the bindNode, then no key Column is
//   generated for this transpose.
//
//   transValsTree_: This ItemExpr tree contains a list of pairs which is
//   NULL terminated (for ease of processing).  Each pair contains in child(0),
//   a list of transpose items for a given transpose set and in child(1), a
//   list of ColReferences to the new value columns associated with this
//   transpose set. A transpose item is a list of value expressions.
//   This pointer is set to NULL by bindNode.
//
// For Example:
//
//           SELECT *
//             FROM Table
//        TRANSPOSE A,B AS C1
//                  X,Y,Z as C2
//                  (1,'hello'),(2,'world') AS (C3, C4)
//           KEY BY K1
//
//   For the above query, after parsing, the TRANSPOSE node will look like:
//
//                  TRANSPOSE
//               keyCol_  transValsTree_
//                  |      |
//                  K1     O------O---------O---NULL
//                         |      |         |
//                         O      O         O--O
//                         |\     |\        |  |\
//                         O C1   O C2      | C3 C4
//                         |\     |\        O---------O---NULL
//                         A O    X O       |         |
//                           |\     |\      O         O
//                           B NULL Y O     |\        |\
//                                    |\    1 'hello' 2 'world'
//                                    Z NULL
//
// O - represent ITM_LIST nodes.
//
// bindNode binds this structure to form a new structure contained in
// the vector of ValueIdLists, transUnionVector_.
//
//   transUnionVector_: This is a vector of ValueIdLists. There is one entry
//   for each transpose set, plus one entry for the key values. Each entry
//   contains a list of ValueIdUnion Nodes. The first entry contains a list
//   with one ValueIdUnion node. This node is for the Const. Values (1 - N)
//   representing the Key Values. The other entries contain lists of
//   ValueIdUnion nodes for the Transposed Values. Each of these entries of
//   the vector represent a transpose set.  If the transpose set contains a
//   list of values, then there will be only one ValueIdUnion node in the
//   list.  If the transpose set contains a list of lists of values, then
//   there will be as many ValueIdUnion nodes as there are items in the
//   sublists. (see example below.)
//   transUnionVector_ is generated in bindNode().
//
//   transUnionVectorSize_: This is the number of entries in transUnionVector_.
//
// For the above query, after binding, the TRANSPOSE node will look like:
//
//               TRANSPOSE
//                 transUnionVectorSize_: 4
//                 transUnionVector_:
//                     ValueIdUnion(1,2,3,4,5,6,7)
//                     ValueIdUnion(A,B)
//                     ValueIdUnion(X,Y,Z)
//                     ValueIdUnion(1,2) , ValueIdUnion('hello','world')
//
//
RelExpr *Transpose::bindNode(BindWA *bindWA)
{

  // If this node has already been bound, we are done.
  //
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  BindContext *curContext = bindWA->getCurrentScope()->context();
  curContext->inTransposeClause() = TRUE;

  // Bind the child nodes.
  //
  bindChildren(bindWA);
  if (bindWA->errStatus()) return this;

  // At this point the Transpose relational operator has two or three
  // expressions:
  // keyCol_        --- A ColReference to the new keyCol. (possibly NULL)
  // transValsTree_ --- expressions for the transposed values and their
  //                    ColReferences.
  //
  // transpose::bindNode() performs the following steps:
  //
  // 1 - Construct a list of transpose set expressions
  //     and a list of ColReferences associated with each transpose set
  //     expression.
  //
  // 2 - Allocate a return descriptor and add the columns from the
  //     childs descriptor to it.
  //
  // 3 - Allocate the transUnionVector_
  //
  // 4 - Construct a ValueIdUnion node for the Key Values. Bind this node.
  //     Add the keyColName to the return descriptor with the valueId of this
  //     node.  Add the valueId of this node as the first entry of
  //     a ValueIdList in the first entry of transUnionVector_.
  //
  // 5 - For each transpose set, Construct as many ValueIdUnion nodes as
  //     there are values in each item of the transpose set. Within a
  //     given transpose set, the number of values per item must be the
  //     same.  In the example above, the third transpose set contains the
  //     items (1, 'hello') and (2, 'world'). These both have two values per
  //     item.  The others all have 1 value per item. The ValueIdUnions
  //     generated will contain the i'th value from each item. Bind each
  //     of these ValueUnionId nodes. Add the value column name to the
  //     return descriptor with the valueId of this node. Add the valueId
  //     of this node the ValueIdList in the proper entry of
  //     transUnionVector_.
  //
  // 6 - Set the return descriptor, and bindSelf.
  //

  CollIndex i, j, k;
  CollIndex numTransSets = 0;

  // Get a pointer to the head of this list of pairs.
  // This is the last time we will have to reference this tree.
  //
  ItemExpr *transTree = (ItemExpr *)removeTransValsTree();

  // Allocate two ItemExpr Lists. One for the list of lists of (lists of)
  // expressions. And the other for the list of (lists of)  ColReferences.
  //
  ItemExprList transSetsList(bindWA->wHeap());
  ItemExprList newColsList(bindWA->wHeap());

  // Populate these lists and
  // determine how many transpose sets there are in this tree.
  // In the example above, there should be three.
  //
  while (transTree) {
    transSetsList.insert(transTree->child(0)->child(0));
    newColsList.insert(transTree->child(0)->child(1));
    numTransSets++;
    transTree = transTree->child(1);
  }

  // Must have at least one value expression in the transpose values list.
  //
  CMPASSERT(numTransSets > 0);

  // Using the example above, at this point:
  //
  //   transSetsList                          newColsList
  //  |      |      |                          |   |   |
  //  O      O      O---------O---NULL         C1  C2  O
  //  |\     |\     |         |                        |\
  //  A O    X O    O         O                       C3 C4
  //    |\     |\   |\        |\
  //    B NULL Y O  1 'hello' 2 'world'
  //             |\
  //             Z NULL
  //
  // Allocate the return descriptor.  This will contain the
  // columns of the child node as well as the new columns added
  // by the transpose operator.  The column order is:
  //
  //       [childs columns][keyCol][valCol1][valCol2] ...
  //
  // Using the example, this would be:
  //
  //       [childs columns], K1, C1, C2, C3, C4
  //
  RETDesc *resultTable = new(bindWA->wHeap()) RETDesc(bindWA);

  // Add the columns from the child to the RETDesc.
  //
  const RETDesc &childTable = *child(0)->getRETDesc();
  resultTable->addColumns(bindWA, childTable);

  transUnionVectorSize_ = numTransSets + 1;
  transUnionVector() = new(bindWA->wHeap())
    ValueIdList[transUnionVectorSize_];
  //If there is a lob column return error. Transpose not allowed on lob columns.
 
  for (i = 0; i < resultTable->getDegree(); i++)
    {
      if ((resultTable->getType(i)).getFSDatatype() == REC_BLOB || 
	  (resultTable->getType(i)).getFSDatatype() == REC_CLOB)
	{
	  *CmpCommon::diags() << DgSqlCode(-4322);
	  bindWA->setErrStatus();
	  return this;
	}
    }
 
    

  // Get the key column reference
  // This is the last time we need this ItemExpr.
  //
  ColReference *keyColumn = (ColReference *)removeKeyCol();

  // If no key column has been specified, then no key col will be
  // generated.
  //
  if (keyColumn) {

    //Get the key column name.
    //
    NAString keyColName(keyColumn->getColRefNameObj().getColName(), bindWA->wHeap());

    // Construct and Bind the ValueIdUnion node as the union of constants
    // from 1 to the total number of transpose expressions. In the above
    // example this will be from 1 to 9, since there are 3 transpose sets
    // and each set has 3 expressions.
    //
    ValueIdList constVals;
    ItemExpr *constExpr;

    CollIndex keyVal;

    // For each expression in each transpose set.
    //
    for (i = 0, keyVal = 1; i < numTransSets; i++) {

      // Determine how many expressions are in each transpose set.
      //
      CollIndex numTransItems = 0;

      ItemExpr *transSet = transSetsList[i];

      while (transSet) {
        numTransItems++;
        transSet = transSet->child(1);
      }

      for (j = 0; j < numTransItems; j++, keyVal++) {

        // Construct the constant value
        //
        constExpr = new(bindWA->wHeap()) SystemLiteral(keyVal);

        // Bind the constant value.
        //
        constExpr->bindNode(bindWA);
        if (bindWA->errStatus()) return this;

        // Insert the valueId into the list
        //
        constVals.insert(constExpr->getValueId());
      }
    }

    // Construct the ValueIdUnion node which will represent the key Col.
    //
    ValueIdUnion *keyVidu = new(bindWA->wHeap())
      ValueIdUnion(constVals, NULL_VALUE_ID);

    // Bind the ValueIdUnion node.
    //
    keyVidu->bindNode(bindWA);
    if (bindWA->errStatus()) return this;

    // Add the key column to the RETDesc (as the union of all the constants)
    //
    resultTable->addColumn(bindWA, keyColName, keyVidu->getValueId());

    // The ValueIdUnion for the Key Values is the first entry in
    // the ValueIdList of the first entry of transUnionVector_.
    //
    transUnionVector()[0].insert(keyVidu->getValueId());
  }

  // For each transpose set,
  //   - bind the list of expressions.
  //   - Construct a ValueIdUnion node containing the resulting valueIds.
  //   - Bind this ValueIdUnion node
  //   - Add the associate column name to the return descriptor with the
  //     valueId of the ValueIdUnion node.
  //
  ValueIdList transVals;

  for (i = 0; i < numTransSets; i++) {

    // The column(s) associated with this transpose set.
    // (will be used below, within the inner loop)
    //
    ItemExprList newCols(newColsList[i], bindWA->wHeap());

    // Determine how many expressions are in each transpose set.
    //
    CollIndex numTransItems = 0;

    ItemExpr *transSet = transSetsList[i];

    ItemExprList transItemList(bindWA->wHeap());

    // Populate this list.
    //
    while (transSet) {
      transItemList.insert(transSet->child(0));
      numTransItems++;
      transSet = transSet->child(1);
    }

    ItemExprList transItem(transItemList[0], bindWA->wHeap());

    CollIndex numTransVals = transItem.entries();

    // For a given transpose set, the number of new columns declared
    // must be the same as the number of items per value.  In the example
    // above, the third transpose  set contains the items (1, 'hello') and
    // the columns (C3,C4) both have two entries.
    //
    if (numTransVals != newCols.entries()) {
      *CmpCommon::diags() << DgSqlCode(-4088);
      bindWA->setErrStatus();
      return this;
    }

    for (k = 0; k < numTransVals; k++) {

      ItemExpr *transValueUnionExpr = NULL;

      for (j = 0; j < numTransItems; j++) {

        transItem.clear();
        transItem.insertTree(transItemList[j], ITM_ITEM_LIST);

        // Within a given transpose set, the number of values per item
        // must be the same.  In the example above, the third transpose
        // set contains the items (1, 'hello') and (2, 'world'). These
        // both have two values per item.  The others all have 1 value
        // per item.
        //

        if (numTransVals != transItem.entries()) {
          *CmpCommon::diags() << DgSqlCode(-4088);
          bindWA->setErrStatus();
          return this;
        }

        if (transValueUnionExpr == NULL) {
          transValueUnionExpr = transItem[k];
        }
        else
        {
          transValueUnionExpr = new (bindWA->wHeap())
          ItemList(transValueUnionExpr, transItem[k]);
        }
      }


      // Bind the Transpose Values expressions. Get the expression value Id's
      //
      transVals.clear();
      if(transValueUnionExpr != NULL )
       transValueUnionExpr->convertToValueIdList(transVals,
                                                 bindWA,
                                                 ITM_ITEM_LIST);
      if (bindWA->errStatus()) return this;

      // If there are more than one transpose set, the value columns
      // generated by transpose can be NULL. So, make sure the typing is
      // done properly. This is done by setting the first in the list to
      // be nullable, then the ValueIdUnion will be nullable and the new
      // column will be nullable.  This is not done on the ValueIdUnion
      // node itself, since it will add an Null Instantiate node, and
      // we later assume that this node will always be a ValueIdUnion
      // node.
      //
      if (numTransSets > 1) {
        ValueId valId = transVals[0];

        transVals[0] = valId.nullInstantiate(bindWA, FALSE);
      }

      // Construct and Bind the ValueIdUnion node for the transpose vals.
      //
      ValueIdUnion *valVidu = new(bindWA->wHeap())
      ValueIdUnion(transVals, NULL_VALUE_ID);

      valVidu->bindNode(bindWA);
      if (bindWA->errStatus()) return this;

      // Insert this valueIdUnion node into the list of valueIdUnions
      // in the proper entry in transUnionVector_
      //
      transUnionVector()[i + 1].insert(valVidu->getValueId());

      // Get the val column reference
      //
      ColReference *valCol = (ColReference *)newCols[k];

      // Must have Column Refs to val column.
      //
      CMPASSERT(valCol);

      //Get the val column name.
      //
      NAString valColName( valCol->getColRefNameObj().getColName(), bindWA->wHeap());

      // Add the transpose column
      // (as the union of all of the transposed value columns)
      //
      resultTable->addColumn(bindWA, valColName, valVidu->getValueId());
    }
  }

  // Set the return descriptor
  //
  setRETDesc(resultTable);
  bindWA->getCurrentScope()->setRETDesc(resultTable);

  //
  // Bind the base class.
  //
  return bindSelf(bindWA);
} // Transpose::bindNode()

// -----------------------------------------------------------------------
// The Pack node binds itself by componsing its packing expression from
// all the columns available in its child's RETDesc. The packed columns
// produced by the packing expression are then made available in the Pack
// node's own RETDesc.
// -----------------------------------------------------------------------
RelExpr* Pack::bindNode(BindWA* bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  bindChildren(bindWA);
  if (bindWA->errStatus()) return this;

  // ---------------------------------------------------------------------
  // The Pack node has a packing expression stored as packingExprTree_
  // before binding. If packingExprTree_ is NULL, we are just going to
  // pick up all the columns from the output of its child. During binding,
  // this tree is converted into a value id list.
  // ---------------------------------------------------------------------

  // Create and bind the packing factor item expression.
  ItemExpr* pfie = new (bindWA->wHeap()) SystemLiteral(packingFactorLong());
  pfie->bindNode(bindWA);
  if (bindWA->errStatus()) return this;

  // Insert vid of bound constant into packingFactor valueIdSet.
  packingFactor().clear();
  packingFactor().insert(pfie->getValueId());

  // Create my RETDesc to hold the packed columns.
  RETDesc* resultTable = new (bindWA->wHeap()) RETDesc (bindWA);

  // Bind the tree if its present.
  if (packingExprTree_)
  {
    ItemExpr* packExprTree = removePackingExprTree();
    packExprTree->convertToValueIdList(packingExpr(), bindWA, ITM_ITEM_LIST);
    if (bindWA->errStatus()) return this;

    for (CollIndex i = 0; i < packingExpr().entries(); i++)
    {
      // Add all columns to result table.
      NAString packedColName( "PACKEDCOL_", bindWA->wHeap());
      packedColName += bindWA->fabricateUniqueName();
      Int32 length = packedColName.length();
      char * colName = new (bindWA->wHeap()) char[length + 1];
      colName[length] = 0;
      str_cpy_all(colName, packedColName, packedColName.length());

      ColRefName colRefName(colName);
      resultTable->addColumn(bindWA,
                             colRefName,
                             packingExpr().at(i),
                             USER_COLUMN,
                             colName);
    }
  }
  else // no packing expr tree, get all the columns from child.
  {
    // Get RETDesc from child which is assumed to be a RelRoot. too strict?
    const RETDesc& childTable = *child(0)->getRETDesc();
    ValueIdList childTableVidList;

    // These are only the user columns. Are SYS columns important?
    childTable.getValueIdList(childTableVidList);

    // Initialize packing expression.
    packingExpr().clear();

    // For each column in child's RETDesc, put a PackFunc() on top of it.
    for (CollIndex i = 0; i < childTableVidList.entries(); i++)
    {
      ItemExpr* childItemExpr = childTableVidList[i].getItemExpr();
      PackFunc* packedItemExpr = new (bindWA->wHeap())
                                             PackFunc(childItemExpr,pfie);
      // Bind the packed column.
      packedItemExpr->bindNode(bindWA);
      if (bindWA->errStatus()) return this;

      // Insert into both the result table and my packingExpr_.
      packingExpr().insert(packedItemExpr->getValueId());

      // $$$ Any implications of this? Needed to be seen.
      // Use the original column name as the packed column name. The index
      // is on USER columns only. SYS columns matter?
      ColRefName colRefName = childTable.getColRefNameObj(i);
      const char* heading = childTable.getHeading(i);

      // Insert into RETDesc for RelRoot above it to pick up as select-list.
      resultTable->addColumn(bindWA,
                             colRefName,
                             packedItemExpr->getValueId(),
                             USER_COLUMN,
                             heading);
      // $$$
      // OR: start with making a copy of child's RETDesc and change each col
      // to point to the vid for the packed column instead of the original.
    }
  }

  // Set the result table, bind self and return.
  setRETDesc(resultTable);
  bindWA->getCurrentScope()->setRETDesc(resultTable);
  bindSelf(bindWA);

  // To test packing. Add a unpack node on top of this pack node to check.
  char* env = getenv("PACKING_FACTOR");
  if (env && atol(env) > 0)
  {
    Lng32 pf = atol(env);

    ItemExpr* unPackExpr = NULL;
    ItemExpr* rowFilter = NULL;
    ItemExpr* unPackItem;
    ItemExpr* numRows;
    const NAType* typeInt = new(bindWA->wHeap()) SQLInt(bindWA->wHeap(), TRUE,FALSE);

    ValueIdList packedCols;
    resultTable->getValueIdList(packedCols);

    NAString hostVarName("_sys_UnPackIndex", bindWA->wHeap());
    hostVarName += bindWA->fabricateUniqueName();
    ItemExpr* indexHostVar = new(bindWA->wHeap())
        HostVar(hostVarName,new(bindWA->wHeap()) SQLInt(bindWA->wHeap(), TRUE,FALSE),TRUE);
    indexHostVar->synthTypeAndValueId();

    for (CollIndex i=0; i < packedCols.entries(); i++)
    {
      const NAType* colType =
            &(packedCols[i].getItemExpr()->child(0)->getValueId().getType());

      Lng32 width = colType->getNominalSize();
      Lng32 base = (colType->supportsSQLnullPhysical() ? (pf-1)/CHAR_BIT +1 : 0)
                   + sizeof(Int32);

      // $$$ Some duplicate code to be moved to PackColDesc later.
      ColRefName colRefName;
      colRefName = resultTable->getColRefNameObj(i);
      unPackItem = new(bindWA->wHeap())
        UnPackCol(packedCols[i].getItemExpr(),
                  indexHostVar,
                  width,
                  base,
                  colType->supportsSQLnull(),
                  colType);

      numRows = new(bindWA->wHeap())
       UnPackCol(packedCols[i].getItemExpr(),
                 new(bindWA->wHeap()) SystemLiteral(0),
                 typeInt->getNominalSize(),
                 0,
                 FALSE,
                 typeInt);

      unPackExpr = (unPackExpr ?
                    new(bindWA->wHeap()) ItemList(unPackExpr,unPackItem) :
                    unPackItem);

      rowFilter = (rowFilter ?
                   new(bindWA->wHeap()) ItemList(rowFilter,numRows) :
                   numRows);
    }

    RelExpr* unpack =
           new(bindWA->wHeap()) UnPackRows(pf,unPackExpr,rowFilter,NULL,
                                           this, indexHostVar->getValueId());
    return unpack->bindNode(bindWA);
  }

  return this;

} // Pack::bindNode()

RelExpr * Rowset::bindNode(BindWA* bindWA)
{
  // If this node has already been bound, we are done.
  if (nodeIsBound())
    return this->transformRelexpr_;

  if (bindWA->getHostArraysArea()) {
    bindWA->getHostArraysArea()->done() = TRUE;
  }

  //
  // Bind the child nodes.
  //
  bindChildren(bindWA);
  if (bindWA->errStatus()) return this;

  // Transform current node into a new subtree which performs access to
  // RowSet based on the unpacking and tuple node expression operators.
  // The formed tuple is composed of all input RowSet host variables:
  // Rowset-tuple: array_hv1, array_hv2, ... array_hvN.
  // The Unpack expression is used to retrieve the elements of the Rowset
  // with an indexed operator. For example, retrieve values for index two
  // for each Rowset host variable.
  // The transformed subtree has the following structure
  //
  //         UNPACK
  //            |
  //         TUPLE
  //
  // Note that the original Rowset relational expression has a rename node
  // on top.
  //

  // First find the maxRowSetSize and its rowsetSizeExpr. The rowset size is
  // the smallest declared dimension of the arrays composing the rowset.
  // If a constant rowset size was given in the SQL statement, it must be
  // samaller than the computed value.

  NABoolean hasDifferentSizes = FALSE;
  Lng32  maxRowsetSize  = 0; /* Maximum number of Rows in Rowset */
  ItemExpr *rowsetSizeExpr;
  ItemExpr *hostVarTree;

  // We get the list of input host vars, which is stored in the root of the
  // parse tree
  HostArraysWA *arrayArea = bindWA->getHostArraysArea();
  RelRoot *root =  bindWA->getTopRoot();

  // Do any extra checking at this moment.
  for (hostVarTree = inputHostvars_;
       hostVarTree;
       hostVarTree = hostVarTree->child(1)) {
    CMPASSERT(hostVarTree->getOperatorType() == ITM_ITEM_LIST);
    HostVar *hostVar = (HostVar *)hostVarTree->getChild(0);

    if (hostVar->getOperatorType() != ITM_HOSTVAR ||
        hostVar->getType()->getTypeQualifier() != NA_ROWSET_TYPE) {
      // 30001 A rowset must be composed of host variable arrays
      *CmpCommon::diags() << DgSqlCode(-30001);
      bindWA->setErrStatus();
      return NULL;
    }

    // Get the smallest dimension for rowset size

    SQLRowset* hostVarType  = (SQLRowset *)hostVar->getType();

    if (hostVarType->getNumElements() <= 0) {
      // 30004 The dimesion of the arrays composing the RowSet must be greater
      //       than zero. A value of $0~Int0 was given
      *CmpCommon::diags() << DgSqlCode(-30004)
                          << DgInt0((Int32)hostVarType->getNumElements());
      bindWA->setErrStatus();
      return NULL;
    }

    if (maxRowsetSize == 0)
      maxRowsetSize = hostVarType->getNumElements();
    else if (hostVarType->getNumElements() != maxRowsetSize) {
      // 30005 The dimensions of the arrays composing the RowSet are
      //       different. The smallest dimesnion is assumed.
      // This is just a warning

      // Give the warning only once
      if (hasDifferentSizes == FALSE) {
        if (arrayArea->hasDynamicRowsets()) {
           // 30015 The dimesion of the arrays composing the RowSet must be same
           // in dynamic SQL
            *CmpCommon::diags() << DgSqlCode(-30015) ;
            bindWA->setErrStatus();
            return NULL;
        } // for static SQL this is only a warning.
        hasDifferentSizes = TRUE;
        *CmpCommon::diags() << DgSqlCode(30005);
      }
      // Pick the smallest one
      if (hostVarType->getNumElements() < maxRowsetSize)
        maxRowsetSize = hostVarType->getNumElements();
    }

    // Make sure that the element type null indicator and the corresponding
    // rowset array are both nullable or not nullable. That is, force it

    NAType* hostVarElemType       = hostVarType->getElementType();
    NABoolean hostVarElemNullInd  = !(hostVar->getIndName().isNull());
    // If hostVarType is Unknown then this a dynamic param that has been
    // converted into a hostvar. For dynamic params there is no null 
    // indicator variable/param specified in the query text, so the previous
    // check will always return FALSE. We will set all dynamic params to be 
    // nullable and let type synthesis infer nullability later on.
    if (hostVarElemType->getTypeQualifier() == NA_UNKNOWN_TYPE)
      hostVarElemNullInd = TRUE;

    hostVarElemType->setNullable(hostVarElemNullInd);
  }

  // If a rowset size expression was produced during parsing, it is used
  // to restrict the rowset size during execution. The expression must be
  // an numeric literal (known at compile time) or an integer host variable
  // (known at execution time). We do not allow other type of expression
  // since the rowset size must be know before the statement is executed to
  // avoid copying a lot when the host variable arrays are sent down the
  // execution queue

  // If there is no size specification of the form ROWSET <size> ( <list> ) then
  // we take the size from ROWSET FOR INPUT SIZE <size>
  if (!sizeExpr_ && bindWA->getHostArraysArea()) {
    sizeExpr_ = bindWA->getHostArraysArea()->inputSize();
    if ((bindWA->getHostArraysArea()->getInputArrayMaxSize() > 0) &&
        (!sizeExpr_ )) {
       // ODBC process is performing a bulk insert and we need to create
       // an input parameter to simulate the functionality of ROWSET FOR INPUT
       // SIZE ... syntax.

       NAString name = "__arrayinputsize" ;
       HostVar *node = new (bindWA->wHeap())
                                    HostVar(name,
                                            new(bindWA->wHeap()) SQLInt(bindWA->wHeap(), TRUE,FALSE),
                                            TRUE);
       node->setHVRowsetForInputSize();
       root->addAtTopOfInputVarTree(node);
       sizeExpr_ = (ItemExpr *) node ;
    }
  }

  if (sizeExpr_) {
    if (sizeExpr_->getOperatorType() == ITM_CONSTANT) {
      if (((ConstValue *)sizeExpr_)->getType()->getTypeQualifier()
          != NA_NUMERIC_TYPE) {
        // 30003 Rowset size must be an integer host variable or an
        //       integer constant
        *CmpCommon::diags() << DgSqlCode(-30003);
        bindWA->setErrStatus();
        return NULL;
      }

      if (((ConstValue *)sizeExpr_)->getExactNumericValue() <= 0) {
        // 30004 The dimesion of the arrays composing the RowSet must be
        //       greater than zero. A value of $0~Int0 was given
        *CmpCommon::diags() << DgSqlCode(-30004)
                            << DgInt0((Int32) (((ConstValue *)sizeExpr_)
                                             ->getExactNumericValue()));
        bindWA->setErrStatus();
        return NULL;
      }

      if (((ConstValue *)sizeExpr_)->getExactNumericValue() > maxRowsetSize) {
        // 30002 The given RowSet size ($0~Int0) must be smaller or
        //       equal to the smallest dimension ($1Int1) of the
        //       arrays composing the rowset
        *CmpCommon::diags() << DgSqlCode(-30002)
                            << DgInt0((Int32)
                                      ((ConstValue *)sizeExpr_)
                                      ->getExactNumericValue())
                            << DgInt1(maxRowsetSize);
        bindWA->setErrStatus();
        return NULL;
      }
      else {
        maxRowsetSize = (Lng32)((ConstValue *)sizeExpr_)->getExactNumericValue() ;
      }
    }
    else
      if (!((sizeExpr_->getOperatorType() == ITM_HOSTVAR &&
          ((HostVar *)sizeExpr_)->getType()->getTypeQualifier()
           == NA_NUMERIC_TYPE) ||
          (sizeExpr_->getOperatorType() == ITM_DYN_PARAM ) ||
          ((sizeExpr_->getOperatorType() == ITM_CAST) &&
          (sizeExpr_->child(0)->getOperatorType() == ITM_DYN_PARAM))))
    {
      // 30003 Rowset size must be an integer host variable or an
      //       integer constant
      *CmpCommon::diags() << DgSqlCode(-30014);
      bindWA->setErrStatus();
      return NULL;
    }
    
      // We return a -1 if the execution time rowset size exceeds the maximum
      // declared size
      ItemExpr *maxSize = new (bindWA->wHeap()) SystemLiteral(maxRowsetSize);
      ItemExpr *neg = new (bindWA->wHeap()) SystemLiteral(-1);
      ItemExpr *constrPred = new (bindWA->wHeap())
                               BiRelat(ITM_GREATER, sizeExpr_, maxSize);
      rowsetSizeExpr = new (bindWA->wHeap())
                         IfThenElse(constrPred, neg, sizeExpr_);

      // IfThenElse only works if Case is its parent.
      rowsetSizeExpr = new (bindWA->wHeap()) Case (NULL, rowsetSizeExpr);

      // At code generation time, it is assumed that the size expression
      // is of size integer, so we do this cast. We do not allow null
      // values.
      rowsetSizeExpr = new (bindWA->wHeap())
        Cast(rowsetSizeExpr, new (bindWA->wHeap()) SQLInt(bindWA->wHeap(), TRUE,FALSE));

      // For dynamic rowsets, the parameter specifying rowset for input size
      // must be typed as an non-nullable integer.
      if (sizeExpr_->getOperatorType() == ITM_DYN_PARAM ) {
        sizeExpr_->synthTypeAndValueId();
        SQLInt intType(bindWA->wHeap(), TRUE,FALSE); // TRUE -> allow neagtive values, FALSE -> not nullable
        (sizeExpr_->getValueId()).coerceType(intType, NA_NUMERIC_TYPE);
      }


  }
  else
  {
    rowsetSizeExpr = new (bindWA->wHeap()) SystemLiteral(maxRowsetSize);
  }

  // Construct an index host variable to iterate over the elements of the
  // rowset. The name of the host variable must be unique (fabricated
  // by calling fabricateUniqueName). This host variable is bound since it
  // is not an input of the parse tree. Call synthTypeAndValueId()
  // which does the minimum binding.

  NAString indexName(bindWA->wHeap());
  if (indexExpr_) {
    // Get the name.
    indexName = ((ColReference *)indexExpr_)->getColRefNameObj().getColName();
  } else {
    indexName = "_sys_rowset_index" + bindWA->fabricateUniqueName();
  }

  const NAType *indexType = new (bindWA->wHeap()) SQLInt(bindWA->wHeap(), TRUE, FALSE);
  ItemExpr *indexHostVar = new (bindWA->wHeap())
    HostVar(indexName, indexType,
            TRUE // is system-generated
            );

  indexHostVar->synthTypeAndValueId();

  // Generate the RowsetArrayScan expressions which are used to extract
  // an element value of the rowset array given an index.

  ItemExpr *unPackExpr = NULL;

  for (hostVarTree = inputHostvars_;
       hostVarTree;
       hostVarTree = hostVarTree->child(1)) {
    HostVar *hostVar = (HostVar *)hostVarTree->getChild(0);
    SQLRowset* hostVarType        = (SQLRowset *)hostVar->getType();
    NAType* hostVarElemType       = hostVarType->getElementType();
    Lng32  hostVarElemSize         = hostVarElemType->getTotalSize();
    NABoolean hostVarElemNullInd  = !(hostVar->getIndName().isNull());

    // Force all host variable to have the same number of elements which was
    // found previously
    hostVarType->setNumElements(maxRowsetSize);

    // The element size must be align
    hostVarElemSize = ALIGN(hostVarElemSize,
                            hostVarElemType->getDataAlignment());

    // Assign a valueId for this Host variable. UnPackRows node will need
    // this valueId during its binding.
    //hostVar->synthTypeAndValueId();
    hostVar->bindNode(bindWA);

    ItemExpr *unPackCol =
      new (bindWA->wHeap())
      RowsetArrayScan(hostVar,          // Rowset Host Var array
                      indexHostVar,     // Index
                      maxRowsetSize,    // Cannot go over this size
                      hostVarElemSize,  // Element size in bytes
                      hostVarElemNullInd,
                      hostVarElemType
                      );

    // Construct a list of expressions to extract the Data value from
    // the packed row.  During normalization, this list (or a ValueIdList
    // representing this list) will be reduced to the minimum required.

    // This should be a NULL terminated list, unfortunately, there are
    // many parts in the SQL/MX code that loops over the arity instead
    // of checking for NULL terminated list...the effect a segmentation
    // violation.

    unPackExpr = (unPackExpr
                  ? new (bindWA->wHeap()) ItemList(unPackExpr, unPackCol)
                  : unPackCol);

  }

  // enable rowsetrowcount for rowset update and deletes
  // if the user has not turned the feature OFF.
  // if we have rowsets in where clause and are not in a select
  // then we have either rowset ypdate or delete, for direct rowsets.
  if (arrayArea && 
      (!(arrayArea->getHasDerivedRowsets())) &&
      arrayArea->hasHostArraysInWhereClause() && 
      (arrayArea->hasInputRowsetsInSelectPredicate() != HostArraysWA::YES_) &&
      (CmpCommon::getDefault(ROWSET_ROW_COUNT) == DF_ON)) {
    arrayArea->setRowsetRowCountArraySize(maxRowsetSize);
  }

  if (indexExpr_) {
    /*
     * Create and item expression to obtain the index
     */
    ItemExpr *unPackCol =
      new (bindWA->wHeap())
      RowsetArrayScan(indexHostVar,     // Index
                      indexHostVar,     // Index
                      maxRowsetSize,    // Cannot go over this size
                      indexType->getTotalSize(),
                      0,
                      indexType,
                      ITM_ROWSETARRAY_ROWID
                      );

    unPackExpr = (unPackExpr
                  ? new (bindWA->wHeap()) ItemList(unPackExpr, unPackCol)
                  : unPackCol);
  }

  // Now create a Tuple node to hang the children and input values of the
  // actual Rowset Node to it. Make sure to copy the RelExpr part of Rowset
  // to tuple.

  // Kludge up a dummy child for the index
  ItemExpr *inputs = ((indexExpr_)
                      ? new (bindWA->wHeap()) ItemList(inputHostvars_,
                                                       indexHostVar)
                      : inputHostvars_);

  Tuple *tupleExpr = new (bindWA->wHeap()) Tuple(inputs);
  tupleExpr->setBlockStmt(isinBlockStmt());

  copyTopNode(tupleExpr);

  // Construct the replacement tree for the Rowset operator.
  RelExpr *newSubTree = (new (bindWA->wHeap())
                         UnPackRows(maxRowsetSize,
                                    unPackExpr,
                                    rowsetSizeExpr,
                                    NULL,
                                    tupleExpr,
                                    indexHostVar->getValueId()));

  newSubTree->setBlockStmt(isinBlockStmt());

  // do not set this flag for derived rowsets. This flag is used in generator to determine
  // in onlj and TF TDB must set rownumber when encountering a execution time rowset error.
  if (arrayArea && 
      (!(arrayArea->getHasDerivedRowsets())) &&
      (arrayArea->hasInputRowsetsInSelectPredicate() != HostArraysWA::YES_)) 
  {
    newSubTree->setRowsetIterator(TRUE);
  }

  // Move any predicate on the packed table to be on the result
  // of unpacking.
  newSubTree->addSelPredTree(removeSelPredTree());

  // Remember the transform tree, just in case someone is trying to bind this
  // node again.
  transformRelexpr_  = newSubTree;

  // Bind the new generated subtree.
  return newSubTree->bindNode(bindWA);

} // Rowset::bindNode()

RelExpr * RowsetRowwise::bindNode(BindWA* bindWA)
{
  // If this node has already been bound, we are done.
  if (nodeIsBound())
    return this->transformRelexpr_;

  if (bindWA->getHostArraysArea()) {
    bindWA->getHostArraysArea()->done() = TRUE;
  }

  //
  // Bind the child nodes.
  //
  bindChildren(bindWA);
  if (bindWA->errStatus()) 
    return this;

  // Transform current node into a new subtree which performs access to
  // RowSet based on the unpacking.
  //         UNPACK
  //            |
  //         TUPLE
  //

  // We get the list of input host vars, which is stored in the root of the
  // parse tree
  HostArraysWA *arrayArea = bindWA->getHostArraysArea();
  
  if ((arrayArea->rwrsMaxSize()->getOperatorType() != ITM_CONSTANT) ||
      (((ConstValue *)arrayArea->rwrsMaxSize())->getType()->getTypeQualifier()
       != NA_NUMERIC_TYPE))
    {
      // 30003 Rowset size must be an integer host variable or an
      //       integer constant
      *CmpCommon::diags() << DgSqlCode(-30003);
      bindWA->setErrStatus();
      return NULL;
    }

  // if partition number has been specified, then we don't unpack
  // rows. The whole buffer is shipped to the specified partition.
  if (arrayArea->partnNum())
    return child(0)->castToRelExpr();

  Lng32 maxRowsetSize = 
    (Lng32)((ConstValue *)arrayArea->rwrsMaxSize())->getExactNumericValue() ;

  NAType * typ = new(bindWA->wHeap()) SQLInt(bindWA->wHeap(), FALSE, FALSE); 
  ItemExpr * rwrsInputSizeExpr = 
    new(bindWA->wHeap()) Cast(arrayArea->inputSize(), typ);
  if (bindWA->errStatus()) 
    return this;

  ItemExpr * rwrsMaxInputRowlenExpr =
    new(bindWA->wHeap()) Cast(arrayArea->rwrsMaxInputRowlen(), typ);
  if (bindWA->errStatus()) 
    return this;

  ItemExpr * rwrsBufferAddrExpr = arrayArea->rwrsBuffer(); 
  if (bindWA->errStatus()) 
    return this;
  
  // Construct the replacement tree for the Rowset operator.
  RelExpr *newSubTree = (new (bindWA->wHeap())
                         UnPackRows(maxRowsetSize,
                                    rwrsInputSizeExpr,
                                    rwrsMaxInputRowlenExpr,
                                    rwrsBufferAddrExpr,
                                    child(0)));

  // Remember the transform tree, just in case someone is trying to bind this
  // node again.
  transformRelexpr_  = newSubTree;

  // Bind the new generated subtree.
  return newSubTree->bindNode(bindWA);
} // RowsetRowwise::bindNode()

RelExpr * RowsetFor::bindNode(BindWA* bindWA)
{
  // Binding of this node should not happen. It should have been eliminated
  // by now by the pre-binding step. Its content is used to populate the
  // RowSet node with options.

  CMPASSERT(0);
  return NULL;
}

RelExpr * RowsetInto::bindNode(BindWA* bindWA)
{
  // If this node has already been bound, we are done.
  if (nodeIsBound())
    return this->transformRelexpr_;

  //
  // Bind the child nodes.
  //
  bindChildren(bindWA);
  if (bindWA->errStatus()) return this;

  NABoolean hasDifferentSizes = FALSE;
  Lng32  maxRowsetSize  = 0; /* Maximum number of Rows in Rowset */
  ULng32 numOutputHostvars = 0;

  ItemExpr *rowsetSizeExpr;
  ItemExpr *hostVarTree;

  // Do any extra checking at this moment.
  for (hostVarTree = outputHostvars_;
       hostVarTree;
       hostVarTree = hostVarTree->child(1)) {
    numOutputHostvars++;
    CMPASSERT(hostVarTree->getOperatorType() == ITM_ITEM_LIST);
    HostVar *hostVar = (HostVar *)hostVarTree->getChild(0);

    if (hostVar->getOperatorType() != ITM_HOSTVAR ||
        hostVar->getType()->getTypeQualifier() != NA_ROWSET_TYPE) {
      // 30001 A rowset must be composed of host variable arrays
      *CmpCommon::diags() << DgSqlCode(-30001);
      bindWA->setErrStatus();
      return NULL;
    }

    // Get the smallest dimension for rowset size


    SQLRowset* hostVarType  = (SQLRowset *)hostVar->getType();

    if (hostVarType->getNumElements() <= 0) {
      // 30004 The dimesion of the arrays composing the RowSet must be greater
      //       than zero. A value of $0~Int0 was given
      *CmpCommon::diags() << DgSqlCode(-30004)
                          << DgInt0((Int32)hostVarType->getNumElements());
      bindWA->setErrStatus();
      return NULL;
    }

    if (maxRowsetSize == 0)
      maxRowsetSize = hostVarType->getNumElements();
    else if (hostVarType->getNumElements() != maxRowsetSize) {
      // 30005 Warning: the dimensions of the arrays composing the RowSet are
      //       different. The smallest dimesnion is assumed.
      // This is just a warning

      // Give the warning only once
      if (hasDifferentSizes == FALSE) {
        hasDifferentSizes = TRUE;
        *CmpCommon::diags() << DgSqlCode(30005);
      }
      // Pick the smallest one
      if (hostVarType->getNumElements() < maxRowsetSize)
        maxRowsetSize = hostVarType->getNumElements();
    }

    // Make sure that the element type null indicator and the corresponding
    // rowset array are both nullable or not nullable. That is, force it

    NAType* hostVarElemType       = hostVarType->getElementType();
    NABoolean hostVarElemNullInd  = !(hostVar->getIndName().isNull());

    hostVarElemType->setNullable(hostVarElemNullInd);
  }

  // If a rowset size expression was produced during parsing, it is used
  // to restrict the rowset size during execution. The expression must be
  // an numeric literal (known at compile time) or an integer host variable
  // (known at execution time). We do not allow other type of expression
  // since the rowset size must be know before the statement is executed to
  // avoid copying a lot when the host variable arrays are sent down the
  // execution queue

  if (sizeExpr_) {
    if (sizeExpr_->getOperatorType() == ITM_CONSTANT) {
      if (((ConstValue *)sizeExpr_)->getType()->getTypeQualifier()
          != NA_NUMERIC_TYPE) {
        // 30003 Rowset size must be an integer host variable or an
        //       integer constant
        *CmpCommon::diags() << DgSqlCode(-30003);
        bindWA->setErrStatus();
        return NULL;
      }

      if (((ConstValue *)sizeExpr_)->getExactNumericValue() > maxRowsetSize) {
        // 30002 The given RowSet size ($0~Int0) must be smaller or
        //       equal to the smallest dimension ($1Int1) of the
        //       arrays composing the rowset
        *CmpCommon::diags() << DgSqlCode(-30002)
                            << DgInt0((Int32)
                                      ((ConstValue *)sizeExpr_)
                                      ->getExactNumericValue())
                            << DgInt1(maxRowsetSize);
        bindWA->setErrStatus();
        return NULL;
      }
    }
    else
      if (!(sizeExpr_->getOperatorType() == ITM_HOSTVAR &&
            ((HostVar *)sizeExpr_)->getType()->getFSDatatype()
            == REC_BIN32_SIGNED)) {
        // 30003 Rowset size must be an integer host variable or an
        //       integer constant
        *CmpCommon::diags() << DgSqlCode(-30003);
        bindWA->setErrStatus();
        return NULL;
      }
    rowsetSizeExpr = sizeExpr_;
  }
  else
    rowsetSizeExpr = new (bindWA->wHeap()) SystemLiteral(maxRowsetSize);


  if (getGroupAttr()->isEmbeddedUpdateOrDelete()){
    // 30020 Embedded update/delete cannot be used with SELECT...INTO and rowset.
    *CmpCommon::diags() << DgSqlCode(-30020);
    bindWA->setErrStatus();
    return NULL;
  }

  // Generate the RowsetArrayInto expressions which are used to append
  // an element value to the rowset array.

  // Get RETDesc from its only child one which must be RelRoot type.
  const RETDesc& childTable = *child(0)->getRETDesc();
  ValueIdList childTableVidList;

  childTable.getValueIdList(childTableVidList);

  if (numOutputHostvars != childTableVidList.entries()) {
    // 4094 The number of output host vars  ($0) must equal the number of cols
    *CmpCommon::diags() << DgSqlCode(-4094)
      << DgInt0(numOutputHostvars) << DgInt1(childTableVidList.entries());
    bindWA->setErrStatus();
    return NULL;
  }

  ItemExpr *packExpr = NULL;
  Lng32 i;

  for (hostVarTree = outputHostvars_, i = 0;
       hostVarTree;
       hostVarTree = hostVarTree->child(1), i++) {
    HostVar *hostVar = (HostVar *)hostVarTree->getChild(0);


    SQLRowset* hostVarType        = (SQLRowset *)hostVar->getType();
    NAType* hostVarElemType       = hostVarType->getElementType();
    // hostVarElemType->setNullable(TRUE);
    Lng32  hostVarElemSize         = hostVarElemType->getTotalSize();
    NABoolean hostVarElemNullInd  = !(hostVar->getIndName().isNull());
    ItemExpr* sourceExpr          = childTableVidList[i].getItemExpr();

    ValueId sourceId = childTableVidList[i];
    const NAType& targetType = *hostVarElemType;
    sourceId.coerceType(targetType);
    const NAType& sourceType = sourceId.getType();

    NABoolean relaxCharTypeMatchingRule = FALSE;

    // We make sure that the types that are coming from below this
    // node match properly with the types it has
    if (NOT targetType.isCompatible(sourceType)) {
      // JQ
      // Relaxing Characet Data Type mismatching rule.
      if ( targetType.getTypeQualifier() == NA_CHARACTER_TYPE &&
           sourceType.getTypeQualifier() == NA_CHARACTER_TYPE &&
           ((const CharType&)targetType).getCharSet() == CharInfo::UNICODE &&
           ((const CharType&)sourceType).getCharSet() == CharInfo::ISO88591
         )
      {
        relaxCharTypeMatchingRule = TRUE;
      }

      if ( !relaxCharTypeMatchingRule ) {
        // Incompatible assignment from type $0~String0 to type $1~String1
        *CmpCommon::diags() << DgSqlCode(-30007)
                            << DgString0(sourceType.getTypeSQLname(TRUE /*terse*/))
                            << DgString1(targetType.getTypeSQLname(TRUE /*terse*/));
        bindWA->setErrStatus();
        return FALSE;
      }
    }

    // Force all host variable to have the same number of elements which was
    // found previously
    hostVarType->setNumElements(maxRowsetSize);

    // The element size must be align
    hostVarElemSize = ALIGN(hostVarElemSize,
                            hostVarElemType->getDataAlignment());

    // Preserve the length that is coming from the node below this one
    if (hostVarElemType->getTypeQualifier() == NA_CHARACTER_TYPE &&
        sourceType.getTypeQualifier() == NA_CHARACTER_TYPE) {
      Int32 sourceSize = ((CharType *) &sourceType)->getDataStorageSize();
      Int32 targetSize = ((CharType *) hostVarElemType)->getDataStorageSize();
      if (sourceSize > targetSize ) {
     // Adjust the layout size instead of changing the element size?
        ((CharType *) hostVarElemType)->setDataStorageSize(sourceSize);
      }
    }

    if ( relaxCharTypeMatchingRule == TRUE )
      sourceExpr = new (bindWA->wHeap())
           Translate(sourceExpr, Translate::ISO88591_TO_UNICODE);

    // If the type is external (for instance, decimal or varchar), we must first
    // convert to our internal equivalent type
    if (hostVarElemType->isExternalType()) {
      NAType *internalType = hostVarElemType->equivalentType();
      sourceExpr = new (bindWA->wHeap()) Cast(sourceExpr, internalType);
    }

    sourceExpr = new (bindWA->wHeap()) Cast(sourceExpr, hostVarElemType);

    ItemExpr *packCol =
      new (bindWA->wHeap())
      RowsetArrayInto(sourceExpr,
                      rowsetSizeExpr,    // Runtime size
                      maxRowsetSize,     // Cannot go over this size
                      hostVarElemSize,   // Element size in bytes
                      hostVarElemNullInd,
                      hostVarType
                      );

    // Construct a list of expressions to append the Data value to the
    // RowSet array. This list should be a NULL terminated list,
    // unfortunately, there are many parts in the SQL/MX code that
    // loops over the arity instead of checking for NULL terminated
    // list...the effect a segmentation violation.

    packExpr = (packExpr
                ? new (bindWA->wHeap()) ItemList(packExpr, packCol)
                : packCol);
  }

  // Construct the replacement tree for the RowsetInto operator.
  RelExpr *newSubTree = (new (bindWA->wHeap())
                         Pack(maxRowsetSize,
                              child(0)->castToRelExpr(),
                              packExpr));

  newSubTree->setFirstNRows(getFirstNRows());

  // If we have an ORDER BY when there is an INTO :array, then we
  // add the requirement that the tuples that this Pack node will
  // receive must be sorted

  ValueIdList *ptrReqOrder;

  ptrReqOrder = new (bindWA->wHeap())
      ValueIdList(((RelRoot *) (RelExpr *) newSubTree->child(0))->reqdOrder());

  ((Pack *) newSubTree)->setRequiredOrder(*ptrReqOrder);


  // Remember the transform tree, just in case someone is trying to bind this
  // node again.
  transformRelexpr_  = newSubTree;

  // Bind the new generated subtree.
  return newSubTree->bindNode(bindWA);

} // RowsetInto::bindNode

RelExpr *
IsolatedScalarUDF::bindNode (BindWA *bindWA)
{

  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

     // If we have a RoutineDesc, it means we got transformed from a 
     // a UDFunction ItemExpr, and do NOT need to check all the metadata
     // params etc. 
  if (getRoutineDesc() == NULL ) 
  {
        // If we get here, we created a IsolatedScalarUDF some other way
        // than through the transformation of UDFunction. Either that or
        // we have someone walking over our memory...

     CMPASSERT(0);
     bindWA->setErrStatus();
     return this;
                                                 

  }
  else
  {
     markAsBound();
  }


  return this;
} // IsolatedScalarUDF::bindNode ()

/*
 * This method performs binder functions for the CALLSP node
 * It performs semantic checks on the called stored procedure
 * creates a Tuple child and allocates ValueIds for the parameters
 * It also provides support for the CLI layer processing for OUT
 * parameter processing.
 */
RelExpr *CallSP::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  if (bindWA->getHoldableType() == SQLCLIDEV_ANSI_HOLDABLE)
  {
    *CmpCommon::diags() << DgSqlCode(-4382);
    bindWA->setErrStatus();
    bindWA->setBindingCall (FALSE);
    return this;
  }

  bindWA->setBindingCall (TRUE);
  bindWA->setCurrOrdinalPosition (1);
  bindWA->setCurrParamMode (COM_UNKNOWN_DIRECTION);
  bindWA->clearHVorDPinSPDups ();
  bindWA->setDupWarning (FALSE);
  bindWA->setMaxResultSets(0);

  // try PUBLIC SCHEMA only when no schema was specified
  // and CQD PUBLIC_DEFAULT_SCHEMA is specified
  NAString pSchema = 
    ActiveSchemaDB()->getDefaults().getValue(PUBLIC_SCHEMA_NAME);
  ComSchemaName pubSchema(pSchema);
  NAString pubSchemaIntName = "";
  if ( (getRoutineName().getSchemaName().isNull()) && 
       (!pubSchema.getSchemaNamePart().isEmpty()) )
  {
    pubSchemaIntName = pubSchema.getSchemaNamePart().getInternalName();
  }

  // Invoke GetNARoutine () to retrieve the corresponding NARoutine from
  // NARoutineDB_
  QualifiedName name = getRoutineName();
  const SchemaName &defaultSchema =
    bindWA->getSchemaDB ()->getDefaultSchema();
  name.applyDefaults(defaultSchema);
  setRoutineName(name);
  bindWA->setCurrSPName(&name);

    // in open source, only the SEABASE catalog is allowed.
    // Return an error if some other catalog is being used.
  if ((NOT name.isSeabase()) && (NOT name.isHive()))
      {
	*CmpCommon::diags()
	  << DgSqlCode(-1002)
	  << DgCatalogName(name.getCatalogName());
	
	bindWA->setErrStatus();
	return NULL;
      }


  CmpSeabaseDDL cmpSBD((NAHeap*)bindWA->wHeap());
  TrafDesc *catRoutine = 
	    cmpSBD.getSeabaseRoutineDesc(
				       name.getCatalogName(),
				       name.getSchemaName(),
				       name.getObjectName());

  // try public schema
  if ( !catRoutine && 
       !pubSchemaIntName.isNull() )
  {
    getRoutineName().setSchemaName(pubSchemaIntName);
    if ( !pubSchema.getCatalogNamePart().isEmpty() )
    {
      getRoutineName().setCatalogName(pubSchema.getCatalogNamePart().getInternalName());
    }

    // in open source, only the SEABASE catalog is allowed.
    // Return an error if some other catalog is being used.
    if ((NOT getRoutineName().isSeabase()) && (NOT getRoutineName().isHive()))
      {
	*CmpCommon::diags()
	  << DgSqlCode(-1002)
	  << DgCatalogName(getRoutineName().getCatalogName());
	
	bindWA->setErrStatus();
	return NULL;
      }

    bindWA->resetErrStatus();
    catRoutine = 
	    cmpSBD.getSeabaseRoutineDesc(
                 getRoutineName().getCatalogName(),
                 getRoutineName().getSchemaName(),
                 getRoutineName().getObjectName());
    if ( !bindWA->errStatus() && catRoutine )
    { // if found in public schema, do not show previous error
      CmpCommon::diags()->clear();
    }
  }

  if (bindWA->violateAccessDefaultSchemaOnly(getRoutineName()))
    return NULL;

  if ( NULL == catRoutine )
  {
    // Diagnostic error is set by the readRoutineDef, we just need to
    // make sure the rest of the compiler knows that an error occurred.
    bindWA->setBindingCall (FALSE);
    bindWA->setErrStatus ();
    return this;
  }

  // Create a new NARoutine object
  Int32 error = FALSE;
  NARoutine *routine = new (bindWA->wHeap()) NARoutine ( getRoutineName(),
                                                         catRoutine,
                                                         bindWA,
                                                         error );

  if ( bindWA->errStatus () )
  {
    // Error
    bindWA->setBindingCall (FALSE);
    bindWA->setErrStatus ();
    return this;
  }

  NABoolean createRETDesc=TRUE;
  RoutineDesc *rDesc = new (bindWA->wHeap()) RoutineDesc(bindWA, routine); 
  if (rDesc == NULL ||  bindWA->errStatus ())
  {
    // Error
    bindWA->setBindingCall (FALSE);
    bindWA->setErrStatus ();
    return this;
  }
 
  if (rDesc->populateRoutineDesc(bindWA, createRETDesc) == FALSE ) 
  {
    bindWA->setBindingCall (FALSE);
    bindWA->setErrStatus ();
    return this;
  }

  setRoutineDesc(rDesc);


  //
  // Semantic checks
  //

  // if in trigger and during DDL make sure to Fix up the name 
  // location list so that the name is fully qualified when stored 
  // in the TEXT metadata table
  if ( bindWA->inDDL() && bindWA->isInTrigger () )
  {
     ParNameLocList *pNameLocList = bindWA->getNameLocListPtr();
     if (pNameLocList)
     {
       ParNameLoc * pNameLoc
         = pNameLocList->getNameLocPtr(getRoutineName().getNamePosition());
       CMPASSERT(pNameLoc);
       pNameLoc->setExpandedName(getRoutineName().getQualifiedNameAsAnsiString());
     }
  }

  // Cannot support result sets or out params when
  // SP is invoked within a trigger
  if ( bindWA->isInTrigger () &&
       getNARoutine()->hasOutParams ())
  {
    *CmpCommon::diags() <<  DgSqlCode(-UDR_BINDER_OUTPARAM_IN_TRIGGER)
                          << DgTableName (getRoutineName().getQualifiedNameAsString());
    bindWA->setErrStatus ();
    bindWA->setBindingCall (FALSE);
    return this;
  }

  if ( bindWA->isInTrigger () &&
       getNARoutine()->hasResultSets ())
  {
    *CmpCommon::diags() <<  DgSqlCode(-UDR_BINDER_RESULTSETS_IN_TRIGGER)
                          << DgTableName (getRoutineName().getQualifiedNameAsString());
    bindWA->setErrStatus ();
    bindWA->setBindingCall (FALSE);
    return this;
  }

  const NAColumnArray &params = getNARoutine()->getParams ();
  CollIndex i = 0;
  CollIndex numParams = getNARoutine()->getParamCount ();

  CollIndex numSuppliedParams = countSuppliedParams (getRWProcAllParamsTree());
  if (numSuppliedParams != numParams)
  {
    *CmpCommon::diags() << DgSqlCode(-UDR_BINDER_INCORRECT_PARAM_COUNT)
                        << DgTableName(getRoutineName().getQualifiedNameAsString())
                        << DgInt0((Lng32) numParams)
                        << DgInt1((Lng32) numSuppliedParams);
    bindWA->setErrStatus ();
    bindWA->setBindingCall (FALSE);
    return this;
  }

  short numResultSets = (short) getNARoutine()->getMaxResults();
  bindWA->setMaxResultSets(numResultSets);

  // On to the binding
  // Invoke populateAndBindItemExpr, set up needed data structures

  // Set up a RETDesc if we don't already have one.
  RETDesc *resultTable = getRETDesc();
  if (resultTable == NULL) 
  {
       resultTable = new (bindWA->wHeap()) RETDesc(bindWA);
       setRETDesc(resultTable);
  }

  populateAndBindItemExpr ( getRWProcAllParamsTree(),
                            bindWA );
  if ( bindWA->errStatus ())
  {
    bindWA->setBindingCall (FALSE);
    return this;
  }

  // Clear the Tree since we now have gotten vids for all the parameters.
  setProcAllParamsTree(NULL);

  // Now fix the param index value of the dynamic params or host vars
  LIST (ItemExpr *) &bWA_HVorDPs = bindWA->getSpHVDPs();
  CollIndex numHVorDPs = bWA_HVorDPs.entries();

  ARRAY(ItemExpr *) local_HVorDPs(HEAP, numHVorDPs);
  CollIndex idx, idx1, idx2;

  // Sort the ItemExpr in the order they appeared in the stmt
  for (idx = 0; idx < numHVorDPs; idx++)
  {
    // Copy ItemExpr ptrs to a sorted Array.
    local_HVorDPs.insertAt(bWA_HVorDPs[idx]->getHVorDPIndex() - 1,
                           bWA_HVorDPs[idx]);
  }

  // The following code goes through the list of Exprs and
  // sets index values. The rules are:
  // 1. When a DP or HV is repeated, all of them get the same
  // index value which is equal to the index of the first occurrence
  // 2. Two DPs or HVs are same if their names and the modes are same.
  Int32 currParamIndex = 1;
  for (idx1 = 0; idx1 < numHVorDPs; idx1++)
  {
    ItemExpr *src = local_HVorDPs[idx1];
    const NAString &name1 = (src->getOperatorType() == ITM_HOSTVAR) ?
      ((HostVar *)src)->getName() : ((DynamicParam *)src)->getName();
    ComColumnDirection mode1 = src->getParamMode();

    NABoolean encounteredElement = FALSE;

    for (idx2 = idx1; idx2 < numHVorDPs; idx2++)
    {
      ItemExpr *dest = local_HVorDPs[idx2];

      if (!encounteredElement && dest->getHVorDPIndex() >= currParamIndex)
      {
        // The parameter is encountered the first time
        encounteredElement = TRUE;
        dest->setPMOrdPosAndIndex(dest->getParamMode(),
                                  dest->getOrdinalPosition(),
                                  currParamIndex);
        continue;
      }

      // The parameter is already corrected
      if (dest->getHVorDPIndex() < currParamIndex)
        continue;

      const NAString &name2 = (dest->getOperatorType() == ITM_HOSTVAR) ?
        ((HostVar *)dest)->getName() : ((DynamicParam *)dest)->getName();
      ComColumnDirection mode2 = dest->getParamMode();

      if (name2.compareTo("") == 0)
        continue;

      if (name1.compareTo(name2) == 0 && mode1 == mode2)
      {
        dest->setPMOrdPosAndIndex(dest->getParamMode(),
                                  dest->getOrdinalPosition(),
                                  currParamIndex);
      }
    }

    if (encounteredElement)
      currParamIndex++;
  }

  // Restore the bindWA's HVorDP list since it might be needed
  // while binding the root node in case of HVs.
  bindWA->clearHVorDPinSPDups();
  for (idx = 0; idx < numHVorDPs; idx++)
    bindWA->addHVorDPToSPDups(local_HVorDPs[idx]);

  // Create a tuple child for any subqueries or UDF inputs
  // The hasSubquery() / hasUDF() flag gets set in setInOrOutParam if any of our
  // passed in parameters is a subquery.

  
  if ((getProcInputParamsVids().entries() != 0)  && 
      (hasSubquery() || hasUDF())) 
  {
     Tuple *inTuple = new (bindWA->wHeap()) 
                Tuple(getProcInputParamsVids().rebuildExprTree(ITM_ITEM_LIST), 
                      bindWA->wHeap());
   
     if ( inTuple )
     {
       // Now set and bind the Tuple child
       setChild (0, inTuple);
   
       // Bind this Tuple child
       inTuple->bindNode (bindWA);
       if ( bindWA->errStatus ())
       {
         bindWA->setBindingCall (FALSE);
         return this;
       }
   
       // Get each IN entry from the Tuple and put it in
       //the super's list

       // Need to clear the list to avoid duplicates
       getProcInputParamsVids().clear();

       // Now reinitialize the inputs based on the Tuple processing.
       inTuple->getRETDesc ()->getValueIdList (getProcInputParamsVids());
   
     } // if inTuple
     else
     {
         // Out of memory ...
         bindWA->setBindingCall (FALSE);
         bindWA->setErrStatus();
         return this;
     }
  } // if getProcInputParamVids().entries()
  else
  {

    // If we dont have a subquery parameter, we dont need to go thru
    // Optimization time rules and transformations, hence mark this
    // as a physical node.

    isPhysical_ = TRUE;
  }


  //
  // Not sure whether we need to set the currently scoped RETDesc
  // before binding the base class. Tuple::bindNode() does not do it
  // so we won't either (for now)
  //
  //bindWA->getCurrentScope()->setRETDesc(getRETDesc());

  // add the routine to the UdrStoiList.  The UdrStoi list is used
  // to check valid privileges
  LIST(OptUdrOpenInfo *) udrList = bindWA->getUdrStoiList ();
  ULng32 numUdrs = udrList.entries();
  NABoolean udrReferenced = FALSE;

  // See if UDR already exists
  for (ULng32 stoiIndex = 0; stoiIndex < numUdrs; stoiIndex++)
  {
    if ( 0 ==
         udrList[stoiIndex]->getUdrName().compareTo(
                           getRoutineName().getQualifiedNameAsAnsiString()
                                                  )
       )
    {
      udrReferenced = TRUE;
      break;
    }
  }

  // UDR has not been defined, go ahead an add one
  if ( FALSE == udrReferenced )
  {
    SqlTableOpenInfo *udrStoi = new (bindWA->wHeap ())SqlTableOpenInfo ();
    udrStoi->setAnsiName ( convertNAString(
                           getRoutineName().getQualifiedNameAsAnsiString(),
                           bindWA->wHeap ())
                         );

    OptUdrOpenInfo *udrOpenInfo = new (bindWA->wHeap ())
      OptUdrOpenInfo( udrStoi
                    , getRoutineName().getQualifiedNameAsAnsiString()
                    , (NARoutine *)getNARoutine()
                    );
    bindWA->getUdrStoiList().insert(udrOpenInfo);
  }


  //
  // Bind the base class
  //
  RelExpr *boundExpr = bindSelf(bindWA);
  if (bindWA->errStatus())
  {
    bindWA->setBindingCall (FALSE);
    return boundExpr;
  }

  // Our characteristic inputs get set for us, we don't need to do it
  // ourselves, however, we need to set our characteristic outputs
  getGroupAttr()->addCharacteristicOutputs(getProcOutputParamsVids());

  if (getNARoutine()->isProcedure())
    bindWA->setHasCallStmts(TRUE);

  bindWA->setBindingCall (FALSE);
  return boundExpr;
} // CallSP::bindNode()



// This is the main entry point to walking the ItemExpr tree built by the
// parser, separating the IN and OUT parameters, setting appropriate
// characteristics of the IN/OUT parameters and binding them
// Currently only CallSP uses this code. If this routine needs to be shared
void IsolatedNonTableUDR::populateAndBindItemExpr ( ItemExpr *param,
                                       BindWA *bindWA )
{
  // This method is called recursively
  CollIndex numParams = getEffectiveNARoutine()->getParamCount ();
  CollIndex ordinalPosition = bindWA->getCurrOrdinalPosition ();

  // No parameters, or we are done with the leaf node
  if ( NULL == param )
  {
    return;
  }

  ComColumnDirection mode =
     getEffectiveNARoutine()->getParams()[ordinalPosition-1]->getColumnMode ();

  // This is the structure of the ItemExpr tree
  // For 1 param
  //    ItemExpr
  //
  //  2 params
  //    ItemList
  //     /    \
  //  Param1  Param2
  //
  // > 2 params
  //              ItemList
  //             /        \
  //        Param1         ItemList
  //                    /       \
  //                 Param2    ItemList
  //                        ...        ...
  //                     ...             ...
  //                      /              /   \
  //                    Param (N-2)    /      \
  //                                 /         \
  //                              Param(N-1)   Param(N)
  if ( ITM_ITEM_LIST == param->getOperatorType ())
  {
    // Use left child
    CMPASSERT ((ItemExpr *) NULL != (*param).child (0));

    populateAndBindItemExpr ( (*param).child(0),
                              bindWA );

    if ( bindWA->errStatus ())
      return;

    // Now for the right child
    CMPASSERT ((ItemExpr *) NULL != (*param).child (1));
    populateAndBindItemExpr ( (*param).child(1),
                              bindWA );

    return;

  } // if ITM_ITEM_LIST == param->getOperatorType ()

  // For all leaf nodes we must come here (see the recursive call to
  // populateAndBindItemExp above)
  // Set the bindWA's current ordinal position and parameter mode
  // Let HV and DynamicParam's bindNode take care of the
  // settings. To ensure this, do a bindNode here
  bindWA->setCurrParamMode (mode);
  param->bindNode (bindWA);
  if (bindWA->errStatus ())
    return;

  // Add the IN or OUT params to their respective lists
  // and also create and bind a new ItemExpr for INOUT and OUT
  // params.
  // Also bump up the ordinalPosition count since we are done with this
  // parameter.
  setInOrOutParam (param,/* ordinalPosition,*/ mode, bindWA);
  if ( bindWA->errStatus ())
    return;

  bindWA->setCurrOrdinalPosition (bindWA->getCurrOrdinalPosition () + 1);

} // PopulateAndBindItemExpr



void
IsolatedNonTableUDR::setInOrOutParam (ItemExpr *expr,
                              ComColumnDirection paramMode,
                              BindWA *bindWA)
{
    // Should not get here..
   CMPASSERT(FALSE);
}


// This method separates the IN and OUT parameters Each IN/INOUT param
// is cast to the formal type (from NARoutine). This Cast'ed item expr
// is added to an ItemList tree to be passed to the Tuple ()
// constructor.  For each OUT/INOUT, we create a NATypeToItem
// ItemExpr, bind it and add it to procOutParams_.
//
// This method is called once for each CALL statement argument. If an
// input argument to a CALL is an expression tree such as "? + ?" or
// "abs(:x)" then this method is called once for the entire tree.
//
// Side Effects: OUT: hasSubquery_
//                    neededValueIds_
//                    procAllParamsVids_
//                    procInputParamsVids_
//                    procOutputParamsVids_
void CallSP::setInOrOutParam ( ItemExpr *expr,
                               ComColumnDirection paramMode,
                               BindWA *bindWA)
{

  // Depending on whether this is an IN or OUT parameter, we need to
  // take different actions.

  // For an IN (and INOUT) param, do the following
  // Cast the parameter to its formal type and add it to the list of
  // IN params. This will be used later to create a Tuple child and
  // be bound by the Tuple itself
  CollIndex ordinalPosition = bindWA->getCurrOrdinalPosition ();

  const NAColumnArray &formalParams = getNARoutine()->getParams();
  NAColumn &naColumn = *(formalParams[ordinalPosition-1]);
  const NAType &paramType = *(naColumn.getType());

  // Don't really want to bind this, but how else can we
  // get the ItemExpr's type
  ItemExpr *boundExpr = expr->bindNode (bindWA);
  if ( bindWA->errStatus ())
  {
    return;
  }



  //10-061031-0188-Begin
  //Need to infer charset for string literals part of CALLSP
  //parameters
  ValueId inputTypeId = boundExpr->getValueId();

  if(inputTypeId.getType().getTypeQualifier() == NA_CHARACTER_TYPE)
  {
   const CharType* stringLiteral = (CharType*)&(inputTypeId.getType());

  if(CmpCommon::wantCharSetInference())
  {
    const CharType* desiredType =
       CharType::findPushDownCharType(((CharType&)paramType).getCharSet(), stringLiteral, 0);

    if ( desiredType )
      inputTypeId.coerceType((NAType&)*desiredType, NA_CHARACTER_TYPE);
 }
 }

  NABoolean throwInTranslateNode = FALSE;
  CharInfo::CharSet paramCS = CharInfo::UnknownCharSet;
  CharInfo::CharSet inputCS = CharInfo::UnknownCharSet;

 const NABoolean isJdbc =
    (CmpCommon::getDefault(JDBC_PROCESS) == DF_ON ? TRUE : FALSE);
 const NABoolean isOdbc =
    (CmpCommon::getDefault(ODBC_PROCESS) == DF_ON ? TRUE : FALSE);

 const NAType &inputType = inputTypeId.getType();
  //10-061031-0188-End

  if ( COM_INPUT_COLUMN == paramMode ||
       COM_INOUT_COLUMN == paramMode )
  {
    // If this input argument to the CALL is a single dynamic param
    // then we want to record the formal parameter name. It will later
    // be written into the query plan by the code generator and
    // eventually if this CALL statement is DESCRIBEd, the formal
    // param name gets returned in the SQLDESC_NAME descriptor entry.
    if (expr->getOperatorType() == ITM_DYN_PARAM)
    {
      DynamicParam *dp = (DynamicParam *) expr;
      dp->setUdrFormalParamName(naColumn.getColName());
    }

    // Check to see if we have a Subquery as an input
    if ( !hasSubquery() )
      hasSubquery() = expr->containsSubquery ();


    // Check to see if we have a UDF as an input
    if ( !hasUDF() )
      hasUDF() = (expr->containsUDF () != NULL);

    // Do type checking,
    // If it is not a compatible type report an error
    if (!( NA_UNKNOWN_TYPE == inputType.getTypeQualifier () ||
           paramType.isCompatible(inputType) ||
           expr->getOperatorType () == ITM_DYN_PARAM
         )
       )
    {
      if ( inputType.getTypeQualifier() == NA_CHARACTER_TYPE )
      {
         paramCS = ((CharType&)paramType).getCharSet();
         inputCS = ((CharType&)inputType).getCharSet();

         NABoolean CS_unknown = (paramCS == CharInfo::UnknownCharSet) ||
                                (inputCS == CharInfo::UnknownCharSet) ;
         if ( paramType.NAType::isCompatible(inputType) &&
              paramCS != inputCS                        &&
              CS_unknown == FALSE                       &&
              CmpCommon::getDefault(ALLOW_IMPLICIT_CHAR_CASTING) == DF_ON
            )
               throwInTranslateNode = TRUE;
      }
      if ( throwInTranslateNode == FALSE )
      {
        // Error, data types dont match
        *CmpCommon::diags() <<  DgSqlCode(-UDR_BINDER_PARAM_TYPE_MISMATCH)
                            << DgInt0 (ordinalPosition)
                            << DgTableName(getRoutineName().getQualifiedNameAsString())
                            << DgString0 (inputType.getTypeSQLname (TRUE))
                            << DgString1 (paramType.getTypeSQLname (TRUE));
        bindWA->setErrStatus ();
        return;
      }
    } // if NOT isCompatible

    // Create a Cast node if the types are not identical
    if (! (inputType == paramType))
    {
       // First create a Translate node if the character sets are not identical

       if ( throwInTranslateNode )
       {
            Int32 tran_type = find_translate_type( inputCS, paramCS );

            ItemExpr * newTranslateChild =
                new (bindWA->wHeap()) Translate(boundExpr, tran_type );
            boundExpr = newTranslateChild->bindNode(bindWA);
            if (bindWA->errStatus())
               return;
            // NOTE: Leave "expr" at it's old value as code below needs to check
            //       that original ItemExpr rather than the new Translate node.
       }
       Cast *retExpr = new (bindWA->wHeap())
                          Cast(boundExpr, &paramType, ITM_CAST, TRUE);

       boundExpr = retExpr->bindNode (bindWA);
       if ( bindWA->errStatus ())
       {
          return;
       }
    }
    // Fill the ValueIdList for all the params
    getProcAllParamsVids().insert( boundExpr->getValueId());

    // Fill the ValueIdList for Input params
    getProcInputParamsVids().insert( boundExpr->getValueId());

  } // if INPUT or INOUT

  // For OUT (and INOUT) parameters, we create a NATypeToItem object,
  // bind it and add it to the list of OUT parameters (procOutParams_)
  if ( COM_OUTPUT_COLUMN == paramMode ||
       COM_INOUT_COLUMN == paramMode )
  {
    if (!( ITM_HOSTVAR == expr->getOperatorType () ||
           ITM_DYN_PARAM == expr->getOperatorType ()))
    {
      *CmpCommon::diags() << DgSqlCode(-UDR_BINDER_OUTVAR_NOT_HV_OR_DP)
                          << DgInt0(ordinalPosition)
                          << DgTableName(getRoutineName().getQualifiedNameAsString());
      bindWA->setErrStatus ();
      return;
    } // if NOT HOSTVAR or DYNAMIC PARAM

    NATypeToItem *paramTypeItem = new (bindWA->wHeap())
                                      NATypeToItem (naColumn.mutateType());
    ItemExpr *outputExprToBind = NULL;
    outputExprToBind = paramTypeItem->bindNode (bindWA);
    if ( bindWA->errStatus ())
    {
      return;
    }



    // Fill the ValueIdList for all the params
    getProcAllParamsVids().insert( outputExprToBind->getValueId());

    // Fill the ValueIdList for the output params
    addProcOutputParamsVid(outputExprToBind->getValueId ());

    //
    // Populate our RETDesc
    //
    // It has already been alocated
    RETDesc *resultTable = getRETDesc();

    const NAString &formalParamName = naColumn.getColName();
    const NAString *colParamName = &formalParamName;

    // Set the userParamName
    const NAString &userParamName =
            // cannot use the boundExpr here as it will be a cast()
            // for the HostVar or DynamicParam. Use the original
            // ItemExpr pointer instead.
        (ITM_HOSTVAR == expr->getOperatorType()) ?
        ((HostVar *)expr)->getName() :
        ((DynamicParam *)expr)->getName();


    // Typically the name for this output column will be the formal
    // parameter name. Exceptions:
    // - No formal name was specified in the CREATE PROCEDURE. Use
    //   the (possibly empty) dynamic parameter or host variable name
    //   instead.
    // - This is a JDBC or ODBC compile and the client is using a
    //   named host variable or dynamic parameter. JDBC and ODBC want
    //   us to use the client's name in this case.

    if (formalParamName.isNull() ||
        (!userParamName.isNull() && (isJdbc || isOdbc)))
    {
      colParamName = &userParamName;
    }

    ColRefName *columnName = 
        new (bindWA->wHeap())
            ColRefName(*colParamName, bindWA->wHeap());

    resultTable->addColumn(bindWA, *columnName, outputExprToBind->getValueId());

    //
    // We need the following line for static cursor declaration,
    // according to a comment in bindRowValues()
    //
    cmpCurrentContext->saveRetrievedCols_ = resultTable->getDegree();

  } // if OUTPUT or INOUT

} // setInOrOutParam





CollIndex RelRoutine::countSuppliedParams (ItemExpr *tree) const
{
  CollIndex numParams=0;

  if ( !tree )  return 0;

  if (ITM_ITEM_LIST == tree->getOperatorType ())
  {
    numParams += countSuppliedParams (tree->child (0));
    numParams += countSuppliedParams (tree->child (1));
  }
  else
    numParams++;

  return numParams;

} // RelRoutine::countSuppliedParams

void RelRoutine::gatherParamValueIds (const ItemExpr *tree, ValueIdList &paramsList) const
{
  if ( !tree )  return;

  if (ITM_ITEM_LIST == tree->getOperatorType ())
  {
    gatherParamValueIds (tree->child (0), paramsList);
    gatherParamValueIds (tree->child (1), paramsList);
  }
  else
    paramsList.insert(tree->getValueId());
} // RelRoutine::gatherParamValueIds

void ProxyFunc::createProxyFuncTableDesc(BindWA *bindWA, CorrName &corrName)
{
  // Map column definitions into a TrafDesc
  TrafDesc *tableDesc = createVirtualTableDesc();

  // Map the TrafDesc into an NATable. This will also add an
  // NATable entry into the bindWA's NATableDB.
  NATable *naTable =
    bindWA->getNATable(corrName, FALSE /*catmanUsages*/, tableDesc);
  if (bindWA->errStatus())
    return;

  // Allocate a TableDesc and attach it to this RelExpr instance
  setTableDesc(bindWA->createTableDesc(naTable, corrName));
  if (bindWA->errStatus())
    return;

  // Allocate a RETDesc and attach it to this and the BindScope
  setRETDesc(new (bindWA->wHeap()) RETDesc(bindWA, getTableDesc()));
  bindWA->getCurrentScope()->setRETDesc(getRETDesc());
}


RelExpr *ProxyFunc::bindNode(BindWA *bindWA)
{
  // This method now serves as a common bind node for SPProxy and 
  // ExtractSource classes, where we before had SPProxyFunc::bindNode()
  // and ExtractSource::bindNode().

  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // Bind the child nodes
  bindChildren(bindWA);
  if (bindWA->errStatus())
    return this;

  // Declare a correlation name that is unique within this query
  switch (getOperatorType())
  {
    case REL_EXTRACT_SOURCE:
      virtualTableName_ = "EXTRACT_SOURCE_";
      break;
    case REL_SP_PROXY:
      virtualTableName_ = "SP_RESULT_SET_";
      break;
    default:
         CMPASSERT(0);
      break;
  }

  virtualTableName_ += bindWA->fabricateUniqueName();
  CorrName corrName(getVirtualTableName());
  corrName.setSpecialType(ExtendedQualName::VIRTUAL_TABLE);
  
  createProxyFuncTableDesc(bindWA, corrName);
  if (bindWA->errStatus())
    return this;

  // Bind the base class
  RelExpr *boundExpr = bindSelf(bindWA);
  if (bindWA->errStatus())
    return boundExpr;

  // Assign the set of columns that belong to the virtual table
  // as the output values that can be produced by this node.
  getGroupAttr()->addCharacteristicOutputs(getTableDesc()->getColumnList());

  return boundExpr;

} // ProxyFunc::bindNode()

RelExpr *TableMappingUDF::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    {
      bindWA->getCurrentScope()->setRETDesc(getRETDesc());
      return this;
    }

  // Create NARoutine object (no caching for TMUDF)
  NARoutine *tmudfRoutine =NULL;
  CorrName& tmfuncName = getUserTableName();
  tmfuncName.setSpecialType(ExtendedQualName::VIRTUAL_TABLE);

  
  QualifiedName name = getRoutineName();
  const SchemaName &defaultSchema =
    bindWA->getSchemaDB ()->getDefaultSchema();
  name.applyDefaults(defaultSchema);
  setRoutineName(name);

  // Return an error if an unsupported catalog is being used.
  if ((NOT name.isSeabase()) && (NOT name.isHive()))
      {
	*CmpCommon::diags()
	  << DgSqlCode(-1002)
	  << DgCatalogName(name.getCatalogName());
	
	bindWA->setErrStatus();
	return NULL;
      }

  Lng32 diagsMark = CmpCommon::diags()->mark();
  NABoolean errStatus = bindWA->errStatus();

  tmudfRoutine = getRoutineMetadata(name, tmfuncName, bindWA);
  if (tmudfRoutine == NULL)
    {
      // this could be a predefined TMUDF, which is not
      // recorded in the metadata at this time
      OperatorTypeEnum opType =
        PredefinedTableMappingFunction::nameIsAPredefinedTMF(tmfuncName);

      if (opType != REL_ANY_TABLE_MAPPING_UDF)
        {
          // yes, this is a predefined TMUDF
          PredefinedTableMappingFunction *result;

          // discard the errors from the failed name lookup
          CmpCommon::diags()->rewind(diagsMark);
          if (!errStatus)
            bindWA->resetErrStatus();

          // create a new RelExpr
          result = new(bindWA->wHeap())
            PredefinedTableMappingFunction(
                 getArity(),
                 tmfuncName,
                 const_cast<ItemExpr *>(getProcAllParamsTree()),
                 opType);

          // copy data members of the base classes
          TableMappingUDF::copyTopNode(result);

          // set children
          result->setArity(getArity());
          for (int i=0; i<getArity(); i++)
            result->child(i) = child(i);

          if (opType == REL_TABLE_MAPPING_BUILTIN_LOG_READER ||
              opType == REL_TABLE_MAPPING_BUILTIN_JDBC)
            {
              // The event log reader and JDBC TMUDFs are being migrated
              // to real UDFs, use of the predefined UDFs is deprecated.
              // Issue a warning. Eventually these predefined functions
              // will be removed.
              (*CmpCommon::diags())
                << DgSqlCode(4323)
                << DgString0(tmfuncName.getExposedNameAsAnsiString());
            }

          // Abandon the current node and return the bound new node.
          // Next time it will reach this method it will call an
          // overloaded getRoutineMetadata() that will succeed.
          return result->bindNode(bindWA);
        }

      // getRoutineMetadata has already set the diagnostics area
      // and set the error status
      CMPASSERT(bindWA->errStatus());
      return NULL;
    }

  // Bind the child nodes.
  bindChildren(bindWA);
  if (bindWA->errStatus())
    return this;

  // Use information from child to populate childInfo_
  NAHeap *heap = CmpCommon::statementHeap();
  for(Int32 i = 0; i < getArity(); i++)
  {
    NAString childName(heap);
    NAColumnArray childColumns(heap) ;
    RETDesc *childRetDesc = child(i)->getRETDesc();
    
    // Get Name
    LIST(CorrName*) nameList(STMTHEAP);
    childRetDesc->getXTNM().dumpKeys(nameList);
    if (nameList.entries() == 1)
    {
      childName = (nameList[0])->getExposedNameAsString();
    }
    else
    {
      childName = "_inputTable" + bindWA->fabricateUniqueName();
    }
    
    // ask for histograms of all child outputs, since we don't
    // know what the UDF will need and what predicates exist
    // on passthru columns of the UDF
    bindWA->getCurrentScope()->context()->inWhereClause() = TRUE;

    // Get NAColumns
    
    CollIndex numChildCols = childRetDesc->getColumnList()->entries();
    for(CollIndex j=0; j < numChildCols; j++)
    {
      const NAType &childColType = childRetDesc->getType(j);
      const NAType *adjustedChildColType = &childColType;
      NAColumn * childCol = NULL;

      if (childColType.getSimpleTypeName() == "NUMERIC" &&
          getBinaryStorageSize(childColType.getPrecision()) !=
          childColType.getNominalSize())
        {
          // In the optimizer code, we may choose different storage
          // sizes for the same type, for example a NUMERIC(1,0)
          // sometimes has one byte, sometimes 2 bytes of storage. For
          // examples, see NumericType::synthesizeType() and CQD
          // TRAF_CREATE_TINYINT_LITERAL.

          // In the TMUDF code, on the other hand, we compute the
          // storage size from the precision. So, make sure we follow
          // the TMUDF rules here, when we describe its input table
          adjustedChildColType = new(heap) SQLNumeric(heap,
               getBinaryStorageSize(childColType.getPrecision()),
               childColType.getPrecision(),
               childColType.getScale(),
               ((const NumericType &) childColType).isSigned(),
               childColType.supportsSQLnull());
        }

      childCol = new (heap) NAColumn(
        childRetDesc->getColRefNameObj(j).getColName().data(),
        j,
        adjustedChildColType->newCopy(heap),
        heap);
      childColumns.insert(childCol);

      bindWA->markAsReferencedColumn(childRetDesc->getValueId(j));
    }
    bindWA->getCurrentScope()->context()->inWhereClause() = FALSE;

    // get child root
    CMPASSERT(child(i)->getOperator().match(REL_ROOT) ||
      child(i)->getOperator().match(REL_RENAME_TABLE));
    RelRoot * myChild;
    if (child(i)->getOperator().match(REL_RENAME_TABLE))
      myChild = (RelRoot *) (child(i)->child(0).getPtr());
    else
      myChild = (RelRoot *) child(i).getPtr();

    // output vidList from child RetDesc, 
    // can also get from child Root compExpr
    ValueIdList vidList;
    childRetDesc->getValueIdList(vidList, USER_COLUMN);
    ValueIdSet childPartition(myChild->partitionArrangement());
    ValueIdList childOrder(myChild->reqdOrder());

    // request multi-column histograms for the PARTITION BY columns
    bindWA->getCurrentScope()->context()->inGroupByClause() = TRUE;

    // replace 1-based ordinals in the child's partition by / order by with
    // actual columns
    for (ValueId cp=childPartition.init();
         childPartition.next(cp);
         childPartition.advance(cp))
      {
        NABoolean negate;
        ConstValue *cv = cp.getItemExpr()->castToConstValue(negate);

        if (cv &&
            cv->canGetExactNumericValue())
        {
          Lng32 scale = 0;
          Int64 ordinal = cv->getExactNumericValue(scale);

          if (!negate && scale == 0 && ordinal >= 1 && ordinal <= vidList.entries())
            {
              // remove this ValueId from the set and add the corresponding
              // column value. Note that this won't cause problems with the
              // iterator through the set, since we don't need to apply
              // this conversion on the new element we are inserting
              childPartition -= cp;
              childPartition += vidList[ordinal-1];
            }
          else
            {
              *CmpCommon::diags()
                << DgSqlCode(-11154)
                << DgInt0(ordinal)
                << DgString0("PARTITION BY")
                << DgInt1(vidList.entries());
              bindWA->setErrStatus();
              return NULL;
            }
        }
        bindWA->markAsReferencedColumn(cp);
      }
    bindWA->getCurrentScope()->context()->inGroupByClause() = FALSE;

    for (CollIndex co=0; co<childOrder.entries(); co++)
      {
        NABoolean negate;
        ItemExpr *ie = childOrder[co].getItemExpr();
        ConstValue *cv = NULL;

        if (ie->getOperatorType() == ITM_INVERSE)
          ie = ie->child(0);
        cv = ie->castToConstValue(negate);

        if (cv &&
            cv->canGetExactNumericValue())
        {
          Lng32 scale = 0;
          Int64 ordinal = cv->getExactNumericValue(scale);

          // replace the const value with the actual column
          if (!negate && scale == 0 && ordinal >= 1 && ordinal <= vidList.entries())
            if (ie == childOrder[co].getItemExpr())
              {
                // ascending order
                childOrder[co] = vidList[ordinal-1];
              }
            else
              {
                // desc order, need to add an InverseOrder on top
                ItemExpr *inv = new(bindWA->wHeap()) InverseOrder(
                     vidList[ordinal-1].getItemExpr());
                inv->synthTypeAndValueId();
                childOrder[co] = inv->getValueId();
              }
          else
            {
              *CmpCommon::diags()
                << DgSqlCode(-11154)
                << DgInt0(ordinal)
                << DgString0("ORDER BY")
                << DgInt1(vidList.entries());
              bindWA->setErrStatus();
              return NULL;
            }
        }
      }

    TableMappingUDFChildInfo * cInfo = new (heap) TableMappingUDFChildInfo(
      childName, 
      childColumns,
      myChild->getPartReqType(), 
      childPartition,
      childOrder,
      vidList);
    childInfo_.insert(cInfo);
  }

  RoutineDesc *tmudfRoutineDesc = new (bindWA->wHeap()) RoutineDesc(bindWA, tmudfRoutine); 
  if (tmudfRoutineDesc == NULL ||  bindWA->errStatus ())
  {
    // Error
    bindWA->setBindingCall (FALSE);
    bindWA->setErrStatus ();
    return this;
  }
  setRoutineDesc(tmudfRoutineDesc);
  // xcnm will be empty because the routineDesc does not contain any 
  // output columns yet
  RETDesc *rDesc = new (bindWA->wHeap()) RETDesc(bindWA, tmudfRoutineDesc);
  bindWA->getCurrentScope()->setRETDesc(rDesc);
  setRETDesc(rDesc);

  dllInteraction_ = new (bindWA->wHeap()) TMUDFDllInteraction();

  // ValueIDList of the actual input parameters 
  // (tmudfRoutine has formal parameters)
  if (getProcAllParamsTree() && (getProcAllParamsVids().isEmpty() == TRUE)) 
  {
      ((ItemExpr *)getProcAllParamsTree())->convertToValueIdList(
		  getProcAllParamsVids(), bindWA, ITM_ITEM_LIST);
      if (bindWA->errStatus()) return NULL;
      
      // Clear the Tree since we now have gotten vids for all the parameters.
      setProcAllParamsTree(NULL);
  }
  getProcInputParamsVids().insert(getProcAllParamsVids());

  // invoke the optional UDF compiler interface or a default
  // implementation to validate scalar inputs and produce a list of
  // output columns

  NABoolean status = dllInteraction_->describeParamsAndMaxOutputs(this, bindWA);
  if (!status)
    {
      bindWA->setErrStatus();
      return NULL;
    }

  checkAndCoerceScalarInputParamTypes(bindWA);
  if (bindWA->errStatus())
    return NULL;

  createOutputVids(bindWA);
  if (bindWA->errStatus())
    return NULL;

  // create a ValueIdMap that allows us to translate
  // output columns that are passed through back to
  // input columns (outputs of the child), this can
  // be used to push down predicates, translate
  // required order and partitioning, etc.
  status = dllInteraction_->createOutputInputColumnMap(
       this,
       udfOutputToChildInputMap_);
  if (!status)
    {
      bindWA->setErrStatus();
      return NULL;
    }

  // add the routine to the UdrStoiList.  The UdrStoi list is used
  // to check valid privileges
  LIST(OptUdrOpenInfo *) udrList = bindWA->getUdrStoiList ();
  ULng32 numUdrs = udrList.entries();
  NABoolean udrReferenced = FALSE;

  // See if UDR already exists
  for (ULng32 stoiIndex = 0; stoiIndex < numUdrs; stoiIndex++)
  {
    if ( 0 ==
         udrList[stoiIndex]->getUdrName().compareTo(
                           getRoutineName().getQualifiedNameAsAnsiString()
                                                  )
       )
    {
      udrReferenced = TRUE;
      break;
    }
  }

  // UDR has not been defined, go ahead an add one
  if ( FALSE == udrReferenced )
  {
    SqlTableOpenInfo *udrStoi = new (bindWA->wHeap ())SqlTableOpenInfo ();
    udrStoi->setAnsiName ( convertNAString(
                           getRoutineName().getQualifiedNameAsAnsiString(),
                           bindWA->wHeap ())
                         );

    OptUdrOpenInfo *udrOpenInfo = new (bindWA->wHeap ())
      OptUdrOpenInfo( udrStoi
                    , getRoutineName().getQualifiedNameAsAnsiString()
                    , (NARoutine *)getNARoutine()
                    );
    bindWA->getUdrStoiList().insert(udrOpenInfo);
  }

  RelExpr *boundExpr = bindSelf(bindWA);
  if (bindWA->errStatus())
    return NULL;
  return boundExpr;
}

RelExpr * FastExtract::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // check validity of target location
  if (getTargetType() == FILE)
  {
    char reasonMsg[256];
    NABoolean raiseError = FALSE;
    if ((unsigned char)(getTargetName().data()[0]) != SLASH_C)
    {
      raiseError = TRUE;
      sprintf(reasonMsg,"Relative path name was used");
    }
    else if (getTargetName().length() > 512)
    {
      raiseError = TRUE;
      sprintf(reasonMsg,"Length exceeds 512 characters");
    }
    else
    {
      char * sqroot = getenv("TRAF_HOME");
      if (sqroot && (! CmpCommon::context()->getSqlmxRegress()) &&
          (strncmp(sqroot, getTargetName().data(),strlen(sqroot)) == 0))
      {
        raiseError = TRUE;
        sprintf(reasonMsg,"Database system directory was used");
      }
    }
    if (raiseError  && strncmp(getTargetName().data(),"hdfs://",7) != 0 )
    {
      *CmpCommon::diags() << DgSqlCode(-4378) << DgString0(reasonMsg) ;
      bindWA->setErrStatus();
      return NULL;
    }
  }

  if (getDelimiter().length() == 0)
  {
    delimiter_ = ActiveSchemaDB()->getDefaults().getValue(TRAF_UNLOAD_DEF_DELIMITER);
  }

  // if inserting into a hive table and an explicit null string was
  // not specified in the unload command, and the target table has a user
  // specified null format string, then use it.
  const HHDFSTableStats* hTabStats = NULL;
  if ((isHiveInsert()) &&
      (hiveTableDesc_ && hiveTableDesc_->getNATable() && 
       hiveTableDesc_->getNATable()->getClusteringIndex()))
    {
      hTabStats = 
        hiveTableDesc_->getNATable()->getClusteringIndex()->getHHDFSTableStats();
      
      if (NOT nullStringSpec_)
        {
          if (hTabStats->getNullFormat())
            {
              nullString_ = hTabStats->getNullFormat();
              nullStringSpec_ = TRUE;
            }
        }
    }

  // if an explicit or user specified null format was not used, then
  // use the default null string.
  if (NOT nullStringSpec_) 
    {
      nullString_ = HIVE_DEFAULT_NULL_STRING;
    }
  
  if (getRecordSeparator().length() == 0)
  {
    recordSeparator_ = ActiveSchemaDB()->getDefaults().getValue(TRAF_UNLOAD_DEF_RECORD_SEPARATOR);
  }

  if (!isHiveInsert())
  {
    bindWA->setIsFastExtract();
  }

  ValueIdList tgtColList;
  TupleList *tl = NULL;
  if ((isHiveInsert() && hTabStats && hTabStats->isTextFile()) &&
      (child(0) && child(0)->getOperatorType() == REL_TUPLE_LIST))
  {
    // if my child is a TupleList, then all tuples are to be converted/cast
    // to the corresponding target type of the tgtColList.
    // Pass on the tgtColList to TupleList so it can generate the Cast nodes
    // with the target types during the TupleList::bindNode.
    hiveTableDesc_->getUserColumnList(tgtColList);
    tl = (TupleList *)child(0)->castToRelExpr();
    tl->castToList() = tgtColList;

    tl->setCastTo(TRUE);
    tl->setHiveTextInsert(TRUE);
  }

  // Bind the child nodes.
  bindChildren(bindWA);
  if (bindWA->errStatus())
    return this;

  // Use information from child to populate childInfo_
  NAHeap *heap = CmpCommon::statementHeap();

  RETDesc *childRETDesc = child(0)->getRETDesc();
  // output vidList from child RetDesc,
  // can also get from child Root compExpr
  ValueIdList vidList;
  childRETDesc->getValueIdList(vidList, USER_COLUMN);

  if (isHiveInsert())
    {
      // validate number of columns and column types of the select list
      ValueIdList tgtCols;

      hiveTableDesc_->getUserColumnList(tgtCols);

      if (vidList.entries() != tgtCols.entries())
        {
          // 4023 degree of row value constructor must equal that of target table
          *CmpCommon::diags() << DgSqlCode(-4023)
                              << DgInt0(vidList.entries())
                              << DgInt1(tgtCols.entries());
          bindWA->setErrStatus();
          return NULL;
        }

      // Check that the source and target types are compatible.
      for (CollIndex j=0; j<vidList.entries(); j++)
        {
          Assign *tmpAssign = new(bindWA->wHeap())
            Assign(tgtCols[j].getItemExpr(), vidList[j].getItemExpr());

          if ( CmpCommon::getDefault(ALLOW_IMPLICIT_CHAR_CASTING) == DF_ON )
            tmpAssign->tryToDoImplicitCasting(bindWA);
          const NAType *targetType = tmpAssign->synthesizeType();
          if (!targetType) {
            bindWA->setErrStatus();
            return NULL;
          }
        }
    }
  else 
    {
      NABoolean hasLob = FALSE;
      CollIndex i = 0;
      for (i = 0; (i < vidList.entries() && !hasLob); i++)
	hasLob = vidList[i].getType().isLob();
      if (hasLob) {
	*CmpCommon::diags() << DgSqlCode(-4495) 
			    << DgColumnName(childRETDesc->getColRefNameObj(i).
					    getColRefAsAnsiString());
	bindWA->setErrStatus();
	return NULL;
      }
      
    }
    

  setSelectList(vidList);

  if (includeHeader())
  {
    const ColumnDescList &columnsRET = *(childRETDesc->getColumnList());
    for (CollIndex i = 0; i < columnsRET.entries(); i++)
    {
      if (columnsRET[i]->getHeading())
        header_ += columnsRET[i]->getHeading();
      else if (!(columnsRET[i]->getColRefNameObj().isEmpty()))
        header_ += columnsRET[i]->getColRefNameObj().getColName();
      else
        header_ += "EXPR";

      if (i < (columnsRET.entries() -1))
      {
        header_ += " ";
        header_ += delimiter_;
        header_ += " ";
      }
    }
  }
  else
  {
          header_ = "NO_HEADER" ;
  }

  // no rows are returned from this operator.
  // Allocate an empty RETDesc and attach it to this and the BindScope.
  setRETDesc(new (bindWA->wHeap()) RETDesc(bindWA));

  RelExpr *boundExpr = bindSelf(bindWA);
  if (bindWA->errStatus()) return NULL;
  return boundExpr;
}


RelExpr * ControlRunningQuery::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }
  //
  // Check to see if user is authorized to control this query.
  //
  if (!isUserAuthorized(bindWA))
     return NULL;

  //
  // Bind the child nodes.
  //
  bindChildren(bindWA);
  if (bindWA->errStatus())
    return this;

  // no rows are returned from this operator. 
  // Allocate an empty RETDesc and attach it to this and the BindScope.
  //
  setRETDesc(new (bindWA->wHeap()) RETDesc(bindWA));

  //
  // Bind the base class.
  //
  RelExpr *boundExpr = bindSelf(bindWA);
  if (bindWA->errStatus()) 
    return boundExpr;


  ValueIdSet ov;
  getPotentialOutputValues(ov);
  getGroupAttr()->addCharacteristicOutputs(ov);

  return boundExpr;
} // ControlRunningQuery::bindNode()


bool ControlRunningQuery::isUserAuthorized(BindWA *bindWA)
{
  bool userHasPriv = false;
  Int32 sessionID = ComUser::getSessionUser();

  // Check to see if the current user owns the query id.
  // This only has to be done for the Cancel query request.
  // This option to check privilege is not available unless
  // the query Id was supplied.
  if ((action_ == Cancel) &&
      (qs_ == ControlQid))
  {
    // The user ID associated with the query is stored in the QID. 
    // To be safe, copy the QID to a character string.
    Int32 qidLen = queryId_.length();
    char *pQid = new (bindWA->wHeap()) char[qidLen+1];
    str_cpy_all(pQid, queryId_.data(), qidLen);
    pQid[qidLen] = '\0';

    // Set up the returned parameters
    // Max username can be (128 * 2) + 2 (delimiters) + 1 (null indicator)
    char username[2 * MAX_USERNAME_LEN + 2 + 1];
    Int64 usernameLen = sizeof(username) - 1;
 
    // Call function to extract the username from the QID
    Int32 retcode = ComSqlId::getSqlQueryIdAttr(ComSqlId::SQLQUERYID_USERNAME,
                                                pQid,
                                                qidLen,
                                                usernameLen,
                                                &username[0]);
    if (retcode == 0)
    {
      // The username stored in the QID is actually the userID preceeded with
      // a "U".  Check for a U and convert the succeeding characters 
      // to integer.  This integer value is compared against the current userID.
      username[usernameLen] = '\0';  
      if (username[0] == 'U')
      {
        Int64 userID = str_atoi(&username[1],usernameLen - 1);  
        if (sessionID == userID || sessionID == ComUser::getRootUserID())
          userHasPriv = true;
      }
      // If userName does not begin with a 'U', ignore and continue
    }
    // If retcode != 0, continue, an invalid QID could be specified which
    // is checked later in the code
  }
  
  // The current user does not own the query, see if the current user has 
  // the correct QUERY privilege.  Code above only supports cancel, but other 
  // checks could be added.  Component checks for all query operations.
  if (!userHasPriv)
  {
    SQLOperation operation;
    switch (ControlRunningQuery::action_)
    {
      case ControlRunningQuery::Suspend:
        operation = SQLOperation::QUERY_SUSPEND;
        break;
      case ControlRunningQuery::Activate:
        operation = SQLOperation::QUERY_ACTIVATE;
        break;
      case ControlRunningQuery::Cancel:
        operation = SQLOperation::QUERY_CANCEL;
        break;
      default:
        operation = SQLOperation::UNKNOWN;
    }
    
    NAString privMDLoc = CmpSeabaseDDL::getSystemCatalogStatic();
    privMDLoc += ".\"";
    privMDLoc += SEABASE_PRIVMGR_SCHEMA;
    privMDLoc += "\"";

    PrivMgrComponentPrivileges componentPriv(
      privMDLoc.data(),CmpCommon::diags());

    userHasPriv = componentPriv.hasSQLPriv(sessionID,operation,true);
    
    if (!userHasPriv)
    {
      // ANSI requests a special SqlState for cancel requests
      if (ControlRunningQuery::action_ == ControlRunningQuery::Cancel)
        *CmpCommon::diags() << DgSqlCode(-8029);
      else
        *CmpCommon::diags() << DgSqlCode(-1017);
      bindWA->setErrStatus();
    }
    
    if (bindWA->errStatus())
      return false;
  }
  return true;
  
}// ControlRunningQuery::isUserAuthorized()

RelExpr * CommonSubExprRef::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  CSEInfo *info = CmpCommon::statement()->getCSEInfo(internalName_);
  CommonSubExprRef *parentCSE = bindWA->inCSE();

  DCMPASSERT(info);

  // eliminate any CommonSubExprRef nodes that are not truly common,
  // i.e. those that are referenced only once
  if (info->getNumConsumers() <= 1)
    {
      info->eliminate();
      return child(0).getPtr()->bindNode(bindWA);
    }

  bindWA->setInCSE(this);

  // establish the parent/child relationship
  addParentRef(parentCSE);

  bindChildren(bindWA);
  if (bindWA->errStatus())
    return this;

  // we know that our child is a RenameTable (same name as this CSE,
  // whose child is a RelRoot, defining the CTE. Copy the bound select
  // list of the CTE.
  CMPASSERT(child(0)->getOperatorType() == REL_RENAME_TABLE &&
            child(0)->child(0)->getOperatorType() == REL_ROOT);
  columnList_ = static_cast<RelRoot *>(child(0)->child(0).getPtr())->compExpr();

  bindWA->setInCSE(parentCSE);

  return bindSelf(bindWA);
}

RelExpr * OSIMControl::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
  {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }
  //Create OptimizerSimulator if this is called first time.
  if(!CURRCONTEXT_OPTSIMULATOR)
      CURRCONTEXT_OPTSIMULATOR = new(CTXTHEAP) OptimizerSimulator(CTXTHEAP);
      
  //in respond to force option of osim load, 
  //e.g. osim load from '/xxx/xxx/osim-dir', force
  //if true, when loading osim tables/views/indexes
  //existing objects with same qualified name 
  //will be droped first
  CURRCONTEXT_OPTSIMULATOR->setForceLoad(isForceLoad());
  //Set OSIM mode
  if(!CURRCONTEXT_OPTSIMULATOR->setOsimModeAndLogDir(targetMode_, osimLocalDir_.data()))
  {
      bindWA->setErrStatus();
      return this;
  }

  return ControlAbstractClass::bindNode(bindWA);
}

