/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1998-2014 Hewlett-Packard Development Company, L.P.
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
//
**********************************************************************/
#ifndef DESC_H
#define DESC_H

typedef Lng32 DataType;

#include "Platform.h"
#include "dfs2rec.h"
#include "BaseTypes.h"
#include "CmpCommon.h"	// defines HEAP for allocate() + all funcs in sqlcat
#include "ItemConstr.h"	// defines ConstraintType enum
#include "NAFileSet.h"
#include "ComSmallDefs.h"

class ElemDDLColDef;

enum ConstraintType { UNIQUE_CONSTRAINT, PRIMARY_KEY_CONSTRAINT, REF_CONSTRAINT,
		      CHECK_CONSTRAINT, MP_CHECK_CONSTRAINT,
			// The next value is for catsim only;
			// in ARK, NOT NULL constraints are CHECK constraints:
		      CATSIM_NOT_NULL_CONSTRAINT
		    };

enum desc_nodetype {
  DESC_UNKNOWN_TYPE = 0,
  DESC_CHECK_CONSTRNTS_TYPE,
  DESC_COLUMNS_TYPE,
  DESC_CONSTRNTS_TYPE,
  DESC_CONSTRNT_KEY_COLS_TYPE,
  DESC_FILES_TYPE,
  DESC_HISTOGRAM_TYPE,
  DESC_HIST_INTERVAL_TYPE,
  DESC_INDEXES_TYPE,
  DESC_KEYS_TYPE,
  DESC_PARTNS_TYPE,
  DESC_REF_CONSTRNTS_TYPE,
  DESC_TABLE_TYPE,
  DESC_USING_MV_TYPE,  // MV  -- marks an MV using this object
  DESC_VIEW_TYPE,
  DESC_SCHEMA_LABEL_TYPE,
  DESC_SEQUENCE_GENERATOR_TYPE,
  DESC_HBASE_HASH2_REGION_TYPE,
  DESC_HBASE_RANGE_REGION_TYPE,
  DESC_ROUTINE_TYPE
};

typedef ComDiskFileFormat UnderlyingFileType;

struct desc_struct;	// forward reference; fully defined way at bottom

struct header_desc_struct {
  Int32 nodetype;
  Int32 OSV;			// Object Schema Version, created at object creation time
  Int32 OFV;
  desc_struct *next;
};

struct table_desc_struct {
  char *tablename;
  char *catalogName;
  char *parentTableName;
  char *schemalabelfilename;    // the physical file, to be Opened for auth-cking.
  ComTimestamp schemaRedefTime;
  UnderlyingFileType underlyingFileType;  // SQLMP or SQLMX 
  ULng32 createtime[2];
  ULng32 redeftime[2];
  ULng32 cachetime[2];
  Int32 issystemtablecode;	// logically belongs in files_desc_struct but..

  Int32 isUMDTable;               // VO: Set to true if the table is one of the histograms UMD tables,
                                //     the system_defaults UMD table or an odbc_schema UMD table.
                                //     Otherwise set to false.
  Int32 isMVtable;  // MV -- mark this table as an MV table
  Int32 hasIUDLog;    // MV -- does this table have a log ?
  Int32 isMVMetaDataObject;
  Int32 isSynonymNameTranslationDone;
  ComSInt32  mvAttributesBitmap;

  Int32 isVolatile;
  Int32 isInMemoryObjectDefn;
  Int32 isDroppable;
  ComInsertMode insertMode;

  Int32 record_length;
  Int32 colcount;
  Int32 constr_count;
  Cardinality rowcount;
  ComPartitioningScheme partitioningScheme;   // round robin, range, etc
  ComAccessPathType accessPathType;     //BT, IX, isVP, hasVP, LobTable --needed for binder check
  ComRowFormat  rowFormat;
  ULng32 catUID[2];
  ULng32 schemaUID[2];
  ULng32 objectUID[2];
  Lng32 owner;
  Lng32 schemaOwner;
  ComObjectType objectType; 
  void * rcb;
  ULng32 rcbLen;
  ULng32 maxRecLen;
  ULng32 keyLen;
  const void *securityLabel;
  ULng32 securityLabelLen;
  void * privInfo;
  void * secKeySet;
  void *constraintInfo;
  ULng32 constraintInfoLen;
  ComBoolean isInsertOnly;
  desc_struct *columns_desc;
  desc_struct *indexes_desc;
  desc_struct *constrnts_desc;
  desc_struct *views_desc;
  desc_struct *constrnts_tables_desc;
  desc_struct *referenced_tables_desc;
  desc_struct *referencing_tables_desc;
  desc_struct *histograms_desc;
  desc_struct *files_desc;
  desc_struct *using_mvs_desc;
// for hbase's region keys
  desc_struct *hbase_regionkey_desc;
  desc_struct *sequence_generator_desc;
#ifdef NA_LITTLE_ENDIAN
  void encode() {}
#endif
};

// MV 
struct using_mv_desc_struct {
  char *mvName;
  ComMVRefreshType refreshType; // unknown here means "non incremental"
  Int32  rewriteEnabled;
  Int32  isInitialized;
};

struct view_desc_struct {
  char * viewname;
  char * viewfilename;    // the physical file, to be Opened for auth-cking.
  char * viewtext;
  char * viewchecktext;
  Int32 updatable;
  Int32 insertable;
  CharInfo::CharSet viewtextcharset;
#ifdef NA_LITTLE_ENDIAN
  void encode() {}
#endif
};

struct indexes_desc_struct {
  char *tablename;  // name of the base table
  char *ext_indexname; // (ansi) name of index, as specified in CREATE INDEX
  char *indexname;  // physical name of index. Different from ext_indexname
                    // for ARK tables.
  ULng32 redeftime[2];
  Int32 issystemtablecode;	// ...but too much bother in ReadTableDef.C
  // Indicates whether it is a vertical partition or an index
  ComPartitioningScheme partitioningScheme;     // round robin, range, etc.
  Int32 isVerticalPartition;
  Int32 isCreatedExplicitly;
  Int32 isPacked;
  Int32 isVolatile;

  // this object was only created in mxcmp memory(catman cache, NAtable
  // cache. It doesn't exist in metadata or physical labels.
  // Used to test different access plans without actually creating
  // the object. 
  Int32 isInMemoryObjectDefn;

  Int32 notAvailable;
  Lng32 packingScheme;
  Lng32 packingFactor;
  Int32 keytag;
  Int32 unique;
  Int32 record_length;
  Int32 colcount;
  Int32 blocksize;
  Lng32 numSaltPartns; // number of salted partns created for a seabase table.
  char * hbaseCreateOptions;
  desc_struct *files_desc;
  // Clustering keys columns
  desc_struct *keys_desc;
  // Columns that are not part of the clustering key.
  // Used specially for vertical partition column(s).
  desc_struct *non_keys_desc;
  // Partitioning key columns -- used by NonStop SQL/MX to accommodate
  // independent partitioning and clustering keys
  desc_struct *partitioning_keys_desc;
#ifdef NA_LITTLE_ENDIAN
  void encode() {}
#endif
};

struct files_desc_struct {
  FileOrganizationEnum fileorganization;
  Int32 audit;
  Int32 auditcompress;
  Int32 clearOnPurge;
  Int32 lockLength;
  Int32 fileCode; 
  Int32 buffered;
  Int32 compressed;
  Int32 dcompressed;
  Int32 icompressed;
  Int32 blockSize;
  Int32 decoupledPartitionKeyList;
  desc_struct *partns_desc;
#ifdef NA_LITTLE_ENDIAN
  void encode() {}
#endif
};

struct keys_desc_struct {
  char *indexname;
  char *keyname;
  Int32  keyseqnumber;
  Int32  tablecolnumber;
  Int32  basetablecolnumber;  // for indexes, this is the corresponding base
                            // table colnum. This is needed for the parallel
                            // label operations.
  Int32  ordering; // 0 is ascending, -1 is descending
  char *hbaseColFam;
  char *hbaseColQual;
#ifdef NA_LITTLE_ENDIAN
  void encode() {}
#endif
};

struct partns_desc_struct
{
  char * tablename;
  Int32 primarypartition;
  char * partitionname;
  char * logicalpartitionname;
  char * firstkey;
  Lng32 firstkeylen;         //soln:10-031112-1256
  Lng32 encodedkeylen;
  char * encodedkey;
  char * lowKey;
  char * highKey;
  Int32    indexlevel;
  Int32  priExt;
  Int32  secExt;
  Int32  maxExt;
  char * givenname;
#ifdef NA_LITTLE_ENDIAN
  void encode() {}
#endif
};

struct hbase_region_desc_struct
{
  Lng32  beginKeyLen;
  char * beginKey;
  Lng32  endKeyLen;
  char * endKey;
};

struct columns_desc_struct {
  char *tablename;
  char *colname;
  Int32 colnumber;
  DataType datatype;
  char * pictureText;
  Lng32 length;
  Lng32 scale;
  Lng32 precision;
  rec_datetime_field datetimestart, datetimeend;
  short datetimefractprec, intervalleadingprec;
  Lng32 offset;
  short null_flag;
  short upshift;
  short caseinsensitive;
  char colclass; // 'S' -- system generated, 'U' -- user created
  short addedColumn;
  Cardinality uec;
  char *highval;
  char *lowval;
  ComColumnDefaultClass defaultClass;
  char *defaultvalue;
  char *heading;
  CharInfo::CharSet   character_set;
  CharInfo::CharSet   encoding_charset;
  CharInfo::Collation collation_sequence;
  short stored_on_disk;
  char *computed_column_text;
  char *hbaseColFam;
  char *hbaseColQual;
  ULng32 hbaseColFlags;
  ComParamDirection paramDirection;
  NABoolean isOptional; 
#ifdef NA_LITTLE_ENDIAN
  void encode() {}
#endif
};

struct constrnts_desc_struct {
  char *constrntname;
  char *tablename;
  ConstraintType type;
  Int32  colcount;
  Int32  isEnforced;
  char *indexname;
  desc_struct *check_constrnts_desc;
  desc_struct *constr_key_cols_desc;
  desc_struct *referenced_constrnts_desc;
  desc_struct *referencing_constrnts_desc;
#ifdef NA_LITTLE_ENDIAN
  void encode() {}
#endif
};

struct check_constrnts_desc_struct {
  Int32 seqnumber;
  char *constrnt_text;
  CharInfo::CharSet constrnt_textcharset;
#ifdef NA_LITTLE_ENDIAN
  void encode() {}
#endif
};

struct constrnt_key_cols_desc_struct {
  char *colname;
  Int32  position;
#ifdef NA_LITTLE_ENDIAN
  void encode() {}
#endif
};

struct ref_constrnts_desc_struct {
  char *constrntname;
  char *tablename;
#ifdef NA_LITTLE_ENDIAN
  void encode() {}
#endif
};

struct histogram_desc_struct
{
  char *tablename;
  Int32  tablecolnumber;
  char *histid;
  Int32  colposition;
  Cardinality rowcount;
  Cardinality uec;
  char *highval;
  char *lowval;
  desc_struct *hist_interval_desc;
#ifdef NA_LITTLE_ENDIAN
  void encode() {}
#endif
};

struct hist_interval_desc_struct
{
  char * histid;
  Int32    intnum;
  char * intboundary;
  Cardinality rowcount;
  Cardinality uec;
#ifdef NA_LITTLE_ENDIAN
  void encode() {}
#endif
};

struct sequence_generator_desc_struct {
  ComSequenceGeneratorType  sgType;
  ComSInt64                 startValue;
  ComSInt64                 increment;
  ComSQLDataType            sqlDataType;
  ComFSDataType             fsDataType;
  ComSInt64                 maxValue;
  ComSInt64                 minValue;
  ComBoolean                cycleOption;
  ComSInt64                  cache;
  ComSInt64                  objectUID;
  char *                          sgLocation;
  ComSInt64                 nextValue;
#ifdef NA_LITTLE_ENDIAN
  void encode() {}
#endif  
};

struct routine_desc_struct {
  char *routineName;
  char *externalName;
  char *librarySqlName;
  char *libraryFileName;
  char *signature;
  ComSInt32 paramsCount;
  desc_struct *params;
  ComRoutineLanguage language;
  ComRoutineType UDRType;
  ComRoutineSQLAccess sqlAccess;
  ComRoutineTransactionAttributes transactionAttributes;
  ComSInt32 maxResults;
  ComRoutineParamStyle paramStyle;
  NABoolean isDeterministic;
  NABoolean isCallOnNull;
  NABoolean isIsolate;
  ComRoutineExternalSecurity externalSecurity;
  ComRoutineExecutionMode executionMode;
  Int32 stateAreaSize;
  ComRoutineParallelism parallelism;
#ifdef NA_LITTLE_ENDIAN
  void encode() {}
#endif
};

union body_struct {
  check_constrnts_desc_struct check_constrnts_desc;
  columns_desc_struct columns_desc;
  constrnts_desc_struct constrnts_desc;
  constrnt_key_cols_desc_struct constrnt_key_cols_desc;
  files_desc_struct files_desc;
  histogram_desc_struct histogram_desc;
  hist_interval_desc_struct hist_interval_desc;
  indexes_desc_struct indexes_desc;
  keys_desc_struct keys_desc;
  partns_desc_struct partns_desc;
  ref_constrnts_desc_struct ref_constrnts_desc;
  table_desc_struct table_desc;
  view_desc_struct view_desc;
  using_mv_desc_struct  using_mv_desc;  // MV 
  sequence_generator_desc_struct  sequence_generator_desc;  
  hbase_region_desc_struct hbase_region_desc;
  routine_desc_struct routine_desc;
};

struct desc_struct {
  header_desc_struct header;
  body_struct body;

  void pack(char *buffer);
  Lng32 getLength(void);
};

// uses HEAP of CmpCommon!
desc_struct *readtabledef_allocate_desc(desc_nodetype nodetype);

desc_struct *readtabledef_make_column_desc(
				const char *tablename,
				const char *colname,
				Lng32 &colnumber,		// INOUT
				DataType datatype,
				Lng32 length,
				Lng32 &offset,			// INOUT
				short null_flag = FALSE,
				NABoolean tablenameMustBeAllocated = FALSE,
				desc_struct *passedDesc = NULL,
				SQLCHARSET_CODE datacharset = SQLCHARSETCODE_UNKNOWN // i.e., use CharInfo::DefaultCharSet;
				);

#endif
