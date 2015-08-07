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
#ifndef RETDESC_H
#define RETDESC_H
/* -*-C++-*-
**************************************************************************
*
* File:         RETDesc.h
* Description:  Descriptor for the table derived from a RelExpr
* Created:      5/19/95
* Language:     C++
*
*
*	When such a mutual pair
*	And on such a twain can do't, in which I bind,
*	On pain of punishment, the world to weet
*	We stand up peerless.
*		-- Mark Antony, Antony and Cleopatra, I:i
*
**************************************************************************
*/


#include "Collections.h"
#include "ColumnDesc.h"
#include "ColumnNameMap.h"
#include "ObjectNames.h"
#include "TableNameMap.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class RETDesc;
class RETDescList;

// -----------------------------------------------------------------------
// forward declarations
// -----------------------------------------------------------------------

class TableDesc;
class RoutineDesc;

// ***********************************************************************
// RETDesc : A descriptor for the table derived from a RelExpr
// ***********************************************************************
class RETDesc : public NABasicObject
{
private:
  // ---------------------------------------------------------------------
  // Disallowed constructor and assignment operator.
  // There are no implementations for these:  not called privately either.
  //   (These *could* be written such that they used the source RETDesc's
  //   bindWA backpointer to append the new RETDesc to the bindWA's RDList
  //   like the other ctors do, and created new copies of all data members,
  //   in order to have distinct RETDescs on the list to be destroyed at
  //   the end of the stmt to reclaim memory.  But there's no need to write
  //   and allow these, as the public bindWA-passing ctors below suffice.)
  // ---------------------------------------------------------------------
  RETDesc(const RETDesc&);
  RETDesc& operator=(const RETDesc&);
  
public:
  // ---------------------------------------------------------------------
  // Constructors and destructor
  // ---------------------------------------------------------------------
  RETDesc(); 
  RETDesc(BindWA *bindWA);
  RETDesc(BindWA *bindWA, const RETDesc& sourceTable); 
  RETDesc(BindWA *bindWA, TableDesc *tdesc, CorrName *newCorrName = NULL);
  RETDesc(BindWA *bindWA, RoutineDesc *rdesc, QualifiedName *newQualName = NULL);
  ~RETDesc();

  // ---------------------------------------------------------------------
  // A list of the tables described by this RETDesc (in the FROM clause
  // of this result table).
  // Only one name (the canonical name) for each table ref appears in the list.
  // If the includePartitionName flag is set then then the partition name, 
  // or partition number, or location name (only one of these can be specified by user)
  // specified by user will be returned as part of the table name. 
  // (if these extensions have been specified by the user for the table of interest)
  // ---------------------------------------------------------------------
  void getTableList(LIST(TableNameMap*) &xtnmList,
		    NAString *textList = NULL) const;
  static void formatTableList(LIST(TableNameMap*) &xtnmList,
			      NAString *textList,
			      NABoolean includePartitionName = FALSE) /*const*/;

  NABoolean isEmpty() const	{ return xtnm_.isEmpty() && xcnm_.isEmpty(); }

  // ---------------------------------------------------------------------
  // The number of user columns in the result table.
  // ---------------------------------------------------------------------
  CollIndex getDegree() const { return userColumnList_.entries(); }

  // ---------------------------------------------------------------------
  // Get BindWA backpointer, as a non-const reference so can be used as lvalue.
  // Perhaps simpler just to make RETDescList a friend class ...
  // ---------------------------------------------------------------------
  BindWA *& getBindWA() { return bindWA_; }

  // ---------------------------------------------------------------------
  // A list of the result table's user columns.
  // ---------------------------------------------------------------------
  const ColumnDescList *getColumnList() const { return &userColumnList_; }

  // ---------------------------------------------------------------------
  // A list of the result table's system columns.
  // ---------------------------------------------------------------------
  const ColumnDescList *getSystemColumnList() const
  {
    return &systemColumnList_;
  }

  const XTNM &getXTNM() const {return  xtnm_;}

  // ---------------------------------------------------------------------
  // A list of the user columns that match a given qualifier.
  // Note that the TableNameMap entry may exist but the column list be empty,
  // either because BindRelExpr.C passed in an empty list, or due to
  // enforcement of ANSI 6.3 SR4 in XTNM::insertNames.
  // ---------------------------------------------------------------------
  ColumnDescList *getQualColumnList(const CorrName& tableName) const
  {
    const TableNameMap *table = xtnm_.get(&tableName);
    return (table != NULL)
           ? table->getColumnList()
           : NULL;
  }


  // ---------------------------------------------------------------------
  // The name of the user column at the given position.
  // ---------------------------------------------------------------------
  const ColRefName& getColRefNameObj(const CollIndex i) const
  {
    return userColumnList_[i]->getColRefNameObj();
  }

  // ---------------------------------------------------------------------
  // The ValueId of the user column at the given position.
  // ---------------------------------------------------------------------
  ValueId getValueId(const CollIndex i) const
  {
    return userColumnList_[i]->getValueId();
  }

  // ---------------------------------------------------------------------
  // The NAType of the user column at the given position.
  // ---------------------------------------------------------------------
  const NAType &getType(const CollIndex i)
  {
    if (naTypeForUserColumnList_.entries() > 0)
      return *naTypeForUserColumnList_[i];
    else
      return userColumnList_[i]->getValueId().getType();
  }

  // create a list of NATypes corresponding to each entry in the
  // userColumnList_ in RETDesc. Used by generator to convert to
  // this type during output expr code gen.
  NABoolean createNATypeForUserColumnList(CollHeap * heap);

  void changeNATypeForUserColumnList(CollIndex index, const NAType * newType);

  // ---------------------------------------------------------------------
  // The heading of the user column at the given position.
  // ---------------------------------------------------------------------
  const char *getHeading(const CollIndex i) const
  {
    return userColumnList_[i]->getHeading();
  }

  // ---------------------------------------------------------------------
  // Get/set the flags.
  // ---------------------------------------------------------------------
  NABoolean isGrouped() const { return groupedFlag_; }
  void setGroupedFlag() { groupedFlag_ = TRUE; }

  NABoolean isRoutine() const { return routineFlag_; }
  void setRoutineFlag() { routineFlag_ = TRUE; }

  // ---------------------------------------------------------------------
  // A ValueIdList containing the ValueId of each user column.
  // ---------------------------------------------------------------------
  void getValueIdList(ValueIdList &vidList,
		      const ColumnClass colClass = USER_COLUMN) const
  {
    switch (colClass) {
    case USER_COLUMN:
      userColumnList_.getValueIdList(vidList);
      break;
    case SYSTEM_COLUMN:
      systemColumnList_.getValueIdList(vidList);
      break;
    case USER_AND_SYSTEM_COLUMNS:
      userColumnList_.getValueIdList(vidList);
      systemColumnList_.getValueIdList(vidList);
      break;
    default:
      CMPASSERT(FALSE);
    }
  }

  // ---------------------------------------------------------------------
  // Add a column to the table descriptor.
  // ---------------------------------------------------------------------
  void addColumn(BindWA *bindWA,
		 const ColRefName& colRefName,
                 const ValueId valId,
                 const ColumnClass colClass = USER_COLUMN,
		 const char * heading = NULL);

  // ---------------------------------------------------------------------
  // Delete a column from the table descriptor.
  // ---------------------------------------------------------------------
  void delColumn(BindWA *bindWA,			// could be const
		 const ColRefName& colRefName,
                 const ColumnClass colClass);

  // ---------------------------------------------------------------------
  // Add several columns to the table descriptor.
  // ---------------------------------------------------------------------
  void addColumns(BindWA *bindWA,
		  const ColumnDescList& columnList,
                  const ColumnClass colClass = USER_COLUMN,
		  const CorrName *newCorrName = NULL)
  {
    for (CollIndex i = 0; i < columnList.entries(); i++)
      addColumn(bindWA,
		newCorrName
		  ? ColRefName(columnList[i]->getColRefNameObj().getColName(),
			       *newCorrName)
		  : columnList[i]->getColRefNameObj(),
                columnList[i]->getValueId(),
                colClass,
		columnList[i]->getHeading());
  }

  void addColumns(BindWA *bindWA, 
		  const RETDesc& sourceTable,
		  const CorrName *newCorrName = NULL)
  {
    addColumns(bindWA, *sourceTable.getColumnList(), USER_COLUMN, newCorrName);
    addColumns(bindWA, *sourceTable.getSystemColumnList(), SYSTEM_COLUMN, newCorrName);
  }

  // ---------------------------------------------------------------------
  // Null-instantiate and add columns from columnList into the "this" RETDesc.
  // ---------------------------------------------------------------------
  void nullInstantiateAndAddColumns(BindWA *bindWA,
				    NABoolean forceCast,
				    const ColumnDescList& columnList,
				    const ColumnClass colClass = USER_COLUMN);

  // ---------------------------------------------------------------------
  // Null-instantiate columns from const "this" into a new returned RETDesc,
  // also appending list of affected columns to nullOutputList.
  // ---------------------------------------------------------------------
  RETDesc *nullInstantiate(BindWA *bindWA,
			   NABoolean forceCast,
			   ValueIdList& nullOutputList) const;

  // ---------------------------------------------------------------------
  // Lookup the given column name.
  // ---------------------------------------------------------------------
  ColumnNameMap *findColumn(const ColRefName &name) const
  {
    return xcnm_.get(&name);
  }

  ColumnNameMap *findColumn(const NAString &simpleColNameStr) const
  {
    ColRefName name(simpleColNameStr);
    return findColumn(name);
  }

  ColumnNameMap *findColumn(const ValueId vid) const;

        // find an entry whose vid matches and is either
        // 1. fully-qualified and whose vid matches ...or...
        // 2. is a fabricated name

  // MVs --
  // Propaget the Op@ and SYKEY columns from a lower scope.
  void propagateColumn(BindWA		 *bindWA, 
		       const ColRefName&  colName,
		       NABoolean	  isFromRoot,
		       ColumnClass        colClass);
  void propagateOpAndSyskeyColumns(BindWA *bindWA, NABoolean isFromRoot);

  // ---------------------------------------------------------------------
  // Display/print, for debugging.
  // ---------------------------------------------------------------------
  static void displayDown(RelExpr *);

  void display() const;

  void print(FILE* ofd = stdout,
	     const char* indent = DEFAULT_INDENT,
             const char* title = "RETDesc") const;

private:

  // ---------------------------------------------------------------------
  // Create a column descriptor, insert it into the column list, and add
  // the exposed column name(s) to the hash table.
  // ---------------------------------------------------------------------
  void addColumnDesc(BindWA *bindWA,			// could be const
		     const ColRefName& colRefName,
                     const ValueId valId,
		     const char * heading,
                     ColumnDescList& columnList);

  // ---------------------------------------------------------------------
  // Delete a column descriptor from the column list and the hash table.
  // ---------------------------------------------------------------------
  void delColumnDesc(BindWA *bindWA,			// could be const
		     const ColRefName& colRefName,
                     ColumnDescList& columnList);

  // ---------------------------------------------------------------------
  // A list of the user column descriptors for this table descriptor.
  // Also used as the list of output columns from a Routine.
  // ---------------------------------------------------------------------
  ColumnDescList userColumnList_;

  // ---------------------------------------------------------------------
  // A list of NATypes corresponding to the entries of userColumnList_.
  // Set after RelRoot is bound in RelRoot::bindNode.
  // Used by generator at select list output time to convert to these
  // types as the type of the corresponding value id in userColumnList_
  // may have changed between bind and code generate phases.
  // ---------------------------------------------------------------------
  NAList<NAType*> naTypeForUserColumnList_;

  // ---------------------------------------------------------------------
  // A list of the system column descriptors for this table descriptor.
  // ---------------------------------------------------------------------
  ColumnDescList systemColumnList_;

  // ---------------------------------------------------------------------
  // A list of the column descriptors by qualifier.
  // ---------------------------------------------------------------------
  XTNM xtnm_;

  // ---------------------------------------------------------------------
  // A hash table of the column names and their ValueId's.
  // ---------------------------------------------------------------------
  XCNM xcnm_;

  // ---------------------------------------------------------------------
  // Grouped flag - TRUE if the table is a result of a GROUP BY.
  // ---------------------------------------------------------------------
  NABoolean groupedFlag_;

  // ---------------------------------------------------------------------
  // Routine flag - TRUE if the table is a result of Outputs from a routine.
  // ---------------------------------------------------------------------
  NABoolean routineFlag_;

  // ---------------------------------------------------------------------
  // Backpointer to the creating BindWA
  // ---------------------------------------------------------------------
  BindWA *bindWA_;

}; // class RETDesc

// ***********************************************************************
// RETDescList : A list of RETDescs
// ***********************************************************************
class RETDescList : public LIST(RETDesc *)
{
public:

  RETDescList(CollHeap* h/*=0*/) : LIST(RETDesc *)(h) {}
 
  ~RETDescList() { clearAndDestroy(); }
 
  // Remove all descriptors from the list and call their destructors
  void clearAndDestroy()
  {
    for (CollIndex i = 0; i < entries(); i++) {
      at(i)->getBindWA() = NULL;
      delete at(i);
    }
    clear();
  }

}; // class RETDescList

#endif /* RETDESC_H */
