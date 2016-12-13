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
#ifndef NAFILESET_H
#define NAFILESET_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         NAFileSet.h
 * Description:  A description of a file set, i.e., a set of files
 *               that implement a table, an index, a view, etc. 
 * Created:      12/20/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "Collections.h"
#include "NAColumn.h"
#include "ObjectNames.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class NAFileSet;
class NAFileSetList;

// -----------------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------------
class PartitioningFunction;
class TrafDesc;
class HHDFSTableStats;
class HbaseCreateOption;

// -----------------------------------------------------------------------
// An enumerated type that describes how data is organized in EVERY
// file that belongs to this file set.
// A key sequenced file is implemented by a B+ tree.
// A hash file contains records whose key columns compute to the 
// same hash value when the hash function that is used for 
// distributing the data is applied to them.
// -----------------------------------------------------------------------
enum FileOrganizationEnum 
{
  KEY_SEQUENCED_FILE,
  HASH_FILE
};

// -----------------------------------------------------------------------
// A NAFileSet object describes common attributes of a set of files
// that are used for implementing a base table, a single-table or a
// multi-table index, a materialized view or any other SQL object.
// -----------------------------------------------------------------------
class NAFileSet : public NABasicObject
{
  friend class NATable;
public:

  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  NAFileSet(const QualifiedName & fileSetName,
	    const QualifiedName & extFileSetObj,
	    const NAString & extFileSetName,
	    enum FileOrganizationEnum org,
	    NABoolean isSystemTable,
	    Lng32 countOfFiles,
	    Cardinality estimatedNumberOfRecords,
	    Lng32 recordLength,
	    Lng32 blockSize,
	    Int32 indexLevels,
	    const NAColumnArray & allColumns,
	    const NAColumnArray & indexKeyColumns,
	    const NAColumnArray & horizontalPartKeyColumns,
	    PartitioningFunction * forHorizontalPartitioning,
	    short keytag,
	    Int64 redefTime,
	    NABoolean audited,
	    NABoolean auditCompressed,
	    NABoolean compressed,
	    ComCompressionType dcompressed,
	    NABoolean icompressed,
	    NABoolean buffered,
	    NABoolean clearOnPurge,
	    NABoolean packedRows,
            NABoolean hasRemotePartition,
	    NABoolean isUniqueSecondaryIndex,
            NABoolean isDecoupledRangePartitioned,
            Lng32 fileCode,
	    NABoolean isVolatile,
	    NABoolean inMemObjectDefn,
            Int64 indexUID,
            TrafDesc *keysDesc,
            HHDFSTableStats *hHDFSTableStats,
            Lng32 numSaltPartns,
            NAList<HbaseCreateOption*>* hbaseCreateOptions,
            CollHeap * h=0);

  // copy ctor
  NAFileSet (const NAFileSet & orig, CollHeap * h=0) ; //not written

  virtual ~NAFileSet();
  
  // ---------------------------------------------------------------------
  // Accessor functions
  // ---------------------------------------------------------------------
  const QualifiedName & getFileSetName() const	    { return fileSetName_; }
  const NAString      & getExtFileSetName() const   { return extFileSetName_;}
  const QualifiedName & getExtFileSetObj() const    { return extFileSetObj_; }
  const QualifiedName & getRandomPartition() const;

  const NAColumnArray & getAllColumns() const      { return allColumns_; }

  // Method to get counts for various types of columns.
  // - excludeNonUserSpecifiedAlternateIndexColumns returns the number
  //   of columns in an index that are specified directly by the
  //   user (salt included unless exludeSystemColumns is also set).
  //   Returns 0 when called on the clusteringindex.
  // - Other options should be self-explanatory.
  Int32 getCountOfColumns(
       NABoolean excludeNonKeyColumns = FALSE,
       NABoolean excludeNonUserSpecifiedAlternateIndexColumns = FALSE,
       NABoolean excludeSystemColumns = TRUE,
       NABoolean excludeAlwaysComputedSystemColumns = FALSE) const;

  const NAColumnArray & getIndexKeyColumns() const
                                              { return indexKeyColumns_; }

  const TrafDesc * getKeysDesc() const { return keysDesc_; }
  TrafDesc * getKeysDesc() { return keysDesc_; }

  Lng32 getCountOfFiles() const                  { return countOfFiles_; }

  Cardinality getEstimatedNumberOfRecords() const
                                     { return estimatedNumberOfRecords_; }
  Lng32 getRecordLength() const             { return recordLength_; }
  Lng32 getLockLength() const             { return lockLength_; }
  Lng32 getKeyLength();
  Lng32 getEncodedKeyLength();
  Lng32 getBlockSize() const                   { return blockSize_; }

  Int32 getIndexLevels() const                   { return indexLevels_; }

  Lng32 getPackingScheme() const     { return packingScheme_; }
  Lng32 getPackingFactor() const     { return packingFactor_; }

  Lng32 getFileCode() const { return fileCode_; }

  const Int64 &getIndexUID() const { return indexUID_; }
  Int64 &getIndexUID() { return indexUID_; }

  const HHDFSTableStats *getHHDFSTableStats() const { return hHDFSTableStats_; }
  HHDFSTableStats *getHHDFSTableStats()             { return hHDFSTableStats_; }

  Lng32 numSaltPartns() const { return numSaltPartns_; } 
  NAList<HbaseCreateOption*> * hbaseCreateOptions() const
    { return hbaseCreateOptions_;}

  Lng32 numMaxVersions() const { return numMaxVersions_; } 

  // ---------------------------------------------------------------------
  // Mutator functions
  // ---------------------------------------------------------------------

  void setCountOfFiles (Lng32 count)   {countOfFiles_ = count;}
  void setIndexLevels (Int32 numLevels) {indexLevels_ = numLevels;}
  void setHasRemotePartitions(NABoolean flag) {hasRemotePartition_ = flag;}
  void setPartitioningFunction(PartitioningFunction * pFunc)
                                                     {partFunc_ = pFunc;}
  void setFileSetName(QualifiedName & rhs) {fileSetName_ = rhs;}
  void setEstimatedNumberOfRecords(Cardinality newEstimate)
                     { estimatedNumberOfRecords_ = newEstimate; }

  // ---------------------------------------------------------------------
  // Query the file organization.
  // ---------------------------------------------------------------------
  NABoolean isKeySequenced() const  
                     { return (fileOrganization_ == KEY_SEQUENCED_FILE); }
  NABoolean isHashed() const      
                              { return (fileOrganization_ == HASH_FILE); }
  NABoolean isSyskeyLeading() const;
  Int32 getSysKeyPosition()  const;
  NABoolean hasSyskey() const;
  NABoolean hasOnlySyskey() const;
  NABoolean hasSingleColVarcharKey() const;

  NABoolean isDecoupledRangePartitioned() const
                    { return isDecoupledRangePartitioned_; }
 
  // ---------------------------------------------------------------------
  // Query miscellaneous flags.
  // ---------------------------------------------------------------------
  NABoolean isAudited() const			      { return audited_; }
  NABoolean isAuditCompressed() const	      { return auditCompressed_; }
  NABoolean isCompressed() const	      { return compressed_; }
  ComCompressionType getDCompressed() const   { return dcompressed_; }
  NABoolean isICompressed() const	      { return icompressed_; }
  NABoolean isBuffered() const	              { return buffered_; }
  NABoolean isClearOnPurge() const	      { return clearOnPurge_; }
  NABoolean isSystemTable() const		{ return isSystemTable_; }
  NABoolean isRemoteIndexGone() const         { return thisRemoteIndexGone_;}
  void      setRemoteIndexGone()              { thisRemoteIndexGone_=TRUE;}

  short getKeytag() const 		       { return keytag_; }

  const Int64 &getRedefTime() const		    { return redefTime_; }

  // ---------------------------------------------------------------------
  // Query the partitioning function.
  // ---------------------------------------------------------------------
  NABoolean isPartitioned() const;

  Lng32 getCountOfPartitions() const;

  NABoolean containsPartition(const NAString &partitionName) const;
  
  const NAColumnArray & getPartitioningKeyColumns() const
                                       { return partitioningKeyColumns_; }

  NAString getBestPartitioningKeyColumns(char separator) const;

  PartitioningFunction * getPartitioningFunction() const
                                                     { return partFunc_; }

  NABoolean isPacked() const { return packedRows_; };
  
  NABoolean hasRemotePartitions() const { return hasRemotePartition_; };

  // Get and Set methods for the bitFlags_.
  NABoolean uniqueIndex() const { return (bitFlags_ & UNIQUE_INDEX)  != 0; }
  void setUniqueIndex(NABoolean v)      
           { (v ? bitFlags_ |= UNIQUE_INDEX : bitFlags_ &= ~UNIQUE_INDEX); }

  NABoolean notAvailable() const { return (bitFlags_ & NOT_AVAILABLE)  != 0; }
  void setNotAvailable(NABoolean v)      
           { (v ? bitFlags_ |= NOT_AVAILABLE : bitFlags_ &= ~NOT_AVAILABLE); }

  NABoolean isVolatile() const{return (bitFlags_ & IS_VOLATILE)  != 0; }
  void setIsVolatile(NABoolean v)      
           { (v ? bitFlags_ |= IS_VOLATILE : bitFlags_ &= ~IS_VOLATILE); }

  const NABoolean isInMemoryObjectDefn() const{return (bitFlags_ & IN_MEMORY_OBJECT_DEFN)  != 0; }
  void setInMemoryObjectDefn(NABoolean v)      
           { (v ? bitFlags_ |= IN_MEMORY_OBJECT_DEFN : bitFlags_ &= ~IN_MEMORY_OBJECT_DEFN); }


  NABoolean isCreatedExplicitly() const{return (bitFlags_ & IS_EXPLICIT_INDEX)  != 0; }
  void setIsCreatedExplicitly(NABoolean v)      
           { (v ? bitFlags_ |= IS_EXPLICIT_INDEX : bitFlags_ &= ~IS_EXPLICIT_INDEX); }

  //For NATable caching, clear up the statement specific stuff
  //so that this can be used for following statements
  void resetAfterStatement();

  //For NATable caching, prepare for use by a statement.
  void setupForStatement();

  NABoolean isSqlmxAlignedRowFormat() const { return rowFormat_ == COM_ALIGNED_FORMAT_TYPE; }

  void setRowFormat(ComRowFormat rowFormat) { rowFormat_ = rowFormat; }

private:

  // ---------------------------------------------------------------------
  // The fully qualified PHYSICAL name for the file set,
  // e.g. { "\NSK.$VOL", ZSDNGUOF, T5AB0000 }.
  // ---------------------------------------------------------------------
  QualifiedName fileSetName_;

  // ---------------------------------------------------------------------
  // The EXTERNAL name of fileset, as specified by the user.
  // For ARK tables, this is the ANSI name of the index/basetable specified
  // when the index/basetable was created (the preceding field, fileSetName_,
  // thus represents the actual physical name of the index).
  //
  // For SQL/MP or simulator tables, both fileSetName_ and extFileSetName_
  // fields are nearly the same, just formatted differently (one without
  // double-quote delimiters, one with).
  // ---------------------------------------------------------------------
  NAString extFileSetName_;
  QualifiedName extFileSetObj_;

  // ---------------------------------------------------------------------
  // File organization.
  // The following enum describes how records are organized in each
  // file of this file set.
  // ---------------------------------------------------------------------
  FileOrganizationEnum fileOrganization_;
  
  // ---------------------------------------------------------------------
  // The number of files that belong to this file set.
  // (May need a set of file names/NAFile also/instead someday.)
  // ---------------------------------------------------------------------
  Lng32 countOfFiles_;

  // ---------------------------------------------------------------------
  // The number of records can either be a value that is computed by
  // UPDATE STATISTICS or may be estimated by examining the EOFs of
  // each file in this file set and accumulating an estimate.
  // ---------------------------------------------------------------------
  Cardinality estimatedNumberOfRecords_;
  
  // ---------------------------------------------------------------------
  // Record length in bytes.
  // ---------------------------------------------------------------------
  Lng32 recordLength_;

  // ---------------------------------------------------------------------
  // Key length in bytes.
  //----------------------------------------------------------------------
  Lng32 keyLength_;

  // ---------------------------------------------------------------------
  // Encoded key length in bytes.
  //----------------------------------------------------------------------
  Lng32 encodedKeyLength_;

  // ---------------------------------------------------------------------
  // Lock length in bytes.
  //----------------------------------------------------------------------
  Lng32 lockLength_;

  // ---------------------------------------------------------------------
  // Filecode.
  // ---------------------------------------------------------------------
  Lng32 fileCode_;

  // ---------------------------------------------------------------------
  // Size of a page (block) that is a contant for every file that 
  // belongs to this file set. It is expressed in bytes.
  // ---------------------------------------------------------------------
  Lng32 blockSize_;
  
  // ----------------------------------------------------------------------
  // Packing information. Packing scheme: describes version of packed
  // record format used in this file set's packed records.
  // Packing factor: number of unpacked (logical) rows per packed (physical) record.
  // ----------------------------------------------------------------------
  Lng32      packingScheme_;
  Lng32      packingFactor_;

  // ---------------------------------------------------------------------
  // since the index levels could be different for every file that  
  // belongs to this file set, we probably should save index levels
  // per file. But, for now, we save the maximum of the index levels
  // of the files that belong to this fileset.                      
  // ---------------------------------------------------------------------
  Int32 indexLevels_;
  
  // ---------------------------------------------------------------------
  // Array of all the columns that appear in each record in every file
  // that belongs to this file set.
  // ---------------------------------------------------------------------
  NAColumnArray allColumns_;

  // ---------------------------------------------------------------------
  // Array of key columns that implement a B+ tree or hash index per
  // file that belongs to this file set.
  // ---------------------------------------------------------------------
  NAColumnArray indexKeyColumns_;

  // uid for index
  Int64 indexUID_;

  TrafDesc *keysDesc_;  // needed for parallel label operations.

  // ---------------------------------------------------------------------
  // Horizontal partitioning:
  // The partitioning function describes how data is distributed 
  // amongst the files contained in this file set. Note that the scheme
  // for distributing the data is orthogonal to the scheme for 
  // organizing records within a file.
  // ---------------------------------------------------------------------

  // --------------------------------------------------------------------
  // For persistent range or hash partitioned data, the partitioning 
  // key is expressed as a list of NAColumns. The list represents the 
  // sequence in which the key columns appear in the declaration of 
  // the partitioning scheme such as the SQL CREATE TABLE statement. 
  // It does not imply an ordering on the key columns. If an ordering 
  // exists, it is implemented by a partitioning function. 
  // 
  // A NAFileSet need not contain a partitioning key.
  // --------------------------------------------------------------------
  NAColumnArray partitioningKeyColumns_;
  
  PartitioningFunction * partFunc_;

  // ---------------------------------------------------------------------
  // Some day we may support arbitrary queries, then an additional
  // data member could hold the relational expression that defines
  // the content of the file set. The file set columns would then refer
  // to the result of this expression.
  // ---------------------------------------------------------------------
  // RelExpr *fileSetQuery_; // unbound fileSet definition (no value ids)

  // ---------------------------------------------------------------------
  // This member is poorly named.  For all base tables (MP or MX),
  // i.e. for all primary index NAFilesets, this is zero.
  // For all secondary indexes (MP or MX), this is nonzero.
  // ---------------------------------------------------------------------
  short keytag_;
  
  // ---------------------------------------------------------------------
  // Catalog timestamp for this fileset. Each index gets its own timestamp.
  // ---------------------------------------------------------------------
  Int64 redefTime_;

  // ---------------------------------------------------------------------
  // Miscellaneous flags
  // ---------------------------------------------------------------------
  NABoolean audited_;
  NABoolean auditCompressed_;
  NABoolean compressed_; 
  ComCompressionType dcompressed_;
  NABoolean icompressed_;
  NABoolean buffered_;
  NABoolean clearOnPurge_;
  NABoolean isSystemTable_;
  NABoolean packedRows_;
  NABoolean hasRemotePartition_;
  NABoolean setupForStatement_;
  NABoolean resetAfterStatement_;
  NABoolean isDecoupledRangePartitioned_;

  // ---------------------------------------------------------------------
  // Miscellaneous Bit flags. It reduces compiler memory consumption +
  // no need to recompile the world on adding a new flag.
  // ---------------------------------------------------------------------
  enum BitFlags
  {
    // Set, if this is a unique secondary index.
    UNIQUE_INDEX = 0x0001,

    // Set, if this index is not available.
    // Could be due to no populate, or if an mv has not been
    // refreshed, or others...
    NOT_AVAILABLE = 0x0002,

    // if this is a volatile index
    IS_VOLATILE = 0x0004,

    // if set, indicates that this object is created in mxcmp/catman
    // memory and not available in metadata or disk.
    // Used to test out plans on tables/indexes without actually
    // creating them.
    IN_MEMORY_OBJECT_DEFN = 0x0008,

    // if this index was explicitly created by user and not internally
    // created to implement a unique constraint.
    IS_EXPLICIT_INDEX = 0x00010
    
      
  };
  ULng32 bitFlags_;

  NABoolean thisRemoteIndexGone_;

  // ---------------------------------------------------------------------
  // HDFS file-level stats for Hive tables
  // ---------------------------------------------------------------------
  HHDFSTableStats *hHDFSTableStats_;

  // number of salted partitions specified at table create time.
  Lng32 numSaltPartns_;

  // if table was created with max versions greater than 1 using
  // hbase_options clause.
  Lng32 numMaxVersions_;

  NAList<HbaseCreateOption*> * hbaseCreateOptions_;

  ComRowFormat rowFormat_; 
  
};

class NAFileSetList : public LIST(NAFileSet *)
{
public:
  NAFileSetList(CollHeap* h/*=0*/) :  LIST(NAFileSet *)(h) {  }
};

#endif /* NAFILESET_H */

