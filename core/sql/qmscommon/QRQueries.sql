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
/* -*-C++-*-
  ************************************************************************
  *
  * File:          QRQueries.sql
  * Description:   SQL operations for Query Rewrite queries
  *
  * Created:      04/01/2009
  * Language:     C++
  *
  *
 *****************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "QRQueries.h"
#include "Platform.h"  //64-bit project
#include "NAWinNT.h"
#include "wstr.h"
#include "csconvert.h"
#include "catapirequest.h"

EXEC SQL MODULE HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.MVQR_N29_000 NAMES ARE ISO88591;

/* For CharSet project: add 3 CQDs */ 
EXEC SQL CONTROL QUERY DEFAULT COMP_BOOL_58  'ON';
EXEC SQL CONTROL QUERY DEFAULT ISO_MAPPING   'ISO88591';
EXEC SQL CONTROL QUERY DEFAULT DEFAULT_CHARSET  'ISO88591';

/* For STREAM_TIMEOUT for stream reading from the REWRITE_PUBLISH table */
EXEC SQL CONTROl QUERY DEFAULT STREAM_TIMEOUT '500';

EXEC SQL BEGIN DECLARE SECTION;

static Int32 SQLCODE;
static Int32 result;
 
#define MAX_NODE_NAME 9
#define MAX_CATSYS_NAME 50                // + "CATSYS" = 41+1+1 = 43
#define MAX_SCHEMATA_NAME 50              // + "SCHEMATA" = 43+1+1 = 45
#define MAX_1_PART_INTERNAL_NAME_LEN         128
#define MAX_1_PART_INTERNAL_NAME_LEN_PLUS_1  129
/* 
#define MAX_SMD_TABLE_NAME 290            // 258+.HP_DEFINITION_SCHEMA.yyyyyyyy=258+20+10+2 = 290
#define MAX_CATALOG_NAME 260              // Allows for double quotes in delimited
#define MAX_CATALOG_DEFINITION_NAME 290   // Allows for double quotes in delimited
*/
/* begin */
#define MAX_SMD_TABLE_NAME 546            // 514+.HP_DEFINITION_SCHEMA.yyyyyyyy=514+20+10+2 = 546
#define MAX_CATALOG_NAME 516              // Allows for double quotes in delimited
#define MAX_CATALOG_DEFINITION_NAME 546   // Allows for double quotes in delimited
/*  end */
#define MAX_SCHEMA_VERSION 5
#define MAX_SYSTEM_DEFAULTS 66
#define MAX_ATTR_VALUE 9
#define MAX_TABLE_ATTR 3
/* 
#define MAX_MV_TEXT 3001
*/
/*  Unicode - begin */
#define MAX_MV_UTF8_TEXT_IN_BYTES    12001
#define MAX_MV_UCS2_TEXT_SIZE         3000
#define MAX_MV_UCS2_TEXT_SIZE_PLUS_1  3001
/* Unicode - end */
#define MAX_REWRITE_TABLE 50
#define MAX_OPERATION_TYPE 3
/* 
#define MAX_OBJECT_NAME 500
*/
/*  Unicode - begin */
#define MAX_OBJECT_NAME_IN_NAWCHARS 500
#define MAX_OBJECT_NAME 2001
/*  Unicode - end */
#define MAX_DEFAULTS_VALUE 1000
/* Unicode - begin */
#define MAX_DEFAULTS_UTF8_VALUE_IN_BYTES 4001
#define MAX_UTF8_CMD_LEN_IN_BYTES_PLUS_1 4001
/* Unicode - end */


struct QRMVDataStruct
{
  _int64 objectUID_; 
  _int64 redefTime_;
  _int64 refreshedAt_;
  Int32 hasIgnoreChanges_;
  char mvText_[MAX_MV_UTF8_TEXT_IN_BYTES];
};

struct MVQR_PublishStruct
{
  _int64 operationTimestamp_; 
  _int64 redefTime_;
  _int64 refreshedAt_; 
  _int64 objectUID_; 
  _int64 catalogUID_;                  
  char objectName_[MAX_OBJECT_NAME];
  char objectNewName_[MAX_OBJECT_NAME];
  Int32 descriptorIndex_;
  char operationType_[MAX_OPERATION_TYPE];
  char ignoreChangesUsed_[MAX_OPERATION_TYPE];
  short nullindObjectNewName_;
  short nullindIgnoreChangesUsed_;
  short nullindDescriptorIndex_;
};

char catsysName_[MAX_CATSYS_NAME];
char schemataName_[MAX_CATSYS_NAME];
char systemDefaults_[MAX_SYSTEM_DEFAULTS];       

EXEC SQL END DECLARE SECTION;

#define QRMVDataStruct QRMVData
#define MVQR_PublishStruct MVQR_Publish

/*********************************************************
// FIX_CHAR macro
// Strips trailing blanks from string
//
*********************************************************/
#define FIX_CHAR(AAA) \
{                 \
 AAA[sizeof(AAA) -1] = '\0';  \
 char *tmp = strrchr(AAA, ' '); \
 if (tmp)             \
 {                    \
   while(tmp > AAA && *(tmp-1) == ' ') tmp--; \
   *tmp = '\0';       \
 }                    \
}

/*********************************************************
// FIX_CHAR2 macro
// Appends zero-teminator to string retaining maximum size
//
*********************************************************/
#define FIX_CHAR2(AAA) AAA[sizeof(AAA) -1] = '\0'

/*********************************************************
// Strip trailing NAWchar blanks
*********************************************************/
static void StripTrailingBlanks(NAWchar *x, const Int32 bufSizeInNAWchars)
{
  Int32 i=bufSizeInNAWchars-1;
  x[i]=0; /* Make sure there is a NULL terminator*/
  i -= 1;  
  Int32 more=1/*TRUE*/;
  while(more)
  {
    if(x[i]==' ')
      x[i]=0;
    else
      more=0/*FALSE*/;
    i -= 1;
  }
}

/*********************************************************
// Initialization method: set the full name of the CATSYS table.
*********************************************************/
void QRQueries::setCatsysName(char *name)
{
  strcpy(catsysName_, name);
}

/*********************************************************
// Initialization method: set the full name of the SCHEMATA table.
*********************************************************/
void QRQueries::setSchemataName(char *name)
{
  strcpy(schemataName_, name);
}

/*********************************************************
// Initialization method: set the full name of the system DEFAULTS table.
*********************************************************/
void QRQueries::setSystemDefaultsName(char *name)
{
  strcpy(systemDefaults_, name);
}

/*********************************************************
//
// QRQueries::beginTransaction()
//
*********************************************************/
Int32 QRQueries::beginTransaction()
{
  EXEC SQL BEGIN WORK;

  return SQLCODE;
}

/*********************************************************
//
// QRQueries::commitTransaction()
//
*********************************************************/
Int32 QRQueries::commitTransaction()
{
  EXEC SQL COMMIT WORK;

  return SQLCODE;
}

/*********************************************************
//
// QRQueries::rollbackTransaction()
//
*********************************************************/
Int32 QRQueries::rollbackTransaction()
{
  EXEC SQL ROLLBACK WORK;

  return SQLCODE;
}

/*********************************************************
//
// QRQueries::openSystemDefault
//
*********************************************************/
Lng32 QRQueries::openSystemDefault(const char* defaultName)
{
  EXEC SQL BEGIN DECLARE SECTION;
    char CHARACTER SET IS ISO88591 defName[MAX_DEFAULTS_VALUE];
  EXEC SQL END DECLARE SECTION;
  
  memset (defName, ' ', MAX_DEFAULTS_VALUE);
  memcpy (defName, defaultName, strlen(defaultName));
 
  EXEC SQL DECLARE ObtainSystemDefaults CURSOR FOR
    select translate(attr_value using ucs2toutf8), octet_length(translate(attr_value using ucs2toutf8))
    from :systemDefaults_ prototype 'HP_SYSTEM_CATALOG.SYSTEM_DEFAULTS_SCHEMA.SYSTEM_DEFAULTS'
     where attribute = :defName;
   
  EXEC SQL OPEN ObtainSystemDefaults;

  return SQLCODE;
}

/*********************************************************
// QRQueries::fetchSystemDefault
*********************************************************/
Lng32 QRQueries::fetchSystemDefault(char* value)
{
  EXEC SQL BEGIN DECLARE SECTION;
    char defaultValue[MAX_DEFAULTS_UTF8_VALUE_IN_BYTES];
    Int32 textSize;
  EXEC SQL END DECLARE SECTION;
   
  EXEC SQL FETCH ObtainSystemDefaults INTO :defaultValue,
                                           :textSize;
   
  if (SQLCODE == 0)
  {
    assertLogAndThrow(CAT_QR_COMMON, LL_ERROR,
                      textSize < MAX_DEFAULTS_VALUE, QRDatabaseException,
                      "System default value is too large");
    memcpy(value, defaultValue, textSize);
    value[textSize] = '\0';
  }
   
  return SQLCODE;
}

/*********************************************************
// QRQueries::closeSystemDefault
*********************************************************/
Int32 QRQueries::closeSystemDefault()
{
  EXEC SQL CLOSE ObtainSystemDefaults;

  return SQLCODE;
}

/*********************************************************
//
// QRQueries::openCatalogName
//
*********************************************************/
Lng32 QRQueries::openCatalogName(_int64 catalogUID)
{
  EXEC SQL BEGIN DECLARE SECTION;
   _int64 catUID;
  EXEC SQL END DECLARE SECTION;
 
  catUID = catalogUID;
 
  EXEC SQL DECLARE ObtainCatalogName CURSOR FOR
   SELECT rtrim(cat_name) from :catsysName_ prototype 'HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.CATSYS'
    where cat_uid = :catUID 
   FOR READ COMMITTED ACCESS IN SHARE MODE;

  EXEC SQL OPEN ObtainCatalogName;

  return SQLCODE;
}

/*********************************************************
// QRQueries::fetchCatalogName
*********************************************************/
Lng32 QRQueries::fetchCatalogName(NAString& catalogName)
{
  EXEC SQL BEGIN DECLARE SECTION;
    char CHARACTER SET IS UCS2 catName[MAX_1_PART_INTERNAL_NAME_LEN_PLUS_1];
  EXEC SQL END DECLARE SECTION;
 
  /* memset (catName, ' ', MAX_CATALOG_NAME); */
  wc_str_pad(catName, MAX_1_PART_INTERNAL_NAME_LEN);
  ((NAWchar*)catName)[MAX_1_PART_INTERNAL_NAME_LEN] = 0;
 
  EXEC SQL FETCH ObtainCatalogName INTO :catName;
  
  if (SQLCODE == 0)
  {
    /* FIX_CHAR(catName); */
    StripTrailingBlanks(catName, MAX_1_PART_INTERNAL_NAME_LEN_PLUS_1);
    char tmpCatName[MAX_CATALOG_NAME+1];
    tmpCatName[MAX_CATALOG_NAME] = 0;
    char * p1stUnstranslatedChar = NULL;
    UInt32 outStrLenInBytes = 0;
    UInt32 charCount = 0;  /* number of characters translated/converted */
    Int32 cnvErrStatus = 0;
    char *pSubstitutionChar = NULL; /* Use ? */
    Int32 convFlags = 0;
    cnvErrStatus = UTF16ToLocale
                    ( cnv_version1              /* in  - const enum cnv_version version */
                    , (const char *)catName     /* in  - const char *in_bufr */
                    , NAWstrlen(catName)*BYTES_PER_NAWCHAR /* in  - const int in_len */
                    , tmpCatName                /* out - const char *out_bufr */
                    , MAX_CATALOG_NAME          /* in  - const int out_len */
                    , cnv_UTF8                  /* in  - enum cnv_charset charset */
                    , p1stUnstranslatedChar     /* out - char * & first_untranslated_char */
                    , &outStrLenInBytes         /* out - unsigned int *output_data_len_p */
                    , convFlags                 /* in  - const int cnv_flags */
                    , (Int32)TRUE               /* in  - const int addNullAtEnd_flag */
                    , (Int32)TRUE               /* in  - const int allow_invalids */
                    , &charCount                /* out - unsigned int * translated_char_cnt_p */
                    , pSubstitutionChar         /* in  - const char *substitution_char */
                    );
    /* Conversion from UTF16 to UTF8 should always be successful */
    catalogName = tmpCatName;
  }

  return SQLCODE;
}

/*********************************************************
// QRQueries::closeCatalogName
*********************************************************/
Int32 QRQueries::closeCatalogName()
{
  EXEC SQL CLOSE ObtainCatalogName;

  return SQLCODE;
}

/*********************************************************
//
// QRQueries::openCatalogUID
//
*********************************************************/
Lng32 QRQueries::openCatalogUID(const char *catalogName)
{
  EXEC SQL BEGIN DECLARE SECTION;
    char catName[MAX_CATALOG_NAME];
  EXEC SQL END DECLARE SECTION;
  
  memset (catName, ' ', MAX_CATALOG_NAME);
  memcpy (catName, catalogName, strlen(catalogName));
 
  EXEC SQL DECLARE ObtainCatalogUID CURSOR FOR
    SELECT cat_uid from :catsysName_ prototype 'HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.CATSYS'
    where cat_name = translate(:catName using utf8toucs2)
 FOR READ COMMITTED ACCESS IN SHARE MODE;

  EXEC SQL OPEN ObtainCatalogUID;

  return SQLCODE;
}

/*********************************************************
// QRQueries::fetchCatalogUID
*********************************************************/
Lng32 QRQueries::fetchCatalogUID(_int64& catalogUID)
{
  EXEC SQL BEGIN DECLARE SECTION;
    _int64 catUID;
 EXEC SQL END DECLARE SECTION;

 EXEC SQL FETCH ObtainCatalogUID INTO
    :catUID;
 
  catalogUID = catUID;
  
  return SQLCODE;
}

/*********************************************************
// QRQueries::closeCatalogUID
*********************************************************/
Int32 QRQueries::closeCatalogUID()
{
  EXEC SQL CLOSE ObtainCatalogUID;

  return SQLCODE;
}

/*********************************************************
//
// QRQueries::openVersion
//
*********************************************************/
Lng32 QRQueries::openVersion(_int64 catalogUID)
{
  EXEC SQL BEGIN DECLARE SECTION;
    _int64 catUID;
  EXEC SQL END DECLARE SECTION;
  
  catUID = catalogUID;
 
  EXEC SQL DECLARE ObtainSystemSchemaVersion CURSOR FOR
    SELECT DISTINCT schema_version from :schemataName_ prototype 'HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.SCHEMATA'
    where CAT_UID = :catUID
  FOR READ COMMITTED ACCESS IN SHARE MODE;

  EXEC SQL OPEN ObtainSystemSchemaVersion;

  return SQLCODE;
}

/*********************************************************
// QRQueries::fetchVersion
*********************************************************/
Lng32 QRQueries::fetchVersion(Int32& version)
{
  EXEC SQL BEGIN DECLARE SECTION;
    Int32 ver;
  EXEC SQL END DECLARE SECTION;
 
  EXEC SQL FETCH ObtainSystemSchemaVersion INTO :ver;
  
  version = ver;
  
  return SQLCODE;
}

/*********************************************************
// QRQueries::closeVersion
*********************************************************/
Int32 QRQueries::closeVersion()
{
  EXEC SQL CLOSE ObtainSystemSchemaVersion;

  return SQLCODE;
}

/*********************************************************
//
// QRQueries::openCatalogNames
//
*********************************************************/
Lng32 QRQueries::openCatalogNames()
{
 EXEC SQL DECLARE ObtainCatalogNames CURSOR FOR
    SELECT rtrim(cat_name) from :catsysName_ prototype 'HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.CATSYS'
    FOR READ COMMITTED ACCESS IN SHARE MODE ORDER BY 1;

  EXEC SQL OPEN ObtainCatalogNames;

  return SQLCODE;
}

/*********************************************************
// QRQueries::fetchCatalogNames
*********************************************************/
Lng32 QRQueries::fetchCatalogNames(NAString& catalogName)
{
  EXEC SQL BEGIN DECLARE SECTION;
    char CHARACTER SET IS UCS2 catName[MAX_1_PART_INTERNAL_NAME_LEN_PLUS_1];
  EXEC SQL END DECLARE SECTION;  
  
  wc_str_pad(catName, MAX_1_PART_INTERNAL_NAME_LEN);
  ((NAWchar*)catName)[MAX_1_PART_INTERNAL_NAME_LEN] = 0;

  EXEC SQL FETCH ObtainCatalogNames INTO :catName;
  
  if (SQLCODE == 0)
  {
    /* FIX_CHAR(catName); */
    StripTrailingBlanks(catName, MAX_1_PART_INTERNAL_NAME_LEN_PLUS_1);
    char tmpCatName[MAX_CATALOG_NAME+1];
    tmpCatName[MAX_CATALOG_NAME] = 0;
    char * p1stUnstranslatedChar = NULL;
    UInt32 outStrLenInBytes = 0;
    UInt32 charCount = 0;  /* number of characters translated/converted */
    Int32 cnvErrStatus = 0;
    char *pSubstitutionChar = NULL; /* Use ? */
    Int32 convFlags = 0;
    cnvErrStatus = UTF16ToLocale
                    ( cnv_version1              /* in  - const enum cnv_version version */
                    , (const char *)catName     /* in  - const char *in_bufr */
                    , NAWstrlen(catName)*BYTES_PER_NAWCHAR /* in  - const int in_len */
                    , tmpCatName                /* out - const char *out_bufr */
                    , MAX_CATALOG_NAME          /* in  - const int out_len */
                    , cnv_UTF8                  /* in  - enum cnv_charset charset */
                    , p1stUnstranslatedChar     /* out - char * & first_untranslated_char */
                    , &outStrLenInBytes         /* out - unsigned int *output_data_len_p */
                    , convFlags                 /* in  - const int cnv_flags */
                    , (Int32)TRUE               /* in  - const int addNullAtEnd_flag */
                    , (Int32)TRUE               /* in  - const int allow_invalids */
                    , &charCount                /* out - unsigned int * translated_char_cnt_p */
                    , pSubstitutionChar         /* in  - const char *substitution_char */
                    );
    /* Conversion from UTF16 to UTF8 should always be successful */
    catalogName = tmpCatName;
  }

  return SQLCODE;
}

/*********************************************************
// QRQueries::closeCatalogNames
*********************************************************/
Int32 QRQueries::closeCatalogNames()
{
  EXEC SQL CLOSE ObtainCatalogNames;

  return SQLCODE;
}
                       
/*********************************************************
//
// QRQueries::openMvUIDs
//
*********************************************************/
Lng32 QRQueries::openMvInformation(const NAString& definitionSchema)
{
  EXEC SQL BEGIN DECLARE SECTION;
    char objectsTable[MAX_SMD_TABLE_NAME];
    char mvsTable[MAX_SMD_TABLE_NAME];
    char mvsUsedTable[MAX_SMD_TABLE_NAME];
  EXEC SQL END DECLARE SECTION;
   
  memset (objectsTable, ' ', MAX_SMD_TABLE_NAME);
  strcpy (objectsTable, definitionSchema);
  strcat (objectsTable, ".OBJECTS");
 
  memset (mvsTable, ' ', MAX_SMD_TABLE_NAME);
  strcpy (mvsTable, definitionSchema);
  strcat (mvsTable, ".MVS");
 
  memset (mvsUsedTable, ' ', MAX_SMD_TABLE_NAME);
  strcpy (mvsUsedTable, definitionSchema);
  strcat (mvsUsedTable, ".MVS_USED");
  
  EXEC SQL DECLARE ObtainMvUID CURSOR FOR
    select object_uid, redef_time, refreshed_at, sum(is_IC) has_IC
    from (select o1.object_uid, 
                 o1.redef_time, 
                 refreshed_at,  
                case table_attributes when 'NO' then 0 
                                      when 'IC' then 1
                                      end as is_IC
          from  :objectsTable prototype 'HP_SYSTEM_CATALOG.HP_DEFINITION_SCHEMA.OBJECTS'  o1,
                :objectsTable prototype 'HP_SYSTEM_CATALOG.HP_DEFINITION_SCHEMA.OBJECTS' o2,
                :mvsTable prototype 'HP_SYSTEM_CATALOG.HP_DEFINITION_SCHEMA.MVS' mvs,                   
                :mvsUsedTable prototype 'HP_SYSTEM_CATALOG.HP_DEFINITION_SCHEMA.MVS_USED' mvs_used                    
              where o1.object_type = _iso88591'MV' and 
                    o1.object_uid = mvs.mv_uid and  
                    o1.object_uid = mvs_used.mv_uid and                                    
                    mvs_used.used_object_uid = o2.object_uid and
                    mvs.REWRITE_ENABLED = 'Y'
                FOR SKIP CONFLICT ACCESS IN SHARE MODE
          ) T (object_uid, redef_time, refreshed_at, is_IC)
    group by object_uid, redef_time, refreshed_at;

  EXEC SQL OPEN ObtainMvUID;

  return SQLCODE;
}

/*********************************************************
// QRQueries::fetchMvUIDs
*********************************************************/
Lng32 QRQueries::fetchMvInformation(QRMVData *data)
{
  EXEC SQL BEGIN DECLARE SECTION;
    QRMVDataStruct *hostVarP = data;
  EXEC SQL END DECLARE SECTION;  
  
  EXEC SQL FETCH ObtainMvUID INTO :hostVarP->objectUID_,
                                  :hostVarP->redefTime_,
                                  :hostVarP->refreshedAt_,
                                  :hostVarP->hasIgnoreChanges_;
                                  
  return SQLCODE;
}

/*********************************************************
// QRQueries::closeMvUIDs
*********************************************************/
Lng32 QRQueries::closeMvInformation()
{
  EXEC SQL CLOSE ObtainMvUID;

  return SQLCODE;
}

/*********************************************************
//
// QRQueries::openMvDescriptorText
//
*********************************************************/
Lng32 QRQueries::openMvDescriptorText(const NAString& textTable, 
                                     _int64 objectUID)
{
  EXEC SQL BEGIN DECLARE SECTION;
    char txtTbl[MAX_SMD_TABLE_NAME];
    _int64 objUID;      
  EXEC SQL END DECLARE SECTION;
   
  memset (txtTbl, ' ', MAX_SMD_TABLE_NAME);
  memcpy (txtTbl, textTable, strlen(textTable));
  
  objUID = objectUID;
  
  EXEC SQL DECLARE ObtainMvDescriptorText CURSOR FOR 
          select TEXT, character_length(TEXT)
    from  :txtTbl prototype 'HP_SYSTEM_CATALOG.HP_DEFINITION_SCHEMA.TEXT' t
    where t.object_uid = :objUID and                     
              t.object_sub_id = -2                      
        FOR READ COMMITTED ACCESS IN SHARE MODE order by sequence_num;  

  EXEC SQL OPEN ObtainMvDescriptorText;

  return SQLCODE;
}

/*********************************************************
// QRQueries::fetchMvDescriptorText
*********************************************************/
Int32 QRQueries::fetchMvDescriptorText(QRMVData *data)
{
  EXEC SQL BEGIN DECLARE SECTION;
    QRMVDataStruct *hostVarP = data;
    Int32 textSizeInNAWchars = 0;
    char CHARACTER SET IS UCS2 mvDescText[MAX_MV_UCS2_TEXT_SIZE_PLUS_1];
  EXEC SQL END DECLARE SECTION;  
  
  mvDescText[MAX_MV_UCS2_TEXT_SIZE] = 0;
  EXEC SQL FETCH ObtainMvDescriptorText INTO :mvDescText
                                            ,:textSizeInNAWchars;
  if (textSizeInNAWchars <= 0)
  {
    hostVarP->mvText_[0] = 0;
    return SQLCODE;
  }

  hostVarP->mvText_[MAX_MV_UTF8_TEXT_IN_BYTES - 1] = 0;

  char * p1stUnstranslatedChar = NULL;
  UInt32 outStrLenInBytes = 0;
  UInt32 charCount = 0;  /* number of characters translated/converted */
  Int32 cnvErrStatus = 0;
  char *pSubstitutionChar = NULL; /* Use ? */
  Int32 convFlags = 0;
  cnvErrStatus = UTF16ToLocale
                  ( cnv_version1              /* in  - const enum cnv_version version */
                  , (const char *)mvDescText  /* in  - const char *in_bufr */
                  , textSizeInNAWchars*BYTES_PER_NAWCHAR /* in  - const int in_len */
                  , hostVarP->mvText_         /* out - const char *out_bufr */
                  , MAX_MV_UTF8_TEXT_IN_BYTES /* in  - const int out_len */
                  , cnv_UTF8                  /* in  - enum cnv_charset charset */
                  , p1stUnstranslatedChar     /* out - char * & first_untranslated_char */
                  , &outStrLenInBytes         /* out - unsigned int *output_data_len_p */
                  , convFlags                 /* in  - const int cnv_flags */
                  , (Int32)TRUE               /* in  - const int addNullAtEnd_flag */
                  , (Int32)TRUE               /* in  - const int allow_invalids */
                  , &charCount                /* out - unsigned int * translated_char_cnt_p */
                  , pSubstitutionChar         /* in  - const char *substitution_char */
                  );

  return SQLCODE;
}

/*********************************************************
// QRQueries::closeMvDescriptorText
*********************************************************/
Int32 QRQueries::closeMvDescriptorText()
{
  EXEC SQL CLOSE ObtainMvDescriptorText;

  return SQLCODE;
}

/*********************************************************
//
// QRQueries::openRewritePublish
//
*********************************************************/
Lng32 QRQueries::openRewritePublish(const char* rewriteTableName)
{
  EXEC SQL BEGIN DECLARE SECTION;
   char rewriteTable[MAX_REWRITE_TABLE];
  EXEC SQL END DECLARE SECTION;
  
  memset (rewriteTable, ' ', MAX_REWRITE_TABLE);
  memcpy (rewriteTable, rewriteTableName, strlen(rewriteTableName));
 
  EXEC SQL DECLARE ObtainRewritePublish CURSOR FOR
    SELECT operation_timestamp, 
           redef_time, 
           refresh_at_time, 
           object_uid,
           catalog_uid, 
           object_name,
           object_new_name,
           descriptor_index,
           operation_type, 
           ignore_changes_used
    FROM (delete from :rewriteTable prototype 'HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.REWRITE_PUBLISH'
          FOR SKIP CONFLICT ACCESS) as T
    ORDER BY OPERATION_TIMESTAMP;
   
  EXEC SQL OPEN ObtainRewritePublish;

  return SQLCODE;
}

/*********************************************************
// QRQueries::fetchRewritePublish
*********************************************************/
Lng32 QRQueries::fetchRewritePublish(MVQR_Publish *publish)
{
  EXEC SQL BEGIN DECLARE SECTION;
    MVQR_PublishStruct *hostVarP = publish;
    char CHARACTER SET UCS2 objectName   [MAX_OBJECT_NAME_IN_NAWCHARS+1]; /* plus 1 extra NAWchar */
    char CHARACTER SET UCS2 objectNewName[MAX_OBJECT_NAME_IN_NAWCHARS+1]; /* plus 1 extra NAWchar */
  EXEC SQL END DECLARE SECTION;

  /* Unicode - begin */
  wc_str_pad((NAWchar*)objectName   , MAX_OBJECT_NAME_IN_NAWCHARS);
  wc_str_pad((NAWchar*)objectNewName, MAX_OBJECT_NAME_IN_NAWCHARS);
  ((NAWchar*)objectName   )[MAX_OBJECT_NAME_IN_NAWCHARS] = 0; // add a NULL terminator
  ((NAWchar*)objectNewName)[MAX_OBJECT_NAME_IN_NAWCHARS] = 0; // add a NULL terminator
  /* Unicode - end */

 EXEC SQL FETCH ObtainRewritePublish INTO
   :hostVarP->operationTimestamp_,
   :hostVarP->redefTime_,
   :hostVarP->refreshedAt_,
   :hostVarP->objectUID_,
   :hostVarP->catalogUID_,
   :objectName,
   :objectNewName INDICATOR :hostVarP->nullindObjectNewName_,
   :hostVarP->descriptorIndex_ INDICATOR :hostVarP->nullindDescriptorIndex_,
   :hostVarP->operationType_,
   :hostVarP->ignoreChangesUsed_ INDICATOR :hostVarP->nullindIgnoreChangesUsed_;
   
  if ((SQLCODE >= 0) && (SQLCODE != 100))
   {
     /* Old code - Keep it here for comparison purposes
     FIX_CHAR(hostVarP->objectName_);
     FIX_CHAR(hostVarP->objectNewName_);
     */
     /* Unicode - begin */
     NAWchar tmpObjName[MAX_OBJECT_NAME_IN_NAWCHARS+1]; /* plus 1 extra NAWchar */
     NAWstrncpy(tmpObjName, (const NAWchar *)objectName, MAX_OBJECT_NAME_IN_NAWCHARS-1);
     tmpObjName[MAX_OBJECT_NAME_IN_NAWCHARS-1] = 0;
     StripTrailingBlanks (tmpObjName, (Int32)MAX_OBJECT_NAME_IN_NAWCHARS);

     char * p1stUnstranslatedChar = NULL;
     UInt32 outStrLenInBytes = 0;
     UInt32 charCount = 0;  /* number of characters translated/converted */
     Int32 cnvErrStatus = 0;
     char *pSubstitutionChar = NULL; /* Use ? as the substitute character for invalid chars - the default*/
     Int32 convFlags = 0;
     cnvErrStatus = UTF16ToLocale
                     ( cnv_version1              /* in  - const enum cnv_version version */
                     , (const char *)tmpObjName  /* in  - const char *in_bufr */
                     , NAWstrlen(tmpObjName)*BYTES_PER_NAWCHAR /* in  - const int in_len */
                     , hostVarP->objectName_     /* out - const char *out_bufr */
                     , MAX_OBJECT_NAME           /* in  - const int out_len */
                     , cnv_UTF8                  /* in  - enum cnv_charset charset */
                     , p1stUnstranslatedChar     /* out - char * & first_untranslated_char */
                     , &outStrLenInBytes         /* out - unsigned int *output_data_len_p */
                     , convFlags                 /* in  - const int cnv_flags */
                     , (Int32)TRUE               /* in  - const int addNullAtEnd_flag */
                     , (Int32)TRUE               /* in  - const int allow_invalids */
                     , &charCount                /* out - unsigned int * translated_char_cnt_p */
                     , pSubstitutionChar         /* in  - const char *substitution_char */
                     );

     NAWstrncpy(tmpObjName, (const NAWchar *)objectNewName, MAX_OBJECT_NAME_IN_NAWCHARS-1);
     tmpObjName[MAX_OBJECT_NAME_IN_NAWCHARS-1] = 0;
     StripTrailingBlanks (tmpObjName, (Int32)MAX_OBJECT_NAME_IN_NAWCHARS);

     p1stUnstranslatedChar = NULL;
     outStrLenInBytes = 0;
     charCount = 0;  /* number of characters translated/converted */
     cnvErrStatus = 0;
     pSubstitutionChar = NULL; /* Use ? */
     convFlags = 0;
     cnvErrStatus = UTF16ToLocale
                     ( cnv_version1              /* in  - const enum cnv_version version */
                     , (const char *)tmpObjName  /* in  - const char *in_bufr */
                     , NAWstrlen(tmpObjName)*BYTES_PER_NAWCHAR /* in  - const int in_len */
                     , hostVarP->objectNewName_  /* out - const char *out_bufr */
                     , MAX_OBJECT_NAME           /* in  - const int out_len */
                     , cnv_UTF8                  /* in  - enum cnv_charset charset */
                     , p1stUnstranslatedChar     /* out - char * & first_untranslated_char */
                     , &outStrLenInBytes         /* out - unsigned int *output_data_len_p */
                     , convFlags                 /* in  - const int cnv_flags */
                     , (Int32)TRUE               /* in  - const int addNullAtEnd_flag */
                     , (Int32)TRUE               /* in  - const int allow_invalids */
                     , &charCount                /* out - unsigned int * translated_char_cnt_p */
                     , pSubstitutionChar         /* in  - const char *substitution_char */
                     );
     /*  Unicode - end */
     FIX_CHAR(hostVarP->operationType_);
     FIX_CHAR(hostVarP->ignoreChangesUsed_);
   }

/*----- FOR DEBUGGING DIAG USE THID
 //===================================================================
  long backSQLCODE = SQLCODE;

  EXEC SQL BEGIN DECLARE SECTION;
    long i,num, hv_cond_num,hv_sqlcode;
    char hv_sqlstate[6];
    char hv_message_text[256];
  EXEC SQL END DECLARE SECTION;

  memset (hv_sqlstate, ' ', 6);
  memset (hv_message_text, ' ', 256);

  exec sql get diagnostics :num = NUMBER;

  FILE *myf = fopen ("mymvqrlog", "ac");

  if (num > 0)
     fprintf(myf, "---========= BEGIN OPEN DIAGS =========---\n");

  for (i=1;i<=num;i++) {
    exec sql get diagnostics exception :i
       :hv_cond_num = CONDITION_NUMBER,
       :hv_sqlstate = RETURNED_SQLSTATE,
       :hv_message_text = MESSAGE_TEXT,
       :hv_sqlcode = SQLCODE;

    hv_sqlstate[5] = 0;
    fprintf(myf, "condition number: %d\n", hv_cond_num);
    fprintf(myf, "sqlstate: %s\n", hv_sqlstate);
    fprintf(myf, "message text: %s\n", hv_message_text);
    fprintf(myf, "sqlcode: %ld\n", hv_sqlcode);
    fprintf(myf, "\n");
    }
  if (num > 0)
     fprintf(myf, "---========= END OPEN DIAGS =========---\n");

  fclose (myf);

  return backSQLCODE;

//===================================================================
----------------- END DEBUG DIAG ---*/
 
  return SQLCODE;
}

/*********************************************************
// QRQueries::closeRewritePublish
*********************************************************/
Int32 QRQueries::closeRewritePublish()
{
  EXEC SQL CLOSE ObtainRewritePublish;

  return SQLCODE;
}


/*********************************************************
//
// QRQueries::openMVNames
//
*********************************************************/
Lng32 QRQueries::openMVNames(const NAString& definitionSchema)
{
  EXEC SQL BEGIN DECLARE SECTION;
    char objectsTable[MAX_SMD_TABLE_NAME];
    char mvsTable[MAX_SMD_TABLE_NAME];
  EXEC SQL END DECLARE SECTION;
   
  memset (objectsTable, ' ', MAX_SMD_TABLE_NAME);
  strcpy (objectsTable, definitionSchema);
  strcat (objectsTable, ".OBJECTS");
 
  memset (mvsTable, ' ', MAX_SMD_TABLE_NAME);
  strcpy (mvsTable, definitionSchema);
  strcat (mvsTable, ".MVS");

  EXEC SQL DECLARE ObtainMVNames CURSOR FOR
    SELECT translate(rtrim(o.object_name) using ucs2toutf8),
           translate(rtrim(sch.schema_name) using ucs2toutf8)
    FROM   :objectsTable  prototype 'HP_SYSTEM_CATALOG.HP_DEFINITION_SCHEMA.OBJECTS'  o,
           :schemataName_ prototype 'HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.SCHEMATA'                  sch,
           :mvsTable      prototype 'HP_SYSTEM_CATALOG.HP_DEFINITION_SCHEMA.MVS'      mvs
    WHERE  o.object_type = _iso88591'MV' and 
           o.object_uid = mvs.mv_uid and  
           o.schema_uid = sch.schema_uid and
           mvs.REWRITE_ENABLED = _iso88591'Y'
    ORDER BY 1
    FOR SKIP CONFLICT ACCESS IN SHARE MODE;
    
  EXEC SQL OPEN ObtainMVNames;

  return SQLCODE;
}

/*********************************************************
// QRQueries::fetchMVNames
*********************************************************/
Lng32 QRQueries::fetchMVNames(NAString& objectName, NAString& schemaName)
{
  EXEC SQL BEGIN DECLARE SECTION;
    char objName[MAX_OBJECT_NAME];         
    char schName[MAX_OBJECT_NAME];         
  EXEC SQL END DECLARE SECTION;  
  
  EXEC SQL FETCH ObtainMVNames INTO :objName,
                                    :schName;
  
  if (SQLCODE == 0)
  {
    FIX_CHAR(objName);
    objectName = objName;
    FIX_CHAR(schName);
    schemaName = schName;
  }
  
  return SQLCODE;
}

/*********************************************************
// QRQueries::closeMVNames
*********************************************************/
Lng32 QRQueries::closeMVNames()
{
  EXEC SQL CLOSE ObtainMVNames;

  return SQLCODE;
}

/*********************************************************
// QRQueries::setParserFlags
*********************************************************/
Lng32 QRQueries::setParserFlags()
{
  EXEC SQL set parserflags 3;  
  
  return SQLCODE;
}

/*********************************************************
// QRQueries::controlQueryDefault
*********************************************************/
Lng32 QRQueries::controlQueryDefault(const NAString& cqdName, 
                                    const NAString& cqdValue)
{
  EXEC SQL BEGIN DECLARE SECTION;
    char cqdText[MAX_UTF8_CMD_LEN_IN_BYTES_PLUS_1];
  EXEC SQL END DECLARE SECTION;  
  
  sprintf(cqdText, "CONTROL QUERY DEFAULT %.200s '%.200s';", 
                   cqdName.data(),
                   cqdValue.data() );

  EXEC SQL PREPARE cqdStmt FROM :cqdText;
  if (SQLCODE)
    return SQLCODE;
  
  EXEC SQL EXECUTE cqdStmt;

  return SQLCODE;
}

/*********************************************************
// QRQueries::reDescribeMV
*********************************************************/
Lng32 QRQueries::reDescribeMV(const NAString& mvName, NABoolean rePublish)
{
  EXEC SQL BEGIN DECLARE SECTION;
    char catRequestText[MAX_UTF8_CMD_LEN_IN_BYTES_PLUS_1];
  EXEC SQL END DECLARE SECTION;  

  sprintf(catRequestText, "CREATE TANDEM_CAT_REQUEST&1 %d 2 @%.200s@ @%d@;", 
                           MV_QR_CREATE_DESC, mvName.data(),
                           (rePublish ? 1 : 0) );

  EXEC SQL PREPARE catRequestStmt FROM :catRequestText;
  if (SQLCODE)
    return SQLCODE;
  
  EXEC SQL EXECUTE catRequestStmt;

  return SQLCODE;
}


