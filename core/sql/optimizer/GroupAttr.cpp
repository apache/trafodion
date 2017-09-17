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
**************************************************************************
*
* File:         GroupAttr.C
* Description:  Group Attributes
* Created:      11/16/1994
* Language:     C++
*
*
*
*
**************************************************************************
*/

// -----------------------------------------------------------------------

#include "Sqlcomp.h"
#include "GroupAttr.h"
#include "EstLogProp.h"
#include "ItemConstr.h"
#include "ItemOther.h"
#include "opt.h"
#include "Cost.h"
#include "AppliedStatMan.h"
#include "Analyzer.h"
#include "ScanOptimizer.h"
#include "RelGrby.h" /// temproray, delete after ASM testing
#include "CmpStatement.h" 

// return hash value of a ValueId; this is called (and required) by
// NAHashDictionary<K,V>::getHashCode() to compute a key's hash address.

// typedef ULng32 ULng32;

static ULng32 valueIdHashKeyFunc(const ValueId& vid)
{
  CollIndex x = vid;
  return (ULng32)x;
}

// -----------------------------------------------------------------------
// Constructor for GroupAttributes
// -----------------------------------------------------------------------
GroupAttributes::GroupAttributes() :
     // NB: we don't initialize all data members in this ctor!
     // requiredInputs_
     // requiredOutputs_
     recordLength_(0),
     // inputVarLength_
     numBaseTables_(0),  // only true base tables are counted here
     minChildEstRowCount_(csZero),
     numJoinedTables_(1), // count this node as one table
     numTMUDFs_(0),
     logExprForSynthesis_(NULL),
     inputEstLogProp_(STMTHEAP),
     intermedOutputLogProp_(STMTHEAP),
     outputEstLogProp_(STMTHEAP),
     outputCardinalityForEmptyLogProp_(-1),
     outputMaxCardinalityForEmptyLogProp_(-1),
     availableBtreeIndexes_(STMTHEAP),
     // QSTUFF
     stream_ (FALSE),
     skipInitialScan_(FALSE),
     embeddedIUD_(NO_OPERATOR_TYPE),
     genericUpdateRoot_ (FALSE),
     reorderNeeded_(FALSE),
     // QSTUFF
     cachedSkewValuesPtr_(NULL),
     hasRefOptConstraint_(FALSE),
     hasCompRefOptConstraint_(FALSE),
     potential_(-1),
     hasNonDeterministicUDRs_(FALSE),
     probeCacheable_(FALSE),
     isSeabaseMD_(FALSE)
{
  groupAnalysis_ = new (STMTHEAP) GroupAnalysis(this,STMTHEAP);
}

typedef NAHashDictionary<ValueId,SkewedValueList> vIdSkewValueListHashDict_t;

// copy ctor called by /generator/GenPreCode.cpp
GroupAttributes::GroupAttributes (const GroupAttributes & rhs) :
     requiredInputs_(rhs.requiredInputs_),
     requiredOutputs_(rhs.requiredOutputs_),
     requiredEssentialOutputs_(rhs.requiredEssentialOutputs_),
     recordLength_(rhs.recordLength_),
     inputVarLength_(rhs.inputVarLength_),
     numBaseTables_(rhs.numBaseTables_),
     minChildEstRowCount_(rhs.minChildEstRowCount_),
     numJoinedTables_(rhs.numJoinedTables_),
     numTMUDFs_(rhs.numTMUDFs_),
     logExprForSynthesis_(rhs.logExprForSynthesis_),
     inputEstLogProp_(rhs.inputEstLogProp_, STMTHEAP),
     intermedOutputLogProp_(rhs.intermedOutputLogProp_, STMTHEAP),
     outputEstLogProp_(rhs.outputEstLogProp_, STMTHEAP),
     outputCardinalityForEmptyLogProp_(rhs.outputCardinalityForEmptyLogProp_),
     outputMaxCardinalityForEmptyLogProp_(rhs.outputMaxCardinalityForEmptyLogProp_),
     availableBtreeIndexes_(rhs.availableBtreeIndexes_, STMTHEAP),
     constraints_(rhs.constraints_),
     // QSTUFF
     stream_ (rhs.stream_),
     skipInitialScan_(rhs.skipInitialScan_),
     embeddedIUD_(rhs.embeddedIUD_),
     genericUpdateRoot_ (rhs.genericUpdateRoot_),
     reorderNeeded_(rhs.reorderNeeded_),
     genericUpdateRootOutputs_(rhs.genericUpdateRootOutputs_),
     // QSTUFF
     hasRefOptConstraint_(rhs.hasRefOptConstraint_),
     hasCompRefOptConstraint_(rhs.hasCompRefOptConstraint_),
     potential_(rhs.potential_),
     hasNonDeterministicUDRs_(rhs.hasNonDeterministicUDRs_),
     probeCacheable_(rhs.probeCacheable_),
     isSeabaseMD_(rhs.isSeabaseMD_)
{
  groupAnalysis_ = new (STMTHEAP) GroupAnalysis(*(rhs.groupAnalysis_),STMTHEAP);
  // need to reset the groupAttributes pointer in groupAnalysis_ to
  // point to this object rather than rhs
  groupAnalysis_->groupAttr_ = this;

  if (rhs.cachedSkewValuesPtr_)
    cachedSkewValuesPtr_ = new (STMTHEAP) vIdSkewValueListHashDict_t(*(rhs.cachedSkewValuesPtr_));
  else
    cachedSkewValuesPtr_ = NULL;
}

GroupAttributes::~GroupAttributes()
{
  if (cachedSkewValuesPtr_) {
    NADELETE(cachedSkewValuesPtr_, vIdSkewValueListHashDict_t, STMTHEAP);
  }
}

// -----------------------------------------------------------------------
// normalizeInputsAndOutputs()
//
// This method walks through the Characteristic Inputs and Outputs
// and adds a VEGReference for each member that belongs to a VEG.
// It deletes all values that are covered by the VEGReference.
// -----------------------------------------------------------------------
void GroupAttributes::normalizeInputsAndOutputs(NormWA & normWARef)
{
  requiredInputs_.normalizeNode(normWARef);
  requiredOutputs_.normalizeNode(normWARef);
} // GroupAttributes::normalizeInputsAndOutputs()

// -----------------------------------------------------------------------
// methods dealing with constraints
// -----------------------------------------------------------------------

void GroupAttributes::addConstraint(ItemExpr *c)
{
  NABoolean duplicateConstraint = FALSE;
  NABoolean brandNewConstraint = FALSE;

  // Check if c is a brand new constraint item expression
  if (c->getValueId() == NULL_VALUE_ID)
    brandNewConstraint = TRUE;

  // check for duplicates
  switch (c->getOperatorType())
    {
    case ITM_CARD_CONSTRAINT:
      {
        CardConstraint *cc = (CardConstraint *) c;
        Cardinality minRows,maxRows;

        hasCardConstraint(minRows,maxRows);
        cc->setLowerBound(MAXOF(minRows,cc->getLowerBound()));
        cc->setUpperBound(MINOF(maxRows,cc->getUpperBound()));

        if (cc->getLowerBound() > minRows OR
            cc->getUpperBound() < maxRows)
          {
            // this improves any existing cardinality constraint
            // go and take the old cardinality constraints out of the
            // set because they don't supply interesting info
            for (ValueId oc = constraints_.init();
                 constraints_.next(oc);
                 constraints_.advance(oc))
              if (oc.getItemExpr()->getOperatorType() == ITM_CARD_CONSTRAINT)
                constraints_ -= oc;
          }
        else
          {
            // this is no news, delete this useless new constraint
            duplicateConstraint = TRUE;
          }
      }
      break;

    case ITM_UNIQUE_OPT_CONSTRAINT:
      {
        UniqueOptConstraint *uc = (UniqueOptConstraint *) c;

        // check existing uniqueness constraints whether they are similar
        // and combine uniqueness constraints if possible
        for (ValueId ouc = constraints_.init();
             constraints_.next(ouc);
             constraints_.advance(ouc))
          {
            if (ouc.getItemExpr()->getOperatorType() ==
		ITM_UNIQUE_OPT_CONSTRAINT)
              {
                const ValueIdSet &oucCols =
                  ((UniqueOptConstraint *) ouc.getItemExpr())->uniqueCols();

                if (uc->uniqueCols().contains(oucCols))
                  {
                    // this is no news, delete this useless new constraint
                    duplicateConstraint = TRUE;
                  }
                else if(oucCols.contains(uc->uniqueCols()))
                  {
                    // we are improving an existing uniqueness constraint,
                    // take the existing one out
                    constraints_ -= ouc;
                  }
              } // this is an existing uniqueness constraint
	    else if (ouc.getItemExpr()->getOperatorType() ==
		     ITM_FUNC_DEPEND_CONSTRAINT)
	      {
		// try to reduce the new constraint by eliminating columns
		// that are functionally dependent on the rest
		((FuncDependencyConstraint *) ouc.getItemExpr())->
		  minimizeUniqueCols(uc->uniqueCols());

		// if the determining columns of the functional dependency
		// contain the new unique columns then that functional
		// dependency constraint is no longer useful
		if (((FuncDependencyConstraint *) ouc.getItemExpr())->
		    getDeterminingCols().contains(uc->uniqueCols()))
		  constraints_ -= ouc;
	      }
          } // for each existing constraint
      }
      break;

    case ITM_FUNC_DEPEND_CONSTRAINT:
      {
        FuncDependencyConstraint *fdc = (FuncDependencyConstraint *) c;

	if (isUnique(fdc->getDeterminingCols()))
	  {
	    // if the determining columns are unique then there is no
	    // need to keep track of functional dependencies, we know that
	    // all other columns are dependent on unique columns
	    duplicateConstraint = TRUE;
	  }
	else
	  {
	    // check existing functional dependency constraints
	    // whether they are similar and combine them if possible

	    const ValueIdSet & newDeterminingCols = fdc->getDeterminingCols();
	    ValueIdSet         newDependentCols(fdc->getDependentCols());
	    NABoolean          modifiedNewConstraint = FALSE;

	    for (ValueId ofc = constraints_.init();
		 constraints_.next(ofc);
		 constraints_.advance(ofc))
	      {
		if (ofc.getItemExpr()->getOperatorType() ==
		    ITM_FUNC_DEPEND_CONSTRAINT)
		  {
		    FuncDependencyConstraint * odc =
		      (FuncDependencyConstraint *) ofc.getItemExpr();
		    const ValueIdSet &odcCols = odc->getDeterminingCols();

		    if (newDeterminingCols.contains(odcCols))
		      {
			if (odc->getDependentCols().contains(newDependentCols))
			  {
			    // an equal or better constraint already exists
			    duplicateConstraint = TRUE;
			    break;
			  }
			else if (newDeterminingCols == odcCols)
			  {
			    // replace existing constraint, the new one
			    // has extra dependent columns
			    newDependentCols += odc->getDependentCols();
			    constraints_ -= ofc;
			    modifiedNewConstraint = TRUE;
			  }
		      }
		    else
		      if (odcCols.contains(newDeterminingCols) AND
			  newDependentCols.contains(odc->getDependentCols()))
		      {
			// the existing constraint is less useful than the
			// new one, delete it
			constraints_ -= ofc;
		      }
		  } // this is an existing functional dependency constraint
	      } // for each existing constraint

	    if (modifiedNewConstraint)
	      {
		c = new(CmpCommon::statementHeap())
		  FuncDependencyConstraint(newDeterminingCols,
					   newDependentCols);
		brandNewConstraint = TRUE;
	      }
	  }
      }
      break;

    case ITM_CHECK_OPT_CONSTRAINT:
      {
        CheckOptConstraint *cc = (CheckOptConstraint *) c;

        // check existing uniqueness constraints whether they are similar
        // and combine uniqueness constraints if possible
        for (ValueId occ = constraints_.init();
             constraints_.next(occ);
             constraints_.advance(occ))
          {
            if (occ.getItemExpr()->getOperatorType() ==
		ITM_CHECK_OPT_CONSTRAINT)
              {
                const ValueIdSet &occPreds =
                  ((CheckOptConstraint *) occ.getItemExpr())->getCheckPreds();

                // Note that the check for inclusion of value ids of
                // the predicates is not very useful. A more useful
                // check would be whether the existing predicates
                // imply the new ones or vice versa, but that would be
                // much more complicated code and is not required at
                // this point.

                if (occPreds.contains(cc->getCheckPreds()))
                  {
                    // this is no news, delete this useless new constraint
                    duplicateConstraint = TRUE;
                  }
                else if(cc->getCheckPreds().contains(cc->getCheckPreds()))
                  {
                    // we are improving an existing check constraint,
                    // take the existing one out
                    constraints_ -= occ;
                  }
              } // this is an existing uniqueness constraint
          } // for each existing constraint
      }
      break;

    case ITM_UNIQUE_CONSTRAINT:
      DCMPASSERT("Wrong constraint type used in GA" == 0); // LCOV_EXCL_LINE
      break;

    default:
      break;
    }


  if (duplicateConstraint)
  {
    // If this was a brand new constraint, then we never used it anywhere,
    // so we can safely delete it.
    if (brandNewConstraint)
      delete c;
  }
  else
  {
    // If this is a brand new constraint, then we need to allocate a
    // value id for it.
    if (brandNewConstraint)
    {
      c->allocValueId();
    }
    addConstraint(c->getValueId());
  }
}

// This method returns the operation as a string to be used
// in reporting errors. 
// This method is primarily used for reporting errors
// on embedded IUD. Most error messages are common to all IUD,
// just differing in the operation name.
const NAString GroupAttributes::getOperationWithinGroup() const
{
  switch (embeddedIUD_)
    {
    case REL_UNARY_UPDATE:
      return "UPDATE";
    case REL_UNARY_DELETE:
      return "DELETE";
    case REL_UNARY_INSERT:
      return "INSERT";
    default:
      return "UNKNOWN??";
    } // switch
} // GroupAttributes::getOperationAsStringWithinGroup()

NABoolean GroupAttributes::isUnique(const ValueIdSet &cols) const
{
  for (ValueId x= constraints_.init();
       constraints_.next(x);
       constraints_.advance(x) )
    {
      if (x.getItemExpr()->getOperatorType() == ITM_UNIQUE_OPT_CONSTRAINT)
        {
          UniqueOptConstraint *u = (UniqueOptConstraint *) x.getItemExpr();

          if (cols.contains(u->uniqueCols()))
            return TRUE;
        }
      else if (x.getItemExpr()->getOperatorType() == ITM_CARD_CONSTRAINT)
        {
          CardConstraint *c = (CardConstraint *) x.getItemExpr();

          // If at most one row can be returned everthing is unqiue
          if (c->getUpperBound() <= 1)
            return TRUE;
        }
    }

  return FALSE;
}

// this method goes through the list of constraints for a GroupAttribut
// and returns TRUE if a set of unique columns is found. If multiple sets of
// columns that guarantee uniqueness exist, the first set in constraints_
// list is returned.
NABoolean GroupAttributes::findUniqueCols(ValueIdSet &uniqueCols) const
{
  uniqueCols.clear();

  for (ValueId v = constraints_.init();
       constraints_.next(v);
       constraints_.advance(v))
  {
    if (v.getItemExpr()->getOperatorType() ==ITM_UNIQUE_OPT_CONSTRAINT)
    {
       UniqueOptConstraint *uc = (UniqueOptConstraint *) v.getItemExpr();
       uniqueCols = uc->uniqueCols();
       return TRUE; // could go on to try to find an even better set
                    // of columns, but taking the first one might be ok
                    // for now (finding a "better" set would probably involve
                    // fairly complex logic)
    }
    else if (v.getItemExpr()->getOperatorType() == ITM_CARD_CONSTRAINT)
    {
      CardConstraint *cc = (CardConstraint *) v.getItemExpr();
      // If at most one row can be returned we don't need a unique key,
      // return TRUE and an empty set
      if (cc->getUpperBound() <= 1)
        return TRUE;
    }
  }
  return FALSE; // no unique columns found
}

NABoolean GroupAttributes::hasCardConstraint(Cardinality &minNumOfRows,
                                             Cardinality &maxNumOfRows) const
{
  NABoolean found = FALSE;

  minNumOfRows = (Cardinality)0;
  maxNumOfRows = (Cardinality)INFINITE_CARDINALITY;

  for (ValueId x= constraints_.init();
       constraints_.next(x);
       constraints_.advance(x) )
    {
      if (x.getItemExpr()->getOperatorType() == ITM_CARD_CONSTRAINT)
        {
          CardConstraint *c = (CardConstraint *) x.getItemExpr();

          found = TRUE;

          // return the highest lower bound and the lowest upper bound
          if (minNumOfRows < c->getLowerBound())
            minNumOfRows = c->getLowerBound();
          if (maxNumOfRows > c->getUpperBound())
            maxNumOfRows = c->getUpperBound();
        }
    }

  return found;
}

// LCOV_EXCL_START
NABoolean GroupAttributes::hasConstraintOfType(OperatorTypeEnum constraintType) const
{
  for (ValueId x= constraints_.init();
       constraints_.next(x);
       constraints_.advance(x) )
    {
      if (x.getItemExpr()->getOperatorType() == constraintType)
	  return TRUE;
    }

  return FALSE;
}

void GroupAttributes::getConstraintsOfType(OperatorTypeEnum constraintType, 
					   ValueIdSet& vidSet) const 
{
  for (ValueId x= constraints_.init();
       constraints_.next(x);
       constraints_.advance(x) )
    {
      if (x.getItemExpr()->getOperatorType() == constraintType)
	  vidSet.insert(x);
    }
}
// LCOV_EXCL_STOP

// This method is used flow RefOpt constraints (optimizer version of foreign key
// side of a RI constraint) up the query tree. The input argument is typically
// the set of constraints from a child's GA. This method picks out the refOpt
// constraints from that set, and adds each one of them to the list of constraints
// of this GA, if the outputs of this GA include the foreign key columns. If
// foreign key columns are not included in the output then they cannot be used
// to match a foreignkey-uniquekey predicate at some join further up the tree.
// Also once an refOptconstraint has found its match, it is not flowed up anymore.
void GroupAttributes::addSuitableRefOptConstraints(const ValueIdSet& vidSet)
{
  for (ValueId x= vidSet.init();
       vidSet.next(x);
       vidSet.advance(x) )
    {
      if (x.getItemExpr()->getOperatorType() == ITM_REF_OPT_CONSTRAINT)
      {
	RefOptConstraint * riConstraint = (RefOptConstraint *)x.getItemExpr();
	ValueIdSet fkCols(riConstraint->foreignKeyCols());
	if ((NOT riConstraint->isMatched()) &&
	    getCharacteristicOutputs().contains(fkCols))
	{
	    addConstraint(x);
	    setHasRefOptConstraint(TRUE);
	}
      }
    }
}

// This method is used flow CompRefOpt constraints (optimizer version of 
// unique key side of a RI constraint) up the query tree. The input argument
// is typically the set of constraints from a child's GA. This method picks 
// out the compRefOpt constraints from that set, and adds each one of them 
// to the list of constraints of this GA, if 
// (a) the outputs of this GA include the unique key columns, and 
// (b) the essential outputs of this GA do not contain any valueid 
//     that is not a unique key column.
// (c) the predicates of this node do not include any references to
//     the key columns
// Item (b) is a necessary condition to check that rows are not 
// being filtered due to predicates on non-key columns.
// Item (c) checks that rows are not being filtered by predicates
// on key columns.
// vidSet : IN : is typically the set of constraints of the calling
//               node's child GA.
// preds : IN : is the calling nodes selection predicates. May need 
//              to be expanded to include join preds if CompRefOpt 
//              constraints need to flow through outer and semi joins
// node  : IN/OUT : is typically the calling node. It is used to 
//                  set the referencedTable ptr in the compRefOpt
//                  constraint. This ptr points to the highest node
//                  in the query tree that is unique in the unique key
//                  cols. This node along with all its children can
//                  be eliminated. Since a join of the unique key table
//                  with another relation with one row can also be unique
//                  in the unique key columns and this is a case where we 
//                  don't want to eliminate this join (may need outputs
//                  from the other relation), the referencedTable ptr
//                  cannot flow past a join node.
void GroupAttributes::addSuitableCompRefOptConstraints(
					const ValueIdSet& vidSet, 
					const ValueIdSet& preds,
					const RelExpr* node)
{
  for (ValueId x= vidSet.init();
       vidSet.next(x);
       vidSet.advance(x) )
    {
      if (x.getItemExpr()->getOperatorType() == ITM_COMP_REF_OPT_CONSTRAINT)
      {
	ComplementaryRefOptConstraint * compRIConstraint = 
	  (ComplementaryRefOptConstraint *)x.getItemExpr();
	const ValueIdList& uniqueCols = compRIConstraint->uniqueKeyCols();
	ValueIdSet uniqueColsSet(uniqueCols);
	const TableDesc * tabId = compRIConstraint->getTableDesc();
	const ValueIdSet uniqueTableCols(tabId->getColumnVEGList());
	if (getCharacteristicOutputs().contains(uniqueColsSet) && 
	    (NOT preds.referencesOneValueFromTheSet(uniqueTableCols)))
	{
	  if (isUnique(uniqueCols) && 
	      (NOT node->getOperator().match(REL_ANY_JOIN)))
	  {
	    compRIConstraint->setReferencedTable((RelExpr *)node);
	  }
	  addConstraint(x);
	  setHasCompRefOptConstraint(TRUE);
	} //compRI constraint that matches output    
      } // do nothing if its not a ComplementaryRefOptConstraint
    }  // loop over constraints
}

// a const method for validating eliminated columns in a sort order,
// usually called for validating an earlier result created with
// the next method below, tryToEliminateOrderColumnBasedOnEqualsPred()
NABoolean GroupAttributes::canEliminateOrderColumnBasedOnEqualsPred(
     ValueId col) const
{
  // cast away const-ness and call the non-const method without
  // predicates, which will mean it won't side-effect "this"
  return const_cast<GroupAttributes *>(this)->
    tryToEliminateOrderColumnBasedOnEqualsPred(col, NULL);
}

// This and the previous method are used to match required sort orders
// or arrangements to an actual key ordering of a table, in cases
// where some columns are equated to a constant, like this, with the
// clustering key being (a,b,c)
//
// select ...
// from t
// where a = 5
// order by b
//
// Note that for VEGs, this is handled differently (see
// ValueIdList::satisfiesReqdOrder), this code is only for non-VEG
// cases (usually computed columns, also varchar).
//
// If we eliminate a column based on a supplied predicate, then
// this predicate is added as an Optimizer check constraint to the
// group attributes, so that future calls will continue to accept
// the simplified sort order.
NABoolean GroupAttributes::tryToEliminateOrderColumnBasedOnEqualsPred(
     ValueId col,
     const ValueIdSet *preds)
{
  NABoolean result = FALSE;

  if (preds || hasConstraintOfType(ITM_CHECK_OPT_CONSTRAINT))
    {
      // Comparison failed. If the caller provided predicates,
      // then we can try something similar to what we did with
      // group attributes above. If the predicate equate the
      // column with a constant, then we can eliminate it as
      // well. Note that we don't expect the requirements to
      // omit such columns, since the parent will usually not
      // know about such predicates, so we check this condition
      // only after trying the regular method.
      //
      // This situation typically happens with computed columns
      // where predicates are added after VEGs are formed

      ValueIdSet checkConstraints;
      ValueIdSet checkPreds;

      // look for predicates we remembered from earlier calls
      getConstraintsOfType(ITM_CHECK_OPT_CONSTRAINT,
                           checkConstraints);

      for (ValueId c = checkConstraints.init();
           checkConstraints.next(c);
           checkConstraints.advance(c))
        checkPreds += static_cast<CheckOptConstraint *>(
             c.getItemExpr())->getCheckPreds();

      // also use newly provided predicates
      if (preds)
        checkPreds += *preds;

      // if the column is descending, then get rid of the Inverse
      // operator for the next check
      if (col.getItemExpr()->getOperatorType() == ITM_INVERSE)
        col = col.getItemExpr()->child(0).getValueId();

      // Convert col from a VEGRef to a base column, if needed,
      // the ScanKey method below wants a real column as input.
      // Make sure to pick the base column that is actually
      // referenced in the check predicates, in case there are
      // multiple base columns in the VEG.
      if (col.getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE)
        {
          const ValueIdSet &vegMembers =
            static_cast<VEGReference *>(col.getItemExpr())->
            getVEG()->getAllValues();
          ValueId dummy;

          for (ValueId b=vegMembers.init();
               vegMembers.next(b);
               vegMembers.advance(b))
            if (checkPreds.referencesTheGivenValue(b, dummy))
              {
                // Use this column for comparison. Note that
                // we can have a situation with a VEG(T1.a, T2.b)
                // and a check predicate T1.a = const. The (computed)
                // check predicate got added later, so "const" is
                // not a VEG member. This should not cause trouble,
                // however, since an operator must ensure that
                // a) the VEG members it produces are equal (it needs to
                //    make sure the comparison pred is evaluated), and
                // b) that the predicate applies.
                // So, we cannot have the situation where T1.a=const
                // and T2.b != const in the output of this operator.
                col = b;
                break;
              }
        }

      for (ValueId p = checkPreds.init();
           checkPreds.next(p);
           checkPreds.advance(p))
        {
          ValueId dummy1, dummy2;

          if (p.getItemExpr()->getOperatorType() == ITM_EQUAL &&
              ScanKey::isAKeyPredicateForColumn(
                   p,
                   dummy1, dummy2,
                   col,
                   getCharacteristicInputs()))
            {
              // this is a predicate of the form col = const
              // and col is our current ValueId in tempThis,
              // therefore skip over it and try again
              result = TRUE;

              // if we used a newly provided predicate, then
              // remember it in the constraints, so that when
              // the search engine validates our physical
              // property later, it will come to the same
              // conclusion
              if (preds && preds->contains(p))
                addConstraint(
                     new(CmpCommon::statementHeap()) CheckOptConstraint(
                          ValueIdSet(p)));
            }
        }
    } // predicates or check constraints are supplied

  return result;
}


// -----------------------------------------------------------------------
// Low-level utility for merging Group Attributes.
// -----------------------------------------------------------------------
void GroupAttributes::lomerge (GroupAttributes &other, NABoolean mergeCIO)
{
  // QSTUFF
  CMPASSERT(isStream() == other.isStream());
  CMPASSERT(isSkipInitialScan() == other.isSkipInitialScan());
  CMPASSERT(isEmbeddedUpdateOrDelete() ==
                         other.isEmbeddedUpdateOrDelete());
  CMPASSERT(isGenericUpdateRoot() ==
                         other.isGenericUpdateRoot());
  // QSTUFF

  if (mergeCIO)
    {
      requiredInputs_ += other.requiredInputs_ ;
      requiredOutputs_ += other.requiredOutputs_;
    }
  else
    {
      if (NOT (requiredInputs_ == other.requiredInputs_) OR
               NOT (requiredOutputs_ == other.requiredOutputs_))
        ABORT("Internal error, merging incompatible group attributes"); // LCOV_EXCL_LINE
    }

  // To add the constraints from the other group to this one, we
  // must use the addConstraint(Itemexpr*) method.  This is because
  // we need to check for duplicate constraints, and only the
  // addConstraint method does that. If we are merging two scan
  // groups, for example, it is possible that each group will have
  // allocated two physically different constraints that represent the
  // same logical constraint on the same columns.
  for (ValueId x= other.constraints_.init();
       other.constraints_.next(x);
       other.constraints_.advance(x) )
  {
    addConstraint(x.getItemExpr());
  }

  availableBtreeIndexes_.insert(other.availableBtreeIndexes_);

  numBaseTables_ = MAXOF(numBaseTables_,other.numBaseTables_);
  minChildEstRowCount_ = MINOF(minChildEstRowCount_,other.minChildEstRowCount_);

  numJoinedTables_ = MAXOF(numJoinedTables_,other.numJoinedTables_);
  numTMUDFs_ = MAXOF(numTMUDFs_, other.numTMUDFs_);

  // Look thru the sets of input/output est. logical properties for
  // the "other" set of group attributes.  For those not already in
  // the current set, add them.  If there exists output est. logical
  // properties for the same set of input est. logical properties
  // between the two group attributes, there is no attempt to normalize
  // them.  Just keep the one from "this" group.
  for (CollIndex i = 0; i < other.getInputLogPropList().entries(); i++)
    {
      Int32 index = existsInputLogProp (other.getInputLogPropList()[i]);
      if (index < 0) // this ILP does not already exist
      {
        // ensure that there is a corresponding entry in the
        // intermediate as well as final output LP list.
        if ((i >= other.getOutputLogPropList().entries()) ||
           (i >= other.getIntermedOutputLogPropList().entries()) )
        {
          CCMPASSERT (i < other.getIntermedOutputLogPropList().entries());
          CCMPASSERT (i < other.getOutputLogPropList().entries());
          break;
        }

        // Insert the corresponding input and output est. log. properties
        // into the group attributes.
        inputLogPropList().insert (other.getInputLogPropList()[i]);
        outputLogPropList().insert (other.getOutputLogPropList()[i]);
        intermedOutputLogPropList().insert (other.getIntermedOutputLogPropList()[i]);
      }
    }

  // Merge GroupAnalysis if available
  // Use groupAnalysis of other if I dont have any
  // (Should not really happen, but wont hurt, consider assertion)
  if (other.getGroupAnalysis())
  {
    if (!groupAnalysis_)
    {
      groupAnalysis_ = new(CmpCommon::statementHeap())
        GroupAnalysis(*(other.getGroupAnalysis()));
    }
    else
    {
      //reconcile my GroupAnalysis with the other's GroupAnalysis
      groupAnalysis_->reconcile(other.getGroupAnalysis());
    }
  }

  DCMPASSERT(hasNonDeterministicUDRs_ == other.hasNonDeterministicUDRs_);
  if (other.hasNonDeterministicUDRs_)
    hasNonDeterministicUDRs_ = TRUE;

    if((other.potential_ >= 0 ) &&
       (other.potential_ < potential_))
       potential_ = other.potential_;

} // GroupAttributes::lomerge()

// -----------------------------------------------------------------------
// Comparison (compare required inputs/outputs)
// -----------------------------------------------------------------------
NABoolean GroupAttributes::operator == (const GroupAttributes &other) const
{
  return (requiredInputs_  == other.requiredInputs_  AND
          requiredOutputs_ == other.requiredOutputs_);

  // compare constraints and/or EstLogProp????
}

// -----------------------------------------------------------------------
// hash function
// -----------------------------------------------------------------------
HashValue GroupAttributes::hash() const
{
  HashValue result = 0x0;

  result ^= requiredInputs_;
  result ^= requiredOutputs_;

  // constraints may be added dynamically, therefore they can't be part of
  // the hash function.

  // The estimated log props and the cache are not invariant and therefore
  // can't be part of the hash function.

  return result;
}

// -----------------------------------------------------------------------
// Recommended order for NJ probing.
// Find a good forward probing order for this group. A preferred probing
// order is chosen as the key prefix of any available Btree index
// that is covered by either constants or params/host vars
// and at least one equijoin column, has the minimum number of
// uncovered columns, and satisfys all partitioning and ordering
// requirements.
// Inputs: child0 Group Attributes
//         Number of forced ESPs (-1 if no forcing)
//         Requirement Generator object containing left child requirements
//         Any required order or arrangement for the right child
// Outputs: chosenIndexDesc
//          Indicator if all part key cols are equijoin cols or
//          covered by a partitioning requirement that has no part key cols
// Returns: A list that is a prefix of the chosen index sort key.
// -----------------------------------------------------------------------
#pragma nowarn(770)   // warning elimination
ValueIdList GroupAttributes::recommendedOrderForNJProbing(
                               GroupAttributes* child0GA, // IN
                               Lng32 numForcedParts, //IN
                               RequirementGenerator& rg, // IN
                               ValueIdList& reqdOrder1, // IN
                               ValueIdSet& reqdArr1, // IN
                               IndexDesc* &chosenIndexDesc, // OUT
                               NABoolean partKeyColsAreMappable // OUT
                              )
{
  ValueIdList chosenIndexOrder;
  Lng32 chosenIndexNumUncoveredCols = -1;
  NABoolean chosenIndexPartKeyColsAreMappable = FALSE;
  NABoolean chosenIndexSatisfiesDp2SortOrderPartReq = FALSE;
  Lng32 chosenIndexNumParts = 0;

  ValueIdList currentIndexOrder;
  IndexDesc* currentIndexDesc;
  const ValueIdList* currentIndexSortKey;
  const PartitioningFunction* currentIndexPartFunc;
  const ValueIdSet* currentIndexPartKey = NULL;
  ValueIdList currentIndexPartKeyAsList;
  ValueIdList currentIndexPartKeyEquiJoinCols;
  NABoolean currentIndexPartKeyColsAreMappable;
  Lng32 currentIndexNumParts;
  ValueIdList currentIndexUncoveredCols;
  Lng32 currentIndexNumUncoveredCols;
  NABoolean currentIndexSatisfiesPartReq;
  NABoolean currentIndexSatisfiesDp2SortOrderPartReq;

  const ReqdPhysicalProperty* rppForMe =
    rg.getStartRequirements();
  PartitioningRequirement* partReqForMe =
    rppForMe->getPartitioningRequirement();
  SortOrderTypeEnum sortOrderTypeReq =
    rppForMe->getSortOrderTypeReq();
  PartitioningRequirement* dp2SortOrderPartReq =
    rppForMe->getDp2SortOrderPartReq();

  Lng32 numPartsRequirement = ANY_NUMBER_OF_PARTITIONS;
  NABoolean numPartsForced = FALSE;

  if (numForcedParts != ANY_NUMBER_OF_PARTITIONS)
  {
    numPartsRequirement = numForcedParts;
    numPartsForced = TRUE;
  }
  else if (partReqForMe != NULL)
  {
    numPartsRequirement = partReqForMe->getCountOfPartitions();
  }

  chosenIndexDesc = NULL;

  for (CollIndex i = 0; i < availableBtreeIndexes_.entries(); i++)
  {
    currentIndexDesc = availableBtreeIndexes_[i];
    currentIndexSortKey = (&(currentIndexDesc->getOrderOfKeyValues()));
    currentIndexPartFunc = currentIndexDesc->getPartitioningFunction();

    if (currentIndexPartFunc == NULL)
    {
      currentIndexPartFunc = new(CmpCommon::statementHeap())
        SinglePartitionPartitioningFunction();
      currentIndexNumParts = 1;
      currentIndexPartKeyColsAreMappable = TRUE;
    }
    else
    {
      currentIndexNumParts =
         currentIndexPartFunc->getCountOfPartitions();
      currentIndexPartKey =
         (&(currentIndexPartFunc->getPartitioningKey()));
      if (currentIndexPartKey->isEmpty())
        currentIndexPartKeyColsAreMappable = TRUE;
      else
      {
        currentIndexPartKeyAsList = *currentIndexPartKey;
        currentIndexPartKeyEquiJoinCols =
           currentIndexPartKeyAsList.findNJEquiJoinCols(
                       child0GA->getCharacteristicOutputs(),
                       getCharacteristicInputs(),
                       currentIndexUncoveredCols);
        if (currentIndexPartKeyEquiJoinCols.entries() ==
            currentIndexPartKey->entries())
          currentIndexPartKeyColsAreMappable = TRUE;
        else
          currentIndexPartKeyColsAreMappable = FALSE;
      }
    }

    // If already have an index whose part key columns are mappable,
    // don't pick one which does not.
    if (chosenIndexPartKeyColsAreMappable AND
        NOT currentIndexPartKeyColsAreMappable)
      continue;

    currentIndexOrder = currentIndexSortKey->findNJEquiJoinCols(
                          child0GA->getCharacteristicOutputs(),
                          getCharacteristicInputs(),
                          currentIndexUncoveredCols);

#pragma nowarn(1506)   // warning elimination
    currentIndexNumUncoveredCols = currentIndexUncoveredCols.entries();
#pragma warn(1506)  // warning elimination

    // The current index must have at least one equijoincolumn to use it.
    if (currentIndexOrder.entries() == 0)
      continue;


    // Check the current index partitioning against the
    // partitioning requirements.

    // Assume that the index partitioning satisfies any dp2SortOrderPartReq.
    // If there is no dp2SortOrderPartReq then all indices satisfy it
    // trivially and so this will always be TRUE.
    currentIndexSatisfiesDp2SortOrderPartReq = TRUE;

    // If there is a dp2SortOrderPartReq, and the sortOrderTypeReq
    // is not DP2_OR_ESP_NO_SORT, then the physical partitioning function
    // of the index must match the requirement exactly or we can't
    // use the index. Note that if the sortOrderTypeReq is
    // DP2_OR_ESP_NO_SORT, we could always downgrade the sortOrderTypeReq
    // to ESP_NO_SORT and get rid of the dp2SortOrderPartReq. In this
    // case, we will still consider the index, but will still favor one
    // which does satisfy the dpSortOrderPartReq.
    if ((dp2SortOrderPartReq != NULL) AND
        (NOT dp2SortOrderPartReq->
               partReqAndFuncCompatible(currentIndexPartFunc)))
    {
      currentIndexSatisfiesDp2SortOrderPartReq = FALSE;
      if (sortOrderTypeReq != DP2_OR_ESP_NO_SORT_SOT)
        continue;
    }

    // If already have an index which satisfies any Dp2SortOrderPartReq,
    // don't pick one which does not. Note that if there really is
    // a Dp2SortOrderPartReq than any index which satisfies it must have
    // mappable part keys, so this code will never throw away an index
    // with mappable part keys in favor of one which doesn't.
    if (chosenIndexSatisfiesDp2SortOrderPartReq AND
        NOT currentIndexSatisfiesDp2SortOrderPartReq)
      continue;

    // Assume the index partitioning will not match.
    currentIndexSatisfiesPartReq = FALSE;

    // The index partitioning can always satisfy a replicate
    // no broadcast or exactly one partitioning requirement,
    // unless we are in DP2, in which case the index must have
    // only one partition. We also don't have to worry about being
    // able to map the partitioning keys in this case because the
    // grouped index partitioning function will not have any.
    if ((partReqForMe != NULL) AND
        (partReqForMe->isRequirementReplicateNoBroadcast() OR
         (partReqForMe->isRequirementExactlyOne() AND
          (NOT rppForMe->executeInDP2() OR
           (currentIndexNumParts == 1)))))
    {
      currentIndexSatisfiesPartReq = TRUE;
    }
    // otherwise, can only use index if it's part key cols are mappable
    else if (currentIndexPartKeyColsAreMappable)
    {
      NABoolean numPartsSatisfiesFuzzyReq =
       ((partReqForMe != NULL) AND
        (partReqForMe->castToRequireApproximatelyNPartitions() != NULL) AND
        partReqForMe->castToRequireApproximatelyNPartitions()->
          isPartitionCountWithinRange(currentIndexNumParts));

      // Check the number of partitions against the required # of parts
      // The current index # of parts satisfies the required # of parts
      // if there is no # of parts requirement, or they match exactly,
      // or we are not in DP2 and so we can use grouping to satisfy
      // the requirement, or the requirement is fuzzy, the # of parts
      // falls in the allowable range, and the # of parts is not forced.
      if ((numPartsRequirement == ANY_NUMBER_OF_PARTITIONS) OR
          (numPartsRequirement == currentIndexNumParts) OR
          (NOT rppForMe->executeInDP2() AND
           (numPartsRequirement < currentIndexNumParts)) OR
          (numPartsSatisfiesFuzzyReq AND NOT numPartsForced))
      {
        // If there was no partitioning requirement or the current
        // index is not partitioned, then the index qualifies - we
        // don't need to check any partitioning keys or part. boundaries.
        // We don't need to check part keys if the index is not partitioned
        // because the requirement must have been for exactly one partition
        // or we wouldn't have gotten here - and a requirement for
        // exactly one partition has no part key requirements.
        if ((partReqForMe == NULL) OR
            (currentIndexNumParts == 1))
          currentIndexSatisfiesPartReq = TRUE;
        else
        {
          // if fuzzy requirement, need to compare index partitioning
          // keys against the partitioning requirements
          if (partReqForMe->isRequirementFuzzy())
          {
            if (partReqForMe->getPartitioningKey().isEmpty() OR
                partReqForMe->getPartitioningKey().contains(
                  *currentIndexPartKey))
              currentIndexSatisfiesPartReq = TRUE;
          }
          else
          {
            // fully specified partitioning requirement - must compare
            // both partitioning keys and the partitioning boundaries
            PartitioningFunction* scaledPartFunc =
              currentIndexPartFunc->copy();
            Lng32 scaleNumPartReq = numPartsRequirement;
            scaledPartFunc =
              scaledPartFunc->scaleNumberOfPartitions(scaleNumPartReq);
            if ((scaleNumPartReq == numPartsRequirement) AND
                partReqForMe->partReqAndFuncCompatible(scaledPartFunc))
              currentIndexSatisfiesPartReq = TRUE;
          } // end if fuzzy or fully specified req
        } // end else part req exists and current index is partitioned
      } // end if current index satisfies any number of partitions req
    } // end if current index part key cols are mappable

    if (NOT currentIndexSatisfiesPartReq)
      continue;


    // Check the current index sort order against any sort order requirements

    // Check the equijoin cols portion of the sort key
    rg.makeSortKeyFeasible(currentIndexOrder);

    // If none of the current index equijoin columns were compatible
    // with any required order or arrangement then we can't use it.
    if (currentIndexOrder.entries() == 0)
      continue;

    // Check the uncovered columns portion of the sort key
    // against any split off required order or arrangement for
    // the right child. We don't need to check the equijoin
    // columns because they will be covered by the probes.
    if (NOT reqdArr1.isEmpty() AND
        NOT currentIndexUncoveredCols.satisfiesReqdArrangement(reqdArr1,this))
      continue;
    if (NOT reqdOrder1.isEmpty() AND
        (currentIndexUncoveredCols.satisfiesReqdOrder(reqdOrder1,this) !=
           SAME_ORDER))
      continue;

    // If this is the first index, then it is automatically the
    // best index so far.
    //   We always want to pick an index whose partitioning key columns
    // are all equijoin columns, i.e. they are mappable, over one
    // whose part key cols are not mappable.
    //   We always want to pick an index who satifies any
    // dp2SortOrderPartReq over one which does not. Note that
    // any index which satisfies a dp2SortOrderPartReq must have
    // mappable part keys so this does not violate the previous rule.
    //
    // Otherwise:
    //
    //   We want to pick the index with the least number of uncovered
    // columns, i.e. the index which is the closest to being
    // fully specified. If two indexes have the same number of uncovered
    // columns, we pick the index which has the most partitions
    // (to get the maximum level of parallelism). In the case of a tie,
    // we pick the primary index.
    if ( (chosenIndexNumUncoveredCols == -1) // 1st index checked?
        OR
         (NOT chosenIndexPartKeyColsAreMappable AND // always favor index
          currentIndexPartKeyColsAreMappable)     // with mappable part key
        OR
         (NOT chosenIndexSatisfiesDp2SortOrderPartReq AND // favor if satis.
          currentIndexSatisfiesDp2SortOrderPartReq)     // dp2SortOrderPartReq
        OR
         (currentIndexNumUncoveredCols < chosenIndexNumUncoveredCols)
        OR
         ((currentIndexNumUncoveredCols == chosenIndexNumUncoveredCols) AND
          (currentIndexNumParts > chosenIndexNumParts))
        OR
         ((currentIndexNumUncoveredCols == chosenIndexNumUncoveredCols) AND
          (currentIndexNumParts == chosenIndexNumParts) AND
          currentIndexDesc->isClusteringIndex()))
    {
      chosenIndexOrder = currentIndexOrder;
      chosenIndexNumUncoveredCols = currentIndexNumUncoveredCols;
      chosenIndexDesc = currentIndexDesc;
      chosenIndexPartKeyColsAreMappable =
        currentIndexPartKeyColsAreMappable;
      chosenIndexSatisfiesDp2SortOrderPartReq =
        currentIndexSatisfiesDp2SortOrderPartReq;
      chosenIndexNumParts = currentIndexNumParts;

    }
  } // end for all indexes in the group

  partKeyColsAreMappable = chosenIndexPartKeyColsAreMappable;

  // HEURISTIC: Only pass back the recommended order if the total
  // # of blocks for this access path is greater than the cache size.
  // The costing code treats an in-order scan and an unordered scan
  // the same if the blocks all fit in cache so this should be a good
  // heuristic. We would really like the # of blocks after the key
  // preds have been supplied, but this would be expensive to compute.
  // Instead we over-estimate to avoid not trying an ordered plan
  // when it could be beneficial.

  if (chosenIndexDesc != NULL)
  {
    // Get the blocksize for this access path
    CostScalar blockSizeInKb = chosenIndexDesc->getBlockSizeInKb();

    // Get the colStatsDescList containing the histogram data for
    // all columns of the table from the primary table descriptor.
    const ColStatDescList& csdl =
      chosenIndexDesc->getPrimaryTableDesc()->getTableColStats();

    // Could get the rowcount from any column stats, so just get it
    // from the first column.
    CostScalar numRows = csdl[0]->getColStats()->getRowcount();

    // Determine how many blocks are in a partition of this access path
    CostScalar rowsPerBlock =
      (blockSizeInKb / chosenIndexDesc->getRecordSizeInKb()).getFloor();
    CostScalar numBlocks = (numRows / rowsPerBlock).getCeiling();
    CostScalar numBlocksPerPartition =
      (numBlocks / chosenIndexNumParts).getCeiling();

    // Determine how many blocks are available in the cache for blocks
    // of the same size as this index. Then determine how many users
    // there are and then how much of the cache this user can expect
    // to have available.
    CostScalar cacheSizeInBlocks(getDP2CacheSizeInBlocks(blockSizeInKb));

    NADefaults &defs = ActiveSchemaDB()->getDefaults();
    const CostScalar concurrentUsers = defs.getAsLong(NUMBER_OF_USERS);
    CostScalar cacheForThisUser = cacheSizeInBlocks / concurrentUsers;

    // # of blocks for one partition should exceed the cache size for
    // for this to be useful
/*
    if (numBlocksPerPartition < cacheForThisUser)
    {
      chosenIndexOrder.clear();
      chosenIndexDesc = NULL;
    }
*/
  }

  return chosenIndexOrder;

} // GroupAttributes::recommendedOrderForNJProbing()
#pragma warn(770)  // warning elimination

// -----------------------------------------------------------------------
// coverTest()
//
// A method to determine whether a given set of expressions are
// covered, i.e., they can be satisfied, by the ValueIds that appear
// a) in the GroupAttributes of a relational operator or
// b) a set of new input values that the caller is willing to provide.
//
// This method is called by an operator for a specific child, i.e.,
//         J -> predicates     the join operator can invoke
//        / \                  GroupAttr::coverTest() for either
// scan T1   scan T2           one or both of the scans that are its
//                             children
// Parameters:
//
// ValueIdSet setOfExprOnParent
//    IN:  a read-only reference to a set of expressions that
//         are associated with the parent.
//
// ValueIdSet newExternalInputs
//    IN : a read-only reference to a set of new external inputs
//         (ValueIds) that are provided for evaluating the above
//         expressions.
//
// ValueIdSet coveredExpr
//    OUT: a subset of setOfExprOnParent. It contains the ValueIds
//         of only those expressions that are covered.
//
// ValueIdSet referencedInputs
//    OUT: a subset of newExternalInputs. It contains the
//         ValueIds of only those inputs that are referenced in
//         the expressions that belong to coveredExpr.
//
// ValueIdSet *coveredSubExpr
//    OUT: It contains the ValueIds of all those sub-expressions
//         that are covered in the set (setOfExprOnParent-coveredExpr)
//         However, the expression that contain them are not covered.
//
// ValueIdSet *unCoveredExpr
//    OUT: If non-null, unCoveredExpr contains the value ids of all
//         those expressions and/or subexpressions that could not
//         be covered by the group attributes and the new inputs. In
//         other words, unCoveredExpr contains the minimum set of
//         additional value ids needed to cover all of the expressions
//         in "setOfExprOnParent".
//
// -----------------------------------------------------------------------
void GroupAttributes::coverTest(const ValueIdSet& setOfExprOnParent,
                                const ValueIdSet& newExternalInputs,
                                ValueIdSet& coveredExpr,
                                ValueIdSet& referencedInputs,
                                ValueIdSet* coveredSubExpr,
                                ValueIdSet* unCoveredExpr) const
{

  for (ValueId exprId = setOfExprOnParent.init();
       setOfExprOnParent.next(exprId); setOfExprOnParent.advance(exprId) )
    {
      if ( covers(exprId, newExternalInputs,
                  referencedInputs, coveredSubExpr, unCoveredExpr) )
        coveredExpr += exprId;
    } // loop over expressions

} // GroupAttributes::coverTest()

// -----------------------------------------------------------------------
// covers()
//
// A method to determine whether a given expression is covered,
// i.e., its can be satisfied, by the ValueIds that appear
// a) in the GroupAttributes of a relational operator or
// b) a set of new input values that the caller is willing to provide.
//
// Parameters:
//
// ValueId  exprId
//    IN : a read-only reference to the ValueId of an expression
//         that is to be checked for coverage
//
// ValueIdSet newExternalInputs
//    IN : a read-only reference to a set of new external inputs
//         (ValueIds) that are provided for evaluating the above
//         expressions.
//
// GroupAttributes coveringGA
//    IN : the Group Attributes that are to provide coverage
//
// ValueIdSet referencedInputs
//    OUT: a subset of newExternalInputs. It contains the
//         ValueIds of only those inputs that are referenced in
//         the expression exprId.
//
// ValueIdSet *coveredSubExpr
//    OUT: It contains the ValueIds of all those sub-expressions
//         of exprId that are covered, while exprId itself may
//         not be covered.
//
// ValueIdSet *unCoveredExpr
//    OUT: If non-null, unCoveredExpr contains the value ids of all
//         those expressions and/or subexpressions that could not
//         be covered by the group attributes and the new inputs. In
//         other words, unCoveredExpr contains the minimum set of
//         additional value ids needed to cover all of the expressions
//         in "exprId".
//
// Returns TRUE : If ValueId is covered
//         FALSE: Otherwise.
//
// -----------------------------------------------------------------------
NABoolean GroupAttributes::covers(const ValueId& exprId,
                                  const ValueIdSet& newExternalInputs,
                                  ValueIdSet& referencedInputs,
                                  ValueIdSet* coveredSubExpr,
                                  ValueIdSet* unCoveredExpr) const
{
  NABoolean  coverFlag = TRUE; // assume expr is covered

  // If the given expression belongs to the Characteristic Inputs
  // or the Characteristic Outputs
  if ( isCharacteristicInput(exprId) OR
       isCharacteristicOutput(exprId) )
    {
      // do nothing
    }
  // If the given expression belongs to the new external inputs,
  // update referencedInputs to remember this reference.
  else if (newExternalInputs.contains(exprId))
    referencedInputs += exprId;
  // Otherwise, walk through the item expression to check whether
  // it is covered by the given Group Attributes or newExternalInputs
  else
    {
      ValueIdSet localInputs, localSubExpr, localUnCoveredExpr;

      // Walk through the item expression tree to check for coverage
      ItemExpr *iePtr = exprId.getItemExpr();
      coverFlag = iePtr->isCovered(newExternalInputs, *this,
                                   localInputs, localSubExpr,
                                   localUnCoveredExpr);
      if (coverFlag)
        {
          referencedInputs += localInputs;
        }
      else if (NOT localSubExpr.isEmpty()) // the expression is not covered
        {                                  // but one of its subexpressions is
          referencedInputs += localInputs;
          if (coveredSubExpr != NULL)
            *coveredSubExpr += localSubExpr;
          if (unCoveredExpr != NULL)
            *unCoveredExpr += localUnCoveredExpr;
        }
      else
        {
          // this item expression and all its children are uncovered,
          // so only return this one as uncovered
          if (unCoveredExpr != NULL)
            *unCoveredExpr += exprId;
        }
    } // examine item expression

  return coverFlag;

} // GroupAttributes::covers()

// -----------------------------------------------------------------------
// computeCharacteristicIO()
//
// A method to recompute the Characteristic Inputs and Outputs.
//
// This method is called by an operator for a specific child, i.e.,
//         J -> predicates     the join operator can invoke
//        / \                  GroupAttr::computeCharacteristicIO()
// scan T1   scan T2           for either one or both of the scans
//                             that are its children
//
// Effect :
// The Group Attributes for the child can get new characteristic
// inputs and outputs.
// -----------------------------------------------------------------------
void GroupAttributes::computeCharacteristicIO(const ValueIdSet& newInputs,
                                              const ValueIdSet& exprOnParent,
					      const ValueIdSet& outputExprOnParent,
					      const ValueIdSet& essentialChildOutputs,
					      const ValueIdSet * selPred,
					      const NABoolean childOfALeftJoin,
					      const NABoolean optimizeOutputs,
                                      const ValueIdSet *extraHubEssOutputs)
{
  ValueIdSet coveredSubExpr;   // to contain ALL ValueIds that are covered
                               // by this GA.
  ValueIdSet referencedInputs; // to return the set of Inputs that are
                               // referenced
  ValueId exprId;              // a potential member of the Char. Out.

  // Compute the available inputs
  ValueIdSet availableInputs = requiredInputs_;
  availableInputs += newInputs;
  ValueIdSet colOf2WithConst;
  ValueIdSet essentialKeptOutputs, nonEssentialKeptOutputs;

  //If right child of the left join and selection predicate not empty
  if(childOfALeftJoin && selPred)
  {
   //In case of inner join selection predicates are pushed to its childrens.
   //Hence, no characteristic output need to be output to the parent node.
   //For left joins, selection predicate on the right child, if any, should
   //be evaluated at the join node. So, right child should produce those values
   //in the form of characteristic output.Currently, left join is not transformed
   //to inner join if selection predicate contains an 'or' expression. So, we need
   //to have right child produce those values in the form of characteristic output.

    //This call collects required outputs to evaluate the selection predicate.
    colOf2WithConst.accumulateReferencedValues(requiredOutputs_,*selPred);

    //Remove references to constant expressions
    colOf2WithConst.removeConstExprReferences();
  }

   // The set of exprOnParent that are not covered by the available inputs
  ValueIdSet effectiveOutputExprOnParent(outputExprOnParent);
  effectiveOutputExprOnParent.removeCoveredExprs(availableInputs);

  // Look for covered expressions that the child can contribute
  for (exprId = effectiveOutputExprOnParent.init();
       effectiveOutputExprOnParent.next(exprId);
       effectiveOutputExprOnParent.advance(exprId) )
    {
      // Accumulate the ValueIds of all expressions as well as
      // sub-expressions that are covered, in coveredSubExpr
      if ( covers(exprId, newInputs,
                  referencedInputs, &coveredSubExpr) )
        {
          coveredSubExpr += exprId;
        }
    } // loop over effectiveExprOnParent

  // Remove from covered expressions those that are covered by the
  // available inputs
  coveredSubExpr.removeCoveredExprs(availableInputs);
  ValueIdSet availableValues = referencedInputs;
  availableValues += coveredSubExpr;
  ValueIdSet removedValues ;

  // Now we are going to minimize the set of coverSubExpr
  // such that we do not produce a value and an expression that
  // depends on a value. We will also strip off any inverse
  // nodes, because this is not a characteristic a group can
  // produce - a group can only produce a value, not a value
  // with a particular order. Only a sort key can do that.
  //  E.g. a,b,a+b  ==> a,b
  //       a,a + 1 ==> a
  //       a,b,inverse(a),inverse(b) ==> a,b
  //       inverse(a) ==> a

  // test each of the expressions in coveredSubExpr and see if
  // it is covered by the remaining expressions
  for (exprId = coveredSubExpr.init();
       coveredSubExpr.next(exprId);
       coveredSubExpr.advance(exprId) )
    {
      ValueIdSet valueToTest;
      // Remove the current expression from the set of all expressions
      // so we can check for duplicates.
      availableValues -= exprId;
      // Remove any INVERSE nodes on top before checking for coverage.
      ValueId noInverseVid =
         exprId.getItemExpr()->removeInverseOrder()->getValueId();
      // If the expression was simplified, save the simplified form
      // of the expression, otherwise save the original form.
      if (noInverseVid != exprId)
      {
        valueToTest += noInverseVid;
      }
      else
      {
        valueToTest += exprId;
      }

      // If the current expression is equivalent to some other expression,
      // then remove it. For cases when the parent needs all the outputs
      // do not optimize

      if (optimizeOutputs) {
	valueToTest.removeCoveredExprs(availableValues);
      }

      // The set valueToTest is either empty or contains only the
      // simplified (if possible) form of the expression.
      nonEssentialKeptOutputs += valueToTest;
      if (essentialChildOutputs.contains(valueToTest))
	essentialKeptOutputs += valueToTest;

      availableValues += valueToTest;
    }

  // the top half this method computes non-essential outputs
  // --------------------------------------------------------------------------------------
  // the bottom half of this method computes essential outputs


  // The set of exprOnParent that are not covered by the available inputs
  ValueIdSet effectiveExprOnParent(exprOnParent);
  effectiveExprOnParent.removeCoveredExprs(availableInputs);
  coveredSubExpr.clear();

  // Look for covered expressions that the child can contribute
  for (exprId = effectiveExprOnParent.init();
       effectiveExprOnParent.next(exprId);
       effectiveExprOnParent.advance(exprId) )
    {
      // Accumulate the ValueIds of all expressions as well as
      // sub-expressions that are covered, in coveredSubExpr
      if ( covers(exprId, newInputs,
                  referencedInputs, &coveredSubExpr) )
        {
          coveredSubExpr += exprId;
        }
    } // loop over effectiveExprOnParent

  // Remove from covered expressions those that are covered by the
  // available inputs
  coveredSubExpr.removeCoveredExprs(availableInputs);

  // Now we are going to minimize the set of coverSubExpr
  // such that we do not produce a value and an expression that
  // depends on a value. We will also strip off any inverse
  // nodes, because this is not a characteristic a group can
  // produce - a group can only produce a value, not a value
  // with a particular order. Only a sort key can do that.
  //  E.g. a,b,a+b  ==> a,b
  //       a,a + 1 ==> a
  //       a,b,inverse(a),inverse(b) ==> a,b
  //       inverse(a) ==> a
  
  availableValues += referencedInputs;
  availableValues += coveredSubExpr;
  ValueIdSet essentialOutputsThatHaveBeenMarkedNonEssential;

  // test each of the expressions in coveredSubExpr and see if
  // it is covered by the remaining expressions
  for (exprId = coveredSubExpr.init();
       coveredSubExpr.next(exprId);
       coveredSubExpr.advance(exprId) )
  {
    ValueIdSet valueToTest;
    // Remove the current expression from the set of all expressions
    // so we can check for duplicates.
    availableValues -= exprId;
    // Remove any INVERSE nodes on top before checking for coverage.
    ValueId noInverseVid =
        exprId.getItemExpr()->removeInverseOrder()->getValueId();
    // If the expression was simplified, save the simplified form
    // of the expression, otherwise save the original form.
    if (noInverseVid != exprId)
    {
      valueToTest += noInverseVid;
    }
    else
    {
      valueToTest += exprId;
    }

    // If the current expression is equivalent to some other expression,
    // then remove it. For cases when the parent needs all the outputs
    // do not optimize

    if (optimizeOutputs) {
      availableValues += newInputs;
      valueToTest.removeCoveredExprs(availableValues, 
        &essentialOutputsThatHaveBeenMarkedNonEssential);
      availableValues -= newInputs;
      essentialOutputsThatHaveBeenMarkedNonEssential -= newInputs;
    }

    if ((extraHubEssOutputs && !(extraHubEssOutputs->contains(valueToTest))) ||
        NOT extraHubEssOutputs)
      {
        essentialKeptOutputs += valueToTest;
        nonEssentialKeptOutputs -= valueToTest ;
      }
      else
        nonEssentialKeptOutputs += valueToTest ;

    // The set valueToTest is either empty or contains only the
    // simplified (if possible) form of the expression.
    essentialKeptOutputs += essentialOutputsThatHaveBeenMarkedNonEssential;
    nonEssentialKeptOutputs -=  essentialOutputsThatHaveBeenMarkedNonEssential;
    availableValues += valueToTest;
    essentialOutputsThatHaveBeenMarkedNonEssential.clear();
  }

  ValueIdSet keptOutputs;

  if (optimizeOutputs) {
    minimizeOutputs(nonEssentialKeptOutputs, essentialKeptOutputs, keptOutputs);
    }
  else {
    keptOutputs = nonEssentialKeptOutputs;
    keptOutputs += essentialKeptOutputs;
  }


  //Add outputs evaluated earlier for computing selection predicate
  //at parent node.
  if(childOfALeftJoin && (NOT colOf2WithConst.isEmpty())) 
  {
    essentialKeptOutputs += colOf2WithConst;
  }
  // Set the essential outputs
  requiredEssentialOutputs_ = essentialKeptOutputs;
  requiredOutputs_ = keptOutputs;

  // Compute what inputs are needed by the the new outputs
  // (Modifies referencedInputs not requiredOutputs)
  requiredOutputs_.weedOutUnreferenced(referencedInputs);

  requiredEssentialOutputs_.intersectSet(requiredOutputs_);

  requiredInputs_  += referencedInputs;

} // GroupAttributes::computeCharacteristicIO()

  // --------------------------------------------------------------------
  // GroupAttr::minimizeOutputs()
  //
  // This method takes in (a) a vidset holding non-essential outputs, (2)
  // a vidset holding essential outputs, and (3) an empty set that will hold 
  // all the outputs. The method reduces each set such that no vid is covered 
  // by all the other vids. In removing any covered essentianl vids, the 
  // values that contribute to that coverage are promoted to essential outputs
  // --------------------------------------------------------------------
  void GroupAttributes::minimizeOutputs(ValueIdSet& nonEssentialOutputs, 
                                        ValueIdSet& essentialOutputs,
                                        ValueIdSet& allOutputs)
  {

    NABoolean coverFlag;
    GroupAttributes emptyGA;
    ValueIdSet usedValues;
    allOutputs = nonEssentialOutputs ;
    allOutputs += essentialOutputs;

    for (ValueId exprId = nonEssentialOutputs.init(); 
                          nonEssentialOutputs.next(exprId); 
                          nonEssentialOutputs.advance(exprId))
    {
      allOutputs -= exprId;
      emptyGA.setCharacteristicOutputs(allOutputs);
      if (exprId.getItemExpr()->getOperatorType() == ITM_VEG_PREDICATE)
        {
          VEG * vegPtr = ((VEGPredicate *)(exprId.getItemExpr()))->getVEG();
          coverFlag = emptyGA.covers(vegPtr->getVEGReference()->getValueId(),
                                      allOutputs,
                                      usedValues);
        }
      else
        coverFlag = emptyGA.covers(exprId,
                                    allOutputs,
                                    usedValues);
      if (coverFlag) {
        nonEssentialOutputs.subtractElement(exprId);
      }
      else
        allOutputs += exprId;  // exprId is not covered so place it back in allOutputs
    } // for
    usedValues.clear();  // not needed for nonEssentialOutputs

    for (ValueId exprId = essentialOutputs.init(); 
                        essentialOutputs.next(exprId); 
                        essentialOutputs.advance(exprId))
    {
      allOutputs -= exprId;
      emptyGA.setCharacteristicOutputs(allOutputs);
      if (exprId.getItemExpr()->getOperatorType() == ITM_VEG_PREDICATE)
        {
          VEG * vegPtr = ((VEGPredicate *)(exprId.getItemExpr()))->getVEG();
          coverFlag = emptyGA.covers(vegPtr->getVEGReference()->getValueId(),
                                      allOutputs,
                                      usedValues);
        }
      else
        coverFlag = emptyGA.covers(exprId,
                                    allOutputs,
                                    usedValues);
      if (coverFlag) {
        essentialOutputs.subtractElement(exprId);
      }
      else
        allOutputs += exprId;  // exprId is not covered so place it back in allOutputs
    } // for
    essentialOutputs += usedValues ; // promoting some nonEssential outputs to be essential
  }

// --------------------------------------------------------------------
// resolveCharacteristicInputs()
//
// A method for replacing each VEGReference that is a member of
// the Characteristic Inputs with one or more values that belong
// to the VEG and are also available in the external inputs.
//
// This method is used by the code generator.
//
// Effect :
// The Characteristic Inputs can change.
// --------------------------------------------------------------------
void GroupAttributes::resolveCharacteristicInputs(const ValueIdSet& externalInputs)
{
   // Separate out the VEGReferences
   ValueIdSet vegRefs;
   requiredInputs_.lookForVEGReferences(vegRefs);

   // Replace the VEGRefs contained in any other expression
   requiredInputs_.replaceVEGExpressions(externalInputs, externalInputs);

   // expand the VEGRefs
   ValueIdSet expandedVEGs;
   expandedVEGs.getUnReferencedVEGmembers(vegRefs);
   // in addition to this, if the vegRefs contain any current_time, current_date etc
   // which are also available in externalInputs then they should also be added
   // in requiredInputs_. Genesis case # 10-010906-5361
   ValueIdSet currentConstants;
   currentConstants.clear();
   for (ValueId x = vegRefs.init();
       vegRefs.next(x);
       vegRefs.advance(x))
    {
	 if (x.getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE)
	 {
	   VEGReference *vegRef = ((VEGReference *)(x.getItemExpr()));
	   ValueIdSet allValues = vegRef->getVEG()->getAllValues();
	   // in all these values check for current_user amd current_timestamp
	   for (ValueId vid = allValues.init(); allValues.next(vid); allValues.advance(vid))
	   {
	     ItemExpr * vidExpr = vid.getItemExpr();
	     if ((vidExpr->getOperatorType() == ITM_CURRENT_USER) ||
		 (vidExpr->getOperatorType() == ITM_CURRENT_TIMESTAMP))
	       currentConstants += vid;
	   }
	 }
    }
    // these constants should also be added to the list of expandedVEGs
    expandedVEGs += currentConstants;

   // all the available values become inputs
   expandedVEGs.intersectSet(externalInputs);

   requiredInputs_ += expandedVEGs;
}

// -----------------------------------------------------------------------
// GroupAttributes::resolveCharacteristicOutputs()
// -----------------------------------------------------------------------
void GroupAttributes::resolveCharacteristicOutputs
                         (const ValueIdSet & availableValues,
                          const ValueIdSet & externalInputs)
{
  // ---------------------------------------------------------------------
  // Save a copy of the required outputs before they are rewritten.
  // ---------------------------------------------------------------------
  ValueIdSet redundantOutputs(requiredOutputs_);
  // ---------------------------------------------------------------------
  // Replace VEGReference expressions with corresponding values that
  // belong to childOutputs.
  // ---------------------------------------------------------------------
  requiredOutputs_.replaceVEGExpressions(availableValues,externalInputs);
  // ---------------------------------------------------------------------
  // Delete all external inputs from the required outputs.
  // ---------------------------------------------------------------------
  requiredOutputs_ -= externalInputs;

  // ---------------------------------------------------------------------
  // The remaining code in this procedure is a hack that was added long
  // time ago to work around a design problem. I'm changing it somewhat but
  // not yet removing it.
  //
  // The problem is when more than one child is producing an expression
  // that is not rooted in a VEGRef but contains a VEGRef and both
  // children can produce a member of the VEGRef. When the first child
  // rewrites its output it rewrites the expression such that it no
  // longer contains a VEGRef. When the second child rewrites its outputs
  // it can no longer rewrite the VEGRef nor produce the expression.
  // The solution is to change the normalizer so that only one of the child
  // has the expression as an output.
  // The hack is to remove from the output any expressions that were not
  // rewritten and that cannot be covered. This can hide also sorts of
  // bugs and that is why I don't like it.
  // ---------------------------------------------------------------------
  // ---------------------------------------------------------------------
  // Check which of the required outputs did not change after the rewrite.
  // ---------------------------------------------------------------------
  redundantOutputs.intersectSet(requiredOutputs_);
  // ---------------------------------------------------------------------
  // All those values that can are produced by the child and also appear
  // in the requiredOutputs_ are not redundant by definition
  // ---------------------------------------------------------------------
  redundantOutputs -= availableValues;

  if (NOT redundantOutputs.isEmpty())
    {
      redundantOutputs.removeCoveredExprs(availableValues);
      requiredOutputs_ -= redundantOutputs;
    }

} // GroupAttributes::resolveCharacteristicOutputs()

// Clear the analysis values in groupAnalysis
void GroupAttributes::clearGroupAnalysis()
{
  groupAnalysis_->clear();
}

void GroupAttributes::clearLogProperties()
{
  // clear logical properties if previously synthesized.
  clearConstraints();

  // reset any association with the logical expression for
  // which synthesis was performed.
  setLogExprForSynthesis (NULL);

  // clear any estimated logical properties
  clearAllEstLogProp();

} // GroupAttributes::clearLogProperties


void
GroupAttributes::clearAllEstLogProp()
{
  intermedOutputLogPropList().clear();
  outputLogPropList().clear();

  // input estimated logical properties are not owned by the
  // group ... so just remove the references.
  inputLogPropList().clear();

  // reset the previously calculated output cardinality
  outputCardinalityForEmptyLogProp_ = -1;
  outputMaxCardinalityForEmptyLogProp_ = -1;
}

EstLogPropSharedPtr GroupAttributes::outputLogProp (const EstLogPropSharedPtr& inLP)
{
  // See if the desired output log. property has already been synthesized.
  // If so, just return them.
  Int32 index = existsInputLogProp (inLP);
  if (index >= 0) return outputEstLogProp_[index];

  // OLP has not been synthesized ... synthesize now on demand
  //   for a given logical expression in this group.
  RelExpr * logExpr = getLogExprForSynthesis();

  // The association between group attributes and one logical expr
  // in the group (the first for which we invoked synthLogProp) should
  // have happened already.  If this did not happen, then it means that
  // we have a log expr for which synthLogProp was not invoked ...
  // if so, fix it!!

  CMPASSERT (logExpr);

  EstLogPropSharedPtr outputEstLogProp;

  // check if the properties in this GroupAttr and the inLP could
  // have been cached by ASM

 if( QueryAnalysis::Instance() )
 {
  // get the ASM cache pointer
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();

  // Look for in the ASM cache only if the CQD ASM is ON
  if (appStatMan != NULL)
  {
    GroupAnalysis * groupAnalysis = getGroupAnalysis();

    // properties in the ASM are cached with the parentJBBView
    // of the JBBC

    const JBBSubset * jbbSubsetForThisGroup =\
      groupAnalysis->getParentJBBView();

    // EstLogProps for the RelExprs, such as the one corresponding for
    // alternate index are not cacheable. These will have their
    // JBBSubset equal to NULL. This is set by the Prime Group Analyser.
    // ASM will not cache the estimated logical properties for these
    // RelExprs. The properties will also not be cached, if these
    // have been computed for input estimated logical properties
    // which were not cacheable. The cacheable flag for these
    // properties is set at the time when these properties were
    // computed.

    if ((jbbSubsetForThisGroup != NULL) && (inLP->isCacheable()))
    {

      CANodeIdSet groupNodeSet = jbbSubsetForThisGroup->getJBBCsAndGB();

      // if inLP are cacheable, then they too should have JBBSubset
      // associated with them. The only exception is emptyInputLogProp
      // which are cacheable but have an empty NodeSet.
      // Form a combined nodeSet of the JBBSubset for which these
      // properties are being computed and the inputEstLogProp
      // This will give the outputEstLogProp for the given
      // inputEstLogProp

      if (inLP != (*GLOBAL_EMPTY_INPUT_LOGPROP))
	      groupNodeSet += *(inLP->getNodeSet());

      // lookup, if outputLogProp for these inLP have exist in the cache

      outputEstLogProp = appStatMan->getCachedStatistics(&groupNodeSet);

      if (outputEstLogProp == NULL)
      {
	      // Properties do not exist with ParentJBBView.
	      // But these could have been cached with the localJBBView.

	       if (groupAnalysis->getNodeAnalysis() &&
	        groupAnalysis->getNodeAnalysis()->getJBBC())
	          jbbSubsetForThisGroup = groupAnalysis->getLocalJBBView();

	       if (jbbSubsetForThisGroup != NULL)
	       {
	          CANodeIdSet localNodeSet =\
	            jbbSubsetForThisGroup->getJBBCsAndGB();

	          // if inLP are cacheable, then they too should have JBBSubset
	          // associated with them. Proceed in the same manner as with
	          // the parentJBBView

	          if (inLP != (*GLOBAL_EMPTY_INPUT_LOGPROP))
	            localNodeSet += *(inLP->getNodeSet());
	            // lookup, if outputLogProp for these inLP have exist in the cache
	            outputEstLogProp = appStatMan->getCachedStatistics(&localNodeSet);
	       }
      }

      // The properties were computed for other CANodeIdSet,
      // but these do not exist with these groupAttributes,
      // hence these should be added in the GroupAttr for the given
      // inputLogProp. Also add the same estLogProp to the
      // intermediateEstLogProp list. This is because the outputLogProp
      // inputLogProp and the intermedLogProp are all indexed by the
      // same index.

      if (outputEstLogProp != NULL)
      {
	      inputLogPropList().insert(inLP);
	      outputLogPropList().insert(outputEstLogProp);
	      // check - how and where these intermediateEstLogProps being used.
	      // This might require some change in the logic
	      intermedOutputLogPropList().insert (outputEstLogProp);

	      // There should be exactly one set of output log. properties
	      // corresponding to each set of input log. properties.

	      CCMPASSERT (outputLogPropList().entries() == inputLogPropList().entries());
	      CCMPASSERT (intermedOutputLogPropList().entries() == inputLogPropList().entries());
      }
    }
  }
 }

  // If the Query Analysis instance does not exist, or the properties
  // have not been synthesized by ASM, synthesize the estimateLogProp
  // for this expression including all of its children the usual way.

  if (outputEstLogProp == NULL)
  {
    logExpr->synthEstLogProp (inLP);
    CURRSTMT_OPTGLOBALS->cascade_count++;
    // Return the OLP which we just synthesized.
    index = existsInputLogProp(inLP);

    // We always expect to find the inLP, so assert here.
    //
    DCMPASSERT ((CmpCommon::getDefault(COMP_BOOL_71) == DF_ON) || index >= 0);

    if (index < 0)
    {
      // In release, if this is less than zero, then it may be that
      // this groupAttr is different from logExpr's groupAttr.  Try to
      // reconcile the two GA's by adding the newly synth log prop to
      // this GA as well. And see if that works.  If not then we will
      // assert in release as well.
      //
      index = logExpr->getGroupAttr()->existsInputLogProp(inLP);
      if (index < 0)
      {
        CMPASSERT ("Logical properties for the expression could not be computed");
      }

      addInputOutputLogProp(inLP, logExpr->getGroupAttr()->outputLogPropList()[index]);

      index = existsInputLogProp(inLP);

      if (index < 0)
      {
        CMPASSERT ("Logical properties for the expression could not be computed");
      }
    }

  //#define MONITOR_SAMEROWCOUNT 1
    #ifdef MONITOR_SAMEROWCOUNT

    // all CSD's should have the same rowcount!
    {
      // (this code is in its own scope so its variables don't affect )
      EstLogPropSharedPtr checkOLP = outputEstLogProp_[index] ;
      const ColStatDescList & checkCSDL = checkOLP->getColStats() ;
      checkCSDL.verifyInternalConsistency(0,checkCSDL.entries()) ;
    }
    #endif

    outputEstLogProp = outputEstLogProp_[index] ;
  }

  //++MV
  // !!!!! - This code must be carefully checked by the optimizer group.
  //
  // Update the estimated cardinality by taking the min(max) value between the
  // currently estimated cardinality and the CardConstraint maxRows(minRows) bound
  Cardinality minRows, maxRows;
  if (inLP->getResultCardinality() <= CostScalar (1) &&
      hasCardConstraint(minRows,maxRows))
  {
      outputEstLogProp->setResultCardinality(
	MINOF(outputEstLogProp->getResultCardinality(), maxRows));

      outputEstLogProp->setResultCardinality(
	MAXOF(outputEstLogProp->getResultCardinality(), minRows));

      outputEstLogProp->setMaxCardEst(
	MINOF(outputEstLogProp->getMaxCardEst(), maxRows));

      outputEstLogProp->setMaxCardEst(
	MAXOF(outputEstLogProp->getMaxCardEst(), minRows));
  }

  // Set the Cardinality of EstLogProp to one , if a constraint with maxRows one exists.
  if (inLP->isCardinalityEqOne() && hasCardConstraint(minRows,maxRows) &&
      maxRows == 1)
  {
      outputEstLogProp->setCardinalityEqOne();
  }
  //--MV

  return outputEstLogProp ;
} // GroupAttributes::outputLogProp


EstLogPropSharedPtr GroupAttributes::intermedOutputLogProp (const EstLogPropSharedPtr& inLP)
{
  // See if the desired output log. property has already been synthesized.
  // If so, just return them.
  Int32 index = existsInputLogProp (inLP);
  if (index >= 0) return intermedOutputLogProp_[index];

  // OLP has not been synthesized ... synthesize now on demand
  //   for a given logical expression in this group.
  RelExpr * logExpr = getLogExprForSynthesis();

  // The association between group attributes and one logical expr
  // in the group (the first for which we invoked synthLogProp) should
  // have happened already.  If this did not happen, then it means that
  // we have a log expr for which synthLogProp was not invoked ...
  // if so, fix it!!
  if (logExpr == NULL)
  {
    CMPASSERT ("Trying to get logical properties for an invalid expression");
  }

  // Synthesize the est. output log. property for this expression,
  // including all of its children.  Please note that synthEstLogProp
  // will synthesize both the intermediate OLP as well as final OLP.
  logExpr->synthEstLogProp (inLP);

  // Return the intermediate OLP which we just synthesized.
  index = existsInputLogProp(inLP);
  if (index < 0)
  {
    CMPASSERT ("Logical properties for the expression could not be computed");
  }
  return (intermedOutputLogProp_[index]);

} // GroupAttributes::intermedOutputLogProp


CostScalar GroupAttributes::getResultCardinalityForEmptyInput ()
{
  if ( outputCardinalityForEmptyLogProp_ < 0 ) // i.e., not initialized
    {
      EstLogPropSharedPtr outLP = outputLogProp ((*GLOBAL_EMPTY_INPUT_LOGPROP)) ;
      outputCardinalityForEmptyLogProp_ = outLP->getResultCardinality() ;
      outputMaxCardinalityForEmptyLogProp_ = outLP->getMaxCardEst() ;
      outputCardinalityForEmptyLogProp_.minCsOne();
    }

  return outputCardinalityForEmptyLogProp_ ;
}

CostScalar GroupAttributes::getResultMaxCardinalityForEmptyInput ()
{
  if ( outputMaxCardinalityForEmptyLogProp_ < 0 ) // i.e., not initialized
    {
      EstLogPropSharedPtr outLP = outputLogProp ((*GLOBAL_EMPTY_INPUT_LOGPROP)) ;
      outputCardinalityForEmptyLogProp_ = outLP->getResultCardinality() ;
      outputMaxCardinalityForEmptyLogProp_ = outLP->getMaxCardEst() ;
      outputMaxCardinalityForEmptyLogProp_.minCsOne();
    }

  return outputMaxCardinalityForEmptyLogProp_ ;
}

CostScalar 
GroupAttributes::getResultMaxCardinalityForInput(EstLogPropSharedPtr & inLP)
{
   EstLogPropSharedPtr outLP = outputLogProp (inLP) ;
   CostScalar maxCard = outLP->getResultCardinality() ;

  return maxCard;
}

//----------------------------------------------------------------------
// materializeInputLogProp takes its given inputLogicalProperties, and
// matches them against THIS group's characteristicInput.  Only input
// columns needed by THIS group (the child of a Materialize operator)
// are passed as input columns to a 'normal' invocation of
//             GroupAttributes::outputLogProp
//
// The effect of restricting the inputLogicalProperties is to allow
// the child of the materialize to distinguish between the case where
// it is executed only one time (because it has no dependency upon the
// values used to probe the materialized table) vs. the case where it
// needs to be executed once per probe of the materialize.
//----------------------------------------------------------------------

EstLogPropSharedPtr
GroupAttributes::materializeInputLogProp(const EstLogPropSharedPtr& inLP,
                                         Int32 *multipleReads)
{
   EstLogPropSharedPtr materialInLP(new (STMTHEAP) EstLogProp());

   const ColStatDescList & inColStatsList = inLP->getColStats();
   const ValueIdSet & specifiedInputs = getCharacteristicInputs();

   NABoolean columnFound = FALSE;

   //-------------------------------------------------------------------
   // generate the ColStatDescList of those given input columns that
   // are needed by the child.
   //-------------------------------------------------------------------

   // we probably don't need 'em all, but this is the easiest way to
   // grab all of the multi-column uec information we'll need later
   materialInLP->colStats().insertIntoUecList (inColStatsList.getUecList()) ;

   for ( CollIndex i = 0; i < inColStatsList.entries(); i++ )
     {
      ValueIdList columnVIdList ;
      columnVIdList.insert ( inColStatsList[i]->getVEGColumn() );
      if (columnVIdList.prefixCoveredInSet(specifiedInputs) > 0)
        {
          materialInLP->colStats().insert(inColStatsList[i]);
          columnFound = TRUE;
        }
      else
        {
          NABoolean foundMatch = FALSE;

          // lastly, if any of the members of the input list are themselves
          // VEGReferences, determine if any of those VEGReferences contain
          // one of the valueids associated with the current column stats.
          for (ValueId idIn = specifiedInputs.init();
               specifiedInputs.next(idIn) && !foundMatch;
               specifiedInputs.advance(idIn))
            {
              if ( idIn.getItemExpr()->getOperatorType() ==
                   ITM_VEG_REFERENCE )
                {
                  VEG * nestedVEG = ((VEGReference *) (
                       idIn.getItemExpr()))->getVEG();
                  const ValueIdSet & VEGGroup = nestedVEG->getAllValues();

                  for (ValueId id = VEGGroup.init();
                       VEGGroup.next(id) && !foundMatch;
                       VEGGroup.advance(id))
                    {
                      if ( columnVIdList.contains(id) )
                        foundMatch = TRUE;
                    }
                }
            }

          if (foundMatch)
            {
              materialInLP->colStats().insert(inColStatsList[i]);
              columnFound = TRUE;
            }
        }
     }

   //-------------------------------------------------------------------
   // The FIRST Test (above) is w.r.t. whether or not any columns were
   // found for which there are histogram statistics.
   // The SECOND Test, which has to be made if the first fails, is:
   //     Are there any column references in specifiedInputs?
   //
   // The Absence of availability of histograms does not mean the child
   // of the Materialize is only executed once.  {It simply means we
   // can't do as good a job of estimating its cardinality as we would
   // otherwise do.}
   //-------------------------------------------------------------------
//   if ( !columnFound )
//     {
//       for (ValueId idIn = specifiedInputs.init();
//            specifiedInputs.next(idIn) && !columnFound;
//            specifiedInputs.advance(idIn))
//        {
//        ValueIdSet leafValues;
//          idIn.getItemExpr()->findAll(ITM_BASECOLUMN, leafValues, TRUE, TRUE);

//        if ( !leafValues.isEmpty() )
//          columnFound = TRUE;
//        }
//     }

   //-------------------------------------------------------------------
   // set the input cardinality.... No longer necessarily identical to
   // the one supplied by the  input EstLogProp.
   //   CostScalar resultCardinality_;
   //-------------------------------------------------------------------
   if ( columnFound ) {
         *multipleReads = TRUE;
     materialInLP->setResultCardinality( inLP->getResultCardinality() );
   }
   else {
         *multipleReads = FALSE;
     materialInLP->setResultCardinality( 1 );
   }
   materialInLP->setMaxCardEst( inLP->getMaxCardEst() );

   //-------------------------------------------------------------------
   // set the unresolvedPreds for the materialized InputLP to be those
   // of the given inLP.
   //-------------------------------------------------------------------
   materialInLP->unresolvedPreds() += inLP->getUnresolvedPreds();


   // because of the way materialInLP was built the inputForSemiJoinTSJ was never set
   // therefore we do not have to turn it off before calling outputLogProp
   return  materialInLP;
}  // materializeInputLogProp

//----------------------------------------------------------------------
// materializeOutputLogProp takes its given inputLogicalProperties, and
// matches them against THIS group's characteristicInput.  Only input
// columns needed by THIS group (the child of a Materialize operator)
// are passed as input columns to a 'normal' invocation of
//             GroupAttributes::outputLogProp
//
// The effect of restricting the inputLogicalProperties is to allow
// the child of the materialize to distinguish between the case where
// it is executed only one time (because it has no dependency upon the
// values used to probe the materialized table) vs. the case where it
// needs to be executed once per probe of the materialize.
//----------------------------------------------------------------------

EstLogPropSharedPtr
GroupAttributes::materializeOutputLogProp(const EstLogPropSharedPtr& inLP,
                                          Int32 *multipleReads)
{
  return outputLogProp(materializeInputLogProp(inLP,multipleReads));
}  // materializeInputLogProp


// -----------------------------------------------------------------------
// This method checks whether the provided set of input log. property
// exists in this set of group attributes.  If so, return the index
// in the list.  Otherwise, return -1.
// -----------------------------------------------------------------------
Int32 GroupAttributes::existsInputLogProp (const EstLogPropSharedPtr& inputLP) const
{
  for (CollIndex i = 0; i < inputEstLogProp_.entries(); i++)
  {
    if (inputEstLogProp_[i]->compareEstLogProp (inputLP) == SAME)
#pragma nowarn(1506)   // warning elimination
      return i;
#pragma warn(1506)  // warning elimination
  }
  return -1;
}

// -----------------------------------------------------------------------
// This method checks whether output properties have been synthesized
// for the set of input logical properties.
// -----------------------------------------------------------------------
NABoolean GroupAttributes::isPropSynthesized (const EstLogPropSharedPtr& inputLP) const
{
  Int32 index = existsInputLogProp (inputLP);

  if (index >= 0)
    return (outputEstLogProp_[index] != NULL);
  else
    return FALSE;
}

// -----------------------------------------------------------------------
// This method adds a reference to the provided input estimated logical
// property.  It also allocates a corresponding set of output estimated
// logical properties.
// -----------------------------------------------------------------------
NABoolean GroupAttributes::addInputOutputLogProp (const EstLogPropSharedPtr& inputLP,
                                                  const EstLogPropSharedPtr& outputLP,
                                                  const EstLogPropSharedPtr& intermedOutputLP)
{
  if (isPropSynthesized (inputLP))
    return FALSE;

  // Insert input and output estimated logical properties at end of lists.
  inputLogPropList().insert (inputLP);

  // before inserting the output logical properties and intermediate
  // logical properties in the group attributes cache, compute UEC
  // reduction for each interval, if any and scale the
  // histograms such that the total of rowcount and UEC from the
  // intervals equal the aggregate rowcount and UEC stored in the
  // colstat header

  ColStatDescList & colStats = outputLP->colStats();
  colStats.copyAndScaleHistograms(1);
  RelExpr * logExpr = getLogExprForSynthesis();
  if (logExpr && 
      (CmpCommon::getDefault(HIST_FREQ_VALS_NULL_FIX) == DF_ON) &&
      (logExpr->getOperator().match(REL_ANY_LEFT_JOIN) || 
       logExpr->getOperator().match(REL_ANY_FULL_JOIN)) )
  {
    colStats.computeMaxFreq(TRUE);
  }

  if ( (CURRSTMT_OPTDEFAULTS->histOptimisticCardOpt() == 3) &&
       logExpr &&
       (logExpr->getOperatorType() == REL_SCAN) )
  {
    colStats.setBaseUecToTotalUec();
  }

  // intermediate logical properties are optional, so check for NULL
  // pointer before proceeding

  if (intermedOutputLP)
	intermedOutputLP->colStats().copyAndScaleHistograms(1);

  if( QueryAnalysis::Instance() )
 {
  // get the ASM cache pointer
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();
  if (appStatMan != NULL)
  {
    GroupAnalysis * groupAnalysis = getGroupAnalysis();

    // ASM caches the properties with parentJBBView of the JBBC
    const JBBSubset * jbbSubsetForThisGroup =\
      groupAnalysis->getParentJBBView();

    // If the parentJBBView of the JBBC is NULL, then get its
    // localJBBView

    if (jbbSubsetForThisGroup == NULL)
    {
       if (groupAnalysis->getNodeAnalysis() &&
	groupAnalysis->getNodeAnalysis()->getJBBC())
	  jbbSubsetForThisGroup = groupAnalysis->getLocalJBBView();
    }

    // If there is no JBBSubset associated with the group, then its
    // estimated logical proeprties cannot be cached. The properties
    // also cannot be cached if the inputEstLogProp for which these
    // properties are being computed is not cacheable. The cacheable
    // flag for these proeprties would be FALSE

    if ((jbbSubsetForThisGroup != NULL) && (inputLP->isCacheable()))
    {

      // Define a pointer to the CANodeIdSet for the key in the cache
      CANodeIdSet * nodeIdSet = new (STMTHEAP)\
	CANodeIdSet(jbbSubsetForThisGroup->getJBBCsAndGB(), STMTHEAP);

      // The estimatedLogProp in the ASM cache would be identified
      // by the NodeSet of the left child, the right child and the
      // outer child.

      if (inputLP != (*GLOBAL_EMPTY_INPUT_LOGPROP))
	      nodeIdSet->insert(*(inputLP->getNodeSet()));

      // Before the properties can be cached, they should have
      // had their nodeSet set equal to the leftNodeSet + rightNodeSet
      // + inputNodeSet. And their cacheable flag to TRUE

      outputLP->setNodeSet(nodeIdSet);
      outputLP->setCacheableFlag(TRUE);

      // Cache the pointer to estLogProp in the ASM cache,
      // keyed by NodeIdSet.
      appStatMan->insertCachePredStatEntry(*nodeIdSet, outputLP);
    }
  }
 }

 // Insert new output estimated logical properties at end of list.
  outputLogPropList().insert (outputLP);

  // See if any intermediate output logical properties are provided?
  // If so, insert it.  Otherwise, set the intermediate LP to that of the
  // final output LP.
  if (intermedOutputLP != NULL)
  {
    intermedOutputLogPropList().insert (intermedOutputLP);
  }
  else
    intermedOutputLogPropList().insert (outputLP);

  // There should be exactly one set of output log. properties
  // corresponding to each set of input log. properties.
  CCMPASSERT (outputLogPropList().entries() == inputLogPropList().entries());
  //CCMPASSERT (intermedOutputLogPropList().entries() == inputLogPropList().entries());

  // in case they have not been added for some reason, these can be recomputed. So no need
  // to assert in release mode
  return TRUE;

} //GroupAttributes::addInputOutputLogProp


// LCOV_EXCL_START
// this method is used by the GUI debugger for displaying the
// estimated logical properties which are cached by the ASM

SHPTR_LIST(EstLogPropSharedPtr) * GroupAttributes::getCachedStatsList()
{

  SHPTR_LIST (EstLogPropSharedPtr) * statsList =
    new (STMTHEAP) SHPTR_LIST (EstLogPropSharedPtr) (STMTHEAP);

  // Check if the instance of the QA and ASM exist
  QueryAnalysis *qa = QueryAnalysis::Instance();
  AppliedStatMan *appStatMan = (qa == NULL) ? NULL : qa->getASM();

  if (appStatMan != NULL)
  {
    // Get the JBBsubset for this this group
    GroupAnalysis * groupAnalysis = getGroupAnalysis();
    const JBBSubset * jbbSubsetForThisGroup =\
      groupAnalysis->getParentJBBView();

    if (jbbSubsetForThisGroup != NULL)
    {
      EstLogPropSharedPtr outputEstLogProp;
      CANodeIdSet groupNodeSet = jbbSubsetForThisGroup->getJBBCsAndGB();
      CANodeIdSet * inNodeSet = NULL;

      // If the JBBsubset for this group exists then its stats
      // should exist in the cache.
      // Get stats from ASM cache for all inputLogProps in the group

      for (CollIndex i = 0; i < inputEstLogProp_.entries(); i++)
      {
	if ((inputEstLogProp_[i]->isCacheable())
	  && (inputEstLogProp_[i] != (*GLOBAL_EMPTY_INPUT_LOGPROP)))
	     inNodeSet = inputEstLogProp_[i]->getNodeSet();

	if (inNodeSet)
	{
	  CANodeIdSet combinedWithInputNodeSet = groupNodeSet;
	  combinedWithInputNodeSet.insert(*inNodeSet);
	  outputEstLogProp =\
	  appStatMan->getCachedStatistics(&combinedWithInputNodeSet);
	}
	else
	outputEstLogProp =\
	  appStatMan->getCachedStatistics(&groupNodeSet);

	// If the outputEstlogProp for this inputLogProp exist
	// in the ASM cache, add them to the estLogProp list that
	// will be returned to the GUI debugger.

	if (outputEstLogProp != NULL)
	  statsList->insert(outputEstLogProp);

	// reinitialize inNodeSet, as it is not necessary that all
	// inputLogProps for which outputLogProps have been evaluated
	// for this group are cacheable

	inNodeSet = NULL;
      }
    }
  }
  return statsList;
}
// LCOV_EXCL_STOP

ColStatsSharedPtr GroupAttributes::getColStatsForSkewDetection(
                                              const ValueId vId,
					      const EstLogPropSharedPtr& inLP)
{
  ColStatDescSharedPtr colStatDesc;

  // get the output logical properties from group attributes for
  // input logical properties
  EstLogPropSharedPtr outputLogProg = outputLogProp(inLP);

  ColStatDescList colStatDescList = outputLogProg->getColStats();

  ColStatsSharedPtr colStats = colStatDescList.getColStatsPtrForPredicate(vId);

  if (vId.getItemExpr()->isAPredicate())
    colStats = colStatDescList.getColStatsPtrForPredicate(vId);
  else {
    ValueIdSet veg(vId);
    colStats = colStatDescList.getColStatsPtrForVEGGroup(veg);
  }

   return colStats;
}

CostScalar GroupAttributes::getSkewnessFactor(const ValueId vId,
					      EncodedValue & mostFreqVal,
					      const EstLogPropSharedPtr& inLP)
{
  NABoolean histFound = FALSE;
  CostScalar avgFreq, totalRowCount, maxFreq = csMinusOne;

  ColStatsSharedPtr colStats = getColStatsForSkewDetection(vId, inLP);
  
  if (colStats == NULL)
    return csMinusOne;


  if ( (!colStats->isOrigFakeHist()) )
  {
    const FrequentValueList & frequentValueList = colStats->getFrequentValues();

    for (CollIndex j = 0; j < frequentValueList.entries(); j++)
    {
      const FrequentValue & frequentValue = frequentValueList[j];

      avgFreq = frequentValue.getFrequency() * frequentValue.getProbability();

      if (maxFreq < avgFreq )
      {
 	maxFreq = avgFreq;
	mostFreqVal = frequentValue.getEncodedValue();
      }
    }
  }

  return maxFreq/(colStats->getRowcount());

} // GroupAttributes::getSkewnessFactor


// LCOV_EXCL_START

SkewedValueList* GroupAttributes::getSkewedValues(const ValueId& vId,
                                              double threshold,
                                              NABoolean& statsExist,
					      const EstLogPropSharedPtr& inLP,
                                              NABoolean includeNullIfSkewed
                                                 )
{
  ColStatDescSharedPtr colStatDesc;
  ValueIdSet vidSet;
  SkewedValueList* svList = NULL;

  if ( cachedSkewValuesPtr_ == NULL )
    cachedSkewValuesPtr_ = new (STMTHEAP) 
                      NAHashDictionary<ValueId,SkewedValueList>
                      (valueIdHashKeyFunc, /* hash func defined above*/
                       20 /*number of buckets*/, 
                       TRUE, /* enforce uniqueness*/
                       STMTHEAP 
                      );
  else {
    svList = cachedSkewValuesPtr_ -> getFirstValue(&vId);
    if ( svList ) {
      statsExist = TRUE;

      // if no useful skewed values in the list, return NULL
      if ( svList->entries() > 0 )
        return svList;
      else
        return NULL;

    }
  }
    
  // get the output logical properties from group attributes for 
  // input logical properties
  ColStatsSharedPtr colStats = getColStatsForSkewDetection(vId, inLP);

  // if no stats ptr or the stats is a faked one (i.e.  no stats updated for the
  // the column in question, then we bail out.
  if (colStats == NULL || colStats->isOrigFakeHist() )
  {
    statsExist = FALSE;
    return NULL;
  } else
    statsExist = TRUE;


  ItemExpr* iePtr = vId.getItemExpr();

  if (iePtr->getOperatorType() == ITM_INSTANTIATE_NULL) {
     iePtr = iePtr -> child(0);
  }

  switch (iePtr->getOperatorType()) {
     case ITM_EQUAL: // this case is used to handle char type when
                     // no VEG is formed for a char predicate, 
                     // or joins involving subqueries.
     case ITM_VEG_PREDICATE:
     case ITM_VEG_REFERENCE:

       // We only care columns of type ITM_BASECOLUMN (columns belonging to 
       // base tables or table-valued stored procedures, see comment on class
       // BaseColumn).
       iePtr->findAll(ITM_BASECOLUMN, vidSet, TRUE, TRUE);

       // If no such columns can be found. Do not bother to continue further,
       // as only base table columns have the potential to be big and skewed. 
       if ( vidSet.entries() == 0 )
          return NULL;

       break;

     default:
       return NULL;
  }

  // get the first column id in the set
  ValueId colVid((CollIndex)0); vidSet.next(colVid);


  NADefaults &defs = ActiveSchemaDB()->getDefaults();

  const double total_rowcount_threshold = 
     defs.getAsDouble(SKEW_ROWCOUNT_THRESHOLD);

  // wide rowlength should lower skew_rowcount_threshold
  CostScalar factor =  MAXOF(getRecordLength() / 1000, 1.0);
  CostScalar modifiedCard = colStats->getRowcount() * factor;
  CostScalar modifiedMaxCard = getResultMaxCardinalityForEmptyInput() * factor;

  // Do not activate skew Buster if the max. cardinality is 10 times
  // less than, or total rowcount on the column is less than
  // SKEW_ROWCOUNT_THRESHOLD.
  if ((modifiedMaxCard < 10 * total_rowcount_threshold)
       OR
       (modifiedCard < total_rowcount_threshold)
       )
    return NULL;

  // Create a new skew value list for newly founded skew values.
  svList = new (STMTHEAP) SkewedValueList(colVid.getType().newCopy(STMTHEAP), STMTHEAP, 20);

  // Add the value list to the hash table, keyed by the vid of the 
  // joining column
  cachedSkewValuesPtr_ -> insert(new (STMTHEAP) ValueId(vId), svList);

  // Figure out which representation should be used in the skew list.
  const NAType* naType = svList->getNAType(); 
  NABoolean useHashValue = naType->useHashRepresentation();


  if ( (!colStats->isOrigFakeHist()) )
  {
    const FrequentValueList & frequentValueList = colStats->getFrequentValues();
    CostScalar thresholdAdFrequency = 
                  threshold * (colStats->getRowcount()).getValue();

    for (CollIndex j = 0; j < frequentValueList.entries(); j++)
    {
      const FrequentValue & frequentValue = frequentValueList[j];
      EncodedValue skewed;
                  
      if ( frequentValue.getFrequency() * frequentValue.getProbability() 
           < thresholdAdFrequency )
        continue;		  

      if (svList->entries() >= CURRSTMT_OPTDEFAULTS->maxSkewValuesDetected())
        break;

      if (frequentValue.getEncodedValue().isNullValue())
      {
         if(includeNullIfSkewed)
            skewed.setValueToNull();
         else
           continue;
      } else {
         if ( useHashValue == FALSE ) {

             skewed = frequentValueList[j].getEncodedValue(); // for SHORT, INTEGER and LARGEINT
         }  else { 

            const NAType& nt = colVid.getType();

            if (
                 nt.getTypeQualifier() == NA_NUMERIC_TYPE &&
                 nt.getTypeName() == LiteralNumeric
            ) {
                skewed = 
                   frequentValueList[j].getEncodedValue().
                         computeHashForNumeric((SQLNumeric*)&nt); // for numeric
            } else
                skewed = frequentValueList[j].getHash(); // for CHAR 
	   }
     }

     svList->insertInOrder(skewed);
     if (skewed.isNullValue())
     {
        svList->setIsNullInList(TRUE);
        svList->setIsNullSkewed(TRUE);
     }
    } // for all skewed values of this ColStat
  } 
  
  // if no useful skewed values in the list, return NULL
  if ( svList->entries() > 0 )
    return svList;
  else
    return NULL;
} // GroupAttributes::getSkewedValues

double GroupAttributes::getAverageVarcharSize(const ValueId vId,
                                             const EstLogPropSharedPtr& inLP)
{
  
  ColStatDescSharedPtr colStatDesc;
  ValueIdSet vidSet;

  // maybe we need to impose restrictions???
  vId.getItemExpr()->findAll(ITM_BASECOLUMN, vidSet, TRUE, TRUE);

  if(vidSet.isEmpty() || 
     vidSet.entries()>1 )
    return -1;

  ValueId colVid((CollIndex)0); vidSet.next(colVid);
  
  if (!(colVid.getType().getVarLenHdrSize() > 0))
    return -1;

  // Check cache for the desired output log. property
  EstLogPropSharedPtr outputLogProg;
  Int32 index = existsInputLogProp (inLP);
  if (index >= 0)
	  outputLogProg = outputEstLogProp_[index];
  else
	  return -1;

  ColStatDescList colStatDescList = outputLogProg->getColStats();

  for (CollIndex i = 0; i < colStatDescList.entries(); i++)
  {
    colStatDesc = colStatDescList[i];
    if ( colVid == colStatDesc->getColumn())
    {
      ColStatsSharedPtr colStats = colStatDesc->getColStats();
      return colStats->getAvgVarcharSize()/100;

    }
  }
  return -1;
} 

double GroupAttributes::getAverageVarcharSize(const ValueIdList & valIdList, UInt32 & maxRowSize)
{

  double cumulAvgVarCharSize = 0;
  UInt32 CumulTotVarCharSize = 0;

  CollIndex numEntries = valIdList.entries();

  for( CollIndex i = 0; i < numEntries; i++ )
  {
    if (valIdList.at(i).getType().getVarLenHdrSize()>0)
    {
      double avgVarCharSize = 0;
      //ValueId vid = valIdList.at(i);

      avgVarCharSize = getAverageVarcharSize( valIdList.at(i));
      if (avgVarCharSize >0)
      {
        cumulAvgVarCharSize += avgVarCharSize;
      }
      else
      {
        cumulAvgVarCharSize += valIdList.at(i).getType().getTotalSize();
      }
      CumulTotVarCharSize += valIdList.at(i).getType().getTotalSize();
    }
  }

  maxRowSize = valIdList.getRowLength();

  return maxRowSize - CumulTotVarCharSize + cumulAvgVarCharSize;
}
void 
GroupAttributes::setEssentialCharacteristicOutputs(const ValueIdSet & vset)
{
  NABoolean misMatchFound = FALSE;
  for (ValueId id = vset.init();
	vset.next(id) && !misMatchFound;
	vset.advance(id))
  {
    if ( NOT isCharacteristicOutput(id) )
      misMatchFound = TRUE;
  }
  CMPASSERT(NOT misMatchFound);
  requiredEssentialOutputs_ = vset;
}
// LCOV_EXCL_STOP

void 
GroupAttributes::addEssentialCharacteristicOutputs(const ValueIdSet & vset)
{
  NABoolean misMatchFound = FALSE;
  for (ValueId id = vset.init();
	vset.next(id) && !misMatchFound;
	vset.advance(id))
  {
    if ( NOT isCharacteristicOutput(id) )
      misMatchFound = TRUE;
  }
  CMPASSERT(NOT misMatchFound);
  requiredEssentialOutputs_ += vset;
}

void 
GroupAttributes::getNonEssentialCharacteristicOutputs(ValueIdSet & vset) const 
{
  vset += getCharacteristicOutputs();
  vset -= getEssentialCharacteristicOutputs();
}


