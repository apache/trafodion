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
* File:         RETDesc.C
* Description:  Descriptor for the table derived from a RelExpr
* Created:      6/1/95
* Language:     C++
*
*
**************************************************************************
*/


#include "Sqlcomp.h"
#include "AllItemExpr.h"
#include "BindWA.h"
#include "Refresh.h"
#include "MvRefreshBuilder.h"
#include "MVInfo.h"

// ***********************************************************************
// RETDesc::RETDesc()
// ***********************************************************************

RETDesc::RETDesc()
: groupedFlag_(FALSE),
  xtnm_(CmpCommon::statementHeap()),
  xcnm_(CmpCommon::statementHeap()),
  userColumnList_(CmpCommon::statementHeap()),
  naTypeForUserColumnList_(CmpCommon::statementHeap()),
  systemColumnList_(CmpCommon::statementHeap()),
  routineFlag_(FALSE),
  bindWA_(NULL)
{
  //cerr << "RETDesc()     \t" << this << " " << endl;	// ##
}

RETDesc::RETDesc(BindWA* bindWA)
: groupedFlag_(FALSE),
  userColumnList_(bindWA->wHeap()),
  naTypeForUserColumnList_(CmpCommon::statementHeap()),
  systemColumnList_(bindWA->wHeap()),
  xtnm_(bindWA->wHeap()),
  xcnm_(bindWA->wHeap()),
  routineFlag_(FALSE),
  bindWA_(bindWA)
{
  bindWA->getRETDescList().insert(this);
  //cerr << "RETDesc(bwa)  \t" << this << " " << endl;	// ##
}

RETDesc::RETDesc(BindWA *bindWA, const RETDesc& sourceTable)
: userColumnList_(bindWA->wHeap()),
  naTypeForUserColumnList_(CmpCommon::statementHeap()),
  systemColumnList_(bindWA->wHeap()),
  xtnm_(bindWA->wHeap()),
  xcnm_(bindWA->wHeap()),
  groupedFlag_(FALSE),
  routineFlag_(FALSE),
  bindWA_(bindWA)
{
  bindWA->getRETDescList().insert(this);
  //cerr << "RETDesc(bwa,RD&)\t" << this << " " << (void*)&sourceTable << " " << endl;  // ##
  addColumns(bindWA, sourceTable);
}

RETDesc::RETDesc(BindWA *bindWA, TableDesc *tdesc, CorrName *newCorrName)
: userColumnList_(bindWA->wHeap()),
  naTypeForUserColumnList_(CmpCommon::statementHeap()),
  systemColumnList_(bindWA->wHeap()),
  xtnm_(bindWA->wHeap()),
  xcnm_(bindWA->wHeap()),
  groupedFlag_(FALSE),
  routineFlag_(FALSE),
  bindWA_(bindWA)
{
  bindWA->getRETDescList().insert(this);
  //cerr << "RETDesc(bwa,TD*)\t" << this << " " << endl;	// ##

  // Get the corrName, and for performance, apply the defaults to it now,
  // before the loop where it is passed down and "defaulted" again and again.
  CorrName& corrName = newCorrName ? *newCorrName : tdesc->getCorrNameObj();
  NAString cat, sch, tbl;
  corrName.extractAndDefaultNameParts(bindWA,
                                      bindWA->getDefaultSchema(),
                                      cat, sch, tbl);

  const ValueIdList& vidList = tdesc->getColumnList();
  for (CollIndex i = 0; i < vidList.entries(); i++) {
    ValueId valId = vidList[i];
    CMPASSERT(valId.getItemExpr()->getOperatorType() == ITM_BASECOLUMN);
    BaseColumn *baseCol = (BaseColumn *) valId.getItemExpr();
    NAColumn *column = baseCol->getNAColumn();
    ColRefName colRefName(column->getColName(), corrName,
                          CmpCommon::statementHeap());
    addColumn(bindWA, colRefName, valId, 
	      column->getColumnClass(),
	      column->getHeading());

  }

} // RETDesc::RETDesc()

RETDesc::RETDesc(BindWA *bindWA, RoutineDesc *rdesc, QualifiedName *newQualName)
: userColumnList_(bindWA->wHeap()),
  naTypeForUserColumnList_(CmpCommon::statementHeap()),
  systemColumnList_(bindWA->wHeap()),
  xtnm_(bindWA->wHeap()),
  xcnm_(bindWA->wHeap()),
  groupedFlag_(FALSE),
  routineFlag_(TRUE),
  bindWA_(bindWA)
{
  bindWA->getRETDescList().insert(this);
  //cerr << "RETDesc(bwa,TD*)\t" << this << " " << endl;	// ##

  // Get the QualifiedName, and for performance, apply the defaults to it now,
  // before the loop where it is passed down and "defaulted" again and again.
  CMPASSERT(newQualName || (rdesc && rdesc->getNARoutine() &&
                      rdesc->getNARoutine()->getRoutineName()));
      
  const QualifiedName& qualName = newQualName ? *newQualName : 
                            rdesc->getNARoutine()->getRoutineName()->getQualifiedNameObj();
  NAString cat, sch, routine;
  qualName.extractAndDefaultNameParts( bindWA->getDefaultSchema(),
                                      cat, sch, routine);

  // Only the output parameters of the routine are part of the RETDesc..
  const ValueIdList& vidList = rdesc->getOutputColumnList();
  const NARoutine *naroutine = NULL;
  if (rdesc->isUUDFRoutine()) naroutine = rdesc->getActionNARoutine();
  else                        naroutine = rdesc->getNARoutine();

  // if (!naroutine) error.

  for (CollIndex i = 0; i < vidList.entries(); i++) {
    ValueId valId = vidList[i];
    CMPASSERT(valId.getItemExpr()->getOperatorType() == ITM_ROUTINE_PARAM);
    RoutineParam *routineOutParam = (RoutineParam *) valId.getItemExpr();
    NAColumn *column = naroutine->getParams()[routineOutParam->getOrdinalPos()];
    ColRefName colRefName(column->getColName(), qualName,
                          CmpCommon::statementHeap());
    addColumn(bindWA, colRefName, valId, 
	      column->getColumnClass(),
	      column->getHeading());
  }

} // RETDesc::RETDesc()

RETDesc::~RETDesc()
{
  systemColumnList_.clearAndDestroy();
  userColumnList_.clearAndDestroy();
  if (bindWA_)
    bindWA_->getRETDescList().remove(this);
  //cerr << "~RETDesc~     \t" << this << " bwa=" << bindWA_ << " " << endl;	// ##
}


// ***********************************************************************
// RETDesc::addColumn()
// ***********************************************************************
void RETDesc::addColumn(BindWA *bindWA,
			const ColRefName& colRefName,
                        const ValueId valId,
                        const ColumnClass colClass,
			const char *heading)
{
  //
  // Create a column descriptor, insert it into the column list, and add the
  // exposed column name(s) to the hash table.
  //
  if (colClass == SYSTEM_COLUMN)
    addColumnDesc(bindWA, colRefName, valId, heading, systemColumnList_);
  else
    addColumnDesc(bindWA, colRefName, valId, heading, userColumnList_);
  //
  // If the column has a qualifier, find the list of columns for that qualifier.
  //
  // Create that list the first time (even for a "system" column -- see how
  // OrderBy processing is done -- this lets errmsg 4001-04 tables-in-scope
  // info to be correct).
  //
  // If the column is a user column, insert it into the list. 
  //
  CorrName corrName(colRefName.getCorrNameObj(), CmpCommon::statementHeap());
  if (corrName != "") {
    ColumnDescList *columnList = getQualColumnList(corrName);
    if (!columnList) {
      columnList = new (bindWA->wHeap()) ColumnDescList(bindWA->wHeap());
      xtnm_.insertNames(bindWA, corrName, columnList);
    }
    if (colClass != SYSTEM_COLUMN) {
      ColumnDesc *column = userColumnList_[getDegree() - 1];
      columnList->insert(column);
    }
  }
} // RETDesc::addColumn

// ***********************************************************************
// RETDesc::delColumn()
// ***********************************************************************
void RETDesc::delColumn(BindWA *bindWA,
			const ColRefName& colRefName,
                        const ColumnClass colClass)
{
  //
  // Delete the column descriptor from the column list and the hash table.
  //
  if (colClass == SYSTEM_COLUMN) {
    delColumnDesc(bindWA, colRefName, systemColumnList_);
    return;
  }
  delColumnDesc(bindWA, colRefName, userColumnList_);
  //
  // If the user column has a qualifier, find the list of columns for that
  // qualifier; if such a list exists, delete the column descriptor from it.
  //
  CorrName corrName(colRefName.getCorrNameObj());
  if (corrName != "") {
    //
    // If a qualified column list has been created, delete column desc from it.
    //
    ColumnDescList *columnList = getQualColumnList(corrName);
    if (columnList) delColumnDesc(bindWA, colRefName, *columnList);
  }
} // RETDesc::delColumn

// ***********************************************************************
// RETDesc::addColumnDesc()
// ***********************************************************************
void RETDesc::addColumnDesc(BindWA *bindWA,
			    const ColRefName& colRefName,
                            const ValueId valId,
			    const char *heading,
                            ColumnDescList& columnList)
{
  // Create a column descriptor and insert it into the column list.
  //
  ColumnDesc *column = new (bindWA->wHeap()) ColumnDesc(colRefName, valId,
							heading, bindWA->wHeap());
  columnList.insert(column);

  // The newSimpleDup logic, in several places below, is to detect one type
  // of duplicate column that the XCNM hash table cannot:
  // that both "col" and "corr-or-qual.col" appear.
  // E.g, the following duplicates (ambiguities) are detected:
  //	select a,ta.a from ta order by a;	-- found by XCNM logic
  //	select ta.a,a from ta order by a;	-- found by XCNM logic
  //	select * from ta x, ta y order by a;	-- found by XCNM logic
  //	select a,ta.a from ta order by ta.a;	-- found by newSimpleDup logic
  //	select ta.a,a from ta order by ta.a;	-- found by newSimpleDup logic
  // while preserving the legality of the following:
  //	select * from ta x, ta y order by x.a,y.a;	-- legal!
  //	select a,ta.a from ta;				-- legal, of course
  //
  // To understand this newSimpleDup logic, first realize that if multiple
  // tables are in the FROM-list, then the XCNM logic will disallow references
  // to duplicate simple names ("col") in the select-list.
  // Thus newSimpleDup only has to deal with queries on one table.
  //
  ValueId newSimpleDup(NULL_VALUE_ID);

  CorrName corrName(colRefName.getCorrNameObj(), CmpCommon::statementHeap());
  ExtendedQualName::SpecialTableType specialType = corrName.getSpecialType();

  // Add the exposed column name(s) to the hash table.
  // First, the simple name "col" ...
  //
  NAString simpleColNameStr(colRefName.getColName(),
                            CmpCommon::statementHeap());
  ColRefName simpleColRefName(simpleColNameStr);
  simpleColRefName.setSpecialType(specialType);
  ColumnNameMap *xcnmEntry = findColumn(simpleColRefName);
  if (xcnmEntry) {
    if (NOT xcnmEntry->isDuplicate())
      newSimpleDup = xcnmEntry->getValueId();	// *not* "newSimpleDup = valId"!
    xcnmEntry->setDuplicateFlag();
  } else
    xcnm_.insert(new (bindWA->wHeap()) ColumnNameMap(simpleColRefName, column, bindWA->wHeap()));

  if (corrName == "") {
    if (newSimpleDup != NULL_VALUE_ID && simpleColNameStr != "") {
      // Find previously added xcnm's whose simple name is "col"
      // and whose valueId matches, and mark them as dups.
      LIST(ColumnNameMap*) xcnmList(CmpCommon::statementHeap());
      xcnm_.dump(xcnmList);
      for (CollIndex i = 0; i < xcnmList.entries(); i++) {
	if (xcnmList[i]->getValueId() == newSimpleDup &&
	    xcnmList[i]->getColRefNameObj().getColName() == simpleColNameStr)
	  xcnmList[i]->setDuplicateFlag();
      }
    }
    return;
  }

  // ... Second, the name "corr-or-qual.col", if "corr" or "qual" is present:
  // if "corr", insert "corr.col" into hash table;
  // if "qual", insert fully expanded "c.s.t.col" into hash table, plus
  //    if "c"==default-catalog, insert "s.t.col", plus
  //      if also "s"==default-schema, insert "t.col".
  //
  // The insertion and the duplicate flagging allow our later discovery
  // of ambiguous column references, e.g.,
  //   "select * from (select a,b as a from ta) as aa order by aa.a;"
  //
  // The logic here is similar to that in RETDesc::delColumnDesc and
  // XTNM::insertNames -- just a little more complicated (pointers, and
  // construction of objects only if needed) because it's a tiny bit more
  // efficient in runtime for this most common codepath.

  NAString cat, sch, tbl;
  Int32 defaultMatch = corrName.extractAndDefaultNameParts(
					bindWA,
                                        bindWA->getDefaultSchema(),
                                        cat, sch, tbl);

  ColRefName *cstColRefName = NULL, *stColRefName = NULL, *tColRefName = NULL;
  const ColRefName *canonicalColRefName;

  if (corrName.getCorrNameAsString() != "") {
    canonicalColRefName = &colRefName;		// "corr.col" just as passed in
    CMPASSERT(!defaultMatch);
  } else {

    cstColRefName = new(bindWA->wHeap())
                      ColRefName(simpleColNameStr, 
                                 CorrName(tbl,bindWA->wHeap(),sch,cat),
                                 bindWA->wHeap());
    cstColRefName->setSpecialType(specialType);
    canonicalColRefName = cstColRefName;	// "c.s.t.col"

    if (defaultMatch) {
      stColRefName = new(bindWA->wHeap())
	               ColRefName(simpleColNameStr, 
                                  CorrName(tbl,bindWA->wHeap(),sch),
                                  bindWA->wHeap());
      stColRefName->setSpecialType(specialType);

      if (defaultMatch > 1) {
	tColRefName = new(bindWA->wHeap())
	                ColRefName(simpleColNameStr, 
                                   CorrName(tbl),
                                   bindWA->wHeap());
	tColRefName->setSpecialType(specialType);
      }
    }
  }

  // This first if-test is just a performance hack:
  // If simple "col" wasn't a duplicate, "canon.col" can't be, so skip findCol
  if (xcnmEntry) {
    xcnmEntry = findColumn(*canonicalColRefName);
    if (xcnmEntry) {
      xcnmEntry->setDuplicateFlag();
      
      if(xcnmEntry->getValueId() != valId){
	xcnmEntry->setQualifiedColumnAmbiguousFlag();
      }

      if (defaultMatch) {
	xcnmEntry = findColumn(*stColRefName);
	CMPASSERT(xcnmEntry);
	xcnmEntry->setDuplicateFlag();

	if(xcnmEntry->getValueId() != valId){
	  xcnmEntry->setQualifiedColumnAmbiguousFlag();
	}

	if (defaultMatch > 1) {
	  xcnmEntry = findColumn(*tColRefName);
	  CMPASSERT(xcnmEntry);
	  xcnmEntry->setDuplicateFlag();

	  if(xcnmEntry->getValueId() != valId){
	    xcnmEntry->setQualifiedColumnAmbiguousFlag();
	  }

	} // "t.col" must be marked as a dup too
      } // "s.t.col" must be marked as a dup too
    } // "canon.col" was a dup
  } // simple "col" was a dup

  if (!xcnmEntry) {
    // If simple "col" was added on a previous call -- and thus was found
    // to be a dup in the logic above -- then mark these xcnm's we are
    // about to insert as dups, right off the bat.
    NABoolean isDup = newSimpleDup == valId;
    xcnm_.insert(new (bindWA->wHeap())
		 ColumnNameMap(*canonicalColRefName, column, bindWA->wHeap(), isDup));
    if (defaultMatch) {
      xcnm_.insert(new (bindWA->wHeap())
		   ColumnNameMap(*stColRefName, column, bindWA->wHeap(), isDup));
      if (defaultMatch > 1) {
	xcnm_.insert(new (bindWA->wHeap())
		     ColumnNameMap(*tColRefName, column, bindWA->wHeap(), isDup));
      } // "t.col" not dup, must be inserted
    } // "s.t.col" not dup, must be inserted
  } // "canon.col" not dup, insert

  delete cstColRefName;
  delete stColRefName;
  delete tColRefName;

} // RETDesc::addColumnDesc

// ***********************************************************************
// RETDesc::delColumnDesc()
// ***********************************************************************
void RETDesc::delColumnDesc(BindWA *bindWA,
			    const ColRefName& colRefName,
                            ColumnDescList& columnList)
{
  //
  // Delete all matching column descriptor(s) from the column list.
  //
  // "if (columnList.remove(&tmpColumn))"
  // does not work here, since columnList is a list of pointers,
  // i.e. remove does pointer comparisons only (and never finds a match).
  for (CollIndex i = 0; i < columnList.entries(); i++) {
    ColumnDesc *colDesc = columnList[i];
    if (colDesc->getColRefNameObj() == colRefName) {
      columnList.remove(colDesc);
      break;
    }
  }
  //
  // Delete the exposed column name(s) from the hash table.
  //
  CorrName corrName(colRefName.getCorrNameObj());
  ExtendedQualName::SpecialTableType specialType = corrName.getSpecialType();

  NAString simpleColNameStr(colRefName.getColName(),
                            CmpCommon::statementHeap());
  ColRefName simpleColRefName(simpleColNameStr);
  simpleColRefName.setSpecialType(specialType);
  xcnm_.remove(&simpleColRefName);
  if (corrName == "") return;

  NAString cat, sch, tbl;
  Int32 defaultMatch = corrName.extractAndDefaultNameParts(
					bindWA,
                                        bindWA->getDefaultSchema(),
                                        cat, sch, tbl);

  ColRefName cstColRefName(simpleColNameStr, CorrName(tbl,bindWA->wHeap(),sch,cat));
  ColRefName stColRefName(simpleColNameStr, CorrName(tbl,bindWA->wHeap(),sch));
  ColRefName tColRefName(simpleColNameStr, CorrName(tbl));
  cstColRefName.setSpecialType(specialType);
  stColRefName.setSpecialType(specialType);
  tColRefName.setSpecialType(specialType);

  const ColRefName *canonicalColRefName;
  if (corrName.getCorrNameAsString() != "")
    canonicalColRefName = &colRefName;		// "corr.col"
  else
    canonicalColRefName = &cstColRefName;	// "c.s.t.col"

  xcnm_.remove(canonicalColRefName);
  if (defaultMatch) {
    xcnm_.remove(&stColRefName);
    if (defaultMatch > 1)
      xcnm_.remove(&tColRefName);
  }

} // RETDesc::delColumnDesc

// ***********************************************************************
// RETDesc::nullInstantiateAndAddColumns()
// Operates on the "this" RETDesc, adding columns to it from columnList.
// ***********************************************************************
void RETDesc::nullInstantiateAndAddColumns(BindWA *bindWA,
					   NABoolean forceCast,
					   const ColumnDescList& columnList,
					   const ColumnClass colClass)
{
  for (CollIndex i = 0; i < columnList.entries(); i++) {
    ValueId nullId = columnList[i]->getValueId().nullInstantiate(bindWA,
								 forceCast);
    addColumn(bindWA, columnList[i]->getColRefNameObj(), nullId, colClass);
  }
} // RETDesc::nullInstantiateAndAddColumns

// ***********************************************************************
// RETDesc::nullInstantiate()
// Returns a new RETDesc, the null-instantiated copy of the const "this";
// and appends list of columns to nullOutputList.
// ***********************************************************************
RETDesc *RETDesc::nullInstantiate(BindWA *bindWA,
				  NABoolean forceCast,
				  ValueIdList& nullOutputList) const
{
  RETDesc *resultTable = new (bindWA->wHeap()) RETDesc();

  resultTable->nullInstantiateAndAddColumns(bindWA,
					    forceCast,
					    *getColumnList(),
					    USER_COLUMN);

  resultTable->nullInstantiateAndAddColumns(bindWA,
					    forceCast,
					    *getSystemColumnList(),
					    SYSTEM_COLUMN);

  resultTable->getValueIdList(nullOutputList, USER_AND_SYSTEM_COLUMNS);
  return resultTable;
} // RETDesc::nullInstantiate()

// ***********************************************************************
// RETDesc::findColumn()
// find an entry whose vid matches and is either
// 1. fully-qualified and whose vid matches ...or...
// 2. is a fabricated name
// ***********************************************************************
ColumnNameMap *RETDesc::findColumn(const ValueId vid) const
{
  LIST(ColumnNameMap *) colNameMapList(STMTHEAP);

  ColumnNameMap *currColNameMap = NULL;

  xcnm_.dump(colNameMapList);

  for (CollIndex i = 0; i < colNameMapList.entries(); i++)
  {
    ColumnNameMap *colNameMap = colNameMapList[i];
    if (colNameMap->getValueId() EQU vid)
    {
      const ColRefName &colRefName = colNameMap->getColumnDesc()->
                                     getColRefNameObj();
      if (colRefName.isFabricated())
      {
        return colNameMap;
      }
      else if (NOT colRefName.isEmpty() AND
               colRefName.getCorrNameObj().getQualifiedNameObj().
               fullyExpanded())
      {
        return colNameMap;
      }
      else if(!currColNameMap || 
              (colRefName.getCorrNameObj().getQualifiedNameObj().
               numberExpanded() >
               currColNameMap->getColumnDesc()->getColRefNameObj().
               getCorrNameObj().getQualifiedNameObj().numberExpanded())) {
        currColNameMap = colNameMap;
      }
    }
  } // for
  return currColNameMap;
} // RETDesc::findColumn()

// ***********************************************************************
// RETDesc::getTableList()
// Returns a list of the FROM-clause tables described by this RETDesc.
// Only one name (the canonical name) for each table ref appears in the list.
// ***********************************************************************
void RETDesc::getTableList(LIST(TableNameMap*) &xtnmList,
			   NAString *textList) const
{
  xtnmList.clear();
  xtnm_.dump(xtnmList);
  CollIndex i;

  // Get rid of dummy entries inserted by XTNM::insertNames().
  for (i = xtnmList.entries(); i-- > 0; )
    if (!xtnmList[i]->getColumnList()) 
      xtnmList.removeAt(i);

  // Separate names by number of parts (corr, tbl, sch.tbl, cat.sch.tbl)
  LIST(TableNameMap*) xtnmCORR(STMTHEAP), xtnmT(STMTHEAP); 
  LIST(TableNameMap*) xtnmST(STMTHEAP), xtnmCST(STMTHEAP);
  for (i = 0; i < xtnmList.entries(); i++) {
    const CorrName& corr = xtnmList[i]->getTableName();
    const QualifiedName& qual = corr.getQualifiedNameObj();
    if (!corr.isFabricated())
      if (!corr.getCorrNameAsString().isNull())
	xtnmCORR.insert(xtnmList[i]);
      else if (qual.getSchemaName().isNull())
	xtnmT.insert(xtnmList[i]);
      else if (qual.getCatalogName().isNull())
	xtnmST.insert(xtnmList[i]);
      else
	xtnmCST.insert(xtnmList[i]);
  }
  ComASSERT(xtnmCST.entries() >= xtnmST.entries());
  ComASSERT(xtnmCST.entries() >= xtnmT.entries());

  // Put into the list only the full 3-part names and the correlation names
  xtnmList = xtnmCST;
  for (i = 0; i < xtnmCORR.entries(); i++)
    xtnmList.insert(xtnmCORR[i]);

  formatTableList(xtnmList, textList);	// a no-op if textList passed as NULL

} // RETDesc::getTableList

/*static*/ void RETDesc::formatTableList(LIST(TableNameMap*) &xtnmList,
					 NAString *textList,
					 NABoolean includePartitionName) /*const*/
{
  if (!textList) return;		// do nothing if passed as NULL

  // Sort in ascending order by name, removing any duplicates.
  if (xtnmList.entries() > 1) {
    LIST(TableNameMap*) xtnmT(STMTHEAP);
    xtnmT = xtnmList;
    xtnmList.clear();

    while (xtnmT.entries()) {
      NAString minT(CmpCommon::statementHeap());
      if (includePartitionName)
	minT = xtnmT[0]->getTableName().getExposedNameAsStringWithPartitionNames();
      else
	minT = xtnmT[0]->getTableName().getExposedNameAsAnsiString();

      CollIndex minTi = 0;

      for (CollIndex i = xtnmT.entries() - 1; i > 0; i--) {

        NAString T(CmpCommon::statementHeap());
	if (includePartitionName)
	  T = xtnmT[i]->getTableName().getExposedNameAsStringWithPartitionNames(); 
	else
	  T = xtnmT[i]->getTableName().getExposedNameAsAnsiString();

	if (minT > T) {
	  minT = T; minTi = i; break;
	}
	else if (minT == T)		// remove dups
	  xtnmT.removeAt(i);		// this is why we loop backwards
      }		// for

      if (xtnmList.entries() == 0 ||	// don't insert dups
          xtnmList[xtnmList.entries()-1]->getTableName() != 
	  xtnmT[minTi]->getTableName())
	xtnmList.insert(xtnmT[minTi]);

      xtnmT.removeAt(minTi);		// been there, seen that minimum
    }		// while
  }		// if

  for (CollIndex i = 0; i < xtnmList.entries(); i++) {
    NAString T(CmpCommon::statementHeap());
	if (includePartitionName)
	  T = xtnmList[i]->getTableName().getExposedNameAsStringWithPartitionNames();
	else
	  T = xtnmList[i]->getTableName().getExposedNameAsAnsiString(FALSE, TRUE);
    if (!T.isNull()) { 			// if not fabricated
      if (!textList->isNull())
        *textList += ", ";
      *textList += T;
      if (xtnmList[i]->getTableName().isVolatile())
	{
	  *textList += "(volatile)";
	}
    }
  }

} // RETDesc::formatTableList

// ***********************************************************************
// MVs --
// These methods are only called from a RelRoot or RenameTable nodes when
// the BindWA flag isBindingMvRefresh is set: during CREATE MV, and during
// the binding of an INTERNAL REFRESH command.
// If the RETDesc of the current scope has column called colName - copy 
// it over. If the column is already in this RETDesc (by ValueId), 
// delete it and insert it again with the original name (NOT of the view).
// ***********************************************************************
void RETDesc::propagateColumn(BindWA	       *bindWA, 
			      const ColRefName& colName, 
			      NABoolean		isFromRoot,
			      ColumnClass       colClass)
{
  ColumnNameMap *colMap = NULL; 
  // Find the column in the RETDesc of the current scope.
  RETDesc *retDesc = bindWA->getCurrentScope()->getRETDesc();
  if (retDesc != NULL)
    colMap = retDesc->findColumn(colName);

  if (colMap != NULL)
  {
    // If it is not in 'this', copy it over.
    ColumnNameMap *viewCol = findColumn(colMap->getValueId());
    if (viewCol != NULL)
    {
      NABoolean isSyskeyOfLog = 
	colName.getCorrNameObj().getSpecialType() == ExtendedQualName::IUD_LOG_TABLE;
      if (isFromRoot || isSyskeyOfLog)
	return;
      else
      {
	// this RETDesc already has the ValueId - delete it before reinserting.
	const ColRefName& viewColRefName = 
	  viewCol->getColumnDesc()->getColRefNameObj();
	delColumn(bindWA, viewColRefName, colClass);
      }
    }

    addColumn(bindWA, colMap->getColumnDesc()->getColRefNameObj(), 
              colMap->getValueId(), colClass);
  } 
}

// Find the columns to propagate: the @OP column, and the SYSKEY columns 
// of the underlying tables.
void RETDesc::propagateOpAndSyskeyColumns(BindWA *bindWA, NABoolean isFromRoot)
{
  // Find the SYSKEY and @OP columns from the user column list.
  ValueIdList vidList;
  RETDesc *source = bindWA->getCurrentScope()->getRETDesc();
  if (source == NULL)
    return;

  source->getValueIdList(vidList, USER_COLUMN);
  for (CollIndex i=0; i<vidList.entries(); i++)
  {
    ColumnNameMap *colMap = source->findColumn(vidList[i]);
    if (colMap == NULL)
      continue;

    const ColRefName& colName = 
      colMap->getColumnDesc()->getColRefNameObj();
    if (colName.getColName() != "SYSKEY"       &&
        colName.getColName() != MavBuilder::getVirtualOpColumnName())
      continue;

    propagateColumn(bindWA, colName, isFromRoot, USER_COLUMN);
  }

  // Find the SYSKEY columns from the system column list.
  vidList.clear();
  source->getValueIdList(vidList, SYSTEM_COLUMN);
  for (CollIndex j=0; j<vidList.entries(); j++)
  {
    ColumnNameMap *colMap = source->findColumn(vidList[j]);
    if (colMap == NULL)
      continue;

    const ColRefName& colName = 
      colMap->getColumnDesc()->getColRefNameObj();
    if (colName.getColName() != "SYSKEY")
      continue;

    propagateColumn(bindWA, colName, isFromRoot, SYSTEM_COLUMN);
  }
}

// create a list of NATypes corresponding to each entry in the
// userColumnList_ in RETDesc. Used by generator to convert to
// this type during output expr code gen.
// Return: TRUE, if error. FALSE, if all ok.
NABoolean RETDesc::createNATypeForUserColumnList(CollHeap * heap)
{
  for (CollIndex i = 0; i < userColumnList_.entries(); i++) 
    {
      const NAType &userColType = userColumnList_[i]->getValueId().getType();
      NAType * newType = userColType.newCopy(heap);
      naTypeForUserColumnList_.insert(newType);
    }
  return FALSE;
}
	
void RETDesc::changeNATypeForUserColumnList(CollIndex index, const NAType * newType)
{
  naTypeForUserColumnList_.removeAt(index);
  naTypeForUserColumnList_.insertAt(index, (NAType*)newType);
}
				 
// ***********************************************************************
// Display/print, for debugging.
// ***********************************************************************
void RETDesc::display() const { print(); }

void RETDesc::print(FILE* ofd, const char* indent, const char* title) const
{
#ifndef NDEBUG
  CollIndex i;

#pragma nowarn(1506)   // warning elimination 
  BUMP_INDENT(indent);
#pragma warn(1506)  // warning elimination 
  fprintf(ofd,"\n NEW RetDesc START \n%s%s %p %s\n --------------------- \n",
    NEW_INDENT, title, this,
    groupedFlag_ ? "(grouped) " : "");

  LIST(TableNameMap*) xtnmList(STMTHEAP);
  xtnm_.dump(xtnmList);
  for (i = 0; i < xtnmList.entries(); i++)
    fprintf(ofd, "%s\n ",
		 xtnmList[i]->getTableName().getExposedNameAsString().data());
  fprintf(ofd,"\n\n\n");

  for (i = 0; i < userColumnList_.entries(); i++) {
    if (i > 0) fprintf(ofd,",\n ");
    userColumnList_[i]->print(ofd, NEW_INDENT, "");
  }
  if (systemColumnList_.entries()) fprintf(ofd,"\nSYSTEM COLUMNS:\n; ");
  for (i = 0; i < systemColumnList_.entries(); i++) {
    if (i > 0) fprintf(ofd,", ");
    systemColumnList_[i]->print(ofd, NEW_INDENT, "");
  }
  if (userColumnList_.entries() || systemColumnList_.entries())
    fprintf(ofd,"\n\nUSER COLUMNS:\n\n");

  LIST(ColumnNameMap*) xcnmList(CmpCommon::statementHeap());
  xcnm_.dump(xcnmList);
  for (i = 0; i < xcnmList.entries(); i++) {
    if (i > 0) fprintf(ofd,",\n");
    xcnmList[i]->print(ofd, NEW_INDENT, "");
  }
  if (i) fprintf(ofd,"\n");

#endif
} // RETDesc::print()

static void displayDownHelper(RelExpr *re, RETDesc *prevRD, int level)
{
  RETDesc *thisRD = re->getRETDesc();
  char indent[20];

  snprintf(indent, sizeof(indent), "Level %4d:  ", level);
  cout << endl
       << endl
       << indent
       << "====== Operator: " << re->getText().data()
       << endl
       << flush;
  if (thisRD != prevRD && thisRD)
    thisRD->print(stdout, indent);
  else if (thisRD == NULL)
    cout << indent << "++++++ RETDesc is NULL" << endl;
  else
    cout << indent << "++++++ RETDesc is the same as its parent" << endl;


  for (int c=0; c<re->getArity(); c++)
    {
      RelExpr *x = re->child(c);
      if (x)
        {
          cout << indent << "++++++ Child " << c << endl;
          displayDownHelper(x, thisRD, level+1);
        }
    }
} // RETDesc::displayDown

/*static*/ void RETDesc::displayDown(RelExpr *re)
{
  displayDownHelper(re, NULL, 1);
} // RETDesc::displayDown

