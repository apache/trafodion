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
#ifndef STMTDDLCREATETABLE_H
#define STMTDDLCREATETABLE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLCreateTable.h
 * Description:  class for Create Table Statement (parser node)
 *
 *
 * Created:      3/9/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ComSmallDefs.h"
#include "ElemDDLNode.h"
#include "ElemDDLColDefArray.h"
#include "ElemDDLColRefArray.h"
#include "ElemDDLDivisionClause.h"
#include "ElemDDLSaltOptions.h"
#include "ElemDDLLikeCreateTable.h"
#include "ElemDDLLocation.h"
#include "ElemDDLPartitionArray.h"
#include "ElemDDLSGOptions.h"
#include "ElemDDLTableFeature.h"
#include "ItemConstValueArray.h"
#include "ParDDLFileAttrsCreateTable.h"
#include "ParDDLLikeOptsCreateTable.h"
#include "StmtDDLNode.h"
#include "StmtDDLAddConstraintArray.h"
#include "StmtDDLAddConstraintCheckArray.h"
#include "StmtDDLAddConstraintRIArray.h"
#include "StmtDDLAddConstraintUniqueArray.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLCreateTable;
class StmtDDLCreateHbaseTable;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Create Table statement
// -----------------------------------------------------------------------
class StmtDDLCreateTable : public StmtDDLNode
{

  //
  // The following public global functions help to improve
  // performance.  They are passed to the method traverseList
  // of the class ElemDDLList.  For information, please read
  // the descriptions of the method ElemDDLList::traverseList.
  //

  // Visit an element in a left linear tree list.  Each element
  // represents either a column definition or a table constraint
  // definition.
  //
  // pNode contains a pointer pointing to a Create Table parse
  //   node (this node).
  // index contains the index of the element in the list.  Each
  //   element is a leaf node in the left linear tree representing
  //   the list.
  // pElement contains a pointer pointing to the element in the
  //   list.
  //
  // All parameters are input parameters passed by the method
  // traverseList of class ElemDDLList (or a class derived from
  // class ElemDDLList).
  //
  // StmtDDLCreateTable_visitTableDefElement is a global function
  // defined in StmtDDLCreate.C
  //
  friend void StmtDDLCreateTable_visitTableDefElement(
       ElemDDLNode * pThisNode,
       CollIndex index,
       ElemDDLNode * pElement);

  // Visit an element in a left linear tree list.  Each element
  // contains information about partition attributes in a partition.
  //
  // StmtDDLCreateTable_visitPartitionElement is a global function
  // defined in StmtDDLCreate.C
  //
  friend void StmtDDLCreateTable_visitPartitionElement(
       ElemDDLNode * pThisNode,
       CollIndex index,
       ElemDDLNode * pElement);

public:

  // (default) constructor
  StmtDDLCreateTable(const QualifiedName & TableQualName,
                     ElemDDLNode * pTableBodyDefinition,
                     ElemDDLNode * pCreateTableAttributeList = NULL,
		     ElemDDLNode * pInsertColumnsList = NULL,
		     RelExpr     * queryExpression = NULL,
                     CollHeap    * heap = PARSERHEAP());

  // virtual destructor
  virtual ~StmtDDLCreateTable();

  // cast
  virtual StmtDDLCreateTable * castToStmtDDLCreateTable();

  // ---------------------------------------------------------------------
  // accessors
  // ---------------------------------------------------------------------

  // methods relating to parse tree
  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);


  inline const QualifiedName & getTableNameAsQualifiedName() const;
  inline       QualifiedName & getTableNameAsQualifiedName();

  inline const QualifiedName & getOrigTableNameAsQualifiedName() const;
  inline       QualifiedName & getOrigTableNameAsQualifiedName();

  inline       StmtDDLAddConstraintCheckArray & getAddConstraintCheckArray();
  inline const StmtDDLAddConstraintCheckArray & getAddConstraintCheckArray()
                                                const;

  inline       StmtDDLAddConstraintPK * getAddConstraintPK() const;

  inline       StmtDDLAddConstraintRIArray & getAddConstraintRIArray();
  inline const StmtDDLAddConstraintRIArray & getAddConstraintRIArray() const;

  inline       StmtDDLAddConstraintUniqueArray & getAddConstraintUniqueArray();
  inline const StmtDDLAddConstraintUniqueArray & getAddConstraintUniqueArray()
                                                 const;

  inline const ElemDDLColDefArray & getColDefArray() const;
  inline       ElemDDLColDefArray & getColDefArray();

        // returns an array of pointers pointing
        // to Column Definition parse node.  If
        // LIKE clause is specifed, the returned
        // array is empty.  (Note that the list
        // of column definition and Like clause
        // can not both appear within a Create
        // Table statement.)

  inline       ParDDLFileAttrsCreateTable & getFileAttributes();
  inline const ParDDLFileAttrsCreateTable & getFileAttributes() const;

        // returns the object containing the file attributes
        // associating with the table being created.  Please
        // note that some file attributes in the returned
        // object only apply to the primary partition of the
        // table.

  inline NABoolean getIsConstraintPKSpecified() const;

        // returns TRUE if a primary key (column or table)
        // constraint definition appears; returns FALSE
        // otherwise.

  inline NABoolean getIsLikeOptionSpecified() const;

        // returns TRUE if LIKE clause appears;
        // returns FALSE otherwise.

  inline NABoolean getIsLocationSpecified() const;

        // returns TRUE if LOCATION clause/phrase associating
        // with the primary partition appears; returns FALSE
        // otherwise.

  inline NABoolean getIsDivisionClauseSpecified() const;

        // returns TRUE if DIVISION BY clause appears;
        // returns FALSE otherwise.

  inline NABoolean getIsStoreBySpecified() const;

        // returns TRUE if STORE BY clause appears;
        // returns FALSE otherwise.

  inline NABoolean getIsUniqueStoreByKeylistSpecified() const;

        // returns TRUE if STORE BY clause has a UNIQUE key list;
        // returns FALSE otherwise.

  inline NABoolean getIsUniqueStoreByPrimaryKeySpecified() const;

        // returns TRUE if STORE BY clause has a UNIQUE primary key;
        // returns FALSE otherwise.

  inline NABoolean getIsHashV1PartitionSpecified() const;
       //
       // Returns TRUE if HASH PARTITION was specified; otherwise FALSE
       //

  inline NABoolean getIsHashV2PartitionSpecified() const;

       //
       // Returns TRUE if HASH2 PARTITION was specified; otherwise FALSE
       //

  inline NABoolean getIsRoundRobinPartitioningSpecified() const;

       // returns TRUE if ROUND ROBIN partitioning is specified;
       // returns FALSE otherwise. 
       //

        // returns TRUE if this is an internally generated CREATE TABLE
        // statement for creating a VP; returns FALSE otherwise.

  inline       ElemDDLColRefArray & getKeyColumnArray();
  inline const ElemDDLColRefArray & getKeyColumnArray() const;

        // return the array of pointers pointing to the parse
        // nodes representing the key columns appear in a
        // STORE BY clause.  If no key columns appear, the
        // returned array is empty.

  inline const CorrName & getLikeSourceTableCorrName() const;
  inline const NAString getLikeSourceTableName() const;
  inline const ExtendedQualName::SpecialTableType getLikeSourceTableType() const; 
  inline const QualifiedName & getOrigLikeSourceTableName() const;
  inline       QualifiedName & getOrigLikeSourceTableName();

        // returns the name of the source table in the LIKE
        // clause.  If LIKE clause is not specified, an
        // empty string is returned.


  void setLikeSourceTableName(CorrName &srcName);

  inline const ParDDLLikeOptsCreateTable & getLikeOptions() const;
  inline       ParDDLLikeOptsCreateTable & getLikeOptions();

        // returns an object containing information about
        // all LIKE options associating with the specified
        // Create Table.  If LIKE clause is not specified,
        // the returned information is not relevant.

  inline const NAString & getGuardianLocation() const;

        // returns an empty string unless the parse node
        // is bound (the method bindNode is invoked).
        // After the parse node is bound, returns the
        // location of the primary partition in Guardian
        // physical device name format.  If the LOCATION
        // clause is not specified, a default location is
        // used.

  inline const NAString & getLocation() const;

        // returns the location name specified in the LOCATION
        // clause/phrase associating with the primary partition;
        // returns an empty string if the LOCATION clause
        // associating with the primary partition does not
        // appear.

  inline NAString getLocationName() const;

        // same as getLocation()

  inline NAString getPartitionName() const;

        // returns the name of the partition name if LOCATION
        // is specified. An empty string is passes if LOCATION
        // is not specified. 

  inline ElemDDLLocation::locationNameTypeEnum getLocationNameType() const;

        // returns the type of the location name (e.g., an OSS
        // path name, a Guardian device name, an OSS environment
        // variable name, etc.)  If LOCATION clause does not
        // appear, the returned value has no meaning.

  inline const ElemDDLPartitionArray & getPartitionArray() const;
  inline ElemDDLPartitionArray & getPartitionArray();

        // returns an array of pointers pointing
        // to Partition parse nodes (each Partition
        // parse node contains all legal attributes
        // associating with a partition)

  ComPartitioningScheme getPartitioningScheme() const;

  inline const ElemDDLColRefArray & getPrimaryKeyColRefArray() const;
  inline       ElemDDLColRefArray & getPrimaryKeyColRefArray();

        // returns column name list in primary key clause if
        // specified; otherwise, an empty array is returned.

  inline const ElemDDLColRefArray & getPartitionKeyColRefArray() const;
  inline       ElemDDLColRefArray & getPartitionKeyColRefArray();

        // returns column name list in partition by clause if
        // specified; otherwise, an empty array is returned.

  inline ComStoreOption getStoreOption() const;

        // returns the store option specified in the STORE BY
        // clause.  If the STORE BY clause is not specfied,
        // returns COM_UNKNOWN_STORE_OPTION.
        //

  inline const NAString getTableName() const;

  inline ExtendedQualName::SpecialTableType getTableType() const; // ++MV
  inline NABoolean isSpecialTypeSpecified() const; // ++MV

  inline NABoolean isLikeOptionSpecified() const;

        // same as getIsLikeOptionSpecified() -
        // returns TRUE if LIKE clause appears;
        // returns FALSE otherwise.

  inline NABoolean isLocationSpecified() const;

        // same as getIsLocationSpecified() -
        // returns TRUE if the location clause/phrase appears;
        // returns FALSE otherwise.

  inline NABoolean isPartitionSpecified() const;

        // returns TRUE if the PARTITION clause appears;
        // returns FALSE otherwise.

  inline NABoolean isPartitionBySpecified() const;

        // returns TRUE if the PARTITION BY clause appears;
        // returns FALSE otherwise.

  inline NABoolean isDivisionClauseSpecified() const;

        // returns TRUE if the DIVISION BY clause appears;
        // returns FALSE otherwise.

  inline NABoolean isHbaseOptionsSpecified() const;

        // returns TRUE if HBASE_OPTIONS clause appears;
        // returns FALSE otherwise.

  inline NABoolean isStoreBySpecified() const;

        // returns TRUE if the STORE BY clause appears;
        // returns FALSE otherwise.

  inline NABoolean isUniqueStoreByKeylistSpecified() const;

        // returns TRUE if the STORE BY clause has a UNIQUE keylist;
        // returns FALSE otherwise.

  inline NABoolean isUniqueStoreByPrimaryKeySpecified() const;

        // returns TRUE if the STORE BY clause has a UNIQUE primary key;
        // returns FALSE otherwise.

  inline NABoolean isAttributeSpecified() const;

        // returns TRUE if the attribute clause appears;
        // returns FALSE otherwise.

  inline NABoolean isTableFeatureSpecified() const;

        // returns TURE if table feature clause appears;
        // returns FALSE otherwise

  inline const NAString &getHiveOptions()
  {return hiveOptions_; }
  void setHiveOptions(NAString h)
  {hiveOptions_ = h;}

  // POS
  inline NABoolean isPOSNumPartnsSpecified() const;
  inline NABoolean isPOSInitialTableSizeSpecified() const;
  inline NABoolean isPOSMaxTableSizeSpecified() const;
  inline NABoolean isPOSDiskPoolSpecified() const;
  inline NABoolean isPOSIgnoreSpecified() const;
  inline NABoolean isNumRowsSpecified() const;
  inline NABoolean isIndexLevelsSpecified() const;
  inline NABoolean isPartnEOFSpecified() const;
  
  inline ComSInt32 getPOSNumPartns() const;
  inline ComSInt32 getPOSInitialTableSize() const;
  inline ComSInt32 getPOSMaxTableSize() const;
  inline ComSInt32 getPOSDiskPool() const;
  inline ComSInt32 getPOSNumDiskPools() const;
  inline NABoolean getPOSIgnore() const;
  inline double getNumRows() const;
  inline ComSInt32 getIndexLevels() const;
  inline ComSInt64 getPartnEOF() const;

  inline void setPOSNumPartns(ComSInt32 partns);
  inline void setIsPOSNumPartnsSpecified(NABoolean flag);
  inline void setPOSDiskPool(ComSInt32 diskPool);
  inline void setPOSNumDiskPools(ComSInt32 diskPool);

  inline ComInsertMode getInsertMode() const;

  ComBoolean isRegularTable() const   
  {return (eInsertMode_ == COM_REGULAR_TABLE_INSERT_MODE);}
  ComBoolean isSetTable() const      
  {return (eInsertMode_ == COM_SET_TABLE_INSERT_MODE);}
  ComBoolean isMultiSetTable() const  
  {return (eInsertMode_ == COM_MULTISET_TABLE_INSERT_MODE);}

  inline const ElemDDLSGOptions* getSGOptions() const;
  inline       ElemDDLSGOptions* getSGOptions();
  inline       void setSGOptions(ElemDDLSGOptions* pSGOptions);

  inline const NAString &getAuthId() const;
  inline       NAString &getAuthId();

  inline       ElemDDLDivisionClause::divisionTypeEnum getDivisionType() const;
  inline       ItemExprList * getDivisionExprList();
  inline       ElemDDLDivisionClause * getDivisionClause();
  inline       ElemDDLColRefArray * getDivisionColRefArray();
  inline       ElemDDLSaltOptionsClause * getSaltOptions();
  inline       ElemDDLColRefArray * getSaltColRefArray();

  inline       ElemDDLHbaseOptions * getHbaseOptionsClause();

  // ---------------------------------------------------------------------
  // mutators
  // ---------------------------------------------------------------------

  void setChild(Lng32 index, ExprNode * newNode);

  void setConstraint(ElemDDLNode * pConstraint);

        // sets the Constraint data member with the information in
        // the parse node pointed by pConstraint.  This parse node
        // represents either a column or a table constraint definition.

  inline void setIsStoreByClauseSpecifiedFlag(NABoolean bFlag = TRUE)
                                                 { isStoreByClauseSpec_ = bFlag; }
  inline void setStoreOption(ComStoreOption storeOpt) { storeOption_ = storeOpt; }

  inline void setIsRoundRobinPartitioningSpecified();

  inline void setTableType(ExtendedQualName::SpecialTableType tableType);

  inline void setInsertMode(ComInsertMode insertMode);

  // sets the Constraint data member with the information in
  // the parse node pointed by pConstraint.  This parse node
  // represents either a column or a table constraint definition.


  inline const ElemDDLNode * insertColumnsList() const;
  inline       ElemDDLNode * insertColumnsList();

  inline const RelExpr * getQueryExpression() const;
  inline       RelExpr * getQueryExpression();

  // sets & gets the start position of the 'AS' query within the input string
  const StringPos getStartOfCreateTableQueryPosition() const;
  inline void setStartOfCreateTableQueryPosition(const StringPos startPos);

  // sets & gets the start & end position of the table attribute list within
  // the input string.
  // Ex: create table t max table size 100 as select * from t;
  //    will set the start position of 'max' token
  //    and set the end position to 'as' token.
  const StringPos getStartOfCreateTableAsAttrListPosition() const;
  inline void setStartOfCreateTableAsAttrListPosition(const StringPos startPos);
  const StringPos getEndOfCreateTableAsAttrListPosition() const;
  inline void setEndOfCreateTableAsAttrListPosition(const StringPos endPos);
  const Int32 getCreateTableAsIsoMapping() const;
  inline void setCreateTableAsIsoMapping(const Int32 charset);
  const Int32 getCreateTableAsScannedInputCharset() const;
  inline void setCreateTableAsScannedInputCharset(const Int32 charset);

  //void setTableTraits(StmtDDLTableTraits tableTraits );

  // ---------------------------------------------------------------------
  // other public methods
  // ---------------------------------------------------------------------

  // method for binding
  ExprNode * bindNode(BindWA *bindWAPtr);

  // method for collecting information
  void synthesize();

        // collects information in the parse sub-tree and
        // copy/move them to the current parse node.

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;

  void setCTAcolumnsAreRenamed(NABoolean r);
  NABoolean ctaColumnsAreRenamed();

  void setLoadIfExists(NABoolean l);
  NABoolean loadIfExists();

  void setNoLoad(NABoolean l);
  NABoolean noLoad();

  void setDeleteData(NABoolean d);
  NABoolean deleteData();

  void setTableFeature(ElemDDLTableFeature * pFeature);
  NABoolean isDroppable();
  NABoolean isInsertOnly();

  // explain support for tables created without the LIKE clause.
  virtual NABoolean explainSupported() { return (getIsLikeOptionSpecified() ? FALSE : TRUE); }

  NABoolean createIfNotExists() { return createIfNotExists_; }
  void setCreateIfNotExists(NABoolean v) { createIfNotExists_ = v; }

  NABoolean mapToHbaseTable() { return mapToHbaseTable_; }
  void setMapToHbaseTable(NABoolean v) { mapToHbaseTable_ = v; }

  NABoolean isHbaseDataFormatString() { return (hbaseDataFormat_ == TRUE); }
  void setHbaseDataFormat(NABoolean v) { hbaseDataFormat_ = v; }

private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  void computeDefaultPrimaryPartition();

        // Computes the default partitioning scheme and then
        // creates the Primary Partition parse node.  This
        // method should not be invoked unless the Partition
        // clause does not appear.

  void setFileAttributes(ElemDDLFileAttrClause * pFileAttrClause);

        // Copies the information in the specified file
        // attribute clause (pointed to by pFileAttrClause)
        // to data member fileAttributes_ in this object.
        //
        // This method can only be invoked during the
        // construction of this object when the (file)
        // attributes clause appears.

  void setMVFileAttributes(ElemDDLMVFileAttrClause * pMVFileAttrClause);

		// the same as file attributes but apply only to MVs

  void setPartitions(ElemDDLPartitionClause * pPartitionClauseNode);

        // Copies the information in the specified parse node
        // to the partitionArray_.
        //
        // This method can only be invoked during the construction
        // of this object when the PARTITION clause appears.

  void setPrimaryPartition(ElemDDLNode * pFirstSecondaryPartitionNode);

        // Allocates the primary partition node and inserts its
        // pointer at the beginning of the partitionArray_.  The
        // kind of the primary partition node must be the same
        // as that of the specified secondary partition node.
        //
        // This method is only invoked during the construction
        // of this object when the PARTITION clause appears.

  void setSecondaryPartition(ElemDDLPartition * pPartitionNode);

        // Copies the information in the specified partition node
        // (pointed to by pPartitionNode) to the partitionArray_
        // in this object.
        //
        // This method can only be invoked during the construction
        // of this object when the PARTITION clause appears.

  void setTableOption(ElemDDLNode * pTableOption);

        // Copies the information in the specified table option
        // (e.g., a PARTITION clause, a file attribute clause,
        // a location clause, etc.) to this object.
        //
        // This method can only be invoked during the construction
        // of this object.
  void checkHbasePartitionKey();

  //
  // please do not use the following methods
  //

  StmtDDLCreateTable();                                        // DO NOT USE
  StmtDDLCreateTable(const StmtDDLCreateTable &);              // DO NOT USE
  StmtDDLCreateTable & operator=(const StmtDDLCreateTable &);  // DO NOT USE

  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------

  // the tablename specified by user in the create stmt.
  // This name is not fully qualified during bind phase.
  QualifiedName origTableQualName_;

  // The syntax of table name is
  // [ [ catalog-name . ] schema-name . ] table-name
  QualifiedName tableQualName_;

  // ++ MV
  // The type of the table
  ExtendedQualName::SpecialTableType tableType_;
  NABoolean isSpecialTypeSpecified_;
  // --MV

  // insert mode
  ComInsertMode eInsertMode_;

  // list of column definitions

  ElemDDLColDefArray columnDefArray_;

  // LIKE clause related information

  NABoolean isLikeClauseSpec_;
  CorrName likeSourceTableCorrName_; 
  QualifiedName origLikeSourceTableQualName_;
  ParDDLLikeOptsCreateTable likeOptions_;

  // ATTRIBUTES clause
  //   list of file attributes

  NABoolean isAttributeClauseSpec_;
  ParDDLFileAttrsCreateTable fileAttributes_;


  // MVATTRIBUTE clause
  // the list is stored in the fileAttributes_ 
  NABoolean	isMVFileAttributeClauseSpec_;


  // LOCATION clause
  //   for the primary partition

  NABoolean isLocationClauseSpec_;
  NAString locationName_;
  NAString partitionName_;
  ElemDDLLocation::locationNameTypeEnum locationNameType_;

  // guardianLocation_ is empty until the parse node is
  // bound (the method bindNode() is invoked).  The method
  // bindNode() converts the specified location (in the
  // LOCATION clause associating with the primary key) to
  // a Guardian physical device and then saves the computed
  // name in this data member.  If the LOCATION clause does
  // not appear, a default location name is selected.
  //
  NAString guardianLocation_;

  // PRIMARY KEY clause
  //
  //   This clause appears in either column or table
  //   constraint definition

  NABoolean isPrimaryKeyClauseSpec_;
  StmtDDLAddConstraintPK * pAddConstraintPK_;
  ElemDDLColRefArray primaryKeyColRefArray_;

  // PARTITION clause
  //   list of partition definitions

  NABoolean isPartitionClauseSpec_;
  ElemDDLPartitionArray partitionArray_;

  // HASH PARTITION 
  NABoolean isHashV1PartitionSpec_;  // Old hash partition scheme
  NABoolean isHashV2PartitionSpec_;  // New hash partition scheme

  // PARTITION BY clause
  NABoolean isPartitionByClauseSpec_;
  ElemDDLColRefArray partitionKeyColRefArray_;

  // Round Robin Partitioning -- no syntax yet

  NABoolean isRoundRobinPartitioningSpecified_;

  // DIVISION BY clause

  NABoolean isDivisionByClauseSpec_;
  ElemDDLDivisionClause *pDivisionByClauseParseNode_;

  // SALT clause for HBase tables
  ElemDDLSaltOptionsClause *pSaltOptions_;

  // STORE BY clause

  NABoolean isStoreByClauseSpec_;
  NABoolean isUniqueStoreByKeylist_;
  NABoolean isUniqueStoreByPrimaryKey_;
  ComStoreOption storeOption_;
  ElemDDLColRefArray keyColRefArray_;

  // HBASE OPTIONS clause

  NABoolean isHbaseOptionsSpec_;
  ElemDDLHbaseOptions *pHbaseOptionsParseNode_;

  // POS
  NABoolean isPOSNumPartnsSpecified_;
  ComSInt32 posNumPartns_;
  
  NABoolean isPOSInitialTableSizeSpecified_;
  NABoolean isPOSMaxTableSizeSpecified_;
  NABoolean isPOSIgnoreSpecified_;
  ComSInt32 posInitialTableSize_;
  ComSInt32 posMaxTableSize_;
  NABoolean posIgnore_;

  NABoolean isPOSDiskPoolSpecified_;
  ComSInt32 posDiskPool_;
  ComSInt32 posNumDiskPools_;

  NABoolean isNumRowsSpecified_;
  double numRows_;

  NABoolean isIndexLevelsSpecified_;
  ComSInt32 indexLevels_;

  NABoolean isPartnEOFSpecified_;
  ComSInt64 partnEOF_;

  // Aligned or Packed table data format
  NABoolean isAlignedTableFormatSpecified_;

  // external table to be mapped to hbase table
  NABoolean mapToHbaseTable_;

  // data format of external mapped table.
  // TRUE:  data is in string/varchar format
  // FALSE: data is in native type format (for ex: 2 byte short for integer...)
  // This field is only valid if mapToHbaseTable_ is set.
  NABoolean hbaseDataFormat_;

  // Each element of the following arrays is a pointer
  // pointing to a StmtDDLAddConstraint parse node in
  // the parse sub-tree containing the table body
  // definition.  Class StmtDDLAddConstraint represents
  // Alter Table <table-name> Add Constraint statements.
  //
  // Note that the Create Table statement does not accept
  // the Alter Table ... syntax.  In order to process
  // the Create Table parse node easier, we create a kludge
  // parse node derived from class StmtDDLAddConstraint for
  // each column or table constraint definition in the
  // Create Table statement.
  //
  // Note that the array addConstraintUniqueArray_ does
  // not include Primary Key constraint.
  //
  StmtDDLAddConstraintCheckArray  addConstraintCheckArray_;
  StmtDDLAddConstraintRIArray     addConstraintRIArray_;
  StmtDDLAddConstraintUniqueArray addConstraintUniqueArray_;

  //
  // The following array is a kludge.  In order to process
  // the Create Table parse node easier, we create a kludge
  // parse node derived from class StmtDDLAddConstraint for
  // each column or table constraint definition in the
  // Create Table statement.  Since the kludge parge nodes
  // are not part of the Create Table parse tree, we need
  // to save the pointers to them in the following array
  // so we can delete them when we invoke the destructor
  // to destroy the Create Table parse node.  For more
  // information, please read the content of the header
  // file StmtDDLAlterTable.h.
  //
  StmtDDLAddConstraintArray addConstraintArray_;

  //
  // pointer to primary partition node
  //
  //   The parse node for the primary partition is created
  //   during the construction of this object.  The former
  //   is not part of the parse tree.
  //

  ElemDDLPartition * pPrimaryPartition_;

  //
  // pointers to child parse nodes
  //

  enum { INDEX_TABLE_DEFINITION = 0,
         INDEX_ATTRIBUTE_LIST,
         MAX_STMT_DDL_CREATE_TABLE_ARITY };

  ElemDDLNode * children_[MAX_STMT_DDL_CREATE_TABLE_ARITY];

  StringPos startOfCreateTableQuery_;
  StringPos startOfCreateTableAsAttrList_;
  StringPos endOfCreateTableAsAttrList_;
  Int32       createTableAsIsoMapping_;
  Int32       createTableAsScannedInputCharset_;

  // CTAs query columns were renamed by the Create Table DDL.
  // Ex: create table(a) as select b from t;
  NABoolean ctaColumnsAreRenamed_;

  // optional list of columns that are to be inserted into during the
  // insert...select phase of the CTAS stmt.
  ElemDDLNode * pInsertColumnsList_;

  // AS query which will be used to populate the created table.
  RelExpr     * pQueryExpression_;

  // if the table being 'create table as'ed already exists, and this
  // flag is set, then do the insert...select part.
  NABoolean loadIfExists_;

  // if deleteData option is specified and the
  // table exists during CTAS and loadIfExists_ has been specified,
  // then delete data from the table before doing the insert.
  // This flag is valid only if loadIfExists_ is set.
  NABoolean deleteData_;

  // Do not do the insert...select part of the table being created
  // using CTAS. This will only create the table.
  NABoolean noLoad_;

  NABoolean isTableFeatureSpecified_;
  NABoolean isDroppable_;
  NABoolean isInsertOnly_;

  ElemDDLSGOptions * pSGOptions_;

  // create only if table doesnt exist. Otherwise just return.
  NABoolean createIfNotExists_;

  // optional options specified during CTAS into a hive table
  NAString hiveOptions_;

}; // class StmtDDLCreateTable

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLCreateTable
// -----------------------------------------------------------------------

//
// accessors
//

inline QualifiedName &
StmtDDLCreateTable::getOrigTableNameAsQualifiedName()
{
  return origTableQualName_;
}

inline const QualifiedName &
StmtDDLCreateTable::getOrigTableNameAsQualifiedName() const
{
  return origTableQualName_;
}

inline QualifiedName &
StmtDDLCreateTable::getTableNameAsQualifiedName()
{
  return tableQualName_;
}

inline const QualifiedName &
StmtDDLCreateTable::getTableNameAsQualifiedName() const
{
  return tableQualName_;
}

inline StmtDDLAddConstraintCheckArray &
StmtDDLCreateTable::getAddConstraintCheckArray()
{
  return addConstraintCheckArray_;
}

inline const StmtDDLAddConstraintCheckArray &
StmtDDLCreateTable::getAddConstraintCheckArray() const
{
  return addConstraintCheckArray_;
}

inline StmtDDLAddConstraintPK *
StmtDDLCreateTable::getAddConstraintPK() const
{
  return pAddConstraintPK_;
}

inline StmtDDLAddConstraintRIArray &
StmtDDLCreateTable::getAddConstraintRIArray()
{
  return addConstraintRIArray_;
}

inline const StmtDDLAddConstraintRIArray &
StmtDDLCreateTable::getAddConstraintRIArray() const
{
  return addConstraintRIArray_;
}

inline StmtDDLAddConstraintUniqueArray &
StmtDDLCreateTable::getAddConstraintUniqueArray()
{
  return addConstraintUniqueArray_;
}

inline const StmtDDLAddConstraintUniqueArray &
StmtDDLCreateTable::getAddConstraintUniqueArray() const
{
  return addConstraintUniqueArray_;
}

//
// returns an array of pointers pointing
// to Column Definition parse node.  If
// LIKE clause is specifed, the returned
// array is empty.  (Note that the list
// of column definition and Like clause
// can not both appear within a Create
// Table statement.)
//
inline const ElemDDLColDefArray &
StmtDDLCreateTable::getColDefArray() const
{
  return columnDefArray_;
}
inline ElemDDLColDefArray &
StmtDDLCreateTable::getColDefArray()
{
  return columnDefArray_;
}

inline const ParDDLFileAttrsCreateTable &
StmtDDLCreateTable::getFileAttributes() const
{
  return fileAttributes_;
}

inline ParDDLFileAttrsCreateTable &
StmtDDLCreateTable::getFileAttributes()
{
  return fileAttributes_;
}

inline const NAString &
StmtDDLCreateTable::getGuardianLocation() const
{
  return guardianLocation_;
}

inline NABoolean
StmtDDLCreateTable::getIsConstraintPKSpecified() const
{
  return isPrimaryKeyClauseSpec_;
}

// is the like clause specified?
inline NABoolean
StmtDDLCreateTable::getIsLikeOptionSpecified() const
{
  return isLikeClauseSpec_;
}

// is location clause/phrase specified?
inline NABoolean
StmtDDLCreateTable::getIsLocationSpecified() const
{
  return isLocationClauseSpec_;
}

// is the STORE BY clause specified?
// same as getIsStoreBySpecified()
inline NABoolean
StmtDDLCreateTable::getIsStoreBySpecified() const
{
  return isStoreByClauseSpec_;
}

// is the STORE BY clause a UNIQUE keylist?
// same as getIsUniqueStoreByKeylistSpecified()
inline NABoolean
StmtDDLCreateTable::getIsUniqueStoreByKeylistSpecified() const
{
  return isUniqueStoreByKeylist_;
}

// is the STORE BY clause a UNIQUE primary key?
// same as getIsUniqueStoreByPrimaryKeySpecified()
inline NABoolean
StmtDDLCreateTable::getIsUniqueStoreByPrimaryKeySpecified() const
{
  return isUniqueStoreByPrimaryKey_;
}


// is HASH partition specified?
inline NABoolean StmtDDLCreateTable::getIsHashV1PartitionSpecified() const
{
  return isHashV1PartitionSpec_;
}

// is HASH2 partition specified?
inline NABoolean StmtDDLCreateTable::getIsHashV2PartitionSpecified() const
{
  return isHashV2PartitionSpec_;
}

inline NABoolean StmtDDLCreateTable::getIsRoundRobinPartitioningSpecified() const
{
 return isRoundRobinPartitioningSpecified_;
}        

// get key column list in STORE BY clause
inline const ElemDDLColRefArray &
StmtDDLCreateTable::getKeyColumnArray() const
{
  return keyColRefArray_;
}

// get key column list in STORE BY clause
inline ElemDDLColRefArray &
StmtDDLCreateTable::getKeyColumnArray()
{
  return keyColRefArray_;
}

// if LIKE clause is not specified, an
// empty string is returned
inline const CorrName &
StmtDDLCreateTable::getLikeSourceTableCorrName() const
{
  return likeSourceTableCorrName_;
}

inline const NAString
StmtDDLCreateTable::getLikeSourceTableName() const
{
  return likeSourceTableCorrName_.getQualifiedNameObj().getQualifiedNameAsAnsiString();
}

inline void StmtDDLCreateTable::setLikeSourceTableName(CorrName &srcName)
{
  likeSourceTableCorrName_ = srcName;
}

inline QualifiedName &
StmtDDLCreateTable::getOrigLikeSourceTableName()
{
  return origLikeSourceTableQualName_;
}

inline const QualifiedName &
StmtDDLCreateTable::getOrigLikeSourceTableName() const
{
  return origLikeSourceTableQualName_;
}

//++ MV
inline const ExtendedQualName::SpecialTableType 
StmtDDLCreateTable::getLikeSourceTableType() const
{
  return likeSourceTableCorrName_.getSpecialType();
}
//-- MV

// if LIKE clause is not specified, the
// returned information is not relevant
inline const ParDDLLikeOptsCreateTable &
StmtDDLCreateTable::getLikeOptions() const
{
  return likeOptions_;
}

inline ParDDLLikeOptsCreateTable &
StmtDDLCreateTable::getLikeOptions()
{
  return likeOptions_;
}

inline const NAString &
StmtDDLCreateTable::getLocation() const
{
  return locationName_;
}

inline NAString
StmtDDLCreateTable::getLocationName() const
{
  return locationName_;
}

inline NAString
StmtDDLCreateTable::getPartitionName() const
{
  return partitionName_;
}

inline ElemDDLLocation::locationNameTypeEnum
StmtDDLCreateTable::getLocationNameType() const
{
  return locationNameType_;
}

// returns an array of pointers pointing
// to Partition parse nodes (each Partition
// parse node contains all legal attributes
// associating with a partition)
inline const ElemDDLPartitionArray &
StmtDDLCreateTable::getPartitionArray() const
{
  return partitionArray_;
}
inline ElemDDLPartitionArray &
StmtDDLCreateTable::getPartitionArray()
{
  return partitionArray_;
}

// get column name list in primary key clause
inline const ElemDDLColRefArray &
StmtDDLCreateTable::getPrimaryKeyColRefArray() const
{
  return primaryKeyColRefArray_;
}

// get column name list in primary key clause
inline ElemDDLColRefArray &
StmtDDLCreateTable::getPrimaryKeyColRefArray()
{
  return primaryKeyColRefArray_;
}

// get column name list in partition by clause
inline const ElemDDLColRefArray &
StmtDDLCreateTable::getPartitionKeyColRefArray() const
{
  return partitionKeyColRefArray_;
}

// get column name list in partition by clause
inline ElemDDLColRefArray &
StmtDDLCreateTable::getPartitionKeyColRefArray()
{
  return partitionKeyColRefArray_;
}

// is HBASE_OPTIONS specified?
inline NABoolean StmtDDLCreateTable::isHbaseOptionsSpecified() const
{
  return isHbaseOptionsSpec_;
}

// is the DIVISION BY clause specified?
// same as getIsDivisionClauseSpecified()
inline NABoolean StmtDDLCreateTable::isDivisionClauseSpecified() const
{
  return isDivisionByClauseSpec_;
}

inline NABoolean StmtDDLCreateTable::getIsDivisionClauseSpecified() const
{
  return isDivisionByClauseSpec_;
}

inline ElemDDLDivisionClause::divisionTypeEnum StmtDDLCreateTable::getDivisionType() const
{
  return ( pDivisionByClauseParseNode_ EQU NULL
           ? ElemDDLDivisionClause::UNKNOWN_DIVISION_TYPE
           : pDivisionByClauseParseNode_->getDivisionType() );
}

inline ElemDDLDivisionClause * StmtDDLCreateTable::getDivisionClause()
{
  return pDivisionByClauseParseNode_;
}

inline ItemExprList * StmtDDLCreateTable::getDivisionExprList()
{
  return ( pDivisionByClauseParseNode_ EQU NULL
           ? NULL
           : pDivisionByClauseParseNode_->getDivisionExprList() );
}

inline ElemDDLColRefArray * StmtDDLCreateTable::getDivisionColRefArray()
{
  return ( pDivisionByClauseParseNode_ EQU NULL
           ? NULL
           : &pDivisionByClauseParseNode_->getDivisionColRefArray() );
}

inline ElemDDLHbaseOptions * StmtDDLCreateTable::getHbaseOptionsClause()
{
  return pHbaseOptionsParseNode_;
}

inline ElemDDLSaltOptionsClause * StmtDDLCreateTable::getSaltOptions()
{
  return pSaltOptions_;
}

inline ElemDDLColRefArray * StmtDDLCreateTable::getSaltColRefArray()
{
  return ( pSaltOptions_ EQU NULL
           ? NULL
           : &pSaltOptions_->getSaltColRefArray() );
}

inline ComStoreOption
StmtDDLCreateTable::getStoreOption() const
{
  return storeOption_;
}

// get table name
inline const NAString
StmtDDLCreateTable::getTableName() const
{
  return tableQualName_.getQualifiedNameAsAnsiString();
}

// is the like clause specified?
// same as getIsLikeOptionSpecified()
inline NABoolean
StmtDDLCreateTable::isLikeOptionSpecified() const
{
  return isLikeClauseSpec_;
}

// is location clause/phrase specified?
// same as getIsLocationSpecified()
inline NABoolean
StmtDDLCreateTable::isLocationSpecified() const
{
  return isLocationClauseSpec_;
}

// is PARTITION clause specified?
inline NABoolean
StmtDDLCreateTable::isPartitionSpecified() const
{
  return isPartitionClauseSpec_;
}

// is PARTITION BY clause specified?
inline NABoolean
StmtDDLCreateTable::isPartitionBySpecified() const
{
  return isPartitionByClauseSpec_;
}

// is the STORE BY clause specified?
// same as getIsStoreBySpecified()
inline NABoolean
StmtDDLCreateTable::isStoreBySpecified() const
{
  return isStoreByClauseSpec_;
}

// is the STORE BY clause's key list qualified by UNIQUE?
inline NABoolean
StmtDDLCreateTable::isUniqueStoreByKeylistSpecified() const
{
  return isUniqueStoreByKeylist_;
}

// is the STORE BY clause's primary key qualified by UNIQUE?
inline NABoolean
StmtDDLCreateTable::isUniqueStoreByPrimaryKeySpecified() const
{
  return isUniqueStoreByPrimaryKey_;
}

// is the attributes clause specified?
inline NABoolean
StmtDDLCreateTable::isAttributeSpecified() const
{
  return isAttributeClauseSpec_;
}

inline void StmtDDLCreateTable::setIsRoundRobinPartitioningSpecified()
{
  isRoundRobinPartitioningSpecified_ = TRUE;  
}        

// ++MV
inline NABoolean StmtDDLCreateTable::isSpecialTypeSpecified() const
{
  return isSpecialTypeSpecified_;
}

inline void StmtDDLCreateTable::setTableType(ExtendedQualName::SpecialTableType tableType)
{
  tableType_ = tableType;
  isSpecialTypeSpecified_ = TRUE;
}

inline const ElemDDLNode * 
StmtDDLCreateTable::insertColumnsList() const
{
  return pInsertColumnsList_;
}

inline ElemDDLNode * 
StmtDDLCreateTable::insertColumnsList()
{
  return pInsertColumnsList_;
}

inline const RelExpr *
StmtDDLCreateTable::getQueryExpression() const
{
  return pQueryExpression_;
}

inline RelExpr *
StmtDDLCreateTable::getQueryExpression()
{
  return pQueryExpression_;
}

inline 
const StringPos StmtDDLCreateTable::getStartOfCreateTableQueryPosition() const
{
  return startOfCreateTableQuery_;
}

inline void 
StmtDDLCreateTable::setStartOfCreateTableQueryPosition(const StringPos startPos)
{
  startOfCreateTableQuery_ = startPos;
}

inline 
const StringPos StmtDDLCreateTable::getStartOfCreateTableAsAttrListPosition() const
{
  return startOfCreateTableAsAttrList_;
}

inline void 
StmtDDLCreateTable::setStartOfCreateTableAsAttrListPosition(const StringPos startPos)
{
  startOfCreateTableAsAttrList_ = startPos;
}

inline 
const StringPos StmtDDLCreateTable::getEndOfCreateTableAsAttrListPosition() const
{
  return endOfCreateTableAsAttrList_;
}

inline void 
StmtDDLCreateTable::setEndOfCreateTableAsAttrListPosition(const StringPos endPos)
{
  endOfCreateTableAsAttrList_ = endPos;
}

inline 
const Int32 StmtDDLCreateTable::getCreateTableAsIsoMapping() const
{
  return createTableAsIsoMapping_;
}

inline void 
StmtDDLCreateTable::setCreateTableAsIsoMapping(const Int32 charset)
{
  createTableAsIsoMapping_ = charset;
}

inline 
const Int32 StmtDDLCreateTable::getCreateTableAsScannedInputCharset() const
{
  return createTableAsScannedInputCharset_;
}

inline void 
StmtDDLCreateTable::setCreateTableAsScannedInputCharset(const Int32 charset)
{
  createTableAsScannedInputCharset_ = charset;
}

inline ExtendedQualName::SpecialTableType StmtDDLCreateTable::getTableType() const
{
  return tableType_;
}

//-- MV

// POS
NABoolean 
StmtDDLCreateTable::isPOSNumPartnsSpecified() const
{
  return isPOSNumPartnsSpecified_;
}

NABoolean 
StmtDDLCreateTable::isPOSInitialTableSizeSpecified() const
{
  return isPOSInitialTableSizeSpecified_;
}

NABoolean 
StmtDDLCreateTable::isPOSMaxTableSizeSpecified() const
{
  return isPOSMaxTableSizeSpecified_;
}

NABoolean
StmtDDLCreateTable::isPOSIgnoreSpecified() const
{
  return isPOSIgnoreSpecified_;
}

NABoolean
StmtDDLCreateTable::isPOSDiskPoolSpecified() const
{
  return isPOSDiskPoolSpecified_;
}

inline ComSInt32 StmtDDLCreateTable::getPOSDiskPool() const
{
  return posDiskPool_;
}

inline void StmtDDLCreateTable::setPOSDiskPool(ComSInt32 diskPool)
{
  posDiskPool_ = diskPool;
}

inline ComSInt32 StmtDDLCreateTable::getPOSNumDiskPools() const
{
  return posNumDiskPools_;
}

inline void StmtDDLCreateTable::setPOSNumDiskPools(ComSInt32 numOfDiskPools)
{
  posNumDiskPools_ = numOfDiskPools;
}
NABoolean 
StmtDDLCreateTable::isNumRowsSpecified() const
{
  return isNumRowsSpecified_;
}

NABoolean 
StmtDDLCreateTable::isIndexLevelsSpecified() const
{
  return isIndexLevelsSpecified_;
}

NABoolean 
StmtDDLCreateTable::isPartnEOFSpecified() const
{
  return isPartnEOFSpecified_;
}

inline ComSInt32 StmtDDLCreateTable::getPOSNumPartns() const
{
  return posNumPartns_;
}

inline ComSInt32 StmtDDLCreateTable::getPOSInitialTableSize() const
{
  return posInitialTableSize_;
}

inline ComSInt32 StmtDDLCreateTable::getPOSMaxTableSize() const
{
  return posMaxTableSize_;
}

inline NABoolean StmtDDLCreateTable::getPOSIgnore() const
{
  return posIgnore_;
}

inline double StmtDDLCreateTable::getNumRows() const
{
  return numRows_;
}

inline ComSInt32 StmtDDLCreateTable::getIndexLevels() const
{
  return indexLevels_;
}

inline ComSInt64 StmtDDLCreateTable::getPartnEOF() const
{
  return partnEOF_;
}

inline void StmtDDLCreateTable::setPOSNumPartns(ComSInt32 partns)
{
  posNumPartns_ = partns;
}

inline void StmtDDLCreateTable::setIsPOSNumPartnsSpecified(NABoolean flag)
{
  isPOSNumPartnsSpecified_ = flag;
}

inline ComInsertMode StmtDDLCreateTable::getInsertMode() const
{
  return eInsertMode_;
}

inline void StmtDDLCreateTable::setInsertMode(ComInsertMode insertMode)
{
  eInsertMode_ = insertMode;
}

inline void StmtDDLCreateTable::setCTAcolumnsAreRenamed(NABoolean r)
{
  ctaColumnsAreRenamed_ = r;
}

inline NABoolean StmtDDLCreateTable::ctaColumnsAreRenamed()
{
  return ctaColumnsAreRenamed_;
}

inline void StmtDDLCreateTable::setLoadIfExists(NABoolean l)
{
  loadIfExists_ = l;
}

inline NABoolean StmtDDLCreateTable::loadIfExists()
{
  return loadIfExists_;
}

inline void StmtDDLCreateTable::setNoLoad(NABoolean l)
{
  noLoad_ = l;
}

inline void StmtDDLCreateTable::setDeleteData(NABoolean d)
{
 deleteData_ = d;
}

inline NABoolean StmtDDLCreateTable::deleteData()
{
  return deleteData_;
}

inline NABoolean StmtDDLCreateTable::noLoad()
{
  return noLoad_;
}

// is the attributes clause specified?
inline NABoolean
StmtDDLCreateTable::isTableFeatureSpecified() const
{
  return isTableFeatureSpecified_;
}

inline void StmtDDLCreateTable::setTableFeature(ElemDDLTableFeature * pFeature)
{
  if (isTableFeatureSpecified_)
    *SqlParser_Diags << DgSqlCode(-3167);
  isTableFeatureSpecified_ = TRUE;

  switch (pFeature->getTableFeature())
  {
    case COM_DROPPABLE:
      isDroppable_ = TRUE;
      isInsertOnly_ = FALSE;
      break;
    case COM_DROPPABLE_INSERT_ONLY:
      isDroppable_ = TRUE;
      isInsertOnly_ = TRUE;
      break;
    case COM_NOT_DROPPABLE:
      isDroppable_ = FALSE;
      isInsertOnly_ = FALSE;
      break;
    case COM_NOT_DROPPABLE_INSERT_ONLY:
      isDroppable_ = FALSE;
      isInsertOnly_ = TRUE;
      break;
    default:
      NAAbort("StmtDDLCreateTable", __LINE__, "internal logic error");
      break;
   }
}

inline NABoolean StmtDDLCreateTable::isDroppable()
{
  return isDroppable_;
}
inline NABoolean StmtDDLCreateTable::isInsertOnly()
{
  return isInsertOnly_;
}

inline const ElemDDLSGOptions * StmtDDLCreateTable::getSGOptions() const 
{
  return pSGOptions_;
}

inline ElemDDLSGOptions * StmtDDLCreateTable::getSGOptions() 
{
  return pSGOptions_;
}

inline void StmtDDLCreateTable::setSGOptions(ElemDDLSGOptions *pSGOptions) 
{
  pSGOptions_ = pSGOptions;
}

// ------------------------------------------------------------------------------------------
// Create Hbase Table statement. Create a native hbase table.
// ------------------------------------------------------------------------------------------
class StmtDDLCreateHbaseTable : public StmtDDLNode
{
public:

  // (default) constructor
  StmtDDLCreateHbaseTable(const QualifiedName & TableQualName,
			  ConstStringList * csl,
			  ElemDDLHbaseOptions *	hbaseOptions = NULL,		  
			  CollHeap    * heap = PARSERHEAP());

  // virtual destructor
  virtual ~StmtDDLCreateHbaseTable();

  // cast
  virtual StmtDDLCreateHbaseTable * castToStmtDDLCreateHbaseTable();

  // ---------------------------------------------------------------------
  // accessors
  // ---------------------------------------------------------------------

  // methods relating to parse tree
  virtual Int32 getArity() const { return 0; };
  virtual ExprNode * getChild(Lng32 index) { return NULL; }

  inline const QualifiedName & getTableNameAsQualifiedName() const;
  inline       QualifiedName & getTableNameAsQualifiedName();

  inline const QualifiedName & getOrigTableNameAsQualifiedName() const;
  inline       QualifiedName & getOrigTableNameAsQualifiedName();

  inline const NAString getTableName() const;

  inline ElemDDLHbaseOptions * getHbaseOptionsClause();

  ConstStringList* csl() { return csl_; }

  const ConstStringList* csl() const { return csl_; }

  // ---------------------------------------------------------------------
  // other public methods
  // ---------------------------------------------------------------------

  // method for binding
  ExprNode * bindNode(BindWA *bindWAPtr);

  // method for collecting information
  void synthesize();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;

 private:
  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------

  // the tablename specified by user in the create stmt.
  // This name is not fully qualified during bind phase.
  QualifiedName origTableQualName_;

  // The syntax of table name is
  // [ [ catalog-name . ] schema-name . ] table-name
  QualifiedName tableQualName_;

  ConstStringList * csl_;

  ElemDDLHbaseOptions *pHbaseOptionsParseNode_;

}; // class StmtDDLCreateHbaseTable

// accessors
//

inline QualifiedName &
StmtDDLCreateHbaseTable::getOrigTableNameAsQualifiedName()
{
  return origTableQualName_;
}

inline const QualifiedName &
StmtDDLCreateHbaseTable::getOrigTableNameAsQualifiedName() const
{
  return origTableQualName_;
}

inline QualifiedName &
StmtDDLCreateHbaseTable::getTableNameAsQualifiedName()
{
  return tableQualName_;
}

inline const QualifiedName &
StmtDDLCreateHbaseTable::getTableNameAsQualifiedName() const
{
  return tableQualName_;
}

// get table name
inline const NAString
StmtDDLCreateHbaseTable::getTableName() const
{
  return tableQualName_.getQualifiedNameAsAnsiString();
}

inline ElemDDLHbaseOptions * StmtDDLCreateHbaseTable::getHbaseOptionsClause()
{
  return pHbaseOptionsParseNode_;
}

#endif // STMTDDLCREATETABLE_H
