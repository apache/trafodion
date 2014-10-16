/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1995-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
#ifndef STMTDDLCREATEINDEX_H
#define STMTDDLCREATEINDEX_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLCreateIndex.h
 * Description:  class representing Create Index Statement parser nodes
 *
 *
 * Created:      9/21/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ElemDDLNode.h"
#include "ElemDDLColRefArray.h"
#include "ElemDDLDivisionClause.h"
#include "ElemDDLSaltOptions.h"
#include "ElemDDLLocation.h"
#include "ElemDDLPartitionArray.h"
#include "NAString.h"
#include "ParDDLFileAttrsCreateIndex.h"
#include "StmtDDLNode.h"
#include "ComSmallDefs.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLCreateIndex;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Create Index statement
// -----------------------------------------------------------------------
class StmtDDLCreateIndex : public StmtDDLNode
{

public:

  // initialize constructor
  StmtDDLCreateIndex(NABoolean isUnique,
                     const NAString & anIndexName,
                     const QualifiedName & aTableName,
                     ElemDDLNode * pColumnList,
                     NABoolean posIgnore,
                     ElemDDLNode * pAttributeList,
                     CollHeap    * heap = PARSERHEAP());

  // virtual destructor
  virtual ~StmtDDLCreateIndex();

  // cast
  virtual StmtDDLCreateIndex * castToStmtDDLCreateIndex();

  //
  // accessors
  //

  // methods relating to parse tree
  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);

  inline const ElemDDLColRefArray & getColRefArray() const;
  inline ElemDDLColRefArray & getColRefArray();

        // returns a NAList of ElemDDLColRef parse nodes.

  inline unsigned short getDSlackPercentage() const;

  inline const ParDDLFileAttrsCreateIndex & getFileAttributes() const;
  inline       ParDDLFileAttrsCreateIndex & getFileAttributes();

        // returns the object containing the file attributes
        // associating with the index being created.  Please
        // note that some file attributes in the returned
        // object only apply to the primary partition of the
        // index.

  inline const NAString & getGuardianLocation() const;

        // returns an empty string unless the parse node
        // is bound (the method bindNode is invoked).
        // After the parse node is bound, returns the
        // location of the primary partition in Guardian
        // physical device name format.  If the LOCATION
        // clause is not specified, a default location is
        // used.

  inline unsigned short getISlackPercentage() const;

  inline const QualifiedName & getOrigTableNameAsQualifiedName() const;
  inline       QualifiedName & getOrigTableNameAsQualifiedName();

  inline const NAString & getIndexName() const;
  inline const QualifiedName & getIndexNameAsQualifiedName() const;
  inline       QualifiedName & getIndexNameAsQualifiedName() ;

  inline const QualifiedName & getTableNameAsQualifiedName() const;
  inline       QualifiedName & getTableNameAsQualifiedName() ;

  inline const NAString & getLocation() const;

        // returns location name if specified; otherwise,
        // an empty string is returned.

  inline NAString getLocationName() const;

        // returns location name if specified; otherwise,
        // an empty string is returned.

  inline NAString getPartitionName() const;
        // returns the partition name if specified; otherwise
        // an empty string is returned.

  inline NABoolean getPOSIgnore() const;

  ElemDDLLocation::locationNameTypeEnum getLocationNameType() const;

        // returns the type of the specified location name;
        // e.g., a Guardian device name.  If location clause
        // is not specified, the returned value has no meaning.

  inline const NAString & getParallelExecConfigFileName() const;

        // returns the name of the config. file specified
        // in the Parallel Execution clause; returns an
        // empty string if the clause does not appear.

  inline const ElemDDLPartitionArray & getPartitionArray() const;
  inline ElemDDLPartitionArray & getPartitionArray();

        // returns a NAList of ElemDDLPartition parse nodes.
        // If Partition clause not specified, the list is empty.

  inline const ElemDDLColRefArray & getPartitionKeyColRefArray() const;
  inline       ElemDDLColRefArray & getPartitionKeyColRefArray();

        // returns column name list in partition by clause if
        // specified; otherwise, an empty array is returned.

  const NAString getTableName() const;

        // returns table name, in external format.

  inline NABoolean isAttributeSpecified() const;

        // returns TRUE if the index file attribute clause appears;
        // returns FALSE otherwise.

  inline NABoolean isDSlackSpecified() const;

        // returns TRUE if DSlack clause appears;
        // returns FALSE otherwise.

  inline NABoolean isLocationSpecified() const;

        // returns TRUE if location clause is specified;
        // returns FALSE otherwise.

  inline NABoolean isISlackSpecified() const;

        // returns TRUE if ISlack clause appears;
        // returns FALSE otherwise.

  inline NABoolean isPopulateOptionSpecified() const;
        
        // return TRUE if IsPopulate clause appear;
        // return FALSE otherwise.

inline NABoolean isNoPopulateOptionSpecified() const;
        
        // return TRUE if IsNoPopulate clause appear;
        // return FALSE otherwise.
    
  inline NABoolean isParallelExecutionClauseSpecified() const;

        // returns TRUE if the parallel execution clause appears;
        // returns FALSE otherwise.

  inline NABoolean isParallelExecutionEnabled() const;

        // returns TRUE if the parallel execution clause appears
        // with ON option; returns FALSE otherwise.

  inline ComPartitioningScheme getPartitioningScheme() const;
        //
        // Returns the partitioning scheme (could be HASH or RANGE)
        //

  inline NABoolean isPartitionSpecified() const;

        // returns TRUE if the partition clause appears;
        // returns FALSE otherwise.

  inline NABoolean isPartitionBySpecified() const;

        // returns TRUE if the PARTITION BY clause appears;
        // returns FALSE otherwise.

  inline NABoolean isUniqueSpecified() const;

        // returns TRUE if the Unique keyword is specified;
        // returns FALSE otherwise.

  inline NABoolean isNumRowsSpecified() const;
  inline double getNumRows() const;

  inline NABoolean isDivisionClauseSpecified() const;

        // returns TRUE if the DIVISION {BY|AS} clause appears;
        // returns FALSE otherwise.

  inline NABoolean isHbaseOptionsSpecified() const;

        // returns TRUE if HBASE_OPTIONS clause appears;
        // returns FALSE otherwise.


  inline ElemDDLDivisionClause::divisionTypeEnum getDivisionType() const;
  inline ItemExprList * getDivisionExprList();

  inline       ElemDDLSaltOptionsClause * getSaltOptions();

  // this will return NULL in all cases as Traf 0.9 since SALT calumns
  // cannot be be explicitly specified in the CREATE INDEX statement
  // Only SALT LIKE TABLE is supported for an index
  inline       ElemDDLColRefArray * getSaltColRefArray();

  inline       ElemDDLHbaseOptions * getHbaseOptionsClause();

  //
  // mutators
  //

  void setChild(Lng32 index, ExprNode * newNode);

  //
  // method for binding
  //

  ExprNode * bindNode(BindWA * pBindWA);

  //
  // method for collecting information
  //
  
  void synthesize();

        // collects information in the parse sub-tree and
        // copy/move them to the current parse node.

  //
  // methods for tracing
  //

  virtual const NAString displayLabel1() const;
  virtual const NAString displayLabel2() const;
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;

  // Ghost Object
  NABoolean isIndexOnGhostTable() { return isIndexOnGhostTable_; }
  NABoolean isIndexOnGhostTable() const { return isIndexOnGhostTable_; }
  void setIsIndexOnGhostTable(NABoolean g) { isIndexOnGhostTable_ = g; }

  virtual NABoolean explainSupported() { return TRUE; }

private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  void setFileAttributes(ElemDDLFileAttrClause * pFileAttrClause);

        // Copies the information in the specified file
        // attribute clause (pointed to by pFileAttrClause)
        // to data member fileAttributes_ in this object.
        // 
        // This method can only be invoked during the
        // construction of this object when the (file)
        // attributes clause appears.

  void setIndexOption(ElemDDLNode * pOption);

        // Copies the information in an Index Option parse node
        // pointed to by pOption to this object.  An index option
        // parse node represents a Location clause, a Partition
        // clause, a File Attribute clause, or a load option.
        // This method should only be invoked from a constructor.

  void setPartitions(ElemDDLPartitionClause * pPartitionClause);

        // Copies the information in the specified partition
        // clause (pointed to by pPartitionClause) to this object.
        //
        // This method is only invoked during the contruction of
        // this object when the partition clause appears.

  void setPrimaryPartition(ElemDDLNode * pFirstSecondaryPartitionNode);

        // Allocates the primary partition node and inserts its
        // pointer at the beginning of the partitionArray_.  The
        // kind of the primary partition node must be the same
        // as that of the specified secondary partition node.
        //
        // This method is only invoked during the construction
        // of this object when the partition clause appears.

  void setSecondaryPartition(ElemDDLPartition * pSecondaryPartitionNode);

        // Copies the information of the specified secondary
        // partition (pointed to by pSecondaryPartitionNode)
        // to this object.
        //
        // This method is only invoked during the construction
        // of this object when the partition clause appears.

  //
  // please do not use the following methods
  //
  
  StmtDDLCreateIndex();                                        // DO NOT USE
  StmtDDLCreateIndex(const StmtDDLCreateIndex &);              // DO NOT USE
  StmtDDLCreateIndex & operator=(const StmtDDLCreateIndex &);  // DO NOT USE

  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------

  NABoolean isUnique_;

        // Set to TRUE if the keyword UNIQUE is specified;
        // set to FALSE otherwise.

  NABoolean posIgnore_;

	// Set to TRUE if the keywords IGNORE POS is specified
	// set to FALSE otherwise.

  // index name can only be a simple name
  NAString indexName_;
  QualifiedName indexQualName_;

  // the tablename specified by user in the create stmt.
  // This name is not fully qualified during bind phase.
  QualifiedName origTableQualName_;

  // The syntax of table name is
  // [ [ catalog-name . ] schema-name . ] table-name

  QualifiedName tableQualName_;

  // list of column names.  A column name may be followed
  // by a ordering keyword; e.g., DESCENDING
  ElemDDLColRefArray columnRefArray_;

  //
  // LOCATION clause
  //
  NABoolean isLocationClauseSpec_;
  NAString locationName_;
  ElemDDLLocation::locationNameTypeEnum locationNameType_;
  NAString partitionName_;

  // guardianLocation_ is empty until the parse node is
  // bound (the method bindNode() is invoked).  The method
  // bindNode() converts the specified location (in the
  // LOCATION clause associating with the primary key) to
  // a Guardian physical device and then saves the computed
  // name in this data member.  If the LOCATION clause does
  // not appear, a default location name is selected.
  NAString guardianLocation_;

  //
  // Optional Partition Type
  //
  ComPartitioningScheme partitioningScheme_ ;

  //
  // PARTITION clause
  //   list of partition definitions
  //
  NABoolean isPartitionClauseSpec_;
  ElemDDLPartitionArray partitionArray_;

  // DIVISION BY clause

  NABoolean isDivisionClauseSpec_;
  ElemDDLDivisionClause *pDivisionClauseParseNode_;

  // SALT clause for HBase tables
  ElemDDLSaltOptionsClause *pSaltOptions_;

  // HBASE OPTIONS clause

  NABoolean isHbaseOptionsSpec_;
  ElemDDLHbaseOptions *pHbaseOptionsParseNode_;

  // PARTITION BY clause
  NABoolean isPartitionByClauseSpec_;
  ElemDDLColRefArray partitionKeyColRefArray_;

  NABoolean isNumRowsSpecified_;
  double numRows_;

  //
  // Index load information
  //

  NABoolean isDSlackClauseSpec_;
  unsigned short dSlackPercentage_;

  NABoolean isISlackClauseSpec_;
  unsigned short iSlackPercentage_;

  NABoolean isParallelExecutionClauseSpec_;
  NABoolean isParallelExec_;  // TRUE (ON); FALSE (OFF)
  NABoolean isPopulated_;     // True if populate option is specified, false if no populate is specified.
  NABoolean isNoPopulated_;   // True if no populate option is specified, false otherwise.
  Int32       populateCount_ ;  // to prevent user to enter the populate clause multiple times.
  Int32       noPopulateCount_; // to prevent user to enter the no populate clause multiple times.
  NAString configFileName_;

  //
  // ATTRIBUTE(S) clause
  //   index file attributes
  //
  NABoolean isAttributeClauseSpec_;
  ParDDLFileAttrsCreateIndex fileAttributes_;

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

  enum { INDEX_COLUMN_REF_LIST = 0,
         INDEX_OPTION_LIST,
         MAX_STMT_DDL_CREATE_INDEX_ARITY };


  ElemDDLNode * children_[MAX_STMT_DDL_CREATE_INDEX_ARITY];

  // TRUE if the index is to be created on a ghost table
  NABoolean isIndexOnGhostTable_;

}; // class StmtDDLCreateIndex

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLCreateIndex
// -----------------------------------------------------------------------
inline QualifiedName &
StmtDDLCreateIndex::getOrigTableNameAsQualifiedName()
{
  return origTableQualName_;
}

inline const QualifiedName &
StmtDDLCreateIndex::getOrigTableNameAsQualifiedName() const
{
  return origTableQualName_;
}

inline QualifiedName &
StmtDDLCreateIndex::getIndexNameAsQualifiedName()
{
  return indexQualName_;
}

inline const QualifiedName & 
StmtDDLCreateIndex::getIndexNameAsQualifiedName() const
{
  return indexQualName_;
}

inline QualifiedName & 
StmtDDLCreateIndex::getTableNameAsQualifiedName()
{
  return tableQualName_;
}

inline const QualifiedName & 
StmtDDLCreateIndex::getTableNameAsQualifiedName() const
{
  return tableQualName_;
}

inline const ElemDDLColRefArray &
StmtDDLCreateIndex::getColRefArray() const
{
  return columnRefArray_;
}

inline ElemDDLColRefArray &
StmtDDLCreateIndex::getColRefArray()
{
  return columnRefArray_;
}

inline unsigned short
StmtDDLCreateIndex::getDSlackPercentage() const
{
  return dSlackPercentage_;
}

inline const ParDDLFileAttrsCreateIndex &
StmtDDLCreateIndex::getFileAttributes() const
{
  return fileAttributes_;
}

inline ParDDLFileAttrsCreateIndex &
StmtDDLCreateIndex::getFileAttributes()
{
  return fileAttributes_;
}

inline const NAString &
StmtDDLCreateIndex::getGuardianLocation() const
{
  return guardianLocation_;
}

inline unsigned short
StmtDDLCreateIndex::getISlackPercentage() const
{
  return iSlackPercentage_;
}

// get index name
inline const NAString &
StmtDDLCreateIndex::getIndexName() const
{
  return indexName_;
}

inline const NAString &
StmtDDLCreateIndex::getLocation() const
{
  return locationName_;
}

inline NAString
StmtDDLCreateIndex::getLocationName() const
{
  return locationName_;
}

inline NAString
StmtDDLCreateIndex::getPartitionName() const
{
  return partitionName_;
}

inline NABoolean
StmtDDLCreateIndex::getPOSIgnore() const
{
  return posIgnore_;
}

inline ElemDDLLocation::locationNameTypeEnum
StmtDDLCreateIndex::getLocationNameType() const
{
  return locationNameType_;
}

// get config. file name specified in Parallel Execution clause
inline const NAString &
StmtDDLCreateIndex::getParallelExecConfigFileName() const
{
  return configFileName_;
}

inline NABoolean StmtDDLCreateIndex::isDivisionClauseSpecified() const
{
  return isDivisionClauseSpec_;
}

// is HBASE_OPTIONS specified?
inline NABoolean StmtDDLCreateIndex::isHbaseOptionsSpecified() const
{
  return isHbaseOptionsSpec_;
}


inline ElemDDLDivisionClause::divisionTypeEnum StmtDDLCreateIndex::getDivisionType() const
{
  return ( pDivisionClauseParseNode_ EQU NULL
           ? ElemDDLDivisionClause::UNKNOWN_DIVISION_TYPE
           : pDivisionClauseParseNode_->getDivisionType() );
}

inline ItemExprList * StmtDDLCreateIndex::getDivisionExprList()
{
  return ( pDivisionClauseParseNode_ EQU NULL
           ? NULL
           : pDivisionClauseParseNode_->getDivisionExprList() );
}

inline ElemDDLHbaseOptions * StmtDDLCreateIndex::getHbaseOptionsClause()
{
  return pHbaseOptionsParseNode_;
}

inline ElemDDLSaltOptionsClause * StmtDDLCreateIndex::getSaltOptions()
{
  return pSaltOptions_;
}

inline ElemDDLColRefArray * StmtDDLCreateIndex::getSaltColRefArray()
{
  return ( pSaltOptions_ EQU NULL
           ? NULL
           : &pSaltOptions_->getSaltColRefArray() );
}

//
// Returns partitioning scheme set by default or by "<partition-type> PARTITION" syntax
//

inline ComPartitioningScheme StmtDDLCreateIndex::getPartitioningScheme() const
{ 
  return partitioningScheme_; 
} 

// returns an array of pointers pointing
// to Partition parse nodes (each Partition
// parse node contains all legal attributes
// associating with a partition)
inline const ElemDDLPartitionArray &
StmtDDLCreateIndex::getPartitionArray() const
{
  return partitionArray_;
}
inline ElemDDLPartitionArray &
StmtDDLCreateIndex::getPartitionArray()
{
  return partitionArray_;
}

// get column name list in partition by clause
inline const ElemDDLColRefArray &
StmtDDLCreateIndex::getPartitionKeyColRefArray() const
{
  return partitionKeyColRefArray_;
}

// get column name list in partition by clause
inline ElemDDLColRefArray &
StmtDDLCreateIndex::getPartitionKeyColRefArray()
{
  return partitionKeyColRefArray_;
}

// is the attributes clause specified?
inline NABoolean
StmtDDLCreateIndex::isAttributeSpecified() const
{
  return isAttributeClauseSpec_;
}

// is the DSlack clause specified?
inline NABoolean
StmtDDLCreateIndex::isDSlackSpecified() const
{
  return isDSlackClauseSpec_;
}

// is the ISlack clause specified?
inline NABoolean
StmtDDLCreateIndex::isISlackSpecified() const
{
  return isISlackClauseSpec_;
}

// is location clause specified?
inline NABoolean
StmtDDLCreateIndex::isLocationSpecified() const
{
  return isLocationClauseSpec_;
}

// is the parallel execution clause specified?
inline NABoolean
StmtDDLCreateIndex::isParallelExecutionClauseSpecified() const
{
  return isParallelExecutionClauseSpec_;
}

inline NABoolean
StmtDDLCreateIndex::isParallelExecutionEnabled() const
{
  return isParallelExec_;
}

// is the partition clause specified?
inline NABoolean
StmtDDLCreateIndex::isPartitionSpecified() const
{
  return isPartitionClauseSpec_;
}

// is the no populate clause specified ?
inline NABoolean
StmtDDLCreateIndex::isNoPopulateOptionSpecified() const
{
  return isNoPopulated_;
}

// is the populate clause specified ?
inline NABoolean
StmtDDLCreateIndex::isPopulateOptionSpecified() const
{
  return isPopulated_ ;
}

// is PARTITION BY clause specified?
inline NABoolean
StmtDDLCreateIndex::isPartitionBySpecified() const
{
  return isPartitionByClauseSpec_;
}

// is the unique keyword specified?
inline NABoolean
StmtDDLCreateIndex::isUniqueSpecified() const
{
  return isUnique_;
}

NABoolean 
StmtDDLCreateIndex::isNumRowsSpecified() const
{
  return isNumRowsSpecified_;
}

inline double StmtDDLCreateIndex::getNumRows() const
{
  return numRows_;
}

#endif // STMTDDLCREATEINDEX_H
