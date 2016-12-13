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
#ifndef NACOLUMN_H
#define NACOLUMN_H
/* -*-C++-*-
******************************************************************************
*
* File:         A column
* Description:  Column class declarations
* Created:      4/27/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "ComSmallDefs.h"
#include "BaseTypes.h"
#include "ObjectNames.h"
#include "ExpLOBenums.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class NAColumn;
class NAColumnArray;

// -----------------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------------
class CheckConstraint;
class DomainDesc;
class NATable;
class NAType;
class TrafColumnsDesc;

enum ColumnClass  { SYSTEM_COLUMN, USER_COLUMN, USER_AND_SYSTEM_COLUMNS };
enum SortOrdering { NOT_ORDERED = 0, ASCENDING = +1, DESCENDING = -1 };

// ***********************************************************************
//
// NAColumn : New Abstraction for a Column
//
// A NAColumn is the internal representation for a column that belongs
// to an SQL table or index for which a NATable or NAFileSet object
// has been allocated.
// It contains a physical description for the column such as its position
// in the row, its data type and size of the storage that is required,
// any constraints that exist. (The type information and the constraints
// are encapsulated in a Domain object.) It can persist across the
// compilation of several SQL statements.
//
// ***********************************************************************

class NAColumn : public NABasicObject
{
public:

  enum KeyKind	  { NON_KEY = 0,
		    INDEX_KEY = 0x1, PARTITIONING_KEY = 0x2,
                    PRIMARY_KEY = 0x4,
                    PRIMARY_KEY_NOT_SERIALIZED = 0x8
		  };
  enum NAColumnAssert { MustBeBaseColumn, MustBeColumn, NoError };

  // ---------------------------------------------------------------------
  // Constructor
  // ---------------------------------------------------------------------

  NAColumn(const char* colName,
           Lng32 position,
           NAType *type,
           CollHeap *h,
           const NATable* table = NULL,
           ColumnClass columnClass = USER_COLUMN,
           const ComColumnDefaultClass defaultClass = COM_NO_DEFAULT,
           char* defaultValue = NULL,
           char* heading = NULL,
           NABoolean upshift = FALSE,
           NABoolean addedColumn = FALSE,
           ComColumnDirection colDirection = COM_UNKNOWN_DIRECTION,
           NABoolean isOptional = FALSE,
           char *routineParamType = NULL,
           NABoolean storedOnDisk = TRUE,
           char *computedColExpr = NULL,
           NABoolean isSaltColumn = FALSE,
           NABoolean isDivisioningColumn = FALSE,
           NABoolean isAlteredColumn = FALSE)
  : heap_(h),
    colName_(colName, h),
    position_(position),
    type_(type),
    table_(table),
    columnClass_(columnClass),
    defaultClass_(defaultClass),
    defaultValue_(defaultValue),
    heading_(heading),
    upshift_(upshift),
    addedColumn_(addedColumn),
    alteredColumn_(isAlteredColumn),
    keyKind_(NON_KEY),
    clusteringKeyOrdering_(NOT_ORDERED),
    isNotNullNondroppable_(NULL),
    mvSystemAddedColumn_(FALSE),
    referenced_(NOT_REFERENCED),
    needHistogram_(DONT_NEED_HIST),
    hasJoinPred_(FALSE),
    hasRangePred_(FALSE),
    columnMode_ (colDirection),
    isUnique_(FALSE),
    isOptional_(isOptional),
    storedOnDisk_(storedOnDisk),
    computedColumnExpression_(computedColExpr),
    isSaltColumn_(isSaltColumn),
    isDivisioningColumn_(isDivisioningColumn),
    lobNum_(-1),
    lobStorageType_(Lob_HDFS_File),
    lobStorageLocation_(NULL),
    hbaseColFlags_(0)
  {
    routineParamType_[0] = 0;
     if (routineParamType) strncpy(routineParamType_, routineParamType, 2);
     else                  routineParamType_[0] = 0;
    routineParamType_[2] = 0;
  }

  NAColumn (const NAColumn & nac, NAMemory * h):
  heap_(h),
    colName_(nac.colName_,h),
    position_(nac.position_),
    type_(nac.type_),
    table_(nac.table_),
    columnClass_(nac.columnClass_),
    defaultClass_(nac.defaultClass_),
    defaultValue_(nac.defaultValue_),
    heading_(nac.heading_),
    upshift_(nac.upshift_),
    addedColumn_(nac.addedColumn_),
    alteredColumn_(nac.alteredColumn_),
    keyKind_(nac.keyKind_),
    clusteringKeyOrdering_(nac.clusteringKeyOrdering_),
    isNotNullNondroppable_(nac.isNotNullNondroppable_),
    referenced_(nac.referenced_),
    needHistogram_(nac.needHistogram_),
    hasJoinPred_(nac.hasJoinPred_),
    hasRangePred_(nac.hasRangePred_),
    columnMode_ (nac.columnMode_),
    isUnique_(nac.isUnique_),
    isOptional_(nac.isOptional_),
    storedOnDisk_(nac.storedOnDisk_),
    computedColumnExpression_(nac.computedColumnExpression_),
    isSaltColumn_(nac.isSaltColumn_),
    isDivisioningColumn_(nac.isDivisioningColumn_),
    lobNum_(nac.lobNum_),
    lobStorageType_(nac.lobStorageType_),
    lobStorageLocation_(nac.lobStorageLocation_),
    hbaseColFam_(nac.hbaseColFam_),
    hbaseColQual_(nac.hbaseColQual_),
    hbaseColFlags_(nac.hbaseColFlags_)
  {
    routineParamType_[0] = 0;
    if (nac.routineParamType_) strncpy(routineParamType_, nac.routineParamType_, 2);
    routineParamType_[2] = 0;
  }

  virtual ~NAColumn();
  virtual void deepDelete();

  //IF U R CHANGING ANYTHING IN NAColumn MAKE SURE TO READ THE FOLLOWING
  //Anything that changes in a NAColumn object over the course
  //of a single statement is reset back to its value at it was
  //after NATable construction.
  //********************************************************************
  //If change any datamembers of NAColumn after NATable contruction
  //U SHOULD RESET those datamembers in here to their value as it is
  //after NATable construction.
  //NATable construction is invoke from BindWA::getNATable which is
  //called while binding leaf nodes like Scan, Insert, etc.
  //This is used to reset the NAColumn object so that it can be used
  //by subsequent statements. This method is invoked at the end of a
  //statement by NATable::resetAfterStatement().
  //********************************************************************
  void resetAfterStatement();

  // deepCopy of the members necessary
  static NAColumn * deepCopy(const NAColumn & nac,NAMemory * heap);

  // ---------------------------------------------------------------------
  // Accessor functions
  // ---------------------------------------------------------------------
  inline const NAString& getColName() const	{ return colName_; }
  inline const NATable* getNATable() const	{ return table_; }
  const QualifiedName* getTableName(NABoolean okIfNoTable = FALSE) const;

  inline ColRefName getFullColRefName() const
  {
    const QualifiedName* qn = getTableName(TRUE);
    return qn ? ColRefName(getColName(), *qn) : ColRefName(getColName());
  }
  inline NAString getFullColRefNameAsAnsiString() const
         { return getFullColRefName().getColRefAsAnsiString(); }

  inline Lng32 getPosition() const 		{ return position_; }
  inline const NAType* getType() const		{ return type_; }
  inline NAType*& mutateType()			{ return type_; }
  //Returns TRUE if type_ is a numeric type
  NABoolean isNumeric() const;
  inline const char* getDefaultValue() const	{ return defaultValue_; }
  inline const ComColumnDefaultClass getDefaultClass() const	{ return defaultClass_; }
  inline const char* getHeading() const	        { return heading_; }
  inline ColumnClass getColumnClass() const     { return columnClass_; }
  inline NABoolean isUpshiftReqd() const        { return upshift_; }
  inline NABoolean isUserColumn() const         { return columnClass_ == USER_COLUMN; }
  inline NABoolean isSystemColumn() const       { return columnClass_ == SYSTEM_COLUMN;}
  inline NABoolean isSyskeyColumn() const       { return columnClass_ == SYSTEM_COLUMN && colName_ == "SYSKEY" &&
                                                         defaultClass_ == COM_NO_DEFAULT;}

  inline NABoolean isIdentityColumn() const {return (defaultClass_ == COM_IDENTITY_GENERATED_BY_DEFAULT ||
                                                     defaultClass_ == COM_IDENTITY_GENERATED_ALWAYS);}
  inline NABoolean isIdentityColumnByDefault() const {return defaultClass_ == COM_IDENTITY_GENERATED_BY_DEFAULT;}
  inline NABoolean isIdentityColumnAlways() const {return defaultClass_ == COM_IDENTITY_GENERATED_ALWAYS;}
  inline virtual NABoolean isComputedColumn() const { return (defaultClass_ == COM_ALWAYS_COMPUTE_COMPUTED_COLUMN_DEFAULT ||
                                                      defaultClass_ == COM_ALWAYS_DEFAULT_COMPUTED_COLUMN_DEFAULT);}
  inline NABoolean isComputedColumnAlways() const { return defaultClass_ == COM_ALWAYS_COMPUTE_COMPUTED_COLUMN_DEFAULT; }
  inline const char* getComputedColumnExprString() const { return computedColumnExpression_; }
  inline NABoolean isStoredOnDisk() const       { return storedOnDisk_; }
  inline NABoolean isAddedColumn() const { return addedColumn_; }
  inline NABoolean isAlteredColumn() const { return alteredColumn_; }
  inline NABoolean isSaltColumn() const        { return isSaltColumn_;}
  inline NABoolean isDivisioningColumn() const { return isDivisioningColumn_; }

  inline SortOrdering getClusteringKeyOrdering() const	{return clusteringKeyOrdering_;}
  inline NABoolean isClusteringKey() const	{ return clusteringKeyOrdering_
  						 != NOT_ORDERED; 	}
  inline void setClusteringKey(SortOrdering o)	{ clusteringKeyOrdering_ = o;
  					  setIndexKey(); 		}

  inline NABoolean isUnique() const            { return isUnique_;}
  inline void setIsUnique()                    { isUnique_ = TRUE; }
  inline NABoolean isOptional() const          { return isOptional_;}
  inline void setIsOptional(NABoolean optional){ isOptional_ = optional; }
  const char *getRoutineParamType() const      { return &routineParamType_[0]; }
  inline void setRoutineParamType (char *type) { strncpy(routineParamType_, type, 2);
                                                 routineParamType_[2]=0; }
  inline NABoolean isIndexKey() const          { return keyKind_ & INDEX_KEY; }
  inline void setIndexKey()                    { keyKind_ |= INDEX_KEY; }
  inline NABoolean isPartitioningKey() const   { return keyKind_ & PARTITIONING_KEY; }
  inline void setPartitioningKey()             { keyKind_ |= PARTITIONING_KEY; }
  inline NABoolean isPrimaryKey() const        { return keyKind_ & PRIMARY_KEY;}
  inline NABoolean isPrimaryKeyNotSerialized() const 
  { return keyKind_ & PRIMARY_KEY_NOT_SERIALIZED;}
  inline void setPrimaryKey()                  { keyKind_ |= PRIMARY_KEY; }
  inline void setPrimaryKeyNotSerialized()     { keyKind_ |= PRIMARY_KEY_NOT_SERIALIZED; }
  inline void setIsStoredOnDisk(NABoolean x)   { storedOnDisk_ = x; }
  inline NABoolean isReferenced() const        { return (referenced_ != NOT_REFERENCED); }
  inline NABoolean isReferencedForHistogram() const
              { return ((referenced_ == REFERENCED_FOR_MULTI_INTERVAL_HISTOGRAM)
                         || (referenced_ == REFERENCED_FOR_SINGLE_INTERVAL_HISTOGRAM) ); }
  inline NABoolean isReferencedForSingleIntHist() const
         { return (referenced_ == REFERENCED_FOR_SINGLE_INTERVAL_HISTOGRAM); }

  NABoolean isReferencedForMultiIntHist() const
  { return (referenced_ == REFERENCED_FOR_MULTI_INTERVAL_HISTOGRAM); }

  NABoolean needHistogram() const
    { return (needHistogram_==NEED_COMPRESSED) ||
        (needHistogram_==NEED_FULL); }

  NABoolean needFullHistogram() const
    { return needHistogram_==NEED_FULL; }

  NABoolean needCompressedHistogram() const 
    { return needHistogram_==NEED_COMPRESSED; }

  inline NABoolean hasJoinPred() const
              { return hasJoinPred_;}
  inline NABoolean hasRangePred() const
              { return hasRangePred_;}
  inline void setReferenced()
         { if (!isReferenced()) referenced_ = REFERENCED_ANYWHERE; }

  void setReferencedForMultiIntHist()
         { referenced_ = REFERENCED_FOR_MULTI_INTERVAL_HISTOGRAM; }

  inline void setReferencedForSingleIntHist()
         { referenced_ = REFERENCED_FOR_SINGLE_INTERVAL_HISTOGRAM; }

  void setDontNeedHistogram() { needHistogram_ = DONT_NEED_HIST; }
  void setNeedFullHistogram() { needHistogram_ = NEED_FULL; }
  void setNeedCompressedHistogram() { needHistogram_ = NEED_COMPRESSED; }

  //use this method to mark the column as being involved in a join predicate
  inline void setHasJoinPred()
         { if (!hasJoinPred()) hasJoinPred_ = TRUE; }

  //use this method to mard the column as being involved in a range predicate
  inline void setHasRangePred()
         { if (!hasRangePred()) hasRangePred_ = TRUE; }

  inline CheckConstraint* getNotNullNondroppable() { return isNotNullNondroppable_; } const
  inline void setNotNullNondroppable(CheckConstraint *c) { isNotNullNondroppable_ = c;}
  Lng32 getNotNullViolationCode() const;
  const QualifiedName &getNotNullConstraintName() const;
  inline void setNotReferencedAndNotKey()
  {
	  referenced_ = NOT_REFERENCED;
	  keyKind_ &= NON_KEY;
  }
  inline void setNotReferenced()      { referenced_ = NOT_REFERENCED; }

  inline void setMvSystemAddedColumn() { mvSystemAddedColumn_ = TRUE; }
  inline NABoolean isMvSystemAddedColumn() const { return mvSystemAddedColumn_; }

  static NABoolean createNAType(TrafColumnsDesc *column_desc	/*IN*/,
				const NATable *table  		/*IN*/,
				NAType *&type       		/*OUT*/,
				NAMemory *heap			/*IN*/,
				Lng32 * errorCode = NULL
				);

  // ---------------------------------------------------------------------
  // Standard operators
  // ---------------------------------------------------------------------
  NABoolean operator==(const NAColumn& other) const;
  NABoolean operator==(const NAString& other) const;

  // needed by priority_queue for printing column names in order
  // do NOT use this operator for any other purpose because it
  // copies only the column name
  void operator = (const NAColumn& other)
    { colName_ = other.colName_; }

  // ---------------------------------------------------------------------
  // Display function for debugging
  // ---------------------------------------------------------------------
  void print(FILE* ofd = stdout,
             const char* indent = DEFAULT_INDENT,
	     const char* title = "NAColumn",
             CollHeap *c=NULL, char *buf=NULL) const;

  inline void display() const { print(); }
  void trace (FILE *f) const;

  inline ComColumnDirection getColumnMode ()  const {return columnMode_;}
  inline void setColumnMode (ComColumnDirection mode){columnMode_ = mode;}

  short &lobNum() {return lobNum_; }
  LobsStorage &lobStorageType() { return lobStorageType_; }
  char* &lobStorageLocation() { return lobStorageLocation_; }

  void setIndexColName(const char *idxName)
  {indexColName_ = idxName;}
  const NAString& getIndexColName() const	
  { 
    return (indexColName_.isNull() ? getColName() : indexColName_); 
  }

  void setHbaseColFam(const char * colFam)
  {
    if (colFam)
      hbaseColFam_ = colFam;
  }
  const NAString &getHbaseColFam() const
  {
    return 
      hbaseColFam_;
  }

  void setHbaseColQual(const char * colQual)
  {
    if (colQual)
      hbaseColQual_ = colQual;
  }
  const NAString &getHbaseColQual() const
  {
    return hbaseColQual_;
  }

  void setHbaseColFlags(const ULng32 colFlags)
  {
    hbaseColFlags_ = colFlags;
  }

  const ULng32 getHbaseColFlags() const
  {
    return hbaseColFlags_;
  }

  void resetSerialization() {
     hbaseColFlags_ &= SEABASE_SERIALIZED;
  }

  enum {
    SEABASE_SERIALIZED = 0x0001
  };

private:
  enum referencedState { NOT_REFERENCED, REFERENCED_ANYWHERE, REFERENCED_FOR_MULTI_INTERVAL_HISTOGRAM, REFERENCED_FOR_SINGLE_INTERVAL_HISTOGRAM };

  enum histRequirement {
    DONT_NEED_HIST, NEED_COMPRESSED, NEED_FULL
  };
  NAMemory * heap_;
  // ---------------------------------------------------------------------
  // The column name
  // ---------------------------------------------------------------------
  NAString colName_;

  // ---------------------------------------------------------------------
  // The ordinal position of the column within the table or index, zero-based.
  // ---------------------------------------------------------------------
  Lng32 position_;

  // ---------------------------------------------------------------------
  // A pointer to its Type Descriptor.
  // ---------------------------------------------------------------------
  NAType *type_;

  // ---------------------------------------------------------------------
  // A pointer to its containing table (NULL if caller doesn't care)
  // ---------------------------------------------------------------------
  const NATable *table_;

  // ---------------------------------------------------------------------
  // Column class (SYSTEM_COLUMN or USER_COLUMN).
  // ---------------------------------------------------------------------
  ColumnClass columnClass_;

  // ---------------------------------------------------------------------
  // Default value (NULL pointer if NO DEFAULT; "NULL" if DEFAULT NULL;
  // some string otherwise). NOTE: This is stored in UCS2.
  // ---------------------------------------------------------------------
  const ComColumnDefaultClass defaultClass_;
  char *defaultValue_;

  // ---------------------------------------------------------------------
  // MP column heading
  // ---------------------------------------------------------------------
  char *heading_;

  // ---------------------------------------------------------------------
  // MP column UPSHIFT flag
  // ---------------------------------------------------------------------
  NABoolean upshift_;

  // ---------------------------------------------------------------------
  // Indicates whether this is a key in any index, a horiz partitioning key,
  // or a primary key
  // (a PK column is also a CLUSTERING KEY col only for NONDROPPABLE PK's).
  // ---------------------------------------------------------------------
  Int32 keyKind_;

  // ---------------------------------------------------------------------
  // Indicates whether this is a CLUSTERING KEY column
  // ---------------------------------------------------------------------
  SortOrdering clusteringKeyOrdering_;

  // ---------------------------------------------------------------------
  // If this column does not support nulls, then save the
  // "col NOT NULL NONDROPPABLE" constraint so Generator can store the
  // constraint name for Executor to put into diags as runtime violation
  // EXE_TABLE_CHECK_CONSTRAINT or EXE_CHECK_OPTION_VIOLATION
  // when an attempt is made to put a NULL into the column
  // (which would otherwise be reported as EXE_NUMERIC_OVERFLOW).
  // ---------------------------------------------------------------------
  CheckConstraint *isNotNullNondroppable_;

  // ---------------------------------------------------------------------
  // Indicates if this column is unique
  // ---------------------------------------------------------------------
  NABoolean isUnique_;

  // ---------------------------------------------------------------------
  // Optional flag - used with UDFs and other routines
  // ---------------------------------------------------------------------
  NABoolean isOptional_;

  // ---------------------------------------------------------------------
  // For UDFs:  The parameter type if this col is an argument to a UDF
  // ---------------------------------------------------------------------
  char routineParamType_[3];

  // ---------------------------------------------------------------------
  // Set to enum type REFERENCED_ANYWHERE if the column is referenced
  // anywhere in a DML query or set to REFERENCED_FOR_MULTI_INTERVAL_HISTOGRAM if the
  // optimizer need Histograms to be fetched for the column.
  // see BindItemExpr markAsReferencedColumn() for criteria.
  // ---------------------------------------------------------------------
  referencedState referenced_;

  // need to decouple concept of referenced_ 
  // (ie, where column is referenced in query) 
  // from concept of needHistogram_
  // (ie, if we need to get histograms for column
  //  and type needed: NEED_FULL or NEED_COMPRESSED)
  histRequirement needHistogram_;

  // MVs --
  // This column was added to the MV by CREATE MV. Don't show it 
  // in SELECT * statements.
  NABoolean mvSystemAddedColumn_;

  // ----------------------------------------------------
  // Set to TRUE if this column was an added column.
  // ----------------------------------------------------
  NABoolean addedColumn_;

  // ----------------------------------------------------
  // Set to TRUE if this column was altered by datatype change
  // ----------------------------------------------------
  NABoolean alteredColumn_;

  // ----------------------------------------------------
  // Set to TRUE if there is a join predicate on this column.
  // ----------------------------------------------------
  NABoolean hasJoinPred_;

  // ----------------------------------------------------
  // Set to TRUE if there is a range predicate on this column.
  // ----------------------------------------------------
  NABoolean hasRangePred_;

  // Used by CALL statement parameters
  ComColumnDirection columnMode_;

  // Is this (computed) column physically materialized in the
  // record on disk or is it virtual, computed when needed?
  NABoolean storedOnDisk_;

  // expression to compute a computed column or NULL, stored in UTF8
  char *computedColumnExpression_;
  NABoolean isSaltColumn_;
  NABoolean isDivisioningColumn_;

  // next 3 fields are for lob columns. They identify where
  // the lob data for that column is stored.
  short lobNum_;
  LobsStorage lobStorageType_;
  char *lobStorageLocation_;

  NAString indexColName_;

  NAString hbaseColFam_;
  NAString hbaseColQual_;
  ULng32 hbaseColFlags_;

}; // class NAColumn

typedef NABoolean (NAColumn::*NAColumnBooleanFuncPtrT)() const;

// ***********************************************************************
// An array of column pointers
// ***********************************************************************

class NAColumnArray : public LIST(NAColumn*)
{
public:

  NAColumnArray(CollHeap* h=CmpCommon::statementHeap()) : LIST(NAColumn*)(h), ascending_(h) {}

  // Copy constructor
  // NOTE: This only copies NAColumn pointers.  It does not do a deep copy.
  //       So, it should not be used to create an NAColumn on a new heap.
  NAColumnArray(const NAColumnArray & naca, CollHeap *h=0)
    : LIST (NAColumn*)(naca, h), ascending_(naca.ascending_, h)
    {}

  virtual ~NAColumnArray();
  virtual void deepDelete();
  // Assignment
  // NOTE: This only copies NAColumn pointers.  It does not do a deep copy.
  //       So, it should not be used to copy to an NAColumn on a different heap.
  NAColumnArray & operator=(const NAColumnArray& other);

  NABoolean operator==(const NAColumnArray& other) const;

  virtual void print( FILE* ofd = stdout,
		      const char* indent = DEFAULT_INDENT,
                      const char* title = "NAColumnArray",
                      CollHeap *c=NULL, char *buf=NULL) const;

  void display() const { print(); }
  void trace (FILE *f) const;

  // ---------------------------------------------------------------------
  // A method for inserting new NAColumns is required in order to grow
  // the ascending_ array at the same rate as the LIST(NAColumn *).
  // ---------------------------------------------------------------------
  void insertAt(CollIndex index, NAColumn * newColumn);
  void insert(NAColumn * newColumn)	{ insertAt(entries(),newColumn); }

  // ---------------------------------------------------------------------
  // check and set whether a column in the column list has ascending or
  // descending order (ascending is the default order, if nothing was set)
  // ---------------------------------------------------------------------
  NABoolean isAscending(CollIndex i) const
  { return (ascending_.used(i) ? ascending_[i] : TRUE); }
  void setAscending(CollIndex i, NABoolean a = TRUE)
  { ascending_.insertAt(i,a); }

  // ---------------------------------------------------------------------
  // Mark each NAColumn in the array
  // ---------------------------------------------------------------------
  void setIndexKey() const
  { for (CollIndex i = 0; i < entries(); i++) at(i)->setIndexKey(); }
  void setPartitioningKey() const
  { for (CollIndex i = 0; i < entries(); i++) at(i)->setPartitioningKey(); }

  // ---------------------------------------------------------------------
  // calculate offsets for columns, assuming that all columns in the
  // NAColumn array are contiguously stored
  // ---------------------------------------------------------------------
  Lng32 getOffset(Lng32 position) const;

  // ---------------------------------------------------------------------
  // get column (non-const, can be modified) by position or by name lookup
  // ---------------------------------------------------------------------
  NAColumn * getColumn(Lng32 index) const;	// same as nacolarray[position]
  NAColumn * getColumn(const char* colName) const;
  NAColumn * getColumnByPos(Lng32 position) const;
  void removeByPosition(Lng32 position);

  // return 
  //    i (i>=0) if the column is found in the array via NAColumn::operator==
  //    -1 if the column is not found in the array
  Int32 getColumnPosition(NAColumn&) const;

  Int32 getColumnPosition(NAString&) const;

  // get total storage size (aggregated over each element)
  Int32 getTotalStorageSize() const;

  // For Trafodion tables column qualifier is an unsigned 
  // numeric > 0. This method is used during alter table add
  // column to find the maximum value currently in use. Columns
  // are deleted during alter table drop column.
  ULng32 getMaxTrafHbaseColQualifier() const;

  NAString getColumnNamesAsString(char separator) const;
  NAString getColumnNamesAsString(char separator, UInt32 ct) const;

private:

  ARRAY(NABoolean) ascending_; // ignore for non-key or hash key columns

}; // class NAColumnArray

#endif /* NACOLUMN_H */
