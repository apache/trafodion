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

struct hive_sd_desc* populateSD(HiveMetaData *md, Int32 mainSdID, 
                                Int32 tblID, NAText* tblStr, size_t& pos);
struct hive_column_desc* populateColumns(HiveMetaData *md, Int32 cdID,  
                                         NAText* tblStr, size_t& pos);
struct hive_pkey_desc* populatePartitionKey(HiveMetaData *md, Int32 tblID,  
                                            NAText* tblStr, size_t& pos);
struct hive_skey_desc* populateSortCols(HiveMetaData *md, Int32 sdID,  
                                        NAText* tblStr, size_t& pos);
struct hive_bkey_desc* populateBucketingCols(HiveMetaData *md, Int32 sdID,  
                                             NAText* tblStr, size_t& pos);
NABoolean populateSerDeParams(HiveMetaData *md, Int32 serdeID, 
                              char& fieldSep, char& recordSep,  
                              NABoolean &nullFormatSpec, NAString &nullFormat,
                              NAText* tblStr, size_t& pos);

NABoolean findAToken (HiveMetaData *md, NAText* tblStr, size_t& pos, 
                      const char* tok, const char* errStr,
                      NABoolean raiseError = TRUE);

NABoolean extractValueStr (HiveMetaData *md, NAText* tblStr, size_t& pos, 
                           const char* beginTok, const char * endTok, 
                           NAText& valueStr, const char* errStr,
                           NABoolean raiseError = TRUE);


HiveMetaData::HiveMetaData() : tbl_(NULL),
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
  CollHeap *h = CmpCommon::contextHeap();

  hive_tbl_desc* ptr ;
  while (tbl_) {
    ptr = tbl_->next_;       
    NADELETEBASIC(tbl_, h);
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

struct hive_sd_desc* populateSD(HiveMetaData *md, Int32 mainSdID, 
                                Int32 tblID,  NAText* tblStr, size_t& pos)
{
  struct hive_sd_desc* result = NULL;
  struct hive_sd_desc* mainSD = NULL;
  struct hive_sd_desc* last = NULL;
  char fieldTerminator, recordTerminator;

  size_t foundB;
  
  if (!findAToken(md, tblStr, pos, "sd:StorageDescriptor(", 
                  "getTableDesc::sd:StorageDescriptor(###"))
    return NULL;
  struct hive_column_desc* newColumns = populateColumns(md, 0, 
                                                        tblStr, pos);
  if (!newColumns)
    return NULL;

  NAText locationStr;
  if(!extractValueStr(md, tblStr, pos, "location:", ",", 
                      locationStr, "populateSD::location:###"))
    return NULL;
    
  NAText inputStr;
  if(!extractValueStr(md, tblStr, pos, "inputFormat:", ",", 
                      inputStr, "populateSD:inputFormat:###"))
    return NULL;
  
  NAText outputStr;
  if(!extractValueStr(md, tblStr, pos, "outputFormat:", ",", 
                      outputStr, "populateSD:outputFormat:###"))
    return NULL;
  
  NAText compressedStr;
  NABoolean isCompressed = FALSE;
  if(!extractValueStr(md, tblStr, pos, "compressed:", ",", 
                      compressedStr, "populateSD:compressed:###"))
    return NULL;
  if (compressedStr == "true")
    isCompressed = TRUE;
  
  NAText numBucketsStr;
  if(!extractValueStr(md, tblStr, pos, "numBuckets:", ",", 
                      numBucketsStr, "populateSD:numBuckets:###"))
    return NULL;
  Int32 numBuckets = atoi(numBucketsStr.c_str());
  
  NABoolean nullFormatSpec = FALSE;
  NAString nullFormat;
  NABoolean success = populateSerDeParams(md, 0, fieldTerminator, 
                                          recordTerminator, 
                                          nullFormatSpec, nullFormat,
                                          tblStr, pos);
  if (!success)
    return NULL;

  struct hive_bkey_desc* newBucketingCols = 
    populateBucketingCols(md, 0, tblStr, pos);

  struct hive_skey_desc* newSortCols = populateSortCols(md, 0, 
                                                        tblStr, pos);

  struct hive_sd_desc* newSD = new (CmpCommon::contextHeap()) 
    struct hive_sd_desc(0, //SdID
                        locationStr.c_str(),
                        0, // creation time
                        numBuckets,
                        inputStr.c_str(),
                        outputStr.c_str(),
                        (nullFormatSpec ? nullFormat.data() : NULL),
                        hive_sd_desc::TABLE_SD, 
                        // TODO : no support for hive_sd_desc::PARTN_SD
                        newColumns, 
                        newSortCols, 
                        newBucketingCols,
                        fieldTerminator,
                        recordTerminator,
                        isCompressed
                        );
  
  result = newSD;
  
  // TODO : loop over SDs
  if (findAToken(md, tblStr, pos, "sd:StorageDescriptor(", 
                 "getTableDesc::sd:StorageDescriptor(###)",FALSE))
    return NULL;

  return result;
}

   
NABoolean hive_sd_desc::isOrcFile() const
{
  return strstr(inputFormat_, "Orc") && 
    strstr(outputFormat_, "Orc");
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

struct hive_column_desc* populateColumns(HiveMetaData *md, Int32 cdID,  
                                         NAText* tblStr, size_t& pos)
{
  struct hive_column_desc* result = NULL;
  struct hive_column_desc* last = result;

  std::size_t foundB ;
  if (!findAToken(md, tblStr, pos, "cols:", 
                  "populateColumns::cols:###"))
    return NULL;
  
  std::size_t foundE = pos;
  if (!findAToken(md, tblStr, foundE, ")],", 
                  "populateColumns::cols:],###"))
    return NULL;
  
  Int32 colIdx = 0;
  while (pos < foundE)
    {
      NAText nameStr;
      if(!extractValueStr(md, tblStr, pos, "FieldSchema(name:", ",", 
                          nameStr, "populateColumns::FieldSchema(name:###"))
        return NULL;
      
      NAText typeStr;
      if(!extractValueStr(md, tblStr, pos, "type:", ", comment", 
                          typeStr, "populateColumns::type:###"))
        return NULL;
      
      pos++;
      if (!findAToken(md, tblStr, pos, ",", 
                      "populateColumns::comment:,###"))
        return NULL;
      
      struct hive_column_desc* newCol = new (CmpCommon::contextHeap())
        struct hive_column_desc(0, 
                                nameStr.c_str(),
                                typeStr.c_str(),
                                colIdx);
      
      if ( result == NULL ) {
        last = result = newCol;
      } else {
        last->next_ = newCol;
        last = newCol;
      }
      
      colIdx++;
    } // end of while
  
  return result;
}

struct hive_pkey_desc* populatePartitionKey(HiveMetaData *md, Int32 tblID,  
                                            NAText* tblStr, size_t& pos)
{
  hive_pkey_desc* result = NULL;
  hive_pkey_desc* last = NULL;

  std::size_t foundB ;
  if (!findAToken(md, tblStr, pos, "partitionKeys:",
                  "populatePartitionKeys::partitionKeys:###"))
    return NULL;
  
  std::size_t foundE = pos ;
  if (!findAToken(md, tblStr, foundE, "],",
                  "populatePartitionKeys::partitionKeys:],###"))
    return NULL;
  
  Int32 colIdx = 0;
  while (pos < foundE)
    {
      foundB = tblStr->find("FieldSchema(name:", pos);
      if ((foundB == std::string::npos)||(foundB > foundE)) {
        return NULL; // no part Key
      }
      
      foundB = foundB + strlen("FieldSchema(name:");
      pos = foundB ;
      if (!findAToken(md, tblStr, pos, ",",
                      "populatePartitionKeys::comment:,###"))
        return NULL;
      
      NAText nameStr = tblStr->substr(foundB, pos-foundB);
      
      NAText typeStr;
      if(!extractValueStr(md, tblStr, pos, "type:", ", comment", 
                          typeStr, "populatePartitionKeys::type:###"))
        return NULL;
      
      pos++;
      if (!findAToken(md, tblStr, pos, ",",
                      "populateColumns::comment:,###"))
        return NULL;
      
      
      hive_pkey_desc* newPkey = new (CmpCommon::contextHeap())
        struct hive_pkey_desc(nameStr.c_str(),
                              typeStr.c_str(),
                              colIdx);
      
      if ( result == NULL ) {
        last = result = newPkey;
      } else {
        last->next_ = newPkey;
        last = newPkey;
      }
      
      colIdx++;
    } // end of while

  return result;
}

struct hive_skey_desc* populateSortCols(HiveMetaData *md, Int32 sdID,  
                                        NAText* tblStr, size_t& pos)
{
  hive_skey_desc* result = NULL;
  hive_skey_desc* last = NULL;

  std::size_t foundB ;
  if (!findAToken(md, tblStr, pos, "sortCols:",
                  "populateSortCols::sortCols:###"))
    return NULL;
  
  std::size_t foundE = pos ;
  if (!findAToken(md, tblStr, foundE, "],",
                  "populateSortCols::sortCols:],###"))
    return NULL;
  
  if ((foundE - pos)<=10) //this is important to avoid major performance impact when looking for non existent Order(col over and over, parsing to the end of string. hot spot flagged using gprof
    return NULL;
  Int32 colIdx = 0;
  while (pos < foundE)
    {
      foundB = tblStr->find("Order(col:", pos);
      if ((foundB == std::string::npos)||(foundB > foundE)) {
        return NULL;
      }
      
      foundB = foundB + strlen("Order(col:");
      pos = foundB ;
      if (!findAToken(md, tblStr, pos, ",",
                      "populateSortCols::name:,###"))
        return NULL;
      NAText nameStr = tblStr->substr(foundB, pos-foundB);
      
      NAText orderStr;
      if(!extractValueStr(md, tblStr, pos, "order:", ",", 
                          orderStr, "populateSortCols::order:###"))
        return NULL;
      
      pos++;
      if (!findAToken(md, tblStr, pos, ",",
                      "populateSortColumns::comment:,###"))
        return NULL;
      
      hive_skey_desc* newSkey  = new (CmpCommon::contextHeap())
        struct hive_skey_desc(nameStr.c_str(),
                              colIdx,
                              atoi(orderStr.c_str()));
      
      if ( result == NULL ) {
        last = result = newSkey;
      } else {
        last->next_ = newSkey;
        last = newSkey;
      }
      
      colIdx++;
    } // end of while

  return result;
}

static int getAsciiDecimalValue(const char * valPtr)
{
  if (str_len(valPtr) <= 0) return 0;
  if (str_len(valPtr) == 1) return valPtr[0];
  return atoi(valPtr);
}

NABoolean populateSerDeParams(HiveMetaData *md, Int32 serdeID, 
                              char& fieldTerminator, char& recordTerminator,
                              NABoolean &nullFormatSpec, NAString &nullFormat,
                              NAText* tblStr, size_t& pos)
{

  fieldTerminator  = '\001';  // this the Hive default ^A or ascii code 1
  recordTerminator = '\n';    // this is the Hive default

  if (!findAToken(md, tblStr, pos, "serdeInfo:",
                  "populateSerDeParams::serdeInfo:###"))
    return FALSE;

  std::size_t foundB = pos;
  std::size_t foundE = pos;

  if (!findAToken(md, tblStr, foundE, "}),",
                  "populateSerDeParams::serDeInfo:)},###"))
    return FALSE;
  
  NAText serdeStr = tblStr->substr(foundB, foundE-foundB);

  const char * nullStr = "serialization.null.format=";
  const char * fieldStr = "field.delim=" ;
  const char * lineStr = "line.delim=" ;

  nullFormatSpec = FALSE;
  foundB = serdeStr.find(nullStr);
  if (foundB != std::string::npos)
    {
      nullFormatSpec = TRUE;
      std::size_t foundNB = foundB + strlen(nullStr);
      std::size_t foundNE = serdeStr.find(", ", foundNB);
      if (foundNE == std::string::npos)
        {
          foundNE = serdeStr.length();
        }
      nullFormat = NAString(serdeStr.substr(foundNB, (foundNE-foundNB)));
    }

  std::size_t foundDelim = serdeStr.find(fieldStr);
  if ((foundDelim != std::string::npos))
    fieldTerminator = serdeStr.at(foundDelim+strlen(fieldStr));

  foundDelim = serdeStr.find(lineStr);
  if ((foundDelim != std::string::npos))
    recordTerminator = serdeStr.at(foundDelim+strlen(lineStr));
  
  pos = foundE;
  
  return TRUE;
}

struct hive_bkey_desc* populateBucketingCols(HiveMetaData *md, Int32 sdID,  
                                             NAText* tblStr, size_t& pos)
{
  hive_bkey_desc* result = NULL;
  hive_bkey_desc* last = NULL;

  std::size_t foundB ;
  if (!findAToken(md, tblStr, pos, "bucketCols:",
                  "populateBucketingCols::bucketCols:###"))
    return NULL;

  std::size_t foundE = pos ;
  if (!findAToken(md, tblStr, foundE, "],",
                  "populateBucketingCols::bucketCols:],###"))
    return NULL;
  
  
  pos = pos + strlen("bucketCols:[");
  if (pos == foundE)
    return NULL ; // empty bucket cols list. This line is code is for 
  // clarity alone, the while condition alone is sufficient.
  
  Int32 colIdx = 0;
  while (pos < foundE)
    {
      foundB = tblStr->find(",", pos);
      if ((foundB == std::string::npos)||(foundB > foundE)) {
        foundB = foundE; // we have only one bucketing col or
        // this is the last bucket col
      }
      NAText nameStr = tblStr->substr(pos, foundB-pos);
      pos = foundB + 1;
      
      hive_bkey_desc* newBkey  = new (CmpCommon::contextHeap())
        struct hive_bkey_desc(nameStr.c_str(),
                              colIdx);
      
      if ( result == NULL ) {
        last = result = newBkey;
      } else {
        last->next_ = newBkey;
        last = newBkey;
      }
      
      colIdx++;
    } // end of while

  return result;
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

struct hive_tbl_desc* HiveMetaData::getFakedTableDesc(const char* tblName)
{
  CollHeap *h = CmpCommon::contextHeap();
  hive_column_desc* c1 = new (h) hive_column_desc(1, "C1", "int", 0);
  hive_column_desc* c2 = new (h) hive_column_desc(2, "C2", "string", 1);
  hive_column_desc* c3 = new (h) hive_column_desc(3, "C3", "float", 2);

   c1->next_ = c2;
   c2->next_ = c3;

   // sort key c1
   hive_skey_desc* sk1 = new (h) hive_skey_desc("C1", 1, 1);

   // bucket key c2
   hive_bkey_desc* bk1 = new (h) hive_bkey_desc("C2", 1);


   hive_sd_desc* sd1 = new (h)hive_sd_desc(1, "loc", 0, 1, "ift", "oft", NULL,
                                           hive_sd_desc::TABLE_SD, c1, 
                                           sk1, bk1, '\010', '\n',
                                           FALSE);

   hive_tbl_desc* tbl1 = new (h) hive_tbl_desc(1, "myHive", "default", "me",
                                               "MANAGED",
                                               0, NULL, NULL, sd1, 0);

   return tbl1;
}

struct hive_tbl_desc* HiveMetaData::getTableDesc(const char* schemaName,
                                                 const char* tblName)
{
    struct hive_tbl_desc *ptr = tbl_;

    while (ptr) {

      if ( !(strcmp(ptr->tblName_, tblName)
             ||strcmp(ptr->schName_, schemaName))) {
        if (validate(ptr->tblID_, ptr->redeftime(), schemaName, tblName))
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
          NADELETEBASIC(ptr, CmpCommon::contextHeap());
          ptr = NULL;
          break;
        }
      }

      ptr = ptr->next_;

   }

   // table not found in cache, try to read it from metadata
   hive_tbl_desc * result = NULL;
   Int64 creationTS;

   NAText* tblStr = new (CmpCommon::statementHeap()) string();
   if (!tblStr)
     return NULL;

   HVC_RetCode retCode = HiveClient_JNI::getHiveTableStr(schemaName, 
                                                  tblName, *tblStr);
   if ((retCode != HVC_OK) && (retCode != HVC_DONE)) {
     recordError((Int32)retCode, "HiveClient_JNI::getTableStr()");
     return NULL;
   }
   if (retCode == HVC_DONE) // table not found.
     return NULL;

   NAText tblNameStr;
   size_t pos = 0;
   if(!extractValueStr(this, tblStr, pos, "tableName:", ",", 
                       tblNameStr, "getTableDesc::tableName:###"))
     return NULL;
   
   NAText schNameStr;
   pos = 0;
   if(!extractValueStr(this, tblStr, pos, "dbName:", ",", 
                       schNameStr, "getTableDesc::dbName:###"))
     return NULL;
   
   NAText ownerStr;
   pos = 0;
   if(!extractValueStr(this, tblStr, pos, "owner:", ",", 
                       ownerStr, "getTableDesc:owner:###"))
     return NULL;

   NAText createTimeStr;
   pos = 0;
   if(!extractValueStr(this, tblStr, pos, "createTime:", ",", 
                       createTimeStr, "getTableDesc::createTime:###"))
     return NULL;

   creationTS = atol(createTimeStr.c_str());

   
   // TODO: need to handle multiple SDs
   struct hive_sd_desc* sd = populateSD(this, 0,0, tblStr, pos);
   if (!sd)
     return NULL;
   struct hive_pkey_desc* pkey = populatePartitionKey(this, 0, 
                                                      tblStr, pos);
   
   NAText tableTypeStr;
   pos = 0;
   if(!extractValueStr(this, tblStr, pos, "tableType:", ")", 
                       tableTypeStr, "getTableDesc:tableType:###"))
     return NULL;
   
   NAText viewOriginalStr;
   NAText viewExpandedStr;
   if ((NOT tableTypeStr.empty()) && (tableTypeStr == "VIRTUAL_VIEW"))
     {
       pos = 0;
       if(!extractValueStr(this, tblStr, pos, " viewOriginalText:", ", viewExpandedText:", 
                           viewOriginalStr, "getTableDesc:viewOriginalText:###"))
         return NULL;

       pos = 0;
       if(!extractValueStr(this, tblStr, pos, "viewExpandedText:", ", tableType:", 
                           viewExpandedStr, "getTableDesc:viewExpandedText:###"))
         return NULL;
     }

   result = 
     new (CmpCommon::contextHeap()) 
     struct hive_tbl_desc(0, // no tblID with JNI 
                          tblNameStr.c_str(), 
                          schNameStr.c_str(),
                          ownerStr.c_str(),
                          tableTypeStr.c_str(),
                          creationTS,
                          viewOriginalStr.c_str(),
                          viewExpandedStr.c_str(),
                          sd, pkey);
   
   // add the new table to the cache
   result->next_ = tbl_;
   tbl_ = result;
   
   
   //delete tblStr ;

   return result;
}

NABoolean HiveMetaData::validate(Int32 tableId, Int64 redefTS, 
                                 const char* schName, const char* tblName)
{
   NABoolean result = FALSE;

   // validate creation timestamp

   Int64 currentRedefTime = 0;
   HVC_RetCode retCode = HiveClient_JNI::getRedefTime(schName, tblName, 
                                                 currentRedefTime);
   if ((retCode != HVC_OK) && (retCode != HVC_DONE)) {
     return recordError((Int32)retCode, "HiveClient_JNI::getRedefTime()");
   }
   if ((retCode == HVC_DONE) || (currentRedefTime != redefTS))
     return result;
   else
     return TRUE;

  return result;
}

hive_tbl_desc::hive_tbl_desc(Int32 tblID, const char* name, const char* schName,
                             const char * owner,
                             const char * tableType,
                             Int64 creationTS, 
                             const char * viewOriginalText,
                             const char * viewExpandedText,
                             struct hive_sd_desc* sd,
                             struct hive_pkey_desc* pk)
     : tblID_(tblID), 
       viewOriginalText_(NULL), viewExpandedText_(NULL),
       sd_(sd), creationTS_(creationTS), pkey_(pk), next_(NULL)
{  
  tblName_ = strduph(name, CmpCommon::contextHeap());
  schName_ = strduph(schName, CmpCommon::contextHeap()); 

  if (owner)
    owner_ = strduph(owner, CmpCommon::contextHeap());
  else
    owner_ = NULL;

  if (tableType)
    tableType_ = strduph(tableType, CmpCommon::contextHeap());
  else
    tableType_ = NULL;

  if (isView())
    {
      if (viewOriginalText)
        viewOriginalText_ = strduph(viewOriginalText, CmpCommon::contextHeap());
      else
        viewOriginalText_ = NULL;

      if (viewExpandedText)
        viewExpandedText_ = strduph(viewExpandedText, CmpCommon::contextHeap());
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
  Int64 result = creationTS_;

  struct hive_sd_desc* sd = sd_;

  while (sd) {
    if (sd->creationTS_ > result)
      result = sd->creationTS_;
    sd = sd->next_;
  }
  return result;
}

hive_tbl_desc::~hive_tbl_desc()
{
  CollHeap *h = CmpCommon::contextHeap();
  if (tblName_)
    NADELETEBASIC(tblName_, h);
  if (schName_)
    NADELETEBASIC(schName_, h);

   hive_sd_desc* ptr ;
   while (sd_) {
    ptr = sd_->next_;       
    NADELETEBASIC(sd_, h);
    sd_ = ptr;
  }
 
   hive_pkey_desc* ptr1 ;
   while (pkey_) {
    ptr1 = pkey_->next_;       
    NADELETEBASIC(pkey_, h);
    pkey_ = ptr1;
  }
}

hive_sd_desc::~hive_sd_desc()
{
  CollHeap *h = CmpCommon::contextHeap();
  if (location_)
    NADELETEBASIC(location_, h);
  if (inputFormat_)
    NADELETEBASIC(inputFormat_, h);
  if (outputFormat_)
    NADELETEBASIC(outputFormat_, h);

  hive_column_desc* ptr ;
  while (column_) {
    ptr = column_->next_;       
    NADELETEBASIC(column_, h);
    column_ = ptr;
  }
 
  hive_skey_desc* ptr1 ;
  while (skey_) {
    ptr1 = skey_->next_;       
    NADELETEBASIC(skey_, h);
    skey_ = ptr1;
  }

  hive_bkey_desc* ptr2 ;
  while (bkey_) {
    ptr2 = bkey_->next_;       
    NADELETEBASIC(bkey_, h);
    bkey_ = ptr2;
  }
}


hive_pkey_desc::~hive_pkey_desc()
{
  CollHeap *h = CmpCommon::contextHeap();
  if (name_)
    NADELETEBASIC(name_, h);
  if (type_)
    NADELETEBASIC(type_, h);
}

hive_skey_desc::~hive_skey_desc()
{
  CollHeap *h = CmpCommon::contextHeap();
  if (name_)
    NADELETEBASIC(name_, h);
}

hive_bkey_desc::~hive_bkey_desc()
{
  CollHeap *h = CmpCommon::contextHeap();
  if (name_)
    NADELETEBASIC(name_, h);
}

hive_column_desc::~hive_column_desc()
{
  CollHeap *h = CmpCommon::contextHeap();
  if (name_)
    NADELETEBASIC(name_, h);
  if (type_)
    NADELETEBASIC(type_, h);
}
