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

#ifndef HIVE_HOOK_H
#define HIVE_HOOK_H 1

#include "Platform.h"
#include "NABoolean.h"
#include "CmpCommon.h"

class HiveClient_JNI;

static char* strduph(const char *s, CollHeap* h) {
  char *d = new (h) char[strlen (s) + 1];   // Space for length plus nul
  if (d == NULL) return NULL;          // No memory
  strcpy (d,s);                        // Copy the characters
  return d;                            // Return the new string
}

struct hiveMetastoreStruct
{

  hiveMetastoreStruct() : conn(0), tblID(0), sdID(0), cdID(0)
  {}

  ~hiveMetastoreStruct() {}

   void* conn;
   Int32 tblID;
   Int32 sdID;
   Int32 cdID;
};

   
struct hive_column_desc
{
   NAHeap *heap_;
   Int32 columnID_;
   char* name_;
   char* type_;
   Int32 intIndex_;
   struct hive_column_desc* next_;

   hive_column_desc(NAHeap *heap, Int32 cdID, const char* name, const char* type, Int32 index) 
     : heap_(heap), columnID_(cdID), intIndex_(index), next_(0)
   { 
     name_ = strduph(name, heap_); 
     type_ = strduph(type, heap_);
   }

  ~hive_column_desc();
};
   
struct hive_pkey_desc
{
   NAHeap *heap_;
   char* name_;
   char* type_;
   Int32 idx_;

   struct hive_pkey_desc* next_;

   hive_pkey_desc(NAHeap *heap, const char* name, const char* type, Int32 idx) : 
        heap_(heap), idx_(idx), next_(0)
   {
     name_ = strduph(name, heap_);
     type_ = strduph(type, heap_);
   }

  ~hive_pkey_desc();
};


struct hive_skey_desc
{
   NAHeap *heap_;
   char* name_;
   Int32 idx_;
   Int32 orderInt_;

   struct hive_skey_desc* next_;

   hive_skey_desc(NAHeap *heap, const char* name, Int32 idx, Int32 order) :
        heap_(heap), idx_(idx), next_(NULL), orderInt_(order)
   {
     name_ = strduph(name, heap_);
   }

  ~hive_skey_desc();

};

struct hive_bkey_desc
{
   NAHeap *heap_;
   char* name_;
   Int32 idx_;

   struct hive_bkey_desc* next_;

   hive_bkey_desc(NAHeap *heap, const char* name, Int32 idx) :
        heap_(heap), idx_(idx), next_(NULL)
   {
     name_ = strduph(name, heap_);
   }

  ~hive_bkey_desc();
};

struct hive_sd_desc
{
   NAHeap *heap_;
   enum sd_desc_kind { TABLE_SD = 'T', PARTN_SD = 'P' };
   Int32 sdID_;
   char* location_;
   Int64 creationTS_;
   Int32 buckets_;
   char* inputFormat_;
   char* outputFormat_;
   char kind_;
   struct hive_column_desc* column_;
   struct hive_skey_desc* skey_;
   struct hive_bkey_desc* bkey_;

   char fieldTerminator_;
   char recordTerminator_;
   char* nullFormat_;

   NABoolean isCompressed_;
   const char *partitionColValues_;

   struct hive_sd_desc* next_;

   hive_sd_desc(NAHeap *heap, Int32 sdID, const char* loc, Int64 creationTS, Int32 buckets,
                const char* ift, const char* of, 
                const char* nf,
                char knd,
                struct hive_column_desc* column,
                struct hive_skey_desc* skey,
                struct hive_bkey_desc* bkey,
                char fieldTerminator, char recordTerminator,
                const NABoolean isCompressed,
                const char *pColVals
                )

        : heap_(heap), sdID_(sdID), creationTS_(creationTS),
      buckets_(buckets), kind_(knd), column_(column),
      skey_(skey), bkey_(bkey), 
      fieldTerminator_(fieldTerminator),
      recordTerminator_(recordTerminator),
      isCompressed_(isCompressed),
      next_(NULL)
   {
     if (loc != NULL)
        location_ = strduph(loc, heap_);
     else
        location_ = NULL;
     inputFormat_ = strduph(ift, heap_);
     outputFormat_= strduph(of, heap_);
     nullFormat_ = (nf ? strduph(nf, heap_) : NULL);
     if (pColVals)
       partitionColValues_ = strduph(pColVals, heap_);
     else
       partitionColValues_ = NULL;
   }

  ~hive_sd_desc();

   char getFieldTerminator() const { return fieldTerminator_; }
   char getRecordTerminator() const { return recordTerminator_;}
   char *getNullFormat() const {return nullFormat_; }

   NABoolean isSequenceFile() const;
   NABoolean isOrcFile() const;
   NABoolean isParquetFile() const;
   NABoolean isTextFile() const;
   NABoolean isTrulyText() const 
     { return !isSequenceFile() && isTextFile(); };
};

struct hive_tblparams_desc
{
  hive_tblparams_desc(NAHeap *heap, const char * tblParamsStr,
                      Int32 obp, Int64 oss, Int32 oris, 
                      const char * oc, const char* bfc,
                      double bff, NABoolean oci);
  ~hive_tblparams_desc();

  char* getTblParamsStr() { return tblParamsStr_; }
  NABoolean getOrcBlockPadding() { return (orcBlockPadding_ == 1); }
  Int64 getOrcStripeSize() { return orcStripeSize_; }
  Int32 getOrcRowIndexStride() { return orcRowIndexStride_; }
  char * getOrcCompression() { return orcCompression_; }
  char * getOrcBloomFilterColumns() { return orcBloomFilterColumns_; }
  double getOrcBloomFilterFPP() { return orcBloomFilterFPP_; }
  NABoolean getOrcCreateIndex() { return orcCreateIndex_; }

  Int64 getParquetBlockSize() { return orcStripeSize_; }
  Int32 getParquetPageSize() { return orcRowIndexStride_; }
  char * getParquetCompression() { return orcCompression_; }
  Int64 getParquetDictionaryPageSize() { return orcBloomFilterFPP_; }
  NABoolean getParquetEnableDictionary() { return orcCreateIndex_; }
  Int32 getParquetWriterMaxPadding() { return orcBlockPadding_;}

private: 
  NAHeap *heap_; 
  char* tblParamsStr_;

  Int32 orcBlockPadding_;
  Int64 orcStripeSize_;
  Int32 orcRowIndexStride_;
  char  orcCompression_[16];
  char * orcBloomFilterColumns_;  
  double orcBloomFilterFPP_;
  NABoolean orcCreateIndex_;
};

struct hive_tbl_desc
{
  NAHeap *heap_;
  Int32 tblID_; // not used with JNI
  char* tblName_;
  char* schName_;
  char* owner_;
  char* tableType_;
  Int64 creationTS_;
  // redefineTS_ = hdfs modification time of the hive files too
  Int64 redefineTS_;
  // next 2 fields are used if hive object is a view.
  // Contents are populated during HiveMetaData::getTableDesc.
  // original text is what was used at view creation time.
  // expanded text contains fully qualified object/col names.
  char * viewOriginalText_;
  char * viewExpandedText_;

  struct hive_sd_desc* sd_;
  struct hive_pkey_desc* pkey_;

  struct hive_tblparams_desc* tblParams_;  

  struct hive_tbl_desc* next_;
  
  hive_tbl_desc(NAHeap *heap, Int32 tblID, const char* name, const char* schName, 
                const char * owner,
                const char * tableType,
                Int64 creationTS, 
                const char * viewOriginalText,
                const char * viewExpandedText,
                struct hive_sd_desc* sd,
                struct hive_pkey_desc* pk,
                struct hive_tblparams_desc* tp);

  ~hive_tbl_desc();

  NABoolean isView() 
  { 
    if (tableType_ && (strcmp(tableType_, "VIRTUAL_VIEW") == 0))
      return TRUE;
    else
      return FALSE;
  }

  NABoolean isExternalTable() 
  { 
    if (tableType_ && (strcmp(tableType_, "EXTERNAL_TABLE") == 0))
      return TRUE;
    else
      return FALSE;
  }

  NABoolean isManagedTable() 
  { 
    if (tableType_ && (strcmp(tableType_, "MANAGED_TABLE") == 0))
      return TRUE;
    else
      return FALSE;
  }

  struct hive_sd_desc* getSDs() { return sd_; };
  
  struct hive_skey_desc* getSortKeys();
  struct hive_bkey_desc* getBucketingKeys();
  struct hive_pkey_desc* getPartKey() { return pkey_; };
  struct hive_column_desc* getColumns();
  
  Int32 getNumOfCols();
  Int32 getNumOfPartCols() const;
  Int32 getNumOfSortCols();
  Int32 getNumOfBucketCols();
  
  Int32 getPartColNum(const char* name);
  Int32 getBucketColNum(const char* name);
  Int32 getSortColNum(const char* name);

  Int64 redeftime();
  Int64 setRedeftime(Int64 redeftime); 
};

class HiveMetaData
{
 
public:
  HiveMetaData(NAHeap *heap); 
  ~HiveMetaData();
  
  NABoolean init();
  
  struct hive_tbl_desc* getTableDesc(const char* schemaName,
                                     const char* tblName,
                                     NABoolean validateOnly,
                                     // force to reread from Hive MD
                                     NABoolean rereadFromMD,
                                     NABoolean readPartnInfo);
  struct hive_tbl_desc* getFakedTableDesc(const char* tblName);
  
  // validate a cached hive table descriptor
  NABoolean validate(hive_tbl_desc *hDesc);
  
  // iterator over all tables in a Hive schema (default)
  // or iterate over all schemas in the Hive metadata
  void position();
  struct hive_tbl_desc * getNext();
  void advance();
  NABoolean atEnd();
  void clear();

  // what the Hive default schema is called in the Hive metadata
  static const char *getDefaultSchemaName() { return "default"; }
  
  // get lower-level error code
  Int32 getErrCode() const { return errCode_; }
  
  const char* getErrDetail() const {return errDetail_; }
  
  const char* getErrCodeStr() const {return errCodeStr_; }
  
  const char* getErrMethodName() const {return errMethodName_; }
  
  void resetErrorInfo();

   // return TRUE for success, otherwise record error info and return FALSE
  NABoolean recordError(Int32 errCode, const char *errMethodName);

  void recordParseError(Int32 errCode, const char* errCodeStr,
                        const char *errMethodName, const char* errDetail);

protected:
  NAHeap *heap_;
  // read metadata for one table
  struct hive_tbd_desc* read(const char *schemaName,
                             const char *tableName);

  struct hive_tbl_desc* tbl_;
  hive_tbl_desc * currDesc_;
  Int32 errCode_;
  const char *errCodeStr_;
  const char *errMethodName_;
  const char *errDetail_;
};


#endif
