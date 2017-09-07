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

#include "TrafDDLdesc.h"
#include "CmpCommon.h"
#include "SQLTypeDefs.h"

// -----------------------------------------------------------------------
// Allocate a column_desc and do simple initialization of several fields,
// based on what's passed in.  Many of the fields we just default,
// to either hardcoded values or to zero.  The callers,
// in arkcmplib + generator + optimizer, can set additional fields afterwards.
// -----------------------------------------------------------------------
TrafDesc *TrafMakeColumnDesc(const char *tablename,
                             const char *colname,
                             Lng32 &colnumber,	// INOUT
                             Int32 datatype,
                             Lng32 length,
                             Lng32 &offset,	// INOUT
                             NABoolean null_flag,
                             SQLCHARSET_CODE datacharset,
                             NAMemory * space
                             )
{
  #undef  COLUMN
  #define COLUMN returnDesc->columnsDesc()

  // Pass in the optional "passedDesc" if you just want to overwrite an
  // already existing desc.
  TrafDesc *returnDesc = 
    TrafAllocateDDLdesc(DESC_COLUMNS_TYPE, space);

  COLUMN->colname = (char *)colname;

  COLUMN->colnumber = colnumber;
  COLUMN->datatype = datatype;
  COLUMN->length = length;
  COLUMN->offset = offset;
  COLUMN->setNullable(null_flag);

  // Hardcode some fields here.
  // All other fields (scale, precision, etc) default to zero!

  COLUMN->colclass = 'U';
  COLUMN->setDefaultClass(COM_NO_DEFAULT);

  if (DFS2REC::isAnyCharacter(datatype)) {
    if (datacharset == SQLCHARSETCODE_UNKNOWN) {
    COLUMN->character_set      = CharInfo::DefaultCharSet;
    COLUMN->encoding_charset   = CharInfo::DefaultCharSet;
    }
    else {
    COLUMN->character_set      = (CharInfo::CharSet)datacharset;
    COLUMN->encoding_charset   = (CharInfo::CharSet)datacharset;
    }
    COLUMN->collation_sequence = CharInfo::DefaultCollation;
    if (DFS2REC::isSQLVarChar(datatype))
      offset += SQL_VARCHAR_HDR_SIZE;
  }
  else {	// datetime, interval, numeric, etc.
    COLUMN->datetimestart = COLUMN->datetimeend = REC_DATE_UNKNOWN;
  }

  colnumber++;

  offset += length;
  if (null_flag) offset += SQL_NULL_HDR_SIZE;

  return returnDesc;
}

// -----------------------------------------------------------------------
// Allocate one of the primitive structs and initialize to all zeroes.
// Uses HEAP (StatementHeap) of CmpCommon or space.
// -----------------------------------------------------------------------
TrafDesc *TrafAllocateDDLdesc(desc_nodetype nodetype, NAMemory * space)
{
  size_t size = 0;
  TrafDesc * desc_ptr = NULL;

  switch (nodetype)
    {
    case DESC_CHECK_CONSTRNTS_TYPE:
      desc_ptr = new GENHEAP(space) TrafCheckConstrntsDesc();
      break;
    case DESC_COLUMNS_TYPE:
      desc_ptr = new GENHEAP(space) TrafColumnsDesc();
      break;
    case DESC_CONSTRNTS_TYPE:
      desc_ptr = new GENHEAP(space) TrafConstrntsDesc();
      break;
    case DESC_CONSTRNT_KEY_COLS_TYPE:
      desc_ptr = new GENHEAP(space) TrafConstrntKeyColsDesc();
      break;
    case DESC_FILES_TYPE:
      desc_ptr = new GENHEAP(space) TrafFilesDesc();
      break;
    case DESC_HBASE_RANGE_REGION_TYPE:
      desc_ptr = new GENHEAP(space) TrafHbaseRegionDesc();
      break;
    case DESC_HISTOGRAM_TYPE:
      desc_ptr = new GENHEAP(space) TrafHistogramDesc();
      break;
    case DESC_HIST_INTERVAL_TYPE:
      desc_ptr = new GENHEAP(space) TrafHistIntervalDesc();
      break;
    case DESC_INDEXES_TYPE:
      desc_ptr = new GENHEAP(space) TrafIndexesDesc();
      break;
    case DESC_KEYS_TYPE:
      desc_ptr = new GENHEAP(space) TrafKeysDesc();
      break;
    case DESC_LIBRARY_TYPE:
      desc_ptr = new GENHEAP(space) TrafLibraryDesc();
      break;
     case DESC_PARTNS_TYPE:
      desc_ptr = new GENHEAP(space) TrafPartnsDesc();
      break;
    case DESC_REF_CONSTRNTS_TYPE:
      desc_ptr = new GENHEAP(space) TrafRefConstrntsDesc();
      break;
    case DESC_ROUTINE_TYPE:
      desc_ptr = new GENHEAP(space) TrafRoutineDesc();
      break;
    case DESC_SEQUENCE_GENERATOR_TYPE:   
      desc_ptr = new GENHEAP(space) TrafSequenceGeneratorDesc();
      break;
    case DESC_TABLE_TYPE:
      desc_ptr = new GENHEAP(space) TrafTableDesc();
      break;
    case DESC_VIEW_TYPE:
      desc_ptr = new GENHEAP(space) TrafViewDesc();
      break;	       
    case DESC_USING_MV_TYPE: 
      desc_ptr = new GENHEAP(space) TrafUsingMvDesc();
      break;
    case DESC_PRIV_TYPE: 
      desc_ptr = new GENHEAP(space) TrafPrivDesc();
      break;
    case DESC_PRIV_GRANTEE_TYPE: 
      desc_ptr = new GENHEAP(space) TrafPrivGranteeDesc();
      break;
    case DESC_PRIV_BITMAP_TYPE: 
      desc_ptr = new GENHEAP(space) TrafPrivBitmapDesc();
      break;
    default:
      assert(FALSE);
      break;
    }

  // if not being allocated from space, memset all bytes to 0.
  // If allocated from space, it will be set to 0 during space allocation.
  if ((! space) || (!space->isComSpace()))
    memset((char*)desc_ptr+sizeof(TrafDesc), 0, 
           desc_ptr->getClassSize()-sizeof(TrafDesc));

  return desc_ptr;

}

TrafDesc::TrafDesc(UInt16 nodeType) 
  : NAVersionedObject(nodeType),
    nodetype(nodeType),
    version(CURR_VERSION),
    descFlags(0),
    next(NULL)
{}

Lng32 TrafDesc::validateSize()
{
  if (getImageSize() != getClassSize())
    return -1;

  return 0;
}

Lng32 TrafDesc::validateVersion()
{
  if (version != CURR_VERSION)
    return -1;

  return 0;
}

Lng32 TrafDesc::migrateToNewVersion(
     NAVersionedObject *&newImage)
{
  short tempimagesize = getClassSize();
  // -----------------------------------------------------------------
  // The base class implementation of migrateToNewVersion() is only
  // called with newImage == NULL when the same function is not
  // redefined at the subclass. That means no new version of that
  // subclass has been invented yet.
  // -----------------------------------------------------------------
  if (newImage == NULL)
    {
      if (validateSize())
        return -1;

      if (validateVersion())
        return -1;
    }

  return NAVersionedObject::migrateToNewVersion(newImage);
}

char *TrafDesc::findVTblPtr(short classID)
{
  char *vtblptr = NULL;

  switch (classID)
    {
    case DESC_CHECK_CONSTRNTS_TYPE:
      GetVTblPtr(vtblptr, TrafCheckConstrntsDesc);
      break;
    case DESC_COLUMNS_TYPE:
      GetVTblPtr(vtblptr, TrafColumnsDesc);
      break;
    case DESC_CONSTRNTS_TYPE:
      GetVTblPtr(vtblptr, TrafConstrntsDesc);
      break;
    case DESC_CONSTRNT_KEY_COLS_TYPE:
      GetVTblPtr(vtblptr, TrafConstrntKeyColsDesc);
      break;
    case DESC_FILES_TYPE:
      GetVTblPtr(vtblptr, TrafFilesDesc);
      break;
    case DESC_HBASE_RANGE_REGION_TYPE:
      GetVTblPtr(vtblptr, TrafHbaseRegionDesc);
      break;
    case DESC_HISTOGRAM_TYPE:
      GetVTblPtr(vtblptr, TrafHistogramDesc);
      break;
    case DESC_HIST_INTERVAL_TYPE:
      GetVTblPtr(vtblptr, TrafHistIntervalDesc);
      break;
    case DESC_INDEXES_TYPE:
      GetVTblPtr(vtblptr, TrafIndexesDesc);
      break;
    case DESC_KEYS_TYPE:
      GetVTblPtr(vtblptr, TrafKeysDesc);
      break;
    case DESC_LIBRARY_TYPE:
      GetVTblPtr(vtblptr, TrafLibraryDesc);
      break;
     case DESC_PARTNS_TYPE:
      GetVTblPtr(vtblptr, TrafPartnsDesc);
      break;
    case DESC_REF_CONSTRNTS_TYPE:
      GetVTblPtr(vtblptr, TrafRefConstrntsDesc);
      break;
    case DESC_ROUTINE_TYPE:
      GetVTblPtr(vtblptr, TrafRoutineDesc);
      break;
    case DESC_SEQUENCE_GENERATOR_TYPE:   
      GetVTblPtr(vtblptr, TrafSequenceGeneratorDesc);
      break;
    case DESC_TABLE_TYPE:
      GetVTblPtr(vtblptr, TrafTableDesc);
      break;
    case DESC_VIEW_TYPE:
      GetVTblPtr(vtblptr, TrafViewDesc);
      break;	       
    case DESC_USING_MV_TYPE: 
      GetVTblPtr(vtblptr, TrafUsingMvDesc);
      break;
    case DESC_PRIV_TYPE: 
      GetVTblPtr(vtblptr, TrafPrivDesc);
      break;
    case DESC_PRIV_GRANTEE_TYPE: 
      GetVTblPtr(vtblptr, TrafPrivGranteeDesc);
      break;
    case DESC_PRIV_BITMAP_TYPE: 
      GetVTblPtr(vtblptr, TrafPrivBitmapDesc);
      break;
    default:
      assert(FALSE);
      break;
    }

  return vtblptr;
}

// pack and unpack methods for various descriptor structs

Long TrafDesc::pack(void * space)
{
  next.pack(space);

  return NAVersionedObject::pack(space);
}

Lng32 TrafDesc::unpack(void * base, void * reallocator)
{
  if(next.unpack(base, reallocator)) return -1;

  return NAVersionedObject::unpack(base, reallocator);
}

Long TrafCheckConstrntsDesc::pack(void * space)
{
  constrnt_text = (constrnt_text ? (char*)(((Space*)space)->convertToOffset(constrnt_text)) : NULL);

  return TrafDesc::pack(space);
}

Lng32 TrafCheckConstrntsDesc::unpack(void * base, void * reallocator)
{
  constrnt_text = (constrnt_text ? (char*)((char*)base - (Long)constrnt_text) : NULL);

  return TrafDesc::unpack(base, reallocator);
}

Long TrafColumnsDesc::pack(void * space)
{
  colname = (colname ? (char*)(((Space*)space)->convertToOffset(colname)) : NULL);

  pictureText = (pictureText ? (char*)(((Space*)space)->convertToOffset(pictureText)) : NULL);

  defaultvalue = (defaultvalue ? (char*)(((Space*)space)->convertToOffset(defaultvalue)) : NULL);
  heading = (heading ? (char*)(((Space*)space)->convertToOffset(heading)) : NULL);
  computed_column_text = (computed_column_text ? (char*)(((Space*)space)->convertToOffset(computed_column_text)) : NULL);

  hbaseColFam = (hbaseColFam ? (char*)(((Space*)space)->convertToOffset(hbaseColFam)) : NULL);
  hbaseColQual = (hbaseColQual ? (char*)(((Space*)space)->convertToOffset(hbaseColQual)) : NULL);

  return TrafDesc::pack(space);
}

Lng32 TrafColumnsDesc::unpack(void * base, void * reallocator)
{
  colname = (colname ? (char*)((char*)base - (Long)colname) : NULL);

  pictureText = (pictureText ? (char*)((char*)base - (Long)pictureText) : NULL);

  defaultvalue = (defaultvalue ? (char*)((char*)base - (Long)defaultvalue) : NULL);
  heading = (heading ? (char*)((char*)base - (Long)heading) : NULL);
  computed_column_text = (computed_column_text ? (char*)((char*)base - (Long)computed_column_text) : NULL);

  hbaseColFam = (hbaseColFam ? (char*)((char*)base - (Long)hbaseColFam) : NULL);
  hbaseColQual = (hbaseColQual ? (char*)((char*)base - (Long)hbaseColQual) : NULL);

  return TrafDesc::unpack(base, reallocator);
}

Long TrafConstrntsDesc::pack(void * space)
{
  constrntname = (constrntname ? (char*)(((Space*)space)->convertToOffset(constrntname)) : NULL);
  tablename = (tablename ? (char*)(((Space*)space)->convertToOffset(tablename)) : NULL);
  
  check_constrnts_desc.pack(space);
  constr_key_cols_desc.pack(space);
  referenced_constrnts_desc.pack(space);
  referencing_constrnts_desc.pack(space);

  return TrafDesc::pack(space);
}

Lng32 TrafConstrntsDesc::unpack(void * base, void * reallocator)
{
  constrntname = (constrntname ? (char*)((char*)base - (Long)constrntname) : NULL);
  tablename = (tablename ? (char*)((char*)base - (Long)tablename) : NULL);

  if (check_constrnts_desc.unpack(base, reallocator)) return -1;
  if (constr_key_cols_desc.unpack(base, reallocator)) return -1;
  if (referenced_constrnts_desc.unpack(base, reallocator)) return -1;
  if (referencing_constrnts_desc.unpack(base, reallocator)) return -1;

  return TrafDesc::unpack(base, reallocator);
}

Long TrafConstrntKeyColsDesc::pack(void * space)
{
  colname = (colname ? (char*)(((Space*)space)->convertToOffset(colname)) : NULL);

  return TrafDesc::pack(space);
}

Lng32 TrafConstrntKeyColsDesc::unpack(void * base, void * reallocator)
{
  colname = (colname ? (char*)((char*)base - (Long)colname) : NULL);

  return TrafDesc::unpack(base, reallocator);
}


Long TrafFilesDesc::pack(void * space)
{
  partns_desc.pack(space);

  return TrafDesc::pack(space);
}

Lng32 TrafFilesDesc::unpack(void * base, void * reallocator)
{
  if (partns_desc.unpack(base, reallocator)) return -1;

  return TrafDesc::unpack(base, reallocator);
}

Long TrafHbaseRegionDesc::pack(void * space)
{
  beginKey = (beginKey ? (char*)(((Space*)space)->convertToOffset(beginKey)) : NULL);
  endKey = (endKey ? (char*)(((Space*)space)->convertToOffset(endKey)) : NULL);

  return TrafDesc::pack(space);
}

Lng32 TrafHbaseRegionDesc::unpack(void * base, void * reallocator)
{
  beginKey = (beginKey ? (char*)((char*)base - (Long)beginKey) : NULL);
  endKey = (endKey ? (char*)((char*)base - (Long)endKey) : NULL);

  return TrafDesc::unpack(base, reallocator);
}

Long TrafHistogramDesc::pack(void * space)
{
  tablename = (tablename ? (char*)(((Space*)space)->convertToOffset(tablename)) : NULL);
  histid = (histid ? (char*)(((Space*)space)->convertToOffset(histid)) : NULL);
  highval = (highval ? (char*)(((Space*)space)->convertToOffset(highval)) : NULL);
  lowval = (lowval ? (char*)(((Space*)space)->convertToOffset(lowval)) : NULL);

  hist_interval_desc.pack(space);

  return TrafDesc::pack(space);
}

Lng32 TrafHistogramDesc::unpack(void * base, void * reallocator)
{
  tablename = (tablename ? (char*)((char*)base - (Long)tablename) : NULL);
  histid = (histid ? (char*)((char*)base - (Long)histid) : NULL);
  highval = (highval ? (char*)((char*)base - (Long)highval) : NULL);
  lowval = (lowval ? (char*)((char*)base - (Long)lowval) : NULL);

  if (hist_interval_desc.unpack(base, reallocator)) return -1;
  
  return TrafDesc::unpack(base, reallocator);
}

Long TrafHistIntervalDesc::pack(void * space)
{
  histid = (histid ? (char*)(((Space*)space)->convertToOffset(histid)) : NULL);
  intboundary = (intboundary ? (char*)(((Space*)space)->convertToOffset(intboundary)) : NULL);

  return TrafDesc::pack(space);
}

Lng32 TrafHistIntervalDesc::unpack(void * base, void * reallocator)
{
  histid = (histid ? (char*)((char*)base - (Long)histid) : NULL);
  intboundary = (intboundary ? (char*)((char*)base - (Long)intboundary) : NULL);

  return TrafDesc::unpack(base, reallocator);
}

Long TrafIndexesDesc::pack(void * space)
{
  tablename = (tablename ? (char*)(((Space*)space)->convertToOffset(tablename)) : NULL);
  indexname = (indexname ? (char*)(((Space*)space)->convertToOffset(indexname)) : NULL);
  hbaseSplitClause = (hbaseSplitClause ? (char*)(((Space*)space)->convertToOffset(hbaseSplitClause)) : NULL);
  hbaseCreateOptions = (hbaseCreateOptions ? (char*)(((Space*)space)->convertToOffset(hbaseCreateOptions)) : NULL);

  files_desc.pack(space);
  keys_desc.pack(space);
  non_keys_desc.pack(space);
  hbase_regionkey_desc.pack(space);

  return TrafDesc::pack(space);
}

Lng32 TrafIndexesDesc::unpack(void * base, void * reallocator)
{
  tablename = (tablename ? (char*)((char*)base - (Long)tablename) : NULL);
  indexname = (indexname ? (char*)((char*)base - (Long)indexname) : NULL);
  hbaseSplitClause = (hbaseSplitClause ? (char*)((char*)base - (Long)hbaseSplitClause) : NULL);
  hbaseCreateOptions = (hbaseCreateOptions ? (char*)((char*)base - (Long)hbaseCreateOptions) : NULL);

  if (files_desc.unpack(base, reallocator)) return -1;
  if (keys_desc.unpack(base, reallocator)) return -1;
  if (non_keys_desc.unpack(base, reallocator)) return -1;
  if (hbase_regionkey_desc.unpack(base, reallocator)) return -1;

  return TrafDesc::unpack(base, reallocator);
}

Long TrafKeysDesc::pack(void * space)
{
  keyname = (keyname ? (char*)(((Space*)space)->convertToOffset(keyname)) : NULL);
  hbaseColFam = (hbaseColFam ? (char*)(((Space*)space)->convertToOffset(hbaseColFam)) : NULL);
  hbaseColQual = (hbaseColQual ? (char*)(((Space*)space)->convertToOffset(hbaseColQual)) : NULL);

  return TrafDesc::pack(space);
}

Lng32 TrafKeysDesc::unpack(void * base, void * reallocator)
{
  keyname = (keyname ? (char*)((char*)base - (Long)keyname) : NULL);
  hbaseColFam = (hbaseColFam ? (char*)((char*)base - (Long)hbaseColFam) : NULL);
  hbaseColQual = (hbaseColQual ? (char*)((char*)base - (Long)hbaseColQual) : NULL);

  return TrafDesc::unpack(base, reallocator);
}

Long TrafLibraryDesc::pack(void * space)
{
  libraryName = (libraryName ? (char*)(((Space*)space)->convertToOffset(libraryName)) : NULL);
  libraryFilename = (libraryFilename ? (char*)(((Space*)space)->convertToOffset(libraryFilename)) : NULL);

  return TrafDesc::pack(space);
}

Lng32 TrafLibraryDesc::unpack(void * base, void * reallocator)
{
  libraryName = (libraryName ? (char*)((char*)base - (Long)libraryName) : NULL);
  libraryFilename = (libraryFilename ? (char*)((char*)base - (Long)libraryFilename) : NULL);

  return TrafDesc::unpack(base, reallocator);
}

Long TrafPartnsDesc::pack(void * space)
{
  tablename = (tablename ? (char*)(((Space*)space)->convertToOffset(tablename)) : NULL);
  partitionname = (partitionname ? (char*)(((Space*)space)->convertToOffset(partitionname)) : NULL);
  logicalpartitionname = (logicalpartitionname ? (char*)(((Space*)space)->convertToOffset(logicalpartitionname)) : NULL);
  firstkey = (firstkey ? (char*)(((Space*)space)->convertToOffset(firstkey)) : NULL);
  encodedkey = (encodedkey ? (char*)(((Space*)space)->convertToOffset(encodedkey)) : NULL);
  lowKey = (lowKey ? (char*)(((Space*)space)->convertToOffset(lowKey)) : NULL);
  highKey = (highKey ? (char*)(((Space*)space)->convertToOffset(highKey)) : NULL);
  givenname = (givenname ? (char*)(((Space*)space)->convertToOffset(givenname)) : NULL);

  return TrafDesc::pack(space);
}

Lng32 TrafPartnsDesc::unpack(void * base, void * reallocator)
{
  tablename = (tablename ? (char*)((char*)base - (Long)tablename) : NULL);
  partitionname = (partitionname ? (char*)((char*)base - (Long)partitionname) : NULL);
  logicalpartitionname = (logicalpartitionname ? (char*)((char*)base - (Long)logicalpartitionname) : NULL);
  firstkey = (firstkey ? (char*)((char*)base - (Long)firstkey) : NULL);
  encodedkey = (encodedkey ? (char*)((char*)base - (Long)encodedkey) : NULL);
  lowKey = (lowKey ? (char*)((char*)base - (Long)lowKey) : NULL);
  highKey = (highKey ? (char*)((char*)base - (Long)highKey) : NULL);
  givenname = (givenname ? (char*)((char*)base - (Long)givenname) : NULL);

  return TrafDesc::unpack(base, reallocator);
}

Long TrafRefConstrntsDesc::pack(void * space)
{
  constrntname = (constrntname ? (char*)(((Space*)space)->convertToOffset(constrntname)) : NULL);
  tablename = (tablename ? (char*)(((Space*)space)->convertToOffset(tablename)) : NULL);

  return TrafDesc::pack(space);
}

Lng32 TrafRefConstrntsDesc::unpack(void * base, void * reallocator)
{
  constrntname = (constrntname ? (char*)((char*)base - (Long)constrntname) : NULL);
  tablename = (tablename ? (char*)((char*)base - (Long)tablename) : NULL);

  return TrafDesc::unpack(base, reallocator);
}

Long TrafRoutineDesc::pack(void * space)
{
  routineName = (routineName ? (char*)(((Space*)space)->convertToOffset(routineName)) : NULL);
  externalName = (externalName ? (char*)(((Space*)space)->convertToOffset(externalName)) : NULL);
  librarySqlName = (librarySqlName ? (char*)(((Space*)space)->convertToOffset(librarySqlName)) : NULL);
  libraryFileName = (libraryFileName ? (char*)(((Space*)space)->convertToOffset(libraryFileName)) : NULL);
  signature = (signature ? (char*)(((Space*)space)->convertToOffset(signature)) : NULL);

  params.pack(space);

  return TrafDesc::pack(space);
}

Lng32 TrafRoutineDesc::unpack(void * base, void * reallocator)
{
  routineName = (routineName ? (char*)((char*)base - (Long)routineName) : NULL);
  externalName = (externalName ? (char*)((char*)base - (Long)externalName) : NULL);
  librarySqlName = (librarySqlName ? (char*)((char*)base - (Long)librarySqlName) : NULL);
  libraryFileName = (libraryFileName ? (char*)((char*)base - (Long)libraryFileName) : NULL);
  signature = (signature ? (char*)((char*)base - (Long)signature) : NULL);

  if (params.unpack(base, reallocator)) return -1;

  return TrafDesc::unpack(base, reallocator);
}

Long TrafSequenceGeneratorDesc::pack(void * space)
{
  sgLocation = (sgLocation ? (char*)(((Space*)space)->convertToOffset(sgLocation)) : NULL);

  return TrafDesc::pack(space);
}

Lng32 TrafSequenceGeneratorDesc::unpack(void * base, void * reallocator)
{
  sgLocation = (sgLocation ? (char*)((char*)base - (Long)sgLocation) : NULL);

  return TrafDesc::unpack(base, reallocator);
}

Long TrafTableDesc::pack(void * space)
{
  tablename = (tablename ? (char*)(((Space*)space)->convertToOffset(tablename)) : NULL);
  snapshotName = (snapshotName ? (char*)(((Space*)space)->convertToOffset(snapshotName)) : NULL);
  default_col_fam = (default_col_fam ? (char*)(((Space*)space)->convertToOffset(default_col_fam)) : NULL);
  all_col_fams = (all_col_fams ? (char*)(((Space*)space)->convertToOffset(all_col_fams)) : NULL);

  columns_desc.pack(space);
  indexes_desc.pack(space);
  constrnts_desc.pack(space);
  views_desc.pack(space);
  constrnts_tables_desc.pack(space);
  referenced_tables_desc.pack(space);
  referencing_tables_desc.pack(space);
  histograms_desc.pack(space);
  files_desc.pack(space);
  hbase_regionkey_desc.pack(space);
  sequence_generator_desc.pack(space);
  priv_desc.pack(space);

  return TrafDesc::pack(space);  
}

Lng32 TrafTableDesc::unpack(void * base, void * reallocator)
{
  tablename = (tablename ? (char*)((char*)base - (Long)tablename) : NULL);
  snapshotName = (snapshotName ? (char*)((char*)base - (Long)snapshotName) : NULL);
  default_col_fam = (default_col_fam ? (char*)((char*)base - (Long)default_col_fam) : NULL);
  all_col_fams = (all_col_fams ? (char*)((char*)base - (Long)all_col_fams) : NULL);

  if (columns_desc.unpack(base, reallocator)) return -1;
  if (indexes_desc.unpack(base, reallocator)) return -1;
  if (constrnts_desc.unpack(base, reallocator)) return -1;
  if (views_desc.unpack(base, reallocator)) return -1;
  if (constrnts_tables_desc.unpack(base, reallocator)) return -1;
  if (referenced_tables_desc.unpack(base, reallocator)) return -1;
  if (referencing_tables_desc.unpack(base, reallocator)) return -1;
  if (histograms_desc.unpack(base, reallocator)) return -1;
  if (files_desc.unpack(base, reallocator)) return -1;
  if (hbase_regionkey_desc.unpack(base, reallocator)) return -1;
  if (sequence_generator_desc.unpack(base, reallocator)) return -1;
  if (priv_desc.unpack(base, reallocator)) return -1;

  return TrafDesc::unpack(base, reallocator);
}

Long TrafUsingMvDesc::pack(void * space)
{
  mvName = (mvName ? (char*)(((Space*)space)->convertToOffset(mvName)) : NULL);

  return TrafDesc::pack(space);
}

Lng32 TrafUsingMvDesc::unpack(void * base, void * reallocator)
{
  mvName = (mvName ? (char*)((char*)base - (Long)mvName) : NULL);

  return TrafDesc::unpack(base, reallocator);
}

Long TrafViewDesc::pack(void * space)
{
  viewname = (viewname ? (char*)(((Space*)space)->convertToOffset(viewname)) : NULL);
  viewfilename = (viewfilename ? (char*)(((Space*)space)->convertToOffset(viewfilename)) : NULL);

  viewtext = (viewtext ? (char*)(((Space*)space)->convertToOffset(viewtext)) : NULL);
  viewchecktext = (viewchecktext ? (char*)(((Space*)space)->convertToOffset(viewchecktext)) : NULL);
  viewcolusages = (viewcolusages ? (char*)(((Space*)space)->convertToOffset(viewcolusages)) : NULL);

  return TrafDesc::pack(space);
}

Lng32 TrafViewDesc::unpack(void * base, void * reallocator)
{
  viewname = (viewname ? (char*)((char*)base - (Long)viewname) : NULL);
  viewfilename = (viewfilename ? (char*)((char*)base - (Long)viewfilename) : NULL);

  viewtext = (viewtext ? (char*)((char*)base - (Long)viewtext) : NULL);
  viewchecktext = (viewchecktext ? (char*)((char*)base - (Long)viewchecktext) : NULL);
  viewcolusages = (viewcolusages ? (char*)((char*)base - (Long)viewcolusages) : NULL);

  return TrafDesc::unpack(base, reallocator);
}
 
Long TrafPrivDesc::pack (void * space)
{
   privGrantees.pack(space);

   return TrafDesc::pack(space);
}
 
Lng32 TrafPrivDesc::unpack(void * base, void * reallocator)
{
  if (privGrantees.unpack(base, reallocator)) return -1;
  return TrafDesc::unpack(base, reallocator);
}

Long TrafPrivGranteeDesc::pack(void * space)
{
  objectBitmap.pack(space);
  columnBitmaps.pack(space);

  return TrafDesc::pack(space);
}

Lng32 TrafPrivGranteeDesc::unpack(void * base, void *reallocator)
{
  if (objectBitmap.unpack(base, reallocator)) return -1;
  if (columnBitmaps.unpack(base, reallocator)) return -1;

  return TrafDesc::unpack(base, reallocator);
}

