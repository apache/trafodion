/* -*-C++-*- */
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
#ifndef ELEMDDLNODE_H
#define ELEMDDLNODE_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLNode.h
 * Description:  Base class representing generic parse nodes in DDL
 *               statements
 *
 *
 * Created:      3/29/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "NATraceList.h"   // gets definition of NATraceList  
#include "ExprNode.h"
#include "ComASSERT.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLNode;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class BindWA;
class ElemDDLAlterTableMove;
class ElemDDLAuthSchema;
class ElemDDLLibClientFilename;
class ElemDDLLibClientName;
class ElemDDLLibPathName;
class ElemDDLColDef;
class ElemProxyColDef;
class ElemDDLColDefault;
class ElemDDLColHeading;
class ElemDDLColName;
class ElemDDLColNameList;
class ElemDDLColNameListNode;
class ElemDDLColRef;
class ElemDDLColRefList;
class ElemDDLColViewDef;
class ElemDDLConstraint;
class ElemDDLConstraintAttr;
class ElemDDLConstraintAttrDroppable;
class ElemDDLConstraintAttrEnforced;
class ElemDDLConstraintCheck;
class ElemDDLConstraintName;
class ElemDDLConstraintNameList;
class ElemDDLConstraintNotNull;
class ElemDDLLobAttrs;
class ElemDDLSeabaseSerialized;
class ElemDDLLoggable;
class ElemDDLConstraintPK;
class ElemDDLConstraintPKColumn;
class ElemDDLConstraintRI;
class ElemDDLConstraintUnique;
class ElemDDLCreateMVOneAttributeTableList; // MV OZ
class ElemDDLDivisionClause;
class ElemDDLFileAttr;
class ElemDDLFileAttrAllocate;
class ElemDDLFileAttrAudit;
class ElemDDLFileAttrAuditCompress;
class ElemDDLFileAttrBlockSize;
class ElemDDLFileAttrBuffered;
class ElemDDLFileAttrClause;
class ElemDDLFileAttrClearOnPurge;
class ElemDDLFileAttrCompression;
class ElemDDLFileAttrDCompress;
class ElemDDLFileAttrDeallocate;
class ElemDDLFileAttrICompress;
class ElemDDLFileAttrList;
class ElemDDLPartnAttrList;
class ElemDDLFileAttrMaxSize;
class ElemDDLFileAttrExtents;
class ElemDDLFileAttrMaxExtents;
class ElemDDLFileAttrUID;
class ElemDDLFileAttrRowFormat;
class ElemDDLFileAttrColFam;
class ElemDDLFileAttrNoLabelUpdate;
class ElemDDLFileAttrOwner;
//++ MV
class ElemDDLFileAttrRangeLog;
class ElemDDLFileAttrLockOnRefresh;
class ElemDDLFileAttrInsertLog;
class ElemDDLFileAttrMvsAllowed;
class ElemDDLMVFileAttrClause;
class ElemDDLFileAttrMVCommitEach;
class ElemDDLFileAttrMvAudit;

//-- MV

class ElemDDLFileAttrPOSNumPartns;
class ElemDDLFileAttrPOSTableSize;
class ElemDDLFileAttrPOSDiskPool;
class ElemDDLFileAttrPOSIgnore;

class ElemDDLGrantee;
class ElemDDLKeyValue;
class ElemDDLKeyValueList;
class ElemDDLLibrary;
class ElemDDLLike;
class ElemDDLLikeCreateTable;
class ElemDDLLikeOpt;
class ElemDDLLikeOptWithoutConstraints;
class ElemDDLLikeOptWithHeadings;
class ElemDDLLikeOptWithHorizontalPartitions;
class ElemDDLLikeOptWithoutSalt;
class ElemDDLLikeSaltClause;
class ElemDDLLikeOptWithoutDivision;
class ElemDDLLikeLimitColumnLength;
class ElemDDLLikeOptWithoutRowFormat;
class ElemDDLLikeOptWithoutLobColumns;
class ElemDDLList;
class ElemDDLLocation;
class ElemDDLOptionList;
class ElemDDLParallelExec;
class ElemDDLParamDef;
class ElemDDLPartition;
class ElemDDLPartitionByOpt;
class ElemDDLPartitionByColumnList;
class ElemDDLPartitionClause;
class ElemDDLPartitionList;
class ElemDDLPartitionRange;
class ElemDDLPartitionSingle;
class ElemDDLPartitionSystem;
class ElemDDLPassThroughParamDef;
class ElemDDLPrivAct;
class ElemDDLPrivActAlter;
class ElemDDLPrivActAlterLibrary;
class ElemDDLPrivActAlterMV;
class ElemDDLPrivActAlterMVGroup;
class ElemDDLPrivActAlterSynonym;
class ElemDDLPrivActAlterRoutine;
class ElemDDLPrivActAlterRoutineAction;
class ElemDDLPrivActAlterTable;
class ElemDDLPrivActAlterTrigger;
class ElemDDLPrivActAlterView;
class ElemDDLPrivActCreate;
class ElemDDLPrivActCreateLibrary;
class ElemDDLPrivActCreateMV;
class ElemDDLPrivActCreateMVGroup;
class ElemDDLPrivActCreateProcedure;
class ElemDDLPrivActCreateRoutine;
class ElemDDLPrivActCreateRoutineAction;
class ElemDDLPrivActCreateSynonym;
class ElemDDLPrivActCreateTable;
class ElemDDLPrivActCreateTrigger;
class ElemDDLPrivActCreateView;
class ElemDDLPrivActDBA;
class ElemDDLPrivActDelete;
class ElemDDLPrivActDrop;
class ElemDDLPrivActDropLibrary;
class ElemDDLPrivActDropMV;
class ElemDDLPrivActDropMVGroup;
class ElemDDLPrivActDropProcedure;
class ElemDDLPrivActDropRoutine;
class ElemDDLPrivActDropRoutineAction;
class ElemDDLPrivActDropSynonym;
class ElemDDLPrivActDropTable;
class ElemDDLPrivActDropTrigger;
class ElemDDLPrivActDropView;
class ElemDDLPrivActInsert;
class ElemDDLPrivActMaintain;
class ElemDDLPrivActReferences;
class ElemDDLPrivActRefresh;
class ElemDDLPrivActReorg;
class ElemDDLPrivActSelect;
class ElemDDLPrivActTransform;
class ElemDDLPrivActUpdate;
class ElemDDLPrivActUpdateStats;
class ElemDDLPrivActAllDDL;
class ElemDDLPrivActAllDML;
class ElemDDLPrivActAllOther;
class ElemDDLPrivActUsage;
class ElemDDLPrivActWithColumns;
class ElemDDLPrivileges;
class ElemDDLSaltOptionsClause;
class ElemDDLRefAct;
class ElemDDLRefActCascade;
class ElemDDLRefActNoAction;
class ElemDDLRefActRestrict;
class ElemDDLRefActSetDefault;
class ElemDDLRefActSetNull;
class ElemDDLRefTrigAct;
class ElemDDLRefTrigActDeleteRule;
class ElemDDLRefTrigActUpdateRule;
class ElemDDLReferences;
class ElemDDLTableFeature;
class ElemDDLHbaseOptions;
class ElemDDLSchemaName;
class ElemDDLSGOptions;
class ElemDDLSGOption;
class ElemDDLSGOptionStartValue;
class ElemDDLSGOptionMaxValue;
class ElemDDLSGOptionMinValue;
class ElemDDLSGOptionIncrement;
class ElemDDLSGOptionCacheOption;
class ElemDDLSGOptionCycleOption;
class ElemDDLSGOptionDatatype;
class ElemDDLStoreOpt;
class ElemDDLStoreOptDefault;
class ElemDDLStoreOptEntryOrder;
class ElemDDLStoreOptKeyColumnList;
class ElemDDLStoreOptNondroppablePK;
class ElemDDLUdfExecutionMode;
class ElemDDLUdfFinalCall;
class ElemDDLUdfOptimizationHint;
class ElemDDLUdfParallelism;
class ElemDDLUdfSpecialAttributes;
class ElemDDLUdfStateAreaSize;
class ElemDDLUdfVersionTag;
class ElemDDLUdrDeterministic;
class ElemDDLUdrExternalName;
class ElemDDLUdrExternalPath;
class ElemDDLUdrIsolate;
class ElemDDLUdrLanguage;
class ElemDDLUdrLibrary;
class ElemDDLUdrMaxResults;
class ElemDDLUdrParamStyle;
class ElemDDLUdrSqlAccess;
class ElemDDLUdrTransaction;
class ElemDDLUdrExternalSecurity;
class ElemDDLUudfParamDef;
class ElemDDLWithCheckOption;
class ElemDDLWithGrantOption;
class ElemDDLIndexPopulateOption;
class ElemDDLQualName; // MV - RG
class StmtDDLAddConstraint;
class StmtDDLAddConstraintCheck;
class StmtDDLAddConstraintPK;
class StmtDDLAddConstraintRI;
class StmtDDLAddConstraintUnique;
class StmtDDLAlterAuditConfig;
class StmtDDLAlterCatalog;
class StmtDDLAlterSchema;
class StmtDDLAlterIndex;
class StmtDDLAlterIndexAttribute;
class StmtDDLAlterIndexHBaseOptions;
class StmtDDLAlterLibrary;
class StmtDDLAlterTable;
class StmtDDLAlterTableAttribute;
class StmtDDLAlterTableAddColumn;
class StmtDDLAlterTableDropColumn;
class StmtDDLAlterTableAlterColumnLoggable;
class StmtDDLAlterTableDisableIndex;
class StmtDDLAlterTableEnableIndex;
class StmtDDLAlterTableColumn;
class StmtDDLAlterTableMove;
class StmtDDLAlterTablePartition;
class StmtDDLAlterTableRename;
class StmtDDLAlterTableStoredDesc;
class StmtDDLAlterTableNamespace;
class StmtDDLAlterTableAlterColumnDatatype;
class StmtDDLAlterTableAlterColumnRename;
class StmtDDLAlterTableAlterColumnDefaultValue;
class StmtDDLAlterTableAlterColumnSetSGOption;
class StmtDDLAlterTableSetConstraint;
class StmtDDLAlterTableToggleConstraint;
class StmtDDLAlterTableHBaseOptions;
class StmtDDLAlterMvRGroup;
class StmtDDLAlterRoutine;
class StmtDDLAlterTrigger;
class StmtDDLAlterMV; 
class StmtDDLAlterUser;
class StmtDDLAlterView;
class StmtDDLAlterDatabase;
class StmtDDLCreateCatalog;
class StmtDDLCreateComponentPrivilege;
class StmtDDLCreateIndex;
class StmtDDLPopulateIndex;
class StmtDDLCreateLibrary;
class StmtDDLCreateRoutine;
class StmtDDLCreateSchema;
class StmtDDLCreateSequence;
class StmtDDLCreateTable;
class StmtDDLCreateHbaseTable;
class StmtDDLCreateMvRGroup;
class StmtDDLCreateTrigger;
class StmtDDLCreateMV;  
class StmtDDLCreateView;
class StmtDDLDropCatalog;
class StmtDDLDropComponentPrivilege;
class StmtDDLDropIndex;
class StmtDDLDropLibrary;
class StmtDDLDropRoutine;
class StmtDDLDropSequence;
class StmtDDLDropSchema;
class StmtDDLDropSQL;
class StmtDDLDropTable;
class StmtDDLDropHbaseTable;
class StmtDDLDropMvRGroup;
class StmtDDLDropTrigger;
class StmtDDLDropMV;  
class StmtDDLDropView;
class StmtDDLGiveAll;
class StmtDDLGiveCatalog;
class StmtDDLGiveObject;
class StmtDDLGiveSchema;
class StmtDDLGrant;
class StmtDDLGrantComponentPrivilege;
class StmtDDLSchGrant;
class StmtDDLInitializeSQL;
class StmtDDLReInitializeSQL;
class StmtDDLRevoke;
class StmtDDLRevokeComponentPrivilege;
class StmtDDLSchRevoke;
class StmtDDLDropConstraint;
class StmtDDLDropModule;
class StmtDMLSetTransaction;
class StmtDDLRegisterCatalog;
class StmtDDLUnregisterCatalog;
class StmtDDLCreateSynonym;
class StmtDDLAlterSynonym;
class StmtDDLDropSynonym;
class StmtDDLCreateExceptionTable;
class StmtDDLDropExceptionTable;
class StmtDDLRegisterComponent;
class StmtDDLRegisterUser;
class StmtDDLRegOrUnregObject;
class StmtDDLCreateRole;
class StmtDDLRoleGrant;
class StmtDDLCleanupObjects;
class StmtDDLonHiveObjects;
class StmtDDLCommentOn;

class QualifiedName;

// -----------------------------------------------------------------------
//
// -----------------------------------------------------------------------
class ElemDDLNode : public ExprNode
{

public:

  enum WhichAll {ALL_DML, ALL_DDL, ALL};

  // constructor
  ElemDDLNode(OperatorTypeEnum otype = ELM_ANY_ELEM)
  : ExprNode(otype)
  {
    setNonCacheable();
  }

  // virtual destructor
  virtual ~ElemDDLNode();

  // perform a safe type cast (return NULL pointer for illegal casts)
  virtual ElemDDLNode                   * castToElemDDLNode();
  virtual const ElemDDLNode             * castToElemDDLNode() const;
  virtual ElemDDLAlterTableMove         * castToElemDDLAlterTableMove();
  virtual ElemDDLAuthSchema             * castToElemDDLAuthSchema();
  virtual ElemDDLLibClientFilename      * castToElemDDLLibClientFilename();
  virtual ElemDDLLibClientName          * castToElemDDLLibClientName();
  virtual ElemDDLLibPathName            * castToElemDDLLibPathName();
  virtual ElemDDLColDef                 * castToElemDDLColDef();
  virtual ElemProxyColDef               * castToElemProxyColDef();
  virtual ElemDDLColDefault             * castToElemDDLColDefault();
  virtual ElemDDLColHeading             * castToElemDDLColHeading();
  virtual ElemDDLColName                * castToElemDDLColName();
  virtual ElemDDLColNameList            * castToElemDDLColNameList();
  virtual ElemDDLColNameListNode        * castToElemDDLColNameListNode();
  virtual ElemDDLColRef                 * castToElemDDLColRef();
  virtual ElemDDLColRefList             * castToElemDDLColRefList();
  virtual ElemDDLColViewDef             * castToElemDDLColViewDef();
  virtual ElemDDLConstraint             * castToElemDDLConstraint();
  virtual ElemDDLConstraintAttr         * castToElemDDLConstraintAttr();
  virtual ElemDDLConstraintAttrDroppable*
                castToElemDDLConstraintAttrDroppable();
  virtual ElemDDLConstraintAttrEnforced*
                castToElemDDLConstraintAttrEnforced();
  virtual ElemDDLConstraintCheck        * castToElemDDLConstraintCheck();
  virtual ElemDDLConstraintName         * castToElemDDLConstraintName();
  virtual ElemDDLConstraintNameList     * castToElemDDLConstraintNameList();
  virtual ElemDDLConstraintNotNull      * castToElemDDLConstraintNotNull();
  virtual ElemDDLLoggable               * castToElemDDLLoggable();
  virtual ElemDDLLobAttrs               * castToElemDDLLobAttrs();
  virtual ElemDDLSeabaseSerialized  * castToElemDDLSeabaseSerialized();
  virtual NABoolean isConstraintNotNull() const { return FALSE; }
  virtual ElemDDLConstraintPK           * castToElemDDLConstraintPK();
  virtual ElemDDLConstraintPKColumn     * castToElemDDLConstraintPKColumn();
  virtual ElemDDLConstraintRI           * castToElemDDLConstraintRI();
  virtual ElemDDLConstraintUnique       * castToElemDDLConstraintUnique();
  virtual ElemDDLCreateMVOneAttributeTableList  * castToElemDDLCreateMVOneAttributeTableList(); // MV OZ
  virtual ElemDDLDivisionClause         * castToElemDDLDivisionClause();
  virtual ElemDDLFileAttr               * castToElemDDLFileAttr();
  virtual ElemDDLFileAttrAllocate       * castToElemDDLFileAttrAllocate();
  virtual ElemDDLFileAttrAudit          * castToElemDDLFileAttrAudit();
  virtual ElemDDLFileAttrAuditCompress  * castToElemDDLFileAttrAuditCompress();
  virtual ElemDDLFileAttrBlockSize      * castToElemDDLFileAttrBlockSize();
  virtual ElemDDLFileAttrBuffered       * castToElemDDLFileAttrBuffered();
  virtual ElemDDLFileAttrClause         * castToElemDDLFileAttrClause();
  virtual ElemDDLFileAttrClearOnPurge   * castToElemDDLFileAttrClearOnPurge();
  virtual ElemDDLFileAttrCompression    * castToElemDDLFileAttrCompression();
  virtual ElemDDLFileAttrDCompress      * castToElemDDLFileAttrDCompress();
  virtual ElemDDLFileAttrDeallocate     * castToElemDDLFileAttrDeallocate();
  virtual ElemDDLFileAttrICompress      * castToElemDDLFileAttrICompress();
  virtual ElemDDLFileAttrList           * castToElemDDLFileAttrList();
  virtual ElemDDLPartnAttrList          * castToElemDDLPartnAttrList();
  virtual ElemDDLFileAttrMaxSize        * castToElemDDLFileAttrMaxSize();
  virtual ElemDDLFileAttrExtents        * castToElemDDLFileAttrExtents();
  virtual ElemDDLFileAttrMaxExtents     * castToElemDDLFileAttrMaxExtents();
  virtual ElemDDLFileAttrUID		* castToElemDDLFileAttrUID();
  virtual ElemDDLFileAttrRowFormat	* castToElemDDLFileAttrRowFormat();
  virtual ElemDDLFileAttrColFam	* castToElemDDLFileAttrColFam();
  virtual ElemDDLFileAttrNoLabelUpdate  * castToElemDDLFileAttrNoLabelUpdate();
  virtual ElemDDLFileAttrOwner          * castToElemDDLFileAttrOwner();

//++ MV
  virtual ElemDDLFileAttrRangeLog		* castToElemDDLFileAttrRangeLog(); 
  virtual ElemDDLFileAttrLockOnRefresh  * castToElemDDLFileAttrLockOnRefresh(); 
  virtual ElemDDLFileAttrInsertLog		* castToElemDDLFileAttrInsertLog();
  virtual ElemDDLFileAttrMvsAllowed		* castToElemDDLFileAttrMvsAllowed();
  virtual ElemDDLMVFileAttrClause		* castToElemDDLMVFileAttrClause();
  virtual ElemDDLFileAttrMVCommitEach	* castToElemDDLFileAttrMVCommitEach();
  virtual ElemDDLFileAttrMvAudit		* castToElemDDLFileAttrMvAudit();

//-- MV
  
  virtual ElemDDLFileAttrPOSNumPartns * castToElemDDLFileAttrPOSNumPartns();
  virtual ElemDDLFileAttrPOSTableSize * castToElemDDLFileAttrPOSTableSize();
  virtual ElemDDLFileAttrPOSDiskPool  * castToElemDDLFileAttrPOSDiskPool();
  virtual ElemDDLFileAttrPOSIgnore    * castToElemDDLFileAttrPOSIgnore();

  virtual ElemDDLGrantee                * castToElemDDLGrantee();
  virtual ElemDDLKeyValue               * castToElemDDLKeyValue();
  virtual ElemDDLKeyValueList           * castToElemDDLKeyValueList();
  virtual ElemDDLLibrary                * castToElemDDLLibrary();
  virtual ElemDDLLike                   * castToElemDDLLike();
  virtual ElemDDLLikeCreateTable        * castToElemDDLLikeCreateTable();
  virtual ElemDDLLikeOpt                * castToElemDDLLikeOpt();
  virtual ElemDDLLikeOptWithoutConstraints * castToElemDDLLikeOptWithoutConstraints();
  virtual ElemDDLLikeOptWithHeadings    * castToElemDDLLikeOptWithHeadings();
  virtual ElemDDLLikeOptWithHorizontalPartitions    * castToElemDDLLikeOptWithHorizontalPartitions();
  virtual ElemDDLLikeOptWithoutSalt     * castToElemDDLLikeOptWithoutSalt();
  virtual ElemDDLLikeSaltClause         * castToElemDDLLikeSaltClause();
  virtual ElemDDLLikeOptWithoutDivision * castToElemDDLLikeOptWithoutDivision();
  virtual ElemDDLLikeLimitColumnLength  * castToElemDDLLikeLimitColumnLength();
  virtual ElemDDLLikeOptWithoutRowFormat* castToElemDDLLikeOptWithoutRowFormat();
  virtual ElemDDLLikeOptWithoutLobColumns * castToElemDDLLikeOptWithoutLobColumns();
  virtual ElemDDLList                   * castToElemDDLList();
  virtual ElemDDLLocation               * castToElemDDLLocation();
  virtual ElemDDLOptionList             * castToElemDDLOptionList();
  virtual ElemDDLParallelExec           * castToElemDDLParallelExec();
  virtual ElemDDLParamDef               * castToElemDDLParamDef();
  virtual ElemDDLPartition              * castToElemDDLPartition();
  virtual ElemDDLPartitionByOpt         * castToElemDDLPartitionByOpt();
  virtual ElemDDLPartitionByColumnList  * castToElemDDLPartitionByColumnList();
  virtual ElemDDLPartitionClause        * castToElemDDLPartitionClause();
  virtual ElemDDLPartitionList          * castToElemDDLPartitionList();
  virtual ElemDDLPartitionRange         * castToElemDDLPartitionRange();
  virtual ElemDDLPartitionSingle        * castToElemDDLPartitionSingle();
  virtual ElemDDLPartitionSystem        * castToElemDDLPartitionSystem();  
  virtual ElemDDLPassThroughParamDef    * castToElemDDLPassThroughParamDef();
  virtual ElemDDLPrivAct                * castToElemDDLPrivAct();
  virtual ElemDDLPrivActAlter           * castToElemDDLPrivActAlter();
  virtual ElemDDLPrivActAlterLibrary    * castToElemDDLPrivActAlterLibrary();
  virtual ElemDDLPrivActAlterMV         * castToElemDDLPrivActAlterMV();
  virtual ElemDDLPrivActAlterMVGroup    * castToElemDDLPrivActAlterMVGroup();
  virtual ElemDDLPrivActAlterRoutine    * castToElemDDLPrivActAlterRoutine();
  virtual ElemDDLPrivActAlterRoutineAction * castToElemDDLPrivActAlterRoutineAction();
  virtual ElemDDLPrivActAlterSynonym    * castToElemDDLPrivActAlterSynonym();
  virtual ElemDDLPrivActAlterTable      * castToElemDDLPrivActAlterTable();
  virtual ElemDDLPrivActAlterTrigger    * castToElemDDLPrivActAlterTrigger();
  virtual ElemDDLPrivActAlterView       * castToElemDDLPrivActAlterView();
  virtual ElemDDLPrivActCreate          * castToElemDDLPrivActCreate();
  virtual ElemDDLPrivActCreateLibrary   * castToElemDDLPrivActCreateLibrary();
  virtual ElemDDLPrivActCreateMV        * castToElemDDLPrivActCreateMV();
  virtual ElemDDLPrivActCreateMVGroup   * castToElemDDLPrivActCreateMVGroup();
  virtual ElemDDLPrivActCreateProcedure * castToElemDDLPrivActCreateProcedure();
  virtual ElemDDLPrivActCreateRoutine    * castToElemDDLPrivActCreateRoutine();
  virtual ElemDDLPrivActCreateRoutineAction * castToElemDDLPrivActCreateRoutineAction();
  virtual ElemDDLPrivActCreateSynonym   * castToElemDDLPrivActCreateSynonym();
  virtual ElemDDLPrivActCreateTable     * castToElemDDLPrivActCreateTable();
  virtual ElemDDLPrivActCreateTrigger   * castToElemDDLPrivActCreateTrigger();
  virtual ElemDDLPrivActCreateView      * castToElemDDLPrivActCreateView();
  virtual ElemDDLPrivActDBA             * castToElemDDLPrivActDBA();
  virtual ElemDDLPrivActDelete          * castToElemDDLPrivActDelete();
  virtual ElemDDLPrivActDrop            * castToElemDDLPrivActDrop();
  virtual ElemDDLPrivActDropLibrary     * castToElemDDLPrivActDropLibrary();
  virtual ElemDDLPrivActDropMV          * castToElemDDLPrivActDropMV();
  virtual ElemDDLPrivActDropMVGroup     * castToElemDDLPrivActDropMVGroup();
  virtual ElemDDLPrivActDropProcedure   * castToElemDDLPrivActDropProcedure();
  virtual ElemDDLPrivActDropRoutine    * castToElemDDLPrivActDropRoutine();
  virtual ElemDDLPrivActDropRoutineAction * castToElemDDLPrivActDropRoutineAction();
  virtual ElemDDLPrivActDropSynonym     * castToElemDDLPrivActDropSynonym();
  virtual ElemDDLPrivActDropTable       * castToElemDDLPrivActDropTable();
  virtual ElemDDLPrivActDropTrigger     * castToElemDDLPrivActDropTrigger();
  virtual ElemDDLPrivActDropView        * castToElemDDLPrivActDropView();
  virtual ElemDDLPrivActInsert          * castToElemDDLPrivActInsert();
  virtual ElemDDLPrivActMaintain        * castToElemDDLPrivActMaintain();
  virtual ElemDDLPrivActReferences      * castToElemDDLPrivActReferences();
  virtual ElemDDLPrivActRefresh         * castToElemDDLPrivActRefresh();
  virtual ElemDDLPrivActReorg           * castToElemDDLPrivActReorg();
  virtual ElemDDLPrivActSelect          * castToElemDDLPrivActSelect();
  virtual ElemDDLPrivActTransform       * castToElemDDLPrivActTransform();
  virtual ElemDDLPrivActUpdate          * castToElemDDLPrivActUpdate();
  virtual ElemDDLPrivActUpdateStats     * castToElemDDLPrivActUpdateStats();
  virtual ElemDDLPrivActAllDDL          * castToElemDDLPrivActAllDDL();
  virtual ElemDDLPrivActAllDML          * castToElemDDLPrivActAllDML();
  virtual ElemDDLPrivActAllOther        * castToElemDDLPrivActAllOther();
  virtual ElemDDLPrivActUsage           * castToElemDDLPrivActUsage();
  virtual ElemDDLPrivActWithColumns     * castToElemDDLPrivActWithColumns();
  virtual ElemDDLPrivileges             * castToElemDDLPrivileges();  
  virtual ElemDDLRefAct                 * castToElemDDLRefAct();
  virtual ElemDDLRefActCascade          * castToElemDDLRefActCascade();
  virtual ElemDDLRefActNoAction         * castToElemDDLRefActNoAction();
  virtual ElemDDLRefActRestrict         * castToElemDDLRefActRestrict();
  virtual ElemDDLRefActSetDefault       * castToElemDDLRefActSetDefault();
  virtual ElemDDLRefActSetNull          * castToElemDDLRefActSetNull();
  virtual ElemDDLRefTrigAct             * castToElemDDLRefTrigAct();
  virtual ElemDDLRefTrigActDeleteRule   * castToElemDDLRefTrigActDeleteRule();
  virtual ElemDDLRefTrigActUpdateRule   * castToElemDDLRefTrigActUpdateRule();
  
  virtual ElemDDLReferences             * castToElemDDLReferences();
  virtual ElemDDLSaltOptionsClause      * castToElemDDLSaltOptionsClause();
  virtual ElemDDLSchemaName             * castToElemDDLSchemaName();

  virtual ElemDDLSGOptions              * castToElemDDLSGOptions();
  virtual ElemDDLSGOption               * castToElemDDLSGOption();
  virtual ElemDDLSGOptionStartValue     * castToElemDDLSGOptionStartValue();
  virtual ElemDDLSGOptionMinValue       * castToElemDDLSGOptionMinValue();
  virtual ElemDDLSGOptionMaxValue       * castToElemDDLSGOptionMaxValue();
  virtual ElemDDLSGOptionIncrement      * castToElemDDLSGOptionIncrement();
  virtual ElemDDLSGOptionCacheOption    * castToElemDDLSGOptionCacheOption();
  virtual ElemDDLSGOptionCycleOption    * castToElemDDLSGOptionCycleOption();
  virtual ElemDDLSGOptionDatatype    * castToElemDDLSGOptionDatatype();
  
  virtual ElemDDLStoreOpt               * castToElemDDLStoreOpt();
  virtual ElemDDLStoreOptEntryOrder     * castToElemDDLStoreOptEntryOrder();
  virtual ElemDDLStoreOptDefault        * castToElemDDLStoreOptDefault();
  virtual ElemDDLStoreOptKeyColumnList  * castToElemDDLStoreOptKeyColumnList();
  virtual ElemDDLStoreOptNondroppablePK *
                castToElemDDLStoreOptNondroppablePK();

  virtual ElemDDLTableFeature		* castToElemDDLTableFeature();
  virtual ElemDDLHbaseOptions  	* castToElemDDLHbaseOptions();

  virtual ElemDDLUdfExecutionMode       * castToElemDDLUdfExecutionMode();
  virtual ElemDDLUdfFinalCall           * castToElemDDLUdfFinalCall();
  virtual ElemDDLUdfOptimizationHint    * castToElemDDLUdfOptimizationHint();
  virtual ElemDDLUdfParallelism         * castToElemDDLUdfParallelism();
  virtual ElemDDLUdfSpecialAttributes   * castToElemDDLUdfSpecialAttributes();
  virtual ElemDDLUdfStateAreaSize       * castToElemDDLUdfStateAreaSize();
  virtual ElemDDLUdfVersionTag          * castToElemDDLUdfVersionTag();
  virtual ElemDDLUdrDeterministic       * castToElemDDLUdrDeterministic();
  virtual ElemDDLUdrExternalName        * castToElemDDLUdrExternalName();
  virtual ElemDDLUdrExternalPath        * castToElemDDLUdrExternalPath();
  virtual ElemDDLUdrIsolate             * castToElemDDLUdrIsolate();
  virtual ElemDDLUdrLanguage            * castToElemDDLUdrLanguage();
  virtual ElemDDLUdrLibrary             * castToElemDDLUdrLibrary();
  virtual ElemDDLUdrMaxResults          * castToElemDDLUdrMaxResults();
  virtual ElemDDLUdrParamStyle          * castToElemDDLUdrParamStyle();
  virtual ElemDDLUdrSqlAccess           * castToElemDDLUdrSqlAccess();
  virtual ElemDDLUdrTransaction         * castToElemDDLUdrTransaction();
  virtual ElemDDLUdrExternalSecurity    * castToElemDDLUdrExternalSecurity();
  virtual ElemDDLUudfParamDef           * castToElemDDLUudfParamDef();
  virtual ElemDDLWithCheckOption        * castToElemDDLWithCheckOption();
  virtual ElemDDLWithGrantOption        * castToElemDDLWithGrantOption();
  virtual ElemDDLIndexPopulateOption    * castToElemDDLIndexPopulateOption();
  virtual ElemDDLQualName				* castToElemDDLQualName();  // MV - RG
  virtual StmtDDLAddConstraint          * castToStmtDDLAddConstraint();
  virtual StmtDDLAddConstraintCheck     * castToStmtDDLAddConstraintCheck();
  virtual StmtDDLAddConstraintPK        * castToStmtDDLAddConstraintPK();
  virtual StmtDDLAddConstraintRI        * castToStmtDDLAddConstraintRI();
  virtual StmtDDLAddConstraintUnique    * castToStmtDDLAddConstraintUnique(); 
  virtual StmtDDLAlterAuditConfig       * castToStmtDDLAlterAuditConfig();
  virtual StmtDDLAlterCatalog           * castToStmtDDLAlterCatalog();
  virtual StmtDDLAlterSchema            * castToStmtDDLAlterSchema();
  virtual StmtDDLAlterIndex             * castToStmtDDLAlterIndex();
  virtual StmtDDLAlterIndexAttribute    * castToStmtDDLAlterIndexAttribute();
  virtual StmtDDLAlterIndexHBaseOptions * castToStmtDDLAlterIndexHBaseOptions();
  virtual StmtDDLAlterLibrary           * castToStmtDDLAlterLibrary();
  virtual StmtDDLAlterTable             * castToStmtDDLAlterTable();
  virtual StmtDDLAlterTableAttribute    * castToStmtDDLAlterTableAttribute();
  virtual StmtDDLAlterTableAddColumn    * castToStmtDDLAlterTableAddColumn();
  virtual StmtDDLAlterTableDropColumn    * castToStmtDDLAlterTableDropColumn();
  virtual StmtDDLAlterTableAlterColumnLoggable * 
						castToStmtDDLAlterTableAlterColumnLoggable(); //++ MV
  virtual StmtDDLAlterTableDisableIndex * castToStmtDDLAlterTableDisableIndex();
  virtual StmtDDLAlterTableEnableIndex * castToStmtDDLAlterTableEnableIndex();

  virtual StmtDDLAlterTableColumn       * castToStmtDDLAlterTableColumn();
  virtual StmtDDLAlterTableMove         * castToStmtDDLAlterTableMove();
  virtual StmtDDLAlterTableHBaseOptions * castToStmtDDLAlterTableHBaseOptions();
  virtual StmtDDLAlterTablePartition    * castToStmtDDLAlterTablePartition();
  virtual StmtDDLAlterTableRename       * castToStmtDDLAlterTableRename();
  virtual StmtDDLAlterTableStoredDesc   * castToStmtDDLAlterTableStoredDesc();
  virtual StmtDDLAlterTableNamespace    * castToStmtDDLAlterTableNamespace();
  virtual StmtDDLAlterTableAlterColumnDefaultValue* 
                castToStmtDDLAlterTableAlterColumnDefaultValue();
  virtual StmtDDLAlterTableAlterColumnDatatype* 
                castToStmtDDLAlterTableAlterColumnDatatype();
  virtual StmtDDLAlterTableAlterColumnRename* 
                castToStmtDDLAlterTableAlterColumnRename();
  virtual StmtDDLAlterTableSetConstraint*
                castToStmtDDLAlterTableSetConstraint();
  virtual StmtDDLAlterTableToggleConstraint*
                castToStmtDDLAlterTableToggleConstraint();
  virtual StmtDDLAlterTableAlterColumnSetSGOption* 
                castToStmtDDLAlterTableAlterColumnSetSGOption();
  virtual StmtDDLAlterMvRGroup			* castToStmtDDLAlterMvRGroup();
  virtual StmtDDLAlterRoutine           * castToStmtDDLAlterRoutine();
  virtual StmtDDLAlterTrigger           * castToStmtDDLAlterTrigger();
  virtual StmtDDLAlterMV                * castToStmtDDLAlterMV();  
  virtual StmtDDLAlterUser              * castToStmtDDLAlterUser();
  virtual StmtDDLAlterView              * castToStmtDDLAlterView();
  virtual StmtDDLAlterDatabase          * castToStmtDDLAlterDatabase();

  virtual StmtDDLCreateCatalog          * castToStmtDDLCreateCatalog();
  virtual StmtDDLCreateComponentPrivilege * castToStmtDDLCreateComponentPrivilege();
  virtual StmtDDLCreateIndex            * castToStmtDDLCreateIndex();
  virtual StmtDDLPopulateIndex            * castToStmtDDLPopulateIndex();
  virtual StmtDDLCreateLibrary          * castToStmtDDLCreateLibrary();
  virtual StmtDDLCreateRoutine          * castToStmtDDLCreateRoutine();
  virtual StmtDDLCreateSchema           * castToStmtDDLCreateSchema();
  virtual StmtDDLCreateSequence            * castToStmtDDLCreateSequence();
  virtual StmtDDLCreateTable            * castToStmtDDLCreateTable();
  virtual StmtDDLCreateHbaseTable            * castToStmtDDLCreateHbaseTable();
  virtual StmtDDLCreateMvRGroup         * castToStmtDDLCreateMvRGroup();
  virtual StmtDDLCreateTrigger          * castToStmtDDLCreateTrigger();
  virtual StmtDDLCreateMV               * castToStmtDDLCreateMV(); 
  virtual StmtDDLCreateView             * castToStmtDDLCreateView();
  virtual StmtDDLDropCatalog            * castToStmtDDLDropCatalog();
  virtual StmtDDLDropComponentPrivilege * castToStmtDDLDropComponentPrivilege();
  virtual StmtDDLDropIndex              * castToStmtDDLDropIndex();
  virtual StmtDDLDropLibrary            * castToStmtDDLDropLibrary();
  virtual StmtDDLDropModule             * castToStmtDDLDropModule();
  virtual StmtDDLDropRoutine            * castToStmtDDLDropRoutine();
  virtual StmtDDLDropSchema             * castToStmtDDLDropSchema();
  virtual StmtDDLDropSequence            * castToStmtDDLDropSequence();
  virtual StmtDDLDropSQL                * castToStmtDDLDropSQL();
  virtual StmtDDLDropTable              * castToStmtDDLDropTable();
  virtual StmtDDLDropHbaseTable              * castToStmtDDLDropHbaseTable();
  virtual StmtDDLDropMvRGroup		* castToStmtDDLDropMvRGroup();
  virtual StmtDDLDropTrigger            * castToStmtDDLDropTrigger(); 
  virtual StmtDDLDropMV                 * castToStmtDDLDropMV();  
  virtual StmtDDLDropView               * castToStmtDDLDropView();
  virtual StmtDDLGiveAll                * castToStmtDDLGiveAll();
  virtual StmtDDLGiveCatalog            * castToStmtDDLGiveCatalog();
  virtual StmtDDLGiveObject             * castToStmtDDLGiveObject();
  virtual StmtDDLGiveSchema             * castToStmtDDLGiveSchema();
  virtual StmtDDLGrant                  * castToStmtDDLGrant();
  virtual StmtDDLGrantComponentPrivilege * castToStmtDDLGrantComponentPrivilege();
  virtual StmtDDLSchGrant		* castToStmtDDLSchGrant();
  virtual StmtDDLReInitializeSQL        * castToStmtDDLReInitializeSQL();
  virtual StmtDDLInitializeSQL          * castToStmtDDLInitializeSQL();
  virtual StmtDDLRevoke                 * castToStmtDDLRevoke();
  virtual StmtDDLRevokeComponentPrivilege * castToStmtDDLRevokeComponentPrivilege();
  virtual StmtDDLSchRevoke              * castToStmtDDLSchRevoke();
  virtual StmtDDLDropConstraint         * castToStmtDDLDropConstraint();
  virtual StmtDDLRegisterCatalog        * castToStmtDDLRegisterCatalog();
  virtual StmtDDLUnregisterCatalog      * castToStmtDDLUnregisterCatalog();
  virtual StmtDDLCreateSynonym          * castToStmtDDLCreateSynonym();
  virtual StmtDDLAlterSynonym           * castToStmtDDLAlterSynonym();
  virtual StmtDDLDropSynonym            * castToStmtDDLDropSynonym();
  virtual StmtDDLCreateExceptionTable   * castToStmtDDLCreateExceptionTable();
  virtual StmtDDLDropExceptionTable     * castToStmtDDLDropExceptionTable();
  virtual StmtDDLRegisterComponent      * castToStmtDDLRegisterComponent();
  virtual StmtDDLRegisterUser           * castToStmtDDLRegisterUser();
  virtual StmtDDLRegOrUnregObject       * castToStmtDDLRegOrUnregObject();
  virtual StmtDDLCreateRole             * castToStmtDDLCreateRole();
  virtual StmtDDLRoleGrant              * castToStmtDDLRoleGrant();
  virtual StmtDDLCleanupObjects         * castToStmtDDLCleanupObjects();
  virtual StmtDDLCommentOn              * castToStmtDDLCommentOn();
  virtual StmtDDLonHiveObjects            * castToStmtDDLonHiveObjects();

  //
  // operator
  //

  virtual ElemDDLNode * operator[](CollIndex index);

        // treats this node as an array (of one element).  For
        // more information about the following methods, please
        // read the descriptions of the corresponding methods
        // in file ElemDDLList.h

  //
  // accessors
  //

  virtual CollIndex entries() const;

        // treats this node as an array (of one element).
        // Returns 1, the number of element in this array.

  virtual Int32 getArity() const;

        // gets the degree of this node (the number of
        // child parse node linking to this node).  If
        // this node does not have any children, returns
        // the value 0.

  virtual ExprNode * getChild(Lng32 index);

        // returns the pointer to the index child parse node.
        // If the specified index is out-of-range, this method
        // invokes the macro ABORT.  If this node does not have
        // any child parse nodes, returns the NULL pointer value.

  virtual void traverseList(ElemDDLNode * pNode,
                            void (*visitNode)(ElemDDLNode *,
                                              CollIndex,
                                              ElemDDLNode *));

        // treat this node as a list (of one element).  The
        // list is represented by a left linear tree.  For more
        // information about this method, please read the
        // description of the corresponding method in file
        // ElemDDLList.h

  //
  // mutators
  //

  virtual void setChild(Lng32 index, ExprNode * pChildNode);

        // modifies the indexed pointers in this node to
        // point to a child parse node.  If the specified
        // index is out-of-range or if this node does not
        // have any child parse nodes, this method invokes
        // the macro ABORT.

  //
  // method for binding
  //

  virtual ExprNode * bindNode(BindWA * pBindWA);

  // method to apply defaults and do object name validation
  virtual NABoolean applyDefaultsAndValidateObject(BindWA * pBindWA,
						   QualifiedName *qn);

  //
  // methods for tracing
  //

  virtual const NAString displayLabel1() const;
  virtual const NAString displayLabel2() const;
  virtual const NAString displayLabel3() const;
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;
  virtual void print(FILE * f = stdout,
                     const char * prefix = "",
                     const char * suffix = "") const;

  virtual NAString getSyntax() const {ComASSERT(FALSE); return "";}

private:

}; // class ElemDDLNode

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLNode
// -----------------------------------------------------------------------


#endif // ELEMDDLNODE_H

