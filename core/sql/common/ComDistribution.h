/* -*-C++-*- */
#ifndef COMDISTRIBUTION_H
#define COMDISTRIBUTION_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComDistribution.h
 * Description:  Supports distributed databases.
 *               
 * Created:      5/23/2003
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

#include "NABoolean.h"
#include "ComSizeDefs.h"
#include "ComCharSetDefs.h"
#include "ComDiags.h"
#include "ComSmallDefs.h"

  #ifndef __DERROR__
    #include "fs/feerrors.h"
  #endif //__DERROR

// -----------------------------------------------------------------------
// SQL Name Conversion helpers
// -----------------------------------------------------------------------

#include "NAWinNT.h"
#include "wstr.h"

// -----------------------------------------------------------------------
// Functions to convert SQL names from the for-internal-process character
// set (i.e., the Unicode UTF-8 character set) to the Unicode UCS-2/UTF-16
// character set and vice versa.  Names are stored in the metadata table
// in Unicode UCS-2/UTF-16 encoding format.
//
// Similar APIs with NAString and NAWString parameters in place of
// char* and NAWchar* parameters are declared and defined in
// header and source files w:/common/ComAnsiNamePart.h and .cpp
// -----------------------------------------------------------------------

// returned error code described in w:/common/csconvert.h
Int32 ComAnsiNameToUTF8 ( const NAWchar * inAnsiNameInUCS2  // in  - valid ANSI SQL name in UCS2/UTF16
                        , char *      outBuf4AnsiNameInUTF8 // out - out buffer
                        , const Int32 outBufSizeInBytes     // in  - out buffer max len in bytes
                        );

// returned error code described in w:/common/csconvert.h
Int32 ComAnsiNameToUCS2 ( const char * inAnsiNameInUTF8     // in  - valid name in default ANSI name char set
                        , NAWchar *   outAnsiNameInNAWchars // out - out buffer
                        , const Int32 outBufSizeInNAWchars  // in  - out buffer max len in NAWchars
                        , const NABoolean padWithSpaces = FALSE // in  - fill remainder with spaces if TRUE;
                        );                                      //       otherwise, just add a NULL terminator.

// -----------------------------------------------------------------------
// Meatadata Distribution
// -----------------------------------------------------------------------

enum DistributionErrorCode {
                              DISTRIBUTION_NO_ERROR                       = 0
                            , DISTRIBUTION_NOT_FOUND                      = 100
                            , DISTRIBUTION_CATALOG_NOT_VISIBLE_LOCAL      = 1002
                            , DISTRIBUTION_FIRST_ERROR                    = 25400
                            , DISTRIBUTION_CATALOG_NOT_VISIBLE_REMOTE     = 25401
                            , DISTRIBUTION_CATALOG_ALREADY_VISIBLE        = 25402
                            , DISTRIBUTION_INDEPENDENT_CATALOG_EXISTS     = 25403
                            , DISTRIBUTION_CANNOT_ESTABLISH_RELATION      = 25404
                            , DISTRIBUTION_CANNOT_CREATE_REMOTE_CATALOG   = 25405
                            , DISTRIBUTION_RUNTIME_VISIBILITY_CHECK_FAIL  = 25406
                            , DISTRIBUTION_SYSTEM_SCHEMA_UNAVAILABLE      = 25407
                            , DISTRIBUTION_NODE_IS_UNAVAILABLE            = 25420
                            , DISTRIBUTION_AUTOMATIC_REFERENCE_EXISTS     = 25421
                            , DISTRIBUTION_CANNOT_UNREGISTER_OBJECTS      = 25422
                            , DISTRIBUTION_SCHEMA_HAS_NO_REPLICA          = 25423
                            , DISTRIBUTION_SCHEMA_REPLICA_EXISTS          = 25424
                            , DISTRIBUTION_CANNOT_UNREPLICATE_AUTOMATIC   = 25425
                            , DISTRIBUTION_VOLUME_IS_UNAVAILABLE          = 25426
                            , DISTRIBUTION_CANNOT_UNREGISTER_CATALOG      = 25427
                            , DISTRIBUTION_CANNOT_REGISTER_CATALOG        = 25428
                            , DISTRIBUTION_CANNOT_UNREGISTER_AUTOMATIC    = 25429
                            , DISTRIBUTION_CANNOT_REGISTER_SYSTEM_CATALOG = 25430
                            , DISTRIBUTION_SQLMX_OBJECTS_NOT_SUPPORTED    = 25431
                            , DISTRIBUTION_DEBUG_WARNING                  = 25490
                            , DISTRIBUTION_LAST_ERROR                     = 25499
                           };

enum AvailabilityErrorCode {
                              AVAILABILITY_NO_ERROR
                            , AVAILABILITY_ERROR_PARTITION_UNAVAIL
                            , AVAILABILITY_ERROR_VOLUME_UNAVAIL
                            , AVAILABILITY_ERROR_NODE_UNAVAIL
                            , AVAILABILITY_ERROR_MISCELLANEOUS  
};

//----------------------------------------------------------------------
//
//  Build an ANSI schema name from its individual parts
//
void 
ComBuildSchemaName ( const char * catalogName,   // in, catalog name (internal format)
                     const char * schemaName,    // in, schema name (internal format)
                     char * ansiSchemaName,      // out, ANSI name (external format)
                     const Int32 ansiNameOutBufSize); // in, ANSI name output buffer size in bytes

//----------------------------------------------------------------------
//
//  Build an ANSI name from its individual parts
//
void 
ComBuildANSIName ( const char * catalogName,   // in, catalog name (internal format)
                   const char * schemaName,    // in, schema name (internal format)
                   const char * objectName,    // in, object name (internal format)
                   char * ansiName,            // out, ANSI name (external format)
                   const Int32 ansiNameOutBufSize // in, ANSI name output buffer size in bytes
                   = ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES + 1  //  for the worst case
                   );

//----------------------------------------------------------------------
//
//  Convert between various enums and literals. Output literal must 
//  be at least 3 characters.
//
#pragma nowarn(449)   // Disregard warning 449: No constructor to 
typedef struct        // initialize const members
{
  Int32                      enum_;
  const char *               literal_;
} literalAndEnumStruct;
#pragma warn(449)

// General enum to literal translation
void enumToLiteral ( const literalAndEnumStruct * conversionTable,
                     const Int32 noOfElements,
                     const Int32 enumValue,
                     char * literal,
                     NABoolean & found);

// General literal to enum translation
Int32 literalToEnum (const literalAndEnumStruct * conversionTable,
                   const Int32 noOfElements,
                   const char * literal,
                   NABoolean & found);

//----------------------------------------------------------------------
//
//  Convert a name space enum to text. Output buffer must be at least
//  32 characters.
//
void 
ComNameSpaceEnumToText (const ComAnsiNameSpace nameSpace,
                        char * typeOfObject);

//----------------------------------------------------------------------
//
//  Convert an object type enum to text. Output buffer must be at least
//  26 characters.
//
void ComObjectTypeEnumToText (const ComObjectType objectType,
                              char * typeOfObject);

//----------------------------------------------------------------------
//
//  Convert between name space enum and name space literal. Output literal must 
//  be at least 3 characters.
//
void ComNameSpaceEnumToLiteral (const ComAnsiNameSpace nameSpace,
                                char * nameSpaceLiteral);

ComAnsiNameSpace ComNameSpaceLiteralToEnum (const char * nameSpaceLiteral);

//----------------------------------------------------------------------
//
//  Convert between object type enum and object type literal. Output literal must 
//  be at least 3 characters.
//
void ComObjectTypeEnumToLiteral ( const ComObjectType objectType,
                                  char * objectTypeLiteral);

ComObjectType ComObjectTypeLiteralToEnum (const char * objectTypeLiteral);

//----------------------------------------------------------------------
//
//  Convert between privilege type enum and privilege type literal. 
//
void ComPrivTypeEnumToLiteral (const ComPrivilegeType privType,
                               char* privTypeLiteral);

ComPrivilegeType ComPrivTypeLiteralToEnum (const char * privTypeLiteral);

//----------------------------------------------------------------------
//
//  Convert between privilege type enum and privilege type literal. 
//
void ComQIActionTypeEnumToLiteral (const ComQIActionType qiType,
                                   char* qiTypeLiteral);

ComQIActionType ComQIActionTypeLiteralToEnum (const char * qiTypeLiteral);

//----------------------------------------------------------------------
//
//  Convert between schema level operation enum and literal. Output literal must 
//  be at least 3 characters.
//
void ComSchemaOperationEnumToLiteral ( const ComSchemaOperation operation, char * operationLiteral);

ComSchemaOperation ComSchemaOperationLiteralToEnum (const char * operationLiteral);

//----------------------------------------------------------------------
//
//  Convert between utility operation enum and literal. Output literal must 
//  be at least 3 characters.
//
void ComUtilOperationEnumToLiteral ( const ComUtilOperation operation, char * operationLiteral);

ComUtilOperation ComUtilOperationLiteralToEnum (const char * operationLiteral);

//----------------------------------------------------------------------
//
//  enum and translation functions for the VERSION_INFO stored procedure
//

#define MAX_INPUT_TYPE_SIZE 15
enum VersionInfoSPInputType 
{
  iUnknown = 0,           // Must be zero
  iTable = 1,
  iTableAll = 2,
  iIndex = 3,
  iIndexTable = 4,
  iSchema = 5,
  iSystemSchema = 6,
  iView = 7,
  iProcedure = 8,
  iTrigger = 9,
  iConstraint = 10,
  iModule = 11,
  iSynonym = 12
};

void ComVersionInfoSPInputTypeToLiteral ( const VersionInfoSPInputType inputType,
                                          char * inputTypeLiteral);

VersionInfoSPInputType ComVersionInfoSPLiteralToInputType (const char * inputTypeLiteral);

//----------------------------------------------------------------------
//
//  enum and translation functions for the RELATEDNESS stored procedure
//

enum RelatednessSPInputType 
{
  rUnknown = 0,           // Must be zero
  rCatalog = 1,
  rSchema = 2,
  rNode = 3
};

void ComRelatednessSPInputTypeToLiteral ( const RelatednessSPInputType inputType,
                                          char * inputTypeLiteral);

RelatednessSPInputType ComRelatednessSPLiteralToInputType (const char * inputTypeLiteral);

//----------------------------------------------------------------------
//
//  enum and translation functions for the FEATURE_VERSION_INFO stored procedure
//

enum FeatureVersionInfoSPInputType 
{
  fUnknown        = 0,           // Must be zero
  fCatalog        = 1,
  fCatalogCascade = 2,
  fSchema         = 3,
  fSchemaCascade  = 4
};

void ComFeatureVersionInfoSPInputTypeToLiteral ( const FeatureVersionInfoSPInputType inputType
                                               , char * inputTypeLiteral);

FeatureVersionInfoSPInputType ComFeatureVersionInfoSPLiteralToInputType 
                                               ( const char * inputTypeLiteral );

//----------------------------------------------------------------------
//
//  Translate an anchor file access error to something more sensible.
//
Lng32 ComTranslateAnchorFileAccessError (const Lng32 sqlCode, 
                                        const Lng32 fsError, 
                                        const char * nodeName,
                                        ComDiagsArea * da);

//----------------------------------------------------------------------
//  Class VisibilityErrorTranslator:
//     Translate late name resolution errors in a ComDiagsArea to
//     corresponding visibility errors, for a particular object.
//
//  Warning: Do not keep VisibilityErrorTranslator objects 
//  hanging around after their Guardian Name has
//  been deallocated. 
//
class VisibilityErrorTranslator : public ComDiagsTranslator
{
public:
  VisibilityErrorTranslator (const char * ansiName,
                             const char * guardianName); 
  const NABoolean translateCondition 
                            (ComDiagsArea &diags,
                             const ComCondition &cond);

private:
  // Pointer to the ANSI name of the object
  const char * ansiName_;
  // Pointer to the Guardian name of the object
  const char * guardianName_;
};

//----------------------------------------------------------------------
//  Class MetaDataErrorTranslator:
//     Translate executor errors into relevant catalog manager errors.
//
//     NB: Objects of this class will store a pointer to the table name they 
//         are constructed with. It is thus not recommendable to let such
//         objects survive the table name's allocation.
//
//     NB: Metadata access through DDOL will not use this class in R2.0. That is,
//         utilities will not perform the SMD table error translation.
//
class MetaDataErrorTranslator : public ComDiagsTranslator
{
public:
  // Constructor. The tableName points to the name of the object that encountered
  // an error. This can be an external format ANSI name (in case of a metadata 
  // table error) or an external format Guardian name (in case of a resource fork 
  // error). The nodeName parameter can be NULL.
  MetaDataErrorTranslator (const char * tableName,
                           const NABoolean isSystemSchemaTable = FALSE,
                           const char * nodeName = NULL); 



private:
  // analyzeCondition will be called once per condition in the
  // original diags area, before translateCondition is called.
  // It will recognise certain common combinations of errors
  // and register if they are present or not.
  void analyzeCondition (const ComCondition &cond);

  // translateCondition will be called once per condition in the
  // original diags area. Depending upon the outcome of analyzeErrors,
  // it will recognise certain common combinations of errors
  // and translate accordingly
  // - if isUnavailable_ then we know there is an error -8580 followed
  //   by a -8551, and we have saved the relevant Guardian file name
  //   and FE error. Throw away both errors and issue <something> 
  // - if isAbsent_ then we know there is an error -8580 followed
  //   by a -8577, and we have saved the relevant Guardian file name
  //   and FE error. Throw away both errors and issue <something else> 
  // 
  const NABoolean translateCondition (ComDiagsArea &diags,
                                const ComCondition &cond);
  
  enum ErrorCombination { YES, NO, MAYBE };

  void beforeAnalyze (void);
  void afterTranslate (void);

  char nodeName_[ComMAX_GUARDIAN_NAME_PART_LEN+1];
  char partitionName_[ComMAX_FULLY_QUALIFIED_GUARDIAN_NAME_LEN+1];
  const char * tableName_;
  NABoolean isSystemSchemaTable_;
  NABoolean isResourceFork_;
  NABoolean expectMore_;

  ErrorCombination isUnavailable_;      // The combination of -8580 and -8551
  ErrorCombination isAbsent_;           // The combination of -8580 and -8577

  Lng32 nskError_;

};

//----------------------------------------------------------------------
//
//  Handy defines, to convert between different string formats (fix length&space filled vs. zero-terminated)
//
//
// set an item that the compiler knows the size of to low values
#define SetToLowValues(x) memset (x,0,sizeof(*x));
// set an item that the compiler knows the size of to high values
#define SetToHighValues(x) memset (x,0xff,sizeof(*x));
// set an item that the compiler knows the size of to spaces
#define SetToSpaces(x) memset (x,' ',sizeof(*x));
// strip trailing blanks from a string, for which the compiler knows the length
#define StripTrailingBlanks(x) {Int32 more=1,i=sizeof(x)-2;while(more){if(x[i]==' ')x[i]=0;else more=0;i--;}}
// strip trailing blanks from a string, for which the compiler does not know the length
#define StripTrailingBlanksExplicit(x,l) {Int32 more=1,i=l-2;while(more){if(x[i]==' ')x[i]=0;else more=0;i--;}}
// copy and space fill a string into an item that the compiler knows the size of
#define StringToFixLengthChar(s,f) {SetToSpaces(f);memmove(f,s,strlen(s));}
// copy a space-padded item into a string. Both items have lengths known to the compiler.
#define FixLengthCharToString(f,s) {SetToSpaces(s);memmove(s,f, sizeof(*s));*s[sizeof(*s)-1]=0;StripTrailingBlanks(*s);}
// copy a space-padded item into a string. Neither item has a length known to the compiler
#define FixLengthCharToStringExplicit(f,s,l) {char * p = s;memmove(p,f,l);p[l-1]=0;StripTrailingBlanksExplicit(p,l);}

// strip trailing blanks from a string stored in a buffer with known buffer size in NAWchar elements
// The passed-in buffer (pointed by x) must have the extra room for a NULL terminator.
// -----------------------------------------------------------------------
void ComStripTrailingBlanks(NAWchar *x, const Int32 bufSizeInNAWchars);
// copy and space fill a string into a buffer (an array of NAWchar elements)
void StringToArrayOfNAWcharsSpacesPaddedNullTerminated ( const NAWchar * inStrNullTerminated
                                                       , const Int32     inStrLenInNAWchars
                                                       , NAWchar *       outBuf
                                                       , const Int32     outBufSizeInNAWchars
                                                       );

#endif // COMDISTRIBUTION_H
