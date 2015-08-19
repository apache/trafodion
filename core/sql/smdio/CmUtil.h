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
#ifndef CMUTIL_H
#define CMUTIL_H
#include "ComSmallDefs.h"
#include "ComVersionDefs.h"
#include "ComDistribution.h"
#include "OperTypeEnum.h"
#include "nawstring.h"
//#include "CatError.h"


#define YesNoToBoolean(val) (COM_YES == (val) ? TRUE : FALSE)
#define BooleanToYesNo(val) (TRUE == (val) ? COM_YES : COM_NO)

enum CM_SQLCODE_CHECK { CM_CHECK_0
                      , CM_CHECK_100
                      , CM_CHECK_0_100
};
enum CM_OPERATION     { CM_DELETE
                      , CM_INSERT
                      , CM_SELECT
                      , CM_UPDATE
                      };


extern ComString CmUtilGetSMDLocation(ComString sys_name,
                                      ComString catalog_name,
                                      COM_VERSION version);

inline void CmGetComNameAsLit(const ComAnsiNameSpace enumValue, char * literal)
{ ComNameSpaceEnumToLiteral (enumValue,  literal); };

inline ComAnsiNameSpace CmGetComNameAsAnsiName(const char * literal)
{ return ComNameSpaceLiteralToEnum (literal); };

inline void CmGetComObjectAsLit(const ComObjectType enumValue, char * literal)
{ ComObjectTypeEnumToLiteral (enumValue,  literal); };

inline ComObjectType CmGetComObjectAsComObjectType (const char * literal)
{ return ComObjectTypeLiteralToEnum (literal); };

inline void CmGetComPrivAsLit (const ComPrivilegeType enumValue, char * literal)
{ ComPrivTypeEnumToLiteral (enumValue,  literal); };

inline ComPrivilegeType CmGetComPrivAsPrivilegeType (const char * literal)
{ return ComPrivTypeLiteralToEnum (literal); };

extern void CmGetComObjectClassAsLit (const ComObjectClass enumValue, char * literal);

extern ComObjectClass CmGetComObjectClassAsObjectClass (const char * literal);

extern void CmGetComComponentPrivilegeClassAsLit (const ComComponentPrivilegeClass enumValue, char * literal);

extern ComComponentPrivilegeClass CmGetComComponentPrivilegeClassAsComponentPrivilegeClass (const char * literal);

extern void CmGetComClusterAsLit (const ComClusteringScheme enumValue, char * literal);

extern ComClusteringScheme CmGetComClusterAsClusterScheme (const char * literal);

extern void CmGetComPartAsLit (const ComPartitioningScheme enumValue, char * literal);

extern ComPartitioningScheme CmGetComPartAsPartitionScheme (const char * literal);

extern void CmGetComAccPathTypeAsLit (const ComAccessPathType enumValue, char * literal);

extern ComAccessPathType CmGetComAccPathTypeAsAccessPathType (const char * literal);

extern void CmGetRowFormatTypeAsLit (const ComRowFormat enumValue, char * literal);

extern void CmGetCompressionTypeAsLit (const ComCompressionType enumValue, char *literal);

extern ComCompressionType CmGetComCompressionTypeAsCompressionType(const char * literal);

extern ComRowFormat CmGetComRowFormatAsRowFormat(const char * literal);

extern void CmGetInsertModeAsLit (const ComInsertMode enumValue, char * literal);

extern ComInsertMode CmGetComInsertModeAsInsertMode(const char * literal);

extern void CmGetComOrderAsLit (const ComColumnOrdering enumValue, char * literal);

extern ComColumnOrdering CmGetComOrderAsComColumnOrder (const char * literal);

extern void CmGetComDirectionAsLit (const ComColumnDirection enumValue, char * literal);

extern ComColumnDirection CmGetComDirectionAsComColumnDirection (const char * literal);

extern void CmGetComParamDirectionAsLit (const ComParamDirection enumValue, char * literal);

extern ComParamDirection CmGetComDirectionAsComParamDirection (const char * literal);

extern void CmGetComStoreByDetailsAsLit (const ComStoreByDetails enumValue, char * literal);

extern ComStoreByDetails CmGetStoreByDetailsAsComStoreByDetails (const char * literal);

extern void CmGetComClassAsLit (const ComColumnClass enumValue, char * literal);

extern ComColumnClass CmGetComClassAsComColumnClass (const char * literal);

extern void ComGetRoutineParamTypeAsLit  (const ComRoutineParamType enumValue, char * literal);

extern ComRoutineParamType ComGetRoutineParamTypeAsComRoutineParamType  (const char * literal);

extern void CmGetComODBCTypeAsLit (const ComODBCDataType enumValue, char * literal);

extern ComODBCDataType CmGetODBCTypeAsComODBCDataType (const char * literal);

extern void CmGetComSQLTypeAsLit (const ComSQLDataType enumValue, char * literal);

extern ComSQLDataType CmGetComSQLTypeAsComSQLDataType (const char * literal);

extern void CmGetComColDefaultAsLit (const ComColumnDefaultClass enumValue, char * literal);

extern ComColumnDefaultClass CmGetComColDefaultAsColDefault (const char * literal);

extern void CmGetComParamDefaultClassAsLit (const ComParamDefaultClass enumValue, char * literal);

extern ComParamDefaultClass CmGetParamDefaultClassAsComParamDefaultClass (const char * literal);

extern void CmGetComGrantorAsLit (const ComGrantorType enumValue, char * literal);

extern ComGrantorType CmGetComGrantorAsGrantorType (const char * literal);

extern void CmGetComGranteeAsLit (const ComGranteeType enumValue, char * literal);

extern ComGranteeType CmGetComGranteeAsGranteeType (const char * literal);

extern void CmGetComRebindAsLit (const ComAutoRebindOption enumValue, char * literal);

extern ComAutoRebindOption CmGetComRebindAsAutoRebindOption (const char * literal);

extern void CmGetComRCMatchAsLit (const ComRCMatchOption enumValue, char * literal);

extern ComRCMatchOption CmGetComRCMatchAsRCMatch (const char * literal);

extern void CmGetComRCUpdateAsLit (const ComRCUpdateRule enumValue, char * literal);

extern ComRCUpdateRule CmGetComRCUpdateAsRCUpdate (const char * literal);

extern void CmGetComRCDeleteAsLit (const ComRCDeleteRule enumValue, char * literal);

extern ComRCDeleteRule CmGetComRCDeleteAsRCDelete (const char * literal);

extern void CmGetComConstrTypeAsLit (const ComConstraintType enumValue, char * literal);

extern ComConstraintType CmGetComConstrTypeAsConstrType (const char * literal);

extern void CmGetComViewCheckAsLit (const ComViewCheckOption enumValue, char * literal);

extern ComViewCheckOption CmGetComViewCheckAsViewCheckOption (const char * literal);

// view_type added to VWS in v2500
extern void CmGetComViewTypeAsLit (const ComViewType enumValue, char * literal);

extern ComViewType CmGetComViewTypeAsViewType (const char * literal);

extern ComBoolean YN_TRUEFALSE (const char * literal);

extern void CmGetComBooleanAsLit (const ComBoolean enumValue, char * literal);

extern void CmGetComPartnStatusAsLit (const ComPartnStatus enumValue, char * literal);

extern ComPartnStatus CmGetComPartnStatusAsPartnStatus (const char * literal);

extern void CmGetComDdlStatusAsLit (const ComDdlStatus enumValue, char * literal);

extern ComDdlStatus CmGetComDdlStatusAsDdlStatus (const char * literal);

extern void CmGetTableFeatureAsLit (const ComTableFeature enumValue, char * literal);

extern ComTableFeature CmGetTableFeature (const char * literal);

extern void CmGetComRoutineLanguageAsLit (const ComRoutineLanguage enumValue, char * literal);

extern ComRoutineLanguage CmGetComRoutineLanguageAsRoutineLanguage (const char * literal);

extern void CmGetComRoutineParamStyleAsLit (const ComRoutineParamStyle enumValue, char * literal);

extern ComRoutineParamStyle CmGetComRoutineParamStyleAsRoutineParamStyle (const char * literal);

extern void CmGetComRoutineParamTypeAsLit (const ComRoutineParamType enumValue, char * literal);

extern ComRoutineParamType CmGetRoutineParamTypeAsComRoutineParamType (const char * literal);

extern void CmGetComRoutineSQLAccessAsLit (const ComRoutineSQLAccess enumValue, char * literal);

extern ComRoutineSQLAccess CmGetComRoutineSQLAccessAsRoutineSQLAccess (const char * literal);

extern void CmGetComRoutineTransactionAttributesAsLit (const ComRoutineTransactionAttributes enumValue, char * literal);

extern ComRoutineTransactionAttributes
    CmGetComRoutineTransactionAttributesAsRoutineTransactionAttributes (const char * literal);

extern void CmGetComRoutineTypeAsLit (const ComRoutineType enumValue, char * literal);

extern ComRoutineType CmGetComRoutineTypeAsRoutineType (const char * literal);

extern void CmGetComRoutineParallelismAsLit (ComRoutineParallelism enumValue, char * literal);

extern ComRoutineParallelism CmGetRoutineParallelismAsComRoutineParallelism (const char * literal);

extern void CmGetComRoutineExternalSecurityAsLit (ComRoutineExternalSecurity enumValue, char * literal);

extern ComRoutineExternalSecurity CmGetRoutineExternalSecurityAsComRoutineExternalSecurity (const char * literal);

extern void CmGetComRoutineExecutionModeAsLit (const ComRoutineExecutionMode enumValue, char * literal);

extern ComRoutineExecutionMode CmGetRoutineExecutionModeAsComRoutineExecutionMode (const char * literal);

inline void CmGetComUtilAsLit (const ComUtilOperation enumValue, char * literal)
  { ComUtilOperationEnumToLiteral (enumValue,  literal); };

inline ComUtilOperation CmGetComUtilAsUtilOperation (const char * literal)
  { return ComUtilOperationLiteralToEnum (literal); };

extern void CmGetComActivationTimeAsLit(const ComActivationTime enumValue, char * literal);

extern void CmGetComOperationAsLit(const ComOperation enumValue, char * literal);

extern void CmGetComGranularityAsLit(const ComGranularity enumValue, char * literal);

extern void CmGetComYesNoAsLit (const ComYesNo enumValue, char * literal );

extern ComActivationTime CmGetComActivationTime ( const char *literal );

extern ComOperation CmGetComOperation ( const char *literal );

extern ComGranularity CmGetComGranularity ( const char *literal );

extern ComYesNo CmGetComYesNo ( const char *literal );

//----------------------------------------------------------------------------
// MVS section start
extern void CmGetComMVTypeAsLit (const ComMVType enumValue, char * literal );

extern void CmGetComMVStatusAsLit(const ComMVStatus enumValue, char * literal);

extern void CmGetComRangeLogTypeAsLit(const ComRangeLogType enumValue, char * literal);

extern void CmGetComMvsAllowedAsLit(const ComMvsAllowed enumValue, char * literal);

extern void CmGetComMvAuditTypeAsLit(const ComMvAuditType enumValue, char * literal);

extern void CmGetComMVRefreshTypeAsLit (const ComMVRefreshType enumValue, char * literal );

extern void CmGetComLeftJoinTableTypeAsLit (const ComLeftJoinTableType enumValue, char * literal );

extern void CmGetComMVColTypeAsLit (const ComMVColType enumValue, char * literal );

extern void CmGetComIncRefStatusAsLit (const ComMVIncRefStatus enumValue, char * literal );

extern void CmGetMVOperatorTypeAsLit ( OperatorTypeEnum enumValue, char * literal );

extern void CmGetMVSUsedTableAttributeAsLit (const ComMVSUsedTableAttribute enumValue, char * literal );

extern void CmGetMVSUsageTypeAsLit (const ComMVSUsageType enumValue, char * literal);

extern void CmGetComMVAttributeAsLit (const ComMVAttribute enumValue, char * literal);


extern ComMVType CmGetComMVType ( const char *literal );

extern ComMVRefreshType  CmGetComMVRefreshType ( const char *literal );

extern ComMVStatus CmGetComMVStatus(const char *literal);

extern ComRangeLogType CmGetComRangeLogType(const char *literal);

extern ComMvsAllowed CmGetComMvsAllowed(const char *literal);

extern ComMvAuditType CmGetComMvAuditType(const char *literal);

extern ComLeftJoinTableType CmGetComLeftJoinTableType ( const char *literal );

extern ComMVColType CmGetComMVColType ( const char *literal );

extern ComMVIncRefStatus CmGetComIncRefStatus ( const char *literal );

extern OperatorTypeEnum CmGetMVOperatorTypeEnum ( const char *literal );

extern ComMVSUsedTableAttribute CmGetMVSUsedTableAttribute ( const char *literal );

extern ComMVSUsageType CmGetMVSUsageType ( const char *literal );

extern ComMVAttribute  CmGetComMVAttribute( const char *literal );



// MVS section end
//----------------------------------------------------------------------------
// Histogramssection starts

extern void CmGetHistReasonTypeAsLit (const ComHistReasonType enumValue, char*literal);

extern ComHistReasonType CmGetHistReasonType (const char *literal);

// Histograms section end
//----------------------------------------------------------------------------

// [Distribution/Versioning]
inline ComSchemaOperation CmGetSchemaOperation (const char * literal)
  { return ComSchemaOperationLiteralToEnum (literal); }

inline void CmGetSchemaOperationAsLit (const ComSchemaOperation enumValue, char * literal)
  { ComSchemaOperationEnumToLiteral (enumValue,  literal); }


// Schema Type Translations
//----------------------------------------------------------------------------
extern void CmGetComSchemaTypeLit (const ComSchemaType enumValue, char * literal );

extern ComSchemaType CmGetComSchemaType ( const char *literal );



//----------------------------------------------------------------------------
// Version 2000 -> 1200 grantor/grantee translation functions.
inline void CmOwnerGrantorToUserGrantor (char * grantor_type)
{
  if (!strcmp (grantor_type, COM_SCHEMA_OWNER_GRANTOR_LIT))
    strcpy (grantor_type, COM_USER_GRANTOR_LIT);
}

inline void CmOwnerGranteeToUserGrantee (char * grantee_type)
{
  if (!strcmp (grantee_type, COM_SCHEMA_OWNER_GRANTEE_LIT))
    strcpy (grantee_type, COM_USER_GRANTEE_LIT);
}

//----------------------------------------------------------------------------
// Version 2000 grantor/grantee translation function.
// These will be used for
// - version 1200->2000 translations
// - early version 2000-> real version 2000 translations (fixup for internal-only use)
void CmTranslateV1200ToV2000Privs ( ComGrantorType & grantorType
                                  , ComGranteeType & granteeType
                                  , const ComUserID & grantor
                                  , const ComUserID & objectOwner
                                  , const ComUserID & schemaOwner);

//----------------------------------------------------------------------------
// Exception table type for EXCEPTION_USAGE table

extern void CmGetExceptionTableTypeAsLit (const ComExceptionTableType enumValue, char*literal);

extern ComExceptionTableType CmGetExceptionTableType (const char *literal);

//----------------------------------------------------------------------------
// V2400 / LDAP / Sequence Generators

void CmGetRoleIdStatusAsLit (const ComRoleIdStatus enumValue, char * literal);
ComRoleIdStatus CmGetRoleIdStatus (const char * literal);

void CmGetSequenceGeneratorTypeAsLit (const ComSequenceGeneratorType enumValue, char * literal);
ComSequenceGeneratorType CmGetSequenceGeneratorType (const char * literal);

//---------------------------------------------------------------------------
// Added for security in SQ 1.0
// Translation for columns that hold ComIdClass (G, R, U for Group, Role and User)
extern void CmGetComIdClassAsLit (const ComIdClass enumValue, char * literal);
extern ComIdClass CmGetLitAsIdClass (const char * literal);

// Translation for ID status in ID_MAPPING table
extern void CmGetComIdStatusAsLit (const ComIdStatus enumValue, char * literal);
extern ComIdStatus CmGetLitAsIdStatus (const char * literal);

//---------------------------------------------------------------------------

void CmAnsiNameToUCS2(const NAString &inMbs, NAWString &outWcs);
void CmAnsiNameToUTF8(const NAWString &inWcs, NAString &outMbs);

//---------------------------------------------------------------------------

#endif
