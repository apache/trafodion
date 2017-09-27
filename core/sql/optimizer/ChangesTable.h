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
#ifndef CHANGESTABLE_H
#define CHANGESTABLE_H
/* -*-C++-*-
******************************************************************************
*
* File:         ChangesTable.h
* Description:  Insertion, scanning and deleting from a changes table
*               such as the triggers temporary table, and the MV log.
* Created:      10/10/2000
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "ObjectNames.h"
#include "Triggers.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------

class ChangesTable;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class BindWA;
class ColReference;
class TableDesc;
class Scan;
class Union;
class UpdateColumns;
class GenericUpdate;
class SQLInt;
class NRowsClause;
class DeltaDefinition;
class MVInfoForDML;

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class ChangesTable : public NABasicObject
{
public:
  enum RowsType { NONE                   = 0x00000000,
		  ALL_ROWS               = 0xFFFFFFFF,
		  INSERTED_ROWS          = 0x00000001,
		  DELETED_ROWS           = 0x00000002,
		  PHASE1_ROWS            = 0x00000004,
		  CATCHUP_ROWS           = 0x00000008,
		  ALL_MULTI_TXN_CTX_ROWS = 0x00000010
		};

  // The value to increment each row by in addition to the timestamp value
  // when computing the @TS column
  enum RowTsCounter { ROW_TS_INCR = 3 };

  virtual ~ChangesTable() {}


  // Accessors
  inline const CorrName *getTableName()         const { return tableCorr_; }
  inline const CorrName& getSubjectTableName()  const { return subjectTable_; }
  inline const NATable  *getNaTable()           const { return naTable_; }
  inline const NATable  *getSubjectNaTable()    const { return subjectNaTable_; }
  inline OperatorTypeEnum getOpType()           const { return opType_; }
  inline const CorrName& getCorrNameForNewRow() const { return corrNameForNewRow_; }
  inline const CorrName& getCorrNameForOldRow() const { return corrNameForOldRow_; }
  
  // Work methods
  RelExpr *buildInsert(NABoolean useLeafInsert,
		       Lng32 enforceRowsTypeForUpdate=ALL_ROWS,
		       NABoolean isUndo= FALSE,
		       NABoolean isForMvLogging= FALSE
                       ) const;
  RelExpr *buildUndo() const ;
  RelExpr *buildDelete(RowsType whichRows=ALL_ROWS) const;
  Scan    *buildScan(RowsType type) const;
  RelExpr *buildOldAndNewJoin() const;
  RelExpr *transformScan() const;

  ItemExpr *buildBaseColsSelectList(const CorrName& tableName) const;
  
  // build a list of all columns that correspond to a column in the base table
  ItemExpr *buildColsCorrespondingToBaseSelectList() const;

  ItemExpr *buildClusteringIndexVector(const NAString *corrName = NULL,
                                       NABoolean useInternalSyskey = FALSE) const;

  // build a list for renaming column names to their equivalent names in the
  // base table
  ItemExpr *buildRenameColsList() const;

  inline void addCorrelationName(const NAString& correlationName) 
    { tableCorr_->setCorrName(correlationName); }
  static void addSuffixToTableName(NAString& tableName, const NAString& suffix);
  static CorrName* buildChangesTableCorrName(const QualifiedName& tableName,
                                             const NAString&      suffix,
					     ExtendedQualName::SpecialTableType tableType,
					     CollHeap            *heap);

protected:
  // This is an abstract class, so the Ctors can only be called by the 
  // sub-classes.
  ChangesTable(const CorrName&   name, 
	       OperatorTypeEnum  opType,
	       BindWA           *bindWA,
	       RowsType          scanType = NONE);

  ChangesTable(const GenericUpdate *baseNode, 
	       BindWA           *bindWA);

  ChangesTable(Scan             *baseNode, 
	       RowsType          scanType, 
	       BindWA           *bindWA);

  // This method is called from the Ctor of sub-classes to initialize 
  // data members that cannot be initialized from inside the Ctor.
  void initialize();

  // This method sets the type of the Union above the Tuples in update
  // operations.
  virtual void setTuplesUnionType(Union *unionNode) const {};

  // Pure virtual methods implemented by sub-classes.
  // These methods are called by buildInsert().
  virtual ItemExpr *createAtColExpr(const NAColumn *naColumn, 
				    NABoolean       isInsert,
				    NABoolean       isUpdate,
				    NABoolean isUndo = FALSE) const = 0;
  virtual ItemExpr *createBaseColExpr(const NAString& colName, 
				      NABoolean       isInsert,
				      NABoolean       isUpdate) const;
  virtual ItemExpr *createSyskeyColExpr(const NAString& colName, 
				        NABoolean       isInsert) const = 0;
  
  virtual CorrName *calcTargetTableName(const QualifiedName &tableName) const = 0;
  virtual ItemExpr *createSpecificWhereExpr(RowsType type) const = 0;
  virtual NABoolean isEmptyDefaultTransitionName() const = 0;
  virtual NABoolean supportsLateBinding() const = 0;

  Scan		   *scanNode_;
  CorrName          subjectTable_;
  CorrName         *tableCorr_;
  const NATable    *naTable_;
  const NATable    *subjectNaTable_;
  OperatorTypeEnum  opType_;
  RowsType          scanType_;
  CorrName          corrNameForNewRow_;
  CorrName          corrNameForOldRow_;
  BindWA           *bindWA_;
  CollHeap         *heap_;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class TriggersTempTable : public ChangesTable
{
public:
  TriggersTempTable(const GenericUpdate *baseNode, 
		    BindWA              *bindWA);

  TriggersTempTable(const CorrName &subjectTableName,
		    Scan           *baseNode, 
		    RowsType        scanType, 
		    BindWA         *bindWA);

  virtual ~TriggersTempTable() {}

  inline void setBeforeTriggersExist() { beforeTriggersExist_ = TRUE; }
  inline NABoolean getBeforeTriggersExist() const { return beforeTriggersExist_; }

  RelExpr *transformScan() const;

  virtual NABoolean isEmptyDefaultTransitionName() const { return FALSE; }

  virtual ItemExpr *createAtColExpr(const NAColumn *naColumn, 
				    NABoolean       isInsert,
				    NABoolean       isUpdate,
				    NABoolean       isUndo = FALSE
				    ) const;
  virtual ItemExpr *createSyskeyColExpr(const NAString& colName,
					NABoolean       isInsert) const;

  virtual CorrName *calcTargetTableName(const QualifiedName &tableName) const;

  virtual ItemExpr *createSpecificWhereExpr(RowsType type) const;

  virtual NABoolean supportsLateBinding() const { return FALSE; }

  // build the block to replace the scan in the MV tree
  RelExpr *buildScanForMV() const;

  // For triggers transformation - use the existing exec id.
  void setBoundExecId(ItemExpr *execId) { boundExecId_ = execId; }

private:
  NABoolean beforeTriggersExist_;
  ItemExpr *boundExecId_;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class MvIudLog : public ChangesTable
{
public:
  MvIudLog(CorrName&         name, 
	   BindWA           *bindWA, 
	   OperatorTypeEnum  opType);

  MvIudLog(const GenericUpdate *baseNode, 
	   BindWA              *bindWA);

  virtual ~MvIudLog() {}

  MVInfoForDML	  *getMvInfo()		 const { return mvInfo_; } 

  // not used for this templog type
  virtual NABoolean isEmptyDefaultTransitionName() const { return TRUE; }

  virtual NABoolean supportsLateBinding() const { return TRUE; }

  virtual ItemExpr *createAtColExpr(const NAColumn *naColumn, 
				    NABoolean       isInsert,
				    NABoolean       isUpdate,
				    NABoolean       isUndo=FALSE) const;

  virtual ItemExpr *createSyskeyColExpr(const NAString& colName,
					NABoolean       isInsert) const;  

  virtual CorrName *calcTargetTableName(const QualifiedName &tableName) const;

  virtual ItemExpr *createSpecificWhereExpr(RowsType type) const;

  static ItemExpr *buildLogEpochPredicate(const DeltaDefinition *deltaDef,
                                          CollHeap              *heap);

  inline void setMvInfo(MVInfoForDML *mvInfo) { mvInfo_ = mvInfo; }

  inline NABoolean needsRangeLogging() const
    { return needsRangeLogging_; }
  inline void setUpdatedColumns(UpdateColumns *updatedColumns)
    { updatedColumns_ = updatedColumns; }
  inline void setDeltaDefinition(DeltaDefinition *deltaDef)
    { deltaDef_ = deltaDef; }

  inline NABoolean getAddOrigScanPredicateFlag() const 
    { return addOrigScanPredicate_; }
  inline void setAddOrigScanPredicateFlag(NABoolean flag) 
    { addOrigScanPredicate_=flag; }
  NABoolean fixReferencesFromBaseTableToLog(ItemExpr *expr) const;

  ItemExpr *buildIndirectUpdateExpression() const;

protected:
  virtual ItemExpr *createColExprForEpoch() const;
  virtual ItemExpr *createColExprForOpType(NABoolean isInsert,
					   NABoolean isUpdate,
					   NABoolean isUndo =FALSE) const;
  virtual ItemExpr *createColExprForIgnore() const;
  virtual ItemExpr *createColExprForBitmap(const NAColumn *naColumn,
					   NABoolean isUpdate) const;
  virtual ItemExpr *createColExprForRangeSize(NABoolean isInsert) const;
  virtual ItemExpr *createColExprForAtSyskey(NABoolean isInsert) const;
  virtual ItemExpr *createColExprForAtTimestamp(NABoolean isInsert,
                                                NABoolean isUpdate,
                                                NABoolean isUndo) const;

  // Build a predicate that would select only lines that affect at least 
  // one of the columns of the current mv
  virtual ItemExpr *buildBitmapColPredicate() const;

  ItemExpr *constructUpdateBitmapFromList(const LIST(Lng32) &columnList) const;

  ItemExpr *buildOrigScanPredicate() const;

  // For mvIudLog, the union above Tuples must be ordered.
  virtual void setTuplesUnionType(Union *unionNode) const;

  DeltaDefinition *deltaDef_;

private:
  // Called by the Ctors only.
  void initRangeLogging();

  NABoolean        needsRangeLogging_;
  UpdateColumns   *updatedColumns_;
  NABoolean        addOrigScanPredicate_;
  MVInfoForDML	  *mvInfo_;
};

//////////////////////////////////////////////////////////////////////////////
// This class is used by the MVLOG command for inserting manual ranges
// into the log.
//////////////////////////////////////////////////////////////////////////////
class MvIudLogForMvLog : public MvIudLog
{
public:
  MvIudLogForMvLog(CorrName&  tableName, 
	           BindWA    *bindWA);

  virtual ~MvIudLogForMvLog() {}

  virtual ItemExpr *createColExprForEpoch() const;
  virtual ItemExpr *createColExprForOpType(NABoolean isInsert,
					   NABoolean isUpdate,
					   NABoolean isUndo = FALSE) const;
  virtual ItemExpr *createColExprForIgnore() const;
  virtual ItemExpr *createColExprForBitmap(const NAColumn *naColumn,
					   NABoolean isUpdate) const;
  virtual ItemExpr *createColExprForRangeSize(NABoolean isInsert) const;
  virtual ItemExpr *createColExprForAtSyskey(NABoolean isInsert) const;

  virtual ItemExpr *createBaseColExpr(const NAString& colName, 
				      NABoolean       isInsert,
				      NABoolean       isUpdate) const;

  virtual ItemExpr *createColExprForAtTimestamp(NABoolean isInsert,
                                                NABoolean isUpdate,
                                                NABoolean isUndo) const;

private:
};


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class MvLogForContextRows : public ChangesTable
{
public:
  MvLogForContextRows(const QualifiedName&  baseTableName,
		      const CorrName&       mvName,
		      Lng32                  epochNumber,
		      ItemExpr		   *catchupNo,
		      BindWA               *bindWA);

  virtual ~MvLogForContextRows() {}

  // not used for this templog type
  virtual NABoolean isEmptyDefaultTransitionName() const { return TRUE; }

  virtual NABoolean supportsLateBinding() const { return FALSE; }

  virtual ItemExpr *createAtColExpr(const NAColumn *naColumn, 
				    NABoolean       isInsert,
				    NABoolean       isUpdate,
				    NABoolean isUndo = FALSE) const;

  virtual ItemExpr *createSyskeyColExpr(const NAString& colName,
					NABoolean       isInsert) const;

  virtual CorrName *calcTargetTableName(const QualifiedName &tableName) const;

  virtual ItemExpr *createSpecificWhereExpr(RowsType type) const;

  static ItemExpr *buildLogEpochPredicate(const DeltaDefinition *deltaDef,
                                          CollHeap              *heap);

private:
  Lng32      epochNumber_;
  ItemExpr *catchupNo_;
};


#endif
