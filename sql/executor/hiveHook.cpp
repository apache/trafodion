/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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
#include "hiveHook.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>

#undef _MSC_VER

#include "libdrizzle-5.1/libdrizzle.h"
#include "str.h"
#include "NAStringDef.h"
#include "HBaseClient_JNI.h"
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
                         NAText* tblStr, size_t& pos);

NABoolean findAToken (HiveMetaData *md, NAText* tblStr, size_t& pos, 
                      const char* tok, const char* errStr,
                      NABoolean raiseError = TRUE);

NABoolean extractValueStr (HiveMetaData *md, NAText* tblStr, size_t& pos, 
                           const char* beginTok, const char * endTok, 
                           NAText& valueStr, const char* errStr,
                           NABoolean raiseError = TRUE);


HiveMetaData::HiveMetaData(NABoolean jniVal) : tbl_(NULL),
                               currDesc_(NULL),
                               sqlConnection_(NULL),
                               errCode_(0) ,
                               errDetail_(NULL),
                               errMethodName_(NULL),
                               errCodeStr_(NULL),
                               useJNI_(jniVal),
                               client_(NULL)
{
}

HiveMetaData::~HiveMetaData()
{
  CollHeap *h = CmpCommon::contextHeap();

  disconnect();
  if (desc_.URL_)
    NADELETEBASIC((char *) desc_.URL_, h);
  if(desc_.userID_)
    NADELETEBASIC((char *) desc_.userID_, h);
  if(desc_.password_)
    NADELETEBASIC((char *) desc_.password_, h);
  if(desc_.schema_)
    NADELETEBASIC((char *) desc_.schema_, h);

  hive_tbl_desc* ptr ;
  while (tbl_) {
    ptr = tbl_->next_;       
    NADELETEBASIC(tbl_, h);
    tbl_ = ptr;
  }
}

NABoolean HiveMetaData::init(mysqlDesc &desc,
                             NABoolean readEntireSchema,
                             const char * hiveSchemaName,
			     const char * tabSearchPredStr)
{
  CollHeap *h = CmpCommon::contextHeap();
  if (!useJNI_) {
    if (desc.URL_)
      desc_.URL_ = strduph(desc.URL_, h);
    if(desc.userID_)
      desc_.userID_ = strduph(desc.userID_, h); 
    if(desc.password_)
      desc_.password_ = strduph(desc.password_, h); 
    if(desc.schema_)
      desc_.schema_ = strduph(desc.schema_, h);
  }

  /* Create a connection */
  if (!connect())
    return FALSE; // errCode_ should be set

  if (!readEntireSchema)
    return TRUE;

  if (!useJNI_)
  {
    char stmtBuf[500];
    drizzle_return_t ret;
    drizzle_result_st *res = NULL;
    drizzle_row_t row;

    // SQL statement to retrieve the names of all Hive tables
    if (tabSearchPredStr)
      snprintf(stmtBuf, 500,
               "SELECT TBL_NAME "
               "FROM TBLS "
               "WHERE TBLS.DB_ID IN (SELECT DB_ID "
               "FROM DBS "
               "WHERE NAME='%s') AND %s "
               "ORDER BY TBL_NAME DESC",
               hiveSchemaName,
               tabSearchPredStr);
    else
      snprintf(stmtBuf, 500,
               "SELECT TBL_NAME "
               "FROM TBLS "
               "WHERE TBLS.DB_ID IN (SELECT DB_ID "
               "FROM DBS "
               "WHERE NAME='%s') "
               "ORDER BY TBL_NAME DESC",
               hiveSchemaName);

    res = drizzle_query(sqlConnection_, stmtBuf, 0, &ret);
    if (ret != DRIZZLE_RETURN_OK)
      return recordError(ret, "drizzle_query for names of all tables");

    ret = drizzle_result_buffer(res);
    if (ret != DRIZZLE_RETURN_OK)
      return recordError(ret, "drizzle_result_buffer for all tables");

    while ((row = drizzle_row_next(res)))
      {
        getTableDesc(hiveSchemaName, row[0]);
      }

    drizzle_result_free(res);
  }
  else // useJNI
  {
    int i = 0 ;
    LIST(NAText *) tblNames(h);
    HVC_RetCode retCode = client_->getAllTables(hiveSchemaName, tblNames);
    if ((retCode != HVC_OK) && (retCode != HVC_DONE)) {
      return recordError((Int32)retCode, "HiveClient_JNI::getAllTables()");
    }
    
    while (i < tblNames.entries())
    {
      getTableDesc(hiveSchemaName, tblNames[i]->c_str());
      delete tblNames[i];
    }
  }
  currDesc_ = 0;
  //disconnect();

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

NABoolean HiveMetaData::connect()
{
  if (!useJNI_)
  {
    if (!sqlConnection_)
      {
        NAString host;
        Int32 port;
        NAString options;
        drizzle_return_t ret;
        drizzle_st *con;

        if (!splitURL(desc_.URL_,
                      host,
                      port,
                      options))
          return recordError(DRIZZLE_RETURN_MAX+1,"splitURL");

        // create a connection object
        con = drizzle_create(host,
                             port,
                             desc_.userID_,
                             desc_.password_,
                             desc_.schema_,
                             NULL);
        if (con == NULL)
          return recordError(DRIZZLE_RETURN_MAX+1,"drizzle_create");

        // create a connection
        ret = drizzle_connect(con);
        if (ret != DRIZZLE_RETURN_OK)
          return recordError(ret, "drizzle_connect");

        sqlConnection_ = con;
      }
    else
      return TRUE;
  }
  else // use JNI
  {
    if (!client_)
    {
      HiveClient_JNI* hiveClient = HiveClient_JNI::getInstance();
      if (hiveClient->isInitialized() == FALSE)
      {
        HVC_RetCode retCode = hiveClient->init();
        if (retCode != HVC_OK)
          return recordError((Int32)retCode, "HiveClient_JNI::init()");
      }
    
      if (hiveClient->isConnected() == FALSE)
      {
          Text metastoreURI("");
          HVC_RetCode retCode = 
            hiveClient->initConnection(metastoreURI.c_str());
          if (retCode != HVC_OK)
            return recordError((Int32)retCode, 
                               "HiveClient_JNI::initConnection()");
      }
      client_ = hiveClient;
    } // client_ exists
    return TRUE;
  } // we got a connection, either drizzle or JNI
  return TRUE;
}

NABoolean HiveMetaData::disconnect()
{
  if (!useJNI_)
  {
    if (sqlConnection_)
    {
      drizzle_return_t ret = drizzle_quit(sqlConnection_);
      if (ret != DRIZZLE_RETURN_OK) {
        recordError(ret, "drizzle_quit");
        sqlConnection_ = NULL;
        return FALSE;
      }
      else {
        sqlConnection_ = NULL;
        return TRUE;
      } 
    }
  }
  else  // use JNI
  {
    if (client_)
    {
      client_ = NULL; // client connections is owned by CliGlobals. 
      return TRUE; // do not disconnect from hiveMD
    }
  }

  return TRUE;
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
// keep this list of strings in sync with the enum drizzle_return_t defined
// in return.h of the drizzle library
static const char* const drizzleErrorEnumStr[] = 
{
  "DRIZZLE_RETURN_OK",
  "DRIZZLE_RETURN_IO_WAIT",
  "DRIZZLE_RETURN_PAUSE",
  "DRIZZLE_RETURN_ROW_BREAK",
  "DRIZZLE_RETURN_MEMORY",
  "DRIZZLE_RETURN_ERRNO",
  "DRIZZLE_RETURN_INTERNAL_ERROR",
  "DRIZZLE_RETURN_GETADDRINFO",
  "DRIZZLE_RETURN_NOT_READY",
  "DRIZZLE_RETURN_BAD_PACKET_NUMBER",
  "DRIZZLE_RETURN_BAD_HANDSHAKE_PACKET",
  "DRIZZLE_RETURN_BAD_PACKET",
  "DRIZZLE_RETURN_PROTOCOL_NOT_SUPPORTED",
  "DRIZZLE_RETURN_UNEXPECTED_DATA",
  "DRIZZLE_RETURN_NO_SCRAMBLE",
  "DRIZZLE_RETURN_AUTH_FAILED",
  "DRIZZLE_RETURN_NULL_SIZE",
  "DRIZZLE_RETURN_ERROR_CODE",
  "DRIZZLE_RETURN_TOO_MANY_COLUMNS",
  "DRIZZLE_RETURN_ROW_END",
  "DRIZZLE_RETURN_LOST_CONNECTION",
  "DRIZZLE_RETURN_COULD_NOT_CONNECT",
  "DRIZZLE_RETURN_NO_ACTIVE_CONNECTIONS",
  "DRIZZLE_RETURN_HANDSHAKE_FAILED",
  "DRIZZLE_RETURN_TIMEOUT",
  "DRIZZLE_RETURN_INVALID_ARGUMENT",
  "DRIZZLE_RETURN_SSL_ERROR",
  "DRIZZLE_RETURN_EOF",
  "DRIZZLE_RETURN_STMT_ERROR",
  "DRIZZLE_RETURN_BINLOG_CRC",
  "DRIZZLE_RETURN_TRUNCATED",
  "DRIZZLE_RETURN_INVALID_CONVERSION",
  "DRIZZLE_RETURN_NOT_FOUND",
  "DRIZZLE_RETURN_MAX",
  "NONE"
};

NABoolean HiveMetaData::recordError(Int32 errCode,
                                    const char *errMethodName)
{
  if (!useJNI_)
  {
    if (errCode != DRIZZLE_RETURN_OK)
      {
        errCode_ = errCode;
        errCodeStr_ = drizzleErrorEnumStr[errCode_];
        errMethodName_ = errMethodName;
        if (sqlConnection_)
          errDetail_ = drizzle_error(sqlConnection_);

        return FALSE;
      }
    return TRUE;
  }
  else
  {
    if (errCode != HVC_OK)
      {
        errCode_ = errCode;
        if (client_)
          errCodeStr_ = client_->getErrorText((HVC_RetCode)errCode_);
        errMethodName_ = errMethodName;
        errDetail_ = GetCliGlobals()->getJniErrorStrPtr();
        return FALSE;
      }
    return TRUE;
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

  if (!md->useJNI())
  {
    char query[400];
    int sdID, serdeID;
    drizzle_return_t ret;
    drizzle_result_st *res = NULL;
    drizzle_row_t row;
    size_t empty = 0;

    snprintf(query, 400,
             "SELECT SDS.SD_ID, LOCATION, NUM_BUCKETS, INPUT_FORMAT, OUTPUT_FORMAT, SERDE_ID, COALESCE(CREATE_TIME,0) "
             "from SDS LEFT OUTER JOIN PARTITIONS ON SDS.SD_ID = PARTITIONS.SD_ID AND PARTITIONS.TBL_ID = %d "
             "where SDS.SD_ID = %d ORDER BY SD_ID",
             tblID, mainSdID);

    res = drizzle_query(md->getConn(), query, 0, &ret);
    if (ret != DRIZZLE_RETURN_OK)
      {
        md->recordError(ret, 
                        "drizzle_query for select from SDS");
        return NULL;
      }

    ret = drizzle_result_buffer(res);
    if (ret != DRIZZLE_RETURN_OK)
      {
        md->recordError(ret, 
                        "drizzle_result_buffer for SDS");
        return NULL;
      }

    while ((row = drizzle_row_next(res)))
      {
        sdID = atoi(row[0]);

        struct hive_column_desc* newColumns = populateColumns(md, tblID, 
                                                              NULL, empty);
        struct hive_skey_desc* newSortCols = populateSortCols(md, sdID, 
                                                              NULL, empty);
        struct hive_bkey_desc* newBucketingCols = 
          populateBucketingCols(md, sdID, NULL, empty);
 
        serdeID = atoi(row[5]);
        populateSerDeParams(md, serdeID, fieldTerminator, recordTerminator, 
                            NULL, empty);

        struct hive_sd_desc* newSD = new (CmpCommon::contextHeap()) 
          struct hive_sd_desc(sdID, 
                              row[1],
                              atol(row[6]),
                              atoi(row[2]),
                              row[3],
                              row[4],
                              (sdID == mainSdID
                               ? hive_sd_desc::TABLE_SD
                               : hive_sd_desc::PARTN_SD),
                              newColumns, 
                              newSortCols, 
                              newBucketingCols,
                              fieldTerminator,
                              recordTerminator
                              );

        if (newSD->kind_ == hive_sd_desc::TABLE_SD)
          mainSD = newSD;
        else if ( result == NULL )
          last = result = newSD;
        else {
          last->next_ = newSD;
          last = newSD;
        }

      }

    // make sure the main SD for the table itself comes first
    mainSD->next_ = result;
    result = mainSD;

    drizzle_result_free(res);
  }
  else // use JNI
  {
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
    
    NAText numBucketsStr;
    if(!extractValueStr(md, tblStr, pos, "numBuckets:", ",", 
                        numBucketsStr, "populateSD:numBuckets:###"))
      return NULL;
    Int32 numBuckets = atoi(numBucketsStr.c_str());
    
    NABoolean success = populateSerDeParams(md, 0, fieldTerminator, 
                                            recordTerminator, tblStr, pos);
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
                          hive_sd_desc::TABLE_SD, 
                          // TODO : no support for hive_sd_desc::PARTN_SD
                          newColumns, 
                          newSortCols, 
                          newBucketingCols,
                          fieldTerminator,
                          recordTerminator
                          );

    // TODO : loop over SDs
    result = newSD;
  }

   return result;
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

  if (!md->useJNI())
  {
    drizzle_return_t ret;
    drizzle_result_st *res = NULL;
    drizzle_row_t row;
    char query[1000];

    snprintf(query, 1000,
             "SELECT COLUMN_NAME, TYPE_NAME, INTEGER_IDX from COLUMNS_V2 WHERE CD_ID = %d ORDER BY INTEGER_IDX", cdID);

    res = drizzle_query(md->getConn(), query, 0, &ret);
    if (ret != DRIZZLE_RETURN_OK)
      {
        md->recordError(ret, 
                        "drizzle_query for select from COLUMNS_V2");
        return NULL;
      }

    ret = drizzle_result_buffer(res);
    if (ret != DRIZZLE_RETURN_OK)
      {
        md->recordError(ret, 
                        "drizzle_result_buffer for select from COLUMNS_V2");
        return NULL;
      }

    while ((row = drizzle_row_next(res)))
      {
        struct hive_column_desc* newCol = new (CmpCommon::contextHeap())
          struct hive_column_desc(cdID, 
                                  row[0],
                                  row[1],
                                  atoi(row[2]));

        if ( result == NULL ) {
          last = result = newCol;
        } else {
          last->next_ = newCol;
          last = newCol;
        }
      }

    drizzle_result_free(res);
  }
  else // use JNI
  {
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
        if(!extractValueStr(md, tblStr, pos, "type:", ",", 
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
  } // end of JNI else

  return result;
}

struct hive_pkey_desc* populatePartitionKey(HiveMetaData *md, Int32 tblID,  
                                            NAText* tblStr, size_t& pos)
{
  hive_pkey_desc* result = NULL;
  hive_pkey_desc* last = NULL;

  if (!md->useJNI())
  {
    char query[400];
    drizzle_return_t ret;
    drizzle_result_st *res = NULL;
    drizzle_row_t row;

    snprintf(query, 400,
             "SELECT PKEY_NAME, PKEY_TYPE, INTEGER_IDX from PARTITION_KEYS WHERE TBL_ID = %d ORDER BY INTEGER_IDX", tblID);

    res = drizzle_query(md->getConn(), query, 0, &ret);
    if (ret != DRIZZLE_RETURN_OK)
      {
        md->recordError(ret, 
                        "drizzle_query for select from PARTITION_KEYS");
        return NULL;
      }

    ret = drizzle_result_buffer(res);
    if (ret != DRIZZLE_RETURN_OK)
      {
        md->recordError(ret, 
                        "drizzle_result_buffer for select from PARTITION_KEYS");
        return NULL;
      }

    while ((row = drizzle_row_next(res)))
      {
        hive_pkey_desc* newPkey = new (CmpCommon::contextHeap())
          hive_pkey_desc(row[0],
                         row[1],
                         atoi(row[2]));
      
        if ( result == NULL ) {
          last = result = newPkey;
        } else {
          last->next_ = newPkey;
          last = newPkey;
        }
      }

    drizzle_result_free(res);
  }
  else
  {
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
       if(!extractValueStr(md, tblStr, pos, "type:", ",", 
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
  }

  return result;
}

struct hive_skey_desc* populateSortCols(HiveMetaData *md, Int32 sdID,  
                                        NAText* tblStr, size_t& pos)
{
  hive_skey_desc* result = NULL;
  hive_skey_desc* last = NULL;

  if (!md->useJNI())
  {
    drizzle_return_t ret;
    drizzle_result_st *res = NULL;
    drizzle_row_t row;
    char query[400];
    snprintf(query, 400,
             "SELECT COLUMN_NAME, INTEGER_IDX, \"ORDER\" from SORT_COLS WHERE SD_ID = %d ORDER BY INTEGER_IDX", sdID);

    res = drizzle_query(md->getConn(), query, 0, &ret);
    if (ret != DRIZZLE_RETURN_OK)
      {
        md->recordError(ret, 
                        "drizzle_query for select from SORT_COLS");
        return NULL;
      }

    ret = drizzle_result_buffer(res);
    if (ret != DRIZZLE_RETURN_OK)
      {
        md->recordError(ret, 
                        "drizzle_result_buffer for select from SORT_COLS");
        return NULL;
      }

    while ((row = drizzle_row_next(res)))
      {
        hive_skey_desc* newSkey = new (CmpCommon::contextHeap())
          hive_skey_desc(row[0],
                         atoi(row[1]),
                         atoi(row[2]));

        if ( result == NULL ) {
          last = result = newSkey;
        } else {
          last->next_ = newSkey;
          last = newSkey;
        }
      }

    drizzle_result_free(res);
  }
  else // use JNI 
  {

    std::size_t foundB ;
    if (!findAToken(md, tblStr, pos, "sortCols:",
                    "populateSortCols::sortCols:###"))
      return NULL;

    std::size_t foundE = pos ;
    if (!findAToken(md, tblStr, foundE, "],",
                    "populateSortCols::sortCols:],###"))
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
  }

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
                              NAText* tblStr, size_t& pos)
{

  fieldTerminator  = '\001';  // this the Hive default ^A or ascii code 1
  recordTerminator = '\n';    // this is the Hive default

  if (!md->useJNI())
  {
    drizzle_return_t ret;
    drizzle_result_st *res = NULL;
    drizzle_row_t row;
    char query[400];
    snprintf(query, 400, "SELECT PARAM_KEY, PARAM_VALUE from SERDE_PARAMS WHERE SERDE_ID = %d", serdeID);

    res = drizzle_query(md->getConn(), query, 0, &ret);
    if (ret != DRIZZLE_RETURN_OK)
      {
        md->recordError(ret, 
                        "drizzle_query for select from SERDE_PARAMS");
        return FALSE;
      }

    ret = drizzle_result_buffer(res);
    if (ret != DRIZZLE_RETURN_OK)
      {
        md->recordError(ret, 
                        "drizzle_result_buffer for select from SERDE_PARAMS");
        return FALSE;
      }

    while ((row = drizzle_row_next(res)))
      {
        if ( strcmp(row[0], "field.delim") == 0 ) {
          fieldTerminator = getAsciiDecimalValue(row[1]);
        } else
          if ( strcmp(row[0], "record.delim") == 0 ) {
            recordTerminator = getAsciiDecimalValue(row[1]);
          }
      }

    drizzle_result_free(res);
  }
  else // use JNI
  {
    std::size_t foundB ;
    if (!findAToken(md, tblStr, pos, "serdeInfo:",
                    "populateSerDeParams::serdeInfo:###"))
      return NULL;

    std::size_t foundE = pos ;
    if (!findAToken(md, tblStr, foundE, "}),",
                    "populateSerDeParams::serDeInfo:)},###"))
      return NULL;
    
    
    const char * fieldStr = "field.delim" ;
    const char * lineStr = "line.delim" ;

    foundB = tblStr->find(fieldStr,pos);
    if ((foundB != std::string::npos) && (foundB < foundE))
      fieldTerminator = tblStr->at(foundB+strlen(fieldStr)+1);

    foundB = tblStr->find("line.delim=",pos);
    if ((foundB != std::string::npos) && (foundB < foundE))
      fieldTerminator = tblStr->at(foundB+strlen(lineStr)+1);

    pos = foundE;
     
  }
  return TRUE;
}

struct hive_bkey_desc* populateBucketingCols(HiveMetaData *md, Int32 sdID,  
                                             NAText* tblStr, size_t& pos)
{
  hive_bkey_desc* result = NULL;
  hive_bkey_desc* last = NULL;

  if (!md->useJNI())
  {
    drizzle_return_t ret;
    drizzle_result_st *res = NULL;
    drizzle_row_t row;
    char query[400];
    snprintf(query, 400,
             "SELECT BUCKET_COL_NAME, INTEGER_IDX from BUCKETING_COLS WHERE SD_ID = %d ORDER BY INTEGER_IDX", sdID);

    res = drizzle_query(md->getConn(), query, 0, &ret);
    if (ret != DRIZZLE_RETURN_OK)
      {
        md->recordError(ret, 
                        "drizzle_query for select from BUCKETING_COLS returned an error");
        return NULL;
      }

    ret = drizzle_result_buffer(res);
    if (ret != DRIZZLE_RETURN_OK)
      {
        md->recordError(ret, 
                        "drizzle_result_buffer for select from BUCKETING_COLS");
        return NULL;
      }

    while ((row = drizzle_row_next(res)))
      {
        hive_bkey_desc* newBkey = new (CmpCommon::contextHeap()) 
          hive_bkey_desc(row[0],
                         atoi(row[1]));

     
        if ( result == NULL ) {
          last = result = newBkey;
        } else {
          last->next_ = newBkey;
          last = newBkey;
        }
      }

    drizzle_result_free(res);
  }
  else // use JNI
  {
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
        pos = foundB;
        
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
  }

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


   hive_sd_desc* sd1 = new (h)hive_sd_desc(1, "loc", 0, 1, "ift", "oft", 
                                           hive_sd_desc::TABLE_SD, c1, 
                                           sk1, bk1, '\010', '\n');

   hive_tbl_desc* tbl1 = new (h) hive_tbl_desc(1, "myHive", "default", 
                                               0, sd1, 0);

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
   NABoolean needToConnect ;
   if (!useJNI_)
     needToConnect = (sqlConnection_ == NULL);
   else
     needToConnect = (client_ == NULL);

   /* Create a connection */
   if (needToConnect)
     if (!connect())
       return NULL;

   if (!useJNI_)
   {
     drizzle_return_t ret;
     drizzle_result_st *res = NULL;
     drizzle_row_t row;

     char query[1000];
     snprintf(query, 1000,
              "SELECT TBL_ID, SD_ID, CREATE_TIME, TBL_NAME, DBS.NAME "
              "from TBLS, DBS "
              "where TBLS.DB_ID= DBS.DB_ID and DBS.NAME = '%s' and TBLS.TBL_NAME='%s'",
              schemaName,       tblName);

     res = drizzle_query(sqlConnection_, query, 0, &ret);
     if (ret != DRIZZLE_RETURN_OK)
       {
         recordError(ret, 
                     "drizzle_query for select from TBLS, DBS");
         return NULL;
       }

     ret = drizzle_result_buffer(res);
     if (ret != DRIZZLE_RETURN_OK)
       {
         recordError(ret, 
                     "drizzle_result_buffer for select from TBLS DBS");
         return NULL;
       }

     int sdID, tblID;
     size_t empty;

     while ((row = drizzle_row_next(res)))
       {
         tblID = atoi(row[0]);
         sdID = atoi(row[1]);
         creationTS = atol(row[2]);

         struct hive_sd_desc* sd = populateSD(this, sdID, tblID, NULL, empty);
         struct hive_pkey_desc* pkey = populatePartitionKey(this, tblID, 
                                                            NULL, empty);

         result = 
           new (CmpCommon::contextHeap()) 
           struct hive_tbl_desc(tblID, 
                                row[3], 
                                row[4],
                                creationTS,
                                sd, pkey);

         // add the new table to the cache
         result->next_ = tbl_;
         tbl_ = result;
       }

     drizzle_result_free(res);
   }
   else //use JNI
   {
     NAText* tblStr = new (CmpCommon::statementHeap()) string();
     if (!tblStr)
       return NULL;

     HVC_RetCode retCode = client_->getHiveTableStr(schemaName, 
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

     result = 
       new (CmpCommon::contextHeap()) 
       struct hive_tbl_desc(0, // no tblID with JNI 
                            tblNameStr.c_str(), 
                            schNameStr.c_str(),
                            creationTS,
                            sd, pkey);

     // add the new table to the cache
     result->next_ = tbl_;
     tbl_ = result;
     

     //delete tblStr ;
   }

   // disconnect only if we connected in this method
   // do not disconnect if we are using JNI. Connections are expensive to obtain
   if (needToConnect && !useJNI_)
     disconnect();

   return result;
}

NABoolean HiveMetaData::validate(Int32 tableId, Int64 redefTS, 
                                 const char* schName, const char* tblName)
{
   NABoolean result = FALSE;

   // validate creation timestamp

   if (!connect())
     return FALSE;

   if(!useJNI_)
   {

     drizzle_return_t ret;
     drizzle_result_st *res = NULL;
     drizzle_row_t row;
     char query[400];

     snprintf(query, 400,
              "SELECT CASE WHEN MAX(PARTITIONS.CREATE_TIME) IS NULL THEN MAX(TBLS.CREATE_TIME) "
              "ELSE MAX(PARTITIONS.CREATE_TIME) END REDEF_TIME "
              "FROM TBLS LEFT OUTER JOIN PARTITIONS ON TBLS.TBL_ID = PARTITIONS.TBL_ID "
              "WHERE TBLS.TBL_ID = %d",
              tableId);

     res = drizzle_query(sqlConnection_, query, 0, &ret);
     if (ret != DRIZZLE_RETURN_OK)
       {
         recordError(ret, "drizzle_query for validate");
         return FALSE;
       }

     ret = drizzle_result_buffer(res);
     if (ret != DRIZZLE_RETURN_OK)
       {
         recordError(ret, "drizzle_result_buffer for validate");
         return FALSE;
       }

     if ((row = drizzle_row_next(res)))
       if (row[0] && (redefTS == atol(row[0])))
         result = TRUE;

     // if the value returned by the query is NULL then row[0] is the null ptr.
     // we return false in that case.

     drizzle_result_free(res);
   }
   else
   {
     Int64 currentRedefTime = 0;
     HVC_RetCode retCode = client_->getRedefTime(schName, tblName, 
                                                 currentRedefTime);
    if ((retCode != HVC_OK) && (retCode != HVC_DONE)) {
      return recordError((Int32)retCode, "HiveClient_JNI::getRedefTime()");
    }
    if ((retCode == HVC_DONE) || (currentRedefTime != redefTS))
      return result;
    else
      return TRUE;
   }

  return result;
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
