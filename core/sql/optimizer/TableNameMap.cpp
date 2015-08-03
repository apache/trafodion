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
* File:         TableNameMap.C
* Description:  A name map for a table
* Created:      04/16/96
* Language:     C++
*
*
**************************************************************************
*/

// -----------------------------------------------------------------------

#include "BindWA.h"
#include "RETDesc.h"
#include "TableNameMap.h"

XTNM::~XTNM() 
{ 
  // Doing clearAndDestroy here will cause some ColumnDescList* being
  // deleted twice, in the case of 3 parts name. 
  // But without this there will be memory leak. What should be done is
  // in XTNM::insertNames, make sure ColumnDescList* is not used more 
  // than one time, note ColumnDescList is passed in as a pointer.

  //clearAndDestroy();
}

// XTNM clients must use this method, not the simplistic insert()
// inherited from NAKeyLookup!  
// The logic here is cribbed from RETDesc::addColumnDesc.
//
void XTNM::insertNames(BindWA *bindWA,
		       CorrName& corrName,
		       ColumnDescList *cols)
{
  // For checking of ANSI 6.3 SR 3+4.
  //
  const TableNameMap *prev;

  // corrName contains a corr name.
  // That is the only exposed name, so we insert just one TableNameMap.
  //
  if (corrName.getCorrNameAsString() != "") {
    if (prev = get(&corrName)) {
      if (prev->getTableName().getCorrNameAsString() != "")
        // 4056 Exposed name appears multiple times.
        *CmpCommon::diags() << DgSqlCode(-4056) 
	  << DgTableName(corrName.getExposedNameAsAnsiString());
      else {
        // 4057 Correlation name X conflicts with table name C.S.X
	QualifiedName tblName(corrName.getCorrNameAsString(),
			      bindWA->getDefaultSchema().getSchemaName(),
			      bindWA->getDefaultSchema().getCatalogName());
        *CmpCommon::diags() << DgSqlCode(-4057)
	  << DgString0(corrName.getExposedNameAsAnsiString())
	  << DgTableName(tblName.getQualifiedNameAsAnsiString());
      }
      bindWA->setErrStatus();
      return;
    }
    insert(new(bindWA->wHeap()) TableNameMap(corrName, cols, bindWA->wHeap()));
    return;
  }

  // Here, corrName does not contain a corr name, only a qualified name.
  // Dissect out the 3 parts of qual name; 
  // fill in corrName's underlying qual name's default cat & sch if need be.
  // (This method has the desired effect only when 
  // corrName does not contain a corr name.)
  //
  NAString catName(CmpCommon::statementHeap()),
           schName(CmpCommon::statementHeap()),
           tblName(CmpCommon::statementHeap());
  Int32 defaultMatch = corrName.extractAndDefaultNameParts(
					bindWA,
					bindWA->getDefaultSchema(),
					catName, schName, tblName);
  CorrName cstCorr(tblName, CmpCommon::statementHeap(), schName, catName);
  CorrName stCorr(tblName, CmpCommon::statementHeap(), schName);
  CorrName tCorr(tblName,CmpCommon::statementHeap());

  cstCorr.setSpecialType(corrName);
  stCorr.setSpecialType(corrName);
  tCorr.setSpecialType(corrName);
  cstCorr.setPartnClause(corrName.getPartnClause());
  stCorr.setPartnClause(corrName.getPartnClause());
  tCorr.setPartnClause(corrName.getPartnClause());

  cstCorr.setIsVolatile(corrName.isVolatile());
  stCorr.setIsVolatile(corrName.isVolatile());
  tCorr.setIsVolatile(corrName.isVolatile());

  if (prev = get(&cstCorr)) {
    // 4056 Exposed name appears multiple times.
    *CmpCommon::diags() << DgSqlCode(-4056) 
      << DgTableName(corrName.getExposedNameAsAnsiString());
    bindWA->setErrStatus();
    return;
  }
  if (prev = get(&tCorr)) {	// simple table name, i.e. qualified identifier
    if (prev->getTableName().getCorrNameAsString() != "") {
      // 4057 Correlation name Q conflicts with table name C.S.Q
      *CmpCommon::diags() << DgSqlCode(-4057)
	<< DgString0(tCorr.getExposedNameAsAnsiString())
	<< DgTableName(cstCorr.getExposedNameAsAnsiString());
      bindWA->setErrStatus();
      return;
    }
    // else, it's okay: prev is pointing to a dummy "T" not a correlation name
    // (see last line below).
  }

  // Always insert "C.S.T".
  // If "C" is the default, also insert "S.T".
  // If "C" and "S" are both the default, also insert "T".
  //
  insert(new(bindWA->wHeap()) TableNameMap(cstCorr, cols, bindWA->wHeap()));
  if (defaultMatch) {
    insert(new(bindWA->wHeap()) TableNameMap(stCorr, cols, bindWA->wHeap()));
    if (defaultMatch > 1)
      insert(prev = new(bindWA->wHeap()) 
	     TableNameMap(tCorr, cols, bindWA->wHeap()));  // possibly overwrite dummy
  }
  else if (catName == "" && schName == "") return;   // ## tmp! Only for our
			// current dev/regress sqlci environment, where we have
			// no default cat&sch and none is required.
			// This is *not* ANSI; remove! ##

  // If we didn't now or earlier insert "T", insert a dummy one now.
  // This allows the two checks above enforcing the second half of 6.3 SR 4
  // (corr name shall not be same as qual ident of exposed table name).
  //
  if (NOT prev) insert(new(bindWA->wHeap()) 
		       TableNameMap(tCorr, NULL /* dummy name */, bindWA->wHeap()));

} // XTNM::insertNames

void TableViewUsageList::display(NABoolean newline, size_t indent) const
{
  if (newline) cerr << endl;

  for (CollIndex i=0; i<entries(); i++) {
    cerr << at(i)->viewCount() << (at(i)->isView() ? ' ' : '.');

    for (size_t b=at(i)->viewCount()*indent; b; b--) cerr << ' ';

    cerr << at(i)->getTableName().getQualifiedNameAsAnsiString() << endl;
  }
}

void TableViewUsageList::display() const
{ display(TRUE, 2); }

// Get a formatted textual list of TOPMOST view names that use (contain) the
// given basetable name.
// Return number of times basetable name was seen, either contained in a view
// or at the top level itself
// (if only seen in the latter case, the formatted list will be empty).
//
Int32 TableViewUsageList::getViewsOnTable(
			CollIndex begIx, CollIndex endIx, Int32 viewCount,
			const QualifiedName &baseName,
			ExtendedQualName::SpecialTableType baseType,
			const QualifiedName *additionalNameToFormat,
			NAString &resultingViewNames) const
{
  Int32 baseNameSeen = 0;
  const QualifiedName *topViewName = NULL;
  CollHeap *h = CmpCommon::statementHeap();
  LIST(TableNameMap*) tmpViewList(h);
  for (CollIndex i=begIx; i<endIx; i++) {

    CMPASSERT(at(i)->viewCount() >= viewCount);

    if (at(i)->isView()) {
      if (at(i)->viewCount() == viewCount)
        topViewName = &at(i)->getTableName();
    }

    else if (at(i)->getTableName() == baseName &&
    	     at(i)->getSpecialType() == baseType) {
      baseNameSeen++;
      if (at(i)->viewCount() > viewCount) {
        CMPASSERT(topViewName);
	tmpViewList.insert(new (h) TableNameMap(*topViewName,NULL,h));
      }
    }
  }

  if (baseNameSeen) {
    if (additionalNameToFormat)
      tmpViewList.insert(new (h) TableNameMap(*additionalNameToFormat,NULL,h));
    RETDesc::formatTableList(tmpViewList, &resultingViewNames);
  }
  return baseNameSeen;
} // TableViewUsageList::getViewsOnTable

