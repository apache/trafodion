/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComMisc.cpp
 * Description:  Miscellaneous global functions
 *
 *
 * Created:      11/07/09
 * Language:     C++
 *
 *
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
 *
 *
 *****************************************************************************
 */

#include "Platform.h"

#include "ComOperators.h"
#include "ComASSERT.h"
#include "ComMisc.h"
#include "ComDistribution.h" // enumToLiteral, literalToEnum, literalAndEnumStruct
#include "CmpSeabaseDDL.h"
#include <sys/stat.h>

// define the enum-to-literal function
#define ComDefXLateE2L(E2L,eType,array) void E2L (const eType e, NAString &l) \
{ NABoolean found; char lit[100]; \
  enumToLiteral (array, occurs(array), e, lit, found); \
  ComASSERT(found); l = lit; }

// define the literal-to-enum function
#define ComDefXLateL2E(L2E,eType,array) eType L2E(const char * l) \
{ NABoolean found; \
  eType result = (eType) literalToEnum (array, occurs(array), l, found); \
  ComASSERT(found); \
  return result; }

// Define both
#define ComDefXLateFuncs(L2E,E2L,eType,array) ComDefXLateL2E(L2E,eType,array);ComDefXLateE2L(E2L,eType,array)

// systemCatalog: if passed in, is the name of traf system catalog. 
//                         default is TRAFODION.
NABoolean ComIsTrafodionReservedSchema(
                                    const NAString &systemCatalog,
                                    const NAString &catName,
                                    const NAString &schName)
{
  if (catName.isNull())
    return FALSE;

  NAString trafSysCat;

  if (NOT systemCatalog.isNull())
    trafSysCat = systemCatalog;
  else
    trafSysCat = TRAFODION_SYSCAT_LIT;

  if ((catName == trafSysCat) &&
      (
       (schName == SEABASE_DTM_SCHEMA) ||
       (schName == SEABASE_MD_SCHEMA) ||
       (schName == SEABASE_PRIVMGR_SCHEMA) ||
       (schName == SEABASE_REPOS_SCHEMA) ||
       (schName == SEABASE_UDF_SCHEMA)
       )
      )
    return TRUE;  

  return FALSE;
}

// schema names of pattern  "_%_" are reserved for internal system schemas.
NABoolean ComIsTrafodionReservedSchemaName(
                                    const NAString &schName)
{
  if ((schName.data()[0] == '_') &&
      (schName.data()[schName.length()-1] == '_'))
    return TRUE;
  
  return FALSE;
}

// schema names of pattern "_HV_ ... _" and "_HB_ ... _" are reserved to store
// external hive and hbase tables
NABoolean ComIsTrafodionExternalSchemaName (
     const NAString &schName,
     NABoolean * isHive)
{
  Int32 len (schName.length());

  // skip double quotes around schName
  NABoolean quoted = FALSE;
  if ((len > 2) &&
      (schName(0) == '"') &&
      (schName(len-1) == '"'))
    {
      quoted = TRUE;
      len = len - 2;
    }

  if (isHive)
    *isHive = FALSE;

  // check for HIVE
  Int32 prefixLen = sizeof(HIVE_EXT_SCHEMA_PREFIX);
  if (len > prefixLen && 
      (schName((quoted ? 1 : 0), prefixLen-1) == HIVE_EXT_SCHEMA_PREFIX && 
       schName((quoted ? 1 : 0) + len-1) == '_' ))
    {
      if (isHive)
        *isHive = TRUE;
      return TRUE;
    }

  // check for HBASE
  prefixLen = sizeof(HBASE_EXT_SCHEMA_PREFIX);
  if (len > prefixLen && 
      (schName((quoted ? 1 : 0), prefixLen-1) == HBASE_EXT_SCHEMA_PREFIX && 
       schName((quoted ? 1 : 0) + len-1) == '_' ))
    return TRUE;

  return FALSE;
}

// schema name "_HB_MAP_" is used to store external hbase table mapped
// to trafodion relational table
NABoolean ComIsHbaseMappedSchemaName (
     const NAString &schName)
{
  return (schName == HBASE_EXT_MAP_SCHEMA);
}

// external format of an HBase mapped table used by 
// users: HBASE."_MAP_".<tablename>
NABoolean ComIsHBaseMappedExtFormat(const NAString &catalogNamePart,
                                    const NAString &schemaNamePart)
{
  if ((catalogNamePart == HBASE_SYSTEM_CATALOG) &&
      (schemaNamePart == HBASE_MAP_SCHEMA))
    return TRUE;

  return FALSE;
}

// internal format of HBase mapped table as stored in traf
// metadata: TRAFODION."_HB_MAP_".<tablename>
NABoolean ComIsHBaseMappedIntFormat(const NAString &catalogNamePart,
                                    const NAString &schemaNamePart)
{
  if ((catalogNamePart == TRAFODION_SYSCAT_LIT) &&
      (schemaNamePart == HBASE_EXT_MAP_SCHEMA))
    return TRUE;

  return FALSE;
}

void ComConvertHBaseMappedIntToExt(const NAString &inCatName,
                                   const NAString &inSchName,
                                   NAString &outCatName, 
                                   NAString &outSchName)
{
  outCatName = HBASE_SYSTEM_CATALOG;
  outSchName = HBASE_MAP_SCHEMA;
}

void ComConvertHBaseMappedExtToInt(const NAString &inCatName,
                                   const NAString &inSchName,
                                   NAString &outCatName, 
                                   NAString &outSchName)
{
  outCatName = TRAFODION_SYSCAT_LIT;
  outSchName = HBASE_EXT_MAP_SCHEMA;
}

// ----------------------------------------------------------------------------
// function: ComConvertNativeNameToTrafName
//
// this function converts the native HIVE or HBASE object name into its
// Trafodion external name format.
//
// params:
//    catalogName - catalog name to identify HBASE or HIVE native table
//    schemaName - external name of the HBASE or HIVE schema
//    objectName - external name of the HBASE of HIVE table
//
// If it is not HIVE or HBASE, just return the qualified name
// ----------------------------------------------------------------------------
NAString ComConvertNativeNameToTrafName ( 
  const NAString &catalogName,
  const NAString &schemaName,
  const NAString &objectName)
{
  // generate new schema name 
  NAString tempSchemaName; 
  if (catalogName == HIVE_SYSTEM_CATALOG)
    tempSchemaName += HIVE_EXT_SCHEMA_PREFIX;
  else if(catalogName == HBASE_SYSTEM_CATALOG)
    tempSchemaName += HBASE_EXT_SCHEMA_PREFIX;
  else
    return catalogName + NAString(".") +
           schemaName + NAString(".") +
           objectName; 

  ComAnsiNamePart externalAnsiName(schemaName, ComAnsiNamePart::EXTERNAL_FORMAT);
  tempSchemaName += externalAnsiName.getInternalName();
  tempSchemaName.append ("_");

  // Catalog name is "TRAFODION"
  NAString convertedName (CmpSeabaseDDL::getSystemCatalogStatic());
  convertedName += ".";

  // append transformed schema name, convert internal name to external format
  ComAnsiNamePart internalAnsiName(tempSchemaName, ComAnsiNamePart::INTERNAL_FORMAT);
  convertedName += internalAnsiName.getExternalName();

  // object  name is appended without change
  convertedName += NAString(".") + objectName;

  return convertedName;
}

// ----------------------------------------------------------------------------
// function: ComConvertTrafNameToNativeName
//
// this function converts the Trafodion external table name 
// into its native name format. Both names are in external format.
//
// Example:  TRAFODION."_HV_HIVE_".abc becomes HIVE.HIVE.abc
//
// params:
//    catalogName - catalog name of the external table
//    schemaName - schema name of the extenal table
//    objectName - object name of the external table
//
// ----------------------------------------------------------------------------
NAString ComConvertTrafNameToNativeName( 
  const NAString &catalogName,
  const NAString &schemaName,
  const NAString &objectName)
{

  NAString tempSchemaName; 
  ComAnsiNamePart externalAnsiSchemaName(schemaName, ComAnsiNamePart::EXTERNAL_FORMAT);
  tempSchemaName += externalAnsiSchemaName.getInternalName();

  NAString tempCatalogName; 

  NASubString prefix = tempSchemaName.subString(HIVE_EXT_SCHEMA_PREFIX, 0);
  if ( prefix.length() > 0 ) {
     tempSchemaName.remove(0, prefix.length()); 
     tempSchemaName.remove(tempSchemaName.length()-1, 1); // remove the trailing "_" 
     tempCatalogName = HIVE_SYSTEM_CATALOG;
  }  else {
     // do not reuse prefix here because it becomes immutable after the above 
     // subString() call. 
     NASubString prefix2 = tempSchemaName.subString(HBASE_EXT_SCHEMA_PREFIX, 0);
     if ( prefix2.length() > 0 ) {
       tempSchemaName.remove(0, prefix2.length());; 
       tempSchemaName.remove(tempSchemaName.length()-1, 1); // remove the trailing "_" 
       tempCatalogName = HBASE_SYSTEM_CATALOG;
     } 
  } 

  NAString convertedName;

  ComAnsiNamePart internalAnsiCatalogName(tempCatalogName, ComAnsiNamePart::INTERNAL_FORMAT);
  convertedName += internalAnsiCatalogName.getExternalName();
  convertedName += ".";

  ComAnsiNamePart internalAnsiSchemaName(tempSchemaName, ComAnsiNamePart::INTERNAL_FORMAT);
  convertedName += internalAnsiSchemaName.getExternalName();
  convertedName += ".";

  convertedName += objectName;

  return convertedName;
}

// Hive names specified in the query may have any of the following
// forms after they are fully qualified:
//  hive.hive.t, hive.`default`.t, hive.hivesch.t, hive.hivesch
// These names are valid in traf environment only and are used to determine
// if hive ddl is being processed.
//
// Return equivalent native hive names of the format:
//   `default`.t, `default`.t, hivesch.t, hivesch
// Return NULL string in case of an error.
NAString ComConvertTrafHiveNameToNativeHiveName(
     const NAString &catalogName,
     const NAString &schemaName,
     const NAString &objectName)
{
  NAString newHiveName;
  if (catalogName.compareTo(HIVE_SYSTEM_CATALOG, NAString::ignoreCase) != 0)
    {
      // Invalid hive name in traf environment.
      return newHiveName;
    }

  if (schemaName.compareTo(HIVE_DEFAULT_SCHEMA_EXE, NAString::ignoreCase) == 0) // matches  'default'
    {
      newHiveName += NAString("`") + schemaName + "`";
      if (NOT objectName.isNull())
        newHiveName += ".";
    }
  else if (schemaName.compareTo(HIVE_SYSTEM_SCHEMA, NAString::ignoreCase) == 0) // matches  'hive'
    {
      // set fully qualified hive default schema name `default`
      newHiveName += NAString("`default`");
      if (NOT objectName.isNull())
        newHiveName += ".";
    }
  else // user schema name
    {
      newHiveName += schemaName;
      if (NOT objectName.isNull())
        newHiveName += ".";
    }

  if (NOT objectName.isNull())
    newHiveName += objectName;

  return newHiveName;
}

NABoolean ComTrafReservedColName(
     const NAString &colName)
{

  if ((colName == TRAF_SALT_COLNAME) ||
      (colName == TRAF_SYSKEY_COLNAME))
    return TRUE;

  if ((memcmp(colName.data(), TRAF_DIVISION_COLNAME_PREFIX, strlen(TRAF_DIVISION_COLNAME_PREFIX)) == 0) &&
      (colName.data()[colName.length()-1] == '_'))
    return TRUE;

  return FALSE;
}


Int32  ComGenerateUdrCachedLibName(NAString libname,Int64 redeftime, NAString schemaName, NAString userid, NAString &cachedLibName, NAString &cachedLibPath)
{
  NAString libPrefix, libSuffix;
  struct stat statbuf;
  NAString redefTimeString = Int64ToNAString(redeftime);
  size_t lastDot = libname.last('.');
  if (lastDot != NA_NPOS)
    {
      libSuffix = libname(lastDot,libname.length()-lastDot);
      libPrefix = libname(0,lastDot);
    }
 
  //when isolated user support is added we will pass an actual userid.
  //By default we assume DB__ROOT.
  if (userid.length()!=0)       
    {

      cachedLibPath = getenv("TRAF_VAR") ;
      cachedLibPath += "/udr";
      if ( stat(cachedLibPath, &statbuf) != 0)
         {
           if (mkdir(cachedLibPath,S_IRWXU|S_IRWXG|S_IRWXO))
             {
               return errno;
             }
               
         }
      cachedLibPath +=  "/"+ userid ;
      if (stat(cachedLibPath, &statbuf) != 0)
        {
          if (mkdir(cachedLibPath,S_IRUSR|S_IWUSR|S_IXUSR))//Only this user has 
            //permission to read/write/execute in this directory and below.
            {
              return errno;
            }
               
        }
      cachedLibPath += "/";
      cachedLibPath += getenv("UDR_CACHE_LIBDIR");
      if ( stat(cachedLibPath, &statbuf) != 0)
         {
           if (mkdir(cachedLibPath,S_IRWXU|S_IRWXG|S_IRWXO))
             {
               return errno;
             }
               
         }
     
      cachedLibPath += "/" + schemaName;
      if ( stat(cachedLibPath, &statbuf) != 0)
         {
           if (mkdir(cachedLibPath,S_IRWXU|S_IRWXG|S_IRWXO))
             {
               return errno;
             }
               
         }
     
      
    }
  else
    {
      cachedLibPath = getenv("TRAF_VAR") ;
      cachedLibPath += "/udr";
      if ( stat(cachedLibPath, &statbuf) != 0)
         {
           if (mkdir(cachedLibPath,S_IRWXU|S_IRWXG|S_IRWXO))
             {
               return errno;
             }
               
         }
      cachedLibPath +=  "/"+ NAString("DB__ROOT") ;
      if (stat(cachedLibPath, &statbuf) != 0)
        {
          if (mkdir(cachedLibPath,S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)) // these permissions
            //need to change when we have isolated user support so only DB_ROOT 
            //can access this directory. Right now we allow all to access this directory
            {
              return errno;
            }
               
        }
      cachedLibPath += "/";
      cachedLibPath += getenv("UDR_CACHE_LIBDIR");
      if ( stat(cachedLibPath, &statbuf) != 0)
         {
           if (mkdir(cachedLibPath,S_IRWXU|S_IRWXG|S_IRWXO))
             {
               return errno;
             }
               
         }
    
      cachedLibPath += "/" + schemaName;
      if ( stat(cachedLibPath, &statbuf) != 0)
         {
           if (mkdir(cachedLibPath,S_IRWXU|S_IRWXG|S_IRWXO))
             {
               return errno;
             }
               
         }
     
    }
      
  
  
  cachedLibName += libPrefix + "_" ;
  cachedLibName += redefTimeString;
  cachedLibName += libSuffix ;

  return 0;
  
}


