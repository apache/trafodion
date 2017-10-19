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
 *****************************************************************************
 *
 * File:         mdam.C
 *
 * Created:      //96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "mdam.h"
#include "disjunct.h"
#include "disjuncts.h"
#include "ItemExpr.h"
#include "mdamkey.h"
#include "ItemOther.h"
#include "ItemColRef.h"
#include "NATable.h"
#include "ValueDesc.h"
#include "SchemaDB.h"

#include "OptRange.h"

#undef FSOWARNING

#ifndef NDEBUG
#define FSOWARNING(x) fprintf(stdout, "FileScan optimizer warning: %s\n", x);
#else
#define FSOWARNING(x)
#endif

//-----------------------------------------------------------------------
// This amplifies the threshold,MDAM_MAX_NUM_DISJUNCTS allowing greater
// freedom to while we are computing the disjuncts.
const CollIndex MULIPLICATION_FACTOR = 8;
//-----------------------------------------------------------------------

// ***********************************************************************
// $$$ Functions related to DisjunctArray
// ***********************************************************************

// -----------------------------------------------------------------------
// mdamANDDisjunctArrays()
// -----------------------------------------------------------------------
DisjunctArray * mdamANDDisjunctArrays
  ( DisjunctArray *  leftDisjunctArray,
    DisjunctArray * rightDisjunctArray )
{
  //10-040128-2749 -begin
  //---------------------------------------------------------------------
  // IF one of the operands is NULL. This would mean we have Aborted
  // Disjunct calculation due to the computation exceeding the threshold
  // So Return NULL and Convey the message to MY parent.
  //---------------------------------------------------------------------
  if(!leftDisjunctArray OR !rightDisjunctArray)
	  return NULL; //Abort! -- make it know to others.
  //---------------------------------------------------------------------
  // Check if we Exceed the threshold here. If we do then start spreading
  // the bad news.
  //---------------------------------------------------------------------
  CollIndex LEntries = leftDisjunctArray->entries();
  CollIndex REntries = rightDisjunctArray->entries();
  if((LEntries * REntries) > MDAM_MAX_NUM_DISJUNCTS * MULIPLICATION_FACTOR)
  {
	   delete leftDisjunctArray;
       delete rightDisjunctArray;
	   return NULL;
  }
  //10-040128-2749 -end
  // ---------------------------------------------------------------------
  // If either DisjunctArray is empty (due to contradictory
  // predicates) the result will also, so we can just return whichever
  // has the contradictory predicates.
  // ---------------------------------------------------------------------
  //
  if ( rightDisjunctArray->entries() == 0 )
    {
    return rightDisjunctArray;
    }
  else if ( leftDisjunctArray->entries() == 0 )
    {
    return leftDisjunctArray;
    }
  else
  // ---------------------------------------------------------------------
  // If the right DisjunctArray has only a single entry then the
  // predicates in the ValueIdSet that this entry points to can
  // simply be added to the ValueIdSets that the entries in the left
  // DisjunctArray point to.  The right DisjunctArray and its
  // ValueIdSet entries can then be deleted.  The left DisjunctArray
  // is returned to the previous ItemExpr in this recursion over the
  // expression tree.
  // ---------------------------------------------------------------------
  if ( rightDisjunctArray->entries() == 1 )
    {
    leftDisjunctArray->andDisjunct(rightDisjunctArray);
    return leftDisjunctArray;
    }
  else
  // ---------------------------------------------------------------------
  // If the left DisjunctArray has only a single entry then the
  // predicates in the ValueIdSet that this entry points to can
  // simply be added to the ValueIdSets that the entries in the right
  // DisjunctArray point to.  The left DisjunctArray and its
  // ValueIdSet entries can then be deleted.  The right DisjunctArray
  // is returned to the previous ItemExpr in this recursion over the
  // expression tree.
  // ---------------------------------------------------------------------
  if ( leftDisjunctArray->entries() == 1 )
    {
    rightDisjunctArray->andDisjunct(leftDisjunctArray);
    return rightDisjunctArray;
    }
  else
  // ---------------------------------------------------------------------
  // If both the left and right DisjunctArrays have more than 1 entry
  // a new DisjunctArray will have to be created.  The predicates in
  // each disjunct in the left DisjunctArray will be added to the
  // predicates in each disjunct in the right DisjunctArray-> This will
  // result in a new DisjunctArray with the number of entires equal to
  // the product of the number of entries in both the DisjunctArrays.
  // This DisjunctArray is then returned to the previous ItemExpr in
  // this recursion over the expression tree.  The left and right
  // DisjunctArrays and their ValueIdSet entries are deleted.
  // ---------------------------------------------------------------------
    {
    DisjunctArray * disjunctArray = new(CmpCommon::statementHeap())
      DisjunctArray();
    disjunctArray->andDisjunctArrays( leftDisjunctArray,
                                     rightDisjunctArray );
    return disjunctArray;
    }
} // mdamANDDisjunctArrays


// -----------------------------------------------------------------------
// mdamORDisjunctArrays()
// -----------------------------------------------------------------------
DisjunctArray * mdamORDisjunctArrays
  ( DisjunctArray *  leftDisjunctArray,
    DisjunctArray * rightDisjunctArray )
{
  //10-040128-2749 -begin
  //---------------------------------------------------------------------
  // IF one of the operands is NULL. This would mean we have Aborted
  // Disjuct calculation due to the computation excceding the threshold
  // So Return NULL and Convey the message to MY parent.
  //---------------------------------------------------------------------
  if(!leftDisjunctArray OR !rightDisjunctArray)
	  return NULL; //Abort! -- make it know to others.
  //---------------------------------------------------------------------
  // Check if we Exceed the threshold here. If we do then start spreading
  // the bad news.
  //---------------------------------------------------------------------
  CollIndex LEntries = leftDisjunctArray->entries();
  CollIndex REntries = rightDisjunctArray->entries();
  if((LEntries + REntries) > MDAM_MAX_NUM_DISJUNCTS * MULIPLICATION_FACTOR)
  {
	   delete leftDisjunctArray;
       delete rightDisjunctArray;
	   return NULL;
  }
  //10-040128-2749 -end
  // ---------------------------------------------------------------------
  // If either DisjunctArray is empty (i.e. contains contradictory
  // predicates), we know it will not contribute to the OR.  So, we
  // can just return the other one.
  // ---------------------------------------------------------------------
  //
  if ( rightDisjunctArray->entries() == 0 )
    {
    return leftDisjunctArray;
    }
  else if ( leftDisjunctArray->entries() == 0 )
    {
    return rightDisjunctArray;
    }
  else
  // ---------------------------------------------------------------------
  // If the entries in the left DisjunctArray are fewer than in the
  // right DisjunctArray then they are merged into the right Disjunct
  // Array.  Otherwise, the entries in the right DisjunctArray are
  // merged into the left DisjunctArray.  The array whose entries are
  // merged is deleted.  The other is sent back to the previous
  // ItemExpr in this recursion over the expression tree.
  // ---------------------------------------------------------------------
  if ( leftDisjunctArray->entries() < rightDisjunctArray->entries() )
    {
    rightDisjunctArray->orDisjunctArray(  leftDisjunctArray );
    return rightDisjunctArray;
    }
  else
    {
     leftDisjunctArray->orDisjunctArray( rightDisjunctArray );
    return  leftDisjunctArray;
    }
} // mdamORDisjunctArrays


// ***********************************************************************
// $$$ DisjunctArray
// member functions for class DisjunctArray
// ***********************************************************************

// -----------------------------------------------------------------------
// DisjunctArray::~DisjunctArray()
// -----------------------------------------------------------------------
DisjunctArray::~DisjunctArray()
{
  // Goes through all the array entries and for each entry gets rid of the
  // ValueIdSet it points to
  for (CollIndex index = 0; index < entries(); index++)
    delete at(index);

} // DisjunctArray::~DisjunctArray


// -----------------------------------------------------------------------
// DisjunctArray::andDisjunct()
// -----------------------------------------------------------------------
void DisjunctArray::andDisjunct
  ( DisjunctArray * otherDisjunctArray )
{
  // confirm that there is only one disjunct in the disjunct array passed
  assert(otherDisjunctArray->entries() == 1);

  // ---------------------------------------------------------------------
  // Loop through the entries of the disjunct array being processed.  For
  // each one access the ValueIdSet that the entry points to.  Add to this
  // the predicate value ids from the single disjunct (ValueIdSet) in the
  // disjunct array passed in as a parameter (first entry in
  // otherDisjunctArray). The += operator does a logical OR of the two
  // ValueIdSets (bitmaps).
  // ---------------------------------------------------------------------
  for (CollIndex index = 0; index < entries(); index++)
    *at(index) += *(otherDisjunctArray->at(0));

  // delete the otherDisjunctArray and its ValueIdSet entry
  delete otherDisjunctArray;
} // andDisjunct


// -----------------------------------------------------------------------
// DisjunctArray::andDisjunctArrays()
// -----------------------------------------------------------------------
void DisjunctArray::andDisjunctArrays
  ( DisjunctArray *  leftDisjunctArray,
    DisjunctArray * rightDisjunctArray )
{

  // ---------------------------------------------------------------------
  // Loop through the left disjunct array and for each entry in it it loop
  // though the right disjunct array. For a combination of each of the
  // entries in the two disjunctArrays insert an entry into the new
  // disjunctArray.  Essentially perform a cross-product of the two
  // arrays.  So the number of entries in the new array should be the
  // number of entries in the left disjunct array times the number in the
  // right disjunct array.
  //
  // For each combination of the entries first copy the left disjunct
  // entry creating a new ValueIdSet.  To this newly created ValueIdSet
  // add the predicate value ids in the right array ValueIdSet.  The
  // += operator helps do this.  It performs a logical OR of the
  // predicates in both the left and the right ValueIdSet entries.
  // ---------------------------------------------------------------------
  for (CollIndex l = 0; l < leftDisjunctArray->entries(); l++)
    for (CollIndex r = 0; r < rightDisjunctArray->entries(); r++)
      {
      ValueIdSet * disjunct = new (CmpCommon::statementHeap())
	ValueIdSet(*(leftDisjunctArray->at(l)));
      *disjunct += *(rightDisjunctArray->at(r));
      insert(disjunct);
      }

  // ---------------------------------------------------------------------
  // Delete the left and right disjunct arrays also deleting all the
  // ValueIdSets that they pointed to.
  // ---------------------------------------------------------------------
  delete  leftDisjunctArray;
  delete rightDisjunctArray;
} // andDisjunctArrays


// -----------------------------------------------------------------------
// DisjunctArray::orDisjunctArray()
// -----------------------------------------------------------------------
void DisjunctArray::orDisjunctArray
  ( DisjunctArray * otherDisjunctArray )
{
  // ---------------------------------------------------------------------
  // Loop through the disjunct array passed and insert each entry into
  // the disjunct array being processed.
  // ---------------------------------------------------------------------
  for (CollIndex index = 0; index < otherDisjunctArray->entries(); index++)
    insert(otherDisjunctArray->at(index));

  // ---------------------------------------------------------------------
  // Since the ValueIdSets that the array pointed to still need to be
  // preserved, clear the disjunct array that is not needed anymore (has
  // been merged) and then delete it.  Simply deleting it would also
  // delete the ValueIdSets that it points to.
  // ---------------------------------------------------------------------
  otherDisjunctArray->clear();
  delete otherDisjunctArray;
} // orDisjunctArray



void DisjunctArray::display() const
{
  print();
}

void DisjunctArray::print( FILE* ofd,
			   const char* indent,
			   const char* title) const
{


  fprintf(ofd,title);
  fprintf(ofd,"\n");

  // for every disjunct:
  NAString disStr(CmpCommon::statementHeap());
  for (CollIndex i = 0; i < entries(); i++) {
    ValueIdSet& disjunct = *(*this)[i];

    // Print it:
    disStr =  NAString("disjunct[") +
      NAString((unsigned char)('0'+Int32(i))) + NAString("]: ");
    disjunct.print(ofd,indent,disStr);

  } // for every disjunct

} // display()


//---------------------------------------------------------
// Methods for class ScanKey
//---------------------------------------------------------
// -----------------------------------------------------------------------
// replicateNonKeyVEGPredicates()
//
// Analyze VEGPredicates that appear in the setOfKeyPredicates.
// Replicate them as non-key predicates when their VEG contains entries 
// such that we must result in more than ONE comparison.
// Genesis Solution 10-091110-6239
//
// For example, a query like this:
// SELECT *
// FROM CUBE1 T4
// WHERE
// EXISTS(SELECT T6."SID" FROM SI001 T6 WHERE T6."SID" = T4."SID_RSDRSIR01" AND
// T6."SID" = 7)
// OR
// T4. "SID_RSDRSIR01" = 8
// ;
// 
// The key expression of the scan of t6 will contain t6.sid=7, however if 
// we don't replicate the predicate to the executor predicate we will not
// do the comparison to the outer table t4.
//
// So, if the list of VEG members contains more than 1 entry after we have 
// removed the keyColumns of the index, we need to replicate the expression.
// Note that we remove the index columns from the VEG members beforehand,
// so that we only count base columns and constants.
//
// -----------------------------------------------------------------------

void
ScanKey::replicateNonKeyVEGPredicates(
     const ValueIdSet& setOfKeyPredicates,
     ValueIdSet& setOfNonKeyPredicates /* out */
     ) const
{

  // we need the valueIds of the key columns of the index,
  // expressed in terms of base columns

  ValueIdSet expandedKeyCols = getIndexDesc()->getIndexKey();
  const IndexDesc *UDIdesc = getUDIndexDesc();

  // If we are doing update/delete, then get the keyCols from 
  // the update table as well.
  if (UDIdesc != NULL)
    {
      ValueIdSet expandedUDKeyCols(UDIdesc->getIndexKey());
      expandedKeyCols += expandedUDKeyCols;
    }

  expandedKeyCols = expandedKeyCols.convertToBaseIds();

  // If we are doing update/delete, then get the nonkeyCols from 
  // the update table as well.
  ValueIdSet ghostNonKeyColumnSet;
  if (UDIdesc != NULL)
    UDIdesc->getNonKeyColumnSet(ghostNonKeyColumnSet);
  
  // Find true outer references

  ValueIdSet trueOuterReferences;
  ValueIdSet inputs = getOperatorInputs();


  // This CQD is turned off by default as it is difficult to guarantee
  // that we will not remove two many elements, in which case we could
  // possibly, incorrectly replicate to few predicates, which can leads to
  // incorrect results.

  // For now only trigger half of the algorithm, when we have no inputs to 
  // improve on insert/delete type queries in particular.

  if (CmpCommon::getDefaultLong(ALLOW_INPUT_PRED_REPLICATION_REDUCTION) > 1) 
    {
    for (ValueId inputExprId = inputs.init();
         inputs.next(inputExprId);
         inputs.advance(inputExprId) )
      {
        // Found a VEGReference?
        ItemExpr * inputExprPtr = inputExprId.getItemExpr();
        if (inputExprPtr->getOperatorType() == ITM_VEG_REFERENCE)
          {
            ValueIdSet referenceValues;
    
            // Check if it contains any values that are table columns
            // but not key columns:
            referenceValues = ((VEGReference*)inputExprPtr)->getVEG()->getAllValues();
            ValueIdSet vegConstants;
            referenceValues.getConstants(vegConstants);
            referenceValues -= vegConstants;
  
            for (ValueId refExprId = referenceValues.init();
                 referenceValues.next(refExprId);
                 referenceValues.advance(refExprId) )
              {
                if (refExprId.getItemExpr()->getOperatorType() == ITM_INDEXCOLUMN)
                  referenceValues -= refExprId;
  
                if (getNonKeyColumnSet().contains(refExprId))
                  referenceValues -= refExprId;
  
                if (expandedKeyCols.contains(refExprId))
                  referenceValues -= refExprId;
              }
  
            // remember the true outer references
            // we only need one value
            if (referenceValues.entries())
              trueOuterReferences += referenceValues.init();
          } 
        else 
          {
            // remember the true outer references
            trueOuterReferences += inputExprId;
          }
      }
    }

  // Iterate over all key predicates.
  ItemExpr* exprPtr = NULL;
  for (ValueId exprId = setOfKeyPredicates.init();
       setOfKeyPredicates.next(exprId);
       setOfKeyPredicates.advance(exprId) )
    {
      // Found a VEGPredicate?
      exprPtr = exprId.getItemExpr();
      if (exprPtr->getOperatorType() == ITM_VEG_PREDICATE)
      {
          ValueIdSet predicateValues;
          ValueIdSet constantValues;
          
          // Check if it contains any values that are table columns
          // but not key columns:
          predicateValues = ((VEGPredicate*)exprPtr)->getVEG()->getAllValues();

          // Remove any physical column references, look only at base columns
          // (our local table or outer references) and constants

          for (ValueId elemId = predicateValues.init(); 
               predicateValues.next(elemId);
               predicateValues.advance(elemId))
          {
             if (elemId.getItemExpr()->getOperatorType() == ITM_INDEXCOLUMN)
                predicateValues -= elemId;

             // Remember the constants
             if ((elemId.getItemExpr()->getOperatorType() == ITM_CONSTANT) ||
                 (elemId.getItemExpr()->getOperatorType() == ITM_CACHE_PARAM) ||
                 (elemId.getItemExpr()->getOperatorType() == ITM_DYN_PARAM))
                constantValues += elemId;

          }


          // Remove our key columns
          //   - this leaves outer references, constants and our non-key columns

          predicateValues -= expandedKeyCols;
   
          // Remove references to Update/Delete node
          // ghostNonKeyColumnSet will be empty for any other operation
          predicateValues -= ghostNonKeyColumnSet;

          // If this scan/search key was created for a GenericUpdate with 
          // a Scan child, make sure we subtract those valueIds from the 
          // expressions as well as they represent the before valueId of those
          // associated with the table/index desc of the GenericUpdate. Ie.
          // they represent the same thing.
          //
          // For example update t set b=0 where a=1
          //
          // Will generate a predicate like this "before_t.a = 1 = after_t.a"
          // and we want to avoid scan predicates like this
          // "before_t.a =1 AND after_t.a = 1"
          // 
          // so for Update and Delete we the ghost column references below
          // are for the after_t columns.
          //
          // Another good example is this compund update:
          //
          //  BEGIN
          //     SELECT D_TAX, D_NEXT_O_ID
          //           FROM T42DIST
          //           WHERE (D_W_ID, D_ID) = (1, 10)
          //           FOR REPEATABLE ACCESS IN EXCLUSIVE MODE;
          //
          //     UPDATE T42DIST
          //       SET D_NEXT_O_ID = D_NEXT_O_ID + 1
          //       WHERE (D_W_ID, D_ID) = (8, 10);
          //  END;
          //
          // Here the VEG will contain 3 separate reference to D_ID for example,
          // from each of the 3 instances of the table in the plan:
          // The code below is able to detect that and remove them before we
          // incorrectly replicate the predicate.
          // 
          // The plan looked like this:
          //        CompoundStmt
          //            / \
          //      scan     update
          //                  \
          //                   scan
          // 
          // the two VEGs for the update contained (D_ID, D_ID, 10, D_ID) and
          // (D_W_ID, D_W_ID, 8, D_W_ID) respectively
          // 
          // The code below translates these VEGs to (D_ID, 10) and (D_W_ID,8)
          // where the column references are those from the update table.
       

          // To detect these types of references we apply the following logic:
          // 
          // a) subtract from the predicateValues any valueId that is NOT
          //   1) a constant
          //   2) reference to a nonKeyColumn for the current table
          //   3) an input to the operator
          //      since the input may be a VEGReference like (D_ID,10,D_ID), 
          //      we need to dig into the inputs and pull out elements 
          //      that are not local non key base columns and constants.
          //      these then would be the true outer references..
          //

          if (CmpCommon::getDefaultLong(ALLOW_INPUT_PRED_REPLICATION_REDUCTION) > 0)
            {
              ValueIdSet ghostVids = predicateValues;
    
              // Begin -
              // nonKeyColumnSet does not have base column value id
              // it has value id for the column in physical access path
              // So have to account for that. This caused a wrong result
              // bug.
              ValueIdSet nonKeyBaseColumns;
              ValueIdSet nonKeyColSet = getNonKeyColumnSet();
              for(ValueId vid=nonKeyColSet.init();
                              nonKeyColSet.next(vid);
                              nonKeyColSet.advance(vid))
              {
                // Since we can call this on any ValueIdSet, make sure we actually have
                // an IndexColumn.
                if(vid.getItemExpr()->getOperatorType() == ITM_INDEXCOLUMN)
                  nonKeyBaseColumns.insert(((BaseColumn *)(((IndexColumn *)vid.getItemExpr())->
                    getDefinition().getItemExpr()))->getValueId());
              }
              
              nonKeyColSet += nonKeyBaseColumns;
              // End -
              
              // remove any of our own columns
              ghostVids -= nonKeyColSet;
    
              // remove any constants
              ghostVids -= constantValues;
    
              // remove any inputs
              // after removing the inputs, ghostVids contains only are 
              // references that looks like outer references but since they 
              // are not part of the operator inputs, we can delete these
    
              ghostVids -= trueOuterReferences;
             
              // delete the ghostVids references..
              predicateValues -= ghostVids;

            }

          NABoolean replicatePredicate = FALSE;

          // If we have multiple non-key columns, replicate predicate
          if (predicateValues.entries() > 1)
             replicatePredicate = TRUE;
          else if (predicateValues.entries() == 1)
          {
             // Even if only one entry is left, if this is a non-key
             // column of our table, then we still have to replicate
             // the predicate. (Note: I doubt that this can ever happen.
             // We need at least one outer reference or constant to form
             // a key predicate. So, if there is only one ItemExpr other
             // than a key column, then it must be that outer reference
             // or constant).
             // If it is an outer reference or constant we don't have
             // to replicate the VEGPredicate as the keyExpression suffices.
             //
             // If it is a base table nonKeyColumn we take a DCMPASSERT()
             // since this isn't supposed to be possible.
             predicateValues.intersectSet(getNonKeyColumnSet());
             DCMPASSERT (predicateValues.entries() == 0);
          }
          if (replicatePredicate == TRUE)
          {
              // There is at least non-key column in the VEG!
              // (i.e. keycolumn=1 and nonkeycolumn=1)
              // thus need to replicate the veg
             setOfNonKeyPredicates += exprId;
          }
      }

    } // end for

} //static replicateNonKeyVEGPredicates()


// -----------------------------------------------------------------------
// Input:
//   ie an item expr
//   column a column
// If the ie is a base column OR the ie is a veg reference, this
// method will return true if the ie makes reference to the
// given column. Else it returns FALSE
// -----------------------------------------------------------------------

NABoolean ScanKey::expressionContainsColumn(
     const ItemExpr& ie
     ,const ValueId& column
     )
{

  NABoolean retVal = FALSE;
  if (ie.getValueId() == column)
    {
      // a base column matches a base column
      // OR
      // an index column matches an index column
      retVal = TRUE;
    }
  else
    {
      // the ie must be a reference or a base column, thus
      // if the column is an index column, get its definition:
      ItemExpr *colPtr = column.getItemExpr();
      ValueId col = column;
      if (colPtr->getOperatorType() == ITM_INDEXCOLUMN)
        {
          col = ((IndexColumn *)(colPtr))->getDefinition();
        }

      if (ie.getOperatorType() == ITM_VEG_REFERENCE)
        {
          // see if the column is inside the VEG of the reference:
          if (ie.referencesTheGivenValue(col))
            {
              retVal = TRUE;
            }
        }
      else if (ie.getOperatorType() == ITM_INDEXCOLUMN)
        {
          // is it an index column for base table column "col"
          if (((IndexColumn &)(ie)).getDefinition() == col)
            {
              retVal = TRUE;
            }
        }
      else if (ie.getValueId() == col)
        { // the index column matches the base column
          retVal = TRUE;
        } // if column is an index column
    } //

  return retVal;
}




NABoolean
ScanKey::isAKeyPredicate(
     const ValueId & predId,
     ValueId & referencedInput,
     ValueId & intervalExclusionExpr) const
{
  NABoolean retVal = FALSE;
  const ValueIdList& keyCols = getKeyColumns();
  for (CollIndex i=0; i < keyCols.entries(); i++)
    {
      const ValueId& col= keyCols[i];
      if (isAKeyPredicateForColumn(predId
                                   ,referencedInput
                                   ,intervalExclusionExpr
                                   ,col
                                   ))
        {
          retVal = TRUE;
          break;
        }
    }

  return retVal;
}

NABoolean
ScanKey::isAKeyPredicate(const ValueId& predId) const
{

  ValueId dummy1(NULL_VALUE_ID);
  ValueId dummy2(NULL_VALUE_ID);
  return isAKeyPredicate(predId, dummy1, dummy2);
} // isAKeyPredicate

NABoolean
ScanKey::isAKeyPredicateForColumn(
     const ValueId& predId
     ,const ValueId& keyColumn
     ) const
{

  ValueId dummy1(NULL_VALUE_ID);
  ValueId dummy2(NULL_VALUE_ID);
  return isAKeyPredicateForColumn(predId
                                  ,dummy1
                                  ,dummy2
                                  ,keyColumn
                                  ,getOperatorInputs());
} // isAKeyPredicate


NABoolean
ScanKey::isAKeyPredicateForColumn(
     const ValueId & predId
     ,ValueId & referencedInput /* out */
     ,ValueId & intervalExclusionExpr /* out */
     ,const ValueId& keyColumn
     ) const
{
  return isAKeyPredicateForColumn(predId, referencedInput, 
                                  intervalExclusionExpr, keyColumn,
                                  getOperatorInputs());
}

NABoolean
ScanKey::isAKeyPredicateForColumn(
     const ValueId & predId
     ,ValueId & referencedInput /* out */
     ,ValueId & intervalExclusionExpr /* out */
     ,const ValueId& keyColumn
     ,const ValueIdSet& inputValues
     )
{

  ValueIdSet externalInputs;

  externalInputs += inputValues;

  referencedInput = NULL_VALUE_ID;
  intervalExclusionExpr = NULL_VALUE_ID;

  ItemExpr * iePtr = predId.getItemExpr();
  // ---------------------------------------------------------------------
  // The predicate must be a unary or a binary comparison predicate.
  // ---------------------------------------------------------------------
  switch (iePtr->getOperatorType())
    {
    case ITM_VEG_PREDICATE:
    case ITM_EQUAL:
    case ITM_LESS:
    case ITM_LESS_EQ:
    case ITM_LESS_OR_LE:
    case ITM_GREATER:
    case ITM_GREATER_EQ:
    case ITM_GREATER_OR_GE:
    case ITM_IS_NULL:
    case ITM_ASSIGN:
      // continue processing
      break;

    default:
      return FALSE;
    }
  // ---------------------------------------------------------------------
  // One operand of the predicate MUST be the key column.
  // ---------------------------------------------------------------------
  CollIndex arity = iePtr->getArity();
  switch (arity)
    {
    case 3:
      // Ternary preds are created by the optimizer to represent
      // partitioning predicates boundaries
      // child(0) and child(1) contain the usual operands (i.a. A > 3)
      // child(2) contains the intervalExclusion flag:
      intervalExclusionExpr = iePtr->child(2)->getValueId();
      // "fall through" to case 2
    case 2:
      // if the left hand side is (represents) the given key column:
      if (expressionContainsColumn(
           *(iePtr->child(0)->castToItemExpr())
          ,keyColumn) )
	{
          // then the right hand side is the operand to the key pred:
	  referencedInput = iePtr->child(1)->castToItemExpr()->getValueId();
	}
      // if the rhs is the key column:
      // (but in the case of an assign (use in insert), it cannot be)
      else if ( (iePtr->getOperatorType() != ITM_ASSIGN)
                AND
                expressionContainsColumn(*(iePtr->
                                         child(1)->castToItemExpr())
                                         ,keyColumn) )
	{
          // then the lhs is the operand to the key pred:
	  referencedInput = iePtr->child(0)->castToItemExpr()->getValueId();
	}
      else
        // the binary operator did not contain a key col in any of its
        // children, thus it cannot be a key pred:
	return FALSE;
      break;
    case 1:
      // the only unary operator that can be key col. is the
      // is NULL
      if (iePtr->getOperatorType() == ITM_IS_NULL)
        {
          if (expressionContainsColumn(*(iePtr->
                                       child(0)->castToItemExpr())
                                       ,keyColumn))
            {
              return TRUE;
            }
          else
            {
              return FALSE;
            }
        }
      else
        return FALSE;
    default:
      // ------------------------------------------------------------------
      // If this is an equality predicate, represented by a VEG predicate,
      // ensure that the key column is a member of the VEG. Also, ensure
      // that at least one external input value is a member of the VEG.
      // ------------------------------------------------------------------
      if (iePtr->getOperatorType() == ITM_VEG_PREDICATE)
	{

	  // --------------------------------------------------------------
	  // If the key column is indeed a member of the VEG
	  // --------------------------------------------------------------
          const VEG  *veg =
            ((VEGPredicate *)iePtr)->getVEG();
          const VEGReference *vegRef =
            veg->getVEGReference();

	  if (expressionContainsColumn(*vegRef,keyColumn))
	    {

	      // ----------------------------------------------------------
	      // Check if any external input is also a member of the VEG.
	      // It is if the intersection of the external inputs
	      // with the elements of the VEG pred is non-empty
	      // ----------------------------------------------------------
              ValueIdSet VEGMembers;
              ValueIdSet vegSet =veg->getAllValues();
              VEGMembers.replaceVEGExpressionsAndCopy(vegSet);
              // The constants are no longer in the inputs, but
              // below we require that they be in there, thus add
              // them to the VEGMembers:

              //
              // Do not pass in 'TRUE' for now. Seems this is a good fix
              // to recognize cache parameters in key predications. But 
              // more work is needed to fix qatddl04 failure due to this change.
              //
              //VEGMembers.getConstants(externalInputs, TRUE);
              //
              VEGMembers.getConstants(externalInputs);

	      ValueIdSet extInputs;
	      extInputs.replaceVEGExpressionsAndCopy(externalInputs);
	      VEGMembers.intersectSet(extInputs);
	      if (NOT VEGMembers.isEmpty()) // if VEG contains external inputs
		{
		  // Indicate that the VEGReference is the referenced value
		  referencedInput = ((VEGPredicate *)iePtr)->getVEG()
                                          ->getVEGReference()->getValueId();
		}
	      else
		return FALSE;  // VEG does not contain a candidate key value
	    }
	  else
	    return FALSE;
	}
      else
        {
        DCMPASSERT(FALSE);
        return FALSE; // internal error case
        }
    }

  // ---------------------------------------------------------------------a
  // The operand of the key predicate (i.e. the "referenced input")
  // MUST be a value that is covered by the
  // external Inputs:
  //
  // We also need to rule out a pred of the form A=B, where A,B are columns
  // of the same table and A is a key column.
  // ---------------------------------------------------------------------a
  if (referencedInput != NULL_VALUE_ID)
    {
      if (NOT (externalInputs.contains(referencedInput)))
        {
          // Create a valueIdSet that contains referencedInput
          ValueIdSet referencedSet;
          referencedSet += referencedInput;

          // Check if it is covered by the available values
          referencedSet.removeUnCoveredExprs(externalInputs);

          // If the referencedInput is not covered we can't use
          // this predicate as a key predicate
          if (referencedSet.isEmpty())
            {
              // Nop, they are not covered, thus it is not
              // a key pred:
              return FALSE;
            }
        }
    }

  if (intervalExclusionExpr != NULL_VALUE_ID AND
      NOT externalInputs.contains(intervalExclusionExpr))
    {
      return FALSE;
    }

  // If we reached here then it is a key pred:
  return TRUE;

} // ScanKey::isAKeyPredicateForColumn(...)

// helper method used in ScanKey::createComputedColumnPredicates below
static ItemExpr *convertCastToNarrow(ItemExpr *expr,
                                     CollHeap *outHeap,
                                     void *)
{
  if (expr->getOperatorType() == ITM_CAST)
    {
      // avoid potential errors of this cast by converting it to a Narrow
      Cast *src = static_cast<Cast *>(expr);

      // Tried to use NAType::errorsCanOccur to do this only in cases
      // where errors are possible, but ran into multiple issues with
      // the types assigned to constants and VEGRefs, so we convert
      // this unconditionally to a Narrow.

      return new(outHeap)
        Narrow(expr->child(0),
               new(outHeap) HostVar("_sys_ignored_CC_convErrorFlag",
                                    new (outHeap) SQLInt(outHeap, TRUE,FALSE),
                                    TRUE),
               src->getType()->newCopy(outHeap));
    }
  else
    return expr;
}

void ScanKey::createComputedColumnPredicates(ValueIdSet &predicates,           /* in/out */
                                             const ValueIdSet &keyColumns,     /* in */
                                             const ValueIdSet &operatorInputs, /* in */
                                             ValueIdSet &generatedPredicates   /* out */)
{
  generatedPredicates.clear();

  if (predicates.isEmpty())
    return; // nothing to do

  ValueIdSet keyCols = keyColumns.convertToBaseIds();

  CollIndex order = 0;
  for (ValueId v = keyCols.init(); keyCols.next(v); keyCols.advance(v))
  {
     ItemExpr *iePtr = v.getItemExpr();
     switch (iePtr->getOperatorType())
       {
       case ITM_BASECOLUMN:
         {
            if (((BaseColumn *) iePtr)->getNAColumn()->isComputedColumn())
            {
              // get the keyColumns referenced in the computed Column expr
              BaseColumn * bcol = (BaseColumn *) iePtr;
              ItemExpr * compExpr = bcol->getComputedColumnExpr().getItemExpr();
              ValueIdSet potentialPredicates(predicates);
              ValueIdSet keyColsReferencedByCompExpr;
              ValueIdSet keyPredicatesOnCC;
              ValueIdMap colToKeyValueMap;

              bcol->getUnderlyingColumnsForCC(keyColsReferencedByCompExpr);
              potentialPredicates = potentialPredicates.replaceRangeSpecRefs();

              if (keyColsReferencedByCompExpr.entries() > 1 ||
                  !bcol->getNAColumn()->isDivisioningColumn())
                {
                  // if the computed column expression references
                  // multiple key columns or is not a divisioning
                  // column, we only know how to mirror that
                  // keyPredicate if it is an equiPred.

                  potentialPredicates.weedOutNonEquiPreds();
                }

              // make sure we have a key pred for all the columns
              // used in the expression of the computed column
              NABoolean predsForAllUnderlyingCols = TRUE;

              for (ValueId u=keyColsReferencedByCompExpr.init();
                   keyColsReferencedByCompExpr.next(u);
                   keyColsReferencedByCompExpr.advance(u))
                {
                  NABoolean foundPredForThisCol = FALSE;
                  for (ValueId p=potentialPredicates.init();
                       potentialPredicates.next(p);
                       potentialPredicates.advance(p))
                    {
                      ValueId keyValueExpr;
                      ValueId dummy;
                      if (isAKeyPredicateForColumn(p,keyValueExpr,dummy,u,operatorInputs))
                        {
                          foundPredForThisCol = TRUE;
                          // Farther down we are going to replace any occurrences of column
                          // u with the expression keyValueExpr in the ItemExpr tree compExpr
                          // that computes column v. Note that compExpr is written usually in 
                          // terms of BaseColumns except for cases like key predicates. So
                          // add base column and its VEGRefs to the map.
                          DCMPASSERT(u.getItemExpr()->getOperatorType() == ITM_BASECOLUMN);
                          BaseColumn *bc = (BaseColumn *)(u.getItemExpr());
                          colToKeyValueMap.addMapEntry(u,keyValueExpr);
                          colToKeyValueMap.addMapEntry(bc->getTableDesc()->
                                                       getColumnVEGList()[bc->getColNumber()],
                                                       keyValueExpr);
                          keyPredicatesOnCC.insert(p);
                        }
                    }
                  if (!foundPredForThisCol)
                    predsForAllUnderlyingCols = FALSE;
                }

              if (predsForAllUnderlyingCols)
                {
                  // create an expression to add to the keyPreds
                  // that will be used to generate begin-end key exprs
                  if (bcol->getNAColumn()->isDivisioningColumn())
                    generatedPredicates += 
                      keyPredicatesOnCC.createMirrorPreds(v, keyColsReferencedByCompExpr);
                  else
                    {
                      // for columns other than divisioning cols, use
                      // a ValueIdMap to do the rewrite for "=" preds,
                      // this handles compExpr trees that reference
                      // multiple base columns

                      // use the basecolumn Veg, using the basecolumn byitself can cause issues
                      // during codegen downstream
                      ValueId egVid = bcol->getTableDesc()->getColumnVEGList()[bcol->getColNumber()];

                      // The computed column expression may contain CAST operators
                      // that could cause conversion errors, which we don't want
                      // to handle here, they should be handled in the underlying
                      // key columns. Change those CASTs to NARROWs and ignore the
                      // error indicators of those Narrow operators.
                      compExpr = compExpr->treeWalk(convertCastToNarrow,
                                                    CmpCommon::statementHeap());
                      compExpr->synthTypeAndValueId();

                      // create a new predicate bcol = compExpr
                      ItemExpr *mirrorPred = 
                        new(CmpCommon::statementHeap()) BiRelat(
                            ITM_EQUAL,
                            egVid.getItemExpr(),
                            compExpr);
                      mirrorPred->synthTypeAndValueId();
                      ValueId mpValId = mirrorPred->getValueId();
                      ValueId mpValIdRewritten;
                      // now rewrite the predicate such that instead
                      // of base columns it uses the values that those
                      // base columns are equated to. Example (simplified):
                      // 
                      // mpValId:           compCol = hash(col1 + col2)
                      // predicates:        col1 = 5 and col2 = ?
                      // colToKeyValueMap:  col1 -> 5, col2 -> ?
                      // mpValIdRewritten:  compCol = hash(5 + ?)
                      colToKeyValueMap.rewriteValueIdDown(mpValId,
                                                          mpValIdRewritten);
                      generatedPredicates += mpValIdRewritten;
                    } // not a divisioning column
                } // preds for all underlying columns
            } // a computed column 
         } // a base column
       } // switch on operator type
  } // loop over key columns
  predicates += generatedPredicates;
} // createComputedColumnPredicates (...)




void ScanKey::getKeyPredicates(ValueIdSet &keyPredicates, /* out */
			       NABoolean * allKeyPredicates,/*out*/
                               CollIndex disjunctNumber /*in*/) const
{
  // This function fills up keyPredicates with
  // the key predicates in this disjunct. Thus
  // it must return *exactly* the key predicates. Then,
  // assert that the output comes in empty in the first place:
  DCMPASSERT(keyPredicates.isEmpty());

  // get predicates from disjunct
  Disjunct disjunct;
  //Parameters used in ScanOptimizer for MDAM costing.
  //The following two parameters enables us to create an empty executor predicate list
  // if all the predicates get applied as key predicates.
  *allKeyPredicates = TRUE;
  CollIndex  keyPredThisDisjunct = CollIndex(0);

  NABoolean ok = getDisjuncts().get(disjunct,disjunctNumber);
  DCMPASSERT(ok); // the disjuct must exist


  // add the key predicates:

  // Add each predicate in this disjunct into
  // the appropriate column of the cache:
  // for each predicate in the i-th disjunct:
  for (ValueId predId = disjunct.getAsValueIdSet().init();
       disjunct.getAsValueIdSet().next(predId);
       disjunct.getAsValueIdSet().advance(predId) )
  {
    // Changes Needed for New RangeSpecRef ItemExpression tree:
    ValueIdSet vdset;
    if( predId.getItemExpr()->getOperatorType() == ITM_RANGE_SPEC_FUNC)
    {
      // Do we need to call getValueIdSetForReconsItemExpr and use it accordingly 
      // ( Need to think a bit ) 
      //        predId = predId.getItemExpr()->child(1).getValueId();
      predId.getItemExpr()->child(1)->getLeafPredicates(vdset);

      for (ValueId predIdi = vdset.init();
	   vdset.next(predIdi);
	   vdset.advance(predIdi) )
      {
	if (isAKeyPredicate(predIdi))  
	{
	  keyPredicates.insert(predIdi);
	  keyPredThisDisjunct = keyPredThisDisjunct+1;
	  if ( *allKeyPredicates ) 
	  {
	    ItemExpr* exprPtr = predIdi.getItemExpr();
	    // VegPred can contain non-key columns, need to check
	    if ( exprPtr->getOperatorType() == ITM_VEG_PREDICATE) 
	    {
	      ValueIdSet	predicateValues;
	      // Check if it contains any values that are table columns
	      // but not key columns:
	      predicateValues = ((VEGPredicate*)exprPtr)->getVEG()->getAllValues();
	      predicateValues.intersectSet(getNonKeyColumnSet());
	      *allKeyPredicates = (predicateValues.entries() == 0);
	    }
	  }
	}
	else  
	{
	  *allKeyPredicates = FALSE;
	}
      } // 2nd for RangeSpecRef
    } // if ends.
    else 
    {
      if (isAKeyPredicate(predId))  
      {
	keyPredicates.insert(predId);
	keyPredThisDisjunct = keyPredThisDisjunct+1;
	if ( *allKeyPredicates ) 
	{
	  ItemExpr* exprPtr = predId.getItemExpr();
	  
	  // VegPred can contain non-key columns, need to check
	  if ( exprPtr->getOperatorType() == ITM_VEG_PREDICATE) 
	  {
	    ValueIdSet	predicateValues;
	    // Check if it contains any values that are table columns
	    // but not key columns:
	    predicateValues = ((VEGPredicate*)exprPtr)->getVEG()->getAllValues();
	    predicateValues.intersectSet(getNonKeyColumnSet());
	    *allKeyPredicates = (predicateValues.entries() == 0);
	    
	  }
	}
      }
      else 
      {
	*allKeyPredicates = FALSE;
      }
    }
  } // for every predicate


}// getKeyPredicates(...)

void ScanKey::splitRangeSpecRef(ColumnOrderList& keyPredsByCol,
				const ValueId& predId,
				const ValueIdList& columns,
				NABoolean firstElemInRangeSpec,
				const ValueId& predIdRange) const
{
  ItemExpr* exprPtr = predId.getItemExpr();
  NABoolean doBreak = TRUE;
  if(exprPtr->getOperatorType() == ITM_VEG_PREDICATE)
  {
    ValueIdSet	predicateValues;
    predicateValues = ((VEGPredicate*)exprPtr)->getVEG()->getAllValues();
    if(predicateValues.entries() > 2 && predicateValues.referencesAConstExpr())
      doBreak = FALSE;
  }

  // for every column in the key:
  for (CollIndex i=0; i < columns.entries(); i++)
  {
    // if the current pred is a key predicate for the
    // current column, insert it into the appropriate
    // bucket:
    const ValueId& column = columns[i];
    if (isAKeyPredicateForColumn(predId,column))
    {
      if (firstElemInRangeSpec == TRUE && predIdRange != NULL_VALUE_ID)
      {
	keyPredsByCol.insertPredicateInColumn(predIdRange,column);
      }
      else if (firstElemInRangeSpec == FALSE && predIdRange != NULL_VALUE_ID)
      {
	// Nothing to be done.
      }
      else
      {
        // Sol 10-091202-6798, do not pass colum argument
        // because key column could have index column vid and
        // keyPredsByCol could have base column vid.
        // Fix is controlled by CQD COMP_INT_73, set to 1 by default.
        if ((ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_73) >= 1)
          keyPredsByCol.insertPredicate(predId);
        else
	  keyPredsByCol.insertPredicateInColumn(predId,column);
      }

      if (doBreak)
	break;
      //10-050403-6270: Prior to this change the inner loop
      //used to exit.This caused some of the predicates to be
      //skipped if there where in a veg of the form
      //VEG(T1.a1, T2.a2 , <const>) or VEG(T1.a1, T2.a2 ...)
      //Now we go to all the members of the VEG.
    }    
  } // for every column
}

// construct a key column order from the given set:
void ScanKey::getKeyColumnOrderList(ColumnOrderList& keyPredsByCol, /* out*/
				    const ValueIdSet& predSet) const
{


  // add the key predicates:

  // Add each predicate in this disjunct into
  // the appropriate column of the cache:
  // for each predicate in the i-th disjunct:

  // 10-050403-6270:
  // The Idea here is we need to generate a List of unique
  // Column entries on a first seen basis. For Example :
  // If we have key columns like [ d,a,b,c,d,e ] we need to
  // Generate a List which will look like [ d,a,b,c,e ].
  ValueIdList columns;
  const ValueIdList &tempColumns = getKeyColumns();
  columns.insert(tempColumns[0]);
  for(CollIndex j = 1; j < tempColumns.entries(); j++)
  {
	NABoolean noDuplicateFound = TRUE;
    for(CollIndex i=0; i < columns.entries(); i++)
	{
	  //Check if we are actually looking at the same base table column.
	  if(columns[i].getNAColumn() == tempColumns[j].getNAColumn())
	  {
		 noDuplicateFound = FALSE;
 	     break;
	  }
	}
	if(noDuplicateFound)
	   columns.insert(tempColumns[j]);

  }

  for (ValueId predId = predSet.init();
       predSet.next(predId);
       predSet.advance(predId) )
  {
    NABoolean firstElemInRangeSpec = TRUE;	
    if ( (CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON ) )
    {
      if ( predId.getItemExpr()->getOperatorType() == ITM_RANGE_SPEC_FUNC) 
      {
	ValueIdSet vdset;
	predId.getItemExpr()->child(1)->getLeafPredicates(vdset);
	for (ValueId predIdp = vdset.init();
	     vdset.next(predIdp);
	     vdset.advance(predIdp) )
	{
	  splitRangeSpecRef(keyPredsByCol,predIdp,columns, firstElemInRangeSpec,predId.getItemExpr()->child(1)->castToItemExpr()->getValueId());
	  firstElemInRangeSpec = FALSE;
	}
      }
      else
	splitRangeSpecRef(keyPredsByCol,predId,columns,firstElemInRangeSpec, NULL_VALUE_ID);
    }
    else
      splitRangeSpecRef(keyPredsByCol,predId,columns,firstElemInRangeSpec, NULL_VALUE_ID);
  } // for every predicate

} // getKeyColumnOrderList(...)



//---------------------------------------------------------
// Methods for class MdamKey                              |
//---------------------------------------------------------
MdamKey::MdamKey(
     const ValueIdList& keyColumnIdList
     ,const ValueIdSet& operatorInputs
     ,const Disjuncts& associatedDisjuncts
     ,const ValueIdSet& nonKeyColumnSet
     ,const IndexDesc * indexDesc) :
  ScanKey(keyColumnIdList
          ,operatorInputs
          ,associatedDisjuncts
          ,nonKeyColumnSet
          ,indexDesc
          ),
     columnOrderListPtrArrayPtr_(NULL),
     stopColumnArray_(NULL),
     sparseFlagArray_(NULL),
     noExePred_(FALSE)

{
  // construct the stop column array (but only if
  // there are disjuncts)
  if ( getDisjuncts().entries() )
    {
      // The default stop column is the highest:
      stopColumnArray_ = new (CmpCommon::statementHeap())
	CollIndex[getDisjuncts().entries()];
      DCMPASSERT(stopColumnArray_ != NULL);
      setAllStopColumnsToMax();
    }

  // all columns are sparse
  sparseFlagArray_ = new (CmpCommon::statementHeap())
    NABoolean[getKeyColumns().entries()];
  DCMPASSERT(sparseFlagArray_ != NULL);
  setAllColumnsToSparse();

} // MdamKey::MdamKey(..)

MdamKey::~MdamKey()
{
  // No need to delete, MdamKey is allocated from HEAP

  NADELETEBASIC(stopColumnArray_,HEAP); // built in types
  NADELETEBASIC(sparseFlagArray_,HEAP);  // built in types
  if (columnOrderListPtrArrayPtr_ != NULL)
    {
      for (CollIndex i=0; i < columnOrderListPtrArrayPtr_->entries(); i++)
	delete (*columnOrderListPtrArrayPtr_)[i];
      delete columnOrderListPtrArrayPtr_;
    }

} // ~MdamKey

void MdamKey::setAllStopColumnsToMax( )
{
  for (CollIndex i=0; i < getDisjuncts().entries(); i++)
    {
      DCMPASSERT(getKeyColumns().entries() > 0);
      setStopColumn(i,getKeyColumns().entries()-1);
    }
}

void MdamKey::setAllColumnsToSparse()
{
  for (CollIndex i=0; i < getKeyColumns().entries(); i++)
    sparseFlagArray_[i] = TRUE;
}

void MdamKey::setStopColumn(CollIndex disjunctNumber,
		   CollIndex columnOrder)
{
  DCMPASSERT(disjunctNumber >= 0 AND disjunctNumber < getDisjuncts().entries());
  DCMPASSERT(columnOrder >= 0 AND columnOrder < getKeyColumns().entries());
  DCMPASSERT(stopColumnArray_ != NULL);
  stopColumnArray_[disjunctNumber] = columnOrder;
}

CollIndex MdamKey::getStopColumn(CollIndex disjunctNumber) const
{
  DCMPASSERT(disjunctNumber >= 0 AND disjunctNumber < getDisjuncts().entries());
  DCMPASSERT(stopColumnArray_ != NULL);
  return stopColumnArray_[disjunctNumber];
}

NABoolean MdamKey::getSparseFlag(CollIndex columnOrder) const
{
  DCMPASSERT(columnOrder >= 0 AND columnOrder < getKeyColumns().entries());
  DCMPASSERT(sparseFlagArray_ != NULL);
  return sparseFlagArray_[columnOrder];
}

void MdamKey::setSparseFlag(CollIndex columnOrder, NABoolean isSparse)
{
  DCMPASSERT(columnOrder >= 0 AND columnOrder < getKeyColumns().entries());
  DCMPASSERT(sparseFlagArray_ != NULL);
  sparseFlagArray_[columnOrder] = isSparse;
}

void MdamKey::reuseMdamKeyInfo(MdamKey * other)
{
  CollIndex numOfDisjuncts = (other->getDisjuncts()).entries();

  if ( numOfDisjuncts > 0 )
  {
    DCMPASSERT( numOfDisjuncts == getDisjuncts().entries() );
    for (CollIndex i=0; i<numOfDisjuncts; i++)
      setStopColumn(i, other->getStopColumn(i));
  }

  CollIndex numOfKeyColumns = (other->getKeyColumns()).entries();
  DCMPASSERT( numOfKeyColumns == getKeyColumns().entries() );

  for (CollIndex j=0; j<numOfKeyColumns; j++)
    setSparseFlag(j, other->getSparseFlag(j));

  if ( other->getNoExePred() )
	setNoExePred();
}

void MdamKey::preCodeGen(ValueIdSet& executorPredicates,
			 const ValueIdSet& selectionPredicates,
			 const ValueIdSet& availableValues,
			 const ValueIdSet& inputValues,
			 VEGRewritePairs * vegPairsPtr,
			 NABoolean replicateExpression,
                         NABoolean partKeyPredsAdded)
{

  //--------
  // The actions taken here are:
  // 1.- Generate the generator data structures depending on
  //     the access path case decided by the optimizer.
  //     While the data structures are being generated,
  //     any predicates in them are rewritten too (i.e.
  //     VEG predicates are transformed to real predicates).
  //
  // There are three access paths possible:
  //  (see mdam's compiler design doc for details)
  // 1.- Non-MDAM Common predicates
  // 2.- Mdam Common predicates
  // 3.- Disjuncts
  // Actions for access path 1:
  //  These actions do not correspond to MdamKey, they are
  //  done in FileScan::preCodeGen: (they need data from
  //  MdamKey however:
  //   1.1 Generate (and rewrite preds) SearchKey
  // Actions for access path 1 and 2:
  //   2.1 Generate ColumnOrderListArray (and rewrite its predicates)
  // Access path 1 & 2 look the same to the generator (as far as the
  // the generator is concerned, both ara MDAM access)
  // (The executor predicates are computed outside of MdamKey
  //  by FileScan)


  // 1.1 Generate (and rewrite preds) ColumnOrderListArray:
  columnOrderListPtrArrayPtr_ = new (CmpCommon::statementHeap())
    ColumnOrderListPtrArray;
  DCMPASSERT(columnOrderListPtrArrayPtr_ != NULL);
  // for every disjunct:
  ValueIdSet keyPredicates;
  Disjunct disjunct;
  ValueIdSet vegKeyNonKeyPreds;

  // Prepare the executor predicates:
  //getNoExePred() returns the value for noExePred_ identifying if
  //all the predicates has been applied at dp2 so no executor preds
  //are necessary
  //
  ValueIdSet keyColSet(getKeyColumns());
  ValueIdSet inSet(selectionPredicates);
  usePartofSelectionPredicatesFromTheItemExpressionTree(inSet,keyColSet.convertToBaseIds());
  Disjuncts *curDisjuncts = new HEAP MaterialDisjuncts(inSet);

  if (CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON &&
	    selectionPredicates.entries() &&
      !getNoExePred())
    {
      ValueIdSet temp;
      ItemExpr * inputItemExprTree = 
                      selectionPredicates.rebuildExprTree(ITM_AND,FALSE,FALSE);
      ItemExpr * resultOld = revertBackToOldTree(CmpCommon::statementHeap(),
                                                 inputItemExprTree);
      executorPredicates.clear();
      resultOld->convertToValueIdSet(executorPredicates, NULL, ITM_AND, FALSE);
      doNotReplaceAnItemExpressionForLikePredicates(resultOld,executorPredicates,resultOld);
//      executorPredicates.clear();
//      revertBackToOldTreeUsingValueIdSet(const_cast<ValueIdSet &>(selectionPredicates), executorPredicates);
      executorPredicates.replaceVEGExpressions
              (availableValues, inputValues,
               FALSE, // no need for key predicate generation here
               vegPairsPtr, replicateExpression);
    }
  else if( NOT getNoExePred())
    {
      executorPredicates = selectionPredicates;
      executorPredicates.replaceVEGExpressions
                  (availableValues, inputValues,
                   FALSE, // no need for key predicate generation here
                   vegPairsPtr, replicateExpression);
  }

  // transform VEGPreds and VEGRefs in the mdam key:

  // If the curDisjuncts have more than one disjunct and the chosen plan
  // only has 1 disjunct then the mdam common predicates case has occurred
  if (getDisjuncts().entries() == 1  AND  curDisjuncts->entries() > 1)
    {
      ValueIdSet commonPreds = curDisjuncts->getCommonPredicates();
      CMPASSERT(NOT commonPreds.isEmpty());
      delete curDisjuncts;
      curDisjuncts = new HEAP MaterialDisjuncts(commonPreds);
    }

  CollIndex i = 0;
  for (i=0; i < curDisjuncts->entries(); i++)
    {
      // Get key predicates from the disjunct:
      curDisjuncts->get(disjunct,i);
      appendKeyPredicates(keyPredicates,disjunct.getAsValueIdSet(), inputValues);

      // Compute VEG preds that contain
      // key columns as well as non-key column:
      replicateNonKeyVEGPredicates( keyPredicates,
                                    vegKeyNonKeyPreds
                                    );

      // Rewrite disjunct predicates:
      // enforce generation of key predicates by:
      // 1) Setting the generate key predicates flag
      keyPredicates.replaceVEGExpressions(availableValues,
                                          inputValues,
                                          TRUE,  // generate key predicates
                                          vegPairsPtr,
                                          replicateExpression, 
                                          NULL,
                                          NULL,
                                          getIndexDesc() // pass indexDesc
                                                         // so we can pick a 
                                                         // keyCol.
                                         );

      // Create & Populate the columnOrderList:
      ColumnOrderList *columnOrderListPtr =
        new (CmpCommon::statementHeap())
        ColumnOrderList(getKeyColumns());
      DCMPASSERT(columnOrderListPtr != NULL);
      columnOrderListPtr->append(keyPredicates);

      // Insert it into array:
      columnOrderListPtrArrayPtr_->insertAt(i,columnOrderListPtr);

      // prepare for nex round:
      keyPredicates.clear();
      disjunct.clear();

    } // for all disjuncts

  // --------------------------------------------------------------------
  //  To the optimizer, an empty key disjunct means "universally true",
  // i.e. it represents a full table scan. The optimizer can generate
  // empty disjuncts in many cases, especially when MDAM is forced.
  // An empty key disjunct means, in the optimizer, a universally true
  // predicate, i.e. a full table scan.,The generator
  // and executor already generate a full table scan for the case
  // of mdamkeys with a  *single*, empty disjunct. However, it does
  // not generate a full table scan for the case of multiple disjuncts
  // and at least one of them is empty.
  //
  // I added code in MdamKey::preCodeGen that detects whether there
  // are more than one disjunct in the mdamkey and one of them is
  // empty. In this case, an mdamkey with a single, empty, disjunct
  // is generated. Thus, the generator will generate the appropiate
  // full table scan.
  //
  // A warning in precode gen might be useful to indicate that
  // the MdamKey will perform a full table scan (not what the user
  // might have wanted).  This code *does not* print any
  // *official* warnings (but it prints a warning to the console).
  // --------------------------------------------------------------------

  // Note: The "for" loop below does two very important things:
  // 1. Detects empty disjuncts (as discussed above) 
  //
  // and, while it is doing that,
  //
  // 2. Sets the stop column for each disjunct.
  //
  // Note that if we don't set the stop column, then MDAM will traverse
  // *all* of the key columns, whether they have predicates or not, and
  // performance will be terrible.

  NABoolean isEmpty = FALSE;

    for (i=0; i < curDisjuncts->entries(); i++)
      {
        (*columnOrderListPtrArrayPtr_)[i]->setStopColumn(getStopColumn(i));

        // Find out if the disjunct has key preds:
        const ColumnOrderList& coList= *((*columnOrderListPtrArrayPtr_)[i]);
        if (!partKeyPredsAdded)
          isEmpty = TRUE; // assume it is empty
        for (CollIndex j=0; j < coList.entries(); j++)
          {
            if (coList[j] AND (NOT coList[j]->isEmpty()))
              {
                isEmpty=FALSE;
                break;
              }
          }
        if (isEmpty)
          {
            FSOWARNING("Empty disjunct");
            break;
          }
      } // set stop columns and also find out whether there is an empty disjunct

  if (isEmpty)
    {
      // There is an empty disjunct!

      // Create an empty columnOrderList:
      ColumnOrderList *columnOrderListPtr =
        new (CmpCommon::statementHeap())
        ColumnOrderList(getKeyColumns());
      DCMPASSERT(columnOrderListPtr != NULL);
      columnOrderListPtr->append(keyPredicates);
      // get rid of all other disjuncts:
      columnOrderListPtrArrayPtr_->clear();
      // Insert the new, empty, disjunct into array:
      columnOrderListPtrArrayPtr_->insertAt(0,columnOrderListPtr);
      // all predicates are executor predicates..
    }
  else
    {
    if(NOT getNoExePred()){
      // No empty disjuncts; generate appropiate executor
      // preds...
      //
      // common key predicates are not executor preds,
      // and some key preds must be replicated in executor
      // preds:

      // The key predicates (up to the MAX stop column)
      // common to all disjuncts can be safely
      // removed from the executor predicates:
      ValueIdSet commonKeyPredicates;
      getColumnOrderListPtrArray().
        computeCommonKeyPredicates(commonKeyPredicates);
      ColumnOrderList keyPredsByCol(getKeyColumns());
      keyPredsByCol.append(commonKeyPredicates);

      // compute MIN stop column
      CollIndex minStopColumn=getStopColumn(0);
      CollIndex i = 0;
      for (i=0; i < getKeyDisjunctEntries(); i++)
        {
	  CollIndex curStopColumn = getStopColumn(i);
	  if (curStopColumn < minStopColumn)
	    {
	      minStopColumn = curStopColumn;
	    }
        }

      // now remove all common key predicates that are
      // already in the mdam key:

      // Fix for  MDAM issue with wrong results
      // Soln. 10-100508-0134  
      // We can safely remove the common key predicates only 
      // until the minimum stop column as only these would be 
      // truly common to all disjuncts.
      // The earlier code was removing the common predicates until
      // the maximum stop column, if any of the disjuncts had a stop
      // column less than the max stop column, the common key predicate
      // cannot be removed from the executor predicates as it would not 
      // be used by the particular mdam scan disjunct.
      for (i=0; i<=minStopColumn; i++)
	{
          const ValueIdSet *predsPtr = keyPredsByCol[i];
          if (predsPtr AND NOT predsPtr->isEmpty())
            {
	      executorPredicates.remove(*predsPtr);
            }
        }

      // Now add the preds. resulting from VEG preds that contain
      // key columns as well as non-key columns
      // (for the case when:
      //  24:
      // VEGPred_24(VEG{T1.A(1),T1.D(4),T1.A(8),T1.D(11),T1A.A(15),
      // T1A.A(16),4(19)}
      // and we have key column t1.A and non-key column
      // T1.D the rewrite resolves
      // to:
      //60: (T1.A = 4) and (T1.D = 4)
      // Thus, if we don't do this, exe pred. is empty and we get
      // the wrong semantics
      vegKeyNonKeyPreds.replaceVEGExpressions
        (availableValues,
         inputValues,
         FALSE, // no need for key predicate generation here
         vegPairsPtr,
         replicateExpression
         );

      executorPredicates.insert(vegKeyNonKeyPreds);
    }
    } // mdamkey does not contain empty disjuncts
  delete curDisjuncts;

} // MdamKey::preCodeGen(...)



void MdamKey::
appendKeyPredicates(ValueIdSet& keyPredicates, const ValueIdSet& predicates,
                    const ValueIdSet& inputValues) const
{
  // for each predicate in predicates:
  for (ValueId predId = predicates.init();
       predicates.next(predId);
       predicates.advance(predId) )
  {
    // append it to keyPredicates if it is indeed a key predicate:
    NABoolean firstElemInRangeSpec = TRUE;
    if ( (CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON ) &&
	 ( predId.getItemExpr()->getOperatorType() == ITM_RANGE_SPEC_FUNC) )
    {
      ValueIdSet vdset;
      predId.getItemExpr()->child(1)->getLeafPredicates(vdset);
      for (ValueId predIdp = vdset.init();
	   vdset.next(predIdp);
	   vdset.advance(predIdp) )
      {
	// Let it flow through the all the predicates in the set to validate, in case there are any errors
	if (isAKeyPredicate(predIdp))
	{
		if (firstElemInRangeSpec){
	    keyPredicates.insert(predId.getItemExpr()->child(1)->castToItemExpr()->getValueId());
		break;
		}
	  else
	    {} // Do nothing, since already added as INLIST, getType should show as inlist
	  firstElemInRangeSpec = FALSE;
	}
      }
    }
    else
    {
      if (isAKeyPredicate(predId))
	keyPredicates.insert(predId);
      else if (isAPartKeyPredicateForMdam(predId, inputValues))
        keyPredicates.insert(predId);
    }
    // Old Code
    //if (isAKeyPredicate(predId))
    //keyPredicates.insert(predId);
  }

} // MdamKey::appendKeyPredicates(..)

NABoolean 
MdamKey::isAPartKeyPredicateForMdam(const ValueId& predId, const ValueIdSet& inputValues) const
{
  NABoolean retVal = FALSE;
  ValueId referencedInput = NULL_VALUE_ID;
  ValueId intervalExclusionExpr = NULL_VALUE_ID;
  const ValueIdList& keyCols = getKeyColumns();
  ValueIdSet operatorInputs(inputValues);

  operatorInputs += getOperatorInputs();

  for (CollIndex i=0; i < keyCols.entries(); i++)
    {
      const ValueId& col= keyCols[i];
      if (isAKeyPredicateForColumn(predId
                                   ,referencedInput
                                   ,intervalExclusionExpr
                                   ,col
                                   ,operatorInputs
                                   ))
        {
          retVal = TRUE;
          break;
        }
    }

  return retVal;
}



void MdamKey::print( FILE* ofd,
		    const char* indent,
		    const char* title) const
{
  // is print enabled?
  char *ev = getenv("MDAM_PRINT");
  if (ev != NULL AND strcmp(ev,"ON")==0)
    {

      fprintf(ofd,"%s\n",title);

      // Print the key columns:
      const ValueIdList& keyColumns = getKeyColumns();
      DCMPASSERT(NOT keyColumns.isEmpty());
      fprintf(ofd,"//--- Key Columns:\n");
      keyColumns.print(ofd,indent,"");
      fprintf(ofd,"\n\n");


      // now print the disjuncts for every column, if any:
      getDisjuncts().print(ofd,indent,"//--- disjuncts_:\n");
      fprintf(ofd,"\n");

      // print stop columns
      CollIndex i = 0;
      for (i=0; i < getDisjuncts().entries(); i++)
	fprintf(ofd,"stopColumnArray_[%d] = %d\n", i,
		getStopColumn(i));

      // print sparse flag:

      CollIndex maxStopColumn = getStopColumn(0);
      for (i=1; i < getDisjuncts().entries(); i++)
	{
	  if (maxStopColumn < getStopColumn(i))
	    maxStopColumn = getStopColumn(i);
	}
      for (i=0; i <= maxStopColumn; i++)
	fprintf(ofd,
		"Column: [%d] is %s\n",
		i,
		(isColumnSparse(i) ? "Sparse" : "Dense"));


      // print the key predicates by column:
      if (columnOrderListPtrArrayPtr_)
	{
	  columnOrderListPtrArrayPtr_->print();

	  // compute common key predicates:
	  ValueIdSet commonKeyPredicates;
	  columnOrderListPtrArrayPtr_->
	    computeCommonKeyPredicates(commonKeyPredicates);
	  commonKeyPredicates.print(ofd,indent,"Common key predicates: ");
	}
      else
	fprintf(ofd,"columnOrderListPtrArrayPtr_ is empty\n");
      fprintf(ofd,"\n");

    }   // is printing enabled?

} // MdamKey::print()



const ColumnOrderListPtrArray& MdamKey::getColumnOrderListPtrArray() const
{
  // array must have been generated in pre-code
  // gen to use it:
  DCMPASSERT(columnOrderListPtrArrayPtr_ != NULL);
  return *columnOrderListPtrArrayPtr_;
}

CollIndex MdamKey::getKeyDisjunctEntries() const
{
  DCMPASSERT(getDisjuncts().entries()>0);
  return getDisjuncts().entries();
}

void MdamKey::getKeyPredicatesByColumn(ColumnOrderList& keyPredsByCol,
				       CollIndex disjunctNumber) const
{
  // get predicates from disjunct
  Disjunct disjunct;
  NABoolean ok = getDisjuncts().get(disjunct,disjunctNumber);
  DCMPASSERT(ok); // the disjuct must exist

  keyPredsByCol.clearPredicates();  // to guarantee semantics

  getKeyColumnOrderList(keyPredsByCol,disjunct.getAsValueIdSet());

  // All columns return their predicate sets:
  // (The concrete class may change the stop column
  // depending on particular criteria. For instance,
  // for SearchKey we are interested only in orders
  // that looklike [=]^*[RANGE] (seen as a regular expression) )
  keyPredsByCol.setStopColumn(keyPredsByCol.entries()-1);

}// MdamKey::getKeyPredicatesByColumn(...)


//---------------------------------------------------------
// Methods for class Disjuncts                       |
//---------------------------------------------------------

void Disjuncts::computeCommonPredicates()
{
  // the common predicates are the intersection of all the disjuncts
  // in the local disjunct array:
  // they are used to compute the cost for non-mdam and mdam common
  // cases.
  if (entries() > 0) {

    DCMPASSERT(commonPredicates_.entries()==0);
    Disjunct disjunct;
    for (CollIndex i=0; i < entries(); i++)
    {
      get(disjunct,i);
      if(CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON )
      {
	 ValueIdSet inVidset = disjunct.getAsValueIdSet();
	 ValueIdSet outVidset,parsedVs;
         for (ValueId predId = inVidset.init();
	      inVidset.next(predId);
	      inVidset.advance(predId) )
	 {
            if(predId.getItemExpr()->getOperatorType() == ITM_RANGE_SPEC_FUNC )
            {
	       if(predId.getItemExpr()->child(1)->getOperatorType() == ITM_AND ){
	          predId.getItemExpr()->child(1)->convertToValueIdSet(parsedVs, NULL, ITM_AND, FALSE);
		    outVidset +=parsedVs;
            }
	    else if(predId.getItemExpr()->child(1)->getOperatorType() != ITM_AND 
			 && predId.getItemExpr()->child(1)->getOperatorType() != ITM_OR)
	       outVidset += predId.getItemExpr()->child(1)->castToItemExpr()->getValueId();	    
           }
	   else
	     outVidset +=predId;
	 parsedVs.clear();
	}

	if (i==0)
	  commonPredicates_.insert(outVidset);
	else
	  commonPredicates_.intersectSet(outVidset);
     }
     else
     {
	if (i==0)
	  commonPredicates_.insert(disjunct.getAsValueIdSet());
	else
	  commonPredicates_.intersectSet(disjunct.getAsValueIdSet());
     }
    }
  }
} // Disjuncts::computeCommonPredicates(..)

void Disjuncts::print( FILE* ofd,
			       const char* indent,
			       const char* title) const
{

  fprintf(ofd,"%s:\n",title);

  if (entries() > 0)
    {
      commonPredicates_.print(ofd,indent,"\n\nCommon predicates_");

      // print disjuncts:
      NAString disStr(CmpCommon::statementHeap());
      Disjunct disjunct;
      fprintf(ofd,"\n\nDisjuncts_ (one by one):\n");
      for (CollIndex i=0; get(disjunct,i); i++) {
	disStr =  NAString("Disjunct[") +
	  NAString((unsigned char)('0'+Int32(i))) + NAString("]: ");
	disjunct.print(ofd,indent,disStr);
	disjunct.clear();
      }
    }
  else
    fprintf(ofd,"No disjunct entries...\n");


} // print()

void Disjuncts::print() const
{
  print(stdout);
}

NABoolean Disjuncts::
get(Disjunct& disjunct, CollIndex i) const
{

  if (i < entries())
    {
      disjunct.clear();
      return TRUE;
    }
  else
    return FALSE;
}

DisjunctArray * Disjuncts::createEmptyDisjunctArray() const
{
  ValueIdSet *emptyDisjunctPtr = new HEAP ValueIdSet;
  DCMPASSERT(emptyDisjunctPtr != NULL);
  DisjunctArray *disjunctArrayPtr = new HEAP DisjunctArray(emptyDisjunctPtr);
  DCMPASSERT(disjunctArrayPtr != NULL);
  return disjunctArrayPtr;
}

NABoolean isOrItemExpr(ItemExpr* iePtr)
{
   return (iePtr && iePtr->getOperatorType() == ITM_OR);
}

NABoolean isAndOrItemExpr(ItemExpr* iePtr)
{
   return (iePtr &&
           (iePtr->getOperatorType() == ITM_AND || 
            iePtr->getOperatorType() == ITM_OR)
           )
           ;
}

NABoolean Disjuncts::containsSomePredsInRanges(funcPtrT funcP) const
{
//  CollIndex order;
//  CollIndex numOfKeyCols=keyPredsByCol.entries();
//  KeyColumns::KeyColumn::KeyColumnType typeOfRange = KeyColumns::KeyColumn::EMPTY;
  Disjunct disjunct;
  ValueIdSet vs;
  for (CollIndex i=0; i < entries(); i++)
  {
    this->get(disjunct,i);
    vs = disjunct.getAsValueIdSet();
    for (ValueId predId = vs.init();
	 vs.next(predId);
	 vs.advance(predId) )
    {
      if(predId.getItemExpr()->getOperatorType() == ITM_RANGE_SPEC_FUNC )
      {
	if ((*funcP)(predId.getItemExpr()->child(1)))
	  return TRUE;
      }
    }
  }
  return FALSE;
}

NABoolean Disjuncts::containsOrPredsInRanges() const
{
   return containsSomePredsInRanges(isOrItemExpr);
}

NABoolean Disjuncts::containsAndorOrPredsInRanges() const
{
   return containsSomePredsInRanges(isAndOrItemExpr);
  /*
  if (keyPredsByCol.containsPredicates())
  {
    for (order = 0; order < numOfKeyCols; order++)
    {
      if (keyPredsByCol.getPredicateExpressionPtr(order) != NULL)
        typeOfRange = keyPredsByCol.getPredicateExpressionPtr(order)->getType();
      else
      	typeOfRange = KeyColumns::KeyColumn::EMPTY;
      if (typeOfRange == KeyColumns::KeyColumn::INLIST )
	//  ||
	//  typeOfRange != KeyColumns::KeyColumn::RANGE)
	return TRUE;
    } // end of for-loop

  } // if (containsPredicates())
  */
  //return FALSE;
}


//---------------------------------------------------------
// Methods for class DisjunctsDisjuncts                             |
//---------------------------------------------------------
MaterialDisjuncts::MaterialDisjuncts(const ValueIdSet& selectionPredicates) :
     disjunctArrayPtr_(NULL)
{
  createDisjunctArray(selectionPredicates);

  // The semantics is such that at least one entry must be
  // created (even if selectionPredicates is empty, in this
  // case we have one disjunct array with exactly one entry:
  // the empty disjunct)
  DCMPASSERT(disjunctArrayPtr_->entries() >= 1);

  computeCommonPredicates();
}

MaterialDisjuncts::MaterialDisjuncts(Disjunct* disjunct) 
{
  disjunctArrayPtr_ = new HEAP 
           DisjunctArray(new HEAP ValueIdSet(disjunct->getAsValueIdSet()));

  DCMPASSERT(disjunctArrayPtr_->entries() == 1);
}

MaterialDisjuncts::~MaterialDisjuncts()
{
  // $$ Do we still need to delete?

  for (CollIndex i=0; i < disjunctArrayPtr_->entries(); i++)
    delete (*disjunctArrayPtr_)[i];

}


void MaterialDisjuncts::
createDisjunctArray(const ValueIdSet& selectionPredicates)
{
  // $$$ In the future we may want to fuse class DisjunctArray
  // $$$ and Disjuncts, especially when the DisjunctGlobal
  // $$$ is no longer needed.

  // Get the DisjunctArray built until now for the query
  DisjunctArray * prevDisjunctArray = NULL;
  DisjunctArray * currDisjunctArray = NULL;

  // ---------------------------------------------------------------------
  // Loop through the value ids in the ValueIdSet.  For each value id get
  // its ItemExpr node. Perform an mdamTreeWalk on the ItemExpr sub-tree
  // represented by this node.  A DisjunctArray built during this tree
  // walk is returned.  AND this DisjunctArray with the previous existing
  // DisjunctArray.  If no DisjunctArray existed before (is NULL) replace
  // it with this DisjunctArray.
  // ---------------------------------------------------------------------
  for (ValueId exprId = selectionPredicates.init();
       selectionPredicates.next(exprId);
       selectionPredicates.advance(exprId) )
    {
      currDisjunctArray = (exprId.getItemExpr())->mdamTreeWalk();
      if ( prevDisjunctArray == NULL )
	prevDisjunctArray = currDisjunctArray;
      else
	prevDisjunctArray = mdamANDDisjunctArrays( prevDisjunctArray,
						   currDisjunctArray );
    if (!prevDisjunctArray)
      break;
    } // loop thru the value ids in the ValueIdSet


  // If the previous disjunct array is NULL then the selections
  // predicates are empty. Thus create a disjunct array with
  // exactly one disjunct, the empty disjunct.
  // If it is not NULL then it means that the selection preds. contained
  // a pred tree and there was a disj. array created. Then,
  // set the disjunctArrayPtr:
  if (prevDisjunctArray)
    {
      // A disjunct array was created,
      // Does it have at least one entry?
      if (prevDisjunctArray->entries() == 0)
	{
	  // No.
	  // A disjunct array must have at least one entry:
	  // an empty set of value id's:
	  delete prevDisjunctArray;
	  prevDisjunctArray = createEmptyDisjunctArray();
	}
      // Set the ptr:
      disjunctArrayPtr_ = prevDisjunctArray;
    }
  else
    {
      if (CmpCommon::getDefault(COMP_BOOL_21) == DF_OFF)
        disjunctArrayPtr_ = new STMTHEAP DisjunctArray(new STMTHEAP ValueIdSet(selectionPredicates));
      else
        // create a disjunct array with the empty disjunct as it
        // sole entry:
        disjunctArrayPtr_ = createEmptyDisjunctArray();

    }


} // Disjuncts::createDisjunctArray(...)

NABoolean MaterialDisjuncts::
get(Disjunct& disjunct, CollIndex i) const
{

  NABoolean retVal = FALSE;
  if (Disjuncts::get(disjunct,i)) {
    disjunct.insertSet(*(*disjunctArrayPtr_)[i]);
    retVal = TRUE;
  }

  return retVal;

}// MaterialDisjuncts::get(...)



//---------------------------------------------------------
// Methods for class KeyColumn                            |
//---------------------------------------------------------
void KeyColumns::KeyColumn::insert(const ValueId &predicate)
{
  // Figure out the type of the predicate expression for
  // the column when the new
  // predicate is added:
  switch(getType())
    {
    case EMPTY:
      // if the column is EMPTY, then its new type
      // is whatever's type is the new predicate:
      if (predicate.getItemExpr()->getOperatorType() == ITM_VEG_PREDICATE)
       {
        VEG * predVEG = ((VEGPredicate *)(predicate.getItemExpr()))->getVEG();
        const ValueIdSet & VEGGroup = predVEG->getAllValues();
        if (VEGGroup.referencesConstExprCount() > 1)
          setType(CONFLICT_EQUALS);
        else
          setType(getType(predicate));
       }
      else
        setType(getType(predicate));

      break;

    case EQUAL:
      // we have a conflict (but maybe an EQUAL) in any case
      setType(CONFLICT_EQUALS);
      break;

    case RANGE:
      // If the column is already a range,
      // the only way that we cannot have a conflict is if
      // 1.- There is only one predicate in the column AND
      // 2.- the pred is of opposite
      // direction to the existing predicate in the column:
      // ELSE there is a conflict
      if (getPredicates().entries() == 1)
	{

	  OperatorTypeEnum newPredOperType =
	    predicate.getItemExpr()->getOperatorType();

	  ValueId predId = getPredicates().init();
	  getPredicates().next(predId);
	  OperatorTypeEnum exPredOperType =
	    predId.getItemExpr()->getOperatorType();

	  switch (newPredOperType)
	    {
	    case ITM_LESS:
	    case ITM_LESS_EQ:
	    case ITM_LESS_OR_LE:
	      if (exPredOperType == ITM_GREATER
		  OR
		  exPredOperType ==  ITM_GREATER_EQ
		  OR
		  exPredOperType ==  ITM_GREATER_OR_GE)
		{
		  setType(RANGE);
		}
	      else
		{
		  setType(CONFLICT);
		}
	      break;
	    case ITM_GREATER:
	    case ITM_GREATER_EQ:
	    case ITM_GREATER_OR_GE:
	      if (exPredOperType == ITM_LESS
		  OR
		  exPredOperType ==  ITM_LESS_EQ
		  OR
		  exPredOperType ==  ITM_LESS_OR_LE)
		{
		  setType(RANGE);
		}
	      else
		{
		  setType(CONFLICT);
		}
	      break;
	    case ITM_VEG_PREDICATE:
	    case ITM_EQUAL:
	      setType(CONFLICT_EQUALS);
	      break;
	    default:
	      // the new predicate for this column is incompatible
	      // with a range (i.e. old pred: B > 2, new predicate:
	      // B=3
	      setType(CONFLICT);

	    } // inner switch
	} // if there is one predicate
      else
	{
	  // There is already a full range (i.e. A > 3 and A < :hv)
	  // The only outcome is CONFLICT:
	  if (getType(predicate) == EQUAL)
	    {
	      setType(CONFLICT_EQUALS);
	    }
	  else
	    {
	      setType(CONFLICT);
	    }
	}
      break;

    case IS_NULL:
      // else if the column is NULL then if the new operator
      // is not null we are in a conflict
      if (getType(predicate) != IS_NULL)
	{
	  setType(CONFLICT);
	}

      break;

    case INLIST:
	// else if the column is INLIST and if the new operator
	// is not INLIST then it must be a CONFLICT (am I getting philosophical
	// here?)
      if (getType(predicate) == EQUAL)
	{
	  setType(CONFLICT_EQUALS);
	}
      else
	{
	  setType(CONFLICT);
	}
      break;

    case CONFLICT_EQUALS:
    case CONFLICT:
      // it stays conflict
      break;
    default:
      // The column MUST be EMPTY, EQUAL, INLIST, IS_NULL, RANGE, etc
      DCMPASSERT(FALSE);
    } // switch getType(predicate)

  // Now insert the new predicate:
  predicatesForColumn_.insert(predicate);

} // KeyColumns::KeyColumn::insert(const ValueId &predicate)

KeyColumns::KeyColumn::KeyColumnType
KeyColumns::KeyColumn::getType(const ValueId& predId) const
{
  switch(predId.getItemExpr()->getOperatorType())
    {
    case ITM_VEG_PREDICATE:
    case ITM_EQUAL:
      return EQUAL;
    case ITM_LESS:
    case ITM_LESS_EQ:
    case ITM_LESS_OR_LE:
    case ITM_GREATER:
    case ITM_GREATER_EQ:
    case ITM_GREATER_OR_GE:
      return RANGE;
    case ITM_IS_NULL:
      return IS_NULL;
    case ITM_IN:
    case ITM_OR:// RangeSpecRef Item Expression Handling
      return INLIST;
    case ITM_ASSIGN:
      return ASSIGN;
    default:
      CMPABORT; // unsupported predicate
      break;
    }
  return INLIST; // to keep the compiler happy, fix this!
} // getType(predId)

void KeyColumns::KeyColumn::print( FILE* ofd,
				   const char* indent,
				   const char* /*title*/ ) const
{
  char message[80];

  switch(getType())
    {
    case EMPTY:
      strcpy(message,"EMPTY");
      break;
    case EQUAL:
      strcpy(message,"EQUAL");
      break;
    case RANGE:
      strcpy(message,"RANGE");
      break;
    case INLIST:
      strcpy(message,"INLIST");
      break;
    case CONFLICT:
      strcpy(message,"CONFLICT");
      break;
    case CONFLICT_EQUALS:
      strcpy(message,"CONFLICT_EQUALS");
      break;
    case ASSIGN:
      strcpy(message,"ASSIGN");
      break;
    default:
      strcpy(message,"Unknown type...");
      break;
    }

  // print predicates:
  fprintf(ofd,"The type of this column is: [%s]\n"
	  "and its predicates are:\n",
	  message);
  getPredicates().print(ofd,indent,"");
  fprintf(ofd,"\n");
}


void KeyColumns::KeyColumn::clearPredicates()
{
  // -----------------------------------------------------------------------
  // Remove all predicates
  // -----------------------------------------------------------------------
    predicatesForColumn_.clear();

    // -----------------------------------------------------------------------
    // And reset the type:
    // -----------------------------------------------------------------------
    setType(EMPTY);
} // clearPredicates()

// Resolving a conflict means picking some predicate that
// can be used as a key predicate among several conflicting
// key preds for the same column. For instance A < 2 and A < 3
// are conflicting predicates. Resolve conflict will pick any
// one of them (in this case we may like to get A < 2, but
// this is difficult to do in the optimizer)
void KeyColumns::KeyColumn::resolveConflict()
{

  if (getType() == CONFLICT_EQUALS OR
      getType() == CONFLICT)
    {
      // ------------------------------------------------------------------
      // Try to pick an equal, if there is one:
      // ------------------------------------------------------------------

      const ValueIdSet& preds = getPredicates();
      ValueId pred; // outside the loop so it lives until it is inserted...
      ValueId *chosenPredPtr = NULL;

      for (pred = preds.init();
	   preds.next(pred);
	   preds.advance(pred))
	{
	  if (pred.getItemExpr()->getOperatorType() == ITM_EQUAL
	      OR
	      pred.getItemExpr()->getOperatorType() == ITM_VEG_PREDICATE)
	    {
	      chosenPredPtr = &pred;
	      break; // important!
	    }
	} // for


      // -----------------------------------------------------------
      // If we did not find the equal, pick any other that makes
      // sense for a search key (i.e. not a IN, etc):
      // $$$ Should we pick the join predicate if there is one?
      // -----------------------------------------------------------
      if (chosenPredPtr == NULL)
	{
	  NABoolean predFound = FALSE;
	  ValueId *equalPredPtr = NULL;
	  for (pred = preds.init();
	       chosenPredPtr == NULL AND preds.next(pred);
	       preds.advance(pred))
	    {
	      switch (pred.getItemExpr()->getOperatorType())
		{
		case ITM_LESS:
		case ITM_LESS_EQ:
		case ITM_LESS_OR_LE:
		case ITM_GREATER:
		case ITM_GREATER_EQ:
		case ITM_GREATER_OR_GE:
		  chosenPredPtr = &pred;
		  break;
		}
	      if (chosenPredPtr != NULL)
		{
		  break; // important!
		}
	    } // for
	}

      // There must be a useful predicate in a conflict:
      DCMPASSERT(chosenPredPtr != NULL);

      // --------------------------------------------------------
      //  Clear all predicates:
      // --------------------------------------------------------
      clearPredicates();

      // --------------------------------------------------------------
      //  Insert back the chosen predicate:
      // -------------------------------------------------------------
      insert(*chosenPredPtr);


    }
} // resolveConflict(...)


//---------------------------------------------------------
// Methods for class KeyColumns                           |
//---------------------------------------------------------

KeyColumns::KeyColumns() :
     keyColumnPtrCache_(HEAP)
{}

KeyColumns::KeyColumns(const ValueIdList& orderList) :
     keyColumnPtrCache_(HEAP)
{
  for (CollIndex i=0; i < orderList.entries(); i++)
    insertColumn(orderList[i]);
}

void KeyColumns::insertColumn(const ValueId& column)
{

  // don't add it if it's already there!
  NABoolean alreadyIn = FALSE;
  for(CollIndex i=0; i < keyColumnPtrCache_.entries(); i++)
    {
      if (column == keyColumnPtrCache_[i]->getColumnId())
	{
	  alreadyIn = TRUE;
	  break;
	}
    }

  if (NOT alreadyIn)
    {
      KeyColumn *keyColumnPtr = new (CmpCommon::statementHeap())
	KeyColumn(column);
      DCMPASSERT(keyColumnPtr != NULL);
      keyColumnPtrCache_.insertAt(keyColumnPtrCache_.entries(),
				  keyColumnPtr);
      columnSet_.insert(column);
    }

} // KeyColumns::insertColumn(..)


void KeyColumns::append(const ValueIdSet& andPredicateExpression)

{
  // there must be columns in the keyColumns:
  DCMPASSERT(NOT isEmpty());


  // Add each predicate in this disjunct into
  // the appropiate column of the cache:
  ItemExpr *iePtr=NULL;
  for (ValueId predId = andPredicateExpression.init();
       andPredicateExpression.next(predId);
       andPredicateExpression.advance(predId) )
    {
      insertPredicate(predId);
    } // for every predicate

} // KeyColumns::fill(..)


KeyColumns::~KeyColumns()
{
  clear();
} // KeyColumns::~KeyColumns()

const KeyColumns::KeyColumn&
KeyColumns::getKeyColumn(const ValueId& column) const
{
  DCMPASSERT(NOT keyColumnPtrCache_.isEmpty());

  KeyColumns::KeyColumn* result = NULL;

  // look up column in the cache and return the set:
  for (CollIndex i=0; i < keyColumnPtrCache_.entries(); i++)
    {
      if (column == keyColumnPtrCache_[i]->getColumnId())
	{
	  result = keyColumnPtrCache_[i];
	  break;
	}
    }
  if (result == NULL)
    DCMPASSERT(FALSE);
  return *result;
}

KeyColumns::KeyColumn*
KeyColumns::getKeyColumnPtr(const ValueId& column)
{  // The cache must have columns and predicates:

  DCMPASSERT(NOT keyColumnPtrCache_.isEmpty());

  KeyColumns::KeyColumn* result = NULL;

  // look up column in the cache and return the set:
  for (CollIndex i=0; i < keyColumnPtrCache_.entries(); i++)
    {
      if (column == keyColumnPtrCache_[i]->getColumnId())
	{
	  result = keyColumnPtrCache_[i];
	  break;
	}
    }

  DCMPASSERT(result != NULL);
  return result;

} // KeyColumns::getKeyColumn()

const ValueIdSet&
KeyColumns::getPredicatesForColumn(const ValueId& column) const
{
    return getKeyColumn(column).getPredicates();

} // KeyColumns::getPredicatesForColumn(..)

void KeyColumns::getAllPredicates(ValueIdSet& allPredicates) const
{
  // (i.e. predicates in every column referenced in the list):
  for (ValueId column = columnSet_.init();
       columnSet_.next(column);
       columnSet_.advance(column))
    {
      allPredicates.insert(getPredicatesForColumn(column));
    }
}


void KeyColumns::insertPredicateInColumn(const ValueId& predicate,
                                         const ValueId& column)
{
  insertPredicate(predicate, &column);
}

void KeyColumns::insertPredicate(const ValueId& predicate)
{
  insertPredicate(predicate,NULL);
}

void KeyColumns::insertPredicate(const ValueId& predicate,
                                 const ValueId* columnPtr)
{


  ItemExpr * iePtr = predicate.getItemExpr();

  // The key must be formed only of predicates, or
  // in the case of an insert statament, of ASSIGN expressions;
  // in the case of a TriRelational predicate: ITM_LESS(GREATER)_OR_LE(GE)

  DCMPASSERT((predicate.getItemExpr()->isAPredicate())
             OR
             (predicate.getItemExpr()->getOperatorType()==ITM_ASSIGN)
             OR
             (predicate.getItemExpr()->getOperatorType()==ITM_LESS_OR_LE)
             OR
             (predicate.getItemExpr()->getOperatorType()==ITM_GREATER_OR_GE)
	     OR 
	     (predicate.getItemExpr()->getOperatorType()==ITM_OR) // For RangeSpecRef Item Expression
             );

  // The ValueId for the column must come from a column:
  DCMPASSERT(NOT columnPtr
            OR
            ((columnPtr->getItemExpr()->getOperatorType()==ITM_BASECOLUMN)
             OR
             (columnPtr->getItemExpr()->getOperatorType()==ITM_INDEXCOLUMN)) );


  // Is it a valid key predicate?

  switch (iePtr->getOperatorType())
    {
    case ITM_VEG_PREDICATE:
    case ITM_EQUAL: // this are created when = is under an OR
    case ITM_NOT_EQUAL: 
    case ITM_LESS:
    case ITM_LESS_EQ:
    case ITM_LESS_OR_LE:
    case ITM_GREATER:
    case ITM_GREATER_EQ:
    case ITM_GREATER_OR_GE:
    case ITM_IN:
    case ITM_ASSIGN:
    case ITM_IS_NULL:
    case ITM_AND: // this will get cleaned up below...
    case ITM_OR: // Changes for RangeSpecRef Item Expression
      // continue processing
      break;
    default:
      DCMPASSERT(FALSE);  // invalid predicate
    }

  // Some veggies can be rewritted to preds like 2 = 7
  // when the query contains contradictory predicates (as
  // in the query below
  //select * from t1 where (a=2 and b=3)and (d=4 OR a=7 OR B> 1 AND B <= 45);
  // If there is a contradictory pred like 2 = 7 throw away the whole
  // disjunct:
  if (predicate.getItemExpr()->getArity() == 2)
    if ( (predicate.getItemExpr()->getConstChild(0)->
	  castToItemExpr()->getOperatorType() == ITM_CONSTANT)
	 AND
	 (predicate.getItemExpr()->getConstChild(1)->
	  castToItemExpr()->getOperatorType() == ITM_CONSTANT) )
      DCMPASSERT(FALSE); // $$$ How do we handle CONST OPERATOR CONST???

  // Because of VEG rewriting, the predicate may look like:
  // A=2 AND B=2, parse it if so:
  if (predicate.getItemExpr()->getOperatorType() == ITM_AND)
    { // the AND expression can be very complex, in general:
     insertPredicate(predicate.getItemExpr()->getConstChild(0)->
		     castToItemExpr()->getValueId()
                     ,columnPtr
                     );
     insertPredicate(predicate.getItemExpr()->getConstChild(1)->
		     castToItemExpr()->getValueId()
                     ,columnPtr
                     );
    }
  else // it's an appropiate pred, insert it in the appropiate column:
    {
      for (CollIndex i=0; i < keyColumnPtrCache_.entries(); i++)
	{

          // if columnPtr is not NULL, then it means that we
          // need to insert the predicate in the given column:
          if (columnPtr != NULL)
            {
              // insert it into the given column:
              NABoolean found = FALSE;
              if (keyColumnPtrCache_[i]->getColumnId() == *columnPtr)
                {
                  keyColumnPtrCache_[i]->insert(predicate);
                }
            }
          else
            {
              // insert it in all columns that it references:
              // does it refer to the curgrent column?
              // It does if it references an index column or if it references
              // the base table column corresponding to an index column:
              ItemExpr *refExpr = predicate.getItemExpr();

              if (refExpr->getOperatorType() == ITM_ASSIGN)
                refExpr = refExpr->child(0);

              if (refExpr->referencesTheGivenValue(
                       keyColumnPtrCache_[i]->getColumnId())
                  OR
                  refExpr->referencesTheGivenValue(
                       ((IndexColumn *)(keyColumnPtrCache_[i]->
                                        getColumnId().getItemExpr()))->
                       getDefinition() ) )

                {
                  // now that we found the column, append the predicate:
                  keyColumnPtrCache_[i]->insert(predicate);
                  // don't break, VEG predicates may refer to several
                  // columns!
                }
            } // columnPtr == NULL
	} // for..
    }
  // The predicate may not be inserted if we try to insert an
  // AND expression of the form
  //  60: (T1.A = 4) and (T1.D = 4)
  // and T1.A is a key col. but T1.D is not a key col.

} // KeyColumns::insertPredicate(..)


void KeyColumns::clear()
{
  // Remove keyColumns & predicates
  if (NOT keyColumnPtrCache_.isEmpty())
    {
      // remove the KeyColumn instances that were allocated
      // using new (by buildCacheColumns())
      clearPredicates();
      // remove the entries of the cache itself:
      keyColumnPtrCache_.clear();
    }

} // clear

void KeyColumns::clearPredicates()
{
  if (NOT keyColumnPtrCache_.isEmpty())
    {
      // remove the KeyColumn instances that were allocated
      // using new (by buildCacheColumns())
      for (CollIndex i=0; i < keyColumnPtrCache_.entries(); i++)
	keyColumnPtrCache_[i]->clearPredicates();
    }
}

void KeyColumns::print( FILE* ofd,
			const char* indent,
			const char* title) const
{


  // print list:
  fprintf(ofd,title);
  for(CollIndex i=0; i < keyColumnPtrCache_.entries(); i++)
    {


      // print the column
      fprintf(ofd,"Predicates for column: ");
      Int32 c = (Int32)((CollIndex) (keyColumnPtrCache_[i]->getColumnId()));
      NAString unparsed(CmpCommon::statementHeap());
      keyColumnPtrCache_[i]->getColumnId().getItemExpr()->unparse(unparsed);
      fprintf(ofd,"%4d: %s\n",c,(const char *) unparsed);

      // print the predicates for this column:
      if ( keyColumnPtrCache_[i]->getPredicates().isEmpty())
	fprintf(ofd,"\nNo predicates for this column\n\n");
      else
	{
	keyColumnPtrCache_[i]->print(ofd,indent,"");
	}


    } // for every column

} // KeyColumns::print


// -----------------------------------------------------------------------
// methods for ColumnOrderList:
// -----------------------------------------------------------------------
//

ValueId ColumnOrderList::getKeyColumnId(CollIndex order) const
{
  if (order >= entries() || orderKeyColumnPtrList_[order] == NULL)
    return NULL_VALUE_ID;

  return orderKeyColumnPtrList_[order]->getColumnId();
}


NABoolean ColumnOrderList::containsPredicates() const
{
  // for every order:
  // check if there are no predicates:
  NABoolean containsPreds = FALSE;
  for (CollIndex order = 0; order < entries();  order++)
    {
      // Exit the loop as soon as we hit an expression that
      // it's not EMPTY
      if (getPredicateExpressionPtr(order) AND
	  getPredicateExpressionPtr(order)->getType() != KeyColumn::EMPTY)
	{
	  containsPreds = TRUE;
	  break;
	}
    }

  return containsPreds;
} // containsPredicates()

void ColumnOrderList::print( FILE* ofd,
			     const char* indent,
			     const char* /*title*/) const
{
  // This prints the predicates for columns that are valid
  fprintf(ofd,"The mdam columns are (missing trailing key columns were ruled out by the optimizer):\n ");
  for (CollIndex order=0; order < entries(); order++)
    {
      // print the column
      if (orderKeyColumnPtrList_[order])
      {
        fprintf(ofd,"Predicates for column: ");
        Int32 c = (Int32)((CollIndex) (orderKeyColumnPtrList_[order]->getColumnId()));
        NAString unparsed(CmpCommon::statementHeap());
        orderKeyColumnPtrList_[order]->
  	  getColumnId().getItemExpr()->unparse(unparsed);
        fprintf(ofd,"%4d: %s\n",c,(const char *) unparsed);

        // print the predicates for this column:
        if ( (*this)[order] == NULL )
    	{
  	  fprintf(ofd,"\nThere are no predicates for this column");
	}
        else
  	{
  	  getPredicateExpressionPtr(order)->print(ofd,indent,"");
  	}
      }
  }
} //  ColumnOrderList::print(...)



ColumnOrderList::ColumnOrderList(const ValueIdList& listOfColumns):
     KeyColumns(listOfColumns),
     columnList_(listOfColumns),
     orderKeyColumnPtrList_(HEAP)
{
  // Create the orde list and validate all orders to TRUE:
  for(CollIndex i=0; i < listOfColumns.entries(); i++)
    {
      orderKeyColumnPtrList_.insert(NULL);
      validateOrder(i);
    }
} // ColumnOrderList(...)

void ColumnOrderList::validateOrder(CollIndex order)
{
  DCMPASSERT(order >= 0 && order < columnList_.entries());
  orderKeyColumnPtrList_[order] = getKeyColumnPtr(columnList_[order]);
}

void ColumnOrderList::invalidateOrder(CollIndex order)
{
  DCMPASSERT(order >= 0 && order < columnList_.entries());
  orderKeyColumnPtrList_[order] = NULL;
}



void ColumnOrderList::invalidateAllColumns()

{
  // All columns up to (and including) stopColumn are invalid:
  for (CollIndex order=0; order < entries(); order++)
      invalidateOrder(order);
}

const ColumnOrderList::KeyColumn* ColumnOrderList::
getPredicateExpressionPtr(CollIndex order) const
{
  DCMPASSERT(order >= 0 && order < orderKeyColumnPtrList_.entries());
  if (orderKeyColumnPtrList_[order] == NULL)
    return NULL;
  else
    return orderKeyColumnPtrList_[order];
}

const ValueIdSet* ColumnOrderList::operator[] (CollIndex order) const
{
  const KeyColumn *kc =  getPredicateExpressionPtr(order);
  return ( kc == NULL ? NULL : &(kc->getPredicates()) );
}

void ColumnOrderList::setStopColumn(CollIndex stopColumn)

{
  // All columns up to (and including) stopColumn are valid:
  CollIndex order = 0;
  for (order=0; order <= stopColumn; order++)
      validateOrder(order);
  // The rest are invalid (i.e. they return a NULL
  // pointer when operator[] is used on the columnOrderList)
  for (; order < entries(); order++)
    invalidateOrder(order);
}


void ColumnOrderList::resolveConflict(CollIndex order)
{
  DCMPASSERT(order >= 0 && order < orderKeyColumnPtrList_.entries());
  if (orderKeyColumnPtrList_[order] != NULL)
    {
      orderKeyColumnPtrList_[order]->resolveConflict();
    }
} // resolveConflict(CollIndex order)


// -----------------------------------------------------------------------
// Methods for ColumnOrderListPtrArray
// -----------------------------------------------------------------------
// Function and comments were rewritten based on original version
// Created:      //96
// Language:     C++
// Modified:     July 2000
//
// -----------------------------------------------------------------------
// We'll call a "single subset" a set of table rows that could be
// retrieved by MDAM without extra probes(between two probes).
// Suppose, a table is ordered by the list of colums: C0, C1,..., Cn.
// called "column order list". Let Pj be a set of predicates for the
// column Cj. If there is no predicate on the column (Pj is EMPTY)
// we assume that all rows satisfy this predicate.
//
// The single subset order (SSO) for a column order list is the MAXIMUM
// column position j: 0<=j<=n such that the set of table rows selected
// according to all predicates P0,P1,...,Pj could be a single subset.
//
// For instance, given a table ordered by A B C and an expression B=5
// as the predicate set. Since there is no predicates on column A or
// P0 is EMPTY then applying P0 results in a single subset: the whole
// table.

// A B C
// 1 5 4
// 1 6 4
// 6 7 8
// 7 5 9
//
// Applying the next column predicate B = 5 (P1 is EQUAL) results in
// two subsets:
// A B C
// ==========
// 1 5 4      first subset
// ==========
// 1 6 4
// 6 7 8
// ==========
// 7 5 9      second subset
// ==========
// So, in this case SSO is 0.
//
// However if the preds are: A = 1 and B >= 5 and B <= 6, then
// the SSO is 1 (corresponding to col. B)
// A B C
//===========
// 1 5 4       single subset
// 1 6 4
//===========
// 6 7 8
// 7 5 9
//
// Note that SSO should be 0 in case when there is no single subset.
//
// The function returns true if the pred. expression in the column order
// list retrieves a single subset (the whole table is a trivial case of
// a single subset), or FALSE if it isn't ( when the first pericate is
// IN pred, like A IN (2,7,8)
// NOTE. The cases when pred. is CONFLICT or CONFLICT EQUAL needs to be
// investigated more carefully before including them in this function.
// -----------------------------------------------------------------------
// Sample cases:	sinSubOrder   itIsSingleSubset
// 1.- = = ... =	    n		    TRUE
// 2.- = = R		    1		    TRUE
// 3.- R		    0		    TRUE
// 4.- EMPTY		    0		    TRUE
// 5.- IN		    0		    FALSE
// 6.- = = IN		    1		    TRUE
// 7.- = EMPTY		    0		    TRUE
// -----------------------------------------------------------------------

NABoolean
ColumnOrderList::getSingleSubsetOrder(CollIndex& sinSubOrder /* out */) const
{
  NABoolean itIsSingleSubset = TRUE;
  CollIndex order;
  CollIndex numOfKeyCols=entries();
  KeyColumns::KeyColumn::KeyColumnType typeOfRange = KeyColumns::KeyColumn::EMPTY;

  if (containsPredicates())
  {
    for (order = 0; order < numOfKeyCols; order++)
    {
      if (getPredicateExpressionPtr(order) != NULL)
        typeOfRange = getPredicateExpressionPtr(order)->getType();
      else
      	typeOfRange= KeyColumns::KeyColumn::EMPTY;

      // Exit the loop as soon as we hit an expression that
      // it's not an EQUAL:
      if (typeOfRange != KeyColumns::KeyColumn::EQUAL)
        break;
    } // end of for-loop

    // not a single subset when the first pred. is an IN
    if ( order == 0 AND
         typeOfRange == KeyColumns::KeyColumn::INLIST)
    {
      itIsSingleSubset=FALSE;                             // Case 5.
    }

    // if All preds were EQUAL, or the last pred. was IN or EMPTY,
    // we need to substract one, because the end of the
    // single subset actually was reached on previous step
    if ( order == numOfKeyCols OR                         // Case 1.
         typeOfRange == KeyColumns::KeyColumn::INLIST OR  // Case 6.
         typeOfRange == KeyColumns::KeyColumn::EMPTY )    // Case 4,7.
    {
      if ( order < 1 )
	sinSubOrder=0;
      else
	sinSubOrder=order-1;
    }
    else
    {
      // this includes RANGE, CONFLICT and CONFLICT EQUAL
      sinSubOrder=order;                                  // Case 2,3.
    }
  } // if (containsPredicates())
  else
  {
    // single subset by definition when there is no predicates
    sinSubOrder=0;
  }
  return itIsSingleSubset;

} // getSingleSubsetOrder(CollIndex& sinSubOrder)
NABoolean
ColumnOrderList::getSingleSubsetOrderForMdam(CollIndex& sinSubOrder /* out */) const
{
  NABoolean itIsSingleSubset = TRUE;
  CollIndex order;
  CollIndex numOfKeyCols=entries();
  KeyColumns::KeyColumn::KeyColumnType typeOfRange = KeyColumns::KeyColumn::EMPTY;

  if (containsPredicates())
  {
    for (order = 0; order < numOfKeyCols; order++)
    {
      if (getPredicateExpressionPtr(order) != NULL)
        typeOfRange = getPredicateExpressionPtr(order)->getType();
      else
      	typeOfRange= KeyColumns::KeyColumn::EMPTY;
		if (typeOfRange != KeyColumns::KeyColumn::EQUAL
		  &&
		  typeOfRange != KeyColumns::KeyColumn::RANGE)
        break;
    } // end of for-loop

    // not a single subset when the first pred. is an IN
    if ( order == 0 AND
         typeOfRange == KeyColumns::KeyColumn::INLIST)
    {
      itIsSingleSubset=FALSE;                             // Case 5.
    }

    // if All preds were EQUAL, or the last pred. was IN or EMPTY,
    // we need to substract one, because the end of the
    // single subset actually was reached on previous step
    if ( order == numOfKeyCols OR                         // Case 1.
         typeOfRange == KeyColumns::KeyColumn::INLIST OR  // Case 6.
         typeOfRange == KeyColumns::KeyColumn::EMPTY )    // Case 4,7.
    {
      if ( order < 1 )
	sinSubOrder=0;
      else
	sinSubOrder=order-1;
    }
    else
    {
      // this includes RANGE, CONFLICT and CONFLICT EQUAL
      sinSubOrder=order;                                  // Case 2,3.
    }
  } // if (containsPredicates())
  else
  {
    // single subset by definition when there is no predicates
    sinSubOrder=0;
  }
  return itIsSingleSubset;

} // getSingleSubsetOrderForMdam(CollIndex& sinSubOrder)

// -----------------------------------------------------------------------
// Methods for ColumnOrderListPtrArray
// -----------------------------------------------------------------------

ColumnOrderListPtrArray::ColumnOrderListPtrArray():
     ARRAY(ColumnOrderList *)(HEAP)
{}


void ColumnOrderListPtrArray::
computeCommonKeyPredicates(ValueIdSet& commonKeyPredicates) const
{
  DCMPASSERT(commonKeyPredicates.isEmpty());
  ValueIdSet predicates;
  for (CollIndex i=0; i < entries(); i++)
    {
      if (i==0)
	{
	  // gather all key predicates
	  ((*this)[i])->getAllPredicates(commonKeyPredicates);
	}
      else // intersect with all predicates in all the orders:
	{
	  ((*this)[i])->getAllPredicates(predicates);
	  commonKeyPredicates.intersectSet(predicates);
	  predicates.clear(); // will be reused in the next round
	}
    } // for all disjuncts
} // ColumnOrderListPtrArray::computeCommonKeyPredicates(..)



void ColumnOrderListPtrArray::print( FILE* ofd,
				     const char* indent,
				     const char* /*title*/) const
{

  char cstrDisjunct[100];
  for (CollIndex i=0; i < entries(); i++)
    {
      sprintf(cstrDisjunct,"Predicates by column for disjunct %d\n",i);
      (*this)[i]->print(ofd,indent,cstrDisjunct);
    }

}

const ValueIdSet ScanKey::getAllColumnsReferenced() const
{
  ValueIdSet all(getKeyColumns());  
  all.addSet(getNonKeyColumnSet());
  all.addSet(getExecutorPredicates());
  return all;
}


// eof
