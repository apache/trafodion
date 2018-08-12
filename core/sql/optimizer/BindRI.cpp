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
* File:         BindRI.C
* Description:  Binding Referential Integrity Constraints
* Language:     C++
*
*
*	Many things, having full reference
*	To one concent, may work contrariously:
*	As many arrows, loosed several ways,
*	Fly to one mark;
*	As many several ways meet in one town;
*	As many fresh streams meet in one salt sea;
*	As many lines close in the dial's centre:
*	So may a thousand actions, once afoot,
*	End in one purpose, and be all well borne
*	Without defeat.
*	        -- Canterbury, in Henry V (I, ii, 205)
*
******************************************************************************
*/

#if 0	// documentation
/*
  GenericUpdate::bindNode will probably call these methods something like this:

	RefConstraintList referencedByMe;
	RefConstraintList referencingMe;
	if (INSERT || UPDATE)
	  naTable->getRefConstraints().getRefConstraints(
	  				 bindWA, newRecExpr(), referencedByMe);
	if (DELETE || UPDATE)
	  naTable->getUniqueConstraints().getRefConstraints(
	  				 bindWA, newRecExpr(), referencingMe);

	if (!referencedByMe.entries() && !referencingMe.entries())
	  return	// no RI constraints, nothing to do
	if (bindWA-errStatus())
	  return	// binder will have already put diags into diagsArea

	u = new Union
	for (i in referencedByMe)
	  makeScanTree(bindWA, u, referencedByMe[i])
	for (i in referencingMe)
	  makeScanTree(bindWA, u, referencingMe[i])

	make TSJ, attach Union u and original GenericUpdate node, etc etc

  where makeScanTree() is a new function that makes a Scan from the RI info
  passed in and attaches it to the union.  Something like:

  	the table to be scanned comes from RefConstraint::getOtherTableName

	the scan predicate comes from RefConstraint::getPredicateText
	  e.g. "(fk1,fk2)=(uc1,uc2)"

	you can tell whether to use "EXISTS" or "NOT EXISTS" by looking at
	  RefConstraint::isaForeignKeyinTableBeingUpdated
	  or if that's too wordy, use its converse,
	  RefConstraint::referencesTableBeingUpdated

	call parser on some of this text and build rest of tree yourself, etc
*/
#endif // documentation

#include "TrafDDLdesc.h"
#include "BindWA.h"
#include "ItemConstr.h"
#include "NATable.h"

/*static*/ void Constraint::makeColSignature(const ValueIdSet &assigns,
					     ColSignature &outsig)
{
	//## navigate assigns to find target cols,
	//## insert each col's position into the signature

	//## signature is a bitvector of size <natable.nacolarray.entries>
	//## or (to save a little space) size <highest col position + 1>
	//## depending on how the bitvec is implemented
} // Constraint::makeColSignature

AbstractRIConstraint::~AbstractRIConstraint()
{
  NAHeap *heap = (NAHeap *)collHeap();
  CollIndex entryCount = keyColumns_.entries();
  if (heap != NULL) {
     for (CollIndex i = 0 ; i < entryCount; i++) {
          NADELETE(keyColumns_[i], NAColumn, heap);
     }
  }
  else {
     for (CollIndex i = 0 ; i < entryCount; i++) 
          delete keyColumns_[i];
  }
  keyColumns_.clear();
}

NABoolean AbstractRIConstraint::constraintOverlapsUpdateCols(
                                        BindWA *bindWA,
					const ColSignature &updateCols) const
{
  // INSERT and DELETE affect all columns, so all RI constraints need enforcing;
  // only UPDATE affects a subset of columns.  If this constraint contains one
  // of the columns being UPDATEd then we must enforce it; otherwise we can
  // ignore it, for a handy run-time savings.
  if (!bindWA->getCurrentScope()->context()->inUpdate())
    return TRUE;

  //##if updateCols intersects this->colSignature(), then
  //	##Note: needs to be an intersect that does not modify either signature.
  //	##It need not be a full intersect, just return TRUE at first bit that
  //	##is set in both sigs (first bitwise-ANDed (&ed) int that is nonzero).
  //	##Probably should not matter if sigs are of different sizes --
  //	##just need to AND up to the end of the shorter sig.
    return TRUE;

  //##else return FALSE;

  //##The stub above will suffice for correct enforcement of RI.
  //##Implementing this whole signature bitvec business can be deferred
  //##to beyond FCS.

} // AbstractRIConstraint::constraintOverlapsUpdateCols

// Lookup the other table involved in this RI relationship,
// and find the other constraint in the other NATable constraint list.
// I.e., if called by a UniqueConstraint, this looks in the referencing table's
// refConstraints to find the FK constraint passed in riInfo;
// if called by a RefConstraint, this looks in the referenced table's
// uniqueConstraints to find the UC constraint passed in riInfo.
//
AbstractRIConstraint *AbstractRIConstraint::findConstraint(
				  BindWA *bindWA,
				  const ComplementaryRIConstraint &riInfo) const
{
  // Lookup errors should be impossible, due to Ansi transaction semantics
  // during compilation of a query, so no need for fancy diags, just assert
  // (should only happen if catalog corrupt or txn seriously haywire)

  CorrName tempName(riInfo.tableName_);
  NATable *naTable = bindWA->getNATable(tempName, FALSE);
  if (!naTable) return NULL;

  const AbstractRIConstraintList otherConstraints =
    (getOperatorType() == ITM_UNIQUE_CONSTRAINT) ?
      naTable->getRefConstraints() : naTable->getUniqueConstraints();

  // The find() from Collections template doesn't work for us, so roll our own
  AbstractRIConstraint *c;
  for (CollIndex i = 0; i < otherConstraints.entries(); i++)
  {
    c = otherConstraints[i];
    if (c->getConstraintName() == riInfo.constraintName_) return c;
  }
  *CmpCommon::diags() << DgSqlCode(-4353)
        << DgTableName(naTable->getTableName().getQualifiedNameAsAnsiString()) ;
  bindWA->setErrStatus();
  return NULL;

} // AbstractRIConstraint::findConstraint

void AbstractRIConstraint::setKeyColumns(
     const struct TrafConstrntsDesc *desc,
     CollHeap *heap)
{
  TrafConstrntKeyColsDesc *colDesc;
  NAColumn *column;
  CollIndex i = 0;
  TrafDesc *keyColDesc = desc->constr_key_cols_desc;

  while (keyColDesc)
  {
    colDesc = keyColDesc->constrntKeyColsDesc();
    if( colDesc->isSystemKey() )
      column = new (heap) NAColumn(colDesc->colname, colDesc->position, NULL, heap, NULL, SYSTEM_COLUMN);
    else
      column = new (heap) NAColumn(colDesc->colname, colDesc->position, NULL, heap);
    keyColumns_.insertAt(i, column);
    i++;
    keyColDesc = keyColDesc->next; 
  }

  CMPASSERT(desc->colcount == (signed)i); 
}

UniqueConstraint::~UniqueConstraint()
{
  NAHeap *heap = (NAHeap *)collHeap();
  CollIndex entryCount = refConstraintsReferencingMe_.entries();
  if (heap != NULL) { 
     for (CollIndex i = 0 ; i < entryCount; i++) {
          NADELETE(refConstraintsReferencingMe_[i], ComplementaryRIConstraint, heap);
     }
  }
  else {
     for (CollIndex i = 0 ; i < entryCount; i++) 
          delete refConstraintsReferencingMe_[i];
  }
  refConstraintsReferencingMe_.clear();
}

Int32 UniqueConstraint::getRefConstraints(BindWA *bindWA, 
					const ColSignature &updateCols,
					RefConstraintList &resultList)
{
  if (!constraintOverlapsUpdateCols(bindWA, updateCols))
    return 0;

  CollIndex constraintCnt = refConstraintsReferencingMe_.entries();
  
  //   Loop over FKs referencing this UC.
  //   Find one FK
  //   Tell it that it is the "other table" relative to me
  //   Set the FK's UC's column list pointer to my list of cols
  //   Append the FK to the result list
  for (CollIndex i = 0; i < constraintCnt; i++)
  {
      RefConstraint *rc = (RefConstraint *)findConstraint(bindWA, 
      					    *refConstraintsReferencingMe_[i]);
      if (!rc) return 0;

      // If Binder fails because of preivilege error, QI logic removes 
      // NATable marked for deletion and retries the same query. If the deleted 
      // NATable (T1) had RefConstraint on other table T2, then T2's NATable 
      // RefConstraint contains the address of T1 UniqueConstraint keyColumn_.
      // Since T1 has been removed from the cache, T2's 
      // RefConstraint->uniqueConstraintReferencedByMe_.keyColumns_ could contain 
      // invalid address. If this situation occurs, keyColumns_ will be reset 
      // instead of asserting.
      if ( bindWA->shouldLogAccessViolations() )
        rc->uniqueConstraintReferencedByMe_.resetAfterStatement();

	  rc->setOtherTableName();	     
	  CMPASSERT((rc->getOtherTableName() == rc->getDefiningTableName()) || (rc->getOtherTableName() == getDefiningTableName()));

      
    if (!rc->uniqueConstraintReferencedByMe_.keyColumns_)
		rc->uniqueConstraintReferencedByMe_.keyColumns_ = &keyColumns(); // assignment
    else 
	{
 		CMPASSERT(rc->uniqueConstraintReferencedByMe_.keyColumns_ == &keyColumns()); // equality operator
	}
    
	resultList.insert(rc);
  
  }

  return (Int32)constraintCnt;

} // UniqueConstraint::getRefConstraints


void UniqueConstraint::setRefConstraintsReferencingMe (
     const struct TrafConstrntsDesc* desc, 
     CollHeap* heap,
     BindWA* bindWA)
{
  struct TrafDesc *referencingConstraintDesc = desc->referencing_constrnts_desc;
  ComplementaryRIConstraint *constraintsReferencingMe;

  while (referencingConstraintDesc)
    {
      char *refConstrntName = 
        referencingConstraintDesc->refConstrntsDesc()->constrntname;
      char *refTableName = 
        referencingConstraintDesc->refConstrntsDesc()->tablename;
      
      QualifiedName refConstrnt(refConstrntName, 3, heap, bindWA);
      QualifiedName refTable(refTableName, 3, heap, bindWA);
      
      constraintsReferencingMe = new (heap) ComplementaryRIConstraint(refConstrnt, 
                                                                      refTable,
                                                                      heap);
      refConstraintsReferencingMe_.insert(constraintsReferencingMe);
      
      referencingConstraintDesc = referencingConstraintDesc->next;
    }
} // UniqueConstraint::setRefConstraintsReferencingMe

void UniqueConstraint::resetAfterStatement()
{

  CollIndex constraintCnt = refConstraintsReferencingMe_.entries();
  for (CollIndex i = 0; i < constraintCnt; i++)
    {
      refConstraintsReferencingMe_[i]->resetAfterStatement();
    }
}


void ComplementaryRIConstraint::resetAfterStatement()
{
  keyColumns_ = NULL;
}

RefConstraint::~RefConstraint()
{
}

Int32 RefConstraint::getRefConstraints(BindWA *bindWA, 
				     const ColSignature &updateCols,
				     RefConstraintList &resultList)
{
  if (!constraintOverlapsUpdateCols(bindWA, updateCols))
    return 0;

  // Here we are starting at an FK in the table being ins/upd/del.
  // Its "other table" already points to the UC tablename.
  // Now we lookup the UC this FK references, and 
  // set this FK's UC's column list pointer to the UC's list, and
  // append this to the result.
  //
  // This is like the UC::getRefConstraints method, in that it pushes
  // the UC's keyColumns into the FK's UC-half.
  // The other method has variables "rc" and "this"
  // where this method has "this" and "uc".
    
  UniqueConstraint *uc = (UniqueConstraint *)findConstraint(bindWA, 
					uniqueConstraintReferencedByMe_);
  if (!uc) return 0;
  
  if (!uniqueConstraintReferencedByMe_.keyColumns_)
    uniqueConstraintReferencedByMe_.keyColumns_ = &uc->keyColumns(); // assignment
  else 
    {
      CMPASSERT(uniqueConstraintReferencedByMe_.keyColumns_ == &uc->keyColumns()); // equality operator
    }

  // make sure that otherTableName_ is set correctly (set to the referenced
  // table name)
  this->resetOtherTableName();
  
  resultList.insert(this);

  return 1;

} // RefConstraint::getRefConstraints

Int32 AbstractRIConstraintList::getRefConstraints(
					BindWA *bindWA, 
					const ValueIdSet &assigns,
					RefConstraintList &resultList) const
{
  Constraint::ColSignature updateCols(HEAP);
  if (entries() && bindWA->getCurrentScope()->context()->inUpdate())
    Constraint::makeColSignature(assigns, updateCols);
  
  // Do not clear the list that is being sent by the caller. 
  // Just add to the list
  // resultList.clear();

  Int32 constraintCnt = 0;
  for (CollIndex i = 0; i < entries(); i++)
    constraintCnt += at(i)->getRefConstraints(bindWA, updateCols, resultList);
  return constraintCnt;
} // AbstractRIConstraintList::getRefConstraints

void AbstractRIConstraintList::resetAfterStatement()
{  
  for (CollIndex i = 0; i < entries(); i++)
    {
      if (at(i))
	at(i)->resetAfterStatement();
    }
}


// Writes a predicate of the form (fk1 IS NULL OR fk2 IS NULL)
// consisting of fully qualified column names in Ansi (external) format.
void RefConstraint::getMatchOptionPredicateText(NAString &text, 
						NAString *corrName) const
{
  const KeyColumns keyCols = keyColumns();

  NAString tblText = ( (corrName == NULL) ? 
		       getDefiningTableName().getQualifiedNameAsAnsiString() : *corrName);
  text += "(";
  for (CollIndex i = 0; i < keyCols.entries(); i++)
    {
      if (i)
        text += " OR ";
      text += tblText + "." + 
	ToAnsiIdentifier(keyCols[i]->getColName()) +
	" IS NULL";
    }
  text += ")";
}

// Writes a row-value-constructor consisting of fully qualified column names
// in Ansi (external) format
void RefConstraint::getPredicateText(NAString &text, 
				     const QualifiedName &tblName,
				     const KeyColumns &keyColumns,
				     NAString *corrName) const
{
  NAString tblText = ( (corrName == NULL) ? tblName.getQualifiedNameAsAnsiString() : *corrName);
  int pos= 0;
  text += "(";
  for (CollIndex i = 0; i < keyColumns.entries(); i++)
    {
      if(keyColumns[i]->isSystemColumn() )
         continue;
      if (pos > 0)
      {
        text += ",";
      }
      text += tblText + "." + ToAnsiIdentifier(keyColumns[i]->getColName());
      pos++; //move the pos
    }
  text += ")";
}

// Writes an equality predicate between two RVCs, e.g. "(fk1,fk2)=(uc1,uc2)"
void RefConstraint::getPredicateText(NAString &text, NAString *corrName) const
{
  NABoolean isAReferencingConstraint = isaForeignKeyinTableBeingUpdated();
  // is a referencing constraint.
  if (isAReferencingConstraint) {
    getPredicateText(text, getDefiningTableName(), keyColumns(), corrName);
    text += "=";
    getPredicateText(text, 
		     uniqueConstraintReferencedByMe_.tableName_,
		     *uniqueConstraintReferencedByMe_.keyColumns_);
  }
  else {
    getPredicateText(text, getDefiningTableName(), keyColumns());
    text += "=";	
    getPredicateText(text, uniqueConstraintReferencedByMe_.tableName_,
		     *uniqueConstraintReferencedByMe_.keyColumns_,
		     corrName);
  }
  
}

void RefConstraint::KeyColumnsToPositionsList( LIST(Lng32)& colPositions, 
					       const KeyColumns& keyColumns) const
{
  for (CollIndex i=0; i<keyColumns.entries(); i++)
    colPositions.insert(keyColumns[i]->getPosition());
}

void RefConstraint::getOtherTableKeyColumns(BindWA *bindWA, LIST(Lng32)& colPositions) const
{ 
  UniqueConstraint *uc = 
    (UniqueConstraint *)findConstraint(bindWA, uniqueConstraintReferencedByMe_);

  if (!uc) 
    return;

  if (!uniqueConstraintReferencedByMe_.keyColumns_)
    KeyColumnsToPositionsList(colPositions, uc->keyColumns_); 
}

