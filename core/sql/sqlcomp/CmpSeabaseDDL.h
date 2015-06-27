// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2014-2015 Hewlett-Packard Development Company, L.P.
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
#include "desc.h"
#include "CmpMessage.h"
#include "PrivMgrDefs.h"
#include "PrivMgrMD.h"
#include "ElemDDLHbaseOptions.h"
#include "CmpContext.h"

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

class DDLExpr;
class DDLNode;

class NADefaults;

class NAType;

struct desc_struct;
class OutputInfo;

class ByteArrayList;

class HbaseCreateOption;

class Parser;

class NAColumnArray;

struct routine_desc_struct;
struct MDDescsInfo;

class CmpDDLwithStatusInfo;

#include "CmpSeabaseDDLmd.h"

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

  static NABoolean isSeabaseMD(const NAString &catName,
			       const NAString &schName,
			       const NAString &objName);
 
  static NABoolean isSeabasePrivMgrMD(const NAString &catName,
			              const NAString &schName);
 
  static NABoolean isUserUpdatableSeabaseMD(const NAString &catName,
			       const NAString &schName,
			       const NAString &objName);

  static NABoolean isSeabaseReservedSchema(
                                           const NAString &catName,
                                           const NAString &schName);
 
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

  desc_struct * getSeabaseTableDesc(const NAString &catName, 
				    const NAString &schName, 
				    const NAString &objName,
				    const ComObjectType objType,
				    NABoolean includeInvalidDefs = FALSE);

  short getObjectOwner(ExeCliInterface *cliInterface,
                        const char * catName,
                        const char * schName,
                        const char * objName,
                        const char * objType,
                        Int32 * objectOwner);

  static bool describeSchema(
     const NAString & catalogName,
     const NAString & schemaName,
     NAString & output);
     
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

  desc_struct * getSeabaseLibraryDesc(
     const NAString &catName, 
     const NAString &schName, 
     const NAString &libraryName);
     
  desc_struct *getSeabaseRoutineDesc(const NAString &catName,
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
    return (flags & SEABASE_SERIALIZED) != 0;
  }

  short buildColInfoArray(
                          ComObjectType objType,
			  ElemDDLColDefArray * colArray,
			  ComTdbVirtTableColumnInfo * colInfoArray,
			  NABoolean implicitPK,
                          NABoolean alignedFormat,
                          Lng32 *identityColPos = NULL,
			  NAMemory * heap = NULL);

  // The next three methods do use anything from the CmpSeabaseDDL class.
  // They are placed here as a packaging convinience, to avoid code 
  // duplication that would occur if non-member static functions were used.
  // These methods convert VirtTable*Info classes to corresponding desc_struct
  // objects
  void convertVirtTableColumnInfoToDescStruct( 
       const ComTdbVirtTableColumnInfo * colInfo,
       const ComObjectName * objectName,
       desc_struct * column_desc);

  desc_struct * convertVirtTableColumnInfoArrayToDescStructs(
     const ComObjectName * objectName,
     const ComTdbVirtTableColumnInfo * colInfoArray,
     Lng32 numCols);

  desc_struct * convertVirtTableKeyInfoArrayToDescStructs(
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

   Int64 getObjectUIDandOwners(
                     ExeCliInterface * cliInterface,
                     const char * catName,
                     const char * schName,
                     const char * objName,
                     const ComObjectType objectType,
		     Int32 & objectOwner,
		     Int32 & schemaOwner,
		     bool reportErrorNow = true,
                     NABoolean checkForValidDef = FALSE);
  
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

   short switchCompiler(Int32 cntxtType = CmpContextInfo::CMPCONTEXT_TYPE_META);

   short switchBackCompiler();

  ExpHbaseInterface* allocEHI(NADefaults * defs = NULL);
  
  void deallocEHI(ExpHbaseInterface* &ehi);
  void dropLOBHdfsFiles();
 protected:

  enum {
    SEABASE_SERIALIZED = 0x0001,
    // set if we need to get the hbase snapshot info of the table
    GET_SNAPSHOTS = 0x0002
  };

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

  ComBoolean isSeabaseMD(const ComObjectName &name);

  ComBoolean isSeabasePrivMgrMD(const ComObjectName &name);

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

  desc_struct *getSeabaseRoutineDescInternal(const NAString &catName,
                                             const NAString &schName,
                                             const NAString &objName);

  // note: this function expects hbaseCreateOptionsArray to have
  // HBASE_MAX_OPTIONS elements
  short generateHbaseOptionsArray(NAText * hbaseCreateOptionsArray,
    NAList<HbaseCreateOption*> * hbaseCreateOptions);

  short createHbaseTable(ExpHbaseInterface *ehi, 
			 HbaseStr *table,
			 const char * cf1, const char * cf2, const char * cf3,
			 NAList<HbaseCreateOption*> * hbaseCreateOptions = NULL,
                         const int numSplits = 0,
                         const int keyLength = 0,
                         char **encodedKeysBuffer = NULL,
			 NABoolean doRetry = TRUE);

  short alterHbaseTable(ExpHbaseInterface *ehi,
                        HbaseStr *table,
                        NAList<HbaseCreateOption*> * hbaseCreateOptions);

  short dropHbaseTable(ExpHbaseInterface *ehi, 
		       HbaseStr *table, NABoolean asyncDrop = FALSE);

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
                                    Int64 & inUID);
                                    
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
                             Int32 objOwnerID,
                             Int32 schemaOwnerID,
                             Int64 &inUID);

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

  short updateObjectRedefTime(
                              ExeCliInterface *cliInterface,
                              const NAString &catName,
                              const NAString &schName,
                              const NAString &objName,
                              const char * objType,
                              Int64 rt = -1);

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

  // textType:   0, view text.  1, constraint text.  2, computed col text.
  // subID: 0, for text that belongs to table. colNumber, for column based text.
  short updateTextTable(ExeCliInterface *cliInterface,
                        Int64 objUID, 
                        Lng32 textType, 
                        Lng32 subID, 
                        NAString &text);

  ItemExpr * bindDivisionExprAtDDLTime(ItemExpr *expr,
                                       NAColumnArray *availableCols,
                                       NAHeap *heap);
  short validateDivisionByExprForDDL(ItemExpr *divExpr);

  short createEncodedKeysBuffer(char** &encodedKeysBuffer,
				desc_struct * colDescs, desc_struct * keyDescs,
				Lng32 numSplits, Lng32 numKeys, 
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
  
  short buildColInfoArray(ElemDDLParamDefArray *paramArray,
                          ComTdbVirtTableColumnInfo * colInfoArray);
  
  short buildKeyInfoArray(
			  ElemDDLColDefArray *colArray,
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
  
  short gatherViewPrivileges (const StmtDDLCreateView * createViewParseNode,
			      ExeCliInterface * cliInterface,
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
                        NABoolean sameSequenceOfCols, // FALSE, allow "similar" indexes
			ExeCliInterface *cliInterface);

  short createMetadataViews(ExeCliInterface * cliInterface);
  short dropMetadataViews(ExeCliInterface * cliInterface);
  short createSeqTable(ExeCliInterface * cliInterface);
 
  int addSchemaObject(
     ExeCliInterface & cliInterface,
     const ComSchemaName & schemaName,
     ComSchemaClass schemaClass,
     Int32 ownerID,
     NABoolean ignoreIfExists);
     
  short createSchemaObjects(ExeCliInterface * cliInterface);
  
  void  createSeabaseSchema(
     StmtDDLCreateSchema  * createSchemaNode,
     NAString             & currCatName);

  void cleanupObjectAfterError(
                               ExeCliInterface &cliInterface,
                               const NAString &catName, 
                               const NAString &schName,
                               const NAString &objName,
                               const ComObjectType objectType);
  
  short createSeabaseTable2(
                            ExeCliInterface &cliInterface,
                            StmtDDLCreateTable * createTableNode,
                            NAString &currCatName, NAString &currSchName,
                            NABoolean isCompound = FALSE);
  
  void createSeabaseTable(
			  StmtDDLCreateTable                  * createTableNode,
			  NAString &currCatName, NAString &currSchName,
                          NABoolean isCompound = FALSE);
 
  void createSeabaseTableCompound(
			  StmtDDLCreateTable                  * createTableNode,
			  NAString &currCatName, NAString &currSchName);
 
  void createSeabaseTableLike(
			      StmtDDLCreateTable                  * createTableNode,
			      NAString &currCatName, NAString &currSchName);
  
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
		      StmtDDLAddConstraintPK * pkConstr,
		      StmtDDLAddConstraintUniqueArray &uniqueConstrArr,
		      StmtDDLAddConstraintRIArray &riConstrArr,
		      StmtDDLAddConstraintCheckArray &checkConstrArr,
                      NABoolean isCompound = FALSE);
  
  void alterSeabaseTableAddColumn(
                                  StmtDDLAlterTableAddColumn * alterAddColNode,
                                  NAString &currCatName, NAString &currSchName);
  
  void alterSeabaseTableDropColumn(
				   StmtDDLAlterTableDropColumn * alterDropColNode,
				   NAString &currCatName, NAString &currSchName);
  
  short alignedFormatTableAddDropColumn(
                                        Int64 objUID,
                                        NABoolean isAdd,
                                        const NAString &catalogNamePart,
                                        const NAString &schemaNamePart,
                                        const NAString &objectNamePart,
                                        char * colName, const NAColumn * nacol);
  
  short recreateViews(ExeCliInterface &cliInterface,
                      NAList<NAString> &viewNameList,
                      NAList<NAString> &viewDefnList);

  void alterSeabaseTableAlterIdentityColumn(
                                            StmtDDLAlterTableAlterColumnSetSGOption * alterIdentityColNode,
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
                      Lng32 textType,
                      Lng32 textSubID,
		      NAString &constrText);
    
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

  void createSeabaseRoutine(StmtDDLCreateRoutine  * createRoutineNode,
                            NAString &currCatName, NAString &currSchName);
  
  void dropSeabaseRoutine(StmtDDLDropRoutine  * dropRoutineNode,
                          NAString &currCatName, NAString &currSchName);

  void createSeabaseSequence(StmtDDLCreateSequence  * createSequenceNode,
			     NAString &currCatName, NAString &currSchName);

  void alterSeabaseSequence(StmtDDLCreateSequence  * alterSequenceNode,
			     NAString &currCatName, NAString &currSchName);
  
  void dropSeabaseSequence(StmtDDLDropSequence  * dropSequenceNode,
			   NAString &currCatName, NAString &currSchName);
  
  void seabaseGrantRevoke(
			  StmtDDLNode                  * stmtDDLNode,
			  NABoolean isGrant,
			  NAString &currCatName, NAString &currSchName,
                          NABoolean useHBase = FALSE);
  
  void seabaseGrantRevokeHBase(StmtDDLNode                  * stmtDDLNode,
                               NABoolean isGrant,
                               NAString &currCatName, NAString &currSchName);

  void dropSeabaseSchema(StmtDDLDropSchema * dropSchemaNode);
  
  bool dropOneTableorView(
     ExeCliInterface & cliInterface,
     const char * objectName,
     ComObjectType objectType,
     bool isVolatile);
  

 void createNativeHbaseTable(
			     StmtDDLCreateHbaseTable                  * createTableNode,
			     NAString &currCatName, NAString &currSchName);

 void dropNativeHbaseTable(
			     StmtDDLDropHbaseTable                  * createTableNode,
			     NAString &currCatName, NAString &currSchName);
  
  void initSeabaseMD();
  void dropSeabaseMD();
  void createSeabaseMDviews();
  void dropSeabaseMDviews();
  void createSeabaseSeqTable();
  void createSeabaseSchemaObjects();
  void updateVersion();

  short createPrivMgrRepos(ExeCliInterface *cliInterface);
  short initSeabaseAuthorization(ExeCliInterface *cliInterface,
                                std::vector<std::string> &tablesCreated,
                                std::vector<std::string> &tablesUpgraded);

  void dropSeabaseAuthorization(ExeCliInterface *cliInterface);

  NABoolean insertPrivMgrInfo(const Int64 objUID,
                              const NAString &objName,
                              const ComObjectType objectType,
                              const Int32 objOwnerID,
                              const Int32 schemaOwnerID,
                              const Int32 creatorID);

  NABoolean deletePrivMgrInfo(const NAString &objName,
                              const Int64 objUID, 
                              const ComObjectType objType);


  short dropSeabaseObjectsFromHbase(const char * pattern);
  short updateSeabaseAuths(ExeCliInterface * cliInterface, const char * sysCat);

  void purgedataHbaseTable(DDLExpr * ddlExpr,
			   NAString &currCatName, NAString &currSchName);

  short createRepos(ExeCliInterface * cliInterface);
  short dropRepos(ExeCliInterface * cliInterface, 
                  NABoolean oldRepos = FALSE, NABoolean dropSchema = TRUE);
  short alterRenameRepos(ExeCliInterface * cliInterface, NABoolean newToOld);
  short copyOldReposToNew(ExeCliInterface * cliInterface);
  short upgradeRepos(ExeCliInterface * cliInterface, CmpDDLwithStatusInfo *mdui);

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

  desc_struct * getSeabaseMDTableDesc(const NAString &catName, 
				      const NAString &schName, 
				      const NAString &objName,
				      const ComObjectType objType);

  desc_struct * getSeabaseHistTableDesc(const NAString &catName, 
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

  desc_struct * getSeabaseSequenceDesc(const NAString &catName, 
				       const NAString &schName, 
				       const NAString &seqName);
    
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
  
  desc_struct * getSeabaseUserTableDesc(const NAString &catName, 
					const NAString &schName, 
					const NAString &objName,
					const ComObjectType objType,
					NABoolean includeInvalidDefs,
					Int32 ctlFlags);
 
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

  desc_struct * assembleRegionDescs(ByteArrayList* bal, desc_nodetype format);

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
