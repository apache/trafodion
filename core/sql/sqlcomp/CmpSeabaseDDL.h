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

/*
 *****************************************************************************
 *
 * File:         CmpSeabaseDDL.h
 * Description:  This file describes the DDL classes for Trafodion
 *               
 * Contents:
 *   Metadata table descriptors
 *   class CmpSeabaseDDL
 *
 *****************************************************************************
*/

#ifndef _CMP_SEABASE_DDL_H_
#define _CMP_SEABASE_DDL_H_

#include "ComTdb.h"
#include "ExpHbaseDefs.h"
#include "ExpLOBenums.h"
#include "NADefaults.h"
#include "NAColumn.h"
#include "CmpMessage.h"
#include "PrivMgrDefs.h"
#include "PrivMgrMD.h"
#include "ElemDDLHbaseOptions.h"
#include "CmpContext.h"
#include "parser.h"

class ExpHbaseInterface;
class ExeCliInterface;
class Queue;

class StmtDDLCreateTable;
class StmtDDLDropTable;

class StmtDDLCreateHbaseTable;
class StmtDDLDropHbaseTable;

class StmtDDLCreateIndex;
class StmtDDLDropIndex;
class StmtDDLPopulateIndex;

class StmtDDLAlterTableRename;

class StmtDDLAlterTableAddColumn;
class StmtDDLAlterTableDropColumn;

class StmtDDLAlterTableAlterColumnSetSGOption;

class StmtDDLCreateView;
class StmtDDLDropView;

class StmtDDLCreateLibrary;
class StmtDDLDropLibrary;

class StmtDDLCreateRoutine;
class StmtDDLDropRoutine;

class StmtDDLCreateSequence;
class StmtDDLDropSequence;

class StmtDDLDropSchema;
class StmtDDLAlterSchema;

class StmtDDLRegOrUnregObject;

// Classes for user management
class StmtDDLRegisterUser;
class StmtDDLAlterUser;
class CmpSeabaseDDLauth;
class StmtDDLRegisterComponent;
class PrivMgrComponent;

// classes for constraints
class StmtDDLAddConstraint;
class StmtDDLAddConstraintPK;
class StmtDDLAddConstraintUnique;
class StmtDDLAddConstraintRIArray;
class StmtDDLAddConstraintUniqueArray;
class StmtDDLAddConstraintCheckArray;
class StmtDDLDropConstraint;
class StmtDDLAddConstraintCheck;

class ElemDDLColDefArray;
class ElemDDLColRefArray;
class ElemDDLParamDefArray;
class ElemDDLPartitionClause;

class DDLExpr;
class DDLNode;

class NADefaults;

class NAType;

struct TrafDesc;
class OutputInfo;

class HbaseCreateOption;

class Parser;

class NAColumnArray;

class TrafRoutineDesc;
struct MDDescsInfo;

class CmpDDLwithStatusInfo;

#include "CmpSeabaseDDLmd.h"

// The define below gives the maximum rowID length that we will permit
// for Trafodion tables and indexes. The actual HBase limit is more 
// complicated: For Puts, HBase compares the key length to 
// HConstants.MAX_ROW_LENGTH (= Short.MAX_VALUE = 32767). It raises an
// exception if the key length is greater than that. But there are also
// some internal data structures that HBase uses (the WAL perhaps?) that
// are keyed. Experiments show that a Trafodion key of length n causes
// a hang if n + strlen(Trafodion object name) + 16 > 32767. The HBase
// log in these cases shows an IllegalArgumentException Row > 32767 in
// this case. So it seems best to limit Trafodion key lengths to something
// sufficiently smaller than 32767 so we don't hit these hangs. A value
// of 32000 seems safe, since the longest Trafodion table name will be
// TRAFODION.something.something, with each of the somethings topping out
// at 256 bytes.
#define MAX_HBASE_ROWKEY_LEN 32000

#define SEABASEDDL_INTERNAL_ERROR(text)                                   \
   *CmpCommon::diags() << DgSqlCode(-CAT_INTERNAL_EXCEPTION_ERROR) 	  \
                       << DgString0(__FILE__)   		   	  \
                       << DgInt0(__LINE__)			   	  \
                       << DgString1(text) 			   	  

#define CONCAT_CATSCH(tgt,catname,schname)  \
   (tgt = NAString(catname) + \
         NAString(".\"") + \
         NAString(schname) + \
         NAString("\""))

#define HBASE_OPTION_MAX_INTEGER_LENGTH 5

struct objectRefdByMe
{
  Int64 objectUID;
  NAString objectType;
  NAString objectName;
  NAString schemaName;
  NAString catalogName;
};

class CmpSeabaseDDL
{
 public:
  CmpSeabaseDDL(NAHeap *heap, NABoolean syscatInit = TRUE);

  static NABoolean isSeabase(const NAString &catName);

  static NABoolean isHbase(const NAString &catName);

  static bool isHistogramTable(const NAString &tabName);
  static bool isSampleTable(const NAString &tabName);
  static NABoolean isLOBDependentNameMatch(const NAString &name);
  static NABoolean isSeabaseMD(const NAString &catName,
			       const NAString &schName,
			       const NAString &objName);
 
  static ComBoolean isSeabaseMD(const ComObjectName &name);

  static NABoolean isSeabasePrivMgrMD(const NAString &catName,
			              const NAString &schName);
 
  static ComBoolean isSeabasePrivMgrMD(const ComObjectName &name);

  static NABoolean isUserUpdatableSeabaseMD(const NAString &catName,
			       const NAString &schName,
			       const NAString &objName);

  static NABoolean isSeabaseReservedSchema(
                                           const NAString &catName,
                                           const NAString &schName);
 
  static NABoolean isSeabaseExternalSchema(
                                           const NAString &catName,
                                           const NAString &schName);

  static short getTextFromMD(
       const char *catalogName,
       ExeCliInterface * cliInterface,
       Int64 constrUID,
       ComTextType textType,
       Lng32 textSubID,
       NAString &constrText,
       NABoolean binaryData = FALSE);
  
  static short createHistogramTables(
    ExeCliInterface *cliInterface,
    const NAString &schemaName,
    const NABoolean ignoreIfExists,
    NAString &tableNotCreated);

  static std::vector<std::string> getHistogramTables();

  static short invalidateStats(Int64 tableUID);

  short genHbaseRegionDescs(TrafDesc * desc,
                            const NAString &catName, 
                            const NAString &schName, 
                            const NAString &objName);
  
  NABoolean isAuthorizationEnabled();

  short existsInHbase(const NAString &objName,
		      ExpHbaseInterface * ehi = NULL);

  void processSystemCatalog(NADefaults * defs);

  short isPrivMgrMetadataInitialized(NADefaults *defs = NULL,
                                     NABoolean checkAllPrivTables = FALSE);

  short readAndInitDefaultsFromSeabaseDefaultsTable
    (NADefaults::Provenance overwriteIfNotYet, Int32 errOrWarn,
     NADefaults * defs);

  short getSystemSoftwareVersion(Int64 &softMajVers, 
                                 Int64 &softMinVers,
                                 Int64 &softUpdVers);

  short validateVersions(NADefaults *defs, 
			 ExpHbaseInterface * inEHI = NULL,
			 Int64 * mdMajorVersion = NULL,
			 Int64 * mdMinorVersion = NULL,
                         Int64 * mdUpdateVersion = NULL,
                         Int64 * sysSWMajorVersion = NULL,
                         Int64 * sysSWMinorVersion = NULL,
                         Int64 * sysSWUpdVersion = NULL,
                         Int64 * mdSWMajorVersion = NULL,
                         Int64 * mdSWMinorVersion = NULL,
                         Int64 * mdSWUpdateVersion = NULL,
			 Lng32 * hbaseErr = NULL,
			 NAString * hbaseErrStr = NULL);

  short executeSeabaseDDL(DDLExpr * ddlExpr, ExprNode * ddlNode,
			  NAString &currCatName, NAString &currSchName,
                          CmpDDLwithStatusInfo *dws = NULL);

  TrafDesc * getSeabaseTableDesc(const NAString &catName, 
				    const NAString &schName, 
				    const NAString &objName,
				    const ComObjectType objType,
				    NABoolean includeInvalidDefs = FALSE);

  short getSeabaseObjectComment(Int64 object_uid, 
	                            enum ComObjectType object_type, 
	                            ComTdbVirtObjCommentInfo & comment_info,
	                            CollHeap * heap);

  short getObjectOwner(ExeCliInterface *cliInterface,
                        const char * catName,
                        const char * schName,
                        const char * objName,
                        const char * objType,
                        Int32 * objectOwner);

  static bool describeSchema(
     const NAString & catalogName,
     const NAString & schemaName,
     NABoolean isHiveRegistered,
     std::vector<std::string> & outlines);
     
  static Int64 getObjectTypeandOwner(ExeCliInterface *cliInterface,
                              const char * catName,
                              const char * schName,
                              const char * objName,
                              ComObjectType & objectType,
                              Int32 & objectOwner);
                              
  short getSaltText(ExeCliInterface *cliInterface,
                    const char * catName,
                    const char * schName,
                    const char * objName,
                    const char * inObjType,
                    NAString& saltText) ;

  static short genHbaseCreateOptions(
				     const char * hbaseCreateOptionsStr,
				     NAList<HbaseCreateOption*>* &hbaseCreateOptions,
				     NAMemory * heap,
                                     size_t * beginPos,
                                     size_t * endPos);

  static short genHbaseOptionsMetadataString(                                          
                                      const NAList<HbaseCreateOption*> & hbaseCreateOptions,
                                      NAString & hbaseOptionsMetadataString /* out */);

  short updateHbaseOptionsInMetadata(ExeCliInterface * cliInterface,
                                     Int64 objectUID,
                                     ElemDDLHbaseOptions * edhbo);

  TrafDesc * getSeabaseLibraryDesc(
     const NAString &catName, 
     const NAString &schName, 
     const NAString &libraryName);
     
  TrafDesc *getSeabaseRoutineDesc(const NAString &catName,
                                     const NAString &schName,
                                     const NAString &objName);
  
  static NABoolean getOldMDInfo(const MDTableInfo  &mdti,
                                const char* &oldName,
                                const QString* &oldDDL, Lng32 &sizeOfOldDDL);
  
  short createMDdescs(MDDescsInfo *&);

  static NAString getSystemCatalogStatic();

  static NABoolean isEncodingNeededForSerialization(NAColumn * nac);
  
  int32_t verifyDDLCreateOperationAuthorized(
     ExeCliInterface * cliInterface,
     SQLOperation operation,
     const NAString & catalogName,
     const NAString & schemaName,
     ComSchemaClass & schemaClass,
     Int32 & objectOwner,
     Int32 & schemaOwner);
     
  bool isDDLOperationAuthorized(
     SQLOperation operation,
     const Int32 objOwnerId,
     const Int32 schemaOwnerID);
  
  static NABoolean enabledForSerialization(NAColumn * nac);

  static NABoolean isSerialized(ULng32 flags)
  {
    return (flags & NAColumn::SEABASE_SERIALIZED) != 0;
  }

  short buildColInfoArray(
                          ComObjectType objType,
                          NABoolean isMetadataHistOrReposObject,
			  ElemDDLColDefArray * colArray,
			  ComTdbVirtTableColumnInfo * colInfoArray,
			  NABoolean implicitPK,
                          NABoolean alignedFormat,
                          Lng32 *identityColPos = NULL,
                          std::vector<NAString> *userColFamVec = NULL,
                          std::vector<NAString> *trafColFamVec = NULL,
                          const char * defaultColFam = NULL,
			  NAMemory * heap = NULL);

  // The next three methods do use anything from the CmpSeabaseDDL class.
  // They are placed here as a packaging convinience, to avoid code 
  // duplication that would occur if non-member static functions were used.
  // These methods convert VirtTable*Info classes to corresponding TrafDesc
  // objects
  void convertVirtTableColumnInfoToDescStruct( 
       const ComTdbVirtTableColumnInfo * colInfo,
       const ComObjectName * objectName,
       TrafDesc * column_desc);

  TrafDesc * convertVirtTableColumnInfoArrayToDescStructs(
     const ComObjectName * objectName,
     const ComTdbVirtTableColumnInfo * colInfoArray,
     Lng32 numCols);

  TrafDesc * convertVirtTableKeyInfoArrayToDescStructs(
       const ComTdbVirtTableKeyInfo *keyInfoArray,
       const ComTdbVirtTableColumnInfo *colInfoArray,
       Lng32 numKeys);
  
  Int64 getObjectUID(
       ExeCliInterface *cliInterface,
       const char * catName,
       const char * schName,
       const char * objName,
       const char * inObjType,
       const char * inObjTypeStr = NULL,
       char * outObjType = NULL,
       NABoolean lookInObjectsIdx = FALSE,
       NABoolean reportErrorNow = TRUE);
  
  Int64 getObjectInfo(
       ExeCliInterface * cliInterface,
       const char * catName,
       const char * schName,
       const char * objName,
       const ComObjectType objectType,
       Int32 & objectOwner,
       Int32 & schemaOwner,
       Int64 & objectFlags,
       bool reportErrorNow = true,
       NABoolean checkForValidDef = FALSE,
       Int64 * createTime = NULL);
  
  short getObjectName(
       ExeCliInterface *cliInterface,
       Int64 objUID,
       NAString &catName,
       NAString &schName,
       NAString &objName,
       char * outObjType = NULL,
       NABoolean lookInObjects = FALSE,
       NABoolean lookInObjectsIdx = FALSE);
  
  short getObjectValidDef(ExeCliInterface *cliInterface,
                          const char * catName,
                          const char * schName,
                          const char * objName,
                          const ComObjectType objectType,
                          NABoolean &validDef);
  
  short genTrafColFam(int index, NAString &trafColFam);
  
  static short extractTrafColFam(const NAString &trafColFam, int &index);
  
  short processColFamily(NAString &inColFamily,
                         NAString &outColFamily,
                         std::vector<NAString> *userColFamVec,
                         std::vector<NAString> *trafColFamVec);
  
  short switchCompiler(Int32 cntxtType = CmpContextInfo::CMPCONTEXT_TYPE_META);
  
  short switchBackCompiler();
  
  ExpHbaseInterface* allocEHI(NADefaults * defs = NULL);
  
  short ddlInvalidateNATables();
  
  void deallocEHI(ExpHbaseInterface* &ehi);
  void dropLOBHdfsFiles();
  
  static void setMDflags(Int64 &flags, //INOUT
                         Int64 bitFlags)
  {
    flags |= bitFlags;
  }
  
  static void resetMDflags(Int64 &flags, //INOUT
                           Int64 bitFlags)
  {
    flags &= ~bitFlags;
  }
  
  static NABoolean isMDflagsSet(Int64 flags, Int64 bitFlags)
  {
    return (flags &= bitFlags) != 0; 
  }
  
  enum {
    // set if we need to get the hbase snapshot info of the table
    GET_SNAPSHOTS     = 0x0002,

    // set if descr is to be generated in packed format to be stored in metadata
    GEN_PACKED_DESC   = 0x0004,

    // set if stored object descriptor is to be read from metadata.
    READ_OBJECT_DESC  = 0x0008
  };

  enum 
    {
      MD_TABLE_CONSTRAINTS_PKEY_NOT_SERIALIZED_FLG  = 0x0001
    };
protected:
  
  void setFlags(ULng32 &flags, ULng32 flagbits)
  {
    flags |= flagbits;
  }

  void resetFlags(ULng32 &flags, ULng32 flagbits)
  {
    flags &= ~flagbits;
  }
  
  inline const char * getMDSchema() {return seabaseMDSchema_.data();};

  const char * getSystemCatalog();

  ComBoolean isSeabaseReservedSchema(const ComObjectName &name);

  ComBoolean isSeabase(const ComObjectName &name) ;

  ComBoolean isHbase(const ComObjectName &name) ;

  short isMetadataInitialized(ExpHbaseInterface * ehi = NULL);
  short isOldMetadataInitialized(ExpHbaseInterface * ehi);

  ExpHbaseInterface* allocEHI(const char * server, const char * zkPort,
                              NABoolean raiseError);
  
  // if prevContext is defined, get user CQDs from the controlDB of
  // previous context and send them to the new cmp context
  short sendAllControlsAndFlags(CmpContext* prevContext=NULL,
				Int32 cntxtType=-1);

  void restoreAllControlsAndFlags();
  
  void processReturn(Lng32 retcode = 0);

  // construct and return the column name value as stored with hbase rows.
  // colNum is 0-based (first col is 0)
  void getColName(const ComTdbVirtTableColumnInfo columnInfo[],
		  Lng32 colNum, NAString &colName);
  
  void getColName(const char * colFam, const char * colQual,
		  NAString &colName);

  TrafDesc *getSeabaseRoutineDescInternal(const NAString &catName,
                                             const NAString &schName,
                                             const NAString &objName);

  // note: this function expects hbaseCreateOptionsArray to have
  // HBASE_MAX_OPTIONS elements
  short generateHbaseOptionsArray(NAText * hbaseCreateOptionsArray,
    NAList<HbaseCreateOption*> * hbaseCreateOptions);

  short createHbaseTable(ExpHbaseInterface *ehi, 
			 HbaseStr *table,
			 const char * cf1, 
                         NAList<HbaseCreateOption*> * hbaseCreateOptions = NULL,
                         const int numSplits = 0,
                         const int keyLength = 0,
                         char **encodedKeysBuffer = NULL,
			 NABoolean doRetry = FALSE,
                         NABoolean ddlXns = FALSE);

  short createHbaseTable(ExpHbaseInterface *ehi, 
			 HbaseStr *table,
                         std::vector<NAString> &collFamVec,
                         NAList<HbaseCreateOption*> * hbaseCreateOptions = NULL,
                         const int numSplits = 0,
                         const int keyLength = 0,
                         char **encodedKeysBuffer = NULL,
			 NABoolean doRetry = TRUE,
                         NABoolean ddlXns = FALSE);

  short alterHbaseTable(ExpHbaseInterface *ehi,
                        HbaseStr *table,
                        NAList<NAString> &allColFams,
                        NAList<HbaseCreateOption*> * hbaseCreateOptions,
                        NABoolean ddlXns);

  short dropHbaseTable(ExpHbaseInterface *ehi, 
		       HbaseStr *table, NABoolean asyncDrop,
                       NABoolean ddlXns);

  short copyHbaseTable(ExpHbaseInterface *ehi, 
		       HbaseStr *currTable, HbaseStr* oldTable);

  NABoolean xnInProgress(ExeCliInterface *cliInterface);
  short beginXn(ExeCliInterface *cliInterface);
  short commitXn(ExeCliInterface *cliInterface);
  short rollbackXn(ExeCliInterface *cliInterface);
  short autoCommit(ExeCliInterface *cliInterface, NABoolean v);
  short beginXnIfNotInProgress(ExeCliInterface *cliInterface, 
                               NABoolean &xnWasStartedHere);
  short endXnIfStartedHere(ExeCliInterface *cliInterface, 
                           NABoolean &xnWasStartedHere, Int32 cliRC);

  short dropSeabaseObject(ExpHbaseInterface *ehi,
			  const NAString &objName,
			  NAString &currCatName, NAString &currSchName,
			  const ComObjectType objType,
                          NABoolean ddlXns,
			  NABoolean dropFromMD = TRUE,
			  NABoolean dropFromHbase = TRUE);
  
  short dropSeabaseStats(ExeCliInterface *cliInterface,
                         const char * catName,
                         const char * schName,
                         Int64 tableUID);

  short checkDefaultValue(
			  const NAString & colExtName,
			  const NAType    * inColType,
			  ElemDDLColDef   * colNode);
  
  short getTypeInfo(const NAType * naType,
                    NABoolean alignedFormat,
		    Lng32 serializedOption,
		    Lng32 &datatype,
		    Lng32 &length,
		    Lng32 &precision,
		    Lng32 &scale,
		    Lng32 &dtStart,
		    Lng32 &dtEnd,
		    Lng32 &upshifted,
		    Lng32 &nullable,
		    NAString &charset,
		    CharInfo::Collation &collationSequence,
		    ULng32 &colFlags);

  short getColInfo(ElemDDLColDef * colNode, 
                   NABoolean isMetadataHistOrReposColumn,
                   NAString &colFamily,
		   NAString &colName,
                   NABoolean alignedFormat,
		   Lng32 &datatype,
		   Lng32 &length,
		   Lng32 &precision,
		   Lng32 &scale,
		   Lng32 &dtStart,
		   Lng32 &dtEnd,
		   Lng32 &upshifted,
		   Lng32 &nullable,
		   NAString &charset,
                   ComColumnClass &colClass,
		   ComColumnDefaultClass &defaultClass,
		   NAString &defVal,
		   NAString &heading,
		   LobsStorage &lobStorage,
		   ULng32 &hbaseColFlags,
                   Int64 &colFlags);

  short getNAColumnFromColDef(ElemDDLColDef * colNode,
                              NAColumn* &naCol);
  
  short createRowId(NAString &key,
		    NAString &part1, Lng32 part1MaxLen,
		    NAString &part2, Lng32 part2MaxLen,
		    NAString &part3, Lng32 part3MaxLen,
		    NAString &part4, Lng32 part4MaxLen);
  
  short existsInSeabaseMDTable(
			       ExeCliInterface *cliInterface,
			       const char * catName,
			       const char * schName,
			       const char * objName,
			       const ComObjectType objectType = COM_UNKNOWN_OBJECT,
			       NABoolean checkForValidDef = TRUE,
			       NABoolean checkForValidHbaseName = TRUE,
                               NABoolean returnInvalidStateError = FALSE);
  
  Int64 getConstraintOnIndex(
			     ExeCliInterface *cliInterface,
			     Int64 btUID,
			     Int64 indexUID,
			     const char * constrType,
			     NAString &catName,
			     NAString &schName,
			     NAString &objName);

  short getBaseTable(ExeCliInterface *cliInterface,
		     const NAString &indexCatName,
		     const NAString &indexSchName,
		     const NAString &indexObjName,
		     NAString &btCatName,
		     NAString &btSchName,
		     NAString &btObjName,
		     Int64 &btUID,
                     Int32 &btObjOwner,
                     Int32 &btSchemaOwner);
  
  short getUsingObject(ExeCliInterface *cliInterface,
		       Int64 objUID,
		       NAString &usingObjName);

  short getUsingRoutines(ExeCliInterface *cliInterface,
                         Int64 objUID,
                         Queue * & usingRoutinesQueue);
  
  short getUsingViews(ExeCliInterface *cliInterface,
                      Int64 objectUID,
                      Queue * &usingViewsQueue);
  
  short getAllUsingViews(ExeCliInterface *cliInterface,
                         NAString &catName,
                         NAString &schName,
                         NAString &objName,
                         Queue * &usingViewsQueue);

  void handleDDLCreateAuthorizationError(
     int32_t SQLErrorCode,
     const NAString & catalogName, 
     const NAString & schemaName);
     
  short updateSeabaseMDObjectsTable(
                                    ExeCliInterface * cliInterface,
                                    const char * catName,
                                    const char * schName,
                                    const char * objName,
                                    const ComObjectType & objectType,
                                    const char * validDef, 
                                    Int32 objOwnerID,
                                    Int32 schemaOwnerID,
                                    Int64 objectFlags,
                                    Int64 & inUID);
                                
  short deleteFromSeabaseMDObjectsTable(
       ExeCliInterface *cliInterface,
       const char * catName,
       const char * schName,
       const char * objName,
       const ComObjectType & objectType);
  
  short getAllIndexes(ExeCliInterface *cliInterface,
                      Int64 objUID,
                      NABoolean includeInvalidDefs,
                      Queue * &indexInfoQueue);

  short updateSeabaseMDTable(
			     ExeCliInterface *cliInterface,
			     const char * catName,
			     const char * schName,
			     const char * objName,
                             const ComObjectType & objectType,
			     const char * validDef,
			     ComTdbVirtTableTableInfo * tableInfo,
			     Lng32 numCols,
			     const ComTdbVirtTableColumnInfo * colInfo,
			     Lng32 numKeys,
			     const ComTdbVirtTableKeyInfo * keyInfo,
			     Lng32 numIndexes,
			     const ComTdbVirtTableIndexInfo * indexInfo,
                             Int64 &inUID,
                             NABoolean updPrivs = TRUE);

  short deleteFromSeabaseMDTable(
				 ExeCliInterface *cliInterface,
				 const char * catName,
				 const char * schName,
				 const char * objName,
				 const ComObjectType objType);

  short updateSeabaseMDSPJ(
                            ExeCliInterface *cliInterface,
                            const char * catName,
                            const char * schName,
                            const char * libname,
                            const char * libPath,
                            const Int32 ownerID,
                            const Int32 schemaOwnerID,
                            const ComTdbVirtTableRoutineInfo * routineInfo,
                            Lng32 numCols,
                            const ComTdbVirtTableColumnInfo * colInfo);

  short deleteConstraintInfoFromSeabaseMDTables(
						ExeCliInterface *cliInterface,
						Int64 tableUID,
						Int64 otherTableUID, // valid for ref constrs
						Int64 constrUID,
						Int64 otherConstrUID, // valid for ref constrs
						const char * constrCatName,
						const char * constrSchName,
						const char * constrObjName,
						const ComObjectType constrType);

  short updateObjectName(
			 ExeCliInterface *cliInterface,
			 Int64 objUID,
			 const char * catName,
			 const char * schName,
			 const char * objName);

  // retrieved stored desc from metadata, check if it is good,
  // and set retDesc, if passed in.
  short checkAndGetStoredObjectDesc(
       ExeCliInterface *cliInterface,
       Int64 objUID,
       TrafDesc* *retDesc);

  short updateObjectRedefTime(
                              ExeCliInterface *cliInterface,
                              const NAString &catName,
                              const NAString &schName,
                              const NAString &objName,
                              const char * objType,
                              Int64 rt = -1,
                              Int64 objUID = -1,
                              NABoolean force = FALSE);

  short updateObjectValidDef(
			     ExeCliInterface *cliInterface,
			     const char * catName,
			     const char * schName,
			     const char * objName,
			     const char * objType,
			     const char * validDef);
  
  short updateObjectAuditAttr(
			    ExeCliInterface *cliInterface,
			    const char * catName,
			    const char * schName,
			    const char * objName,
			    NABoolean audited,
                            const NAString& objType);

  short updateObjectFlags(
       ExeCliInterface *cliInterface,
       const Int64 objUID,
       const Int64 inFlags,
       NABoolean reset);

  // subID: 0, for text that belongs to table. colNumber, for column based text.
  short updateTextTable(ExeCliInterface *cliInterface,
                        Int64 objUID, 
                        ComTextType textType, 
                        Lng32 subID, 
                        NAString &textInputData,
                        char * binaryInputData = NULL,
                        Lng32 binaryInputDataLen = -1,
                        NABoolean withDelete = FALSE); // del before ins

  // input data in non-char format.
  short updateTextTableWithBinaryData(ExeCliInterface *cliInterface,
                                      Int64 objUID, 
                                      ComTextType textType, 
                                      Lng32 subID, 
                                      char * data,
                                      Int32 dataLen,
                                      NABoolean withDelete);
  
  short deleteFromTextTable(ExeCliInterface *cliInterface,
                            Int64 objUID, 
                            ComTextType textType, 
                            Lng32 subID);

  ItemExpr * bindDivisionExprAtDDLTime(ItemExpr *expr,
                                       NAColumnArray *availableCols,
                                       NAHeap *heap);
  short validateDivisionByExprForDDL(ItemExpr *divExpr);

  short createEncodedKeysBuffer(char** &encodedKeysBuffer,
                                int &numSplits,
				TrafDesc * colDescs, TrafDesc * keyDescs,
				int numSaltPartitions,
                                Lng32 numSaltSplits,
                                NAString *splitByClause,
                                Lng32 numKeys,
                                Lng32 keyLength, NABoolean isIndex);

  short validateRoutine( 
                        ExeCliInterface *cliInterface,
                        const char * className,
                        const char * methodName,
                        const char * externalPath,
                        char * signature,
                        Int32 numSqlParam,
                        Int32 maxResultSets,
                        const char * optionalSig) ;

  short populateKeyInfo(ComTdbVirtTableKeyInfo &keyInfo,
			OutputInfo * oi, NABoolean isIndex = FALSE);
  
  short dropMDTable(ExpHbaseInterface *ehi, const char * tab);
  
  short populateSeabaseIndexFromTable(
				      ExeCliInterface * cliInterface,
				      NABoolean uniqueIndex,
				      const NAString &indexName, 
                                      const ComObjectName &tableName,
				      NAList<NAString> &selColList,
				      NABoolean useLoad );
  
  short buildViewText(StmtDDLCreateView * createViewParseNode,
		      NAString &viewText);
  
  short buildViewColInfo(StmtDDLCreateView * createViewParseNode,
			 ElemDDLColDefArray * colDefArray);
  
  short buildViewTblColUsage(const StmtDDLCreateView * createViewParseNode,
                             const ComTdbVirtTableColumnInfo * colInfoArray,
                             const Int64 viewObjUID, NAString &viewColUsageText);

  short buildColInfoArray(ElemDDLParamDefArray *paramArray,
                          ComTdbVirtTableColumnInfo * colInfoArray);
  
  short buildKeyInfoArray(
			  ElemDDLColDefArray *colArray,
                          NAColumnArray *nacolArray,
			  ElemDDLColRefArray *keyArray,
			  ComTdbVirtTableColumnInfo * colInfoArray,
			  ComTdbVirtTableKeyInfo * keyInfoArray,
			  NABoolean allowNullableUniqueConstr,
                          Lng32 *keyLength = NULL,
			  NAMemory * heap = NULL);

  const char * computeCheckOption(StmtDDLCreateView * createViewParseNode);
  
  short updateViewUsage(StmtDDLCreateView * createViewParseNode,
			Int64 viewUID,
			ExeCliInterface * cliInterface);

  short unregisterHiveViewUsage(StmtDDLCreateView * createViewParseNode,
                                Int64 viewUID,
                                ExeCliInterface * cliInterface);
  
  short gatherViewPrivileges (const StmtDDLCreateView * createViewParseNode,
                              ExeCliInterface * cliInterface,
                              NABoolean viewCreator,
                              Int32 userID,
                              PrivMgrBitmap &privilegesBitmap,
                              PrivMgrBitmap &grantableBitmap);

  short getListOfReferencedTables (ExeCliInterface * cliInterface,
                                   const Int64 objectUID,
                                   NAList<objectRefdByMe> &tablesList);

  short getListOfDirectlyReferencedObjects (ExeCliInterface *cliInterface,
                                            const Int64 objectUID,
                                            NAList<objectRefdByMe> &objectsList);

  short genPKeyName(StmtDDLAddConstraintPK *addPKNode,
		    const char * catName,
		    const char * schName,
		    const char * objName,
		    NAString &pkeyName);

  short constraintErrorChecks(
                              ExeCliInterface * cliInterface,
			      StmtDDLAddConstraint *addConstrNode,
			      NATable * naTable,
			      ComConstraintType ct,
			      NAList<NAString> &keyColList);

  short updateConstraintMD(
			   NAList<NAString> &keyColList,
			   NAList<NAString> &keyColOrderList,
			   NAString &uniqueStr,
			   Int64 tableUID,
			   Int64 uniqueUID,
			   NATable * naTable,
			   ComConstraintType ct,
                           NABoolean enforced,
			   ExeCliInterface *cliInterface);

  short updateRIConstraintMD(
			     Int64 ringConstrUID,
			     Int64 refdConstrUID,
			     ExeCliInterface *cliInterface);

  short updatePKeyInfo(
		       StmtDDLAddConstraintPK *addPKNode,
		       const char * catName,
		       const char * schName,
		       const char * objName,
                       const Int32 ownerID,
                       const Int32 schemaOwnerID,
		       Lng32 numKeys,
		       Int64 * outPkeyUID,
		       Int64 *outTableUID,
		       const ComTdbVirtTableKeyInfo * keyInfoArray,
		       ExeCliInterface *cliInterface);

  short getPKeyInfoForTable (
                            const char *catName,
                            const char *schName,
                            const char *objName,
                            ExeCliInterface *cliInterface,
                            NAString &constrName,
                            Int64 &constrUID);

  short updateRIInfo(
		       StmtDDLAddConstraintRIArray &riArray,
		       const char * catName,
		       const char * schName,
		       const char * objName,
		       ExeCliInterface *cliInterface);

  short genUniqueName(StmtDDLAddConstraint *addUniqueNode,
		    NAString &pkeyName);

  short updateIndexInfo(
			NAList<NAString> &ringKeyColList,
			NAList<NAString> &ringKeyColOrderList,
			NAList<NAString> &refdKeyColList,
			NAString &uniqueStr,
			Int64 constrUID,
			const char * catName,
			const char * schName,
			const char * objName,
			NATable * naTable,
			NABoolean isUnique, // TRUE: uniq constr. FALSE: ref constr.
			NABoolean noPopulate, // TRUE, dont populate index
                        NABoolean isEnforced, // TRUE: contraint is enforced
                        NABoolean sameSequenceOfCols, // FALSE, allow "similar" indexes
			ExeCliInterface *cliInterface);

  short createMetadataViews(ExeCliInterface * cliInterface);
  short dropMetadataViews(ExeCliInterface * cliInterface);
 
  int addSchemaObject(
     ExeCliInterface & cliInterface,
     const ComSchemaName & schemaName,
     ComSchemaClass schemaClass,
     Int32 ownerID,
     NABoolean ignoreIfExists);
     
  short createDefaultSystemSchema(ExeCliInterface * cliInterface);
  short createSchemaObjects(ExeCliInterface * cliInterface);
  
  void  createSeabaseSchema(
     StmtDDLCreateSchema  * createSchemaNode,
     NAString             & currCatName);


  void cleanupObjectAfterError(
                               ExeCliInterface &cliInterface,
                               const NAString &catName, 
                               const NAString &schName,
                               const NAString &objName,
                               const ComObjectType objectType,
                               NABoolean ddlXns);

  NABoolean appendErrorObjName(char * errorObjs, 
                               const char * objName);

  short setupAndErrorChecks(NAString &tabName, QualifiedName &origTableName, 
                            NAString &currCatName, NAString &currSchName,
                            NAString &catalogNamePart, 
                            NAString &schemaNamePart, 
                            NAString &objectNamePart,
                            NAString &extTableName, NAString &extNameForHbase,
                            CorrName &cn,
                            NATable* *naTable,
                            NABoolean volTabSupported, 
                            NABoolean hbaseMapSupported,
                            ExeCliInterface *cliInterface,
                            const ComObjectType objectType = COM_BASE_TABLE_OBJECT,
                            SQLOperation operation = SQLOperation::ALTER_TABLE,
                            NABoolean isExternal = FALSE);
  
  void purgedataObjectAfterError(
                               ExeCliInterface &cliInterface,
                               const NAString &catName, 
                               const NAString &schName,
                               const NAString &objName,
                               const ComObjectType objectType,
                               NABoolean dontForceCleanup);

  short createSeabaseTable2(
                            ExeCliInterface &cliInterface,
                            StmtDDLCreateTable * createTableNode,
                            NAString &currCatName, NAString &currSchName,
                            NABoolean isCompound,
                            Int64 &objUID);
  
  void createSeabaseTable(
			  StmtDDLCreateTable * createTableNode,
			  NAString &currCatName, NAString &currSchName,
                          NABoolean isCompound = FALSE,
                          Int64 * retObjUID = NULL);
 
  void createSeabaseTableCompound(
			  StmtDDLCreateTable                  * createTableNode,
			  NAString &currCatName, NAString &currSchName);
 
  short createSeabaseTableLike2(
       CorrName &cn,
       const NAString &likeTabName,
       NABoolean withPartns = FALSE,
       NABoolean withoutSalt = FALSE,
       NABoolean withoutDivision = FALSE,
       NABoolean withoutRowFormat = FALSE);

  void createSeabaseTableLike(
       ExeCliInterface *cliInterface,
       StmtDDLCreateTable                  * createTableNode,
       NAString &currCatName, NAString &currSchName);

  short createSeabaseTableExternal(
                                   ExeCliInterface &cliInterface,
                                   StmtDDLCreateTable * createTableNode,
                                   const ComObjectName &tgtTableName,
                                   const ComObjectName &srcTableName);

public:
  static NABoolean setupQueryTreeForHiveDDL(
       Parser::HiveDDLInfo * hiveDDLInfo,
       char * inputStr, 
       CharInfo::CharSet inputStrCharSet,
       NAString currCatName,
       NAString currSchName,
       ExprNode** node);

protected:

  // makes a copy of underlying hbase table
  short cloneHbaseTable(
       const NAString &srcTable, const NAString &clonedTable,
       ExpHbaseInterface * inEHI);

  // makes a copy of traf metadata and underlying hbase table
  short cloneSeabaseTable(
       const NAString &srcTableNameStr,
       Int64 srcObjUID,
       const NAString &clonedTableNameStr,
       const NATable * naTable,
       ExpHbaseInterface * ehi,
       ExeCliInterface * cilInterface,
       NABoolean withCreate);

  short cloneAndTruncateTable(
       const NATable * naTable, // IN: source table
     NAString &tempTable, // OUT: temp table
     ExpHbaseInterface * ehi,
     ExeCliInterface * cliInterface);

  short dropSeabaseTable2(
                          ExeCliInterface *cliInterface,
                          StmtDDLDropTable * dropTableNode,
                          NAString &currCatName, NAString &currSchName);
  
  void dropSeabaseTable(
			StmtDDLDropTable                  * dropTableNode,
			NAString &currCatName, NAString &currSchName);
  
  void createSeabaseIndex(
			  StmtDDLCreateIndex                  * createIndexNode,
			  NAString &currCatName, NAString &currSchName);
  
  void populateSeabaseIndex(
			    StmtDDLPopulateIndex                  * populateIndexNode,
			    NAString &currCatName, NAString &currSchName);
  
  void dropSeabaseIndex(
			StmtDDLDropIndex                  * dropIndexNode,
			NAString &currCatName, NAString &currSchName);

  void renameSeabaseTable(
			  StmtDDLAlterTableRename                  * renameTableNode,
			  NAString &currCatName, NAString &currSchName);

  void alterSeabaseTableStoredDesc(
       StmtDDLAlterTableStoredDesc * alterStoredDesc,
       NAString &currCatName, NAString &currSchName);

  void alterSeabaseTableHBaseOptions(
			  StmtDDLAlterTableHBaseOptions * hbaseOptionsNode,
			  NAString &currCatName, NAString &currSchName);
  void alterSeabaseIndexHBaseOptions(
			  StmtDDLAlterIndexHBaseOptions * hbaseOptionsNode,
			  NAString &currCatName, NAString &currSchName);

  void addConstraints(
       ComObjectName &tableName,
       ComAnsiNamePart &currCatAnsiName,
       ComAnsiNamePart &currSchAnsiName,
       StmtDDLNode * ddlNode,
       StmtDDLAddConstraintPK * pkConstr,
       StmtDDLAddConstraintUniqueArray &uniqueConstrArr,
       StmtDDLAddConstraintRIArray &riConstrArr,
       StmtDDLAddConstraintCheckArray &checkConstrArr,
       NABoolean isCompound = FALSE);
  
  void alterSeabaseTableAddColumn(
       StmtDDLAlterTableAddColumn * alterAddColNode,
       NAString &currCatName, NAString &currSchName);
  

  short updateMDforDropCol(ExeCliInterface &cliInterface,
                           const NATable * naTable,
                           Lng32 dropColNum);

  short alignedFormatTableDropColumn(
       const NAString &catalogNamePart,
       const NAString &schemaNamePart,
       const NAString &objectNamePart,
       const NATable * naTable,
       const NAString &altColName,
       ElemDDLColDef *pColDef,
       NABoolean ddlXns,
       NAList<NAString> &viewNameList,
       NAList<NAString> &viewDefnList);
  
 short hbaseFormatTableDropColumn(
      ExpHbaseInterface *ehi,
      const NAString &catalogNamePart,
      const NAString &schemaNamePart,
      const NAString &objectNamePart,
      const NATable * naTable,
      const NAString &altColName,
      const NAColumn * nacol,
      NABoolean ddlXns,
      NAList<NAString> &viewNameList,
      NAList<NAString> &viewDefnList);
  
  void alterSeabaseTableDropColumn(
       StmtDDLAlterTableDropColumn * alterDropColNode,
       NAString &currCatName, NAString &currSchName);
  
  short saveAndDropUsingViews(Int64 objUID,
                              ExeCliInterface *cliInterface,
                              NAList<NAString> &viewNameList,
                              NAList<NAString> &viewDefnList);
  
  short recreateUsingViews(ExeCliInterface *cliInterface,
                           NAList<NAString> &viewNameList,
                           NAList<NAString> &viewDefnList,
                           NABoolean ddlXns);

  void alterSeabaseTableAlterIdentityColumn(
       StmtDDLAlterTableAlterColumnSetSGOption * alterIdentityColNode,
       NAString &currCatName, NAString &currSchName);
  
  short mdOnlyAlterColumnAttr(
       const NAString &catalogNamePart, const NAString &schemaNamePart,
       const NAString &objectNamePart,
       const NATable * naTable, const NAColumn * naCol, NAType * newType,
       StmtDDLAlterTableAlterColumnDatatype * alterColNode,
       NAList<NAString> &viewNameList,
       NAList<NAString> &viewDefnList);
  
  short hbaseFormatTableAlterColumnAttr(
       const NAString &catalogNamePart, const NAString &schemaNamePart,
       const NAString &objectNamePart,
       const NATable * naTable, const NAColumn * naCol, NAType * newType,
       StmtDDLAlterTableAlterColumnDatatype * alterColNode);
  
  short alignedFormatTableAlterColumnAttr
  (
       const NAString &catalogNamePart,
       const NAString &schemaNamePart,
       const NAString &objectNamePart,
       const NATable * naTable,
       const NAString &altColName,
       ElemDDLColDef *pColDef,
       NABoolean ddlXns,
       NAList<NAString> &viewNameList,
       NAList<NAString> &viewDefnList);
  
  void alterSeabaseTableAlterColumnDatatype(
       StmtDDLAlterTableAlterColumnDatatype * alterColumnDatatype,
       NAString &currCatName, NAString &currSchName);
  
  void alterSeabaseTableAlterColumnRename(
       StmtDDLAlterTableAlterColumnRename * alterColumnDatatype,
       NAString &currCatName, NAString &currSchName);
  
  void alterSeabaseTableAddPKeyConstraint(
       StmtDDLAddConstraint * alterAddConstraint,
       NAString &currCatName, NAString &currSchName);
  
  void alterSeabaseTableAddUniqueConstraint(
       StmtDDLAddConstraint * alterAddConstraint,
       NAString &currCatName, NAString &currSchName);
  
  short isCircularDependent(
       CorrName &ringTable,
       CorrName &refdTable,
       CorrName &origRingTable,
       BindWA *bindWA);
  
  void alterSeabaseTableAddRIConstraint(
       StmtDDLAddConstraint * alterAddConstraint,
       NAString &currCatName, NAString &currSchName);
  
  short getCheckConstraintText(StmtDDLAddConstraintCheck *addCheckNode,
			       NAString &checkConstrText);
  
  short getTextFromMD(
       ExeCliInterface * cliInterface,
       Int64 constrUID,
       ComTextType textType,
       Lng32 textSubID,
       NAString &constrText,
       NABoolean binaryData = FALSE);
  
  void alterSeabaseTableAddCheckConstraint(
					StmtDDLAddConstraint * alterAddConstraint,
					NAString &currCatName, NAString &currSchName);

  void alterSeabaseTableDropConstraint(
				  StmtDDLDropConstraint * alterDropConstraint,
				  NAString &currCatName, NAString &currSchName);

  void alterSeabaseTableDisableOrEnableIndex(
					     ExprNode * ddlNode,
					     NAString &currCatName, NAString &currSchName);

  void alterSeabaseTableDisableOrEnableAllIndexes(
                                               ExprNode * ddlNode,
                                               NAString &currCatName,
                                               NAString &currSchName,
                                               NAString &tableName,
                                               NABoolean allUniquesOnly);
  short alterSeabaseTableDisableOrEnableIndex(
                                              const char * catName,
                                              const char * schName,
                                              const char * idxName,
                                              const char * tabName,
                                               NABoolean isDisable);

  void createSeabaseView(
			 StmtDDLCreateView                  * createViewNode,
			 NAString &currCatName, NAString &currSchName);
  
  void dropSeabaseView(
		       StmtDDLDropView                  * dropViewNode,
		       NAString &currCatName, NAString &currSchName);

  void createSeabaseLibrary(StmtDDLCreateLibrary  * createLibraryNode,
                            NAString &currCatName, NAString &currSchName);
  
  void registerSeabaseUser (
                            StmtDDLRegisterUser        * registerUserNode);
  void alterSeabaseUser (
                            StmtDDLAlterUser        * alterUserNode);
  void unregisterSeabaseUser (
                            StmtDDLRegisterUser        * registerUserNode);
  void registerSeabaseComponent (
                            StmtDDLRegisterComponent   * registerComponentNode);

  void dropSeabaseLibrary(StmtDDLDropLibrary  * dropLibraryNode,
                          NAString &currCatName, NAString &currSchName);

  void  alterSeabaseLibrary(StmtDDLAlterLibrary  *alterLibraryNode,
			    NAString &currCatName, NAString &currSchName);

  void createSeabaseRoutine(StmtDDLCreateRoutine  * createRoutineNode,
                            NAString &currCatName, NAString &currSchName);
  
  void dropSeabaseRoutine(StmtDDLDropRoutine  * dropRoutineNode,
                          NAString &currCatName, NAString &currSchName);

  short createSeabaseLibmgr(ExeCliInterface * cliInterface);
  short upgradeSeabaseLibmgr(ExeCliInterface * inCliInterface);
  short dropSeabaseLibmgr(ExeCliInterface *inCliInterface);
  short createLibmgrProcs(ExeCliInterface * cliInterface);
  short grantLibmgrPrivs(ExeCliInterface *cliInterface);
  short createSeabaseLibmgrCPPLib(ExeCliInterface * cliInterface);

  short registerNativeTable
  (
       const NAString &catalogNamePart,
       const NAString &schemaNamePart,
       const NAString &objectNamePart,
       Int32 objOwnerId,
       Int32 schemaOwnerId,
       ExeCliInterface &cliInterface,
       NABoolean isRegister,
       NABoolean isInternal
   );

  short unregisterNativeTable
  (
       const NAString &catalogNamePart,
       const NAString &schemaNamePart,
       const NAString &objectNamePart,
       ExeCliInterface &cliInterface,
       ComObjectType objType = COM_BASE_TABLE_OBJECT
   );

  short registerHiveView
  (
       const NAString &catalogNamePart,
       const NAString &schemaNamePart,
       const NAString &objectNamePart,
       Int32 objOwnerId,
       Int32 schemaOwnerId,
       NATable *naTable,
       ExeCliInterface &cliInterface,
       NABoolean isInternal,
       NABoolean cascade
   );

  short unregisterHiveView
  (
       const NAString &catalogNamePart,
       const NAString &schemaNamePart,
       const NAString &objectNamePart,
       NATable *naTable,
       ExeCliInterface &cliInterface,
       NABoolean cascade
   );

  short unregisterHiveSchema
  (
       const NAString &catalogNamePart,
       const NAString &schemaNamePart,
       ExeCliInterface &cliInterface,
       NABoolean cascade
   );

  void regOrUnregNativeObject (
       StmtDDLRegOrUnregObject * regOrUnregObject,
       NAString &currCatName, NAString &currSchName);
  
  short adjustHiveExternalSchemas(ExeCliInterface *cliInterface);

  void createSeabaseSequence(StmtDDLCreateSequence  * createSequenceNode,
			     NAString &currCatName, NAString &currSchName);

  void alterSeabaseSequence(StmtDDLCreateSequence  * alterSequenceNode,
			     NAString &currCatName, NAString &currSchName);
  
  void dropSeabaseSequence(StmtDDLDropSequence  * dropSequenceNode,
			   NAString &currCatName, NAString &currSchName);
  
  void seabaseGrantRevoke(
			  StmtDDLNode * stmtDDLNode,
			  NABoolean isGrant,
			  NAString &currCatName, NAString &currSchName,
                          NABoolean internalCall = FALSE);
  
  void hbaseGrantRevoke(StmtDDLNode * stmtDDLNode,
                        NABoolean isGrant,
                        NAString &currCatName, NAString &currSchName);
  
  void dropSeabaseSchema(StmtDDLDropSchema * dropSchemaNode);

  void alterSeabaseSchema(StmtDDLAlterSchema * alterSchemaNode);
                          
  
  bool dropOneTableorView(
     ExeCliInterface & cliInterface,
     const char * objectName,
     ComObjectType objectType,
     bool isVolatile);
  

 void createNativeHbaseTable(
                             ExeCliInterface *cliInterface,
                             StmtDDLCreateHbaseTable * createTableNode,
                             NAString &currCatName, NAString &currSchName);

 void dropNativeHbaseTable(
                             ExeCliInterface *cliInterface,
                             StmtDDLDropHbaseTable * dropTableNode,
                             NAString &currCatName, NAString &currSchName);
  
  void processDDLonHiveObjects(StmtDDLonHiveObjects * hddl,
                               NAString &currCatName, NAString &currSchName);

  void dropSeabaseMD(NABoolean ddlXns);
  void createSeabaseMDviews();
  void dropSeabaseMDviews();
  void createSeabaseSchemaObjects();
  void updateVersion();

  short initTrafMD(CmpDDLwithStatusInfo *mdti);
  
  short createPrivMgrRepos(ExeCliInterface *cliInterface, NABoolean ddlXns);
  short initSeabaseAuthorization(ExeCliInterface *cliInterface,
                                 NABoolean ddlXns,
                                 NABoolean isUpgrade,
                                 std::vector<std::string> &tablesCreated,
                                 std::vector<std::string> &tablesUpgraded);

  void dropSeabaseAuthorization(ExeCliInterface *cliInterface, 
                                NABoolean doCleanup = FALSE);

  void doSeabaseCommentOn(StmtDDLCommentOn *commentOnNode,
					 NAString &currCatName, 
					 NAString &currSchName);

  NABoolean insertPrivMgrInfo(const Int64 objUID,
                              const NAString &objName,
                              const ComObjectType objectType,
                              const Int32 objOwnerID,
                              const Int32 schemaOwnerID,
                              const Int32 creatorID);

  NABoolean deletePrivMgrInfo(const NAString &objName,
                              const Int64 objUID, 
                              const ComObjectType objType);


  short dropSeabaseObjectsFromHbase(const char * pattern, NABoolean ddlXns);
  short updateSeabaseAuths(ExeCliInterface * cliInterface, const char * sysCat);

  short truncateHbaseTable(const NAString &catalogNamePart, 
                           const NAString &schemaNamePart, 
                           const NAString &objectNamePart,
                           const NABoolean hasSaltedColumn,
                           ExpHbaseInterface * ehi);

  void purgedataHbaseTable(DDLExpr * ddlExpr,
			   NAString &currCatName, NAString &currSchName);

  short createRepos(ExeCliInterface * cliInterface);
  short dropRepos(ExeCliInterface * cliInterface, 
                  NABoolean oldRepos = FALSE, NABoolean dropSchema = TRUE,
                  NABoolean inRecovery = FALSE);
  short alterRenameRepos(ExeCliInterface * cliInterface, NABoolean newToOld);
  short copyOldReposToNew(ExeCliInterface * cliInterface);
  short dropAndLogReposViews(ExeCliInterface * cliInterface,
                             NABoolean & someViewSaved /* out */);

public:

  short upgradeRepos(ExeCliInterface * cliInterface, CmpDDLwithStatusInfo *mdui);
  short upgradeReposComplete(ExeCliInterface * cliInterface, CmpDDLwithStatusInfo *mdui);
  short upgradeReposUndo(ExeCliInterface * cliInterface, CmpDDLwithStatusInfo *mdui);

  NAString genHBaseObjName(const NAString &catName, 
			   const NAString &schName,
			   const NAString &objName);

protected:

  void processRepository(NABoolean createR, NABoolean dropR, NABoolean upgradeR);

  short updateSeabaseVersions(ExeCliInterface * cliInterface, const char * sysCat,
			      Lng32 majorVersion = -1);

  short getSpecialTableInfo
    (
     NAMemory * heap,
     const NAString &catName, 
     const NAString &schName, 
     const NAString &objName,
     const NAString &extTableName,
     const ComObjectType &objType, 
     ComTdbVirtTableTableInfo* &tableInfo
     );

  TrafDesc * getSeabaseMDTableDesc(const NAString &catName, 
				      const NAString &schName, 
				      const NAString &objName,
				      const ComObjectType objType);

  TrafDesc * getSeabaseHistTableDesc(const NAString &catName, 
					const NAString &schName, 
					const NAString &objName);

  ComTdbVirtTableSequenceInfo * getSeabaseSequenceInfo
    (const NAString &catName, 
     const NAString &schName, 
     const NAString &seqName,
     NAString &extSeqName,
     Int32 & objectOwner,
     Int32 & schemaOwner,
     Int64 & seqUID);

  TrafDesc * getSeabaseSequenceDesc(const NAString &catName, 
				       const NAString &schName, 
				       const NAString &seqName);
    
  ComTdbVirtTablePrivInfo * getSeabasePrivInfo
    (const Int64 objUID,
     const ComObjectType objType);

  Lng32 getSeabaseColumnInfo(ExeCliInterface *cliInterface,
                             Int64 objUID,
                             const NAString &catName,
                             const NAString &schName,
                             const NAString &objName,
                             char *direction,
                             NABoolean *tableIsSalted,
                             Lng32 * identityColPos,
                             Lng32 *numCols,
                             ComTdbVirtTableColumnInfo **colInfoArray);
  
  TrafDesc * getSeabaseUserTableDesc(const NAString &catName, 
					const NAString &schName, 
					const NAString &objName,
					const ComObjectType objType,
					NABoolean includeInvalidDefs,
					Int32 ctlFlags,
                                        Int32 &packedDescLen);
 
  static NABoolean getMDtableInfo(const ComObjectName &ansiName,
                                  ComTdbVirtTableTableInfo* &tableInfo,
				  Lng32 &colInfoSize,
				  const ComTdbVirtTableColumnInfo* &colInfo,
				  Lng32 &keyInfoSize,
				  const ComTdbVirtTableKeyInfo* &keyInfo,
				  Lng32 &indexInfoSize,
				  const ComTdbVirtTableIndexInfo* &indexInfo,
				  const ComObjectType objType);
                                  
  void giveSeabaseAll(StmtDDLGiveAll * giveAllParseNode);
  
  void giveSeabaseObject(StmtDDLGiveObject * giveObjectNode);
  
  void giveSeabaseSchema(
     StmtDDLGiveSchema * giveSchemaNode,
     NAString          & currentCatalogName);

  void glueQueryFragments(Lng32 queryArraySize,
			  const QString * queryArray,
			  char * &gluedQuery,
			  Lng32 &gluedQuerySize);

  short convertColAndKeyInfoArrays(
				    Lng32 btNumCols, // IN
				    ComTdbVirtTableColumnInfo* btColInfoArray, // IN
				    Lng32 btNumKeys, // IN
				    ComTdbVirtTableKeyInfo* btKeyInfoArray, // IN
				    NAColumnArray *naColArray,
				    NAColumnArray *naKeyArr);

  short processDDLandCreateDescs(
				 Parser &parser,
				 const QString *ddl,
				 Lng32 sizeOfddl,

				 NABoolean isIndexTable,

				 Lng32 btNumCols, // IN
				 ComTdbVirtTableColumnInfo* btColInfoArray, // IN
				 Lng32 btNumKeys, // IN
				 ComTdbVirtTableKeyInfo* btKeyInfoArray, // IN
    
				 Lng32 &numCols,
				 ComTdbVirtTableColumnInfo* &colInfoArray,
				 Lng32 &numKeys,
				 ComTdbVirtTableKeyInfo* &keyInfoArray,
				 ComTdbVirtTableIndexInfo* &indexInfo);

  short createIndexColAndKeyInfoArrays(
				       ElemDDLColRefArray & indexColRefArray,
				       NABoolean isUnique,
				       NABoolean hasSyskey,
                                       NABoolean alignedFormat,
                                       NAString &defaultColFam,
				       const NAColumnArray &baseTableNAColArray,
				       const NAColumnArray &baseTableKeyArr,
				       Lng32 &keyColCount,
				       Lng32 &nonKeyColCount,
				       Lng32 &totalColCount,
				       ComTdbVirtTableColumnInfo * &colInfoArray,
				       ComTdbVirtTableKeyInfo * &keyInfoArray,
				       NAList<NAString> &selColList,
                                       Lng32 &keyLength,
				       NAMemory * heap);

  // called by both Create Table and Create Index code
  // Given the optionsclause (from parser) and numSplits (parser/cqd/infered)
  // this method the produced hbaseOptions in a list that can be more
  // easily provided to the HBase create table API as well as in a string
  // that can be stored in Trafodion metadata. Returns 0 on success and a 
  // negative value to indicate failure.
  short setupHbaseOptions(ElemDDLHbaseOptions * hbaseOptionsClause, // in
                         Int32 numSplits, // in
                          const NAString& objName, //in for err handling
                         NAList<HbaseCreateOption*>& hbaseCreateOptions, //out
                         NAString& hco); // out
  

  short lookForTableInMD(
       ExeCliInterface *cliInterface,
       NAString &catNamePart, NAString &schNamePart, NAString &objNamePart,
       NABoolean schNameSpecified, NABoolean isHbaseMapSpecified,
       ComObjectName &tableName, NAString &tabName, NAString &extTableName,
       const ComObjectType objectType = COM_BASE_TABLE_OBJECT);

private:
  enum
  {
    NUM_MAX_PARAMS = 20
  };

  NAHeap *heap_;
  ULng32 savedCmpParserFlags_;
  ULng32 savedCliParserFlags_;

  NAString seabaseSysCat_;
  NAString seabaseMDSchema_; /* Qualified metadata schema */

  const char * param_[NUM_MAX_PARAMS];

  NABoolean cmpSwitched_;
};

#endif // _CMP_SEABASE_DDL_H_
