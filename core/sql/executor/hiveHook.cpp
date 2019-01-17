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
#include "hiveHook.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>

#undef _MSC_VER

#include "str.h"
#include "NAStringDef.h"
#include "HBaseClient_JNI.h"
#include "HiveClient_JNI.h"
#include "Globals.h"


NABoolean findAToken (HiveMetaData *md, NAText* tblStr, size_t& pos, 
                      const char* tok, const char* errStr,
                      NABoolean raiseError = TRUE);

NABoolean extractValueStr (HiveMetaData *md, NAText* tblStr, size_t& pos, 
                           const char* beginTok, const char * endTok, 
                           NAText& valueStr, const char* errStr,
                           NABoolean raiseError = TRUE);


HiveMetaData::HiveMetaData(NAHeap *heap) : heap_(heap), tbl_(NULL),
                               currDesc_(NULL),
                               errCode_(0) ,
                               errDetail_(NULL),
                               errMethodName_(NULL),
                               errCodeStr_(NULL)
{
}

HiveMetaData::~HiveMetaData()
{
  clear();
}

NABoolean HiveMetaData::init()
{
  return TRUE;
}

static NABoolean splitURL(const char *url,
                          NAString &host,
                          Int32 &port,
                          NAString &options)
{
  NABoolean result = TRUE;
  // split the url into host and file name, proto://host:port/file,
  // split point is at the third slash in the URL
  const char *c = url;
  const char *hostMark = NULL;
  const char *portMark = NULL;
  const char *dirMark  = NULL;
  int numSlashes = 0;
  int numColons = 0;
  while (*c && dirMark == NULL)
    {
      if (*c == '/')
        numSlashes++;
      else if (*c == ':')
        numColons++;
      c++;

      if (hostMark == NULL && numSlashes == 2)
        hostMark = c;
      else if (portMark == NULL && hostMark && numColons == 2)
        portMark = c;
      else if (numSlashes == 3 ||                         // regular URL
               (numSlashes == 1 && c == url+1)) // just a file name
        dirMark = c-1; // include the leading slash
    }

  if (dirMark == NULL)
    {
      dirMark = c; // point to end of string
      options = "";
    }
  else
    options = NAString(dirMark);

  if (hostMark)
    host = NAString(hostMark, (portMark ? portMark-hostMark-1
                                        : dirMark-hostMark));
  if (portMark)
    port = atoi(portMark);
  else
    port = 0;

  return result;
}

void HiveMetaData::position()
{
  currDesc_ = tbl_;
}

struct hive_tbl_desc * HiveMetaData::getNext()
{
  return currDesc_;
}

void HiveMetaData::advance()
{
  if (currDesc_)
    currDesc_ = currDesc_->next_;
}

NABoolean HiveMetaData::atEnd()
{
  return (currDesc_ ? FALSE : TRUE);
}

void HiveMetaData::clear()
{
  hive_tbl_desc* ptr ;
  while (tbl_) {
    ptr = tbl_->next_;       
    NADELETEBASIC(tbl_, heap_);
    tbl_ = ptr;
  }

  tbl_ = NULL;
}

NABoolean HiveMetaData::recordError(Int32 errCode,
                                    const char *errMethodName)
{
  if (errCode != HVC_OK)
    {
      errCode_ = errCode;
      errCodeStr_ = HiveClient_JNI::getErrorText((HVC_RetCode)errCode_);
      errMethodName_ = errMethodName;
      errDetail_ = GetCliGlobals()->getJniErrorStr();
      return FALSE;
    }
  return TRUE;
}

void HiveMetaData::recordParseError(Int32 errCode, const char* errCodeStr,
                        const char *errMethodName, const char* errDetail)
{
  errCode_ = errCode;
  errCodeStr_ = errCodeStr;
  errMethodName_ = errMethodName;
  errDetail_ = errDetail;
}

void HiveMetaData::resetErrorInfo()
{
  errCode_ = 0;
  errDetail_ = NULL;
  errMethodName_ = NULL;
  errCodeStr_ = NULL;
}

   
NABoolean hive_sd_desc::isOrcFile() const
{
  return strstr(inputFormat_, "Orc") && 
    strstr(outputFormat_, "Orc");
}

NABoolean hive_sd_desc::isParquetFile() const
{
  return strstr(inputFormat_, "Parquet") &&
    strstr(outputFormat_, "Parquet");
}

NABoolean hive_sd_desc::isSequenceFile() const
{
  return strstr(inputFormat_, "Sequence") && 
    strstr(outputFormat_, "Sequence");
}

NABoolean hive_sd_desc::isTextFile() const
{
  return strstr(inputFormat_, "Text") && 
    strstr(outputFormat_, "Text");
}

static int getAsciiDecimalValue(const char * valPtr)
{
  if (str_len(valPtr) <= 0) return 0;
  if (str_len(valPtr) == 1) return valPtr[0];
  return atoi(valPtr);
}

NABoolean findAToken (HiveMetaData *md, NAText* tblStr, size_t& pos, 
                      const char* tok, const char* errStr, NABoolean raiseError)
{
  size_t foundB = tblStr->find(tok, pos);
  if (foundB == std::string::npos) {
    if (raiseError) {
      NAText *errText = new (CmpCommon::statementHeap()) string(errStr);
      char xPos[7]; str_itoa(pos, xPos);
      errText->append(" at position ");
      errText->append(xPos);
      errText->append(tblStr->c_str());
      md->recordParseError(1500, "PARSE_ERROR", 
                           errText->c_str(), tblStr->c_str());
    }
    return FALSE;
  }
  pos = foundB;
  return TRUE; 
}

NABoolean extractValueStr (HiveMetaData *md, NAText* tblStr, size_t& pos, 
                           const char* beginTok, const char* endTok,
                           NAText& valueStr, const char* errStr, 
                           NABoolean raiseError)
{
  if (!findAToken(md, tblStr, pos, beginTok, errStr, raiseError))
    return FALSE;
  size_t foundB = pos + strlen(beginTok);
  if (!findAToken(md, tblStr, pos, endTok, errStr, TRUE))
    return FALSE;
  valueStr.append(tblStr->substr(foundB, pos-foundB ));
  return TRUE;
}

hive_tblparams_desc::hive_tblparams_desc(NAHeap *heap, const char* tblParamsStr,
                                         NABoolean obp, Int64 oss, Int32 oris,
                                         const char * oc, const char* bfc,
                                         double bff, NABoolean oci) :

     heap_(heap),
     tblParamsStr_(NULL),
     orcBlockPadding_(obp),
     orcStripeSize_(oss), orcRowIndexStride_(oris),
     orcBloomFilterColumns_(NULL),
     orcBloomFilterFPP_(bff),
     orcCreateIndex_(oci)
{
  if (tblParamsStr)
    tblParamsStr_ = strduph(tblParamsStr, heap_);
  
  orcCompression_[0] = 0;
  if (oc)
    strcpy(orcCompression_, oc);
  if (bfc)
    orcBloomFilterColumns_ = strduph(bfc, heap_);
}

struct hive_tbl_desc* HiveMetaData::getFakedTableDesc(const char* tblName)
{
  NAHeap *h = heap_;
  hive_column_desc* c1 = new (h) hive_column_desc(h, 1, "C1", "int", 0);
  hive_column_desc* c2 = new (h) hive_column_desc(h, 2, "C2", "string", 1);
  hive_column_desc* c3 = new (h) hive_column_desc(h, 3, "C3", "float", 2);

   c1->next_ = c2;
   c2->next_ = c3;

   // sort key c1
   hive_skey_desc* sk1 = new (h) hive_skey_desc(h, "C1", 1, 1);

   // bucket key c2
   hive_bkey_desc* bk1 = new (h) hive_bkey_desc(h, "C2", 1);


   hive_sd_desc* sd1 = new (h)hive_sd_desc(h, 1, "loc", 0, 1, "ift", "oft", NULL,
                                           hive_sd_desc::TABLE_SD, c1, 
                                           sk1, bk1, '\010', '\n',
                                           FALSE, NULL);

   hive_tbl_desc* tbl1 = new (h) hive_tbl_desc(h, 1, "myHive", "default", "me",
                                               "MANAGED",
                                               0, NULL, NULL, sd1, NULL, NULL);

   return tbl1;
}

struct hive_tbl_desc* HiveMetaData::getTableDesc( const char* schemaName,
                                                 const char* tblName,
                                                 NABoolean validateOnly,
                                                 NABoolean rereadFromMD,
                                                 NABoolean readPartnInfo)
{
    struct hive_tbl_desc *ptr = tbl_;

    while (ptr) {

      if ( !(stricmp(ptr->tblName_, tblName)
             ||stricmp(ptr->schName_, schemaName))) {
        if ((NOT rereadFromMD) && (validate(ptr)))
           return ptr;
        else {
          // table changed, delete it and re-read below
          if (tbl_ == ptr)
            tbl_ = ptr->next_;
          else {
            struct hive_tbl_desc *ptr2 = tbl_;
          
            while (ptr2) {
              if (ptr2->next_ == ptr)
                ptr2->next_ = ptr->next_;
              ptr2 = ptr2->next_;
            }
          }        
          NADELETEBASIC(ptr, heap_);
          ptr = NULL;
          break;
        }
      }

      ptr = ptr->next_;
   }
   
   if (validateOnly)
     return NULL;

   HVC_RetCode hvcRetcode;
   HiveClient_JNI *hiveClient = HiveClient_JNI::newInstance(heap_, hvcRetcode);
   if (hvcRetcode != HVC_OK) {
      recordError((Int32)hvcRetcode, "HiveClient_JNI::newInstance()");
      return NULL;
   }
 
   hvcRetcode = hiveClient->getHiveTableInfo(schemaName, tblName, readPartnInfo);
   if (hvcRetcode != HVC_OK && hvcRetcode != HVC_DONE) {
      recordError((Int32)hvcRetcode, "HiveClient_JNI::getHiveTableInfo()");
      NADELETE(hiveClient, HiveClient_JNI, heap_);
      return NULL;
   }
   if (hvcRetcode == HVC_DONE) {
      NADELETE(hiveClient, HiveClient_JNI, heap_);
      return NULL;
   }

   // table not found in cache, try to read it from metadata
   hive_tbl_desc * hiveTableDesc;
   hvcRetcode = hiveClient->getHiveTableDesc(heap_, hiveTableDesc);
   if (hvcRetcode != HVC_OK) {
      recordError((Int32)hvcRetcode, "HiveClient_JNI::getHiveTableInfoDetails()");
      NADELETE(hiveClient, HiveClient_JNI, heap_);
      return NULL;
   } 
   // add the new table to the cache
   hiveTableDesc->next_ = tbl_;
   tbl_ = hiveTableDesc;
   NADELETE(hiveClient, HiveClient_JNI, heap_);
   return hiveTableDesc;
}

NABoolean HiveMetaData::validate(hive_tbl_desc *hDesc)
{
   Int64 currentRedefTime = 0;
   HVC_RetCode retCode = HiveClient_JNI::getRedefTime(hDesc->schName_, hDesc->tblName_,
                                                 currentRedefTime);
   if ((retCode != HVC_OK) && (retCode != HVC_DONE)) {
     return recordError((Int32)retCode, "HiveClient_JNI::getRedefTime()");
   }
   if ((retCode == HVC_DONE) || (currentRedefTime != hDesc->redeftime())) 
     return FALSE;
  
   return TRUE;
}

hive_tbl_desc::hive_tbl_desc(NAHeap *heap, Int32 tblID, const char* name, const char* schName, 
                             const char * owner,
                             const char * tableType,
                             Int64 creationTS, 
                             const char * viewOriginalText,
                             const char * viewExpandedText,
                             struct hive_sd_desc* sd,
                             struct hive_pkey_desc* pk,
                             struct hive_tblparams_desc* tp)
     : heap_(heap), tblID_(tblID), 
       viewOriginalText_(NULL), viewExpandedText_(NULL),
       sd_(sd), tblParams_(tp),
       creationTS_(creationTS), 
       redefineTS_(-1), pkey_(pk), next_(NULL)
{  
  tblName_ = strduph(name, heap_);
  schName_ = strduph(schName, heap_);

  if (owner)
    owner_ = strduph(owner, heap_);
  else
    owner_ = NULL;

  if (tableType)
    tableType_ = strduph(tableType, heap_);
  else
    tableType_ = NULL;

  if (isView())
    {
      if (viewOriginalText)
        viewOriginalText_ = strduph(viewOriginalText, heap_);
      else
        viewOriginalText_ = NULL;

      if (viewExpandedText)
        viewExpandedText_ = strduph(viewExpandedText, heap_);
      else
        viewExpandedText_ = NULL;
    }

}

struct hive_column_desc* hive_tbl_desc::getColumns()
{
   struct hive_sd_desc* sd = sd_;

   // assume all SDs have the same column structure!
   if ( sd ) 
      return sd->column_;

   return NULL;
}


struct hive_bkey_desc* hive_tbl_desc::getBucketingKeys()
{
   struct hive_sd_desc* sd = sd_;

   // assume all SDs have the same bucketing key structure!
   if ( sd ) 
      return sd->bkey_;
   

   return NULL;
}

struct hive_skey_desc* hive_tbl_desc::getSortKeys()
{
   struct hive_sd_desc* sd = sd_;

   // assume all SDs have the same sort key structure!
   if ( sd ) {
      return sd->skey_;
   }

   return NULL;
}

Int32 hive_tbl_desc::getNumOfCols()
{
  Int32 result = 0;
  hive_column_desc *cd = getColumns();
  while (cd)
    {
      result++;
      cd = cd->next_;
    }
  return result;
}

Int32 hive_tbl_desc::getNumOfPartCols() const
{
  Int32 result = 0;
  hive_pkey_desc *pk = pkey_;
  while (pk)
    {
      result++;
      pk = pk->next_;
    }
  return result;
}

Int32 hive_tbl_desc::getNumOfSortCols()
{
  Int32 result = 0;
  hive_skey_desc *sk = getSortKeys();
  while (sk)
    {
      result++;
      sk = sk->next_;
    }
  return result;
}

Int32 hive_tbl_desc::getNumOfBucketCols()
{
  Int32 result = 0;
  hive_bkey_desc *bc = getBucketingKeys();
  while (bc)
    {
      result++;
      bc = bc->next_;
    }
  return result;
}

Int32 hive_tbl_desc::getPartColNum(const char* name)
{
  Int32 num = 0;

  hive_pkey_desc * desc = getPartKey();
  while (desc)
    {
      if (strcmp(name, desc->name_) == 0)
        {
          return num;
        }
      
      num++;
      desc = desc->next_;
    }

  return -1;
}

Int32 hive_tbl_desc::getBucketColNum(const char* name)
{
  Int32 num = 0;

  hive_bkey_desc * desc = getBucketingKeys();
  while (desc)
    {
      if (strcmp(name, desc->name_) == 0)
        {
          return num;
        }
      
      num++;
      desc = desc->next_;
    }

  return -1;
}

Int32 hive_tbl_desc::getSortColNum(const char* name)
{
  Int32 num = 0;

  hive_skey_desc * desc = getSortKeys();
  while (desc)
    {
      if (strcmp(name, desc->name_) == 0)
        {
          return num;
        }
      
      num++;
      desc = desc->next_;
    }

  return -1;
}

Int64 hive_tbl_desc::redeftime()
{
  // creationTS_ is in seconds 
  Int64 result = creationTS_ * 1000;
  if (redefineTS_ !=  -1 && redefineTS_ > result)
        result = redefineTS_;
  return result;
}

Int64 hive_tbl_desc::setRedeftime(Int64 redefineTS) 
{
   redefineTS_ = redefineTS; 
   // It is possible that timestamp of the hive files is less than the
   // creation time
   return redeftime();
}

hive_tbl_desc::~hive_tbl_desc()
{
  if (tblName_)
    NADELETEBASIC(tblName_, heap_);
  if (schName_)
    NADELETEBASIC(schName_, heap_);

   hive_sd_desc* ptr ;
   while (sd_) {
    ptr = sd_->next_;       
    NADELETEBASIC(sd_, heap_);
    sd_ = ptr;
  }
 
   hive_pkey_desc* ptr1 ;
   while (pkey_) {
    ptr1 = pkey_->next_;       
    NADELETEBASIC(pkey_, heap_);
    pkey_ = ptr1;
  }
}

hive_sd_desc::~hive_sd_desc()
{
  if (location_)
    NADELETEBASIC(location_, heap_);
  if (inputFormat_)
    NADELETEBASIC(inputFormat_, heap_);
  if (outputFormat_)
    NADELETEBASIC(outputFormat_, heap_);

  hive_column_desc* ptr ;
  while (column_) {
    ptr = column_->next_;       
    NADELETEBASIC(column_, heap_);
    column_ = ptr;
  }
 
  hive_skey_desc* ptr1 ;
  while (skey_) {
    ptr1 = skey_->next_;       
    NADELETEBASIC(skey_, heap_);
    skey_ = ptr1;
  }

  hive_bkey_desc* ptr2 ;
  while (bkey_) {
    ptr2 = bkey_->next_;       
    NADELETEBASIC(bkey_, heap_);
    bkey_ = ptr2;
  }
}


hive_pkey_desc::~hive_pkey_desc()
{
  if (name_)
    NADELETEBASIC(name_, heap_);
  if (type_)
    NADELETEBASIC(type_, heap_);
}

hive_skey_desc::~hive_skey_desc()
{
  if (name_)
    NADELETEBASIC(name_, heap_);
}

hive_bkey_desc::~hive_bkey_desc()
{
  if (name_)
    NADELETEBASIC(name_, heap_);
}

hive_column_desc::~hive_column_desc()
{
  if (name_)
    NADELETEBASIC(name_, heap_);
  if (type_)
    NADELETEBASIC(type_, heap_);
}
