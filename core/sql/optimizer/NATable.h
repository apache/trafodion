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
#ifndef NATABLE_H
#define NATABLE_H

#include <vector>
#include "BaseTypes.h"
#include "Collections.h"
#include "Int64.h"
#include "ItemConstr.h"
#include "ObjectNames.h"
#include "ComSmallDefs.h"            // added for ComDiskFileFormat enum
#include "NAColumn.h"
#include "NAFileSet.h"
#include "Stats.h"
#include "NAMemory.h"
#include "ComMvAttributeBitmap.h"
#include "SequenceGeneratorAttributes.h"
#include "charinfo.h"
#include "nawstring.h"
#include "CmpISPStd.h"
#include "ComSizeDefs.h"
#include "sqlcli.h"
#include "hiveHook.h"
#include "ExpLOBexternal.h"
#include "ComSecurityKey.h"
#include "ExpHbaseDefs.h"
#include "ComViewColUsage.h"

//forward declaration(s)
// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class NATable;
class NATableDB;
class HistogramCache;
class HistogramsCacheEntry;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class BindWA;
class MVInfoForDML;
class NATableDB;
struct TrafDesc;
class HbaseCreateOption;
class PrivMgrUserPrivs;
class ExpHbaseInterface;

typedef QualifiedName* QualifiedNamePtr;
typedef ULng32 (*HashFunctionPtr)(const QualifiedName&);
typedef SUBARRAY(Lng32) CollIndexSet;

NAType* getSQColTypeForHive(const char* hiveType, NAMemory* heap);

#define NATABLE_MAX_REFCOUNT 12

//This class represents a single entry in the histogram cache.
//Each object of this class contains references to all the
//different histograms of a particular base table.
//It is intended to:
// 1) encapsulate a memory/information efficient representation of a 
//    table's cached histograms that live in mxcmp's context heap
//    (which goes away only when mxcmp dies)
// 2) shield & keep invariant the current optimizer interface to histograms
//    that live in mxcmp's statement heap (which goes away at end of each 
//    statement compilation)
class HistogramsCacheEntry : public NABasicObject
{
  friend class HistogramCache;
  
  public:
    // constructor for creating memory efficient representation of colStats
    HistogramsCacheEntry
    (const StatsList & colStats,
     const QualifiedName & qualifiedName,
     Int64 tableUID,
     const Int64 & statsTime,
     const Int64 & redefTime,
     NAMemory * heap);

    //destructor
    virtual ~HistogramsCacheEntry();

    //setter methods
    //should be called to indicate that histograms for a given table
    //have been pre-fetched
    void setPreFetched(NABoolean preFetched = TRUE){preFetched_ = preFetched;};

    const ColStatsSharedPtr getStatsAt(CollIndex x) const;

    const MultiColumnHistogram* getMultiColumnAt(CollIndex x) const;

    NABoolean contains(CollIndex colPos) const 
    { return singleColumnPositions_.contains(colPos); }

    // insert all multicolumns referencing col into list
    // use singleColsFound to avoid duplicates
    void getMCStatsForColFromCacheIntoList
    (StatsList& list, NAColumn& col, ColumnSet& singleColsFound);

    // adds histograms to this cache entry
    void addToCachedEntry(NAColumnArray & columns, StatsList & list);

    // add multi-column histogram to this cache entry
    void addMultiColumnHistogram(const ColStats& mcStat,
                                 ColumnSet* singleColPositions=NULL);

    //accessor methods
    ColStatsSharedPtr const getHistForCol (NAColumn& col) const;

    CollIndex singleColumnCount() const 
    { return full_ ? full_->entries() : 0; }

    CollIndex multiColumnCount() const 
    { return multiColumn_ ? multiColumn_->entries() : 0; }

    NABoolean           preFetched() const {return preFetched_;};
    const QualifiedName* getName() const;

    NABoolean accessedInCurrentStatement() const 
    { return accessedInCurrentStatement_; }

    void resetAfterStatement() 
    { accessedInCurrentStatement_ = FALSE; }

    //overloaded operator to satisfy hashdictionary
    inline NABoolean operator==(const HistogramsCacheEntry & other)
	{return (this == &other);};

    Int64 getRefreshTime() const          { return refreshTime_; };

    void setRefreshTime(Int64 refreshTime) { refreshTime_ = refreshTime ; };

    void updateRefreshTime();

    void setRedefTime(Int64 redefTime) { redefTime_ = redefTime; };

    Int64 getRedefTime() const            { return redefTime_; };

    Int64 getStatsTime() const            { return statsTime_; };

    static Int64 getLastUpdateStatsTime();

    static void setUpdateStatsTime(Int64 updateTime);

    inline NABoolean isAllStatsFake() { return allFakeStats_; };

    inline void allStatsFake(NABoolean allFakeStats) { allFakeStats_ = allFakeStats; }

    inline ULng32 getSize() {return size_;}

    Int64 getTableUID() const { return tableUID_; };
  
    void display() const;
    void print( FILE* ofd = stdout,
                const char* indent = DEFAULT_INDENT,
                const char* title = "HistogramsCacheEntry") const;
    void monitor(FILE* ofd) const;

  private:
  
    inline void setSize(ULng32 newSize){ size_ = newSize;}

    NAMemory * heap_;

    NABoolean preFetched_;

    // ---------------------------------------------------------------------
    // The time histograms for this table were last refreshed
    // ---------------------------------------------------------------------
    Int64 refreshTime_;

    // ---------------------------------------------------------------------
    // The time this table was last altered
    // ---------------------------------------------------------------------
    Int64 redefTime_;

    Int64 statsTime_;  // STATS_TIME value from SB_HISTOGRAMS table

    // ----------------------------------------------------------------
    // Do all columns of this table consis of default statistics
    // ----------------------------------------------------------------
    NABoolean allFakeStats_;

    //The full histograms
    NAList<ColStatsSharedPtr> *full_; 
    // is the memory-efficient contextheap representation 
    // of a table's single-column histograms only 

    ColumnSet singleColumnPositions_; 
    // tracks single-column histograms that are in cache

    // multicolum histograms
    MultiColumnHistogramList *multiColumn_; 
    // is the memory-efficient contextheap representation 
    // of a table's multi-column histograms only

    //pointer to qualified name of the table
    QualifiedName * name_;
    Int64 tableUID_;
    NABoolean       accessedInCurrentStatement_; 
    ULng32 size_;
};// class HistogramsCacheEntry

/****************************************************************************
** Class HistogramCache is used to cache a histograms to be used by a future
** compilation of statements on the same/subset/superset of tables.
** Internally it is basically a hashtable that hashes histograms(value) based
** on the name of the table(key)
** Currently NATable is the only class that references this particular class.
** If we decide to cache the NATable this class will not be necessary because
** NATable keeps the histograms as its private member.
** But the potential gain of caching the whole NATable compared to just caching
** the histogram for a MX table is little as we cache NATable in MX catalog.
*****************************************************************************/

class HistogramCache : public NABasicObject
{
public:
  HistogramCache(NAMemory * heap, Lng32 initSize =107);

	//method called by NATable to get the cached histogram if any
  void getHistograms(NATable& table);

	void invalidateCache();

  inline NAMemory* getHeap() { return heap_; }         

  // set an upper limit for the heap used by the HistogramCache 
  void setHeapUpperLimit (size_t newUpperLimit) { heap_->setUpperLimit(newUpperLimit); }

  inline ULng32 hits() { return hits_; } 
  inline ULng32 lookups() { return lookups_; }

  inline void resetIntervalWaterMark()
    { heap_->resetIntervalWaterMark(); }
        
  void resizeCache(size_t limit);

  inline ULng32 getSize() {return size_;}

  //reset all entries to not accessedInCurrentStatement
  void resetAfterStatement();

  void closeTraceFile();
  void openTraceFile(const char* filename);

  void closeMonitorFile();
  void openMonitorFile(const char* filename);

  FILE *getTraceFileDesc() const { return tfd_; }
  FILE *getMonitorFileDesc() const { return mfd_; }
  void traceTable(NATable& table) const;
  void traceTablesFinalize() const;
  void monitor() const;

  void freeInvalidEntries(Int32 returnedNumQiKeys,
                          SQL_QIKEY * qiKeyArray);

 private:
  DISALLOW_COPY_AND_ASSIGN(HistogramCache);

	//is a helper function for getHistograms look at .cpp file
	//for more detail
	//retreive the statistics from the cache and put
	//them into colStatsList.
  void createColStatsList(NATable& table, 
                          HistogramsCacheEntry* cachedHistograms);

    //gets the StatsList into list from the histogram cache
    //returns the number of columns whose statistics were
    //found in the cache. The columns whose statistics are required
  //are passed in through localArray. 
	Int32 getStatsListFromCache( StatsList & list, //Out
                             NAColumnArray& localArray, //In
                             HistogramsCacheEntry * cachedHistograms, //In
                             ColumnSet & singleColsFound); //In \ Out

	//This method is used to put a StatsList object, that has been
	//fetched using FetchHistograms, into the histogram cache.
	void putStatsListIntoCache(StatsList & colStatsList,
                              const NAColumnArray& colArray,
                              const QualifiedName & qualifiedName,
                              Int64 tableUID,
                              Int64 statsTime,
                              const Int64 & redefTime,
			      NABoolean allFakeStats);

  // lookup given table's histograms.
  // if found, return its HistogramsCacheEntry*.
  // otherwise, return NULL.
  HistogramsCacheEntry* lookUp(NATable& table);

  // decache entry and set it to NULL
  void deCache(HistogramsCacheEntry** entry);
  
  ULng32 entries() const;

  void display() const;
  void print( FILE* ofd = stdout,
	      const char* indent = DEFAULT_INDENT,
              const char* title = "HistogramCache") const;

  size_t memoryLimit() const { return memoryLimit_; }

  NABoolean enforceMemorySpaceConstraints();

  NAMemory * heap_;

  size_t memoryLimit_; 
  // start evicting cache entries when heap_ hits memoryLimit_

  NAList<HistogramsCacheEntry*> lruQ_; 
  // lruQ_.first is least recently used cache entry

  //The Cache
  NAHashDictionary <QualifiedName, HistogramsCacheEntry> * histogramsCache_;
        
  Int64 lastTouchTime_;    // last time cache was touched
  ULng32 hits_;            // cache hit counter
  ULng32 lookups_;         // entries lookup counter 
  ULng32 size_;
  FILE *tfd_; // trace file handle
  FILE *mfd_; // monitor file handle
}; // class HistogramCache


struct NATableEntryDetails {
      char catalog[ComMAX_1_PART_INTERNAL_UTF8_NAME_LEN_IN_BYTES + 1]; // +1 for NULL byte
      char schema[ComMAX_1_PART_INTERNAL_UTF8_NAME_LEN_IN_BYTES + 1];
      char object[ComMAX_1_PART_INTERNAL_UTF8_NAME_LEN_IN_BYTES + 1];
      int size;
};

// ***********************************************************************
// NATable : The basic (Non-Acidic) Table class
//
// A NATable contains the description of the physical schema for an
// SQL table or a table-valued stored procedure, which is a source
// of rows that cannot be decomposed. It contains information such as
// the number of columns as well as the the set of files (fileset)
// used for implementing the table. It can shared across the optimization
// of several SQL statements.
//
// One NATable is shared by one or more references to the same table
// within a given query.
//
// NATables are organized in a search structure called the NATableDB.
// It uses the qualified name (ANSI name or os name) for the table
// as the key for imposing an organization on NATables as well as the
// lookup.
//
// ***********************************************************************

class NATable : public NABasicObject
{
  friend class NATableDB;
public:

  //the type of heap pointed to by the heap_ datamember
  //this is so that in the destructor we can delete heap_ if
  //it is not a statement or a context heap.
  enum NATableHeapType { STATEMENT, CONTEXT, OTHER };

  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------

  NATable(BindWA *bindWA, const CorrName &corrName, NAMemory *heap,
          TrafDesc *inTableDesc = NULL);

  NATable(BindWA *bindWA, const CorrName &corrName, NAMemory *heap,
          struct hive_tbl_desc*);

  virtual ~NATable();

  // Obtain a list of table identifiers for all the indices and vertical
  // partitions of the NATable object, stored in tableIdList_
  const LIST(CollIndex) & getTableIdList() const;
  // void reset();      // not needed/implemented yet but see .C file for notes
  //IMPORTANT READ THIS IF U CHANGE ANYTHING IN NATABLE
  // reset stuff after statement is done so that this NATable object
  // can be used by subsequent statements (i.e. NATable Caching).
  //***************************************************************************
  //If u change anything in the NATable after NATable construction as a result
  //of statement specific state, please set it back to the value it had after
  //NATable construction.
  //***************************************************************************
  void resetAfterStatement();

  //setup this NATable for the statement.
  //This has to be done after NATable construction
  //or after an NATable has been retrieved from the cache
  void setupForStatement();

  // This method changes the partFunc of the base table to remove 
  // all partitions other than the ones specified by pName. Currently
  // this method only supports having a single partition name in pName.
  NABoolean filterUnusedPartitions(const PartitionClause& pClause);

  // by default column histograms are marked to not be fetched, 
  // i.e. needHistogram_ is initialized to DONT_NEED_HIST.
  // this method will mark columns for appropriate histograms depending on
  // where they have been referenced in the query
  void markColumnsForHistograms();

  const QualifiedName& getFullyQualifiedGuardianName();
  ExtendedQualName::SpecialTableType getTableType();
  StatsList* getColStats() { return colStats_; }

  // ---------------------------------------------------------------------
  // Accessor functions
  // ---------------------------------------------------------------------
  const ExtendedQualName &getExtendedQualName() const
        { return qualifiedName_; }
  const QualifiedName &getTableName() const
        { return qualifiedName_.getQualifiedNameObj(); }
  QualifiedName &getTableName() 
        { return qualifiedName_.getQualifiedNameObj(); }
  const NAString getSynonymReferenceName() const
        { return synonymReferenceName_; }
  const ComUID &getSynonymReferenceObjectUid() const 
        { return synonymReferenceObjectUid_; }
  NABoolean getIsSynonymTranslationDone() const
        { return isSynonymTranslationDone_;}
  const QualifiedName &getFileSetName() const
        { return fileSetName_; }
  ExtendedQualName::SpecialTableType getSpecialType() const
        { return qualifiedName_.getSpecialType(); }

  Int32 getReferenceCount() const                 { return referenceCount_; }
  void incrReferenceCount()                     { ++referenceCount_; }
  void decrReferenceCount();
  void resetReferenceCount();

  void setRefsIncompatibleDP2Halloween() 
        { refsIncompatibleDP2Halloween_ = TRUE; }
  NABoolean getRefsIncompatibleDP2Halloween() const
        { return refsIncompatibleDP2Halloween_; }
  void setIsHalloweenTable()
    { isHalloweenTable_ = TRUE; }
  NABoolean getIsHalloweenTable() const
    { return isHalloweenTable_; }

  CollIndex getColumnCount() const              { return colcount_; }
  CollIndex getUserColumnCount() const;
  const NAColumnArray &getNAColumnArray() const { return colArray_; }
  Int32 getRecordLength() const                   { return recordLength_; }
  Cardinality getEstRowCount() const
                 { return clusteringIndex_->getEstimatedNumberOfRecords(); }
  Int32 getKeyCount() const
                 {return clusteringIndex_->getIndexKeyColumns().entries();}

  const NAFileSet *getClusteringIndex() const   { return clusteringIndex_; }
  NAFileSet *getClusteringIndex()               { return clusteringIndex_; }
  const NAFileSetList &getIndexList() const     { return indexes_; }
  NABoolean hasSecondaryIndexes() const         { return indexes_.entries() >1;}
  const NAFileSetList &getVerticalPartitionList() const { return vertParts_; }

  NABoolean getCorrespondingIndex(NAList<NAString> &inputCols,
				  NABoolean explicitIndex,
				  NABoolean lookForUniqueIndex,
				  NABoolean lookForPrimaryKey,
				  NABoolean lookForAnyIndexOrPkey,
                                  NABoolean lookForSameSequenceOfCols,
                                  NABoolean excludeAlwaysComputedSystemCols,
				  NAString *indexName);
  
  NABoolean getCorrespondingConstraint(NAList<NAString> &inputCols,
				       NABoolean uniqueConstr,
				       NAString *constrName = NULL,
				       NABoolean *isPkey = NULL,
                                       NAList<int> *reorderList = NULL);
    
  const TrafDesc * getPartnsDesc() const { return partnsDesc_; }

  // A not-found partition is an offline partition.
  NABoolean containsPartition(const NAString &partitionName) const
  {
    for (CollIndex i=0; i<indexes_.entries(); i++)
      if (indexes_[i]->containsPartition(partitionName))
        return TRUE;
    return FALSE;
  }
  NABoolean isOfflinePartition(const NAString &partitionName) const
  { return !partitionName.isNull() && !containsPartition(partitionName); }

  // move relevant attributes from etTable to this.
  // Currently, column and key info is moved.
  short updateExtTableAttrs(NATable *etTable);

  const Int64 &getCreateTime() const            { return createTime_; }
  const Int64 &getRedefTime() const             { return redefTime_; }
  const Int64 &getCacheTime() const             { return cacheTime_; }
  void setCacheTime(Int64 cacheTime) { cacheTime_ = cacheTime; } 
  const Int64 &getStatsTime() const             { return statsTime_; }

  const ComUID &getCatalogUid() const           { return catalogUID_; }
  const ComUID &getSchemaUid() const            { return schemaUID_; }

  const ComUID &objectUid() const
  {
    if (objectUID_.get_value() == 0)
      const_cast<NATable*>(this)->lookupObjectUid();  // cast off const
    return objectUID_;
  }

  // fetch the object UID that is associated with the external 
  // table object (if any) for a native table.
  // Set objectUID_ to 0 if no such external table exists;
  // set objectUID_ to -1 if there is error during the fetch operation;
  NABoolean fetchObjectUIDForNativeTable(const CorrName& corrName,
                                         NABoolean isView);

  Int64 lookupObjectUid();  // Used to look up uid on demand for metadata tables.
                            // On return, the "Object Not Found" error (-1389) 
                            // is filtered out from CmpCommon::diags().

  bool isEnabledForDDLQI() const;

  const ComObjectType &getObjectType() const { return objectType_; }

  const COM_VERSION &getObjectSchemaVersion() const { return osv_; }
  const COM_VERSION &getObjectFeatureVersion() const { return ofv_; }

  const Int32 &getOwner() const { return owner_; }
  const Int32 &getSchemaOwner() const { return schemaOwner_; }

  const void * getRCB() const { return rcb_; }
  ULng32 getRCBLength() const { return rcbLen_; }
  ULng32 getKeyLength() const { return keyLength_; }

  const char * getParentTableName() const { return parentTableName_; }
  const ComPartitioningScheme &getPartitioningScheme() const { return partitioningScheme_; }
   
  const HostVar* getPrototype() const           { return prototype_; }

  const char *getViewText() const               { return viewText_; }
  const NAWchar *getViewTextInNAWchars() const
  { return viewTextInNAWchars_.length() > 0 ? viewTextInNAWchars_.data() : NULL; }
  const NAWString &getViewTextAsNAWString() const     { return viewTextInNAWchars_; }
  CharInfo::CharSet getViewTextCharSet() const    { return viewTextCharSet_; }
  NABoolean isView() const { return (viewText_ != NULL); }

  // getViewLen is needed to compute buffer len for a parseDML call
  // locale-to-unicode conversion in parseDML requires buffer len (tcr)
  Int32   getViewLen() const
  { return viewText_ ? strlen(viewText_) : 0; }
  Int32   getViewTextLenInNAWchars() const   { return viewTextInNAWchars_.length(); }

  const char *getViewCheck() const              { return viewCheck_; }
  const NAList<ComViewColUsage*> *getViewColUsages() const  { return viewColUsages_; }

  const char *getHiveOriginalViewText() const { return hiveOrigViewText_; }

  NABoolean hasSaltedColumn(Lng32 * saltColPos = NULL);
  const NABoolean hasSaltedColumn(Lng32 * saltColPos = NULL) const;
  NABoolean hasDivisioningColumn(Lng32 * divColPos = NULL);

  void setUpdatable( NABoolean value )
  {  value ? flags_ |= IS_UPDATABLE : flags_ &= ~IS_UPDATABLE; }

  NABoolean isUpdatable() const
  {  return (flags_ & IS_UPDATABLE) != 0; }

  void setInsertable( NABoolean value )
  {  value ? flags_ |= IS_INSERTABLE : flags_ &= ~IS_INSERTABLE; }

  NABoolean isInsertable() const
  {  return (flags_ & IS_INSERTABLE) != 0; }

  void setSQLMXTable( NABoolean value )
  {  value ? flags_ |= SQLMX_ROW_TABLE : flags_ &= ~SQLMX_ROW_TABLE; }

  NABoolean isSQLMXTable() const
  {  return (flags_ & SQLMX_ROW_TABLE) != 0; }

  void setSQLMXAlignedTable( NABoolean value )
  {
    (value
     ? flags_ |= SQLMX_ALIGNED_ROW_TABLE
     : flags_ &= ~SQLMX_ALIGNED_ROW_TABLE);
  }

  NABoolean isSQLMXAlignedTable() const
  {
    if (getClusteringIndex() != NULL)
       return getClusteringIndex()->isSqlmxAlignedRowFormat();
    else
       return getSQLMXAlignedTable();
  }

  NABoolean isAlignedFormat(const IndexDesc *indexDesc) const
  {
    NABoolean isAlignedFormat;

    if (isHbaseRowTable()||
      isHbaseCellTable() || (indexDesc == NULL))
      isAlignedFormat  = isSQLMXAlignedTable();
    else
      isAlignedFormat = indexDesc->getNAFileSet()->isSqlmxAlignedRowFormat();
    return isAlignedFormat;
  }
 
  void setVerticalPartitions( NABoolean value )
  {  value ? flags_ |= IS_VERTICAL_PARTITION : flags_ &= ~IS_VERTICAL_PARTITION;}

  NABoolean isVerticalPartition() const
  {  return (flags_ & IS_VERTICAL_PARTITION) != 0; }

  void setHasVerticalPartitions( NABoolean value )
  {
    value ?
       flags_ |= HAS_VERTICAL_PARTITIONS : flags_ &= ~HAS_VERTICAL_PARTITIONS;
  }

  NABoolean hasVerticalPartitions() const
  {  return (flags_ & HAS_VERTICAL_PARTITIONS) != 0; }

  void setHasAddedColumn( NABoolean value )
  {  value ? flags_ |= ADDED_COLUMN : flags_ &= ~ADDED_COLUMN; }

  NABoolean hasAddedColumn() const
  {  return (flags_ & ADDED_COLUMN) != 0; }

  void setHasVarcharColumn( NABoolean value )
  {  value ? flags_ |= VARCHAR_COLUMN : flags_ &= ~VARCHAR_COLUMN; }

  NABoolean hasVarcharColumn() const
  {  return (flags_ & VARCHAR_COLUMN) != 0; }

  void setVolatileTable( NABoolean value )
  {  value ? flags_ |= VOLATILE : flags_ &= ~VOLATILE; }

  NABoolean isVolatileTable() const
  {  return( (flags_ & VOLATILE) != 0 ); }

  void setIsVolatileTableMaterialized( NABoolean value )
  {  value ? flags_ |= VOLATILE_MATERIALIZED : flags_ &= ~VOLATILE_MATERIALIZED;}

  NABoolean isVolatileTableMaterialized() const
  {  return( (flags_ & VOLATILE_MATERIALIZED) != 0 ); }
  

  // this object was only created in mxcmp memory(catman cache, NAtable
  // cache. It doesn't exist in metadata or physical labels.
  // Used to test different access plans without actually creating
  // the object. 
  void setInMemoryObjectDefn( NABoolean value )
  {  value ? flags_ |= IN_MEM_OBJECT_DEFN : flags_ &= ~IN_MEM_OBJECT_DEFN; }

  NABoolean isInMemoryObjectDefn() const
  {  return( (flags_ & IN_MEM_OBJECT_DEFN) != 0 ); }

  void setRemoveFromCacheBNC( NABoolean value ) /* BNC = Before Next Compilation attempt */
  {  value ? flags_ |= REMOVE_FROM_CACHE_BNC : flags_ &= ~REMOVE_FROM_CACHE_BNC; }

  NABoolean isToBeRemovedFromCacheBNC() const   /* BNC = Before Next Compilation attempt */
  {  return( (flags_ & REMOVE_FROM_CACHE_BNC) != 0 ); }

  void setDroppableTable( NABoolean value )
  {  value ? flags_ |= DROPPABLE : flags_ &= ~DROPPABLE; }

  NABoolean isDroppableTable() const
  {  return( (flags_ & DROPPABLE) != 0 ); }

  ComInsertMode getInsertMode() const           { return insertMode_;}
  void setInsertMode(ComInsertMode im)          { insertMode_ = im; }

  NABoolean isSetTable() const
  {  return(insertMode_ == COM_SET_TABLE_INSERT_MODE); }

  // Set to TRUE if a SYSTEM_COLUMN is being treated as a USER_COLUMN.
  // Currently SYSKEY is treated as a USER_COLUMN if OVERRIDE_SYSKEY CQD 
  // is set to 'on'
  void setSystemColumnUsedAsUserColumn( NABoolean value )
  { value ? flags_ |= SYSTEM_COL_AS_USER_COL : flags_ &= ~SYSTEM_COL_AS_USER_COL;}
  NABoolean hasSystemColumnUsedAsUserColumn() const
  {  return( (flags_ & SYSTEM_COL_AS_USER_COL) != 0 ); }

  void setHasLobColumn( NABoolean value )
  {  value ? flags_ |= LOB_COLUMN : flags_ &= ~LOB_COLUMN; }

  NABoolean hasLobColumn() const
  {  return (flags_ & LOB_COLUMN) != 0; }

  void setHasSerializedEncodedColumn( NABoolean value )
  {  value ? flags_ |= SERIALIZED_ENCODED_COLUMN : flags_ &= ~SERIALIZED_ENCODED_COLUMN; }

  NABoolean hasSerializedEncodedColumn() const
  {  return (flags_ & SERIALIZED_ENCODED_COLUMN) != 0; }

  void setHasSerializedColumn( NABoolean value )
  {  value ? flags_ |= SERIALIZED_COLUMN : flags_ &= ~SERIALIZED_COLUMN; }

  NABoolean hasSerializedColumn() const
  {  return (flags_ & SERIALIZED_COLUMN) != 0; }

  void setIsTrafExternalTable( NABoolean value )
  {  value ? flags_ |= IS_TRAF_EXTERNAL_TABLE : flags_ &= ~IS_TRAF_EXTERNAL_TABLE; }

  NABoolean isTrafExternalTable() const
  {  return (flags_ & IS_TRAF_EXTERNAL_TABLE) != 0; }

  void setIsImplicitTrafExternalTable( NABoolean value )
  {  value ? flags_ |= IS_IMPLICIT_TRAF_EXT_TABLE : flags_ &= ~IS_IMPLICIT_TRAF_EXT_TABLE; }

  NABoolean isImplicitTrafExternalTable() const
  {  return (flags_ & IS_IMPLICIT_TRAF_EXT_TABLE) != 0; }

  void setHasExternalTable( NABoolean value )
  {  value ? flags_ |= HAS_EXTERNAL_TABLE : flags_ &= ~HAS_EXTERNAL_TABLE; }

  NABoolean hasExternalTable() const
  {  return (flags_ & HAS_EXTERNAL_TABLE) != 0; }

  void setIsHbaseMapTable( NABoolean value )
  {  value ? flags_ |= HBASE_MAP_TABLE : flags_ &= ~HBASE_MAP_TABLE; }

  NABoolean isHbaseMapTable() const
  {  return (flags_ & HBASE_MAP_TABLE) != 0; }

  void setHbaseDataFormatString( NABoolean value )
  {  value ? flags_ |= HBASE_DATA_FORMAT_STRING : flags_ &= ~HBASE_DATA_FORMAT_STRING; }

  NABoolean isHbaseDataFormatString() const
  {  return (flags_ & HBASE_DATA_FORMAT_STRING) != 0; }

  void setIsHistogramTable( NABoolean value )
  {  value ? flags_ |= IS_HISTOGRAM_TABLE : flags_ &= ~IS_HISTOGRAM_TABLE; }

  NABoolean isHistogramTable() const
  {  return (flags_ & IS_HISTOGRAM_TABLE) != 0; }

  void setHasHiveExtTable( NABoolean value )
  {  value ? flags_ |= HAS_HIVE_EXT_TABLE : flags_ &= ~HAS_HIVE_EXT_TABLE; }
  NABoolean hasHiveExtTable() const
  {  return (flags_ & HAS_HIVE_EXT_TABLE) != 0; }

  void setHiveExtColAttrs( NABoolean value )
  {  value ? flags_ |= HIVE_EXT_COL_ATTRS : flags_ &= ~HIVE_EXT_COL_ATTRS; }
  NABoolean hiveExtColAttrs() const
  {  return (flags_ & HIVE_EXT_COL_ATTRS) != 0; }

  void setHiveExtKeyAttrs( NABoolean value )
  {  value ? flags_ |= HIVE_EXT_KEY_ATTRS : flags_ &= ~HIVE_EXT_KEY_ATTRS; }
  NABoolean hiveExtKeyAttrs() const
  {  return (flags_ & HIVE_EXT_KEY_ATTRS) != 0; }

  void setIsRegistered( NABoolean value )
  {  value ? flags_ |= IS_REGISTERED : flags_ &= ~IS_REGISTERED; }

  NABoolean isRegistered() const
  {  return (flags_ & IS_REGISTERED) != 0; }

  void setIsInternalRegistered( NABoolean value )
  {  value ? flags_ |= IS_INTERNAL_REGISTERED : flags_ &= ~IS_INTERNAL_REGISTERED; }

  NABoolean isInternalRegistered() const
  {  return (flags_ & IS_INTERNAL_REGISTERED) != 0; }

  void setIsHiveExternalTable( NABoolean value )
  {  value ? flags_ |= IS_HIVE_EXTERNAL_TABLE : flags_ &= ~IS_HIVE_EXTERNAL_TABLE; }
  NABoolean isHiveExternalTable() const
  {  return (flags_ & IS_HIVE_EXTERNAL_TABLE) != 0; }

  void setIsHiveManagedTable( NABoolean value )
  {  value ? flags_ |= IS_HIVE_MANAGED_TABLE : flags_ &= ~IS_HIVE_MANAGED_TABLE; }
  NABoolean isHiveManagedTable() const
  {  return (flags_ & IS_HIVE_MANAGED_TABLE) != 0; }
 
  const CheckConstraintList &getCheckConstraints() const
                                                { return checkConstraints_; }
  const AbstractRIConstraintList &getUniqueConstraints() const
                                                { return uniqueConstraints_; }
  const AbstractRIConstraintList &getRefConstraints() const
                                                { return refConstraints_; }

  NABoolean rowsArePacked() const;

  StatsList &getStatistics();

  StatsList &generateFakeStats();

  inline CostScalar getOriginalRowCount() const { return originalCardinality_ ; }
  void setOriginalRowCount(CostScalar rowcount) {originalCardinality_ = rowcount; }

  const char * getSnapshotName() const { return snapshotName_; }
  // ---------------------------------------------------------------------
  // Standard operators
  // ---------------------------------------------------------------------
  inline NABoolean operator==(const NATable &other) const
                                              { return (this == &other); }

  const ExtendedQualName *getKey() const              { return &qualifiedName_; }

  char * getViewFileName() const              { return viewFileName_; }

  // MV
  // ---------------------------------------------------------------------
  // Materialized Views support
  // ---------------------------------------------------------------------
  const UsingMvInfoList& getMvsUsingMe()   const { return mvsUsingMe_; }
  NABoolean  isAnMV()			   const { return mvAttributeBitmap_.getIsAnMv(); }
  NABoolean  isAnMVMetaData()		   const { return isAnMVMetaData_; }
  MVInfoForDML *getMVInfo(BindWA *bindWA);
  const ComMvAttributeBitmap& getMvAttributeBitmap() const { return mvAttributeBitmap_; }
  NABoolean  verifyMvIsInitializedAndAvailable(BindWA *bindWA) const;

  NABoolean accessedInCurrentStatement(){return accessedInCurrentStatement_;}
  void setAccessedInCurrentStatement()
                                    {accessedInCurrentStatement_ = TRUE;}

  // ---------------------------------------------------------------------
  // Sequence generator support
  // ---------------------------------------------------------------------
  
  const SequenceGeneratorAttributes* getSGAttributes()  const
  { return sgAttributes_; }

  //size is the size of the table's heap
  //this is useful for NATable caching because a seperate
  //heap is created for each cached NATable
  Lng32 getSize(){return (heap_ ? heap_->getTotalSize() : 0);}

  //returns true if this is an MP table with an Ansi Name
  NABoolean isAnMPTableWithAnsiName() const{return isAnMPTableWithAnsiName_;};

  //returns true if this is a UMD table (not an MV UMD table, though)
  NABoolean isUMDTable () const{return isUMDTable_;};

  //returns true if this is a SMD table 
  NABoolean isSMDTable () const{return isSMDTable_;};

  //returns true if this is a MV UMD table 
  NABoolean isMVUMDTable () const{return isMVUMDTable_;};

  //returns true if this is a trigger temporary table
  NABoolean isTrigTempTable () const{return isTrigTempTable_;};

  //returns true if this is an exception table
  NABoolean isExceptionTable () const{return isExceptionTable_;};

  //return true if there were warnings generated during
  //the construction of this NATable object
  NABoolean constructionHadWarnings(){return tableConstructionHadWarnings_;}
  NABoolean doesMissingStatsWarningExist(CollIndexSet & listOfColPositions) const;

  NABoolean insertMissingStatsWarning(CollIndexSet colsSet) const;

  const TrafDesc * getTableDesc() const { return tableDesc_; }
  NAList<HbaseCreateOption*> * hbaseCreateOptions()
    { return clusteringIndex_->hbaseCreateOptions();}

  NABoolean isStatsFetched() const {return statsFetched_; };

  void setStatsFetched(NABoolean flag=TRUE) {statsFetched_ = flag; }

  NABoolean isPartitionNameSpecified() const 
		    { return qualifiedName_.isPartitionNameSpecified(); }

  NABoolean isPartitionRangeSpecified() const 
		    { return qualifiedName_.isPartitionRangeSpecified(); }

  NABoolean isHiveTable() const { return isHive_; }

  NABoolean isHbaseTable() const { return isHbase_; }
  NABoolean isHbaseCellTable() const { return isHbaseCell_; }
  NABoolean isHbaseRowTable() const { return isHbaseRow_; }
  NABoolean isSeabaseTable() const { return isSeabase_; }
  NABoolean isSeabaseMDTable() const { return isSeabaseMD_; }
  NABoolean isSeabasePrivSchemaTable() const 
    { return isSeabasePrivSchemaTable_; }

  NABoolean isUserUpdatableSeabaseMDTable() const { return isUserUpdatableSeabaseMD_; }

  void setIsHbaseTable(NABoolean v) { isHbase_ = v; }
  void setIsHbaseCellTable(NABoolean v) { isHbaseCell_ = v; }
  void setIsHbaseRowTable(NABoolean v) { isHbaseRow_ = v; }
  void setIsSeabaseTable(NABoolean v) { isSeabase_ = v; }
  void setIsSeabaseMDTable(NABoolean v) { isSeabaseMD_ = v; }
  void setIsUserUpdatableSeabaseMDTable(NABoolean v) 
  { isUserUpdatableSeabaseMD_ = v; }

  // returns default string length in bytes
  Int32 getHiveDefaultStringLen() const { return hiveDefaultStringLen_; }
  Int32 getHiveTableId() const                   { return hiveTableId_; }

  void setClearHDFSStatsAfterStmt(NABoolean x) 
   { resetHDFSStatsAfterStmt_ = x; };

  NABoolean getClearHDFSStatsAfterStmt() { return resetHDFSStatsAfterStmt_; };

  NAMemory* getHeap() const { return heap_; }
  NATableHeapType getHeapType() { return heapType_; }

  // Privilege related operations
  PrivMgrDescList  *getPrivDescs() { return privDescs_; }
  PrivMgrUserPrivs *getPrivInfo() const { return privInfo_; }
  void setPrivInfo(PrivMgrUserPrivs *privInfo){ privInfo_ = privInfo; }
  ComSecurityKeySet getSecKeySet() { return secKeySet_ ; }
  void setSecKeySet(ComSecurityKeySet secKeySet) { secKeySet_ = secKeySet; }

  // Get the part of the row size that is computable with info we have available
  // without accessing HBase. The result is passed to estimateHBaseRowCount(),
  // which completes the row size calculation with HBase info.
  Int32 computeHBaseRowSizeFromMetaData() const ;
  Int64 estimateHBaseRowCount(Int32 retryLimitMilliSeconds, Int32& errorCode, Int32& breadCrumb) const;
  NABoolean getHbaseTableInfo(Int32& hbtIndexLevels, Int32& hbtBlockSize) const;
  NABoolean getRegionsNodeName(Int32 partns, ARRAY(const char *)& nodeNames) const;

  static NAArray<HbaseStr>* getRegionsBeginKey(const char* extHBaseName);

  NAString &defaultColFam() { return defaultColFam_; }
  NAList<NAString> &allColFams() { return allColFams_; }

private:
  NABoolean getSQLMXAlignedTable() const
  {  return (flags_ & SQLMX_ALIGNED_ROW_TABLE) != 0; }

  // copy ctor
  NATable (const NATable & orig, NAMemory * h=0) ; //not written

  void setRecordLength(Int32 recordLength) { recordLength_ = recordLength; }

  void getPrivileges(TrafDesc * priv_desc);
  void readPrivileges();

  ExpHbaseInterface* getHBaseInterface() const;
  static ExpHbaseInterface* getHBaseInterfaceRaw();

  //size of All NATable related data after construction
  //this is used when NATables are cached and only then
  //is it meaningful as each NATable has its own heap
  Lng32 initialSize_;

  //size of All NATable related stuff after the end of the
  //previous statement. This is used when NATables are
  //cached and only then is it meaningful as each NATable
  //has its own heap. This is simply the allocated size
  //on the NATable heap.
  Lng32 sizeAfterLastStatement_;

  // -----------------------------------------------------------------------
  // The heap for the dynamic allocation of the NATable members.
  // -----------------------------------------------------------------------
  NAMemory * heap_;

  //The type of heap pointed to by heap_ (i.e. statement, context or other)
  NATableHeapType heapType_;

  // ---------------------------------------------------------------------
  // The number of table references to this table qualified name
  // there are in the entire query.
  // This count includes references in expanded view texts
  // (so includes view with-check-option check constraints),
  // but excludes references in table check constraints or RI constraints.
  // ---------------------------------------------------------------------
  Int32 referenceCount_;

  // ---------------------------------------------------------------------
  // A reference may use access options that are not compatible with the 
  // use of the DP2 Locks method of preventing the Halloween problem.
  // ---------------------------------------------------------------------
  NABoolean refsIncompatibleDP2Halloween_;

  // ---------------------------------------------------------------------
  // Helps keep track of which table, if any, in the query is the 
  // self-referencing table.
  // ---------------------------------------------------------------------
  NABoolean isHalloweenTable_;

  // Bitfield flags to be used instead of numerous NABoolean fields
  enum Flags {
    UNUSED                    = 0x00000000,
    SQLMX_ROW_TABLE           = 0x00000004,
    SQLMX_ALIGNED_ROW_TABLE   = 0x00000008,
    IS_INSERTABLE             = 0x00000010,
    IS_UPDATABLE              = 0x00000020,
    IS_VERTICAL_PARTITION     = 0x00000040,
    HAS_VERTICAL_PARTITIONS   = 0x00000080,
    ADDED_COLUMN              = 0x00000100,
    VARCHAR_COLUMN            = 0x00000200,
    VOLATILE                  = 0x00000400,
    VOLATILE_MATERIALIZED     = 0x00000800,
    SYSTEM_COL_AS_USER_COL    = 0x00001000,
    IN_MEM_OBJECT_DEFN        = 0x00002000,
    DROPPABLE                 = 0x00004000,
    LOB_COLUMN                = 0x00008000,
    REMOVE_FROM_CACHE_BNC     = 0x00010000,  // Remove from NATable Cache Before Next Compilation
    SERIALIZED_ENCODED_COLUMN = 0x00020000,
    SERIALIZED_COLUMN         = 0x00040000,
    IS_TRAF_EXTERNAL_TABLE    = 0x00080000,
    HAS_EXTERNAL_TABLE        = 0x00100000,
    IS_HISTOGRAM_TABLE        = 0x00200000,
    HBASE_MAP_TABLE           = 0x00400000,
    HBASE_DATA_FORMAT_STRING  = 0x00800000,
    HAS_HIVE_EXT_TABLE        = 0x01000000,
    HIVE_EXT_COL_ATTRS        = 0x02000000,
    HIVE_EXT_KEY_ATTRS        = 0x04000000,
    IS_IMPLICIT_TRAF_EXT_TABLE= 0x08000000,
    IS_REGISTERED             = 0x10000000,
    IS_INTERNAL_REGISTERED    = 0x20000000,

    // if underlying hive table was created as an EXTERNAL table.
    //  hive syntax: create external table ...)
    //  Note: this is different than a traf external table created for
    //        a hive table.
    IS_HIVE_EXTERNAL_TABLE    = 0x40000000,

    // if underlying hive table was not created as an EXTERNAL table.
    IS_HIVE_MANAGED_TABLE     = 0x80000000,
  };
    
  UInt32 flags_;

  // ---------------------------------------------------------------------
  // NORMAL, INDEX, VIRTUAL, etc.
  // ---------------------------------------------------------------------
  // ExtendedQualName::SpecialTableType specialType_;

  // ---------------------------------------------------------------------
  // Extended Qualified name for the table. This also has the specialType
  // (NORMAL, INDEX, VIRTUAL, etc.) and the location Name.
  // ---------------------------------------------------------------------
  ExtendedQualName qualifiedName_;

  // ---------------------------------------------------------------------
  // Save the synonym's reference object name here returned from 
  // the catalog manager as well as the UID. If catalog manager has 
  // translated a synonym name to its reference objects set the 
  // isSynonymTranslationDone_ to TRUE. 
  // ---------------------------------------------------------------------
  NAString         synonymReferenceName_;
  NABoolean 	   isSynonymTranslationDone_;
  ComUID           synonymReferenceObjectUid_;

  // ---------------------------------------------------------------------
  // Fileset for the table (to be augmented later).
  // ---------------------------------------------------------------------
  QualifiedName fileSetName_;

  // ---------------------------------------------------------------------
  // Host Var associated with table, if any. Used by generator to
  // build late-name resolution information for use at execution time.
  // ---------------------------------------------------------------------
  HostVar *prototype_;

  // ---------------------------------------------------------------------
  // Column count
  // ---------------------------------------------------------------------
  CollIndex colcount_;

  // ---------------------------------------------------------------------
  // An array of columns belonging to this table
  // ---------------------------------------------------------------------
  NAColumnArray colArray_;

  // ---------------------------------------------------------------------
  // Record length.
  // ---------------------------------------------------------------------
  Int32 recordLength_;

  // ---------------------------------------------------------------------
  // The clustering key of the base table
  // ---------------------------------------------------------------------
  NAFileSet *clusteringIndex_;

  // ---------------------------------------------------------------------
  // A list of the primary index and the alternate indexes for the table
  // ---------------------------------------------------------------------
  NAFileSetList indexes_;

  // ---------------------------------------------------------------------
  // A list of vertical partitions for the table
  // ---------------------------------------------------------------------
  NAFileSetList vertParts_;

  // ---------------------------------------------------------------------
  // A list of column statistics for this table
  // ---------------------------------------------------------------------
  StatsList * colStats_;
  NABoolean statsFetched_;
  CostScalar originalCardinality_;

  // ---------------------------------------------------------------------
  // Catalog timestamps
  // ---------------------------------------------------------------------
  Int64 createTime_;
  Int64 redefTime_;
  Int64 cacheTime_;          //cache_time from the OBJECTS row 
  Int64 statsTime_; // set by NATable::getStatistics() 

  // ---------------------------------------------------------------------
  // UIDs
  // ---------------------------------------------------------------------
  ComUID catalogUID_;
  ComUID schemaUID_;
  ComUID objectUID_;

  // ---------------------------------------------------------------------
  // Object owner
  // ---------------------------------------------------------------------
  Int32 owner_;

  // ---------------------------------------------------------------------
  // ObjectType
  // ---------------------------------------------------------------------
  ComObjectType objectType_;

  // ---------------------------------------------------------------------
  // Schema of this object owner
  // ---------------------------------------------------------------------
  Int32 schemaOwner_;

  // ---------------------------------------------------------------------
  // Partitioning Scheme - needed for parallel DDL operations.
  // ---------------------------------------------------------------------
  ComPartitioningScheme partitioningScheme_;

  // ---------------------------------------------------------------------
  // Parseable view text (includes terminating semicolon);
  // MP "WITH CHECK OPTION" text;
  // PHYSICAL filename of the view (e.g. \NSK.$VOL.ZSDN0000.AJPP0000
  //   in internal format i.e. no delimiting dquotes);
  // ---------------------------------------------------------------------
  char *viewFileName_;
  char *viewText_;
  NAWString viewTextInNAWchars_;
  CharInfo::CharSet viewTextCharSet_;
  char *viewCheck_;
  NAList<ComViewColUsage *> *viewColUsages_;

  // original hive select text used when view was created.
  char *hiveOrigViewText_;

  // ---------------------------------------------------------------------
  // Flags
  // ---------------------------------------------------------------------
  NABoolean accessedInCurrentStatement_;
  NABoolean setupForStatement_;
  NABoolean resetAfterStatement_;
  NABoolean tableConstructionHadWarnings_;
  NABoolean isAnMPTableWithAnsiName_;
  NABoolean isUMDTable_;
  NABoolean isSMDTable_;
  NABoolean isMVUMDTable_;
  NABoolean isTrigTempTable_;
  NABoolean isExceptionTable_;
  ComInsertMode insertMode_;


  // ---------------------------------------------------------------------
  // List of check constraints (only the unbound text is used;
  // the ItemExpr remains unbound here in the NATable)
  // ---------------------------------------------------------------------
  CheckConstraintList checkConstraints_;

  // ---------------------------------------------------------------------
  // Referential Integrity constraint lists
  // ---------------------------------------------------------------------
  AbstractRIConstraintList uniqueConstraints_;
  AbstractRIConstraintList refConstraints_;

  // MV
  // ---------------------------------------------------------------------
  // Materialized Views support
  // ---------------------------------------------------------------------
  NABoolean         isAnMV_; // Is this table a metarialized view?
  NABoolean         isAnMVMetaData_; // Is this table an MV metadata table?
  UsingMvInfoList   mvsUsingMe_; // List of MVs using this table.
  MVInfoForDML	   *mvInfo_;
  ComMvAttributeBitmap mvAttributeBitmap_;

  // Caching stats
  UInt32 hitCount_;
  UInt32 replacementCounter_;
  Int64  sizeInCache_;
  NABoolean recentlyUsed_;

  COM_VERSION osv_;
  COM_VERSION ofv_;

  // RCB information, to be used for parallel label operations.
  void * rcb_;
  ULng32 rcbLen_;
  ULng32 keyLength_;

  char * parentTableName_;

  char *snapshotName_;

  TrafDesc *partnsDesc_;

  TrafDesc *tableDesc_;

  // hash table to store all the column positions for which missing
  // stats warning has been generated. We are not storing ValueIdSet
  // of the columns but the column positions. This is because for cases
  // where same base table appears in both the query and the sub-query, the 
  // warning for the same column can appears twice, since the ValueId
  // for the column will be different in the two places. We don't want 
  // that
  NAHashDictionary <CollIndexSet, Int32> * colsWithMissingStats_;
  
  // cached table Id list
  LIST(CollIndex) tableIdList_;

  // Sequence generator
  SequenceGeneratorAttributes *  sgAttributes_;

  NABoolean isHive_;
  NABoolean isHbase_;
  NABoolean isHbaseCell_;
  NABoolean isHbaseRow_;
  NABoolean isSeabase_;
  NABoolean isSeabaseMD_;
  NABoolean isSeabasePrivSchemaTable_;
  NABoolean isUserUpdatableSeabaseMD_;

  NABoolean resetHDFSStatsAfterStmt_;
  Int32 hiveDefaultStringLen_;  // in bytes
  Int32 hiveTableId_;
  
  // Privilege information for the object
  //   privDescs_ is the list of all grants on the object
  //   privInfo_ are the privs for the current user
  //   secKeySet_ are the security keys for the current user
  PrivMgrDescList  *privDescs_;
  PrivMgrUserPrivs *privInfo_;
  ComSecurityKeySet secKeySet_ ;

  // While creating the index keys, the NAColumn from colArray_
  // is not used in all cases. Sometimes, a new NAColumn is 
  // constructured from the NAColumn. The variable below
  // keeps track of these new columnsa allowing us to 
  // destroy them when NATable is destroyed.
  NAColumnArray newColumns_;

  NAString defaultColFam_;
  NAList<NAString> allColFams_;
}; // class NATable


struct NATableCacheStats {
  char   contextType[8];
  ULng32 numLookups;  
  ULng32 numCacheHits;     
  ULng32 currentCacheSize;
  ULng32 highWaterMark;
  ULng32 maxCacheSize;  
  ULng32 numEntries;  
};

// ***********************************************************************
// A collection of NATables.
// ***********************************************************************
#define NATableDB_INIT_SIZE  23

class NATableDB : public NAKeyLookup<ExtendedQualName,NATable>
{
  
  friend class NATableCacheStatStoredProcedure;
  friend class NATableCacheDeleteStoredProcedure;
          
public:
  NATableDB(NAMemory *h) :
    heap_(h),
    statementTableList_(h),
    statementCachedTableList_(h),
    cachedTableList_(h),
    tablesToDeleteAfterStatement_(h),
    nonCacheableTableIdents_(h),
    nonCacheableTableList_(h),
    metaDataCached_(FALSE),
    cacheMetaData_(FALSE),
    currentCacheSize_(0),
    refreshCacheInThisStatement_(FALSE),
    useCache_(FALSE),
    maxCacheSize_(20*1024*1024),//Max cache size is 20MB by default
    highWatermarkCache_(0),                                      
    totalLookupsCount_(0),        
    totalCacheHits_(0),
    intervalWaterMark_(0),
    NAKeyLookup<ExtendedQualName,NATable> (NATableDB_INIT_SIZE,
                                           NAKeyLookupEnums::KEY_INSIDE_VALUE,
                                           h),
    hiveMetaDB_(NULL),
    replacementCursor_(0)
  {}

  NATableDB (const NATableDB & orig, NAMemory * h) :
    NAKeyLookup<ExtendedQualName,NATable> (orig, h),
    heap_(h),
    statementTableList_(h),
    statementCachedTableList_(h),
    cachedTableList_(h),
    tablesToDeleteAfterStatement_(h),
    nonCacheableTableIdents_(h),
    nonCacheableTableList_(h)
  {}

  NAHeap *getHeap() { return (NAHeap *)heap_; }
  // Obtain a list of table identifiers for the current statement.
  // Allocate the list on the heap passed.
  const LIST(CollIndex) & getStmtTableIdList(NAMemory *heap) const;

  void resetAfterStatement();

  // set an upper limit to the heap used by NATableDB
  void setHeapUpperLimit(size_t newUpperLimit) { if (heap_) heap_->setUpperLimit(newUpperLimit); }

  NATable * get(const ExtendedQualName* key, BindWA * bindWA = NULL, NABoolean findInCacheOnly = FALSE);
  NATable * get(CorrName& corrName, BindWA * bindWA,
                TrafDesc *inTableDescStruct);

  void removeNATable2(CorrName &corrName, ComQiScope qiScope, 
                      ComObjectType ot);
  void removeNATable(CorrName &corrName, ComQiScope qiScope, 
                     ComObjectType ot, 
                     NABoolean ddlXns, NABoolean atCommit);
   
  void RemoveFromNATableCache( NATable * NATablep , UInt32 currIndx );
  void remove_entries_marked_for_removal();
  static void unmark_entries_marked_for_removal();

  void free_entries_with_QI_key( Int32 numSiKeys, SQL_QIKEY* qiKeyArray );

  void setCachingOFF()
  {
    cacheMetaData_= FALSE;
    flushCache();
  }

  void setCachingON();

  inline NABoolean cachingMetaData(){ return cacheMetaData_; }

  void resizeCache(Lng32 sizeInBytes) {maxCacheSize_ = sizeInBytes;};

  void flushCacheEntriesUsedInCurrentStatement();

  void refreshCacheInThisStatement(){refreshCacheInThisStatement_=TRUE;}

  void useCache(){useCache_ = TRUE;}

  void dontUseCache(){useCache_ = FALSE;}

  NABoolean usingCache(){return useCache_;}

  NABoolean isSQUtiDisplayExplain(CorrName& corrName);
  NABoolean isSQUmdTable(CorrName& corrName);

  NABoolean isSQInternalStoredProcedure(CorrName& corrName);

  void getCacheStats(NATableCacheStats & stats);


  inline ULng32 currentCacheSize() { return currentCacheSize_; }
  inline ULng32 intervalWaterMark() { return intervalWaterMark_; }
  inline ULng32 hits() { return totalCacheHits_; }
  inline ULng32 lookups() { return totalLookupsCount_; }

  void resetIntervalWaterMark()
    { intervalWaterMark_ = currentCacheSize_; }

  // get details of this query cache entry
  void getEntryDetails
  (Int32 i,                     // (IN) : NATable cache iterator entry
   NATableEntryDetails &details); // (OUT): cache entry's details

  NABoolean empty() { return end() == 0;}
  Int32 begin() { return  0 ; }
  Int32 end() ;


  static NABoolean isHiveTable(CorrName& corrName);
  NABoolean initHiveStructForHiveTable(CorrName& corrName);

  HiveMetaData* getHiveMetaDB()
  {
    return hiveMetaDB_;
  }

 private:

  void flushCache();

  //this method tries to enforce the memory space constraints
  //on the NATable cache, by selectively deleting entries that
  //that are not needed in the current statement. It applies
  //a variant of the LRU algorithm to pick the entries to be
  //deleted.
  NABoolean enforceMemorySpaceConstraints();

  //maximum allowed size of the cache
  ULng32 maxCacheSize_;

  //current size of the cache
  ULng32 currentCacheSize_;

  //high Watermark of currentCacheSize_ ever reached for statistics
  ULng32 highWatermarkCache_;        // High watermark of currentCacheSize_   
  ULng32 totalLookupsCount_;         // NATable entries lookup counter 
  ULng32 totalCacheHits_;            // cache hit counter
  
  ULng32 intervalWaterMark_;     // resettable watermark

  //List of tables used during the current statement
  LIST(NATable *) statementTableList_;

  //List of cached tables used during the current statement
  LIST(NATable *) statementCachedTableList_;

  //List of tables in the cache
  LIST(NATable *)  cachedTableList_;

  //List of tables to be deleted
  //This list is used to collect
  //tables that are supposed to be
  //deleted during the statement.
  //Deleting during the statement
  //affects compile-time, therefore
  //such tables are collected in a
  //list and deleted after the compiled
  //statement has been sent out to the
  //executor. The delete of the elements
  //in this list is done in
  //NATableDB::resetAfterStatement. This
  //method is called indirectly by the
  //CmpStatement destructor
  LIST(NATable *) tablesToDeleteAfterStatement_;

  //List of names of special tables
  LIST(ExtendedQualName *) nonCacheableTableList_;
  LIST(CollIndex) nonCacheableTableIdents_;

  NAMemory * heap_;

  //indicates if there is something in cache
  NABoolean metaDataCached_;

  //indicates if NATables are to be cached or not
  NABoolean cacheMetaData_;

  //this flag indicates that the entries (i.e. NATable
  //objects) used during the current statement should
  //be re-read from disk instead of using the entries
  //already in the cache. This helps to refresh the
  //entries used in the current statement.
  NABoolean refreshCacheInThisStatement_;

  //indicates whether to use the NATable cache
  //or not. This TRUE for dynamic sql compiles
  //it is turned 'ON' in CmpMain::sqlcomp
  //it is turned 'OFF' after every statement in
  //NATableDB::resetAfterStatement()
  NABoolean useCache_;

  //pointer to current location in the cachedTableList_
  //used for cache entry replacement purposes
  Int32 replacementCursor_;

  HiveMetaData* hiveMetaDB_;

}; // class NATableDB

#endif  /* NATABLE_H */
