// **********************************************************************
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
// **********************************************************************

#include "MVCandidates.h"
#include "QRDescGenerator.h"
#include "Analyzer.h"
#include "NormWA.h"
#include "parser.h"
#include "RelJoin.h"
#include "ItemLog.h"
#include "RelUpdate.h"
#include "ItemOther.h"
#include "MVInfo.h"

#ifdef NA_DEBUG_GUI
  void initializeGUIData(SqlcmpdbgExpFuncs* expFuncs);
#endif

// Prefix for correlation names used for backjoin tables.
const char* const MVCandidates::BACKJOIN_CORRNAME_PREFIX = "BACKJOIN@";

static ULng32 nodeIdHashFn(const CollIndex& i) { return i; }
static ULng32 bjScanHashFn(const NAString& s)  { return NAString::hash(s); }
static OperatorTypeEnum determineAggForRollup(QRMVColumnPtr mvCol);

MVCandidates::MVCandidates(RelRoot* root,
                           QRQueryDescriptorPtr queryDesc,
                           QRDescGenerator& descGenerator)
  : bindWA_(root->getRETDesc()->getBindWA()),
    queryRoot_(root),
    queryDesc_(queryDesc),
    qdescGenerator_(descGenerator),
    mvqrFavoriteCandidates_(NULL),
    forbiddenMVs_(NULL),
    bjScanHash_(bjScanHashFn, 17, TRUE, QueryAnalysis::Instance()->outHeap())
{}

Int32 MVCandidates::nodeCount(ExprNode* node)
{
  // Recurse for each child, and add one for the current node.
  Int32 nodes = 0;
  for (Int32 i=0; i<node->getArity(); i++)
    {
      nodes += nodeCount(node->getChild(i));
    }

  return nodes + 1;
}

// extract the list of candidate MVs from the value of the MVQR_REWRITE_CANDIDATES CQD
void MVCandidates::buildFavoritesList (CollHeap* heap)
{
  NAString MVsStr = "";
  CmpCommon::getDefault(MVQR_REWRITE_CANDIDATES, MVsStr);
  if (MVsStr.length() > 0 )   // have any
  {
     // initialize mvqr favorites list
     if (mvqrFavoriteCandidates_ == NULL)
       mvqrFavoriteCandidates_ = new (heap) LIST(NAString)(heap);

      // prepare to extract the partitions/tokens from the default string
     char *token;
     const char* sep = " ,:" ;
      token = strtok( const_cast<char*> (MVsStr.data()), sep );
      while ( token != NULL )
      {
          mvqrFavoriteCandidates_->insert (token);
          token = strtok( NULL , sep );  // get next token
      } // end of while( token )
  }
}

// root is not necessarily a RelRoot, just the root node of the subtree to be
// displayed.


ItemExpr* MVCandidates::parseExpression(QRExprPtr expr, const NAString& mvName, CollHeap* heap)
{
  return expr->getExprRoot()->toItemExpr(mvName, heap, &bjScanHash_);
}

GroupByAgg* MVCandidates::getGroupByAggNode(RelExpr* childNode,
                                            QRGroupByPtr groupBy,
                                            QRCandidatePtr candidate,
                                            CollHeap* heap)
{
  GroupByAgg* groupByAgg = new(heap) GroupByAgg(childNode, REL_GROUPBY,
                                                NULL, NULL, heap);
  // Check for the Aggregate with no GroupBy case.
  if (groupBy->isEmpty())
    return groupByAgg;

  const ElementPtrList& groupItems = groupBy->getPrimaryList()->getList();
  for (CollIndex i=0; i<groupItems.entries(); i++)
    {
      QRElementPtr groupItem = groupItems[i];
      ElementType elemType = groupItem->getElementType();
      if (elemType == ET_MVColumn)
        {
          QRMVColumnPtr mvcol = groupItem->downCastToQRMVColumn();
          groupByAgg->addGroupExprTree(
              new(heap) ColReference(getColRefName(candidate, mvcol, heap)));
        }
      else if (elemType == ET_Column)
        {
          QRColumnPtr col = groupItem->getReferencedElement()->downCastToQRColumn();
          groupByAgg->addGroupExprTree(
                   new(heap) ColReference(getColRefName(candidate, col, heap)));
        }
      else if (elemType == ET_Expr)
        groupByAgg->addGroupExprTree(parseExpression
                                        (groupItem->downCastToQRExpr(),
                                       	 candidate->getMVName()->getMVName(),
                                         heap));
      else
        assertLogAndThrow1(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                           FALSE, MVCandidateException,
			   "Unexpected element type for grouping item -- %d",
			   elemType)
    }

  return groupByAgg;
}

void MVCandidates::getEqSetMembers(VEGPredicate* vegPred, ValueIdSet& vegMembers)
{
  ValueId vegrefVid = vegPred->getVEG()->getVEGReference()->getValueId();
  const NAList<EqualitySet*>& eqSets = qdescGenerator_.getAllEqualitySets();
  NABoolean found = FALSE;
  for (CollIndex i=0; !found && i<eqSets.entries(); i++)
    {
      if (eqSets[i]->getJoinPredId() == vegrefVid)
        {
          EqualitySet& eqSet = *eqSets[i];
          for (CollIndex j=0; j<eqSet.entries(); j++)
            vegMembers.insert(eqSet[j]->getValueId());
          found = TRUE;
        }
    }
}

ItemExpr* MVCandidates::rewriteVegPredicate(const ElementPtrList& mvColumns,
                                            VEGPredicate* vegPred,
                                            QRCandidatePtr candidate,
                                            CollHeap* heap)
{
  ItemExpr* andBackbone = NULL;
  ItemExpr* prevOperand = NULL;
  ItemExpr* operand;
  ItemExpr* eqPred;
  QRMVColumnPtr mvCol;
  //const ValueIdSet& vegMembers = vegPred->getVEG()->getAllValues();
  ValueIdSet vegMembers;
  getEqSetMembers(vegPred, vegMembers);

  for (ValueId memVid=vegMembers.init();
       vegMembers.next(memVid);
       vegMembers.advance(memVid))
    { 
      // memVid is the valueid of the current veg member
      operand = NULL;
      for (CollIndex i=0; i<mvColumns.entries() && !operand; i++)
        {
          switch (mvColumns[i]->getElementType())
            {
              case ET_MVColumn:
                mvCol = mvColumns[i]->downCastToQRMVColumn();
                if (mvCol->hasRefTo(memVid))
                  operand = new(heap) ColReference(
                                   getColRefName(candidate, mvCol, heap));
                break;

              default:
                assertLogAndThrow1(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                                   FALSE, MVCandidateException,
                                   "No code for handling element of type %d in "
                                   "vegpred yet", mvColumns[i]->getElementType());
            }
        }

      // If we didn't find a replacement, just make a copy of the member's
      // itemexpr.
      if (!operand)
        {
          if (memVid.getItemExpr()->getOperatorType() == ITM_BASECOLUMN)
            continue;
          else
            operand = memVid.getItemExpr()->copyTopNode(0, heap);
        }

      if (prevOperand)
        {
          eqPred = new(heap) BiRelat(ITM_EQUAL, prevOperand, operand);
          if (andBackbone)
            andBackbone = new(heap) BiRelat(ITM_AND, andBackbone, eqPred);
          else
            andBackbone = eqPred;
        }
      prevOperand = operand;
    }

  assertLogAndThrow(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                    andBackbone, MVCandidateException,
                    "rewriteVegPredicate returning NULL");
  return andBackbone;
} // rewriteVegPredicate

ItemExpr* MVCandidates::rewriteItemExpr(const ElementPtrList& mvColumns,
                                        ItemExpr* itemExpr,
                                        QRCandidatePtr candidate,
                                        CollHeap* heap,
                                        CollIndex tblId)
{
  // Copy the ItemExpr as we rewrite it; the original is currently
  // owned by a node in the jbbsubset being replaced, and we would have
  // to figure out which one owns it to take ownership from it. Each node
  // in the copy is marked as unbound.
  ItemExpr* replacement = NULL;
  QRMVColumnPtr mvCol;
  QRColumnPtr col;
  QRElementPtr colRefedElem;
  Int32 numChildren = itemExpr->getArity();
  if (numChildren == 0)
    {
      // This happens when there is an equality predicate on a column that is
      // used in an equijoin, e.g., t1.a=t12.a and t1.a=6.
      if (itemExpr->getOperatorType() == ITM_VEG_PREDICATE)
        return rewriteVegPredicate(mvColumns, 
                                   static_cast<VEGPredicate*>(itemExpr),
                                   candidate, heap);

      // Check each col/mvcol to see if it replaces this leaf node.
      for (CollIndex i=0; i<mvColumns.entries() && !replacement; i++)
        {
          switch (mvColumns[i]->getElementType())
            {
              case ET_MVColumn:
                mvCol = mvColumns[i]->downCastToQRMVColumn();
                if (itemExpr->containsTheGivenValue(mvCol->getRefNum()))
                  {
                    replacement = new(heap) ColReference(
                                           getColRefName(candidate, mvCol, heap));
		    OperatorTypeEnum agg = NO_OPERATOR_TYPE;
		    if (isRollup(candidate))
		      agg = determineAggForRollup(mvCol);
                    if (agg != NO_OPERATOR_TYPE)
                      replacement = new(heap) Aggregate(agg, replacement);
                  }
                break;

              case ET_Column:
              case ET_JoinPred:
                colRefedElem = mvColumns[i]->getReferencedElement();
                if (colRefedElem->getElementType() == ET_Column)
                  {
                    col = colRefedElem->downCastToQRColumn();
                    if (itemExpr->containsTheGivenValue(col->getIDNum()))
                      replacement=new(heap)ColReference(getColRefName(candidate,
                                                                      col, heap));
                  }
                else
                  {
                    QRJoinPredPtr jp = colRefedElem->downCastToQRJoinPred();
                    const ElementPtrList& eqList = jp->getEqualityList();
                    NABoolean foundOne = FALSE;
                    NABoolean foundRightOne = FALSE;
                    for (CollIndex i=0; !foundRightOne && i<eqList.entries(); i++)
                      {
                        if (eqList[i]->getElementType() == ET_Column)
                          {
                            colRefedElem = eqList[i]->getReferencedElement();
                            if (colRefedElem->getElementType() == ET_Column)
                              {
                                col = colRefedElem->downCastToQRColumn();
                                if (itemExpr->containsTheGivenValue(col->getIDNum()))
                                {
                                  foundOne = TRUE;
                                  if (tblId == 0 ||
                                      tblId == col->getTableIDNum())
                                    {
                                      foundRightOne = TRUE;
                                      replacement = new(heap)
                                          ColReference(getColRefName(candidate,
                                                                     col, heap));
                                    }
                                }
                              }
                          }
                      }

                    // We used to pick the first one, now we only accept one from
                    // the correct node. This should still find a member for any
                    // case where it did before.
                    assertLogAndThrow(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                                      !foundOne || foundRightOne,
                                      MVCandidateException,
                                      "Found vegref member, but not one to "
                                      "match target node.");
                  }
                break;

              case ET_Expr:
                // A col may replace an expr, but not vice versa.
                break;

              default:
                assertLogAndThrow1(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                                   FALSE, MVCandidateException,
                                   "rewriteItemExpr() not ready for element of type %d",
                                   mvColumns[i]->getElementType());
                break;
            }
        }

      // If no replacement col/mvcol found, just copy the node.
      if (!replacement)
        {
          if (itemExpr->getOperatorType() == ITM_VEG_REFERENCE)
            {
              // copyTopNode() not defined for vegref, use one of its members.
              // Use a constant if available. There are cases where a constatn-
              // quated column is not available in the MV.
              VEGReference* vegref = static_cast<VEGReference*>(itemExpr);
              ValueId someMemberId = vegref->getVEG()->getAConstant(TRUE);
              if (someMemberId == NULL_VALUE_ID)
                {
                  const ValueIdSet& vegMembers = vegref->getVEG()->getAllValues();
                  someMemberId = vegMembers.init();
                  vegMembers.next(someMemberId);
                }
              replacement = someMemberId.getItemExpr()->copyTopNode();
            }
          else
            replacement = itemExpr->copyTopNode();
        }
    }
  else  // >0 children
    {
      // See if an mv column matches the entire expression.
      for (CollIndex i=0; i<mvColumns.entries() && !replacement; i++)
        {
          switch (mvColumns[i]->getElementType())
            {
              case ET_MVColumn:
                mvCol = mvColumns[i]->downCastToQRMVColumn();
                if ((NumericID)itemExpr->getValueId() == mvCol->getRefNum())
                  {
                    replacement = new(heap) ColReference(
                                             getColRefName(candidate, mvCol, heap));
		    OperatorTypeEnum agg = NO_OPERATOR_TYPE;
		    if (isRollup(candidate))
		      agg = determineAggForRollup(mvCol);
                    if (agg != NO_OPERATOR_TYPE)
                      replacement = new(heap) Aggregate(agg, replacement);
                  }
                break;

              case ET_Expr:
                if ((NumericID)itemExpr->getValueId() == mvColumns[i]->getRefNum())
                  replacement = parseExpression(mvColumns[i]->downCastToQRExpr(),
                                       	        candidate->getMVName()->getMVName(),
                                                heap);
                break;

              default:
                break;
            }
        }

      if (!replacement)
    {
      replacement = itemExpr->copyTopNode();
      ItemExpr* newChild;
      for (Int32 i=0; i<numChildren; i++)
        {
          newChild = rewriteItemExpr(mvColumns, itemExpr->child(i),
                                     candidate, heap, tblId);
          if (newChild)
            replacement->setChild(i, newChild);
        }
    }
    }

  replacement->markAsUnBound();
  return replacement;
}

ItemExpr* MVCandidates::rewriteItemExpr(QRElementPtr mvCol,  // could be col or mvcol
                                        ItemExpr* itemExpr,
                                        QRCandidatePtr candidate,
                                        CollHeap* heap)
{
  ElementPtrList mvColList(heap);
  mvColList.insert(mvCol);
  return rewriteItemExpr(mvColList, itemExpr, candidate, heap);
}

static OperatorTypeEnum determineAggForRollup(QRMVColumnPtr mvCol)
{
  OperatorTypeEnum result = mvCol->getAggForRewrite();
  // If already initialized, return result.
  if (result != ITM_NOT)
    return result;

  ItemExpr* itemExpr = ((ValueId)mvCol->getRefNum()).getItemExpr();
  if (itemExpr->isAnAggregate())
    {
      switch (itemExpr->getOperatorType())
        {
          case ITM_MAX:
            result = ITM_MAX;
	    break;

          case ITM_MIN:
            result = ITM_MIN;
	    break;

          case ITM_SUM:
          case ITM_COUNT:
          case ITM_COUNT_NONULL:
            result = ITM_SUM;
	    break;

          default: 
            assertLogAndThrow1(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                              FALSE, MVCandidateException,
                               "Can't rewrite this agg func for rollup: %d",
                               itemExpr->getOperatorType());
        }
    }
  else
    result = NO_OPERATOR_TYPE;

  mvCol->setAggForRewrite(result);
  return result;
} 

void MVCandidates::rewriteRangePreds(QRCandidatePtr candidate,
                                     RelExpr* scan,
                                     RelExpr* groupBy,
                                     CollHeap* heap)
{
  ValueId rangePredValueId;
  ItemExpr* rewrittenRangePredItemExpr;
  QRElementPtr rangeItem;
  QRRangePredListPtr rangePreds = candidate->getRangePredList();
  if (!rangePreds)
    return;

  for (CollIndex rpInx=0; rpInx<rangePreds->entries(); rpInx++)
    {
      QRRangePredPtr rangePred = (*rangePreds)[rpInx];
      switch (rangePred->getResult())
        {
          case QRElement::Outside:
          case QRElement::Provided:
            // nothing needs to be done
            break;

          case QRElement::NotProvided:
            // Need to use the ItemExpression for the range predicate from the
            // query, and substitute the value id of the MV column in place of
            // the value id of the column the range is on in the query.
            rangePredValueId = rangePred->getRefNum();
            rewrittenRangePredItemExpr = NULL;
            rangeItem = rangePred->getRangeItem();
            if (rangeItem->getElementType() == ET_MVColumn)
              {
		QRMVColumnPtr mvCol = rangeItem->downCastToQRMVColumn();
                if (candidate->isIndirectGroupBy())
                  determineAggForRollup(mvCol);
                if (rangePred->getRefNum() == 0) // If no ref attribute.
                {
                  // This is not a reference to a query range predicate, but
                  // rather one we need to construct here.
                  // We only do IS NOT NULL range preds for LOJ support.
                  NABoolean isNotNull = (rangePred->getOperatorList().entries() == 0);
                  assertLogAndThrow(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                                    isNotNull, MVCandidateException,
                                    "Expecting only IS NOT NULL range predicates to have no ref attributes.");
                  ItemExpr* mvCol = 
                    new(heap) ColReference(getColRefName(candidate, rangeItem, heap));
                  rewrittenRangePredItemExpr = 
                    new(heap) UnLogic(ITM_IS_NOT_NULL, mvCol);                  
                }
                else
                {
                  rewrittenRangePredItemExpr = 
                    rewriteItemExpr(rangeItem,
                                    rangePredValueId.getItemExpr(),
                                    candidate,
                                    heap);
                }
              }
            else if (rangeItem->getElementType() == ET_Column)
              {
                // Range pred for a dimension table of an IGB query is handled
                // in addPredsFromList(). For non-IGB query, a column here is
                // for a NotProvided range predicate on a column of a backjoin
                // table. Use the hash table to see which scan node to add the
                // rewritten predicate to.
                if (!candidate->isIndirectGroupBy())
                  {
                    QRColumnPtr col = rangeItem->getReferencedElement()
                                                ->downCastToQRColumn();
                    Scan* bjScan = bjScanHash_.getFirstValue(&col->getTableID());
                    assertLogAndThrow(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                                      bjScan, MVCandidateException,
                                      "Could not find scan node for back join "
                                      "table.");
                    // Don't assign the rewritten pred to rewrittenRangePredItemExpr,
                    // or it will be added to the MV node below.
                    bjScan->addSelPredTree(
                                    rewriteItemExpr(rangeItem,
                                                    rangePredValueId.getItemExpr(),
                                                    candidate,
                                                    heap));
                  }
              }
            else if (rangeItem->getElementType() == ET_Expr)
              {
                rewrittenRangePredItemExpr = 
                               rewriteItemExpr(rangeItem,
                                               rangePredValueId.getItemExpr(),
                                               candidate,
                                               heap);
              }
            else
              assertLogAndThrow1(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                                 FALSE, MVCandidateException,
                                 "Not ready for range item with element type %d",
                                 rangeItem->getElementType())

            if (rewrittenRangePredItemExpr)
              {
                if (!rewrittenRangePredItemExpr->containsAnAggregate())
              scan->addSelPredTree(rewrittenRangePredItemExpr);
                else if (groupBy)
                  groupBy->addSelPredTree(rewrittenRangePredItemExpr);
                else
                  QRLogger::log(CAT_SQL_COMP_MVCAND, LL_DEBUG,
                    "Rewritten range pred contains an aggregate, but "
                               "there is no Group By node -- pred not added.");
              }
            break;

          default:
            assertLogAndThrow1(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL, 
                               FALSE, MVCandidateException,
                               "Unknown value for result attribute -- %d",
                               rangePred->getResult());
            break;
        }
    }
}

// If back joins were added to get columns used in predicates, this function
// will determine the owning node for a predicate based on its inputs. If a
// single back join scan node is used, it is returned. If >1 back join node is
// used, or the MV node and 1 or more back join node, the root of the join
// tree that connects the MV and all the back join nodes is returned. If only
// the MV is involved, NULL is returned, and the predicate will be attached to
// the MV scan or Group By node as appropriate.
static RelExpr* getBackJoinNode(const ElementPtrList& inputList,
                                RelExpr* bjRoot,
                                NAHashDictionary<const NAString, Scan>& bjScanHash)
{
  QRElementPtr elem;
  RelExpr* bjOwningNode = NULL;
  NABoolean mvUsed = FALSE;
  Scan* bjScan;
  QRColumnPtr col;

  for (CollIndex i=0; i<inputList.entries(); i++)
    {
      elem = inputList[i];
      if (elem->getElementType() == ET_MVColumn)
        {
          if (bjOwningNode)
            return bjRoot;  // multinode pred, put in highest join node
          else
            mvUsed = TRUE;
        }
      else
        {
          assertLogAndThrow1(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                             elem->getElementType() == ET_Column,
                             MVCandidateException,
                             "getBackJoinNode() -- can't handle element of type %d",
                             elem->getElementType());
          col = elem->getReferencedElement()->downCastToQRColumn();
          bjScan = bjScanHash.getFirstValue(&col->getTableID());
          assertLogAndThrow(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                            bjScan, MVCandidateException,
                            "Could not find scan node for back join table.");
          if (mvUsed)
            return bjRoot;      // multinode pred, put in highest join node
          else if (bjOwningNode)
            {
              if (bjScan != bjOwningNode)
                return bjRoot;  // multinode pred, put in highest join node
            }
          else
            bjOwningNode = bjScan;
        }
   }

  return bjOwningNode;
}

void MVCandidates::rewriteResidPreds(QRCandidatePtr candidate,
                                     RelExpr* scan,
                                     RelExpr* groupBy,
                                     RelExpr* bjRoot,
                                     CollHeap* heap)
{
  QRExprPtr residPred;
  QRResidualPredListPtr residPreds = candidate->getResidualPredList();
  if (!residPreds)
    return;

  // Set up the list of element types we want to look for when searching the
  // tree for inputs.
  NAList<ElementType> inputTypes(heap);
  inputTypes.insert(ET_MVColumn);
  inputTypes.insert(ET_Column);   // obtained by back join

  for (CollIndex rpInx=0; rpInx<residPreds->entries(); rpInx++)
    {
      residPred = (*residPreds)[rpInx];
      switch (residPred->getResult())
        {
          case QRElement::Outside:
          case QRElement::Provided:
            // nothing needs to be done
            break;

          case QRElement::NotProvided:
            {
              ValueId residPredValueId = residPred->getRefNum();
              ElementFinderVisitorPtr visitor = 
                    new(heap) ElementFinderVisitor(inputTypes, FALSE,
                                                   ADD_MEMCHECK_ARGS(heap));
              residPred->getExprRoot()->treeWalk(visitor);
              const ElementPtrList& inputList = visitor->getElementsFound();
              RelExpr* bjOwningNode = getBackJoinNode(inputList, bjRoot, bjScanHash_);
              ItemExpr* rewrittenResidPredItemExpr = 
                              rewriteItemExpr(inputList,
                                              residPredValueId.getItemExpr(),
                                              candidate, heap);
              if (bjOwningNode)
                bjOwningNode->addSelPredTree(rewrittenResidPredItemExpr);
              else if (!rewrittenResidPredItemExpr->containsAnAggregate())
                scan->addSelPredTree(rewrittenResidPredItemExpr);
              else if (groupBy)
                groupBy->addSelPredTree(rewrittenResidPredItemExpr);
              else
                QRLogger::log(CAT_SQL_COMP_MVCAND, LL_DEBUG,
                  "Rewritten residual pred contains an aggregate, "
                             "but there is no Group By node -- pred not added.");
              deletePtr(visitor);
            }
            break;

          default:
            assertLogAndThrow1(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL, 
                               FALSE, MVCandidateException,
                               "Unknown value for result attribute -- %d",
                               residPred->getResult());
            break;
        }
    }
}

ColRefName* MVCandidates::getColRefName(QRCandidatePtr candidate,
                                        QRElementPtr elem,
                                        CollHeap* heap)
{
  ColRefName* returnVal;
  ElementType elemType = elem->getElementType();
  if (elemType == ET_Output)
    {
      elem = static_cast<QROutputPtr>(elem)->getOutputItem();
      elemType = elem->getElementType();
    }
  if (elemType == ET_Column)
    {
      QRColumnPtr colElem = elem->downCastToQRColumn();
      const NAString& colName = colElem->getColumnName();
      NAString fqTblName(colElem->getFullyQualifiedColumnName(), heap);
      // Truncate last dot and col name to leave fully qualified table name.
      fqTblName.resize(fqTblName.length() - colName.length() - 1);
      QualifiedName tblQualName(fqTblName, 3, heap, bindWA_);
      NAString corrNameStr;
      // Add special correlation name if the column is from a backjoin table.
      const NAString& tblID = colElem->getTableID();
      if (bjScanHash_.getFirstValue(&tblID))
        (corrNameStr = BACKJOIN_CORRNAME_PREFIX).append(tblID);
      CorrName tblCorrName(tblQualName, heap, corrNameStr);
      returnVal = new(heap) ColRefName(colName, tblCorrName, heap);
    }
  else if (elemType == ET_MVColumn)
    {
      QualifiedName mvQualName(candidate->getMVName()->getMVName(),
                               3, heap, bindWA_);
      CorrName mvCorrName(mvQualName, heap);
      returnVal = new(heap) ColRefName(elem->downCastToQRMVColumn()
                                           ->getMVColName(),
                                       mvCorrName,
                                       heap);
    }
  else
    throw QRDescriptorException("getColRefName() cannot be called for element %s",
                                elem->getElementName());

  return returnVal;
} // getColRefName()

static ItemExpr* 
addToList(ItemExpr *list, ItemExpr *item, CollHeap* heap)
{
  if (list == NULL)
    list = item;
  else
    list = new(heap) ItemList(list, item);
  return list;
}

void MVCandidates::buildOutputExprs(QRCandidatePtr candidate,
                                    RelRoot* root,
                                    ElementPtrList& topValElemList,
                                    CollHeap* heap)
{
  ItemExpr* compExpr;
  ItemExpr* selectList = NULL;
  QRElementPtr outputItem;
  QRMVColumnPtr mvCol;
  QRColumnPtr col;
  QRExprPtr expr;
  QROutputListPtr outputList = candidate->getOutputList();

  // Output list could be null if all items in select list are part of a veg
  // that includes a constant.
  if (!outputList)
    return;

  for (CollIndex outInx=0; outInx<outputList->entries(); outInx++)
    {
      QROutputPtr output = (*outputList)[outInx];
      switch (output->getResult())
        {
          case QRElement::Provided:
            {
              outputItem = output->getOutputItem();
              assertLogAndThrow1(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                                 outputItem->getElementType() == ET_MVColumn,
                                 MVCandidateException,
                                 "Unexpected element type for output item with "
                                     "result='Provided' -- %d",
                                 outputItem->getElementType());
              topValElemList.insert(output);
              if (!candidate->isIndirectGroupBy())
                {
                  mvCol = outputItem->downCastToQRMVColumn();
                  compExpr = new(heap) ColReference
                                        (getColRefName(candidate, mvCol, heap));
                  selectList = addToList(selectList, compExpr, heap);
                }
            }
            break;

          case QRElement::NotProvided:
            {
              const ElementPtrList& outputItemList = output->getOutputItems();
              if (outputItemList.entries() == 0)
                {
                  // @ZX -- Output expression involving only constants. Maybe we
                  //        should assert false here; I don't think such a thing
                  //        will be included in the result descriptor.
                }
              else if (outputItemList.entries() > 1)
                {
                  assertLogAndThrow(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                                      FALSE, MVCandidateException,
                                        "Output item list with multiple entries "
                                            "(partial matching) not handled yet")
                }
              else if (outputItemList[0]->getElementType() == ET_Column)
                {
                  // Output expression is a column requiring a back-join.
                  // There must be a single output item, which is the column.
                  assertLogAndThrow1(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                                     outputItemList.entries() == 1,
                                     MVCandidateException,
                                     "Output item list for col ref should have "
                                         "a single item, instead of %d",
                                     outputItemList.entries());
                  topValElemList.insert(output);
                  if (!candidate->isIndirectGroupBy())
                    {
                      col = outputItemList[0]->downCastToQRColumn();
                      compExpr = new(heap) ColReference
                                            (getColRefName(candidate, col, heap));
                      selectList = addToList(selectList, compExpr, heap);
                    }
                }
              else  // The output item must be an <Expr>.
                {
                  assertLogAndThrow1(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                                     outputItemList.entries() == 1 &&
                                     outputItemList[0]->getElementType() == ET_Expr,
                                     MVCandidateException,
                                     "Element of type <Expr> expected in "
                                         "this <Output> element instead of %d",
                                     outputItemList[0]->getElementType());

                  // Don't include aggregates in the top value list for the
                  // value id map, they are added by a different method.
                  // Aggs are the only items added to the select list of
                  // the root for an IQB query.
                  expr = outputItemList[0]->downCastToQRExpr();
                  NABoolean isAggregate = expr->getExprRoot()->isAnAggregate();
                  NABoolean hasAggregate = expr->getExprRoot()->containsAnAggregate(heap);
                  if (!isAggregate)
                    topValElemList.insert(output);
                  if (!candidate->isIndirectGroupBy() || hasAggregate)
                    {
                      compExpr = parseExpression(expr, candidate->getMVName()->getMVName(), heap);
                      
                      if (hasAggregate)
                        {
                          // Create a fake name for the agg that embeds the
                          // value id it maps to, so we can create the map
                          // vid entry for it when it has its own value id.
                          char colName[20];
                          sprintf(colName, "%.10s%d",
                                  AGG_NAME_PREFIX, output->getRefNum());
                          compExpr = new(heap) 
                            RenameCol(compExpr, new(heap) ColRefName(colName, heap));
                        }
                      selectList = addToList(selectList, compExpr, heap);
                    }
                }
            }
            break;

          case QRElement::Outside:
            // No action; output item is outside this jbbsubset.
            break;

          default:
            assertLogAndThrow1(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                               FALSE, MVCandidateException,
                               "Unknown value for result attribute -- %d",
                               output->getResult());
            break;
        }
    }

  // Add the select list to the node. For an IGB without aggs (unlikely), it
  // will be null.
  if (selectList)
    {
      root->addCompExprTree(selectList);
    }
}  // buildOutputExprs()

void MVCandidates::analyzeCandidateMVs(QRJbbSubsetPtr qrJbbSubset)
{
  // Get the actual (Analyzer) jbb subset corresponding to the tables in the
  // QRJbbSubset, as well as the GroupBy node if there is one.
  QRTableListPtr tables = qrJbbSubset->getTableList();
  CANodeIdSet jbbNodeSet;
  for (CollIndex i=0; i<tables->entries(); i++)
    {
      jbbNodeSet.addElement((*tables)[i]->getRefNum());
    }

  if (qrJbbSubset->hasGroupBy())
    jbbNodeSet.addElement(qrJbbSubset->getRefNum());

  JBBSubset* actualJbbSubset = jbbNodeSet.computeJBBSubset();

  // No jbb subset for single-table queries 
  NABoolean handleRewriteOnSingleTable = 
               (CmpCommon::getDefault(MVQR_REWRITE_SINGLE_TABLE_QUERIES) == DF_ON);

  if (actualJbbSubset->getJBBCs().entries() == 0 && !handleRewriteOnSingleTable)
    {
      QRLogger::log(CAT_SQL_COMP_MVCAND, LL_INFO,
        "JBBSubset has no JBBCs; skipping candidate analysis");
      delete actualJbbSubset;
      return;
    }

  QRCandidateListPtr candidates = qrJbbSubset->getCandidateList();
  QRCandidatePtr candidate;
  QueryAnalysis* qa = QueryAnalysis::Instance();
  CollHeap* heap = qa->outHeap();

  // if we have candidates go ahead and build the favorites list
  if ((mvqrFavoriteCandidates_ == NULL) &&  (candidates->entries()))
    buildFavoritesList (heap);

  NABoolean rewriteSingleNode;
  for (CollIndex i=0; i<candidates->entries(); i++)
    {
      candidate = (*candidates)[i];
      if (actualJbbSubset->getJBBCs().entries() == 0)
        {
          assertLogAndThrow(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                            (handleRewriteOnSingleTable && (queryRoot_->child(0)->getOperator().match(REL_SCAN))), 
                            MVCandidateException,
                            "No JBBCs in jbbsubset analysis");
          rewriteSingleNode = TRUE;
        }
      else
        rewriteSingleNode = FALSE;
      try
        {
          analyzeCandidate(candidate,
                           (candidate->isIndirectGroupBy() 
                               ? const_cast<JBBSubset&>(actualJbbSubset->getJBB()->getMainJBBSubset())
                               : *actualJbbSubset),
                           rewriteSingleNode,
                           jbbNodeSet, heap);
        }
      catch (MVCandidateException&)
        {
          // Throwers of this exception should use one of the macros that logs the
          // exception message with file and line info (e.g., assertLogAndThrow).
          // Here, we just log the fact that the candidate was ignored.
          QRLogger::log(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
            "Skipping candidate MV %s due to exception",
                      candidate->getMVName()->getMVName().data());
        }
    }

  delete actualJbbSubset;
}

ValueId MVCandidates::getRewriteVid(RETDesc* retDesc,
                                    QRElementPtr elem,
                                    QRCandidatePtr candidate,
                                    CollHeap* heap)
{
  const ColumnDescList* colDescList = retDesc->getColumnList();
  CollIndex numCols = colDescList->entries();
  ColRefName* colRefName = getColRefName(candidate, elem, heap);
  for (CollIndex i=0; i<numCols; i++)
    {
      if (*colRefName == (*colDescList)[i]->getColRefNameObj())
        return retDesc->getValueId(i);
    }
    
  assertLogAndThrow(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                    FALSE, MVCandidateException,
                    "Column in <Output> element of result descriptor not found "
                    "in column descriptor list");
}

ValueId MVCandidates::getBaseColValueId(QRElementPtr referencingElem)
{
  if (*referencingElem->getRef() == 'O')
    referencingElem = static_cast<QROutputPtr>(referencingElem)->getOutputItem();

  const NAString& ref = referencingElem->getRef();
  char elemIdChar = *ref.data();

  // If an expression, return null and no mapvid entry will be added.
  if (elemIdChar == 'X' || elemIdChar == 'S' || elemIdChar == 0)
    return NULL_VALUE_ID;

  if (elemIdChar != 'J')
    {
      assertLogAndThrow1(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                         elemIdChar == 'C',
                        MVCandidateException,
                        "Reference %s does not start with 'J' or 'C'",
                        ref.data());
      return referencingElem->getRefNum();
    }

  ValueId vegrefVid = referencingElem->getRefNum();
  ItemExpr* ie = vegrefVid.getItemExpr();
  assertLogAndThrow1(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                     ie->getOperatorType() == ITM_VEG_REFERENCE,
                     MVCandidateException,
                     "Reference %s is not to a vegref", ref.data());

  const ValueIdSet &vegMembers =
            (static_cast<VEGReference*>(ie))->getVEG()->getAllValues();

  ValueId someMemberId = vegMembers.init();
  if (vegMembers.next(someMemberId) && 
      someMemberId.getItemExpr()->getOperatorType() == ITM_BASECOLUMN) 
    return someMemberId;

  assertLogAndThrow1(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                     FALSE, MVCandidateException,
                     "No base column found in vegref referenced by %s",
                     ref.data());

  //else if (pExpr->getOperatorType() == ITM_INDEXCOLUMN)
  //  {
  //    cvid = static_cast<IndexColumn*>(pExpr)->getDefinition(); 
  //    pBC = static_cast<BaseColumn*>(cvid.getItemExpr());
  //  }
}

ValueId MVCandidates::getVegrefValueId(QRElementPtr referencingElem)
{
  // If the reference is not to a column, it won't have a vegref in its
  // query descriptor element. In these cases, the ref number should itself
  // be that of the vegref.
  const NAString& ref = referencingElem->getRef();
  if (*ref.data() != 'C')
    return referencingElem->getRefNum();

  const QRElementHash& hash = qdescGenerator_.getColTblIdHash();
  QRElementPtr referencedElem = hash.getFirstValue(&ref);
  if (referencedElem && referencedElem->getElementType() == ET_Column)
    {
      ValueId vrid = static_cast<QRColumnPtr>(referencedElem)->getVegrefId();
      return (vrid != NULL_VALUE_ID
                    ? vrid
                    : (ValueId)referencingElem->getRefNum());
    }
  else
    {
      QRLogger::log(CAT_SQL_COMP_MVCAND, LL_DEBUG,
        "No vegref found for element refed by id %s, using ref id", ref.toCharStar());
      return referencingElem->getRefNum();
    }
}



// For the method mapVidNode->pushdownCoveredExpr to work correctly, the lower values
// of the MVID have to be expressed as vegrefs hence the reason for this method.
void MVCandidates::populateOneVidMap(RelExpr* root,
                                     ValueIdSet& vegRewriteVids,
                                     QRElementPtr elem,
                                     QRCandidatePtr candidate,
                                     ValueIdMap& vidMap,
                                     CollHeap* heap)
{
  BaseColumn* baseCol;
  Int32 colIndex;
  TableDesc* tableDesc;
  ItemExpr* ie;
  ItemExpr* rewriteIE;
  ValueId rewriteVid;
  ValueId baseColVid;

  rewriteVid = getRewriteVid(root->getRETDesc(),
                             elem,
                             candidate, heap);

  rewriteIE = rewriteVid.getItemExpr();
  if (rewriteIE->getOperatorType() == ITM_BASECOLUMN)
  {
      baseCol = static_cast<BaseColumn*>(rewriteIE);
      colIndex = baseCol->getColNumber();
      tableDesc = baseCol->getTableDesc();
      if (tableDesc)
      {
          ValueIdList baseCols;
          ValueIdList vegCols;
          ValueId vegrefVid;
          baseCols.insert(baseCol->getValueId());
          tableDesc->getEquivVEGCols(baseCols, vegCols);
          ie = vegCols[0].getItemExpr();
          if (ie->getOperatorType() == ITM_VEG_REFERENCE)
              {
              // In a rollup case, sometimes QRMVColumn elements have no direct
              // corresponding element in the query descriptor and therefore
              // have no ref attribute. In these cases, we need to avoid adding
              // a zero value id to the mapvid node.
              vegrefVid = getVegrefValueId(elem);
              if (vegrefVid == NULL_VALUE_ID)
                return;
              if (vidMap.getTopValues().contains(vegrefVid))
                return;
              vidMap.addMapEntry(vegrefVid, ie->getValueId());
            }
          else
            assertLogAndThrow(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                              FALSE, MVCandidateException,
                              "Unable to get vegref for map value id lower value")
          baseColVid = getBaseColValueId(elem);
          if (baseColVid != NULL_VALUE_ID)
            vegRewriteVids.addElement(baseColVid);
      }
  }
  else
  {
     assertLogAndThrow(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                       FALSE, MVCandidateException,
                       "Expecting a base column while constructing map value id lower value")
  }
}

void MVCandidates::populateVidMap(RelExpr* root,
                                  ElementPtrList& vidMapTopElements,
                                  ValueIdSet& vegRewriteVids,
                                  QRCandidatePtr candidate,
                                  ValueIdMap& vidMap,
                                  CollHeap* heap)
{
  QROutputPtr output;
  QRElementPtr elem;
  ElementType elemType;
  QRExprPtr expr;

  for (CollIndex i=0; i<vidMapTopElements.entries(); i++)
    {
      output = vidMapTopElements[i]->downCastToQROutput();
      elem = output->getOutputItem();
      elemType = elem->getElementType();
      // ET_Columns will be present if there are back joins.
      if (elemType == ET_MVColumn || elemType == ET_Column)
        {
          // Use id of Output elem, so it will work for LOJ as well
          populateOneVidMap(root, vegRewriteVids, output, candidate, vidMap, heap);
        }
      else if (elemType == ET_Expr)
        {
          expr = elem->downCastToQRExpr();
          if (expr->getExprRoot()->isAnAggregate())
            {
              // These have already been taken care of and shouldn't be in the
              // top value list.
              assertLogAndThrow(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                                FALSE, MVCandidateException,
                                "Aggregate function found in top value list");
            }
          else
            {
              const ElementPtrList& colsUsed = expr->getInputColumns(heap, FALSE);
              for (CollIndex colInx=0; colInx<colsUsed.entries(); colInx++)
                {
                  elem = colsUsed[colInx];
                  populateOneVidMap(root, vegRewriteVids, elem, candidate, vidMap, heap);
                }
            }
        }
      else
        assertLogAndThrow(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                          FALSE, MVCandidateException,
                          "Output item is neither Column, MVColumn nor expression")
    }
}

void MVCandidates::addBackJoins(QRCandidatePtr candidate,
                                QRTableListPtr bjTables,
                                RelExpr*& node,
                                CollHeap* heap)
{
  QRTablePtr table;
  CollIndex numJoinItems;
  ItemExpr* backJoinPred = NULL;
  ItemExpr* eq;
  Scan* bjScan;
  QRJoinPredListPtr joinPredList = candidate->getJoinPredList();

  // Build the join tree. Use a correlation name for backjoin tables in case of
  // self-join.
  for (CollIndex bjInx=0; bjInx<bjTables->entries(); bjInx++)
    {
      table = (*bjTables)[bjInx];
      QualifiedName tableQualName(table->getTableName(), 3, heap, bindWA_);
      NAString corrNameStr(BACKJOIN_CORRNAME_PREFIX);
      corrNameStr.append(table->getRef());
      CorrName tableCorrName(tableQualName, heap, corrNameStr);
      bjScan = new(heap) Scan(tableCorrName, NULL, REL_SCAN, heap);
      node = new(heap) Join(node, bjScan, REL_JOIN);
      bjScanHash_.insert(&table->getReferencedElement()->getID(), bjScan);
    }

  // Build the join predicate.
  for (CollIndex jpInx=0; jpInx<joinPredList->entries(); jpInx++)
    {
      const ElementPtrList& joinItems =
                (*joinPredList)[jpInx]->getEqualityList();
      numJoinItems = joinItems.entries();
      assertLogAndThrow(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                        numJoinItems > 1, MVCandidateException,
                        "Must have at least 2 items in a JoinPred");
      for (CollIndex jiInx=1; jiInx<numJoinItems; jiInx++)
        {
          eq = new(heap) BiRelat(
                    ITM_EQUAL,
                    new(heap) ColReference
                                (getColRefName(candidate, joinItems[jiInx-1],
                                              heap)),
                    new(heap) ColReference
                                (getColRefName(candidate, joinItems[jiInx],
                                              heap)));
          if (!backJoinPred)
            backJoinPred = eq;
          else
            backJoinPred = new(heap) BiLogic(ITM_AND, backJoinPred, eq);
        }
    }

  // Attach the predicate to the highest join node.
  static_cast<Join*>(node)->setJoinPredTree(backJoinPred);
}  // addBackJoins()

// The result of this function indicates whether or not the passed element
// is owned by tblNode. We are ultimately looking for a column, but the
// passed element could reference a JoinPred the equality list of which we
// must extract the needed column.
static NABoolean igbColMatchesTableNode(QRElementPtr elem, CANodeId tblNode)
{
  QRElementPtr refedElem = elem->getReferencedElement();
  if (refedElem->getElementType() == ET_Column)
    return (static_cast<QRColumnPtr>(refedElem))->getTableIDNum() == tblNode;

  assertLogAndThrow1(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                     refedElem->getElementType() == ET_JoinPred,
                     MVCandidateException,
                     "Column refs element of type %d",
                     refedElem->getElementType());

  NABoolean found = FALSE;
  const ElementPtrList& eqList =
          (static_cast<QRJoinPredPtr>(refedElem))->getEqualityList();
  for (CollIndex i=0; !found && i<eqList.entries(); i++)
    {
     if (eqList[i]->getElementType() == ET_Column)
       {
         refedElem = static_cast<QRColumnPtr>(eqList[i])->getReferencedElement();
         // Not sure if it's possible for an item contained in a JoinPred to
         // reference another JoinPred, but let's check for it since it isn't
         // handled.
         assertLogAndThrow(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                           refedElem->getElementType() == ET_Column,
                           MVCandidateException,
                           "Column element of JoinPred refs a non-column");
         found = static_cast<QRColumnPtr>(refedElem)->getTableIDNum() == tblNode;
       }
    }

  return found;
}

// This function is used to assign range and residual predicates to the proper
// scan nodes in the rewrite of an IGB query.
template <class T>
void MVCandidates::addPredsFromList(QRCandidatePtr candidate,
                                    Scan* scan,
                                    CANodeId tableNodeId,
                                    const NAPtrList<T>& list,
                                    CollHeap* heap)
{
  ItemExpr* itemExpr;

  // For each range/residual (based on type argument T) pred from the query
  // descriptor, see if the table id referenced is tableNodeId. If so, call
  // rewriteItemExpr passing the list of columns occurring in the pred, and
  // add the result to the sel pred list for node.
  for (CollIndex i=0; i<list.entries(); i++)
    {
      QRElementPtr elem = list[i];
      ElementFinderVisitorPtr visitor = 
            new(heap) ElementFinderVisitor(ET_Column, TRUE,
                                           ADD_MEMCHECK_ARGS(heap));
      elem->treeWalk(visitor);
      const ElementPtrList& colList = visitor->getElementsFound();
      if (igbColMatchesTableNode(colList[0], tableNodeId))
        {
          itemExpr = rewriteItemExpr(colList,
                                     ((ValueId)elem->getIDNum()).getItemExpr(),
                                     candidate, heap, tableNodeId);
          scan->addSelPredTree(itemExpr);
        }
    }
}

void MVCandidates::addIGBPreds(QRCandidatePtr candidate, Scan* scan, 
                               CANodeId tableNodeId, QRJBBPtr jbb,
                               CollHeap* heap)
{
  // Get first the JBB's range preds, then the residual preds, passing each to
  // the function that will add to the passed scan node the ones that belong to it.
  const NAPtrList<QRRangePredPtr>& rangePreds = jbb->getHub()->getRangePredList()->getList();
  addPredsFromList(candidate, scan, tableNodeId, rangePreds, heap);
  const NAPtrList<QRExprPtr>& residPreds = jbb->getHub()->getResidualPredList()->getList();
  addPredsFromList(candidate, scan, tableNodeId, residPreds, heap);
}

// elem is a join predicate operand in an IGB query, for which we need to
// create an ItemExpr. This is done differently depending on whether the owning
// table is a fact table or dimension table. The latter do not underlie the
// MV candidate.
ItemExpr* MVCandidates::getIGBJoinCondOp(QRElementPtr elem,
                                         QRCandidatePtr candidate,
                                         CANodeIdSet& jbbSubsetNodes,
                                         NABoolean& fromFactTable,
                                         CollHeap* heap)
{
  if (elem->getElementType() != ET_Column)
    assertLogAndThrow1(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                       FALSE, MVCandidateException,
                       "Can only handle column join condition operands, not %d",
                       elem->getElementType());

  QRElementPtr outputItem;
  QRColumnPtr col = elem->getReferencedElement()->downCastToQRColumn();
  NumericID tableId = col->getTableIDNum();
  CANodeId tableNodeId(tableId);
  fromFactTable = jbbSubsetNodes.containsThisId(tableNodeId);
  if (fromFactTable)
    {
      // Search output list from candidate in result descriptor for an MVColumn
      // with a ref to the column's id#.
      QRMVColumnPtr mvCol;
      QROutputListPtr outputs = candidate->getOutputList();
      if (outputs)
      for (CollIndex i=0; i<outputs->entries(); i++)
        {
          outputItem = (*outputs)[i]->getOutputItem();
          if (outputItem->getElementType() == ET_MVColumn)
            {
              mvCol = outputItem->downCastToQRMVColumn();
              if (mvCol->getRefNum() == col->getIDNum())
                return new(heap) ColReference(getColRefName(candidate,
                                                            mvCol, heap));
            }
        }
      assertLogAndThrow(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                        FALSE, MVCandidateException,
                        "Could not find join operand from fact table for "
                        "indirect Group By query");
    }
  else
    // If the column is from a dimension table, just create a ColReference for it.
    return new(heap) ColReference(getColRefName(candidate, col, heap));
}

void MVCandidates::addIGBDimJoins(QRCandidatePtr candidate,
                                  CANodeIdSet& jbbSubsetNodes,
                                  QRJBBPtr jbbElem,
                                  RelExpr*& node,
                                  NodeIdScanHash& dimScanHash,
                                  CollHeap* heap)
{
  QRElementPtr jbbcElem;
  QRTablePtr table;
  CollIndex numJoinItems;
  ItemExpr* joinPredItem = NULL;
  ItemExpr* eqItem;
  NABoolean op1FromFactTable, op2FromFactTable;
  ItemExpr *leftChild, *rightChild;
  Scan* dimScanNode;
  CollIndex* nodeIdHashKey;

  // Loop through all the dimension tables in the JBB and join them. The fact
  // tables are already in the tree that we start with. They can be distinguished
  // from dimension tables as we look through the jbbc list by checking whether
  // the CANodeId is in jbbSubsetNodes -- these are the ones that comprise the
  // candidate MV, and are always the fact tables in the case of an IGB candidate.
  QRJBBCListPtr jbbcElemList = jbbElem->getHub()->getJbbcList();
  CANodeId tableNodeId;
  for (CollIndex jbbcInx=0; jbbcInx<jbbcElemList->entries(); jbbcInx++)
    {
      jbbcElem = jbbcElemList->getElement(jbbcInx);
      if (jbbcElem->getElementType() == ET_Table)
        {
          table = jbbcElem->downCastToQRTable();
          tableNodeId = table->getIDNum();
          if (!jbbSubsetNodes.containsThisId(tableNodeId))
            {
              QualifiedName tableName(table->getTableName(), 3, heap, bindWA_);
              dimScanNode = new(heap) Scan(tableName, NULL, REL_SCAN, heap);
              nodeIdHashKey = new(heap) CollIndex(table->getIDNum());
              dimScanHash.insert(nodeIdHashKey, dimScanNode);
              addIGBPreds(candidate, dimScanNode, table->getIDNum(), jbbElem, heap);
              node = new(heap) Join(node, dimScanNode, REL_JOIN);
            }
        }
    }

  // Build the join predicates.
  QRJoinPredListPtr joinPredList = jbbElem->getHub()->getJoinPredList();
  for (CollIndex jpInx=0; jpInx<joinPredList->entries(); jpInx++)
    {
      const ElementPtrList& joinItems =
                (*joinPredList)[jpInx]->getEqualityList();
      numJoinItems = joinItems.entries();
      assertLogAndThrow(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                        numJoinItems > 1, MVCandidateException,
                        "Must have at least 2 items in a JoinPred");
      for (CollIndex jiInx=1; jiInx<numJoinItems; jiInx++)
        {
          leftChild = getIGBJoinCondOp(joinItems[jiInx-1]->getReferencedElement(),
                                        candidate, jbbSubsetNodes,
                                        op1FromFactTable, heap);
          rightChild = getIGBJoinCondOp(joinItems[jiInx]->getReferencedElement(),
                                        candidate, jbbSubsetNodes,
                                        op2FromFactTable, heap);
          if (op1FromFactTable && op2FromFactTable)
            continue;  // this join already represented in MV scan node

          eqItem = new(heap) BiRelat(ITM_EQUAL, leftChild, rightChild);
          if (!joinPredItem)
            joinPredItem = eqItem;
          else
            joinPredItem = new(heap) BiLogic(ITM_AND, joinPredItem, eqItem);
        }
    }

  // Attach the join predicate to the top join node.
  if (joinPredItem)
    static_cast<Join*>(node)->setJoinPredTree(joinPredItem);
}

void MVCandidates::mapDimOutputs(QRJBBPtr jbbElem,
                                 CANodeIdSet& jbbSubsetNodes,
                                 NodeIdScanHash& dimScanHash,
                                 ValueIdMap& vidMap,
                                 CollHeap* heap)
{
  QROutputListPtr outputList = jbbElem->getOutputList();
  QROutputPtr output;
  ElementPtrList elems(heap);

  // Get the initial list of cols/exprs that are directly contained in the output
  // elements. When this list is traversed, if an expression is encountered
  // (typically an aggregate function), the columns referenced in the expression
  // will be added to the end of the list, so that all columns used in the query's
  // select list will be part of the map.
  //
  // Each output will have exactly one col/expr, except in the case of NotProvided
  // output expression with partial matching (not yet supported).
  for (CollIndex outInx=0; outInx<outputList->entries(); outInx++)
    {
      output = (*outputList)[outInx];
      const ElementPtrList& outputItems = output->getOutputItems();
      for (CollIndex outItemInx=0; outItemInx<outputItems.entries(); outItemInx++)
        elems.insert(outputItems[outItemInx]);
    }

  QRColumnPtr col;
  CANodeId tblNodeId;
  TableDesc* tdesc;
  ValueId colId;
  CollIndex tblNodeIdNum;
  Scan* dimScan;
  NABoolean found;
  QRElementPtr elem;

  for (CollIndex elemInx=0; elemInx<elems.entries(); elemInx++)
    {
      elem = elems[elemInx]->getReferencedElement();
      switch (elem->getElementType())
        {
          case ET_Column:
            col = elem->downCastToQRColumn();
            tblNodeId = tblNodeIdNum = col->getTableIDNum();
            if (!jbbSubsetNodes.containsThisId(tblNodeId))
              {
                // A dimension table in the IGB query.
                tdesc = tblNodeId.getNodeAnalysis()->getTableAnalysis()->getTableDesc();
                const ValueIdList& qColList = tdesc->getColumnList();
                colId = col->getIDNum();
                found = FALSE;
                for (CollIndex qColInx=0; qColInx<qColList.entries() && !found; qColInx++)
                  {
                    if (qColList[qColInx] == colId)
                      {
                        dimScan = dimScanHash.getFirstValue(&tblNodeIdNum);
                        assertLogAndThrow(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                                          dimScan, MVCandidateException,
                                          "Dimension scan not found in hash table");
                        const ValueIdList& rColList = dimScan->getTableDesc()
                                                             ->getColumnList();
                        vidMap.addMapEntry(colId, rColList[qColInx]);
                        found = TRUE;
                      }
                  }
                assertLogAndThrow1(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                                   found, MVCandidateException,
                                   "Column value id not found in col desc list: %ud",
                                   colId.toUInt32());
              }
            break;

          case ET_Expr:
            {
              // Find the columns in the expression and add them to the end of
              // the list, so they will be processed on subsequent iterations
              // of the loop.
              QRExprPtr expr = elem->downCastToQRExpr();
              ElementFinderVisitorPtr visitor = 
                    new(heap) ElementFinderVisitor(ET_Column, TRUE,
                                                   ADD_MEMCHECK_ARGS(heap));
              expr->treeWalk(visitor);
              elems.insert(visitor->getElementsFound());
            }
            break;

          case ET_JoinPred:
            {
              // Column in output list referenced a joinpred. Put the joinpred's
              // equality list items in the list for processing on subsequent
              // iterations of the loop.
              QRElementPtr eqListElem;
              ElementType  eqListElemType;
              QRJoinPredPtr jp = elem->downCastToQRJoinPred();
              const ElementPtrList& eqList = jp->getEqualityList();
              for (CollIndex i=0; i<eqList.entries(); i++)
                {
                  eqListElem = eqList[i];
                  eqListElemType = eqListElem->getElementType();
                  if (eqListElemType == ET_Column || eqListElemType == ET_Expr)
                    elems.insert(eqListElem);
                }
            }
            break;

          default:
            assertLogAndThrow1(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                               FALSE, MVCandidateException,
                               "Unexpected element type in findDimOutputs: %d",
                               elem->getElementType());
        }
    }
}

// Look for agg fns that we entered in root node.
static void mapVidsForAggs(RETDesc* retDesc, ValueIdMap& vidMap, CollHeap* heap)
{
  const ColumnDescList* colDescList = retDesc->getColumnList();
  CollIndex numCols = colDescList->entries();
  const char* name;
  size_t prefixLen = strlen(AGG_NAME_PREFIX);
  for (CollIndex i=0; i<numCols; i++)
    {
      name = (*colDescList)[i]->getColRefNameObj().getColName().data();
      if (strlen(name) > prefixLen &&
          !strncmp(name, AGG_NAME_PREFIX, prefixLen))
        {
          vidMap.addMapEntry(atoi(name + prefixLen),
                             retDesc->getValueId(i));
        }
    }
}

void MVCandidates::analyzeCandidate(QRCandidatePtr candidate,
                                    JBBSubset& jbbSubsetToReplace,
                                    NABoolean rewriteSingleNode,
                                    CANodeIdSet& jbbSubsetNodes,
                                    CollHeap* heap)
{
  // If the MV is to be ignored - ignore it.
  const NAString& mvName = candidate->getMVName()->getMVName();
  if (isForbiddenMV(mvName))
  {
    QRLogger::log(CAT_SQL_COMP_MVCAND, LL_DEBUG,
      "MV Candidate %s was ignored because this statement writes to it.",
                  mvName.data());
    return;
  }

  // Some result descriptor elements with refs need to access the refed element,
  // which must be the element object used in the query descriptor.
  SetRefVisitorPtr visitor =
          new(heap) SetRefVisitor(qdescGenerator_.getColTblIdHash(),
                                  ADD_MEMCHECK_ARGS(heap));
  candidate->treeWalk(visitor);
  deletePtr(visitor);

  MVMatchPtr match = new(heap) MVMatch(&jbbSubsetToReplace, heap);
  match->setMvName(mvName);

  // Create a scan node, with a parent groupby node if additional grouping
  // of the MV is required. Attach this to a root, and then bind the tree
  // fragment using the binder work area for the query tree.
  QualifiedName mvQualName(match->getMvName(), 3, heap, bindWA_);
  CorrName mvCorrName(mvQualName, heap);
  // First check if the MV exist as it is possible that QMS returned an MV
  // that was just dropped or altered.  This should not be happening but 
  // sometimes if the publish takes longer it could happen.
  Lng32 marker = CmpCommon::diags()->mark();
  NATable *naTable = bindWA_->getNATable(mvCorrName);
  if (bindWA_->errStatus())
  {
    QRLogger::log(CAT_SQL_COMP_MVCAND, LL_DEBUG,
      "Skipping candidate MV %s since it does not exist",
      candidate->getMVName()->getMVName().data());
    for (Int32 i=1; i<=CmpCommon::diags()->getNumber(); i++)
    {
      const NAWchar *errorMsg = (*CmpCommon::diags())[i].getMessageText();
      NAString* errorStr = unicodeToChar(errorMsg, NAWstrlen(errorMsg), CharInfo::ISO88591, NULL, TRUE);
      QRLogger::log(CAT_SQL_COMP_MVCAND, LL_WARN, "  condition %d: %s", i, errorStr->data());
      delete errorStr;
    }
    CmpCommon::diags()->rewind(marker, TRUE);
    bindWA_->resetErrStatus();
    deletePtr(match);
    return;
  }

  MVInfoForDML* mvInfo = naTable->getMVInfo(bindWA_);
  if (mvInfo == NULL || !mvInfo->isInitialized())
  {
    QRLogger::log(CAT_SQL_COMP_MVCAND, LL_DEBUG,
      "Skipping candidate MV %s since it is not initialized",
                 candidate->getMVName()->getMVName().data());
    return;
  }
  
  GroupByAgg* gbNode = NULL;
  Scan* scan = new(heap) Scan(mvCorrName, NULL, REL_SCAN, heap);
  scan->setRewrittenMV();
  QRGroupByPtr groupBy = candidate->getGroupBy();

  // is this MV one of the favorite MVs included in the 
  // MVQR_REWRITE_CANDIDATES default
  NABoolean favoriteMV = FALSE;
  if (mvqrFavoriteCandidates_ != NULL)
    favoriteMV = isAfavoriteMV(match->getMvName());

  RelExpr* node = scan;

  // Back-join MV to any of its underlying tables needed to pick up columns
  // that aren't included in the MV. Make a single ItemExpr for all the back
  // join conditions and attach to topmost join as created above.
  bjScanHash_.clear();  // clear back joins from previous candidate
  QRTableListPtr bjTables = candidate->getTableList();
  NABoolean hasBackJoins = bjTables && bjTables->entries() > 0;
  RelExpr* backJoinRoot = NULL;
  if (hasBackJoins)
   {
      addBackJoins(candidate, bjTables, node, heap);
      backJoinRoot = node;

   }

  // If the candidate replaces an IGB, we need to determine which JBB of the
  // query descriptor it replaces (an IGB always constitutes an entire JBB).
  QRJBBPtr jbbElem = NULL;
  NodeIdScanHash dimScanHash(&nodeIdHashFn, 41, TRUE, heap);
  if (candidate->isIndirectGroupBy())
    {
      CollIndex jbbId = jbbSubsetToReplace.getJBB()->getJBBId();
      const NAPtrList<QRJBBPtr>& jbbList = queryDesc_->getJbbList();
      for (CollIndex jbbInx=0; jbbInx<jbbList.entries() && !jbbElem; jbbInx++)
        {
          if (jbbList[jbbInx]->getIDNum() == jbbId)
            jbbElem = jbbList[jbbInx];
        }
      assertLogAndThrow1(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                         jbbElem, MVCandidateException,
                         "Could not find JBB with id = %d", jbbId);
      addIGBDimJoins(candidate, jbbSubsetNodes, jbbElem, node, dimScanHash, heap);
    }

  if (groupBy && groupBy->getResult() == QRElement::NotProvided)
    node = gbNode = getGroupByAggNode(node, groupBy, candidate, heap);

  ElementPtrList vidMapTopElements(heap);
  RelRoot* root = 
        new(heap) RelRoot(node, REL_ROOT, NULL, NULL, NULL, NULL, heap);
  buildOutputExprs(candidate, root, vidMapTopElements, heap);

  rewriteRangePreds(candidate, scan, gbNode, heap);
  rewriteResidPreds(candidate, scan, gbNode, backJoinRoot, heap);

  // Bind the replacement tree.
  marker = CmpCommon::diags()->mark();
  RelExpr* boundRoot = root->bindNode(bindWA_);
  if (bindWA_->errStatus())
  {
    QRLogger::log(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
      "Skipping candidate MV %s -- failed to bind root",
       candidate->getMVName()->getMVName().data());
    for (Int32 i=1; i<=CmpCommon::diags()->getNumber(); i++)
    {
      const NAWchar *errorMsg = (*CmpCommon::diags())[i].getMessageText();
      NAString* errorStr = unicodeToChar(errorMsg, NAWstrlen(errorMsg), CharInfo::ISO88591, NULL, TRUE);
      QRLogger::log(CAT_SQL_COMP_MVCAND, LL_WARN, "  condition %d: %s", i, errorStr->data());
      delete errorStr;
    }
    CmpCommon::diags()->rewind(marker, TRUE);
    bindWA_->resetErrStatus();
    delete root;
    deletePtr(match);
    return;
  }

  // queryRoot_->getStoiList().insert(scan->getOptStoi());


  // Before transform removes the root node, get the mapvid entries for the
  // aggregate functions we added to its select list. These have been given
  // fake names that embed the value ids they map to.
  ValueIdMap vidMap;
  mapVidsForAggs(root->getRETDesc(), vidMap, heap);

  // Transform and normalize the new tree fragment.
  NormWA normWA(CmpCommon::context());
  ExprGroupId eg(boundRoot);
  normWA.allocateAndSetVEGRegion(IMPORT_AND_EXPORT, boundRoot);
  boundRoot->transformNode(normWA, eg);
  RelExpr* transformedRoot = eg.getPtr();

  transformedRoot->rewriteNode(normWA);
  normWA.setInMVQueryRewrite(TRUE);
  RelExpr* normalizedRoot = transformedRoot->normalizeNode(normWA);
  normWA.setInMVQueryRewrite(FALSE);

  // Take the id mappings from the output list and add them to the ValueIdMap,
  // and for an IGB, add mappings for columns of the dimension tables that are
  // used in the query's select list.
  ValueIdSet vegRewriteVids;
  populateVidMap(normalizedRoot, vidMapTopElements, vegRewriteVids, candidate,
                 vidMap, heap);
  if (candidate->isIndirectGroupBy())
    mapDimOutputs(jbbElem, jbbSubsetNodes, dimScanHash, vidMap, heap);
  MapValueIds* mapVidNode = new(heap) MapValueIds(normalizedRoot, vidMap, heap);
  mapVidNode->addValuesForVEGRewrite(vegRewriteVids);

  mapVidNode->synthLogProp(&normWA);
  normalizedRoot->finishSynthEstLogProp();

  mapVidNode->setIncludesFavoriteMV(favoriteMV);
  mapVidNode->primeGroupAttributes();
  //match->setMvRelExprTree(mapVidNode);


  // until the MVID for IGB uses vegref in the upper and lower values this transformation
  // cannot be used
  if (!candidate->isIndirectGroupBy())
  {
    // Make sure that the output for the operator under the MVID is minimized.
    // In addition to fixing this issue, this fix also ensures minimal message
    // sizes between this operator and its ancestor(s). When the mapvid inputs
    // (bottom values) contains a vegref that includes a constant, it may get
    // removed as a covered expression. To safeguard against this, we copy the
    // map, and restore it if it has shrunk as a result of the call to
    // pushdownCoveredExpr. See Solution 10-100707-1647.
    ValueIdMap mapCopy(mapVidNode->getMap());
     mapVidNode->pushdownCoveredExpr(mapVidNode->getGroupAttr()->getCharacteristicOutputs(),
                                     mapVidNode->getGroupAttr()->getCharacteristicInputs(),
                                     mapVidNode->selectionPred()
                                    );
    const ValueIdList& tops = mapCopy.getTopValues();
    const ValueIdList& bottoms = mapCopy.getBottomValues();
    if (tops.entries() > mapVidNode->getMap().getTopValues().entries())
    {
      mapVidNode->clear();
      for (CollIndex i=0; i<tops.entries(); i++)
        mapVidNode->addMapEntry(tops[i], bottoms[i]);
    }
  }


  // Save some info about the original query analysis before analyzing the
  // candidate tree. This info helps determine if we can test the rewrite by
  // trading out the entire tree.
  ULng32 maxJbbSize = QueryAnalysis::Instance()->getSizeOfLargestJBB();
  NABoolean multiJBBs = QueryAnalysis::Instance()->getJBBs().entries();

  RelExpr* analyzedExpr = QueryAnalysis::Instance()->analyzeThis(mapVidNode, TRUE);
  match->setMvRelExprTree(analyzedExpr);


  if (!rewriteSingleNode)
    // Insert preferred matches at the front of the list, others at back.
    jbbSubsetToReplace.getJBBSubsetAnalysis()->addMVMatch(match, candidate->isPreferredMatch());
  else
    {
       // this is a defensive check, since this same check was done in MVCandidates::analyzeCandidateMVs
       // but just to make sure nothing went wrong in between
       assertLogAndThrow(CAT_SQL_COMP_MVCAND, LL_MVQR_FAIL,
                       queryRoot_->child(0)->getOperator().match(REL_SCAN),
                       MVCandidateException,
                       "Expecting a SCAN node under the Query Root node while rewriting "
                       "a single Table Query");

       ((Scan *)(queryRoot_->child(0).getPtr()))->addMVMatch(match, candidate->isPreferredMatch());
    }
}  // analyzeCandidate()

void MVCandidates::analyzeJbbSubsets(QRJbbResultPtr jbb)
{
  const NAPtrList<QRJbbSubsetPtr>& jbbSubsetList = jbb->getJbbSubsets();
  for (CollIndex i=0; i<jbbSubsetList.entries(); i++)
    {
      analyzeCandidateMVs(jbbSubsetList[i]);
    }
}

// User maintained MVs are typically initialized by using an insert-select,
// that is a perfect match to the MV query. If we rewrite this insert-select,
// it will initialize the MV from its empty self.
// To make things a bit more generic, any statement doing an Insert, Update
// or Delete to an MV, should not be rewritten to use that MV.
// Here we collect the MV names that this statement writes to.
void MVCandidates::findForbiddenMVs()
{
  // Does the query tree have any Insert/UPdate/Delete nodes?
  if (queryRoot_->seenIUD() == FALSE)
    return;

  QueryAnalysis* qa = QueryAnalysis::Instance();
  CollHeap* heap = qa->outHeap();
  collectMVs(queryRoot_, heap);
}

// A recursive method to walk the tree and look for IUD nodes on MVs.
void MVCandidates::collectMVs(RelExpr* topNode, CollHeap* heap)
{
  if (topNode->getOperator().match(REL_ANY_GEN_UPDATE))
  {
    GenericUpdate* gu = (GenericUpdate*)topNode;
    if (gu->getTableDesc()->getNATable()->isAnMV())
    {
      NAString mvName = gu->getTableName().getQualifiedNameAsString();

      if (forbiddenMVs_ == NULL)
        forbiddenMVs_ = new(heap) LIST(NAString)(heap);

      forbiddenMVs_->insert(mvName);
    }
  }
  else // No need to look below GU nodes for more GU nodes.
  {
    for (Int32 i=0; i<topNode->getArity(); i++)
      collectMVs(topNode->child(i), heap);
  }
}

// Is the candidate MV name mvName apear in the forbidden list?
NABoolean MVCandidates::isForbiddenMV(const NAString& mvName)
{
  if (forbiddenMVs_ == NULL)
    return FALSE;

  for (CollIndex i=0; i<forbiddenMVs_->entries(); i++)
    if ((*forbiddenMVs_)[i] == mvName)
      return TRUE;

  return FALSE;
}

void MVCandidates::analyzeResultDescriptor(QRResultDescriptorPtr rd)
{
  findForbiddenMVs();

  const NAPtrList<QRJbbResultPtr>& jbbResultList = rd->getJbbResults();
  for (CollIndex i=0; i<jbbResultList.entries(); i++)
    {
      analyzeJbbSubsets(jbbResultList[i]);
    }

  // clean up the list of candidates MVs, they are no longer needed
  if (mvqrFavoriteCandidates_ != NULL)
  {
     mvqrFavoriteCandidates_->clear();
     delete mvqrFavoriteCandidates_;
  }
}
