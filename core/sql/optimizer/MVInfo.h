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
#ifndef MVINFO_H
#define MVINFO_H

/* -*-C++-*-
******************************************************************************
*
* File:         MVInfo.h
* Description:  Definition of class MVInfo.
*
* Created:      5/09/99
* Language:     C++
* Status:       $State: Exp $
*
*
******************************************************************************
*/



#include <ComSmallDefs.h>
#include <Collections.h>
#include "QRDescriptor.h"
#include "charinfo.h"

//++ MV - for showddl
class CatROMV;
class StmtDDLCreateMV;
// - for clustering key columns in MVInfoForDDL
class ElemDDLColRefArray;
class ElemDDLStoreOptKeyColumnList;
//-- MV

class Scan;
class Join;
class GroupByAgg;
class RelRoot;
class RenameTable;
class ItemExpr;
class QualifiedName;
class Refresh;
class MvRefreshBuilder;
class VEG;
class MVJoinGraph;
class NATable;
class TableDesc;
class QRMVDescriptor;
class XMLString;

class MVInfo;
class MVInfoForDDL;
class MVInfoForDML;
class MVUsedObjectInfo;
class MVColumnInfo;
class ViewColumnNode;
class ViewColumnGraph;
class DeltaDefinitionPtrList;
class ColIndList;

// Incremental aggregate functions.
// There is a data member of this type in MVColumnInfo, and it is saved
// in the SMD tables.
enum MVAggFunc { 
  AGG_COUNT=0, AGG_SUM, AGG_AVG, AGG_MIN, AGG_MAX, 
  AGG_STDDEV, AGG_VARIANCE, AGG_STDDEVW, AGG_VARIANCEW, 
  AGG_OTHER, AGG_COUNTSTAR };

// These two data types are used for adding dependent aggregate columns
// in MAVs. They are used by classes MVInfoForDDL and MVColumnInfo.
// See details in code of MVInfoForDDL::addMavSystemColumns().
typedef ARRAY(MVColumnInfo *) AggregateArray;
typedef NAHashDictionary<NAString, AggregateArray> ExpressionHash;

// This hash is used by areTablesUsedOnlyOnce() for detecting when a base
// table is used more than once in an expanded tree.
typedef NAHashDictionary<const QualifiedName, const MVUsedObjectInfo> TableNameHash;
//----------------------------------------------------------------------------
//  enum: MV_NOT_INCREMENTAL
//
//  description:
//    This enumeration is used to identify reasons why 
//    an mv is set to not incremental (see MvInfoForDDL::setNotIncremental).  
//
//    This provides for more detailed error messages for mvs.
//----------------------------------------------------------------------------
typedef enum
{
  MVNI_DEFAULT = 0, 
  MVNI_ON_VIEW,
  MVNI_NO_COLUMNS,
  MVNI_TABLE_REUSED,
  MVNI_NON_REPEATABLE_EXPRESSION,   
  MVNI_MULTI_GROUP_BY_CLAUSES, 
  MVNI_HAVING_CLAUSE, 
  MVNI_JOIN_TREE_NOT_LEFT_LINEAR, 
  MVNI_JOIN_GRAPH_NOT_CONNECTED, 
  MVNI_COLUMN_NOT_IN_GROUPING_COLUMNS,
  MVNI_NON_EQUAL_JOIN_PREDICATE,
  MVNI_MULTIPLE_COLUMN_JOIN_PREDICATE,
  MVNI_AGGREGATE_NOT_TOP_LEVEL,
  MVNI_DISTINCT,
  MVNI_NUM_REASONS 
} MV_NOT_INCREMENTAL;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// The ViewColumnConnection, ViewTableConnection and ViewColumnGraph classes
// are used during CREATE MV, to map column names from base tables to the
// top most view on that table. It is used when adding needed system columns
// (MJV clustering key columns, and MAV group by columns).
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// class ViewColumnConnection holds the information of the mapping between a
// single view column and the expression that it represents.
class ViewColumnConnection : public NABasicObject
{
public:
  // The parameter is a column descriptor from the RenameTable node on top of
  // a view. It has both the name of the view column, and the ValueId of the
  // mapped expression.
  ViewColumnConnection(const ColumnDesc *colDesc, CollHeap *heap);

  virtual ~ViewColumnConnection() {}

  // operator== required for use in templates
  NABoolean operator==(const ViewColumnConnection &other) const; 

  // Accessors
  const NAString *getColName() const 
    { return key_; }
  const QualifiedName& getTableName() const 
    { return baseColName_.getCorrNameObj().getQualifiedNameObj(); }
  const QualifiedName& getViewName() const 
    { return viewColName_.getCorrNameObj().getQualifiedNameObj(); }
  const ColRefName& getViewColName() const 
    { return viewColName_; }
  const ValueId *getValueId() const
    { return &vid_; }
  NABoolean isComplex() const
    { return isComplex_; }
  NABoolean isInvalid() const
    { return isInvalid_; }

private:
  // Copy Ctor and = operator are not implemented.
  ViewColumnConnection(const ViewColumnConnection& other);
  ViewColumnConnection& operator=(const ViewColumnConnection& other);

  ColRefName	    baseColName_; // The name of the column in the base table.
  const ColRefName  viewColName_; // The name of the column in the view.
  const ValueId	    vid_;         // The column ValueId
  NABoolean	    isComplex_;   // Is the view column mapped to more than
                                  // a single base table column.
  NABoolean         isInvalid_;   // TRUE if no base table column is used by
                                  // this view column.
  NAString         *key_;         // The hash key string - the column name.
};  // class ViewColumnConnection

//----------------------------------------------------------------------------
// The ViewTableConnection class holds the information connecting a base table
// to the views that are defined on top of it.
class ViewTableConnection : public NABasicObject
{
public:

  ViewTableConnection(const QualifiedName& tableName,
		      const QualifiedName& viewName,
		      CollHeap            *heap)
  :   tableName_(tableName, heap),
      topViewName_(viewName, heap),
      colConnections_(hashKey, // Pass NAString::hashKey as function ptr
                      NAHashDictionary_Default_Size,
                      FALSE,    // Do not enforce uniqueness
                      heap)
  {}

  ~ViewTableConnection();

  // Add a column mapping to the view mapping.
  void insert(const ViewColumnConnection *col);

  // Accessors
  const QualifiedName& getTableName()   const { return tableName_; }
  const QualifiedName& getTopViewName() const { return topViewName_; }
  NABoolean            contains(const ColRefName& colName) const;
  const ColRefName&    findTopViewOf(const ColRefName& colName) const;
  NABoolean            isMaskedColumn(const ColRefName& col) const;
  NABoolean            isMaskedTable(const QualifiedName& tableName) const;

  // Display/print, for debugging.
#ifndef NDEBUG
  void display() const;
  void print(FILE* ofd = stdout,
	     const char* indent = DEFAULT_INDENT,
	     const char* title = "ViewTableConnection") const;
#endif

private:
    // Copy Ctor and = operator are not implemented.
  ViewTableConnection(const ViewTableConnection& other);
  ViewTableConnection& operator=(const ViewTableConnection& other);

  typedef NAHashDictionary<const NAString, const ViewColumnConnection> columnConnection;

  // Using the base table column name, find the view column name.
  const ViewColumnConnection *findColumn(const ColRefName& colName) const;

  const QualifiedName	tableName_;      // The name of the base table.
  QualifiedName		topViewName_;    // We only need the top most view.
  columnConnection	colConnections_; // A hash of all the columns by name.
};  // class ViewTableConnection

//----------------------------------------------------------------------------
// The view mappings of all the base tables used in this query.
class ViewColumnGraph: public NABasicObject
{
public:
  ViewColumnGraph(CollHeap *heap)
  : tableConnections_(heap),
    columnValueIds_(ValueIdHash, NAHashDictionary_Default_Size, FALSE, heap),
    heap_(heap)
  {}

  ~ViewColumnGraph();

  NABoolean contains(const ColRefName& colName) const;
  const ColRefName& findTopViewOf(const ColRefName& colName) const;

  // A masked column is one that does not have a mapping to the top most view.
  NABoolean isMaskedColumn(const NAColumn *keyCol) const;
  // A masked table is one that is not visible at the top level, because it
  // is masked by a view.
  NABoolean isMaskedTable(const QualifiedName& tableName) const;

  ViewTableConnection *findTableConnection(const QualifiedName& tableName) const;

  const ColRefName *findViewNameFor(const ValueId vid) const;
  static ULng32 ValueIdHash(const ValueId& vid);

  void addTableMapping(RETDesc *retDesc, NABoolean addSysCols, CollHeap *heap);

  // Display/print, for debugging.
#ifndef NDEBUG
  void display() const;
  void print(FILE* ofd = stdout,
	     const char* indent = DEFAULT_INDENT,
	     const char* title = "ViewColumnGraph") const;
#endif

private:
    // Copy Ctor and = operator are not implemented.
  ViewColumnGraph(const ViewColumnGraph& other);
  ViewColumnGraph& operator=(const ViewColumnGraph& other);

  void addConnection(ViewColumnConnection *colConnection, CollHeap *heap);
  const ViewColumnConnection *findColumn(const ValueId vid) const;

  typedef NAHashDictionary<const ValueId, const ViewColumnConnection> valueIdConnection;

  LIST (ViewTableConnection *) tableConnections_;
  valueIdConnection            columnValueIds_;
  CollHeap                    *heap_;
};  // class ViewColumnGraph

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// The MVVegPredicateColumn and MVVegPredicate classes represent equality
// between columns of base tables used by the MV. This information is saved 
// in the SMD tables.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// The MVVegPredicateColumn class represents a base table column or expression
// that is part of a join equal predicate.
class MVVegPredicateColumn : public NABasicObject
{
public:
  // Detailed Ctor for DML
  MVVegPredicateColumn(const QualifiedName&  tableName,
		       Lng32		     colNumber,
		       NABoolean	     isComplex,
		       ComLeftJoinTableType  joinSide,
		       ComMVSUsageType	     usageType,
		       const NAString&       text, CharInfo::CharSet textCharSet,
		       CollHeap		    *heap)
  : tableName_(tableName, heap),
    colNumber_(colNumber),
    isComplex_(isComplex),
    joinSide_ (joinSide),
    complexExpr_(NULL),
    usageType_(usageType),
    textCharSet_(textCharSet),
    text_(text, heap)
  {}

  // Ctor for DDL
  MVVegPredicateColumn(const BaseColumn  *baseCol, 
		       NABoolean	  isInLeftJoin,
		       NABoolean	  isFromVegRef,
		       ComMVSUsageType	  usageType,
		       const ItemExpr	 *complexExpr,
		       CollHeap	         *heap);

  // copy ctor
  MVVegPredicateColumn(const MVVegPredicateColumn& other, 
		       CollHeap                   *heap,
		       NABoolean  joinSideOverride = FALSE)
  : tableName_(other.tableName_, heap),
    colNumber_(other.colNumber_),
    isComplex_(other.isComplex_),
    joinSide_ (joinSideOverride ? COM_LEFT_OUTER : other.joinSide_),
    complexExpr_(other.complexExpr_),
    usageType_(other.usageType_),
    textCharSet_(other.textCharSet_),
    text_(other.text_, heap)
  {}

  // Accessors
  const QualifiedName&  getTableName() const { return tableName_; }
  Lng32			getColNumber() const { return colNumber_; }
  NABoolean		isComplex()    const { return isComplex_; }
  ComLeftJoinTableType  getJoinSide()  const { return joinSide_; }
  ComMVSUsageType	getUsageType() const { return usageType_; }
  const NAString	getText()      const { return text_; }
  CharInfo::CharSet	getTextCharSet() const { return textCharSet_; }
  
  NABoolean isLeftInnerTable() const { return joinSide_ == COM_LEFT_INNER; }
  NABoolean isLeftOuterTable() const { return joinSide_ == COM_LEFT_OUTER; }

  // Display/print, for debugging.
#ifndef NDEBUG
  void display() const;
  void print(FILE* ofd = stdout,
	     const char* indent = DEFAULT_INDENT,
	     const char* title = "MVVegPredicateColumn") const;
#endif

private:
  // Prevent accidental use of default copy Ctor and = operator.
  MVVegPredicateColumn& operator=(const MVVegPredicateColumn& other);
  MVVegPredicateColumn(const MVVegPredicateColumn& other);

  const QualifiedName		tableName_;
  const Lng32			colNumber_;
  const NABoolean		isComplex_;
  const ComLeftJoinTableType	joinSide_;
  const ItemExpr	       *complexExpr_;
  const ComMVSUsageType		usageType_;
  NAString			text_;
  CharInfo::CharSet		textCharSet_;
};  // class MVVegPredicateColumn

//////////////////////////////////////////////////////////////////////////////
// An MVVegPredicate represents the equality of two or more 
// MVVegPredicateColumn objects.
class MVVegPredicate : public LIST(const MVVegPredicateColumn *)
{
public:
  // The usage type can be either specifically specified, or initialized to 
  // COM_UNKNOWN_USAGE and then set when inserting the first column.
  MVVegPredicate(NABoolean        isOnLeftJoin, 
                 CollHeap        *heap,
		 ComMVSUsageType  usageType = COM_UNKNOWN_USAGE)
  :  LIST(const MVVegPredicateColumn *)(heap),
     isOnLeftJoin_(isOnLeftJoin),
     isIncremental_(TRUE),
     usageType_(usageType)
  {}

  // Copy Ctor.
  MVVegPredicate(const MVVegPredicate& other, CollHeap *heap);

  virtual ~MVVegPredicate();

  NABoolean isOnLeftJoin()  const { return isOnLeftJoin_; }
  NABoolean isIncremental() const { return isIncremental_; }

  // If this VegPredicate is on tableName/colPosition, return its index.
  // otherwise return -1.
  Lng32  findIndexFor(const QualifiedName& tableName, Lng32 colPosition) const;

  // Return the index of some other column on this veg, that covers the one 
  // on colIndex.
  Lng32  getCoveringColFor(Lng32 colIndex, const MVInfo *mvInfo) const;

  // Get one of the VEG columns, that is not complex.
  const MVVegPredicateColumn *getRepresentativeCol() const;

  // Used both in DDL (for creating the used columns bitmap), and in DML (for
  // building the join graph).
  void  markPredicateColsOnUsedObjects(MVInfo *mvInfo, NABoolean isDDL);

  // Build an MVVegPredicateColumn object and insert it into the list.
  void addPredicateMember(ValueId	      vid, 
			  NABoolean	      isFromVegRef,
			  const VEG	     *parentVeg,
			  LIST(const VEG *)&  vegPtrList,
			  CollHeap           *heap);

  // Add all the columns of the VEG to the list.
  void addVegPredicates(const VEG	  *veg, 
		        NABoolean	   isFromVegRef,
		        LIST(const VEG *)& vegPtrList,
			CollHeap          *heap);

  void insert(const MVVegPredicateColumn *);

  // Display/print, for debugging.
#ifndef NDEBUG
  void display() const;
  void print(FILE* ofd = stdout,
	     const char* indent = DEFAULT_INDENT,
	     const char* title = "MVVegPredicate") const;
#endif

private:
  // Prevent accidental use of default copy Ctor and = operator.
  MVVegPredicate& operator=(const MVVegPredicate& other);
  MVVegPredicate(const MVVegPredicate& other);

  // Is this predicate from a left join?
  const NABoolean	isOnLeftJoin_;  
  // Does this predicate violate incremental refresh rules?
  NABoolean		isIncremental_; 
  // Is the predicate from a direct or expanded tree?
  ComMVSUsageType	usageType_;     
};  // class MVVegPredicate

//////////////////////////////////////////////////////////////////////////////
// This class is used to store a collection of flags and attributes of used
// objects, that are needed by the catalog manager.
class MVUsedObjectCatmanFlags : public NABasicObject
{
public:
  MVUsedObjectCatmanFlags() 
  :  objectAttributes_(COM_NO_ATTRIBUTE),
     usageType_(COM_USER_SPECIFIED),
     mvStatus_(COM_MVSTATUS_UNKNOWN),
     mvRefreshType_(COM_UNKNOWN_RTYPE),
     isMvMinmax_(FALSE),
     isInsertLog_(FALSE),
     isLockOnRefresh_(FALSE),
     mvsAllowed_(COM_MVS_ALLOWED_UNKNOWN),
     isAudit_(FALSE),
     rangeLogType_(COM_RANGELOG_UNKNOWN),
     isMetaData_(FALSE),
     mvRefreshedAtTime_(0),
     objectUID_(0),
     schemaUID_(0),
     catalogUID_(0),
     commitEach_(0)
  {}

  // Copy Ctor
  MVUsedObjectCatmanFlags(const MVUsedObjectCatmanFlags& other)
  :  objectAttributes_(other.objectAttributes_),
     usageType_(other.usageType_),
     mvStatus_(other.mvStatus_),
     mvRefreshType_(other.mvRefreshType_),
     isMvMinmax_(other.isMvMinmax_),
     isInsertLog_(other.isInsertLog_),
     isLockOnRefresh_(other.isLockOnRefresh_),
     mvsAllowed_(other.mvsAllowed_),
     isAudit_(other.isAudit_),
     rangeLogType_(other.rangeLogType_)
  {}

  void setObjectAttributes(ComMVSUsedTableAttribute objectAttributes) { objectAttributes_ = objectAttributes; } 
  void setUsageType(ComMVSUsageType usageType)  { usageType_ = usageType; }
  void setMVStatus(ComMVStatus mvStatus)	{ mvStatus_ = mvStatus; }
  void setMVRefreshType(ComMVRefreshType mvRefreshType) { mvRefreshType_ = mvRefreshType; }
  void setIsMvMinmax(NABoolean isMvMinmax)	{ isMvMinmax_ = isMvMinmax; }
  void setIsInsertLog(NABoolean isInsertLog)	{ isInsertLog_ = isInsertLog; }
  void setIsLockOnRefresh(NABoolean isLockOnRefresh) { isLockOnRefresh_ = isLockOnRefresh; }
  void setMvsAllowed(ComMvsAllowed mvsAllowed)	{ mvsAllowed_ = mvsAllowed; }
  void setIsAudit(NABoolean isAudit)		{ isAudit_ = isAudit; }
  void setRangeLogType(ComRangeLogType rangeLogType) { rangeLogType_ = rangeLogType; }
  void setIsMetaData(NABoolean isMetaData)      { isMetaData_ = isMetaData; }
  void setRefreshedAt(ComTimestamp mvRefreshedAtTime) { mvRefreshedAtTime_ = mvRefreshedAtTime; }
  void setObjectUID(const ComUID& objectUID)    { objectUID_ = objectUID; }
  void setSchemaUID(const ComUID& schemaUID)    { schemaUID_ = schemaUID; }
  void setCatalogUID(const ComUID& catalogUID)  { catalogUID_ = catalogUID; }
  void setCommitEach(ComSInt32 nrows)           { commitEach_ = nrows; }

  ComMVSUsedTableAttribute  getObjectAttributes() const { return objectAttributes_; }
  ComMVSUsageType	    getUsageType()	  const { return usageType_; }
  ComMVStatus		    getMVStatus()         const { return mvStatus_; }
  ComMVRefreshType	    getMVRefreshType()    const { return mvRefreshType_; }
  NABoolean		    getIsMvMinmax()       const { return isMvMinmax_; }
  NABoolean		    getIsInsertLog()      const { return isInsertLog_; }
  NABoolean		    getIsLockOnRefresh()  const { return isLockOnRefresh_; }
  ComMvsAllowed		    getMvsAllowed()       const { return mvsAllowed_; }
  NABoolean		    getIsAudit()          const { return isAudit_; }
  ComRangeLogType	    getRangeLogType()     const { return rangeLogType_; }
  NABoolean		    getIsMetaData()       const	{ return isMetaData_; }
  ComTimestamp		    getRefreshedAt()      const { return mvRefreshedAtTime_; }
  ComUID 		    getObjectUID()        const { return objectUID_; }
  ComUID 		    getSchemaUID()        const { return schemaUID_; }
  ComUID 		    getCatalogUID()       const { return catalogUID_; }
  ComSInt32		    getCommitEach()       const { return commitEach_; }

private:
  // Prevent accidental use of default = operator.
  MVUsedObjectCatmanFlags& operator=(const MVUsedObjectCatmanFlags& other);

  ComMVSUsedTableAttribute  objectAttributes_; // Ignore changes/Insert only.
  ComMVSUsageType	    usageType_;        // Direct/Expanded/User specified.
  ComMVStatus               mvStatus_;         // isInitialized, isAvailable
  ComMVRefreshType          mvRefreshType_;    // On statement/On request/Recompute
  NABoolean                 isMvMinmax_;       // Are Min/Max functions used?
  NABoolean                 isInsertLog_;      
  NABoolean                 isLockOnRefresh_;
  ComMvsAllowed             mvsAllowed_;       // Which MVs are allowed?
  NABoolean                 isAudit_;          // Is the table audited?
  ComRangeLogType           rangeLogType_;     // Manual/Mixed/Automatic rangelog.
  NABoolean		    isMetaData_;       // Is this an SMD table?
  ComTimestamp		    mvRefreshedAtTime_;// If this is an MV, when was is last refreshed?
  ComUID		    objectUID_;
  ComUID		    schemaUID_;
  ComUID		    catalogUID_;
  ComSInt32		    commitEach_;       // The table's commit each attribute
}; // class MVUsedObjectCatmanFlags

//////////////////////////////////////////////////////////////////////////////
// This class gives a mapping from a column name to the column's position
// in the base table. This mapping is needed during DML for used objects.
class MVUsedObjectColNameMap : public NABasicObject
{
public:

  MVUsedObjectColNameMap(CollHeap *heap)
    : colNameHash_(hashKey, 50, FALSE, heap),
      colPositionTable_(NULL)
  {}

  virtual ~MVUsedObjectColNameMap();

  // Initialize the main data structure from the base table's NATable.
  void initColNameMap(const NATable *naTable, CollHeap *heap);

  // Find the column position into the base table using the column name.
  Lng32 getColPositionFor(const NAString& colName) const;
  
  NABoolean isEmpty() const { return colNameHash_.isEmpty(); }

private:
  typedef NAHashDictionary<const NAString, Lng32> ColNameHash;

  ColNameHash  colNameHash_;
  Lng32        *colPositionTable_;
};

//////////////////////////////////////////////////////////////////////////////
class MVUsedObjectInfo : public NABasicObject
{
public:
  // DDL Ctor for a used table
  MVUsedObjectInfo(const Scan *scanNode, NABoolean markKeyCols, CollHeap *heap);

  // DDL Ctor for a used view
  MVUsedObjectInfo(const RenameTable *viewNode, CollHeap *heap);

  // DDL Ctor for a used UDF
  MVUsedObjectInfo(const OptUDFInfo *udf, CollHeap *heap);

  // DML Ctor
  MVUsedObjectInfo(CollHeap		*heap,
		   const QualifiedName& name,
	           ComObjectType	objectType,
		   ComMVSUsageType	usageType,
		   const LIST (Lng32) &	usedColumnList,
		   const LIST (Lng32) &	indirectUpdateCols,
		   Lng32			ordinalNumber,
		   Lng32			isInner,
		   NAString             selectionPreds)
  :  objectName_(name, heap),
     objectType_(objectType),
     isUsedDirectly_(TRUE),
     usedColumnList_(usedColumnList, heap),
     indirectUpdateCols_(indirectUpdateCols, heap),
     MVColsReferencingTheCI_(heap),
     ordinalNumber_(ordinalNumber),
     isInnerTableOfLeftJoin_(isInner),
     colNameMap_(heap),
     selectionPredicates_(selectionPreds, heap)
  {}

  // Copy Ctor
  MVUsedObjectInfo(const MVUsedObjectInfo& other, CollHeap *heap)
  :  objectName_(other.objectName_, heap),
     objectType_(other.objectType_),
     isUsedDirectly_(other.isUsedDirectly_),
     usedColumnList_(other.usedColumnList_, heap),
     indirectUpdateCols_(other.indirectUpdateCols_, heap),
     MVColsReferencingTheCI_(other.MVColsReferencingTheCI_, heap),
     ordinalNumber_(other.ordinalNumber_),
     isInnerTableOfLeftJoin_(other.isInnerTableOfLeftJoin_),
     catmanFlags_(other.catmanFlags_),
     colNameMap_(heap),
     selectionPredicates_(other.selectionPredicates_, heap)
  {}

  // operator== required for use in templates
  NABoolean operator==(const MVUsedObjectInfo &other) const;

  NABoolean isUsedColumn(Lng32 col) const 
    { return usedColumnList_.contains(col); }
  NABoolean isIndirectUpdateCol(Lng32 col) const
    { return indirectUpdateCols_.contains(col); }
  NABoolean isMVColRCI(Lng32 col) const
    { return MVColsReferencingTheCI_.contains(col); }
  void addUsedColumn(Lng32 col) 
    { if (!isUsedColumn(col)) usedColumnList_.insert(col); }
  void addIndirectUpdateCol(Lng32 col) 
    { if (!isIndirectUpdateCol(col)) indirectUpdateCols_.insert(col); }
  void addMVColRCI(Lng32 col) 
    { if (!isMVColRCI(col)) MVColsReferencingTheCI_.insert(col); }

  const QualifiedName&  getObjectName()	       const { return objectName_; }
  const QualifiedName&  getInternalObjectNameForAction() const { return internalObjectNameForAction_; }

  NABoolean		isAnMV()	    const { return objectType_ == COM_MV_OBJECT; }
  NABoolean		isAnUDR()	    const { return objectType_ == COM_USER_DEFINED_ROUTINE_OBJECT; }
  ComObjectType		getObjectType()	    const { return objectType_; }
  NABoolean		isUsedDirectly()    const { return isUsedDirectly_; }
  void                  setIsUsedDirectly(NABoolean flag)
                                          { isUsedDirectly_ = flag; }
  const LIST (Lng32) &	getUsedColumnList() const { return usedColumnList_; }
  const LIST (Lng32) &	getIndirectUpdateCols() const { return indirectUpdateCols_; }
  const LIST (Lng32) &	getMVColsReferencingTheCI() const { return MVColsReferencingTheCI_; }
  Lng32			getOrdinalNumber()  const { return ordinalNumber_; }
  NABoolean		isInnerTableOfLeftJoin() const { return isInnerTableOfLeftJoin_; }
  const NAString&       getSelectionPredicates() const { return selectionPredicates_; }

  void setSelectionPredicates(const NAString& preds) { selectionPredicates_ = preds; }

  // Used after running the join graph algorithm.
  void setOrdinalNumber(Lng32 ordinalNumber) { ordinalNumber_ = ordinalNumber; }
  void setAsInnerTable() { isInnerTableOfLeftJoin_ = TRUE; }

  // These methods are used by the catman.
  MVUsedObjectCatmanFlags&	  getCatmanFlags()	 { return catmanFlags_; }
  const MVUsedObjectCatmanFlags&  getCatmanFlags() const { return catmanFlags_; }
  
  NABoolean processSelectionPredicates(const ItemExpr *expr, BindWA *bindWA, NABoolean& toBeRemoved);
  NABoolean processSelectionPredicates(ValueIdSet& expr, BindWA *bindWA);

  // Methods used for DML only.
  void initColNameMap(const NATable *naTable, CollHeap *heap)
  { colNameMap_.initColNameMap(naTable, heap); }
  NABoolean isUsedColumn(const NAString& colName) const 
  { return usedColumnList_.contains(colNameMap_.getColPositionFor(colName)); }
  NABoolean isIndirectUpdateCol(const NAString& colName) const
  { return indirectUpdateCols_.contains(colNameMap_.getColPositionFor(colName)); }
  NABoolean isMVColRCI(const NAString& colName) const
  { return MVColsReferencingTheCI_.contains(colNameMap_.getColPositionFor(colName)); }

  // Display/print, for debugging.
#ifndef NDEBUG
  void display() const;
  void print(FILE* ofd = stdout,
	     const char* indent = DEFAULT_INDENT,
	     const char* title = "MVColumnInfo") const;
#endif

private:
  // Prevent accidental use of default copy Ctor and = operator.
  MVUsedObjectInfo& operator=(const MVUsedObjectInfo& other);
  MVUsedObjectInfo(const MVUsedObjectInfo& other);

  const QualifiedName	  objectName_; 
  ComObjectType		  objectType_;  // Base table, View or MV
  NABoolean		  isUsedDirectly_;
  NABoolean		  isInnerTableOfLeftJoin_;

  const QualifiedName     internalObjectNameForAction_;  // Internal object name for the UUDF ACTION

  // A list of all the table's columns that are used by this MV.
  LIST (Lng32)		  usedColumnList_;

  // The columns that are used either by the clustering index of the base 
  // table, or by one of the join predicates.
  LIST (Lng32)		  indirectUpdateCols_;

  // This is a list of column positions of MV columns that correspond to
  // the clustering index columns of this table. This is later used for 
  // creating the minimal set of secondary indices for MJVs.
  LIST (Lng32)		  MVColsReferencingTheCI_;

  // The position of this table in the join graph solution.
  Lng32			  ordinalNumber_;

  // All the catman ever wanted to know about this used object...
  MVUsedObjectCatmanFlags catmanFlags_;

  // Used In DML to map column names to column positions, for checking if
  // columns are used etc.
  MVUsedObjectColNameMap  colNameMap_; 

  NAString                selectionPredicates_;
};  // class MVUsedObjectInfo

//////////////////////////////////////////////////////////////////////////////
class MVColumnInfo : public NABasicObject
{
public:
  // Ctor for User columns.
  MVColumnInfo (MVInfoForDDL&	  mvInfoObj,
		ComMVType	  mvType,
		const ColumnDesc *colDesc, 
		BindWA		 *bindWA,
		CollHeap	 *heap);

  // Ctor for MAV dependant system columns.
  MVColumnInfo (const MVColumnInfo&   other, 
		MVInfoForDDL&	mvInfoObj, 
		MVAggFunc	aggIndex, 
		ItemExpr       *colExpr, 
		CollHeap       *heap);

  // Ctor for MAV GroupBy columns.
  MVColumnInfo (MVInfoForDDL&	 mvInfoObj,
		ItemExpr	*groupingCol,
		CollHeap	*heap);

  // Ctor for MJV clustering key system added columns.
  MVColumnInfo (MVInfoForDDL&	 mvInfoObj,
		NAColumn	*clusteringKeyCol,
		CollHeap	*heap);

  // Ctor for the COUNT(*) system added column.
  MVColumnInfo (MVInfoForDDL&	 mvInfoObj,
		CollHeap	*heap);

  // Ctor for link only columns (duplicate, redundant or complex).
  MVColumnInfo (Int32		      colNum,
		const QualifiedName&  origTableName,
		Int32		      origColNumber,
		ComMVColType	      colType,
		CollHeap	     *heap);

  // Explicit Ctor for DML
  MVColumnInfo (CollHeap	      *heap,
		Int32		      colNumber,
		const NAString&	      colName,
		ComMVColType	      colType,
		OperatorTypeEnum      operatorType,
		NABoolean	      isSystem,
		const NAString	     &normalizedColText,
		Int32		      dep1,
		Int32		      dep2,
		Int32		      dep3,
		ComMVSUsageType	      usageType,
		const QualifiedName  *origTableName,
		Int32		      origColNumber,
		NABoolean	      isComplex);

  // Copy Ctor
  MVColumnInfo(const MVColumnInfo& other, CollHeap *heap);

  // operator== required for use in templates
  NABoolean operator==(const MVColumnInfo& other) const;

  // Information about the column in the MV
  Int32	            getColNumber()	    const { return colNumber_; }
  const NAString&   getColName()	    const { return colName_; }
  const NAType	   *getColDataType()	    const { return colDataType_; }
  ComMVColType	    getColType()	    const { return colType_; }
  OperatorTypeEnum  getOperatorType()	    const { return operatorType_; }
  NABoolean	    isSystem()		    const { return isSystem_; }
  const NAString&   getNormalizedColText()  const { return normalizedColText_; }
  Int32		    getDepCol1()	    const { return dep1_; }
  Int32		    getDepCol2()	    const { return dep2_; }
  Int32		    getDepCol3()	    const { return dep3_; }
  NABoolean         isInMVCI()              const { return isInMVCI_; }
  NABoolean         isARealColumn()         const
    { return (colType_!=COM_MVCOL_DUPLICATE) &&
  	     (colType_!=COM_MVCOL_COMPLEX); }

  // Information about the column used in the base table
  ComMVSUsageType	 getUsageType()     const { return usageType_; }
  const QualifiedName&	 getOrigTableName() const { return origTableName_; }
  Int32			 getOrigColNumber() const { return origColNumber_; }
  NABoolean		 getIsComplex()	    const { return isComplex_; }
  const NAString&        getBaseColHashKey()const { return baseColHashKey_; }

  NABoolean isIndirectUpdate()  const { return isIndirectUpdate_; }
  NABoolean isCountStarColumn()	const { return (aggIndex_==AGG_COUNTSTAR); }
  NABoolean isMinMaxColumn()	const { return (aggIndex_==AGG_MIN || aggIndex_==AGG_MAX); }
  NABoolean isNotNull()		const { return isNotNull_; }
  ItemExpr *getColExpr()	const { return colExpr_; }
  
  // Mutators
  void setIndirectUpdate() { isIndirectUpdate_ = TRUE; }
  void setNewColumnName(const NAString& newName) { colName_ = newName; }
  void setNotNull(NABoolean value=TRUE) { isNotNull_ = value; }
  void setNotNullFrom(ItemExpr *expr);
  void setNormalizedText(const NAString& text) { normalizedColText_ = text; }
  void setAsRedundant(Lng32 firstMvCol);
  void setInMVCI() { isInMVCI_ = TRUE; }

  void insertIntoExpHash(ExpressionHash& expHash, CollHeap *heap);
  void createDependentColumns(ExpressionHash& expHash, 
			      MVInfoForDDL&   mvInfoObj,
			      CollHeap	     *heap);

  static void calcBaseColHashKey(const QualifiedName& tableName, 
                                 Lng32                 position,
				 NAString&            hashKey);

  // Display/print, for debugging.
#ifndef NDEBUG
  void display() const;
  void print(FILE* ofd = stdout,
	     const char* indent = DEFAULT_INDENT,
	     const char* title = "MVColumnInfo") const;
#endif

private:
  // Prevent accidental use of default copy Ctor and = operator.
  MVColumnInfo& operator=(const MVColumnInfo& other);
  MVColumnInfo(const MVColumnInfo& other);

  ItemExpr   *findMavColumnType(ItemExpr *expr, MVInfoForDDL& mvInfoObj);
  MVAggFunc   findAggrEnum(ItemExpr *expr, MVInfoForDDL& mvInfoObj);
  NABoolean   isNonRepeatableExpressionUsed(BindWA *bindWA);
  NABoolean   isColumnExpressionComplex(ItemExpr *expr);
  void calcNormalizedTextForComplexCols(MVInfoForDDL& mvInfoObj, 
		                        ValueId       directColValueId);
  void setColTypeAndExpr(ItemExpr  *expr, 
                         ComMVType  mvType,
		         ValueId    directColValueId,
			 BindWA    *bindWA);
  Int32  newMavDependentColumn(ExpressionHash&    expHash, 
			     const NAString&	exprTextForHash,
			     MVInfoForDDL&	mvInfoObj,
			     MVAggFunc		depIndex, 
			     ItemExpr	       *depExpr,
			     CollHeap	       *heap);

  Int32  newStddevwDepColumn(Int32             childNo,
			   ExpressionHash& expHash, 
			   const NAString& exprTextForHash,
			   MVInfoForDDL&   mvInfoObj,
			   CollHeap	  *heap);

  // Information about the column in the MV
  const Int32	    colNumber_;		// column ordinal in the MV
  NAString	    colName_;		// column name
  const NAType	   *colDataType_;	// data type (used by DDL only)
  ComMVColType	    colType_;		// 
  OperatorTypeEnum  operatorType_;	// Which aggregate/function
  const NABoolean   isSystem_;		// was added by the system
  NAString	    normalizedColText_;	//
  Int32		    dep1_;
  Int32		    dep2_;
  Int32		    dep3_;

  // Information about the column used in the base table.
  ComMVSUsageType 	 usageType_;
  QualifiedName		 origTableName_;
  Int32			 origColNumber_;  // if mapped 1-to-1 to this col
  NABoolean		 isComplex_;	  // too complex function, or > 1 columns

  NABoolean		 isUsed_;
  NABoolean		 isIndirectUpdate_;
  NAString               baseColHashKey_;

  // Information used only internaly at DDL time, and not saved in the SMD.
  MVAggFunc		 aggIndex_;
  NAString		 exprTextForHash_;
  const NAString	*unBoundText_;
  NABoolean		 isNotNull_;
  ItemExpr		*colExpr_;
  NABoolean		 isInMVCI_;
}; // class MVColumnInfo

typedef LIST(MVColumnInfo*) MVColumnInfoList;

//////////////////////////////////////////////////////////////////////////////
class MVColumns: public NABasicObject
{
public:
  MVColumns(CollHeap *heap);
  MVColumns(CollHeap *heap, const MVColumns& other, NABoolean isIncremental);

  void insert(MVColumnInfo *colInfo, NABoolean isIncremental);

  MVColumnInfo *getMvColInfoByName(const NAString& name) const;
  MVColumnInfo *getMvColInfoByBaseColumn(const QualifiedName& tableName, Lng32 position) const;
  MVColumnInfo *getMvColInfoByBaseColumn(const QualifiedName& tableName, Lng32 baseColPosition, Lng32 mvColPosition) const;
  MVColumnInfo *getMvColInfoByIndex(Lng32 index, NABoolean adjustIndex = TRUE) const;
  const MVColumnInfoList *getAllMvColsAffectedBy(const QualifiedName& tableName, Lng32 position) const;

  // The entries() method and [] operator are shortcuts for accessing the 
  // direct column list, and should only be used for serial access to the 
  // columns list - for loops etc.
  // For access to specific columns by index use getMvColInfoByIndex().
  MVColumnInfo *operator[](Lng32 index) const 
  { return getMvColInfoByIndex(index, FALSE); }
  ULng32 entries() const  // Only real direct columns.
  { return directColumnList_.entries(); }

  // returns # of direct + extra columns.
  ULng32 getTotalNumberOfColumns() const;

  Lng32 getFirstSysAddedColumn() const
  { return firstSysAddedColumn_; }

  void setNewColumnName(MVColumnInfo *colInfo, const NAString& newName);

  // Display/print, for debugging.
#ifndef NDEBUG
  void display() const;
  void print(FILE* ofd = stdout,
	     const char* indent = DEFAULT_INDENT,
	     const char* title = "MVColumns") const;
#endif

private:
  // Prevent accidental use of default copy Ctor and = operator.
  MVColumns& operator=(const MVColumns& other);
  MVColumns(const MVColumns& other);

  typedef NAHashDictionary<const NAString, MVColumnInfo>     ColumnInfoHash;
  typedef NAHashDictionary<const NAString, MVColumnInfoList> ColumnInfoListHash;

  MVColumnInfoList      directColumnList_;
  MVColumnInfoList      extraColumnList_;
  ColumnInfoHash        columnInfoByNameHash_;
  ColumnInfoListHash    columnInfoByBaseHash_;
  Lng32   		syskeyIndexCorrection_;
  Lng32			firstSysAddedColumn_;
  CollHeap	       *heap_;

  static const Lng32     initialHashTableSize_;
}; // class MVColumns

//////////////////////////////////////////////////////////////////////////////
class MVInfo: public NABasicObject
{
public:
  // Ctor for DDL. Initialize only what is known from the CREATE MV syntax.
  MVInfo( const NAString   &nameOfMV,
	  ComMVRefreshType  refreshType,
	  NABoolean	    isRewriteEnabled,
	  ComMVStatus	    mvStatus,
	  const NAString&   mvSelectText,
	  CharInfo::CharSet mvSelectTextCharSet,
	  CollHeap	   *heap);

  // Ctor for DML. All the data is read from the SMD.
  MVInfo( const NAString	     &nameOfMV,
	  ComMVRefreshType	      refreshType,
	  NABoolean		      isRewriteEnabled,
	  ComMVStatus		      mvStatus,
	  const NAString&	      mvSelectText,
	  CharInfo::CharSet	      mvSelectTextCharSet,
	  NABoolean		      isIncremental,
	  ComMVType		      mvType,
	  LIST (MVUsedObjectInfo *)&  usedObjectsList,
	  MVColumnInfoList&           directColumnList,
	  NABoolean		      isLeftLinear,
	  LIST (MVVegPredicate *)&    eqPredicateList,
	  CollHeap		     *heap);

  // Copy Ctor
  // if heap==NULL then copy heap from other. Otherwise, override heap pointer.
  MVInfo(const MVInfo &other, CollHeap *heap);  

  // The Dtor
  virtual ~MVInfo();

  // Methods for the catman to query the information during DDL.
  NAString	      getNameOfMV()       const { return nameOfMV_; }
  ComMVRefreshType    getRefreshType()    const { return refreshType_; }
  NABoolean	      isRewriteEnabled()  const { return isRewriteEnabled_; }
  ComMVStatus	      getMvStatus()       const { return mvStatus_; }
  NABoolean	      isInitialized()     const { return isInitialized_; }
  NABoolean	      isIncremental()     const { return isIncremental_; }
  ComMVType	      getMVType()         const { return mvType_; }
  NABoolean	      isMinMaxUsed()	  const { return isMinMaxUsed_;}
  MVJoinGraph	     *getJoinGraph()	  const { return joinGraph_; }
  Lng32	              getPosOfCountStar() const { return posOfCountStar_; }
  const NAString&     getMVSelectText()   const { return mvSelectText_; }
  CharInfo::CharSet   getMVSelectTextCharSet () const { return mvSelectTextCharSet_; }

  CollHeap	     *getHeap()           const { return heap_; }

  // - for showddl
  NAString	      getTextForShowddl(CatROMV *pMV,
                                        CharInfo::CharSet & showddlOutputCharSet /* out param */ ) const;
  //

  // Access the internal data structures.
	MVColumns& getMVColumns()       { return columnList_; }
  const MVColumns& getMVColumns() const { return columnList_; }
  LIST (MVUsedObjectInfo*)& getUsedObjectsList() { return usedObjectsList_; }
  CollIndex getNumberOfUsedObjects () const { return usedObjectsList_.entries(); }
  const LIST (MVUsedObjectInfo*)& getUsedObjectsList() const { return usedObjectsList_; }
  LIST (MVVegPredicate *)&  getEqPredicateList() { return eqPredicateList_; }
  MVUsedObjectInfo         *findUsedInfoForTable(const QualifiedName& tableName) const;
  const LIST (Lng32)&        getUsedColumns(const QualifiedName& tableName) const;
  const LIST (Lng32)&        getIndirectUpdateColumns(const QualifiedName& tableName) const;

  // Find an equal predicate keyColis part of.
  Lng32 findEqPredicateCovering(NAColumn *keyCol);

  // Is there an equal predicate between these two columns?
  NABoolean isEqPredicateBetween(const QualifiedName& table1, Lng32 col1,
				 const QualifiedName& table2, Lng32 col2) const;

  // Return a parsed RelExpr tree of the MV select text.
  RelRoot  *buildMVSelectTree(const NAString* alternativeText = NULL,
                              CharInfo::CharSet alternativeTextCharSet
                              = CharInfo::UTF8
                              ) const;

  // Create the MV descriptor by taking the MV query from the updated CREATE MV 
  // text, parsing it, binding, normalizing and analyzing it.
  QRMVDescriptorPtr prepareMvDescriptorTree(NABoolean isIncrementalMV, 
                                            NAString& warningMessage,
                                            const NAString* alternativeText = NULL);
    
  MVColumnInfoList* buildColumnInfoListFromColIndList(const ColIndList &currentRCI) const;

  // - These fields don't remain constant during the lifetime
  // of the MV object in the cache. The fields should reflect the 
  // real state of the object in the cache.
  // see CatRWMV::setMvStatus and CatRWMV::setRewrite.
  void setRewriteEnabled(NABoolean newVal) { isRewriteEnabled_ = newVal; }
  void setMvStatus(ComMVStatus newVal) { mvStatus_ = newVal; }

  // Do we need to collect information that is needed only on incremental MVs?
  NABoolean isIncrementalWorkNeeded() const 
    { return (isIncremental()                 && 
    	      getRefreshType()!=COM_RECOMPUTE &&
    	      getRefreshType()!=COM_BY_USER); }

  // Display/print, for debugging.
#ifndef NDEBUG
  void display() const;
  virtual void print(FILE* ofd = stdout,
		     const char* indent = DEFAULT_INDENT,
		     const char* title = "MVInfo") const;
#endif

protected:
  // Simple mutators
  void setPosOfCountStar(Lng32 pos) { posOfCountStar_ = pos; }
  void setIsIncremental(NABoolean isInc) { isIncremental_ = isInc; }
  void setMinMaxIsUsed() { isMinMaxUsed_ = TRUE; }
  void setMvType(ComMVType mvType) { mvType_ = mvType; }
  void newJoinGraph(Int32 initialSize);

  // Called by buildMVSelectTree().
  void fixMvSelectList(RelRoot *mvSelectTree) const;

  // Recursive method used by MVInfoForDDL::areTablesUsedOnlyOnce().
  // Return TRUE is the tables are used only once.
  NABoolean addTableNamesToList(BindWA        *bindWA, 
                                TableNameHash& usedObjectsNames);

  // - for showddl
  NAString getMVRefreshTypaAsString() const;
  NAString getInitializationAsString() const;
  NAString getOptionalNameClause(StmtDDLCreateMV *createMvNode, 
				 const NAString& mvText,
                                 CharInfo::CharSet & mvTextCharSet /* inout param */ ) const;
  NAString getSelectPhraseAsString(NAString& sysAddedCols) const;
  NAString getIgnoreChangesListAsString(StmtDDLCreateMV *createMvNode) const;
  NAString getIgnoreChangesListAsString() const;
  //

private:
  // Prevent accidental use of default copy Ctor and = operator.
  MVInfo& operator=(const MVInfo& other);
  MVInfo(); // do not use
  MVInfo(const MVInfo& other);

  // These fields are initialized by the ctor in both DDL and DML.
  // heap_ MUST be the first data member here, so that it is the first
  // to be initialized by the copy Ctor. See the code for details.
  CollHeap	       *heap_;
  NAString		nameOfMV_;
  ComMVRefreshType	refreshType_;
  NABoolean		isRewriteEnabled_;
  ComMVStatus		mvStatus_;
  NABoolean		isInitialized_;

  // In DDL these fields are calculated from the collected data
  // by the processCollectedInformation() method. In DML they are
  // initialized by the catman.
  NABoolean		    isIncremental_;
  NABoolean		    isMinMaxUsed_;
  ComMVType		    mvType_;
  LIST (MVUsedObjectInfo *) usedObjectsList_;
  MVColumns                 columnList_;
  LIST (MVVegPredicate *)   eqPredicateList_;
  const NAString	    mvSelectText_;
  CharInfo::CharSet	    mvSelectTextCharSet_;
  Lng32			    posOfCountStar_;
  MVJoinGraph		   *joinGraph_;
};  // class MVInfo

//////////////////////////////////////////////////////////////////////////////
class MVInfoForDDL: public MVInfo
{
public:
  MVInfoForDDL(const NAString			  &NameOfMV,
	       ComMVRefreshType			  refreshType,
	       NABoolean			  isRewriteEnabled,
	       ComMVStatus			  mvStatus,
	       const NAString&			  mvSelectText,
	       CharInfo::CharSet		  mvSelectTextCharSet,
	       const ElemDDLStoreOptKeyColumnList *userSpecifiedCI,
	       CollHeap				  *heap);

  virtual ~MVInfoForDDL();

  // Accessors
  NAString&	getSysColNames()           { return sysColNames_; }
  NAString&	getSysColExprs()           { return sysColExprs_; }
  NAString&	getTextForSysCols()        { return textForSysCols_; }
  NABoolean	isLeftLinear()       const { return isLeftLinear_; }
  NABoolean	usesOtherMVs()	     const { return usesOtherMVs_; }
  Lng32		getUserColumnCount() const { return userColumnCount_; }
  NABoolean     isOnView()                 { return ( usedViewsList_.entries() > 0 ); }
  NABoolean     isOnUDF()                  { return ( udfList_.entries() > 0 ); }


  // Methods for querting the view graph
  const ColRefName&       findTopViewOf(const ColRefName& colName) const;
  const ColRefName       *findViewNameFor(const ValueId vid) const;
  NABoolean isMaskedTable(const QualifiedName& tableName) const;

  // Methods called from collectMVinformation().
  void addScanNode(Scan *newNode);
  void addJoinNode(Join *newNode);
  void addGroupByNode(GroupByAgg *newNode);
  void setRootNode(RelRoot *rootNode, BindWA *bindWA);
  void addViewRenameNode(RenameTable *renameNode);
  void setNotIncremental(const MV_NOT_INCREMENTAL notIncrementalReason = MVNI_DEFAULT);

  // These methods are called by StmtDDLCreateMV::bindNode() just after 
  // the tree walk to process the information that was collected.
  void processBoundInformation(BindWA *bindWA);
  void processNormalizedInformation(BindWA *bindWA);
  NABoolean areTablesUsedOnlyOnce(BindWA *bindWA);

  // Used by the class MVColumnInfo.
  NABoolean isLeftJoinInnerTable(const QualifiedName& tableName) const;
  Int32  getNextColIndex() { return realColumnCount_ ++; }
  void addSystemColumnText(const NAString& colName, const NAString& colExp);
  void  NewSystemColumName(const NAString& midText, NAString& colNameString);
  const NAString *getUnBoundTextFor(CollIndex index) const;
  
  CharInfo::CharSet getMVSelectTextCharSet () const { return mvSelectTextCharSet_; }

  // Uses MJVIndexBuilder to build the optimal secondary index
  // !!!ATTENTION!!!
  // The responsibility of the return LIST(MVColumnInfoList*)* deletion 
  // falls on the function's caller
  LIST(MVColumnInfoList*)* getOptimalMJVIndexList (const LIST(Lng32)* mvCI);
  
  //++ MV -
  NABoolean isUsedObjectNotIgnoreChanges(const QualifiedName& tableName);

  void setColAsIndirectUpdate(MVColumnInfo *colInfo);

  // get the reason why we've set as not incremental
  MV_NOT_INCREMENTAL getNotIncrementalReason() const 
      { return notIncrementalReason_; }
  NABoolean isValidNotIncrementalReason( MV_NOT_INCREMENTAL reason )
      { return ((reason >= MVNI_DEFAULT ) && (reason < MVNI_NUM_REASONS)); }

 // returns the UDF usage list
  const LIST(OptUDFInfo *) &getUDFList() const { return udfList_;}
  LIST(OptUDFInfo *) &getUDFListForUpdate()    { return udfList_;}

  // exact number of tables used by an MV
  CollIndex scanNodesNumber()
    { return scanNodes_.entries(); }

#ifndef NDEBUG
  virtual void print(FILE* ofd = stdout,
		     const char* indent = DEFAULT_INDENT,
		     const char* title = "MVInfo") const;
#endif

private:
  // Prevent accidental use of default copy Ctor and = operator.
  MVInfoForDDL& operator=(const MVInfoForDDL& other);
  MVInfoForDDL(const MVInfoForDDL& other);

  // Called by processBoundInformation() and processNormalizedInformation().
  void addMavSystemColumns(BindWA *bindWA);
  void addMjvSystemColumns(BindWA *bindWA);
  void initMVType();
  void initUsedObjectsList(BindWA *bindWA);
  MVUsedObjectInfo *initUsedObject(Scan          *scanNode, 
                                   const NATable *naTable,
				   BindWA        *bindWA);
  void initJoinPredicates(BindWA *bindWA);
  MVVegPredicate *buildNewVegPred(ValueId             predValueId, 
                                  LIST (const VEG *)& vegPtrList,
				  ComMVSUsageType     usageType,
				  NABoolean           isLeftJoin);
  void extractParsedColumnText(Lng32 colIndex, ItemExpr *colExpr);
  void addBaseColsUsedByComputedMvColumns();
  void verifyGroupByColumns(GroupByAgg *groupByNode);
  void addViewsToUsedObjectList();
  void addUDFsToUsedObjectList();
  RelExpr *getChildBelowRootAndRenameNodes(RelExpr *node);
  void addClusteringIndexAsSystemAdded(TableDesc *tDesc, BindWA *bindWA);
  NABoolean isColumnInMVCI(const NAString& colName);
  MVColumnInfo *addNewMjvSystemColumnIfNeeded(BindWA   *bindWA,
                                              NAColumn *keyCol,
                                              Lng32      MVColIndex);

  // Recompute MV Used Objects clustering information
  void addRecomputeMVUSedObjectsCIColumns (BindWA *bindWA);
  void addUsedObjectsClusteringInfo(TableDesc *tDesc,
                                    BindWA    *bindWA);

  void markMVCIasIndirectUpdate();

  // All system added column name start with "SYS_".
  static const char       systemAddedColNamePrefix_[];

  // These fields are used during the DDL data collection phase.
  LIST (Scan *)		  scanNodes_;
  LIST (Join *)		  joinNodes_;
  LIST (GroupByAgg *)     groupByNodes_;
  RelRoot		 *rootNode_;
  const ColumnDescList   *colDescList_;
  Lng32			  sysColCounter_;
  NABoolean		  isLeftLinear_;
  NAString		  sysColNames_;
  NAString		  sysColExprs_;
  NAString		  textForSysCols_;
  Lng32			  userColumnCount_;
  NABoolean		  usesOtherMVs_;
  ViewColumnGraph	 *viewColumnGraph_;
  Lng32			  realColumnCount_;
  LIST (const QualifiedName *) leftJoinInnerTables_;
  LIST (MVUsedObjectInfo *)    usedViewsList_;
  LIST (const NAString *)      unBoundColumnsText_;
  ElemDDLColRefArray	 *userSpecifiedCI_;
  MV_NOT_INCREMENTAL            notIncrementalReason_; 
  QRMVDescriptorPtr      mvDescriptor_;
  XMLString              *xmlText_;
  LIST(OptUDFInfo *) udfList_;
  CharInfo::CharSet      mvSelectTextCharSet_;
};  // class MVInfoForDDL


//////////////////////////////////////////////////////////////////////////////
class MVInfoForDML: public MVInfo
{
public:
 
  MVInfoForDML( const NAString	         &NameOfMV, 
			      ComMVRefreshType		  refreshType,
			      NABoolean			  isRewriteEnabled,
			      ComMVStatus		  mvStatus,
			      const NAString&		  MVSelectText,
			      CharInfo::CharSet		  mvSelectTextCharSet,
			      NABoolean			  isIncremental,
			      ComMVType			  mvType,
			      LIST (MVUsedObjectInfo *)&  usedObjectsList,
			      MVColumnInfoList&           DirectColumnList,
			      NABoolean	  		  isLeftLinear,
			      LIST (MVVegPredicate *)&    eqPredicateList,
			      CollHeap		         *heap);

  // Copy Ctor
  MVInfoForDML(const MVInfo& other, CollHeap *heap)
  : MVInfo(other, heap),
    usedObjectsHash_(QualifiedName::hash, (Lng32)20, FALSE, heap)
  {}

  virtual ~MVInfoForDML();

  // Return a list of MV column positions for the GroupBy columns.
  void getMavGroupByColumns(LIST(Lng32)& gbColList) const;

  // Is this MV defined on a single base table?
  NABoolean isMvOnSingleTable(); 

  // Initialize the join graph algorithm for Multi-delta refresh.
  MVJoinGraph *initJoinGraph(BindWA                       *bindWA, 
                             const DeltaDefinitionPtrList *deltaDefList,			     
			     NABoolean                     checkOnlyInserts);

  // Insert into the usedObjectsHash_ all used objects, by their table
  // names and also by their log names.
  void initUsedObjectsHash();

  // This override is implemented using the optimized usedObjectsHash_
  const MVUsedObjectInfo *findUsedInfoForTable(const QualifiedName& tableName);

  CharInfo::CharSet getMVSelectTextCharSet () const { return mvSelectTextCharSet_; }

#ifndef NDEBUG
  virtual void print(FILE* ofd = stdout,
		     const char* indent = DEFAULT_INDENT,
		     const char* title = "MVInfo") const;
#endif

private:
  // Prevent accidental use of default copy Ctor and = operator.
  MVInfoForDML(const MVInfoForDML& other);
  MVInfoForDML& operator=(const MVInfoForDML& other);

  TableNameHash usedObjectsHash_;
  CharInfo::CharSet mvSelectTextCharSet_;
};  // class MVInfoForDML


#endif
