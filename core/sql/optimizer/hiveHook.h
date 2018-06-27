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
   Int32 columnID_;
   char* name_;
   char* type_;
   Int32 intIndex_;
   struct hive_column_desc* next_;

   hive_column_desc(Int32 cdID, const char* name, const char* type, Int32 index) 
     : columnID_(cdID), intIndex_(index), next_(0)
   { 
     name_ = strduph(name, CmpCommon::contextHeap()); 
     type_ = strduph(type, CmpCommon::contextHeap()); 
   }

  ~hive_column_desc();
};
   
struct hive_pkey_desc
{
   char* name_;
   char* type_;
   Int32 idx_;

   struct hive_pkey_desc* next_;

   hive_pkey_desc(const char* name, const char* type, Int32 idx) : 
        idx_(idx), next_(0)
   {
     name_ = strduph(name, CmpCommon::contextHeap());
     type_ = strduph(type, CmpCommon::contextHeap());
   }

  ~hive_pkey_desc();
};


struct hive_skey_desc
{
   char* name_;
   Int32 idx_;
   Int32 orderInt_;

   struct hive_skey_desc* next_;

   hive_skey_desc(const char* name, Int32 idx, Int32 order) :
        idx_(idx), next_(NULL), orderInt_(order)
   {
     name_ = strduph(name, CmpCommon::contextHeap());
   }

  ~hive_skey_desc();

};

struct hive_bkey_desc
{
   char* name_;
   Int32 idx_;

   struct hive_bkey_desc* next_;

   hive_bkey_desc(const char* name, Int32 idx) :
        idx_(idx), next_(NULL)
   {
     name_ = strduph(name, CmpCommon::contextHeap());
   }

  ~hive_bkey_desc();
};

struct hive_sd_desc
{
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

   struct hive_sd_desc* next_;

   hive_sd_desc(Int32 sdID, const char* loc, Int64 creationTS, Int32 buckets,
                const char* ift, const char* of, 
                const char* nf,
                char knd,
                struct hive_column_desc* column,
                struct hive_skey_desc* skey,
                struct hive_bkey_desc* bkey,
                char fieldTerminator, char recordTerminator,
                const NABoolean isCompressed
                )
        : sdID_(sdID), buckets_(buckets), kind_(knd), column_(column),
          skey_(skey), bkey_(bkey), 
          fieldTerminator_(fieldTerminator),
          recordTerminator_(recordTerminator),
          isCompressed_(isCompressed),
          next_(NULL)
  {
    location_ = strduph(loc, CmpCommon::contextHeap());
    inputFormat_ = strduph(ift, CmpCommon::contextHeap()); 
    outputFormat_= strduph(of, CmpCommon::contextHeap());
    nullFormat_ = (nf ? strduph(nf, CmpCommon::contextHeap()) : NULL);
  }

  ~hive_sd_desc();

   char getFieldTerminator() const { return fieldTerminator_; }
   char getRecordTerminator() const { return recordTerminator_;}
   char *getNullFormat() const {return nullFormat_; }

   NABoolean isSequenceFile() const;
   NABoolean isOrcFile() const;
   NABoolean isTextFile() const;
   NABoolean isTrulyText() const 
     { return !isSequenceFile() && isTextFile(); };
};

struct hive_tbl_desc
{
  Int32 tblID_; // not used with JNI
  char* tblName_;
  char* schName_;
  char* owner_;
  char* tableType_;
  Int64 creationTS_;

  // next 2 fields are used if hive object is a view.
  // Contents are populated during HiveMetaData::getTableDesc.
  // original text is what was used at view creation time.
  // expanded text contains fully qualified object/col names.
  char * viewOriginalText_;
  char * viewExpandedText_;

  struct hive_sd_desc* sd_;
  struct hive_pkey_desc* pkey_;
  
  struct hive_tbl_desc* next_;
  
  hive_tbl_desc(Int32 tblID, const char* name, const char* schName, 
                const char * owner,
                const char * tableType,
                Int64 creationTS, 
                const char * viewOriginalText,
                const char * viewExpandedText,
                struct hive_sd_desc* sd,
                struct hive_pkey_desc* pk);

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
};

class HiveMetaData
{
 
public:
  HiveMetaData(); 
  ~HiveMetaData();
  
  NABoolean init();
  
  struct hive_tbl_desc* getTableDesc(const char* schemaName,
                                     const char* tblName);
  struct hive_tbl_desc* getFakedTableDesc(const char* tblName);
  
  // validate a cached hive table descriptor
  NABoolean validate(Int32 tableId, Int64 redefTS, 
                     const char* schName, const char* tblName);
  
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
