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

#ifndef _MVCANDIDATES_H_
#define _MVCANDIDATES_H_

#include "QRDescriptor.h"
#include "QRSharedPtr.h"
#include "RelScan.h"
#include "RelGrby.h"
#include "ValueDesc.h"
#include "CmpMain.h"

// Classes defined in this file.
class MVCandidateException;
class MVCandidates;
class MVMatch;

// Other forward declarations.
class JBBSubset;
class JBBSubsetAnalysis;
class QRDescGenerator;
class VEGPredicate;

#ifdef _MEMSHAREDPTR
typedef QRIntrusiveSharedPtr<MVMatch> MVMatchPtr;            
#else
typedef MVMatch* MVMatchPtr;            
#endif

// Hash node id from query to get corresponding dim table scan in rewrite of
// Indirect Group By (IGB) query.
typedef NAHashDictionary<CollIndex, Scan> NodeIdScanHash;

#define AGG_NAME_PREFIX "@MVQR"

/**
 * \file
 * Contains the definition of MVCandidates, the class used to translate the
 * result descriptor returned from a MATCH operation into internal optimizer
 * structures. These structures consist primarily of a list of MVMatch objects
 * that are stored with the JBBSubsetAnalysis object corresponding to the JBB
 * subset that the matches are for. The MVMatch class is also defined in this
 * file.
 */

/**
 * Exception thrown when a candidate in the result descriptor must be discarded
 * because an internal error occurred when processing it, or it requires a
 * capability not yet present in the code. This exception should be caught, so
 * the remaining candidates in the result descriptor can be considered, just as
 * if the offending candidate did not exist.
 */
class MVCandidateException : public QRException
{
  public:
    /**
     * Creates an exception with text consisting of the passed template filled in
     * with the values of the other arguments.
     *
     * @param[in] msgTemplate Template for construction of the full message;
     *                        contains printf-style placeholders for arguments,
     *                        passed as part of a variable argument list.
     * @param[in] ... Variable argument list, consisting of a value for each
     *                placeholder in the message template.
     */
    MVCandidateException(const char *msgTemplate ...)
      : QRException()
    {
      qrBuildMessage(msgTemplate, msgBuffer_);
    }

    virtual ~MVCandidateException()
    {}

}; //MVCandidateException

/**
 * Class used to analyze the result descriptor corresponding to a match
 * operation, and create a list of MVMatch objects for each JBB subset that
 * can potentially be replaced by an MV. This is done by passing a result
 * descriptor to the analyzeResultDescriptor() method, which uses several
 * private helper functions to create the match objects.
 */
class MVCandidates
{
  public:
    /**
     * Base of correlation name used for backjoin tables, cmpleted by adding
     * table id. This is needed in case of self-join of a backjoin table.
     */
    static const char* const BACKJOIN_CORRNAME_PREFIX;

    /**
     * Creates an MVCandidates object to produce MVMatch objects for subsets of
     * JBBs contained in the query tree denoted by \c root.
     *
     * @param root Root node of the query being rewritten.
     * @param queryDesc Query descriptor for query being rewritten. This is
     *                  used to rewrite indirect Group By queries.
     * @param descGenerator Object used to generate the query descriptor. Used
     *                      to resolve references to columns from the group by
     *                      list in a candidate based on indirect group by.
     */
    MVCandidates(RelRoot* root,
                 QRQueryDescriptorPtr queryDesc,
                 QRDescGenerator& descGenerator);

    /**
     * Counts the number of nodes in an expression tree. This is used to select
     * the candidate with the fewest nodes in its replacement tree when testing
     * QR by forcing a rewrite of the query in the Analyzer. It is a static
     * member function just for the sake of placing it in a namespace.
     *
     * @param node Find the number of nodes in the subtree rooted here.
     * @return Number of nodes in the tree rooted by \c node.
     */
    static Int32 nodeCount(ExprNode* node);

    /**
     * Analyze a result descriptor, creating MVMatch objects that are attached
     * to the analysis objects for the JBB subsets they match.
     *
     * @param rd Result descriptor containing information about possible matches
     *           and rewrite instructions for the query.
     */
    void analyzeResultDescriptor(QRResultDescriptorPtr rd);

     /**
     * Creates a list of Favorite MVs from the value of CQD
     * MVQR_REWRITE_CANDIDATES.
     * @param heap Heap to use for any allocations.
     */
    void buildFavoritesList (CollHeap* heap);

    /**
     * Returns whether the value of the parameter is in the list of favorite MVs
     *
     * @param mvName Name of the new candidate MV
     * @return TRUE when mvName is in the favorites list
     */
    NABoolean isAfavoriteMV(const NAString& mvName)
      {
        return mvqrFavoriteCandidates_->contains(mvName);
      }
    
  private:
    /**
     * Parses the expression text in an <Expr> element, returning an \c ItemExpr*.
     * The inputs listed in the contained <Input> element are supplied as
     * arguments to the expression.
     *
     * @param expr Deserialized <Expr> element.
     * @param heap Heap to use for any allocations.
     * @return Item expression resulting from the parse of the expression text.
     */
    ItemExpr* parseExpression(QRExprPtr expr, const NAString& mvName, CollHeap* heap);

    /**
     * Creates the \c GroupByAgg node that will be inserted in the replacement
     * tree as the parent of the scan node for the MV. This function is called
     * only when supplemental grouping is required on the candidate MV.
     *
     * @param scanNode The scan node created for the MV.
     * @param groupBy Object corresponding to <GroubNy> element in the result
     *                descriptor.
     * @param candidate The MV candidate.
     * @param heap Heap to be used for any dynamic memory allocations.
     * @return Pointer to the \c GroupByAgg node created for grouping the rows
     *         of the MV.
     */
    GroupByAgg* getGroupByAggNode(RelExpr* childNode,
                                  QRGroupByPtr groupBy,
                                  QRCandidatePtr candidate,
                                  CollHeap* heap);

    void getEqSetMembers(VEGPredicate* vegPred, ValueIdSet& vegMembers);

    ItemExpr* rewriteVegPredicate(const ElementPtrList& mvColumns,
                                  VEGPredicate* itemExpr,
                                  QRCandidatePtr candidate,
                                  CollHeap* heap);
                                 // CollIndex tblId);
    /**
     * Creates an item expression which is a copy of the one passed into the
     * function, but inserting a \c ColReference for the passed MV column in
     * place of any instances of the column that MV col matches.
     *
     * @param mvCol MV column used to replace occurrences of a column from the
     *              original expression.
     * @param itemExpr The original expression.
     * @param candidate The candidate object, representing a rewrite MV.
     * @param heap Heap to use.
     * @return Copy of the \c ItemExpr with the MVColumn substituted in.
     */
    ItemExpr* rewriteItemExpr(QRElementPtr mvCol,
                              ItemExpr* itemExpr,
                              QRCandidatePtr candidate,
                              CollHeap* heap);

    /**
     * Creates an item expression which is a copy of the one passed into the
     * function, but inserting \c ColReferences for the MV columns in the passed
     * list in place of instances of the columns they match.
     *
     * @param mvColumns List of MV columns to use for substitutions.
     * @param itemExpr The original expression.
     * @param candidate The candidate object, representing a rewrite MV.
     * @param heap Heap to use.
     * @param tblId Node id of the table the expression is from. This is only
     *              used for predicates in IGB queries.
     * @return Copy of the \c ItemExpr with the MVColumns substituted in.
     */
    ItemExpr* rewriteItemExpr(const ElementPtrList& mvColumns,
                              ItemExpr* itemExpr,
                              QRCandidatePtr candidate,
                              CollHeap* heap,
                              CollIndex tblId = 0);

    /**
     * Rewrites each of the range predicates identified for the given candidate.
     *
     * @param candidate The candidate object, representing a rewrite MV.
     * @param scan The Scan relexpr for the MV. Rewritten preds are added to it.
     * @param groupBy Group By node on top of the MV scan node, null if there isn't
     *                one. Add rewritten pred here if it contains an aggregate.
     * @param heap Heap to use for any allocations.
     */
    void rewriteRangePreds(QRCandidatePtr candidate,
                           RelExpr* scan,
                           RelExpr* groupBy,
                           CollHeap* heap);

    /**
     * Rewrites each of the residual predicates identified for the given candidate.
     *
     * @param candidate The candidate object, representing a rewrite MV.
     * @param scan The Scan relexpr for the MV. Rewritten preds are added to it.
     * @param groupBy Group By node on top of the MV scan node, null if there isn't
     *                one. Add rewritten pred here if it contains an aggregate.
     * @param heap Heap to use for any allocations.
     */
    void rewriteResidPreds(QRCandidatePtr candidate,
                           RelExpr* scan,
                           RelExpr* groupBy,
                           RelExpr* bjRoot,
                           CollHeap* heap);

    /**
     * Returns a pointer to the \c ColRefName associated with a specified
     * element within a candidate. The element must be either an MVColumn
     * or a Column.
     *
     * @param candidate Candidate containing the element.
     * @param elem The element to get the \c ColRefName for.
     * @param heap Heap to use for allocation.
     * @return Ptr to \c ColRefName object for the element.
     */
    ColRefName* getColRefName(QRCandidatePtr candidate,
                              QRElementPtr elem,
                              CollHeap* heap);

    /**
     * Returns the value id to use in the \c MapValueIds as the bottom value
     * for the passed element, which represents one of the outputs of the root
     * of the candidate query tree. The map node translates this and other
     * value ids from the candidate to the corresponding value ids used in the
     * query being rewritten.
     *
     * @param retDesc \c RETDesc for the table derived by the candidate tree.
     * @param elem Element to get the value id of.
     * @param candidate Candidate containing the element.
     * @param heap Heap to use for allocations.
     * @return Value id to use as source for mapping to a corresponding value
     *         id from the query.
     */
    ValueId getRewriteVid(RETDesc* retDesc,
                          QRElementPtr elem,
                          QRCandidatePtr candidate,
                          CollHeap* heap);

    ValueId getBaseColValueId(QRElementPtr referencingElem);

    /**
     * Returns the ValueId of the vegref derived from the reference attribute of
     * #referencingElem. If the referenced element is a column, the vegref id
     * stored in the QRColumn object is returned. If not a column, the ref number
     * of the referencing element itself is returned. This will typically be a
     * QROutput object, the reference number of which is the vegref of the matched
     * column. This function is used to supply a top value in the MapValueIDs node.
     *
     * @param referencingElem Element that references a vegref id, or the id of a
     *                        QRColumn that stores the vegref id.
     * @return The vegref id to use as a top value in the MapValueIDs node.
     */
    ValueId getVegrefValueId(QRElementPtr referencingElem);

    /**
     * Adds entries to a \c ValueIdMap mapping outputs from a rewrite to the
     * corresponding value ids from the query being rewritten.
     *
     * @param root Root node of the candidate tree. The \c RETDesc for this node
     *             supplies the input values to the mapping.
     * @param vidMapTopElements List of \c Output elements from the candidate.
     *                          These supply the mapping outputs (top values).
     * @param candidate Candidate the node is being created for.
     * @param vidMap Value id map to populate.
     * @param heap Heap to use for allocations.
     * @see #buildOutputExprs Creates the list of output elements used here.
     */
    void populateVidMap(RelExpr* root,
                        ElementPtrList& vidMapTopElements,
                        ValueIdSet& vegRewriteVids,
                        QRCandidatePtr candidate,
                        ValueIdMap& vidMap,
                        CollHeap* heap);


    /**
     * Adds entries to a \c ValueIdMap mapping outputs from a rewrite to the
     * corresponding value ids from the query being rewritten.
     *
     * @param root Root node of the candidate tree. The \c RETDesc for this node
     *             supplies the input values to the mapping.
     * @param elem One output element from the candidate. Will be used as the 
     *             mapping top value.
     * @param candidate Candidate the node is being created for.
     * @param vidMap Value id map to populate.
     * @param heap Heap to use for allocations.
     * @see #buildOutputExprs Creates the list of output elements used here.
     */
    void populateOneVidMap(RelExpr* root,
                           ValueIdSet& vegRewriteVids,
                           QRElementPtr elem,
                           QRCandidatePtr candidate,
                           ValueIdMap& vidMap,
                           CollHeap* heap);


    /**
     * Adds the predicates from \c list to the scan node. This function is
     * called for the rewrite of IGB queries, to add range and residual
     * predicates to the selection predicate of a node representing a dimension
     * table.
     *
     * @param candidate MV candidate being used for the rewrite.
     * @param scan Scan object for a dimension table that is part of an IGB
     *             rewrite.
     * @param tableNodeId The node id that a predicate in the list must match
     *                    before being added to the scan.
     * @param list List of either range or residual predicates from the MV
     *             candidate.
     * @param heap Heap to use for any allocations.
     */
    template <class T>
    void addPredsFromList(QRCandidatePtr candidate, Scan* scan,
                          CANodeId tableNodeId, const NAPtrList<T>& list,
                          CollHeap* heap);

    /**
     * Adds the range and residual predicates of the passed \c Scan node, which
     * is a scan of a dimension table in an IGB rewrite.
     *
     * @param candidate MV candidate being used for the rewrite.
     * @param scan Scan object for the dimension table that will have its
     *             selection predicates added.
     * @param tableNodeId The CA node id for the table.
     * @param jbb Pointer to JBB element containing the range and residual
     *            predicates.
     * @param heap Heap to use for any allocations.
     */
    void addIGBPreds(QRCandidatePtr candidate, Scan* scan, CANodeId tableNodeId,
                     QRJBBPtr jbb, CollHeap* heap);

    /**
     * 
     * @param elem 
     * @param candidate 
     * @param jbbSubsetNodes 
     * @param fromFactTable 
     * @param heap 
     * @return 
     */
    ItemExpr* getIGBJoinCondOp(QRElementPtr elem,
                               QRCandidatePtr candidate,
                               CANodeIdSet& jbbSubsetNodes,
                               NABoolean& fromFactTable,
                               CollHeap* heap);

    /**
     * Builds a join backbone consisting of \c node and the dimension tables in
     * an IGB rewrite. \c node is updated to point to the top join node
     * produced. In the process of building the backbone, the created scan nodes
     * for the dimension tables are added to \c dimScanHash, a hash table
     * mapping table node ids from the original query to the scan node of the
     * corresponding table in the rewrite.
     *
     * @param candidate The MV candidate being used for the IGB rewrite.
     * @param jbbSubsetNodes Set of nodes for the fact tables.
     * @param jbbElem JBB element being rewritten.
     * @param node [inout] Node to join the dimension tables to. Updated to be
     *                     the topmost join resulting from this process.
     * @param dimScanHash [out] Hash table mapping node ids from original query
     *                          to scan nodes of the corresponding dimension
     *                          tables in the rewrite.
     * @param heap Heap to use for any allocations.
     */
    void addIGBDimJoins(QRCandidatePtr candidate,
                        CANodeIdSet& jbbSubsetNodes,
                        QRJBBPtr jbbElem,
                        RelExpr*& node,
                        NodeIdScanHash& dimScanHash,
                        CollHeap* heap);

    /**
     * Adds value id mappings to \c vidMap for any of the rewritten query's
     * select list items that are provided by the dimension tables of an IGB
     * query. These values are not part of the output list of the MV candidate,
     * which is based on only the fact tables. The dimension tables are, however,
     * part of the rewrite, so any value ids from them must be mapped to the
     * corresponding value ids from the original query.
     *
     * @param jbbElem JBB element being rewritten.
     * @param jbbSubsetNodes Set of CA node ids for the fact tables.
     * @param dimScanHash Hash table allowing lookup of dimension table scan
     *                    nodes from the node id of the corresponding dimension
     *                    table nodes from the original query.
     * @param vidMap Value id map to which the mappings will be added.
     * @param heap Heap used for any allocations.
     */
    void mapDimOutputs(QRJBBPtr jbbElem,
                       CANodeIdSet& jbbSubsetNodes,
                       NodeIdScanHash& dimScanHash,
                       ValueIdMap& vidMap,
                       CollHeap* heap);

    /**
     * Back joins needed tables to the MV candidate being processed.

     * @param candidate The MV candidate.
     * @param bjTables List of tables to be back-joined to the MV.
     * @param [out] node The highest join node in the resulting tree.
     * @param heap Heap to use for any allocations.
     */
    void addBackJoins(QRCandidatePtr candidate,
                      QRTableListPtr bjTables,
                      RelExpr*& node,
                      CollHeap* heap);

    /**
     * Adds the characteristic outputs of the root node of the tree representing
     * the rewrite of a JBB subset. This typically requires the addition of a
     * \c MapValueIds node to translate the \c ValueIds of MV columns to the
     * corresponding ValueIds used in the query. In the case of a rewrite for an
     * IGB (Indirect Group By) query, ONLY the value id mapping is performed, and
     * the root node select list is not created.
     *
     * @param candidate The candidate MV.
     * @param root The root of the replacement tree, or \c NULL if called for an
     *             IGB query.
     * @param topValElemList 
     * @param heap Heap to allocate from.
     */
    void buildOutputExprs(QRCandidatePtr candidate,
                          RelRoot* root,
                          ElementPtrList& topValElemList,
                          CollHeap* heap);

    /**
     * Creates MVMatch objects corresponding to the candidates for the given
     * JBBSubset.
     *
     * @param qrJbbSubset JBB subset to analyze candidates for.
     */
    void analyzeCandidateMVs(QRJbbSubsetPtr qrJbbSubset);

    /**
     * Creates an MVMatch object for the passed MV candidate, and inserts it in
     * the list of matches for the given jbb subset.
     *
     * @param candidate Object representing an MV which is a candidate to replace
     *                  the JBB subset.
     * @param jbbSubsetToReplace JBB subset that may be replaced by the candidate.
     * @param testRewriteSingle TRUE iff we are to simulate a rewrite of a
     *                          single-node JBB
     * @param jbbSubsetNodes Set of CA nodes belonging to the jbbsubset.
     * @param heap Heap to use for any allocations.
     */
    void analyzeCandidate(QRCandidatePtr candidate,
                          JBBSubset& jbbSubsetToReplace,
                          NABoolean testRewriteSingle,
                          CANodeIdSet& jbbSubsetNodes,
                          CollHeap* heap);
    /**
     * Creates the MVMatch objects for each JBB subset contained in the given
     * JBB element from the result descriptor.
     *
     * @param jbb Pointer to the JBB element to process the JBB subsets of.
     */
    void analyzeJbbSubsets(QRJbbResultPtr jbb);

    void findForbiddenMVs();
    void collectMVs(RelExpr* topNode, CollHeap* heap);
    NABoolean isForbiddenMV(const NAString& mvName);

    NABoolean isRollup(QRCandidatePtr candidate)
    {
      QRGroupByPtr groupBy = candidate->getGroupBy();
      return groupBy!= NULL && groupBy->getResult() == QRElement::NotProvided;
    }


    /**
     * Binder work area used for the query tree being rewritten.
     */
    BindWA* bindWA_;

    /**
     * Root of the query being rewritten.
     */
    RelRoot* queryRoot_;

    /**
     * Query descriptor for query being rewritten.
     */
    QRQueryDescriptorPtr queryDesc_;

    /**
     * Object used to generate the query descriptor.
     */
    QRDescGenerator& qdescGenerator_;

    // Copy construction/assignment not defined.
    MVCandidates(const MVCandidates&);
    MVCandidates& operator=(const MVCandidates&);

    // list of favorite MVs specified in the MVQR_FAVORITE_CANDIDATES
    LIST(NAString)* mvqrFavoriteCandidates_;

    // List of MVs to ignore
    LIST(NAString)* forbiddenMVs_;

    /**
     * Hash table mapping back join table ids to the corresponding scan nodes,
     * so range predicates on back join nodes can be added later. Also used to
     * see if a table id is for a back join, so special correlation name can
     * be used. This hash table is cleared before processing each new candidate
     * MV, in analyzeCandidate().
     */
     TableNameScanHash bjScanHash_;

};  // class MVCandidates

/**
 * Class representing an MV match for a particular JBB subset. The match
 * object contains the MV name, a pointer to the JBB subset it can replace,
 * and RelExpr subtree that replaces the part of the tree corresponding to
 * that JBB subset.
 */
class MVMatch : public NAIntrusiveSharedPtrObject
{
  public:
    MVMatch(JBBSubset* equivJbbSubset,
            ADD_MEMCHECK_ARGS_DECL(CollHeap* heap = NULL))
      : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap)),
        heap_(heap),
        equivalentJbbSubset_(equivJbbSubset),
        mvName_(heap),
        mvRelExprTree_(NULL),
        alreadyOptimzed_ (FALSE)
      {}

    JBBSubset* getEquivalentJbbSubset() const
      {
        return equivalentJbbSubset_;
      }

    void setEquivalentJbbSubset(JBBSubset* jbbSubset)
      {
        equivalentJbbSubset_ = jbbSubset;
      }

    const NAString& getMvName() const
      {
        return mvName_;
      }

    void setMvName(const NAString& mvName)
      {
        mvName_ = mvName;
      }

    RelExpr* getMvRelExprTree() const
      {
        return mvRelExprTree_;
      }

    void setMvRelExprTree(RelExpr* expr)
      {
        mvRelExprTree_ = expr;
      }

    NABoolean alreadyOptimized ()
      {
        return alreadyOptimzed_;
      }

    void setAlreadyOptimized()
      {
        alreadyOptimzed_ = TRUE;
      }

  private:
    // Copy construction/assignment not defined.
    MVMatch(const MVMatch&);
    MVMatch& operator=(const MVMatch&);

    NAMemory* heap_;
    JBBSubset* equivalentJbbSubset_;
    NAString mvName_;
    RelExpr* mvRelExprTree_;

    NABoolean alreadyOptimzed_;
};  // class MVMatch

#endif  /* _MVCANDIDATES_H_ */
