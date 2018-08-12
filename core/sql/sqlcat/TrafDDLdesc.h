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
//
**********************************************************************/
#ifndef TRAF_DDL_DESC_H
#define TRAF_DDL_DESC_H

// ****************************************************************************
// This file contains DDLDesc classes. DDLDesc classes are used to store
// object definitions that are read from system and privilege manager metadata.
// The DDLDesc's are referenced by other classes such as NATable and SHOWDDL 
// that save and display metadata contents.  
//
// When DDL operations are performed, the associated DDLDesc structure is 
// flatten out and stored in the text table related to object.  When the
// compiler or DDL subsequently require this information, the flattened DDLDesc 
// is read from the metadata and expanded. This information can then be used 
// by the the different components - such as NATable.
//
// Why are there no initializers for most of these classes? The answer is
// that they are all allocated via the factory function TrafAllocateDDLdesc
// (declared at the end of this file). That function zeroes everything out.
// ****************************************************************************

#include "Platform.h"
#include "NAVersionedObject.h"
#include "charinfo.h"
#include "ComSmallDefs.h"

#define GENHEAP(h)    (h ? (NAMemory*)h : CmpCommon::statementHeap())

enum ConstraintType { UNIQUE_CONSTRAINT, PRIMARY_KEY_CONSTRAINT, REF_CONSTRAINT,
		      CHECK_CONSTRAINT
		    };

enum desc_nodetype {
  DESC_UNKNOWN_TYPE = 0,
  DESC_CHECK_CONSTRNTS_TYPE,
  DESC_COLUMNS_TYPE,
  DESC_CONSTRNTS_TYPE,
  DESC_CONSTRNT_KEY_COLS_TYPE,
  DESC_FILES_TYPE,
  DESC_HBASE_RANGE_REGION_TYPE,
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
  DESC_ROUTINE_TYPE,
  DESC_LIBRARY_TYPE,
  DESC_PRIV_TYPE,
  DESC_PRIV_GRANTEE_TYPE,
  DESC_PRIV_BITMAP_TYPE
};

class TrafDesc;
typedef NAVersionedObjectPtrTempl<TrafDesc> DescStructPtr;

class TrafCheckConstrntsDesc;
class TrafColumnsDesc;
class TrafConstrntsDesc;
class TrafConstrntKeyColsDesc;
class TrafHbaseRegionDesc;
class TrafHistogramDesc;
class TrafHistIntervalDesc;
class TrafFilesDesc;
class TrafKeysDesc;
class TrafIndexesDesc;
class TrafLibraryDesc;
class TrafPartnsDesc;
class TrafRefConstrntsDesc;
class TrafRoutineDesc;
class TrafSequenceGeneratorDesc;
class TrafTableDesc;
class TrafUsingMvDesc;
class TrafViewDesc;
class TrafPrivDesc;
class TrafPrivGranteeDesc;
class TrafPrivBitmapDesc;

class TrafDesc : public NAVersionedObject {
public:
  enum {CURR_VERSION = 1};

  TrafDesc(UInt16 nodeType);
  TrafDesc() : NAVersionedObject(-1) {}

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(TrafDesc); }

  virtual Lng32 migrateToNewVersion(NAVersionedObject *&newImage);

  virtual char *findVTblPtr(short classID);

  virtual Long pack(void *space);
  virtual Lng32 unpack(void * base, void * reallocator);

  Lng32 validateSize();
  Lng32 validateVersion();
 
  UInt16 nodetype;
  UInt16 version;
  UInt32 descFlags;
  DescStructPtr next;
  char* descExtension; // extension of descriptor, if it needs to be extended

  virtual TrafCheckConstrntsDesc *checkConstrntsDesc() const { return NULL; }
  virtual TrafColumnsDesc *columnsDesc() const { return NULL; }
  virtual TrafConstrntsDesc *constrntsDesc() const { return NULL; }
  virtual TrafConstrntKeyColsDesc *constrntKeyColsDesc() const { return NULL; }
  virtual TrafFilesDesc *filesDesc() const { return NULL; }
  virtual TrafHbaseRegionDesc *hbaseRegionDesc() const { return NULL; }
  virtual TrafHistogramDesc *histogramDesc() const { return NULL; }
  virtual TrafHistIntervalDesc *histIntervalDesc() const { return NULL; }
  virtual TrafKeysDesc *keysDesc() const { return NULL; }
  virtual TrafIndexesDesc *indexesDesc() const { return NULL; }
  virtual TrafLibraryDesc *libraryDesc() const { return NULL; }
  virtual TrafPartnsDesc *partnsDesc() const { return NULL; }
  virtual TrafRefConstrntsDesc *refConstrntsDesc() const { return NULL; }
  virtual TrafRoutineDesc *routineDesc() const { return NULL; }
  virtual TrafSequenceGeneratorDesc *sequenceGeneratorDesc() const { return NULL; }
  virtual TrafTableDesc *tableDesc() const { return NULL; }
  virtual TrafUsingMvDesc *usingMvDesc() const { return NULL; }
  virtual TrafViewDesc *viewDesc() const { return NULL; }
  virtual TrafPrivDesc *privDesc() const { return NULL; }
  virtual TrafPrivGranteeDesc *privGranteeDesc() const { return NULL; }
  virtual TrafPrivBitmapDesc *privBitmapDesc() const { return NULL; }

};

class TrafCheckConstrntsDesc : public TrafDesc {
public:
  // why almost no initializers? see note at top of file
  TrafCheckConstrntsDesc() : TrafDesc(DESC_CHECK_CONSTRNTS_TYPE)
  {}

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(TrafCheckConstrntsDesc); }
 
  virtual Long pack(void *space);
  virtual Lng32 unpack(void * base, void * reallocator);

  virtual TrafCheckConstrntsDesc *checkConstrntsDesc() const { return (TrafCheckConstrntsDesc*)this; }

  char* constrnt_text;
  char filler[16];
};

class TrafColumnsDesc : public TrafDesc {
public:
  // why almost no initializers? see note at top of file
  TrafColumnsDesc() : TrafDesc(DESC_COLUMNS_TYPE) 
  {};

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(TrafColumnsDesc); }
 
  virtual Long pack(void *space);
  virtual Lng32 unpack(void * base, void * reallocator);

  virtual TrafColumnsDesc *columnsDesc() const { return (TrafColumnsDesc*)this; }

  enum ColumnsDescFlags
    { 
      NULLABLE           = 0x0001,  
      ADDED              = 0x0002,
      UPSHIFTED          = 0x0004,
      CASEINSENSITIVE    = 0x0008,
      OPTIONAL           = 0x0010
    };

  void setNullable(NABoolean v) 
  {(v ? columnsDescFlags |= NULLABLE : columnsDescFlags &= ~NULLABLE); };
  NABoolean isNullable() { return (columnsDescFlags & NULLABLE) != 0; };

  void setAdded(NABoolean v) 
  {(v ? columnsDescFlags |= ADDED : columnsDescFlags &= ~ADDED); };
  NABoolean isAdded() { return (columnsDescFlags & ADDED) != 0; };

  void setUpshifted(NABoolean v) 
  {(v ? columnsDescFlags |= UPSHIFTED : columnsDescFlags &= ~UPSHIFTED); };
  NABoolean isUpshifted() { return (columnsDescFlags & UPSHIFTED) != 0; };

  void setCaseInsensitive(NABoolean v) 
  {(v ? columnsDescFlags |= CASEINSENSITIVE : columnsDescFlags &= ~CASEINSENSITIVE); };
  NABoolean isCaseInsensitive() { return (columnsDescFlags & CASEINSENSITIVE) != 0; };

  void setOptional(NABoolean v) 
  {(v ? columnsDescFlags |= OPTIONAL : columnsDescFlags &= ~OPTIONAL); };
  NABoolean isOptional() { return (columnsDescFlags & OPTIONAL) != 0; };

  rec_datetime_field datetimeStart() 
  { return (rec_datetime_field)datetimestart;}
  rec_datetime_field datetimeEnd() 
  { return (rec_datetime_field)datetimeend;}

  ComColumnDefaultClass defaultClass() 
  { return (ComColumnDefaultClass)defaultClass_;}
  void setDefaultClass(ComColumnDefaultClass v)
  { defaultClass_ = (Int16)v;}

  CharInfo::CharSet characterSet() 
  { return (CharInfo::CharSet)character_set;}
  CharInfo::CharSet encodingCharset() 
  { return (CharInfo::CharSet)encoding_charset;}
  CharInfo::Collation  collationSequence()
  {return (CharInfo::Collation)collation_sequence; }

  ComParamDirection paramDirection() 
  { return (ComParamDirection)paramDirection_;}
  void setParamDirection(ComParamDirection v)
  {paramDirection_ = (Int16)v; }

  char* colname;

  Int32 colnumber;
  Int32 datatype;

  Int32 offset;
  Lng32 length;

  Lng32 scale;
  Lng32 precision;

  Int16/*rec_datetime_field*/ datetimestart, datetimeend;
  Int16 datetimefractprec, intervalleadingprec;

  Int16/*ComColumnDefaultClass*/ defaultClass_;
  Int16/*CharInfo::CharSet*/     character_set;
  Int16/*CharInfo::CharSet*/     encoding_charset;
  Int16/*CharInfo::Collation*/   collation_sequence;

  ULng32 hbaseColFlags;
  Int16/*ComParamDirection*/ paramDirection_;
  char colclass; // 'S' -- system generated, 'U' -- user created
  char filler0;

  Int64 colFlags;
  Int64 columnsDescFlags; // my flags

  char* pictureText;
  char* defaultvalue;
  char* heading;
  char* computed_column_text;
  char* hbaseColFam;
  char* hbaseColQual;

  char filler[24];
};

class TrafConstrntKeyColsDesc : public TrafDesc {
public:
  enum ConsrntKeyDescFlags
    { 
      SYSTEM_KEY   = 0x0001
    };
  // why almost no initializers? see note at top of file
  TrafConstrntKeyColsDesc() : TrafDesc(DESC_CONSTRNT_KEY_COLS_TYPE)
  {
    constrntKeyColsDescFlags = 0;
  }

  void setSystemKey(NABoolean v) 
  {(v ? constrntKeyColsDescFlags |= SYSTEM_KEY: constrntKeyColsDescFlags&= ~SYSTEM_KEY); };
  NABoolean isSystemKey() { return (constrntKeyColsDescFlags & SYSTEM_KEY) != 0; };

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(TrafConstrntKeyColsDesc); }
 
  virtual Long pack(void *space);
  virtual Lng32 unpack(void * base, void * reallocator);

  virtual TrafConstrntKeyColsDesc *constrntKeyColsDesc() const { return (TrafConstrntKeyColsDesc*)this; }

  char* colname;
  Int32  position;

  Int64 constrntKeyColsDescFlags; // my flags

  char filler[16];
};

class TrafConstrntsDesc : public TrafDesc {
public:
  // why almost no initializers? see note at top of file
  TrafConstrntsDesc() : TrafDesc(DESC_CONSTRNTS_TYPE)
  {}

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(TrafConstrntsDesc); }
 
  virtual Long pack(void *space);
  virtual Lng32 unpack(void * base, void * reallocator);

  virtual TrafConstrntsDesc *constrntsDesc() const { return (TrafConstrntsDesc*)this; }

  enum ConstrntsDescFlags
    { 
      ENFORCED           = 0x0001,
      NOT_SERIALIZED     = 0x0002
    };

  void setEnforced(NABoolean v) 
  {(v ? constrntsDescFlags |= ENFORCED : constrntsDescFlags &= ~ENFORCED); };
  NABoolean isEnforced() { return (constrntsDescFlags & ENFORCED) != 0; };

  void setNotSerialized(NABoolean v) 
  {(v ? constrntsDescFlags |= NOT_SERIALIZED : constrntsDescFlags &= ~NOT_SERIALIZED); };
  NABoolean notSerialized() { return (constrntsDescFlags & NOT_SERIALIZED) != 0; };

  char* constrntname;
  char* tablename;

  Int16 /*ConstraintType*/ type;
  Int16 fillerInt16;
  Int32  colcount;

  Int64 constrntsDescFlags; // my flags

  DescStructPtr check_constrnts_desc;
  DescStructPtr constr_key_cols_desc;
  DescStructPtr referenced_constrnts_desc;
  DescStructPtr referencing_constrnts_desc;

  char filler[24];
};

class TrafFilesDesc : public TrafDesc {
public:
  // why almost no initializers? see note at top of file
  TrafFilesDesc() : TrafDesc(DESC_FILES_TYPE)
  {}

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(TrafFilesDesc); }
 
  virtual Long pack(void *space);
  virtual Lng32 unpack(void * base, void * reallocator);

  virtual TrafFilesDesc *filesDesc() const { return (TrafFilesDesc*)this; }

  enum FilesDescFlags
    { 
      AUDITED           = 0x0001
    };

  void setAudited(NABoolean v) 
  {(v ? filesDescFlags |= AUDITED : filesDescFlags &= ~AUDITED); };
  NABoolean isAudited() { return (filesDescFlags & AUDITED) != 0; };

  Int64 filesDescFlags; // my flags
  DescStructPtr partns_desc;
};

class TrafHbaseRegionDesc : public TrafDesc {
public:
  // why almost no initializers? see note at top of file
  TrafHbaseRegionDesc() : TrafDesc(DESC_HBASE_RANGE_REGION_TYPE)
  {}

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(TrafHbaseRegionDesc); }
 
  virtual Long pack(void *space);
  virtual Lng32 unpack(void * base, void * reallocator);

  virtual TrafHbaseRegionDesc *hbaseRegionDesc() const { return (TrafHbaseRegionDesc*)this; }

  Int64  hbaseRegionDescFlags; // my flags

  Lng32  beginKeyLen;
  Lng32  endKeyLen;

  char*  beginKey;
  char*  endKey;

  char filler[16];
};

class TrafHistogramDesc : public TrafDesc {
public:
  // why almost no initializers? see note at top of file
  TrafHistogramDesc() : TrafDesc(DESC_HISTOGRAM_TYPE)
  {}

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(TrafHistogramDesc); }
 
  virtual Long pack(void *space);
  virtual Lng32 unpack(void * base, void * reallocator);

  virtual TrafHistogramDesc *histogramDesc() const { return (TrafHistogramDesc*)this; }

  char*   tablename;
  char*   histid;

  Int32   tablecolnumber;
  Int32   colposition;

  Float32 rowcount;
  Float32 uec;

  char*   highval;
  char*   lowval;

  Int64 histogramDescFlags; // my flags

  DescStructPtr hist_interval_desc;

  char filler[24];
};

class TrafHistIntervalDesc : public TrafDesc {
public:
  // why almost no initializers? see note at top of file
  TrafHistIntervalDesc() : TrafDesc(DESC_HIST_INTERVAL_TYPE)
  {}

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(TrafHistIntervalDesc); }
 
  virtual Long pack(void *space);
  virtual Lng32 unpack(void * base, void * reallocator);

  virtual TrafHistIntervalDesc *histIntervalDesc() const { return (TrafHistIntervalDesc*)this; }

  char*   histid;
  char*   intboundary;

  Float32  rowcount;
  Float32  uec;

  Int32   intnum;
  Int32   fillerInt32;

  Int64 histIntervalDescFlags; // my flags

  char filler[16];
};

class TrafIndexesDesc : public TrafDesc {
public:
  // why almost no initializers? see note at top of file
  TrafIndexesDesc() : TrafDesc(DESC_INDEXES_TYPE)
  {}

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(TrafIndexesDesc); }
 
  virtual Long pack(void *space);
  virtual Lng32 unpack(void * base, void * reallocator);

  virtual TrafIndexesDesc *indexesDesc() const { return (TrafIndexesDesc*)this; }

  enum IndexesDescFlags
    { 
      SYSTEM_TABLE_CODE = 0x0001,  
      EXPLICIT          = 0x0002,
      VOLATILE          = 0x0004,
      IN_MEM_OBJ        = 0x0008,
      UNIQUE            = 0x0010
    };

  void setSystemTableCode(NABoolean v) 
  {(v ? indexesDescFlags |= SYSTEM_TABLE_CODE : indexesDescFlags &= ~SYSTEM_TABLE_CODE); };
  NABoolean isSystemTableCode() { return (indexesDescFlags & SYSTEM_TABLE_CODE) != 0; };

  void setExplicit(NABoolean v) 
  {(v ? indexesDescFlags |= EXPLICIT : indexesDescFlags &= ~EXPLICIT); };
  NABoolean isExplicit() { return (indexesDescFlags & EXPLICIT) != 0; };

  void setVolatile(NABoolean v) 
  {(v ? indexesDescFlags |= VOLATILE : indexesDescFlags &= ~VOLATILE); };
  NABoolean isVolatile() { return (indexesDescFlags & VOLATILE) != 0; };

  void setInMemoryObject(NABoolean v) 
  {(v ? indexesDescFlags |= IN_MEM_OBJ : indexesDescFlags &= ~IN_MEM_OBJ); };
  NABoolean isInMemoryObject() { return (indexesDescFlags & IN_MEM_OBJ) != 0; };

  void setUnique(NABoolean v) 
  {(v ? indexesDescFlags |= UNIQUE : indexesDescFlags &= ~UNIQUE); };
  NABoolean isUnique() { return (indexesDescFlags & UNIQUE) != 0; };

  ComPartitioningScheme partitioningScheme() 
  { return (ComPartitioningScheme)partitioningScheme_; }
  void setPartitioningScheme(ComPartitioningScheme v) 
  { partitioningScheme_ = (Int16)v; }
  ComRowFormat rowFormat() { return (ComRowFormat)rowFormat_; }
  void setRowFormat(ComRowFormat v) { rowFormat_ = (Int16)v; }

  char* tablename;  // name of the base table
  char* indexname;  // physical name of index. Different from ext_indexname
                    // for ARK tables.
  Int64 indexUID;

  Int32 keytag;
  Int32 record_length;
  Int32 colcount;
  Int32 blocksize;

  Int16 /*ComPartitioningScheme*/ partitioningScheme_; 
  Int16 /*ComRowFormat*/          rowFormat_;
  Lng32 numSaltPartns; // number of salted partns created for a seabase table.

  Lng32 numInitialSaltRegions; // initial # of regions created for salted table
  char filler0[4];

  Int64 indexesDescFlags; // my flags

  char*  hbaseSplitClause;
  char*  hbaseCreateOptions;

  DescStructPtr files_desc;

  // Clustering keys columns
  DescStructPtr keys_desc;

  // Columns that are not part of the clustering key.
  // Used specially for vertical partition column(s).
  DescStructPtr non_keys_desc;

  // for hbase's region keys
  DescStructPtr hbase_regionkey_desc;

  char filler[16];
};

class TrafKeysDesc : public TrafDesc {
public:
  // why almost no initializers? see note at top of file
  TrafKeysDesc() : TrafDesc(DESC_KEYS_TYPE)
  {}

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(TrafKeysDesc); }
 
  virtual Long pack(void *space);
  virtual Lng32 unpack(void * base, void * reallocator);

  virtual TrafKeysDesc *keysDesc() const { return (TrafKeysDesc*)this; }

  enum KeysDescFlags
    { 
      DESCENDING           = 0x0001
    };

  void setDescending(NABoolean v) 
  {(v ? keysDescFlags |= DESCENDING : keysDescFlags &= ~DESCENDING); };
  NABoolean isDescending() { return (keysDescFlags & DESCENDING) != 0; };

  char* keyname;
  Int32 keyseqnumber;
  Int32 tablecolnumber;

  Int64 keysDescFlags; // my flags

  char* hbaseColFam;
  char* hbaseColQual;

  char filler[16];
};

class TrafLibraryDesc : public TrafDesc {
public:
  // why almost no initializers? see note at top of file
  TrafLibraryDesc() : TrafDesc(DESC_LIBRARY_TYPE)
  {}

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(TrafLibraryDesc); }
 
  virtual Long pack(void *space);
  virtual Lng32 unpack(void * base, void * reallocator);

  virtual TrafLibraryDesc *libraryDesc() const { return (TrafLibraryDesc*)this; }

  char* libraryName;
  char* libraryFilename;
  Int64 libraryUID;
  Int32 libraryVersion;
  Int32 libraryOwnerID;
  Int32 librarySchemaOwnerID;

  char filler[20];
};

class TrafPartnsDesc : public TrafDesc {
public:
  // why almost no initializers? see note at top of file
  TrafPartnsDesc() : TrafDesc(DESC_PARTNS_TYPE)
  {}

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(TrafPartnsDesc); }
 
  virtual Long pack(void *space);
  virtual Lng32 unpack(void * base, void * reallocator);

  virtual TrafPartnsDesc *partnsDesc() const { return (TrafPartnsDesc*)this; }

  char*  tablename;
  Int32 primarypartition;
  char*  partitionname;
  char*  logicalpartitionname;
  char*  firstkey;
  Lng32 firstkeylen;         //soln:10-031112-1256
  Lng32 encodedkeylen;
  char*  encodedkey;
  char*  lowKey;
  char*  highKey;
  Int32    indexlevel;
  Int32  priExt;
  Int32  secExt;
  Int32  maxExt;
  char*  givenname;
};

class TrafRefConstrntsDesc : public TrafDesc {
public:
  // why almost no initializers? see note at top of file
  TrafRefConstrntsDesc() : TrafDesc(DESC_REF_CONSTRNTS_TYPE)
  {}

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(TrafRefConstrntsDesc); }
 
  virtual Long pack(void *space);
  virtual Lng32 unpack(void * base, void * reallocator);

  virtual TrafRefConstrntsDesc *refConstrntsDesc() const { return (TrafRefConstrntsDesc*)this; }

  Int64 refConstrntsDescFlags; // my flags
  char* constrntname;
  char* tablename;

  char filler[16];
};

class TrafRoutineDesc : public TrafDesc {
public:
  // why almost no initializers? see note at top of file
  TrafRoutineDesc() : TrafDesc(DESC_ROUTINE_TYPE)
  {}

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(TrafRoutineDesc); }
 
  virtual Long pack(void *space);
  virtual Lng32 unpack(void * base, void * reallocator);

  virtual TrafRoutineDesc *routineDesc() const { return (TrafRoutineDesc*)this; }

  Int64 objectUID;
  char* routineName;
  char* externalName;
  char* librarySqlName;
  char* libraryFileName;
  char* signature;
  Int32 paramsCount;
  DescStructPtr params;
  ComRoutineLanguage language;
  ComRoutineType UDRType;
  ComRoutineSQLAccess sqlAccess;
  ComRoutineTransactionAttributes transactionAttributes;
  Int32 maxResults;
  ComRoutineParamStyle paramStyle;
  NABoolean isDeterministic;
  NABoolean isCallOnNull;
  NABoolean isIsolate;
  ComRoutineExternalSecurity externalSecurity;
  ComRoutineExecutionMode executionMode;
  Int32 stateAreaSize;
  ComRoutineParallelism parallelism;
  Int32 owner;
  Int32 schemaOwner;
  DescStructPtr priv_desc;

  Int64 routineDescFlags; // my flags

  char filler[24];
};

class TrafSequenceGeneratorDesc : public TrafDesc {
public:
  // why almost no initializers? see note at top of file
  TrafSequenceGeneratorDesc() : TrafDesc(DESC_SEQUENCE_GENERATOR_TYPE)
  {}

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(TrafSequenceGeneratorDesc); }
 
  virtual Long pack(void *space);
  virtual Lng32 unpack(void * base, void * reallocator);

  virtual TrafSequenceGeneratorDesc *sequenceGeneratorDesc() const { return (TrafSequenceGeneratorDesc*)this; }

  ComSequenceGeneratorType sgType() 
  { return (ComSequenceGeneratorType)sgType_; }
  void setSgType(ComSequenceGeneratorType v) 
  { sgType_ = (Int16)v; }

  Int64                     startValue;
  Int64                     increment;

  Int16 /*ComSequenceGeneratorType*/  sgType_;
  Int16 /*ComSQLDataType*/  sqlDataType;
  Int16 /*ComFSDataType*/   fsDataType;
  Int16                     cycleOption;

  Int64                     maxValue;
  Int64                     minValue;
  Int64                     cache;
  Int64                     objectUID;
  char*                     sgLocation;
  Int64                     nextValue;
  Int64                     redefTime;

  Int64 sequenceGeneratorDescFlags; // my flags

  char filler[16];
};

class TrafTableDesc : public TrafDesc {
public:
  // why almost no initializers? see note at top of file
  TrafTableDesc() : TrafDesc(DESC_TABLE_TYPE)
  {}

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(TrafTableDesc); }
 
  virtual Long pack(void *space);
  virtual Lng32 unpack(void * base, void * reallocator);

  virtual TrafTableDesc *tableDesc() const { return (TrafTableDesc*)this; }

  enum TableDescFlags
    { 
      SYSTEM_TABLE_CODE = 0x0001,  
      UMD_TABLE         = 0x0002,
      MV_TABLE          = 0x0004,
      IUD_LOG           = 0x0008,
      MV_MD_OBJECT      = 0x0010,
      SYN_TRANS_DONE    = 0x0020,
      VOLATILE          = 0x0040,
      IN_MEM_OBJ        = 0x0080,
      DROPPABLE         = 0x0100,
      INSERT_ONLY       = 0x0200
    };

  void setSystemTableCode(NABoolean v) 
  {(v ? tableDescFlags |= SYSTEM_TABLE_CODE : tableDescFlags &= ~SYSTEM_TABLE_CODE); };
  NABoolean isSystemTableCode() { return (tableDescFlags & SYSTEM_TABLE_CODE) != 0; };

  void setUMDTable(NABoolean v) 
  {(v ? tableDescFlags |= UMD_TABLE : tableDescFlags &= ~UMD_TABLE); };
  NABoolean isUMDTable() { return (tableDescFlags & UMD_TABLE) != 0; };

  void setMVTable(NABoolean v) 
  {(v ? tableDescFlags |= MV_TABLE : tableDescFlags &= ~MV_TABLE); };
  NABoolean isMVTable() { return (tableDescFlags & MV_TABLE) != 0; };

  void setIUDLog(NABoolean v) 
  {(v ? tableDescFlags |= IUD_LOG : tableDescFlags &= ~IUD_LOG); };
  NABoolean isIUDLog() { return (tableDescFlags & IUD_LOG) != 0; };

  void setMVMetadataObject(NABoolean v) 
  {(v ? tableDescFlags |= MV_MD_OBJECT : tableDescFlags &= ~MV_MD_OBJECT); };
  NABoolean isMVMetadataObject() { return (tableDescFlags & MV_MD_OBJECT) != 0; };

  void setSynonymTranslationDone(NABoolean v) 
  {(v ? tableDescFlags |= SYN_TRANS_DONE : tableDescFlags &= ~SYN_TRANS_DONE); };
  NABoolean isSynonymTranslationDone() { return (tableDescFlags & SYN_TRANS_DONE) != 0; };

  void setVolatileTable(NABoolean v) 
  {(v ? tableDescFlags |= VOLATILE : tableDescFlags &= ~VOLATILE); };
  NABoolean isVolatileTable() { return (tableDescFlags & VOLATILE) != 0; };

  void setInMemoryObject(NABoolean v) 
  {(v ? tableDescFlags |= IN_MEM_OBJ : tableDescFlags &= ~IN_MEM_OBJ); };
  NABoolean isInMemoryObject() { return (tableDescFlags & IN_MEM_OBJ) != 0; };

  void setDroppable(NABoolean v) 
  {(v ? tableDescFlags |= DROPPABLE : tableDescFlags &= ~DROPPABLE); };
  NABoolean isDroppable() { return (tableDescFlags & DROPPABLE) != 0; };

  void setInsertOnly(NABoolean v) 
  {(v ? tableDescFlags |= INSERT_ONLY : tableDescFlags &= ~INSERT_ONLY); };
  NABoolean isInsertOnly() { return (tableDescFlags & INSERT_ONLY) != 0; };

  ComInsertMode insertMode() { return (ComInsertMode)insertMode_;}
  void setInsertMode(ComInsertMode v) {insertMode_ = (Int16)v;}

  ComPartitioningScheme partitioningScheme() 
  { return (ComPartitioningScheme)partitioningScheme_; }
  void setPartitioningScheme(ComPartitioningScheme v) 
  { partitioningScheme_ = (Int16)v; }
  ComRowFormat rowFormat() { return (ComRowFormat)rowFormat_; }
  void setRowFormat(ComRowFormat v) { rowFormat_ = (Int16)v; }
  ComObjectType objectType() { return (ComObjectType)objectType_; }
  void setObjectType(ComObjectType v) { objectType_ = (Int16)v; }

  char* tablename;

  Int64 createTime;
  Int64 redefTime;
  Int64 cacheTime;

  Int32 mvAttributesBitmap;
  Int32 record_length;
  Int32 colcount;
  Int32 constr_count;

  Int16 /*ComInsertMode*/ insertMode_;
  Int16 /*ComPartitioningScheme*/ partitioningScheme_; 
  Int16 /*ComRowFormat*/  rowFormat_;
  Int16 /*ComObjectType*/ objectType_; 

  // next 8 bytes are fillers for future usage
  Int16 /*ComStorageType*/ storageType_;
  char filler0[6];

  Int64 catUID;
  Int64 schemaUID;
  Int64 objectUID;

  Lng32 owner;
  Lng32 schemaOwner;

  char*  snapshotName;
  char*  default_col_fam;
  char*  all_col_fams;
  Int64 objectFlags;
  Int64 tablesFlags;

  Int64 tableDescFlags; // my flags

  DescStructPtr columns_desc;
  DescStructPtr indexes_desc;
  DescStructPtr constrnts_desc;
  DescStructPtr views_desc;
  DescStructPtr constrnts_tables_desc;
  DescStructPtr referenced_tables_desc;
  DescStructPtr referencing_tables_desc;
  DescStructPtr histograms_desc;
  DescStructPtr files_desc;
  DescStructPtr priv_desc;

  // for hbase's region keys
  DescStructPtr hbase_regionkey_desc;
  DescStructPtr sequence_generator_desc;

  char filler[32];
};

class TrafUsingMvDesc : public TrafDesc {
public:
  // why almost no initializers? see note at top of file
  TrafUsingMvDesc() : TrafDesc(DESC_USING_MV_TYPE)
  {}

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(TrafUsingMvDesc); }
 
  virtual Long pack(void *space);
  virtual Lng32 unpack(void * base, void * reallocator);

  virtual TrafUsingMvDesc *usingMvDesc() const { return (TrafUsingMvDesc*)this; }

  ComMVRefreshType refreshType() { return (ComMVRefreshType)refreshType_;}
  void setRefreshType(ComMVRefreshType v) 
  { refreshType_ = (Int16)v;}

  char* mvName;
  Int32  rewriteEnabled;
  Int32  isInitialized;
  Int16 /*ComMVRefreshType*/ refreshType_; // unknown here means "non incremental"

  char filler[14];
};

class TrafViewDesc : public TrafDesc {
public:
  // why almost no initializers? see note at top of file
  TrafViewDesc() : TrafDesc(DESC_VIEW_TYPE)
  {}

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(TrafViewDesc); }

  virtual Long pack(void *space);
  virtual Lng32 unpack(void * base, void * reallocator);

  virtual TrafViewDesc *viewDesc() const { return (TrafViewDesc*)this; }

  enum ViewDescFlags
    {
      UPDATABLE           = 0x0001,
      INSERTABLE          = 0x0002
    };

  void setUpdatable(NABoolean v)
  {(v ? viewDescFlags |= UPDATABLE : viewDescFlags &= ~UPDATABLE); };
  NABoolean isUpdatable() { return (viewDescFlags & UPDATABLE) != 0; };

  void setInsertable(NABoolean v)
  {(v ? viewDescFlags |= INSERTABLE : viewDescFlags &= ~INSERTABLE); };
  NABoolean isInsertable() { return (viewDescFlags & INSERTABLE) != 0; };

  char*  viewname;
  char*  viewfilename;    // the physical file, to be Opened for auth-cking.
  char*  viewtext;
  char*  viewchecktext;
  char*  viewcolusages;

  Int64 viewDescFlags; // my flags

  Int16 /*CharInfo::CharSet*/ viewtextcharset;

  char filler[22];
};

// --------------------------- privilege descriptors ---------------------------
// The privilege descriptors are organized as follows:
//   privDesc - a TrafPrivDesc containing the privGrantees
//   privGrantees - a TrafPrivGranteeDesc descriptor containing:
//     grantee - int32 value for each user granted a privilege directly or
//               through a role granted to the user
//     objectBitmap  - a TrafPrivBitmapDesc containing granted object privs
//                     summarized across all grantors
//     columnBitmaps - list of TrafPrivBitmapDesc, one per colummn, containing 
//                     granted column privs, summarized across all grantors
//   priv_bits desc - a TrafPrivBitmapDesc descriptor containing:
//     privBitmap - bitmap containing granted privs such as SELECT
//     privWGO - bitmap containing WGO for associated grant (privBitmap)
//     columnOrdinal - column number for bitmap, for objects, column
//                     number is not relavent so it is set to -1.
//   column bits - list of TrafPrivBitmapDesc
class TrafPrivDesc : public TrafDesc {
public:
  // why almost no initializers? see note at top of file
  TrafPrivDesc() : TrafDesc(DESC_PRIV_TYPE)
  {}

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(TrafPrivDesc); }

  virtual Long pack(void *space);
  virtual Lng32 unpack(void * base, void * reallocator);

  virtual TrafPrivDesc *privDesc() const { return (TrafPrivDesc*)this; }

  DescStructPtr privGrantees;
  char filler[16];
};

class TrafPrivGranteeDesc : public TrafDesc {
public:
  // why almost no initializers? see note at top of file
  TrafPrivGranteeDesc() : TrafDesc(DESC_PRIV_GRANTEE_TYPE)
  {}

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(TrafPrivGranteeDesc); }

  virtual Long pack(void *space);
  virtual Lng32 unpack(void * base, void * reallocator);

  virtual TrafPrivGranteeDesc *privGranteeDesc() const { return (TrafPrivGranteeDesc*)this; }

  Int32 grantee;
  DescStructPtr objectBitmap;
  DescStructPtr columnBitmaps;
  char filler[20];
};

class TrafPrivBitmapDesc : public TrafDesc {
public:
  // why almost no initializers? see note at top of file
  TrafPrivBitmapDesc() : TrafDesc(DESC_PRIV_BITMAP_TYPE)
  {}

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()      { return (short)sizeof(TrafPrivBitmapDesc); }

  //virtual Long pack(void *space);
  //virtual Lng32 unpack(void * base, void * reallocator);

  virtual TrafPrivBitmapDesc *privBitmapDesc() const { return (TrafPrivBitmapDesc*)this; }

  Int32  columnOrdinal;
  Int64  privBitmap;
  Int64  privWGOBitmap;
  char filler[20];
};
// ------------------------- end privilege descriptors -------------------------

// If "space" is passed in, use it. (It might be an NAHeap or a ComSpace.)
// If "space" is null, use the statement heap.
TrafDesc *TrafAllocateDDLdesc(desc_nodetype nodetype, 
                              NAMemory * space);

TrafDesc *TrafMakeColumnDesc
(
     const char *tablename,
     const char *colname,
     Lng32 &colnumber,		  // INOUT
     Int32 datatype,
     Lng32 length,
     Lng32 &offset,		  // INOUT
     NABoolean null_flag,
     SQLCHARSET_CODE datacharset, // i.e., use CharInfo::DefaultCharSet;
     NAMemory * space
 );

#endif
