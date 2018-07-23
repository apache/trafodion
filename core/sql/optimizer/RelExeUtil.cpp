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
******************************************************************************
*
* File:         RelExeUtil.cpp
* Description:  Methods for ExeUtil operators
* Created:      10/16/2008
* Language:     C++
*
*
******************************************************************************
*/

#define   SQLPARSERGLOBALS_FLAGS	// must precede all #include's
#define   SQLPARSERGLOBALS_NADEFAULTS

#include "ComRtUtils.h"
#include "Debug.h"
#include "Sqlcomp.h"
#include "AllRelExpr.h"
#include "AllItemExpr.h"
#include "GroupAttr.h"
#include "opt.h"
#include "PhyProp.h"
#include "ScanOptimizer.h"
#include "CmpContext.h"
#include "ExpError.h"
#include "ComTransInfo.h"
#include "BindWA.h"
#include "Refresh.h"
#include "CmpMain.h"
#include "ControlDB.h"
#include "Analyzer.h"
#include "OptHints.h"
#include "ComTdbSendTop.h"
#include "DatetimeType.h"

#  ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#  define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#  endif

#include "StmtDDLCreateView.h"
#include "StmtDDLDropView.h"
#include "StmtDDLCreateTable.h"
#include "StmtDDLDropTable.h"
#include "StmtDDLCreateIndex.h"
#include "StmtDDLPopulateIndex.h"
#include "StmtDDLDropIndex.h"
#include "StmtDDLAlterIndex.h"   // why don't we need StmtDDLAlterTable as well???
#include "StmtDDLAlterSchema.h"
#include "StmtDDLCreateDropSequence.h"
#include "StmtDDLGrant.h"
#include "StmtDDLRevoke.h"
#include "ElemDDLConstraintPK.h"
#include "ElemDDLConstraintUnique.h"
#include "StmtDDLCreateSchema.h"
#include "StmtDDLDropSchema.h"
#include "StmtDDLCreateLibrary.h"
#include "StmtDDLDropLibrary.h"
#include "StmtDDLCreateRoutine.h"
#include "StmtDDLDropRoutine.h"
#include "StmtDDLCleanupObjects.h"
#include "StmtDDLAlterLibrary.h"
#include "StmtDDLRegOrUnregHive.h"
#include "StmtDDLCommentOn.h"
#include "StmtDDLonHiveObjects.h"

#include <cextdecs/cextdecs.h>
#include "wstr.h"
#include "Inlining.h"
#include "Triggers.h"
#include "TriggerDB.h"
#include "MVInfo.h"
#include "Refresh.h"
#include "ChangesTable.h"
#include "MvRefreshBuilder.h"
#include "OptHints.h"
#include "CmpStatement.h"
#include "charinfo.h"
#include "SqlParserGlobals.h"		// must be last #include
#include "ItmFlowControlFunction.h"
#include "HDFSHook.h"
#include "PrivMgrComponentPrivileges.h"
#include "ComUser.h"
#include "CmpSeabaseDDL.h"
#include "SqlTableOpenInfo.h"


NAWchar *SQLTEXTW();
void castComputedColumnsToAnsiTypes(BindWA *bindWA,
				    RETDesc *rd,
				    ValueIdList &compExpr);

#define TEXT_DISPLAY_LENGTH 1001


// -----------------------------------------------------------------------
// Member functions for class GenericUtilExpr
// -----------------------------------------------------------------------

NABoolean GenericUtilExpr::duplicateMatch(const RelExpr & other) const
{
  if (NOT RelExpr::duplicateMatch(other))
    return FALSE;

  // a simplified version, should really check all fields
  GenericUtilExpr &o = (GenericUtilExpr &) other;
  if (NOT (stmtText_ == o.stmtText_ ||
           stmtText_ && o.stmtText_ &&
           (strcmp(stmtText_, o.stmtText_) == 0)))
    return FALSE;

  return TRUE;
}

RelExpr * GenericUtilExpr::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  GenericUtilExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) GenericUtilExpr((char *)NULL, CharInfo::UnknownCharSet,
					   getExprNode(), NULL,
					   getOperatorType(), outHeap);
  else
    result = (GenericUtilExpr *) derivedNode;

  return RelExpr::copyTopNode(result, outHeap);
}

void GenericUtilExpr::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  outputValues.clear();
  if ((getVirtualTableDesc()) &&
      (((ExeUtilExpr*)this)->producesOutput()))
    outputValues.insertList(getVirtualTableDesc()->getColumnList());
} // GenericUtilExpr::getPotentialOutputValues()

// -----------------------------------------------------------------------
// member functions for class GenericUtilExpr
// -----------------------------------------------------------------------

RelExpr * GenericUtilExpr::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  if (getVirtualTableName())
    {
      CorrName corrName(getVirtualTableName());
      corrName.setSpecialType(ExtendedQualName::VIRTUAL_TABLE);
      NATable *naTable = bindWA->getSchemaDB()->getNATableDB()->
	get(&corrName.getExtendedQualNameObj());
      
      if (NOT naTable) {
	TrafDesc *tableDesc = createVirtualTableDesc();
	naTable = bindWA->getNATable(corrName, FALSE/*catmanUsages*/, tableDesc);
	if (bindWA->errStatus())
	  return this;
      }
      
      // Allocate a TableDesc and attach it to this.
      //
      setVirtualTableDesc(bindWA->createTableDesc(naTable, corrName));
      if (bindWA->errStatus())
	return this;
      
      //
      // Allocate an RETDesc and attach it to this and the BindScope.
      //
      if (producesOutput())
	{
	  setRETDesc(new (bindWA->wHeap()) RETDesc(bindWA, getVirtualTableDesc()));
	}
      else
	{
          setRETDesc(new (bindWA->wHeap()) RETDesc(bindWA));
	}
    }
  else
    {
      // no rows are returned from this operator. 
      // Allocate an empty RETDesc and attach it to this and the BindScope.
      //
      setRETDesc(new (bindWA->wHeap()) RETDesc(bindWA));
    }

  bindWA->getCurrentScope()->setRETDesc(getRETDesc());

  //since this is DDL statement, dont use the cached metadata
  //instead recreate NATable objects by re-reading the metadata from
  //disk
  if (dontUseCache())
    CmpCommon::context()->schemaDB_->getNATableDB()->dontUseCache();
  //
  // Bind the base class.
  //
  return bindSelf(bindWA);
} // GenericUtilExpr::bindNode()

// -----------------------------------------------------------------------
// Member functions for class DDLExpr
// -----------------------------------------------------------------------

RelExpr * DDLExpr::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  DDLExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) DDLExpr(getDDLNode(), // ExprNode * ddlNode
                                   (char *)NULL, // char * ddlStmtText
                                   CharInfo::UnknownCharSet,
                                   outHeap);
  else
    result = (DDLExpr *) derivedNode;

  result->specialDDL() = specialDDL();

  result->ddlObjNATable_ = ddlObjNATable_;

  result->explObjName_ = explObjName_;
  result->numExplRows_ = numExplRows_;

  result->objName_ = objName_;
  result->isVolatile_ = isVolatile_;
  result->isTable_ = isTable_;
  result->isSchema_ = isSchema_;
  result->isIndex_ = isIndex_;
  result->isMV_ = isMV_;
  result->isView_ = isView_;
  result->isLibrary_ = isLibrary_;
  result->isRoutine_ = isRoutine_;
  result->isUstat_ = isUstat_;

  result->isCreate_ = isCreate_;
  result->isCreateLike_ = isCreateLike_;
  result->isDrop_ = isDrop_;
  result->isAlter_ = isAlter_;
  result->isCleanup_ = isCleanup_;
  result->qualObjName_ = qualObjName_;
  result->purgedataTableName_ = purgedataTableName_;
  result->isHbase_ = isHbase_;
  result->isNative_ = isNative_;
  result->hbaseDDLNoUserXn_ = hbaseDDLNoUserXn_;

  result->returnStatus_ = returnStatus_;

  result->flags_ = flags_;

  return GenericUtilExpr::copyTopNode(result, outHeap);
}

const NAString DDLExpr::getText() const
{
  NAString result(CmpCommon::statementHeap());

  if (getDDLNode() && getDDLNode()->getOperatorType() ==  DDL_ON_HIVE_OBJECTS)
    result = "HIVE_DDL";
  else
    result = "DDL";

  return result;
}

void
DDLExpr::unparse(NAString &result,
		 PhaseEnum /* phase */,
		 UnparseFormatEnum /* form */,
		 TableDesc * tabId) const
{
  result += "a DDL statement";
}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilExpr
// -----------------------------------------------------------------------
NABoolean ExeUtilExpr::pilotAnalysis(QueryAnalysis* qa)
{
  NABoolean status = RelExpr::pilotAnalysis(qa);
  // stop here if it fails
  if (!status)
    return FALSE;

  if (!qa->newTableAnalysis(this))
    return FALSE;

  return TRUE;
}

HashValue ExeUtilExpr::topHash()
{
  HashValue result = GenericUtilExpr::topHash();

  result ^= tableId_;

  return result;
}

NABoolean ExeUtilExpr::duplicateMatch(const RelExpr & other) const
{
  if (NOT GenericUtilExpr::duplicateMatch(other))
    return FALSE;

  // a simplified version, should really check all fields
  ExeUtilExpr &o = (ExeUtilExpr &) other;
  if (NOT (tableId_ == o.tableId_))
    return FALSE;

  // if tableDesc is not allocated, compare the names
  if (tableId_ == NULL &&
      NOT(tableName_ == o.tableName_))
    return FALSE;

  return TRUE;
}

RelExpr * ExeUtilExpr::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilExpr(getExeUtilType(),
				       getTableName(),
				       getExprNode(), NULL,
				       (char *)NULL,             // char * stmtText
				       CharInfo::UnknownCharSet, // CharInfo::CharSet stmtTextCharSet
				       outHeap);
  else
    result = (ExeUtilExpr *) derivedNode;

  result->tableId_ = tableId_;
  result->virtualTabId_ = virtualTabId_;
  result->setOptStoi(stoi_);

  return GenericUtilExpr::copyTopNode(result, outHeap);
}

const NAString ExeUtilExpr::getText() const
{
  NAString result(CmpCommon::statementHeap());

  switch (type_)
    {
    case DISPLAY_EXPLAIN_:
      result = "EXPLAIN_CMD";
      break;

    case DISPLAY_EXPLAIN_COMPLEX_:
      result = "EXPLAIN_COMPLEX_CMD";
      break;

    case MAINTAIN_OBJECT_:
      result = "MAINTAIN";
      break;

    case LOAD_VOLATILE_:
      result = "LOAD_VOLATILE";
      break;

    case CLEANUP_VOLATILE_TABLES_:
      result = "CLEANUP_VOL_TABS";
      break;

    case GET_VOLATILE_INFO_:
      result = "GET_VOLATILE";
      break;

    case GET_ERROR_INFO_:
      result = "GET_ERROR";
      break;

    case CREATE_TABLE_AS_:
      result = "CREATE_TABLE_AS";
      break;

    case HIVE_TRUNCATE_:
      result = "HIVE_TRUNCATE";
      break;

    case GET_STATISTICS_:
      result = "GET_STATISTICS";
      break;

    case LONG_RUNNING_:
      result = "EXE_LONG_RUNNING";
      break;

    case GET_METADATA_INFO_:
      result = "GET_METADATA_INFO";
      break;

    case GET_VERSION_INFO_:
      result = "GET_VERSION_INFO";
      break;

    case POP_IN_MEM_STATS_:  
      result = "POP_IN_MEM_STATS";
      break;

    case LOB_EXTRACT_:
      result = "LOB_EXTRACT";
      break;
      
    case HBASE_COPROC_AGGR_:
      result = "HBASE_COPROC_AGGR";
      break;
      
    case ORC_FAST_AGGR_:
      result = "ORC_FAST_AGGR";
      break;
      
    case WNR_INSERT_:
      result = "NO_ROLLBACK_INSERT";
      break;
      
    case METADATA_UPGRADE_:
      result = "METADATA_UPGRADE";
      break;

    case GET_QID_:
      result = "GET_QID";
      break;

    case REGION_STATS_:
      result = "REGION_STATS";
      break;
      
    case HIVE_QUERY_:
      result = "HIVE_QUERY";
      break;
      
    default:
      result = "ADD_TO_EXEUTILEXPR::GETTEXT()";
      break;

    }

  return result;
}

void
ExeUtilExpr::unparse(NAString &result,
		 PhaseEnum /* phase */,
		 UnparseFormatEnum /* form */,
		 TableDesc * tabId) const
{
  switch (type_)
    {
    case DISPLAY_EXPLAIN_:
      result += "Explain command";
      break;

    case DISPLAY_EXPLAIN_COMPLEX_:
      result += "Explain complex command";
      break;

    case LONG_RUNNING_:
      result += "Long Running statement";

    default:
      break;
    }
}

// Returns TRUE if current user has requested component privilege
NABoolean ExeUtilExpr::checkForComponentPriv(
  SQLOperation operation,
  BindWA *bindWA)
{
  // If authorization is not enabled, implicitly have priv
  if (!bindWA->currentCmpContext()->isAuthorizationEnabled())
    return TRUE;

  // if DB__ROOT, implicitly have priv
  if (ComUser::getCurrentUser() == ComUser::getRootUserID())
    return TRUE;

  // If have requested component privilege, return TRUE
  NAString privMgrMDLoc = CmpSeabaseDDL::getSystemCatalogStatic() + \
                          NAString(".\"") + \
                          SEABASE_PRIVMGR_SCHEMA + \
                          NAString ("\"");
  PrivMgrComponentPrivileges componentPrivileges(std::string(privMgrMDLoc.data()),
                                                 CmpCommon::diags());
  if (componentPrivileges.hasSQLPriv(ComUser::getCurrentUser(),operation,true))
    return TRUE;
 return FALSE;
}

// Sets up a stoi table entry.  This entry is used later when privileges
// are checked to make sure the current user has required privileges 
void ExeUtilExpr::setupStoiForPrivs (
  SqlTableOpenInfo::AccessFlags privs,
  BindWA *bindWA)
{

  // The stoi requires information from NATable
  NATable *naTable = bindWA->getNATable(getTableName());
  if (naTable == NULL)
  {
    if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
      *CmpCommon::diags() << DgSqlCode(-1034);
    bindWA->setErrStatus();
    return;
  }

  NAString fileName( naTable->getClusteringIndex()->
                          getFileSetName().getQualifiedNameAsString(),
                     bindWA->wHeap());

  SqlTableOpenInfo * stoi_ = new (bindWA->wHeap()) SqlTableOpenInfo;

  stoi_->setFileName(convertNAString(fileName, bindWA->wHeap()));

  // set privileges as requested
  stoi_->setSelectAccess(privs.select_);
  stoi_->setInsertAccess(privs.insert_);
  stoi_->setUpdateAccess(privs.update_);
  stoi_->setDeleteAccess(privs.delete_);

  // Add to stoiList associated with plan
  OptSqlTableOpenInfo *stoiInList = NULL;

  // First see if entry has already been added
  for (CollIndex i=0; i < bindWA->getStoiList().entries(); i++)
    if (strcmp(bindWA->getStoiList()[i]->getStoi()->fileName(), fileName) == 0) {
      stoiInList = bindWA->getStoiList()[i];
      break;
    }

  // If entry has not been added, add it to bindWA's list
  if (!stoiInList)
  {
    OptSqlTableOpenInfo *optStoi = new (bindWA->wHeap())
      OptSqlTableOpenInfo( stoi_,getTableName(), bindWA->wHeap());
    optStoi->setTable ((NATable *)naTable);
    bindWA->getStoiList().insert(optStoi);
  }

  // Adjust list of privileges to include utility requirements
  else
  {
    if (stoi_->getSelectAccess()) stoiInList->getStoi()->setSelectAccess();
    if (stoi_->getInsertAccess()) stoiInList->getStoi()->setInsertAccess();
    if (stoi_->getUpdateAccess()) stoiInList->getStoi()->setUpdateAccess();
    if (stoi_->getDeleteAccess()) stoiInList->getStoi()->setDeleteAccess();
  }
  return;
}

ExeUtilGetStatistics::ExeUtilGetStatistics(NAString statementName,
					   char * optionsStr,
					   CollHeap *oHeap,
                                           short statsReqType,
                                           short statsMergeType,
                                           short activeQueryNum)
     : ExeUtilExpr(GET_STATISTICS_,
		   CorrName("dummyName"), NULL, NULL, NULL, CharInfo::UnknownCharSet, oHeap),
       statementName_(statementName),
       compilerStats_(FALSE), executorStats_(FALSE),
       otherStats_(FALSE), detailedStats_(FALSE),
       oldFormat_(FALSE),
       shortFormat_(FALSE),
       tokenizedFormat_(FALSE),
       errorInParams_(FALSE),
       statsReqType_(statsReqType),
       statsMergeType_(statsMergeType),
       singleLineFormat_(FALSE),
       activeQueryNum_(activeQueryNum)
{
  NABoolean explicitStatsOption = FALSE;

  if (optionsStr)
    {
      optionsStr_ = optionsStr;

      size_t currIndex = 0;

      while (currIndex < optionsStr_.length())
	{
	  if ((optionsStr_.length() - currIndex) < 2)
	    {
	      errorInParams_ = TRUE;
	      return;
	    }

	  NAString option = optionsStr_(currIndex, 2);
	  if (option.isNull())
	    {
	      errorInParams_ = TRUE;
	      return;
	    }

	  option.toUpper();

	  if (option == "CS")
	    {
	      compilerStats_ = TRUE;
	      explicitStatsOption = TRUE;
	    }
	  else if (option == "ES")
	    {
	      executorStats_ = TRUE;
	      explicitStatsOption = TRUE;
	    }
	  else if (option == "OS")
	    {
	      otherStats_ = TRUE;
	      explicitStatsOption = TRUE;
	    }
	  else if (option == "DS")
	    {
	      detailedStats_ = TRUE;
	      explicitStatsOption = TRUE;
	    }
	  else if (option == "OF")
	    oldFormat_ = TRUE;
	  else if (option == "SF")
	    shortFormat_ = TRUE;
	  else if (option == "TF")
	    tokenizedFormat_ = TRUE;
	  else if (option == "NC")
            shortFormat_ = TRUE;
	  else if (option == "SL")
            singleLineFormat_ = TRUE;
          else 
	    {
	      errorInParams_ = TRUE;
	      return;
	    }

	  currIndex += 3;
	}
    }
    else
       oldFormat_ = TRUE;

  if (NOT explicitStatsOption)
    {
      compilerStats_ = TRUE;
      executorStats_ = TRUE;
      otherStats_    = TRUE;
      detailedStats_ = TRUE;
    }

  if (oldFormat_ || shortFormat_)
    {
      compilerStats_ = FALSE;
      otherStats_ = FALSE;
    }
    //COMP_BOOL_195 is no longer is needed
  if (tokenizedFormat_)
    {
      // not yet supported
      errorInParams_ = TRUE;
      return;
    }
};

// -----------------------------------------------------------------------
// Member functions for class ExeUtilProcesVolatileTable
// -----------------------------------------------------------------------
RelExpr * ExeUtilProcessVolatileTable::copyTopNode(RelExpr *derivedNode,
						   CollHeap* outHeap)
{
  ExeUtilProcessVolatileTable *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilProcessVolatileTable(getExprNode(),
						       NULL, CharInfo::UnknownCharSet, outHeap);
  else
    result = (ExeUtilProcessVolatileTable *) derivedNode;

  result->volTabName_ = volTabName_;
  result->isCreate_   = isCreate_;
  result->isTable_    = isTable_;
  result->isIndex_    = isIndex_;
  result->isSchema_   = isSchema_;

  return DDLExpr::copyTopNode(result, outHeap);
}

const NAString ExeUtilProcessVolatileTable::getText() const
{
  NAString result(CmpCommon::statementHeap());

  result = "VOLATILE_DDL";

  return result;
}

void
ExeUtilProcessVolatileTable::unparse(NAString &result,
				     PhaseEnum /* phase */,
				     UnparseFormatEnum /* form */,
				     TableDesc * tabId) const
{
  result = " Volatile DDL";
}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilProcessExceptionTable
// -----------------------------------------------------------------------

RelExpr * ExeUtilProcessExceptionTable::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  DDLExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilProcessExceptionTable(getDDLNode(), 0, CharInfo::UnknownCharSet, outHeap);
  else
    result = (DDLExpr *) derivedNode;

  return DDLExpr::copyTopNode(result, outHeap);
}

const NAString ExeUtilProcessExceptionTable::getText() const
{
  NAString result(CmpCommon::statementHeap());

  result = "LoadExceptionTable";

  return result;
}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilLoadVolatileTable
// -----------------------------------------------------------------------
RelExpr * ExeUtilLoadVolatileTable::copyTopNode(RelExpr *derivedNode,
						 CollHeap* outHeap)
{
  ExeUtilLoadVolatileTable *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilLoadVolatileTable(getTableName(),
						    NULL,
						    outHeap);
  else
    result = (ExeUtilLoadVolatileTable *) derivedNode;

  result->insertQuery_   = insertQuery_;
  result->updStatsQuery_ = updStatsQuery_;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilCleanupVolatileTable
// -----------------------------------------------------------------------
RelExpr * ExeUtilCleanupVolatileTables::copyTopNode(RelExpr *derivedNode,
						    CollHeap* outHeap)
{
  ExeUtilCleanupVolatileTables *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilCleanupVolatileTables(type_, catName_,
							outHeap);
  else
    result = (ExeUtilCleanupVolatileTables *) derivedNode;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilGetErrorInfo
// -----------------------------------------------------------------------
RelExpr * ExeUtilGetErrorInfo::copyTopNode(RelExpr *derivedNode,
					   CollHeap* outHeap)
{
  ExeUtilGetErrorInfo *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilGetErrorInfo(errNum_,
					       outHeap);
  else
    result = (ExeUtilGetErrorInfo *) derivedNode;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilGetVolatileInfo
// -----------------------------------------------------------------------
RelExpr * ExeUtilGetVolatileInfo::copyTopNode(RelExpr *derivedNode,
					      CollHeap* outHeap)
{
  ExeUtilGetVolatileInfo *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilGetVolatileInfo(type_, sessionId_,
						  outHeap);
  else
    result = (ExeUtilGetVolatileInfo *) derivedNode;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilCreateTableAs
// -----------------------------------------------------------------------
RelExpr * ExeUtilCreateTableAs::copyTopNode(RelExpr *derivedNode,
					    CollHeap* outHeap)
{
  ExeUtilCreateTableAs *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilCreateTableAs(getTableName(),
						getExprNode(),
						NULL, CharInfo::UnknownCharSet, outHeap);
  else
    result = (ExeUtilCreateTableAs *) derivedNode;

  result->ctQuery_ = ctQuery_;
  result->siQuery_ = siQuery_;
  result->viQuery_ = viQuery_;
  result->usQuery_ = usQuery_;

  result->loadIfExists_ = loadIfExists_;
  result->noLoad_ = noLoad_;
  result->isVolatile_ = isVolatile_;
  result->deleteData_ = deleteData_;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilHiveTruncateLegacy
// -----------------------------------------------------------------------
RelExpr * ExeUtilHiveTruncateLegacy::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilHiveTruncateLegacy *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilHiveTruncateLegacy(getTableName(),
                                                     pl_,
                                                     outHeap);
  else
    result = (ExeUtilHiveTruncateLegacy *) derivedNode;

  result->hiveTableLocation_= hiveTableLocation_;
  result->hiveHostName_ = hiveHostName_;
  result->hiveHdfsPort_ = hiveHdfsPort_;
  result->suppressModCheck_ = suppressModCheck_;
  result->dropTableOnDealloc_ = dropTableOnDealloc_;
  result->noSecurityCheck_ = noSecurityCheck_;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilHiveTruncate
// -----------------------------------------------------------------------
RelExpr * ExeUtilHiveTruncate::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilHiveTruncate *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilHiveTruncate(getTableName(),
                                               hiveTableName_,
                                               hiveTruncQuery_,
                                               outHeap);
  else
    result = (ExeUtilHiveTruncate *) derivedNode;

  result->dropTableOnDealloc_ = dropTableOnDealloc_;
  result->noSecurityCheck_ = noSecurityCheck_;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
RelExpr * ExeUtilHiveQuery::copyTopNode(RelExpr *derivedNode,
                                        CollHeap* outHeap)
{
  ExeUtilHiveQuery *result;
  if (derivedNode == NULL)
    result = new (outHeap) 
      ExeUtilHiveQuery(hiveQuery(),
                       sourceType(),
                       outHeap);
  else
    result = (ExeUtilHiveQuery *) derivedNode;
  return ExeUtilExpr::copyTopNode(result, outHeap);
}
RelExpr * ExeUtilHiveQuery::bindNode(BindWA *bindWA)
{
  if (type_ != FROM_STRING)
    {
      *CmpCommon::diags() << DgSqlCode(-3242) << DgString0("DDL can only be specified as a string.");
      bindWA->setErrStatus();
      return NULL;
    }

  // insert query returns an error from HiveClient.executeQuery on HDP platform.
  // Until that issue is fixed, disallow insert from 'process hive statement'.
  hiveQuery_ = hiveQuery_.strip(NAString::leading, ' ');
  if (NOT ((hiveQuery_.index("CREATE ", 0, NAString::ignoreCase) == 0) ||
           (hiveQuery_.index("DROP ", 0, NAString::ignoreCase) == 0) ||
           (hiveQuery_.index("ALTER ", 0, NAString::ignoreCase) == 0) ||
           //           (hiveQuery_.index("INSERT ", 0, NAString::ignoreCase) == 0) ||
           (hiveQuery_.index("TRUNCATE ", 0, NAString::ignoreCase) == 0)))
    {
      *CmpCommon::diags() << DgSqlCode(-3242) << DgString0("Specified operation cannot be executed directly by Hive.");
      bindWA->setErrStatus();
      return NULL;
    }

  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;
  return boundExpr;
}
// Member functions for class ExeUtilGetStatistics
// -----------------------------------------------------------------------
RelExpr * ExeUtilGetStatistics::copyTopNode(RelExpr *derivedNode,
					    CollHeap* outHeap)
{
  ExeUtilGetStatistics *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilGetStatistics(statementName_,
						(char*)(optionsStr_.data()),
						outHeap);
  else
    result = (ExeUtilGetStatistics *) derivedNode;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
//  Member functions for class ExeUtilGetProcessStatistics
// -----------------------------------------------------------------------
ExeUtilGetProcessStatistics::ExeUtilGetProcessStatistics(NAString statementName,
						     char * optionsStr,
						     CollHeap *oHeap)
  : ExeUtilGetStatistics(statementName, NULL, 
			 oHeap, 
			 SQLCLI_STATS_REQ_PROCESS_INFO, 
                         SQLCLI_SAME_STATS)
{
}

RelExpr * ExeUtilGetProcessStatistics::copyTopNode(RelExpr *derivedNode,
						 CollHeap* outHeap)
{
  ExeUtilGetProcessStatistics *result;

  if (derivedNode == NULL)
    result = new (outHeap) 
      ExeUtilGetProcessStatistics(statementName_,
				(char*)(optionsStr_.data()),
				outHeap);
  else
    result = (ExeUtilGetProcessStatistics *) derivedNode;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilGetMetadataInfo
// -----------------------------------------------------------------------
RelExpr * ExeUtilGetMetadataInfo::copyTopNode(RelExpr *derivedNode,
					      CollHeap* outHeap)
{
  ExeUtilGetMetadataInfo *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilGetMetadataInfo(ausStr_,
						  infoType_,
						  iofStr_,
						  objectType_,
						  objectName_,
						  &pattern_,
						  returnFullyQualNames_,
						  getVersion_,
						  &param1_,
						  outHeap);
  else
    result = (ExeUtilGetMetadataInfo *) derivedNode;

  result->noHeader_ = noHeader_;
  result->hiveObjs_ = hiveObjs_;
  result->hbaseObjs_ = hbaseObjs_;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}


// -----------------------------------------------------------------------
// Member functions for class ExeUtilDisplayExplain
// -----------------------------------------------------------------------
ExeUtilDisplayExplain::ExeUtilDisplayExplain(
     ExeUtilType opType,
     char * stmtText,
     CharInfo::CharSet stmtTextCharSet,
     char * moduleName,
     char * stmtName,
     char * optionsStr,
     ExprNode * exprNode,
     CollHeap *oHeap)
     : ExeUtilExpr(opType, CorrName("DUMMY"), exprNode, NULL,
                   stmtText, stmtTextCharSet, oHeap),
       moduleName_(NULL), stmtName_(NULL),
       optionsStr_(NULL),
       flags_(0)
{
  if (optionsStr)
    {
      optionsStr_ = new(oHeap) char[strlen(optionsStr) + 1];
      strcpy(optionsStr_, optionsStr);
    }

  if (moduleName)
    {
      moduleName_ = new(oHeap) char[strlen(moduleName) + 1];
      strcpy(moduleName_, moduleName);
    }

  if (stmtName)
    {
      stmtName_ = new(oHeap) char[strlen(stmtName) + 1];
      strcpy(stmtName_, stmtName);
    }

}

RelExpr * ExeUtilDisplayExplain::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilDisplayExplain *result;

  result = new (outHeap) ExeUtilDisplayExplain(DISPLAY_EXPLAIN_,
					       (char *)NULL,                   // stmtText
					       CharInfo::UnknownCharSet,       // stmtTextCharSet
					       moduleName_, stmtName_,
					       NULL, getExprNode(),
					       outHeap);

  result->flags_ = flags_;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

short ExeUtilDisplayExplain::setOptionX(char c, Int32 &numOptions)
{
  switch(c)
    {
    case 'e' : 
      if (isOptionE())
        return -1; // already specified
      flags_ |= OPTION_E; 
      numOptions++;
      break;             // expert mode
    case 'f' : 
      if (isOptionF())
        return -1; // already specified
      flags_ |= OPTION_F; 
      numOptions++;
      break;             // formatted summary mode
    case 'm' : 
      if (isOptionM())
        return -1; // already specified
      flags_ |= OPTION_M; 
      numOptions++;
      break;             // machine readable mode
    case 'n' : 
      if (isOptionN())
        return -1; // already specified
      flags_ |= OPTION_N; 
      numOptions++;
      break;             // normal mode
    case 'c' : 
      if (isOptionC() && (NOT isOptionP()))
        return -1; // already specified
      flags_ |= OPTION_C; 
      break;             // cleansed mode
    case 'p' : 
      if (isOptionP())
        return -1; // already specified
      flags_ |= OPTION_C; 
      flags_ |= OPTION_P; 
      break;             // cleansed mode
    default  : 
      return -1; // error
    }

  return 0;
}

short ExeUtilDisplayExplain::setOptionsX()
{
  Int32 numOptions = 0;
  if (optionsStr_)
    {
      if (strlen(optionsStr_) == 0)
        return -1; // error, cannot be empty string

      for (Int32 i = 0; i < strlen(optionsStr_); i++)
        {
          if (setOptionX(optionsStr_[i], numOptions))
            return -1;
        }
    }

  // nothing specified, set to normal full explain
  if (numOptions == 0)
    setOptionX('n', numOptions);

  if (numOptions > 1)
    return -1; // only one option can be specified

  if ((CmpCommon::getDefault(EXPLAIN_OPTION_C) == DF_ON) &&
      (isOptionN() || isOptionF()))
    setOptionX('c', numOptions);

  if ((isOptionC()) && (isOptionE() || isOptionM()))
    return -1; // 'c' can only be specified with 'n' or 'f'
  
  return 0;
}

RelExpr * ExeUtilDisplayExplain::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  RelExpr * boundExpr = NULL;

  if (setOptionsX())
    {
      NAString errStr("'");
      errStr += optionsStr_;
      errStr += "'";
      *CmpCommon::diags() << DgSqlCode(-15517)
                          << DgString0(errStr);
      bindWA->setErrStatus();
      return NULL;
    }

  boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  return boundExpr;
}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilDisplayExplainComplex
// -----------------------------------------------------------------------
ExeUtilDisplayExplainComplex::ExeUtilDisplayExplainComplex(
     char * stmtText,
     CharInfo::CharSet stmtTextCharSet,
     char * optionsStr,
     ExprNode * exprNode,
     CollHeap *oHeap)
     : ExeUtilDisplayExplain(DISPLAY_EXPLAIN_COMPLEX_,
			     stmtText, stmtTextCharSet,
			     NULL, NULL,
			     optionsStr,
			     exprNode,
			     oHeap)
{
  isVolatile_ = FALSE;
}

RelExpr * ExeUtilDisplayExplainComplex::copyTopNode(RelExpr *derivedNode, 
						    CollHeap* outHeap)
{
  ExeUtilDisplayExplainComplex *result;

  result = new (outHeap) ExeUtilDisplayExplainComplex(NULL, CharInfo::UnknownCharSet,
						      NULL, getExprNode(),
						      outHeap);

  result->type_ = type_;
  result->qry1_ = qry1_;
  result->qry2_ = qry2_;
  result->qry3_ = qry3_;
  result->qry4_ = qry4_;

  result->objectName_ = objectName_;
  result->isVolatile_ = isVolatile_;

  return ExeUtilDisplayExplain::copyTopNode(result, outHeap);
}

static short temptemp()
{
  return 0;
}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilMaintainObject
// -----------------------------------------------------------------------
ExeUtilMaintainObject::ExeUtilMaintainObject(
     enum MaintainObjectType type,
     const CorrName &name,
     QualNamePtrList * multiTablesNames,
     NAList<MaintainObjectOption*> * maintainObjectOptionsList,
     CollHeap *oHeap)
     : ExeUtilExpr(MAINTAIN_OBJECT_, name, NULL, NULL,
                   (char *)NULL,              // char *            stmtText
                   CharInfo::UnknownCharSet,  // CharInfo:CharSet  stmtTextCharSet
                   oHeap),
       type_(type),
       all_(FALSE),
       reorgTable_(FALSE),
       reorgIndex_(FALSE),
       updStatsTable_(FALSE),
       updStatsMvlog_(FALSE),
       updStatsMvs_(FALSE),
       updStatsMvgroup_(FALSE),
       updStatsAllMvs_(FALSE),
       refreshAllMvgroup_(FALSE),
       refreshMvgroup_(FALSE),
       refreshAllMvs_(FALSE),
       refreshMvs_(FALSE),
       reorgMvgroup_(FALSE),
       reorgMvs_(FALSE),
       reorgMvsIndex_(FALSE),
       reorg_(FALSE),
       refresh_(FALSE),
       cleanMaintainCIT_(FALSE),
       getLabelStats_(FALSE),
       getTableLabelStats_(FALSE),
       getIndexLabelStats_(FALSE),
       getLabelStatsIncIndexes_(FALSE),
       getLabelStatsIncInternal_(FALSE),
       getLabelStatsIncRelated_(FALSE),
       getSchemaLabelStats_(FALSE),
       reorgTableOptions_(oHeap),
       reorgIndexOptions_(oHeap),
       updStatsTableOptions_(oHeap),
       updStatsMvlogOptions_(oHeap),
       updStatsMvsOptions_(oHeap),
       updStatsMvgroupOptions_(oHeap),
       refreshMvgroupOptions_(oHeap),
       refreshMvsOptions_(oHeap),
       reorgMvgroupOptions_(oHeap),
       reorgMvsOptions_(oHeap),
       reorgMvsIndexOptions_(oHeap),
       cleanMaintainCITOptions_(oHeap),
       disableUpdStatsTable_(FALSE),
       disableUpdStatsMvs_(FALSE),
       disableRefreshMvs_(FALSE),
       disableReorgMvs_(FALSE),
       disableReorgTable_(FALSE),
       disableReorgIndex_(FALSE),
       enableUpdStatsTable_(FALSE),
       enableUpdStatsMvs_(FALSE),
       enableRefreshMvs_(FALSE),
       enableReorgMvs_(FALSE),
       enableReorgTable_(FALSE),
       enableReorgIndex_(FALSE),
       resetReorgTable_(FALSE),
       resetUpdStatsTable_(FALSE),
       resetUpdStatsMvs_(FALSE),
       resetRefreshMvs_(FALSE),
       resetReorgMvs_(FALSE),
       enableUpdStatsMvlog_(FALSE),
       disableUpdStatsMvlog_(FALSE),
       resetUpdStatsMvlog_(FALSE),
       resetReorgIndex_(FALSE),
       enableReorgMvsIndex_(FALSE),
       disableReorgMvsIndex_(FALSE),
       resetReorgMvsIndex_(FALSE),
       disableReorgMvgroup_(FALSE),
       enableReorgMvgroup_(FALSE),
       resetReorgMvgroup_(FALSE),
       disableRefreshMvgroup_(FALSE),
       enableRefreshMvgroup_(FALSE),
       resetRefreshMvgroup_(FALSE),
       disableUpdStatsMvgroup_(FALSE),
       enableUpdStatsMvgroup_(FALSE),
       resetUpdStatsMvgroup_(FALSE),
       enableTableLabelStats_(FALSE),
       disableTableLabelStats_(FALSE),
       resetTableLabelStats_(FALSE),
       enableIndexLabelStats_(FALSE),
       disableIndexLabelStats_(FALSE),
       resetIndexLabelStats_(FALSE),
       enable_(FALSE),
       disable_(FALSE),
       reset_(FALSE),
       continueOnError_(TRUE),
       returnSummary_(FALSE),
       returnDetailOutput_(FALSE),
       noOutput_(FALSE),
       display_(FALSE),
       displayDetail_(FALSE),
       doTheSpecifiedTask_(FALSE),
       errorInParams_(FALSE),
       getStatus_(FALSE),
       getDetails_(FALSE),
       initialize_(FALSE),
       reinitialize_(FALSE),
       drop_(FALSE),
       createView_(FALSE),
       dropView_(FALSE),
       maintainedTableCreateTime_(0),
       parentTableObjectUID_(0),
       parentTableName_(oHeap),
       parentTableNameLen_(0),
       shortFormat_(FALSE),
       longFormat_(FALSE),
       detailFormat_(FALSE),
       tokenFormat_(FALSE),
       commandFormat_(FALSE),
       ifNeeded_(FALSE),
       run_(FALSE),
       runFrom_(0),
       runTo_(0),
       mvLogTable_(NULL),
       maxTables_(100)
{
//  temptemp();

  NABoolean explicitTask = FALSE;

  NABoolean continueOnErrorSpecified = FALSE;

  NABoolean maintainOptionsSpecified = FALSE;
  NABoolean controlOptionsSpecified = FALSE;

  if (maintainObjectOptionsList)
    {
      for (CollIndex i = 0; i < maintainObjectOptionsList->entries(); i++)
	{
	  MaintainObjectOption * mto = (*maintainObjectOptionsList)[i];
	  switch (mto->option_)
	    {
	    case ALL_:
	      {
		if (all_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		all_ = TRUE;

		maintainOptionsSpecified = TRUE;
	      }
	    break;

	    case UPD_STATS_TABLE_:
	      {
		if (updStatsTable_ || updStatsMvs_ || updStatsMvgroup_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		explicitTask = TRUE;

		if (type == MV_)
		  {
		    updStatsMvs_ = TRUE;

		    if (mto->stringVal1_)
		      {
		        updStatsMvsOptions_ = " ";
		        updStatsMvsOptions_ += mto->stringVal1_;
		      }
 		  }
		else if (type == MVGROUP_)
 		  {
                    errorInParams_ = TRUE;
                    return;

                    /*
 		    updStatsMvgroup_ = TRUE;   //Comment out until truly supported

 		    if (mto->stringVal1_)
 		      {
 		        updStatsMvgroupOptions_ = " ";
  		        updStatsMvgroupOptions_ += mto->stringVal1_;
 		      }
                      */
 		  }
 		else
 		  {
 		    updStatsTable_ = TRUE;

 		    if (mto->stringVal1_)
 		      {
 		        updStatsTableOptions_ = " ";
 		        updStatsTableOptions_ += mto->stringVal1_;
 		      }
  		  }

 		  maintainOptionsSpecified = TRUE;

 	      }
	    break;

	    case UPD_STATS_MVLOG_:
	      {
	        if (updStatsMvlog_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		 explicitTask = TRUE;

		 updStatsMvlog_ = TRUE;

		 if (mto->stringVal1_)
		   {
		     updStatsMvlogOptions_ = " ";
		     updStatsMvlogOptions_ += mto->stringVal1_;
		   }

		 maintainOptionsSpecified = TRUE;

	      }
	    break;

	    case UPD_STATS_MVS_:
	      {
		if (updStatsMvs_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		explicitTask = TRUE;

		updStatsMvs_ = TRUE;

		if (mto->stringVal1_)
	          {
		    updStatsMvsOptions_ = " ";
		    updStatsMvsOptions_ += mto->stringVal1_;
		  }

		maintainOptionsSpecified = TRUE;

	      }
	    break;

	    case UPD_STATS_MVGROUP_:
              {
                if (updStatsMvgroup_)
                  {
                    errorInParams_ = TRUE;
                    return;
                  }

                explicitTask = TRUE;

                updStatsMvgroup_ = TRUE;

               if (mto->stringVal1_)
                  {
                    updStatsMvgroupOptions_ = " ";
	            updStatsMvgroupOptions_ += mto->stringVal1_;
                  }

                maintainOptionsSpecified = TRUE;

              }
            break;

            case UPD_STATS_ALL_MVS_:
	      {
		if (updStatsAllMvs_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		explicitTask = TRUE;

		updStatsAllMvs_ = TRUE;
                updStatsMvs_ = TRUE;

		if (mto->stringVal1_)
	          {
		    updStatsMvsOptions_ = " ";
		    updStatsMvsOptions_ += mto->stringVal1_;
		  }

		maintainOptionsSpecified = TRUE;

	      }
	    break;

	    case REFRESH_MVGROUP_:
              {
	        if (refreshMvgroup_)
                  {
                    errorInParams_ = TRUE;
                    return;
                   }

	        explicitTask = TRUE;

	        refreshMvgroup_ = TRUE;

	        if (mto->stringVal1_)
                  {
                    refreshMvgroupOptions_ = " ";
                    refreshMvgroupOptions_ += mto->stringVal1_;
                   }

                maintainOptionsSpecified = TRUE;

              }
	    break;

	    case REFRESH_ALL_MVS_:
	    {
	      if ((refreshAllMvs_) ||
	          (updStatsMvlog_) ||
	          (refreshMvgroup_) ||
	          (refreshMvs_))
		    {
		      errorInParams_ = TRUE;
		      return;
		    }

	      refreshAllMvs_ = TRUE;
	      updStatsMvlog_ = TRUE;
	      refreshMvgroup_ = TRUE;
 	      refreshMvs_ = TRUE;

	      explicitTask = TRUE;

	      if (mto->stringVal1_)
	        {
	          refreshMvsOptions_ = " ";
	          refreshMvsOptions_ += mto->stringVal1_;
	        }

              if (mto->stringVal1_)
	        {
	          refreshMvgroupOptions_ = " ";
	          refreshMvgroupOptions_ += mto->stringVal1_;
	        }

	      maintainOptionsSpecified = TRUE;

	      }
	    break;

	    case REFRESH_MVS_:
	      {
		if (refreshMvs_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		explicitTask = TRUE;

		refreshMvs_ = TRUE;

		if (mto->stringVal1_)
		  {
		    refreshMvsOptions_ = " ";
		    refreshMvsOptions_ += mto->stringVal1_;
		  }

		maintainOptionsSpecified = TRUE;

	      }
	    break;

	    case REFRESH_:
	      {
		if (refresh_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		refresh_ = TRUE;

		explicitTask = TRUE;

		if (mto->stringVal1_)
		  {
		    refreshMvsOptions_ = " ";
		    refreshMvsOptions_ += mto->stringVal1_;
		  }

		maintainOptionsSpecified = TRUE;

	      }
	    break;

            case CLEAN_MAINTAIN_CIT_:
	      {
		if (cleanMaintainCIT_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		explicitTask = TRUE;

		cleanMaintainCIT_ = TRUE;

		if (mto->stringVal1_)
		  {
		    cleanMaintainCITOptions_ = " ";
		    cleanMaintainCITOptions_ += mto->stringVal1_;
		  }

		maintainOptionsSpecified = TRUE;

	      }
	    break;


	    case DISABLE_:
	      {
		if (enable_ || disable_ || reset_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		disable_ = TRUE;

		//explicitTask = TRUE;

		controlOptionsSpecified = TRUE;
	      }
	    break;

	    case ENABLE_:
	      {
		if (enable_ || disable_ || reset_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		enable_ = TRUE;

		//explicitTask = TRUE;

		controlOptionsSpecified = TRUE;
	      }
	    break;

	    case RESET_:
	      {
		if (enable_ || disable_ || reset_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		reset_ = TRUE;

		//explicitTask = TRUE;

		controlOptionsSpecified = TRUE;
	      }
	    break;

	    case CONTINUE_ON_ERROR_:
	      {
		if (continueOnErrorSpecified)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		continueOnErrorSpecified = TRUE;

		if (mto->numericVal1_ != 0)
		  continueOnError_ = TRUE;
		else
		  continueOnError_ = FALSE;
	      }
	    break;

	    case RETURN_SUMMARY_:
	      {
		if (returnSummary_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		if (mto->stringVal1_)
		  {
		    statusSummaryOptionsStr_ = mto->stringVal1_;
		  }

		returnSummary_ = TRUE;
	      }
	    break;

	    case RETURN_DETAIL_OUTPUT_:
	      {
		if (returnDetailOutput_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		returnDetailOutput_ = TRUE;
	      }
	    break;

	    case NO_OUTPUT_:
	      {
		if (noOutput_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		noOutput_ = TRUE;
	      }
	    break;

	    case DISPLAY_:
	      {
		if (display_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		display_ = TRUE;
	      }
	    break;

	    case DISPLAY_DETAIL_:
	      {
		if (displayDetail_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		displayDetail_ = TRUE;
	      }
	    break;

	    case INITIALIZE_:
	      {
		if (initialize_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		explicitTask = TRUE;

		controlOptionsSpecified = TRUE;

		initialize_ = TRUE;
	      }
	    break;

	    case REINITIALIZE_:
	      {
		if (reinitialize_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		explicitTask = TRUE;

		controlOptionsSpecified = TRUE;

		reinitialize_ = TRUE;
	      }
	    break;

	    case DROP_:
	      {
		if (drop_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		explicitTask = TRUE;

		controlOptionsSpecified = TRUE;

		drop_ = TRUE;
	      }
	    break;

	    case CREATE_VIEW_:
	      {
		if ((createView_) || (dropView_))
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		explicitTask = TRUE;

		controlOptionsSpecified = TRUE;

		createView_ = TRUE;
	      }
	    break;

	    case DROP_VIEW_:
	      {
		if ((createView_) || (dropView_))
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		explicitTask = TRUE;

		controlOptionsSpecified = TRUE;

		dropView_ = TRUE;
	      }
	    break;

	    case GET_STATUS_:
	    case GET_DETAILS_:
	      {
		if ((getStatus_) ||
		    (getDetails_))
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		if (mto->stringVal1_)
		  {
		    formatOptions_ = mto->stringVal1_;

		    formatOptions_ = formatOptions_.strip(NAString::both);
		    formatOptions_.toLower();
		    if (formatOptions_.contains(" "))
		      {
			errorInParams_ = TRUE;
			return;
		      }
		  }

		if (mto->option_ == GET_STATUS_)
		  {
		    explicitTask = TRUE;

		    getStatus_ = TRUE;
		  }
		else
		  getDetails_ = TRUE;
	      }
	    break;

	    case RUN_:
	      {
		if (run_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		run_ = TRUE;
		explicitTask = TRUE;

		if (mto->numericVal1_ > 0)
		  {
		    str_cpy_all((char*)&runFrom_,
				mto->stringVal1_,
				mto->numericVal1_);
		  }

		if (mto->numericVal2_ > 0)
		  {
		    str_cpy_all((char*)&runTo_,
				mto->stringVal2_,
				mto->numericVal2_);
		  }

		if ((runTo_ > 0) &&
		    (runFrom_ > 0) &&
		    (runTo_ < runFrom_))
		  {
		    errorInParams_ = TRUE;
		    return;
		  }
	      }
	    break;

	    case IF_NEEDED_:
	      {
		if (ifNeeded_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		ifNeeded_ = TRUE;
	      }
	    break;

	    case MAX_TABLES_:
	      {
		if (mto->numericVal1_ > 0)
		  {
		    maxTables_ = mto->numericVal1_;
		  }
		else
		  {
		    errorInParams_ = TRUE;
		    return;
		  }
	      }
	    break;

	    case GET_LABEL_STATS_:
	      {
		if (getLabelStats_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		getLabelStats_ = TRUE;
		explicitTask = TRUE;
	      }
	    break;
	    case GET_LABELSTATS_INC_INDEXES_:
	      {
		if ((getLabelStatsIncIndexes_) ||
		    (type_ ==INDEX_) ||
		    (type_ ==SCHEMA_))
		  {
		    errorInParams_ = TRUE;
		    return;
		  }
		getLabelStats_ = TRUE;
		getLabelStatsIncIndexes_ = TRUE;
		explicitTask = TRUE;
	      }
	      break;
	    case GET_LABELSTATS_INC_INTERNAL_:
	      {
		if ((getLabelStatsIncInternal_)  ||
		    (type_ ==INDEX_) ||
		    (type_ ==SCHEMA_))
		  {
		    errorInParams_ = TRUE;
		    return;
		  }
		getLabelStats_ = TRUE;
		getLabelStatsIncInternal_ = TRUE;
		explicitTask = TRUE;
	      }
	      break;
	        case GET_LABELSTATS_INC_RELATED_:
	      {
		if ((getLabelStatsIncRelated_) ||
		    (type_ ==INDEX_) ||
		    (type_ ==SCHEMA_))
		  {
		    errorInParams_ = TRUE;
		    return;
		  }
		getLabelStats_ = TRUE;
		getLabelStatsIncRelated_ = TRUE;
		explicitTask = TRUE;
	      }  
	    break;
	    default:
	      break;
	    } // switch
	} // for
    } // if

  if (noOutput_)
    {
      returnSummary_ = FALSE;
      returnDetailOutput_ = FALSE;
    }
      
  if ((all_) &&
      (explicitTask))
    {
      errorInParams_ = TRUE;
      return;
    }

  if ((NOT all_) &&
      (NOT explicitTask))
    {
      errorInParams_ = TRUE;
      return;
    }

  if (explicitTask)
    {
      doTheSpecifiedTask_ = TRUE;
    }

  if (all_)
    {
      switch (type_)
	{
	case TABLE_:
	  reorgTable_ = TRUE;
	  reorgIndex_ = TRUE;
	  updStatsTable_ = TRUE;
	  updStatsMvlog_ = TRUE;
	  updStatsMvs_ = TRUE;
//	  updStatsMvgroup_ = TRUE;
	  refreshMvgroup_ = TRUE;
//	  reorgMvgroup_ = TRUE;
	  refreshMvs_ = TRUE;
	  reorgMvs_ = TRUE;
	  reorgMvsIndex_ = TRUE;


	  if (enable_)
	    {
	      enableReorgTable_ = TRUE;
	      enableUpdStatsTable_ = TRUE;
              enableUpdStatsMvs_ = TRUE;
              enableRefreshMvs_ = TRUE;
              enableReorgMvs_ = TRUE;
              enableUpdStatsMvlog_ = TRUE;
              enableReorgMvsIndex_ = TRUE;
              enableReorgIndex_ = TRUE;
              enableRefreshMvgroup_ = TRUE;
              enableReorgMvgroup_ = TRUE;
              enableUpdStatsMvgroup_ = TRUE;
	      enableTableLabelStats_ = TRUE;
	      enableIndexLabelStats_ = TRUE;
	    }

	  if (disable_)
	    {
	      disableReorgTable_ = TRUE;
	      disableUpdStatsTable_ = TRUE;
              disableUpdStatsMvs_ = TRUE;
              disableRefreshMvs_ = TRUE;
              disableReorgMvs_ = TRUE;
              disableUpdStatsMvlog_ = TRUE;
              disableReorgMvsIndex_ = TRUE;
              disableReorgIndex_ = TRUE;
              disableRefreshMvgroup_ = TRUE;
              disableReorgMvgroup_ = TRUE;
              disableUpdStatsMvgroup_ = TRUE;
	      disableTableLabelStats_ = TRUE;
	      disableIndexLabelStats_ = TRUE;
	    }

	  if (reset_)
	    {
	      resetReorgTable_ = TRUE;
	      resetUpdStatsTable_ = TRUE;
              resetUpdStatsMvs_ = TRUE;
              resetRefreshMvs_ = TRUE;
              resetReorgMvs_ = TRUE;
              resetUpdStatsMvlog_ = TRUE;
              resetReorgMvsIndex_ = TRUE;
              resetReorgIndex_ = TRUE;
              resetRefreshMvgroup_ = TRUE;
              resetReorgMvgroup_ = TRUE;
              resetUpdStatsMvgroup_ = TRUE;
	      resetTableLabelStats_ = TRUE;
	      resetIndexLabelStats_ = TRUE;
	    }
	  break;

	case INDEX_:

	  reorgIndex_ = TRUE;

          if (enable_)
            {
              enableReorgIndex_ = TRUE;
            }

          if (disable_)
            {
              disableReorgIndex_ = TRUE;
            }

          if (reset_)
            {
              resetReorgIndex_ = TRUE;
            }

	  break;

	case MV_:

	  refresh_ = TRUE;
	  reorg_ = TRUE;
	  refreshMvs_ = TRUE;
	  reorgMvs_ = TRUE;
	  reorgMvsIndex_ = TRUE;
	  updStatsMvs_ = TRUE;

          if (enable_)
	    {
              enableRefreshMvs_ = TRUE;
              enableReorgMvs_ = TRUE;
              enableReorgMvsIndex_ = TRUE;
              enableUpdStatsMvs_ = TRUE;
  	    }

	  if (disable_)
	    {
              disableRefreshMvs_ = TRUE;
              disableReorgMvs_ = TRUE;
              disableReorgMvsIndex_ = TRUE;
              disableUpdStatsMvs_ = TRUE;
	    }

	  if (reset_)
	    {
              resetRefreshMvs_ = TRUE;
              resetReorgMvs_ = TRUE;
              resetReorgMvsIndex_ = TRUE;
              resetUpdStatsMvs_ = TRUE;
	    }
	  break;

	case MVGROUP_:

          refresh_ = TRUE;
	  refreshMvgroup_ = TRUE;
/*                                       // Placeholder for when MVGROUPs handle reorg and update stats
	  reorg_ = TRUE;
	  updStatsMvgroup_ = TRUE;
	  reorgMvgroup_ = TRUE;

          // Add MVs activity as well

          reorgMvs_ = TRUE;
          updStatsMvs_ = TRUE;
          refreshMvs_ = TRUE;
          reorgMvsIndex_ = TRUE;

         if (enable_)
	    {
              enableRefreshMvgroup_ = TRUE;
              enableRefreshMvs_ = TRUE;
              enableReorgMvs_ = TRUE;
              enableUpdStatsMvs_ = TRUE;
              enableReorgMvsIndex_ = TRUE;
              enableReorgMvgroup_ = TRUE;
              enableUpdStatsMvgroup_ = TRUE;
	    }

          if (disable_)
	    {
              disableRefreshMvgroup_ = TRUE;
              disableRefreshMvs_ = TRUE;
              disableReorgMvs_ = TRUE;
              disableUpdStatsMvs_ = TRUE;
              disableReorgMvgroup_ = TRUE;
              disableUpdStatsMvgroup_ = TRUE;
              disableReorgMvsIndex_ = TRUE;
	    }

          if (reset_)
	    {
              resetRefreshMvgroup_ = TRUE;
              resetRefreshMvs_ = TRUE;
              resetReorgMvs_ = TRUE;
              resetUpdStatsMvs_ = TRUE;
              resetReorgMvgroup_ = TRUE;
              resetUpdStatsMvgroup_ = TRUE;
              resetReorgMvsIndex_ = TRUE;

              }
*/

     // Continue to disable the enable, disable, reset of the MVGROUP
      if ( enable_ || disable_ || reset_)
        {
           errorInParams_ = TRUE;
	   return;
        }
	  break;

	case MV_INDEX_:

	  reorgMvsIndex_ = TRUE;

          if (enable_)
            enableReorgMvsIndex_ = TRUE;

	  if (disable_)
            disableReorgMvsIndex_ = TRUE;

	  if (reset_)
            resetReorgMvsIndex_ = TRUE;

	  break;

	case CLEAN_MAINTAIN_:
	  cleanMaintainCIT_ = TRUE;
	  break;

	}
    }

  switch (type_)
    {
    case TABLE_:
      {
	if (reorg_)
	  {
	    if (NOT reorgTable_)
	      reorgTable_ = reorg_;
	  }
       
	if (getLabelStats_)
	  {
	    if (NOT getTableLabelStats_)
	      getTableLabelStats_ = getLabelStats_;
	  }
	
	if (refresh_)
	  {
	    errorInParams_ = TRUE;
	    return;
	  }

	if (enable_)
	  {
	    reorgIndex_ = TRUE;

	    if (reorgTable_)
	      enableReorgTable_ = enable_;

	    if (updStatsTable_)
	      enableUpdStatsTable_ = enable_;

            if (refreshMvs_)
              enableRefreshMvs_ = enable_;

            if (updStatsMvs_)
              enableUpdStatsMvs_ = enable_;

            if (reorgMvs_)
              enableReorgMvs_ = enable_;

            if (updStatsMvlog_)
              enableUpdStatsMvlog_ = enable_;

            if (reorgIndex_)
              enableReorgIndex_ = enable_;

            if (reorgMvsIndex_)
              enableReorgMvsIndex_ = enable_;
	    if (getLabelStats_)
	      enableTableLabelStats_ = enable_;
	  }

	if (disable_)
	  {
	    reorgIndex_ = TRUE;

	    if (reorgTable_)
	      disableReorgTable_ = disable_;

	    if (updStatsTable_)
	      disableUpdStatsTable_ = disable_;

            if (refreshMvs_)
              disableRefreshMvs_ = disable_;

            if (updStatsMvs_)
              disableUpdStatsMvs_ = disable_;

            if (reorgMvs_)
              disableReorgMvs_ = disable_;

            if (updStatsMvlog_)
              disableUpdStatsMvlog_ = disable_;

            if (reorgIndex_)
              disableReorgIndex_ = disable_;

            if (reorgMvsIndex_)
              disableReorgMvsIndex_ = disable_;

	    if (getLabelStats_)
	      disableTableLabelStats_ = disable_;
	  }

	if (reset_)
	  {
	    reorgIndex_ = TRUE;

	    if (reorgTable_)
	      resetReorgTable_ = reset_;

	    if (updStatsTable_)
	      resetUpdStatsTable_ = reset_;

            if (refreshMvs_)
              resetRefreshMvs_ = reset_;

            if (updStatsMvs_)
              resetUpdStatsMvs_ = reset_;

            if (reorgMvs_)
              resetReorgMvs_ = reset_;

            if (updStatsMvlog_)
              resetUpdStatsMvlog_ = reset_;

            if (reorgIndex_)
              resetReorgIndex_ = reset_;

            if (reorgMvsIndex_)
              resetReorgMvsIndex_ = reset_;

	    if (getLabelStats_)
	      resetTableLabelStats_ = reset_;
	  }

        }
      break;

    case INDEX_:

      if ((reorgTable_)     ||
          (updStatsTable_)  || (updStatsMvlog_)   ||
          (updStatsMvs_)    || (updStatsMvgroup_) ||
	  (refresh_)        || (refreshMvs_)      || (refreshMvgroup_) ||
	  (reorgMvs_)       || (reorgMvgroup_)    || (reorgMvsIndex_)  ||
          (cleanMaintainCIT_))
	{
	  errorInParams_ = TRUE;
	  return;
	}

      if (reorg_)
	{
	  if (NOT reorgIndex_)
	    reorgIndex_ = reorg_;

	  if (reorgTableOptions_)
	    {
	      reorgIndexOptions_ = reorgTableOptions_;
	      reorgTableOptions_ = "";
	    }
	 }
      if (getLabelStats_)
	{
	  if (NOT getIndexLabelStats_)
	    getIndexLabelStats_ = getLabelStats_;
	}

      if (enable_)
        enableReorgIndex_ = enable_;

      if (disable_)
        disableReorgIndex_ = disable_;

      if (reset_)
        resetReorgIndex_ = reset_;


      break;

    case MV_:
      if ((reorgTable_)      ||
	  (updStatsTable_)   ||
          (updStatsMvgroup_) ||
	  (refreshMvgroup_)  ||
	  (reorgMvgroup_)    ||
	  (cleanMaintainCIT_))
	{
	  errorInParams_ = TRUE;
	  return;
	}
      
      // REORG ALL MVS for an MV is not supported
      if (reorgMvs_ && !all_)
       {
	  errorInParams_ = TRUE;
	  return;
       }

      // UPDATE STATISTICS ALL MVS for an MV is not supported
      if (updStatsAllMvs_)
       {
	  errorInParams_ = TRUE;
	  return;
       }
      if (getLabelStats_)
	{
	  if (NOT getTableLabelStats_)
	    getTableLabelStats_ = getLabelStats_;
	} 
      if (enable_)
	{
	  if (updStatsMvs_)
	    enableUpdStatsMvs_ = enable_;

          if (refresh_)
            enableRefreshMvs_ = enable_;

          if (reorg_)
            enableReorgMvs_ = enable_;

          if (reorgMvsIndex_)
            enableReorgMvsIndex_ = enable_;
	  
	}

      if (disable_)
	{
	  if (updStatsMvs_)
	    disableUpdStatsMvs_ = disable_;

          if (refresh_)
	    disableRefreshMvs_ = disable_;

          if (reorg_)
	    disableReorgMvs_ = disable_;

          if (reorgMvsIndex_)
            disableReorgMvsIndex_ = disable_;
	}

	if (reset_)
	  {
	    if (updStatsMvs_)
	      resetUpdStatsMvs_ = reset_;

            if (refresh_)
	      resetRefreshMvs_ = reset_;

            if (reorg_)
	      resetReorgMvs_ = reset_;

            if (reorgMvsIndex_)
              resetReorgMvsIndex_ = reset_;
	  }

	if (reorg_)
	  {
	    if (NOT reorgMvs_)
	      reorgMvs_ = reorg_;

	    if (reorgTableOptions_)
	      {
		    reorgMvsOptions_ = reorgTableOptions_;
		    reorgTableOptions_ = "";
	      }

	  }

	if (refresh_)
	  {
	    if (NOT refreshMvs_)
	      refreshMvs_ = refresh_;
	  }

	if (reorgIndex_)
	  {
	    reorgMvsIndex_ = reorgIndex_;
	    reorgIndex_ = FALSE;

            if (reorgIndexOptions_)
	    {
	      reorgMvsIndexOptions_ = reorgIndexOptions_;
	      reorgIndexOptions_ = "";
	    }
	  }

      break;

    case MVGROUP_:

      if ((reorgTable_) || (reorgIndex_) ||
	  (updStatsTable_) || (cleanMaintainCIT_) ||
          (refreshAllMvs_))                                // For now disable UPDSTATS_MVGROUP
	{
	  errorInParams_ = TRUE;
	  return;
	}
      if (updStatsMvgroup_)
	{
	  if (updStatsMvsOptions_)
	    {
	      updStatsMvgroupOptions_ = updStatsMvsOptions_;
	      updStatsMvsOptions_ = "";
	    }
	}

      if (refresh_)
	{
	  if (NOT refreshMvgroup_)
	    refreshMvgroup_ = refresh_;

	  if (refreshMvsOptions_)
	    {
	      refreshMvgroupOptions_ = refreshMvsOptions_;
	      refreshMvsOptions_ = "";
	    }
	}

      if (reorg_)
	{
	  if (NOT reorgMvgroup_)
	    reorgMvgroup_ = reorg_;

	  if (reorgMvsOptions_)
	    {
	      reorgMvgroupOptions_ = reorgMvsOptions_;
	      reorgMvsOptions_ = "";
	    }
	}

      // Continue to disable the enable, disable, reset of the MVGROUP
      if ( enable_ || disable_ || reset_)
        {
           errorInParams_ = TRUE;
	   return;
        }

      if (enable_)
	{
	  if (updStatsMvgroup_)
            enableUpdStatsMvgroup_ = TRUE;

          if (refreshMvgroup_)
            enableRefreshMvgroup_ = TRUE;

          if (reorgMvgroup_)
            enableReorgMvgroup_ = TRUE;
	}

      if (disable_)
	{
	  if (updStatsMvgroup_)
            disableUpdStatsMvgroup_ = TRUE;

          if (refreshMvgroup_)
            disableRefreshMvgroup_ = TRUE;

          if (reorgMvgroup_)
            disableReorgMvgroup_ = TRUE;
	}

      if (reset_)
	{
	  if (updStatsMvgroup_)
            resetUpdStatsMvgroup_ = TRUE;

          if (refreshMvgroup_)
            resetRefreshMvgroup_ = TRUE;

          if (reorgMvgroup_)
            resetReorgMvgroup_ = TRUE;
	}

      break;

    case MV_INDEX_:
      if ((reorgTable_) || (updStatsTable_) || (updStatsMvlog_) ||
          (updStatsMvs_) || (updStatsMvgroup_) ||
	  (refreshMvgroup_) || (refreshMvs_) ||
	  (reorgMvgroup_) || (reorgMvs_) ||
	  (reorgIndex_) || (cleanMaintainCIT_))
	{
	  errorInParams_ = TRUE;
	  return;
	}

      if (enable_)
        enableReorgMvsIndex_ = enable_;

      if (disable_)
        disableReorgMvsIndex_ = disable_;

      if (reset_)
        resetReorgMvsIndex_ = reset_;

      if (reorg_)
	{
	  if (NOT reorgMvsIndex_)
	    reorgMvsIndex_ = reorg_;

	  if (reorgTableOptions_)
	    {
	      reorgMvsIndexOptions_ = reorgTableOptions_;
	      reorgTableOptions_ = "";
	    }

	}
      break;

    case TABLES_:
      {
	if ((all_) ||
	    (updStatsTable_) ||
	    (updStatsMvlog_) ||
	    (updStatsMvs_) || (updStatsMvgroup_) ||
	    (refreshMvgroup_) || (refreshMvs_) ||
	    (reorgMvgroup_) || (reorgMvs_) ||
	    (reorgIndex_) || (cleanMaintainCIT_) ||
	    (enable_ || disable_ || reset_) ||
	    (run_ || getStatus_ || getDetails_))
	  {
	    errorInParams_ = TRUE;
	    return;
	  }

	if (reorg_)
	  {
	    reorgTable_ = reorg_;
	  }

      }
    break;
    case SCHEMA_:
      {
	if (!getLabelStats_)
//	    (NOT reorg_))
	  {
	    errorInParams_ = TRUE;
	    return;
	  }

	if (getLabelStats_)
	  getSchemaLabelStats_ = getLabelStats_;
      }
    } // switch
  
  // if displayOutput is NOT set, set options to reorg to not
  // return status.
  if ((NOT returnSummary_) && (NOT returnDetailOutput_))
    {
      if (reorgTable_)
	reorgTableOptions_ += ", no output";

      if (reorgIndex_)
	reorgIndexOptions_ += ", no output";

      if (reorgMvs_)
	reorgMvsOptions_ += ", no output";

      if (reorgMvsIndex_)
	reorgMvsIndexOptions_ += ", no output";
    }
  else
    {
      if (returnSummary_)
	{
	  if (reorgTable_)
	    reorgTableOptions_ += ", return summary"
	      + (NOT statusSummaryOptionsStr_.isNull() 
		 ? (" options '" + statusSummaryOptionsStr_ + "'") : "");

	  if (reorgIndex_)
	    reorgIndexOptions_ += ", return summary"
	      + (NOT statusSummaryOptionsStr_.isNull() 
		 ? (" options '" + statusSummaryOptionsStr_ + "'") : "");
	  
	  if (reorgMvs_)
	    reorgMvsOptions_ += ", return summary"
	      + (NOT statusSummaryOptionsStr_.isNull() 
		 ? (" options '" + statusSummaryOptionsStr_ + "'") : "");
	  
	  if (reorgMvsIndex_)
	    reorgMvsIndexOptions_ += ", return summary"
	      + (NOT statusSummaryOptionsStr_.isNull() 
		 ? (" options '" + statusSummaryOptionsStr_ + "'") : "");
	}

      if (returnDetailOutput_)
	{
	  if (reorgTable_)
	    reorgTableOptions_ += ", return detail output";
	  
	  if (reorgIndex_)
	    reorgIndexOptions_ += ", return detail output";
	  
	  if (reorgMvs_)
	    reorgMvsOptions_ += ", return detail output";
	  
	  if (reorgMvsIndex_)
	    reorgMvsIndexOptions_ += ", return detail output";
	}
    }

  if ((getStatus_) || (getDetails_))
    {
      if (getStatus_)
      	{
	  if ((all_) ||
	      (updStatsMvlog_) ||
	      (updStatsMvgroup_) ||
	      (refreshMvgroup_) || (refreshMvs_) ||
	      (reorgMvgroup_) ||
	      (reorgIndex_) || (cleanMaintainCIT_) ||
	      (enable_ || disable_ || reset_))
	    {
	      errorInParams_ = TRUE;
	      return;
	    }
	}

      if ((NOT reorg_) && (NOT updStatsTable_) && (NOT updStatsMvs_))
	{
	  if (getDetails_)
	    {
	      reorg_ = TRUE;
	      updStatsTable_ = TRUE;
	    }
	  else
	    {
	      errorInParams_ = TRUE;
	      return;
	    }
	}

      if (reorg_)
	{
	  if (type_ == TABLE_)
	    reorgTable_ = reorg_;
	  else
	    reorgMvs_ = reorg_;
	}

      if (formatOptions_)
	{
	  size_t currIndex = 0;
	  Lng32 numFormats = 0;
	  while (currIndex < formatOptions_.length())
	    {
	      if ((formatOptions_.length() - currIndex) < 2)
		{
		  errorInParams_ = TRUE;
		  return;
		}

	      NAString option = formatOptions_(currIndex, 2);
	      option.strip(NAString::both);
	      if (option.isNull())
		{
		  errorInParams_ = TRUE;
		  return;
		}

	      if (option == "sf")
		{
		  shortFormat_ = TRUE;
		  numFormats++;
		}
	      else if (option == "lf")
		{
		  longFormat_ = TRUE;
		  numFormats++;
		}
	      else if (option == "df")
		{
		  detailFormat_ = TRUE;
		  numFormats++;
		}
	      else if (option == "tf")
		{
		  tokenFormat_ = TRUE;
		  numFormats++;
		}
	      else if (option == "cf")
		{
		  commandFormat_ = TRUE;
		  numFormats++;
		}
	      else
		{
		  errorInParams_ = TRUE;
		  return;
		}

	      currIndex += 3;
	    } // while

	  if (numFormats > 1)
	    {
	      errorInParams_ = TRUE;
	      return;
	    }
	} // formatOptions

      if (getDetails_)
	{
	  if ((longFormat_) || (detailFormat_) || (commandFormat_))
	    {
	      errorInParams_ = TRUE;
	      return;
	    }
	}

    } // getStatus || getDetails
  else if ((NOT run_) && (NOT getSchemaLabelStats_) &&
	   ((type_ == CATALOG_) ||
	    (type_ == SCHEMA_) ||
	    (type_ == DATABASE_)))
    {
      errorInParams_ = TRUE;
      return;
    }

  if (run_)
    {
      if (reorg_)
	{
	  reorgTable_ = reorg_;
	}

      // to run, we need the command format. That will cause maintain
      // commands to be generated at runtime.
      getStatus_ = TRUE;
      shortFormat_ = FALSE;
      longFormat_ = FALSE;
      detailFormat_ = FALSE;
      tokenFormat_ = FALSE;
      commandFormat_ = TRUE;
    }
  else if (ifNeeded_)
    {
      // ifNeeded can only be specified with 'run' option.
      errorInParams_ = TRUE;
      return;
    }

  // these options are allowed but are not yet supported.
  switch (type_)
    {
    case TABLE_:
      {
      }
    break;

    // HERE Needed until MVGROUP support for reorg and updstats
    case MVGROUP_:
      {
	if ((reorg_) ||
	    (reorgMvs_) ||
            (updStatsMvs_) ||
	    (reorgMvsIndex_))
	  {
	    errorInParams_ = TRUE;
	    return;
	  }
      }
    break;

    default:
      break;
    }

  if (multiTablesNames)
    multiTablesNames_ = *multiTablesNames;

}

RelExpr * ExeUtilMaintainObject::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilMaintainObject *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilMaintainObject(type_,
						 getTableName(),
						 NULL,
						 NULL,
						 outHeap);
  else
    result = (ExeUtilMaintainObject *) derivedNode;

  result->setParams(all_, reorgTable_, reorgIndex_,
		    updStatsTable_, updStatsMvlog_,
		    updStatsMvs_, updStatsMvgroup_,
                    updStatsAllMvs_,
		    refreshAllMvgroup_, refreshMvgroup_,
		    refreshAllMvs_, refreshMvs_,
		    reorgMvgroup_,
		    reorgMvs_, reorgMvsIndex_,
		    cleanMaintainCIT_,
		    continueOnError_, 
		    returnSummary_, returnDetailOutput_,
		    noOutput_,
		    display_, displayDetail_,
		    getSchemaLabelStats_,
		    getLabelStats_,
		    getTableLabelStats_, getIndexLabelStats_, 
		    getLabelStatsIncIndexes_,getLabelStatsIncInternal_,
		    getLabelStatsIncRelated_);

  result->setOptionsParams(reorgTableOptions_, reorgIndexOptions_,
			   updStatsTableOptions_, updStatsMvlogOptions_,
			   updStatsMvsOptions_, updStatsMvgroupOptions_,
			   refreshMvgroupOptions_, refreshMvsOptions_,
			   reorgMvgroupOptions_, reorgMvsOptions_,
			   reorgMvsIndexOptions_,
			   cleanMaintainCITOptions_);

  result->setControlParams(disableReorgTable_, enableReorgTable_,
			   disableReorgIndex_, enableReorgIndex_,
			   disableUpdStatsTable_, enableUpdStatsTable_,
			   disableUpdStatsMvs_, enableUpdStatsMvs_,
			   disableRefreshMvs_, enableRefreshMvs_,
			   disableReorgMvs_, enableReorgMvs_,
			   resetReorgTable_, resetUpdStatsTable_,
			   resetUpdStatsMvs_, resetRefreshMvs_,
			   resetReorgMvs_, resetReorgIndex_,
			   enableUpdStatsMvlog_, disableUpdStatsMvlog_,
			   resetUpdStatsMvlog_, enableReorgMvsIndex_,
			   disableReorgMvsIndex_, resetReorgMvsIndex_,
			   enableRefreshMvgroup_, disableRefreshMvgroup_,
			   resetRefreshMvgroup_, enableReorgMvgroup_,
			   disableReorgMvgroup_, resetReorgMvgroup_,
			   enableUpdStatsMvgroup_, disableUpdStatsMvgroup_,
			   resetUpdStatsMvgroup_,enableTableLabelStats_,
			   disableTableLabelStats_,resetTableLabelStats_,
			   enableIndexLabelStats_,disableIndexLabelStats_,
			   resetIndexLabelStats_);

  result->type_ = type_;

  result->doTheSpecifiedTask_ = doTheSpecifiedTask_;

  result->errorInParams_ = errorInParams_;
  result->maintainedTableCreateTime_ = maintainedTableCreateTime_;
  result->parentTableObjectUID_ = parentTableObjectUID_;
  result->parentTableNameLen_ = parentTableNameLen_;
  result->parentTableName_ = parentTableName_;

  result->getStatus_ = getStatus_;
  result->getDetails_ = getDetails_;
  result->shortFormat_ = shortFormat_;
  result->longFormat_ = longFormat_;
  result->detailFormat_ = detailFormat_;
  result->tokenFormat_ = tokenFormat_;
  result->commandFormat_ = commandFormat_;

  result->initialize_ = initialize_;
  result->reinitialize_ = reinitialize_;
  result->drop_ = drop_;
  result->createView_ = createView_;
  result->dropView_ = dropView_;

  result->formatOptions_ = formatOptions_;

  result->statusSummaryOptionsStr_ = statusSummaryOptionsStr_;

  result->run_ = run_;
  result->runFrom_ = runFrom_;
  result->runTo_ = runTo_;
  result->ifNeeded_ = ifNeeded_;

  result->maxTables_ = maxTables_;

  result->multiTablesNames_ = multiTablesNames_;
  result->multiTablesDescs_ = multiTablesDescs_;

  result->skippedMultiTablesNames_ = skippedMultiTablesNames_;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

void ExeUtilMaintainObject::setOptionsParams(const NAString &reorgTableOptions,
					     const NAString &reorgIndexOptions,
					     const NAString &updStatsTableOptions,
					     const NAString &updStatsMvlogOptions,
					     const NAString &updStatsMvsOptions,
					     const NAString &updStatsMvgroupOptions,
					     const NAString &refreshMvgroupOptions,
					     const NAString &refreshMvsOptions,
					     const NAString &reorgMvgroupOptions,
					     const NAString &reorgMvsOptions,
					     const NAString &reorgMvsIndexOptions,
					     const NAString &cleanMaintainCITOptions)
{
  reorgTableOptions_    = reorgTableOptions;
  reorgIndexOptions_    = reorgIndexOptions;
  updStatsTableOptions_ = updStatsTableOptions;
  updStatsMvlogOptions_ = updStatsMvlogOptions;
  updStatsMvsOptions_ = updStatsMvsOptions;
  updStatsMvgroupOptions_ = updStatsMvgroupOptions;
  refreshMvgroupOptions_    = refreshMvgroupOptions;
  refreshMvsOptions_    = refreshMvsOptions;
  reorgMvgroupOptions_      = reorgMvgroupOptions;
  reorgMvsOptions_      = reorgMvsOptions;
  reorgMvsIndexOptions_    = reorgMvsIndexOptions;
  cleanMaintainCITOptions_ = cleanMaintainCITOptions;
}

void ExeUtilMaintainObject::setParams(NABoolean all,
				      NABoolean reorgTable,
				      NABoolean reorgIndex,
				      NABoolean updStatsTable,
				      NABoolean updStatsMvlog,
 				      NABoolean updStatsMvs,
				      NABoolean updStatsMvgroup,
                                      NABoolean updStatsAllMvs,
 				      NABoolean refreshAllMvgroup,
				      NABoolean refreshMvgroup,
				      NABoolean refreshAllMvs,
				      NABoolean refreshMvs,
				      NABoolean reorgMvgroup,
				      NABoolean reorgMvs,
				      NABoolean reorgMvsIndex,
				      NABoolean cleanMaintainCIT,
				      NABoolean continueOnError,
				      NABoolean returnSummary,
				      NABoolean returnDetailOutput,
				      NABoolean noOutput,
				      NABoolean display,
				      NABoolean displayDetail,
				      NABoolean getSchemaLabelStats,
				      NABoolean getLabelStats,
				      NABoolean getTableLabelStats,
				      NABoolean getIndexLabelStats,
				      NABoolean getLabelStatsIncIndexes,
				      NABoolean getLabelStatsIncInternal,
				      NABoolean getLabelStatsIncRelated
				      
				      )
{
  all_           = all;
  reorgTable_    = reorgTable;
  reorgIndex_    = reorgIndex;
  updStatsTable_ = updStatsTable;
  updStatsMvlog_ = updStatsMvlog;
  updStatsMvs_ = updStatsMvs;
  updStatsMvgroup_ = updStatsMvgroup;
  updStatsAllMvs_ = updStatsAllMvs;
  refreshAllMvgroup_ = refreshAllMvgroup;
  refreshMvgroup_    = refreshMvgroup;
  refreshAllMvs_ = refreshAllMvs;
  refreshMvs_    = refreshMvs;
  reorgMvgroup_ = reorgMvgroup_;
  reorgMvs_      = reorgMvs;
  reorgMvsIndex_      = reorgMvsIndex;
  cleanMaintainCIT_   = cleanMaintainCIT;
  continueOnError_   = continueOnError;
  returnDetailOutput_ = returnDetailOutput;
  returnSummary_ = returnSummary;
  noOutput_ = noOutput;
  display_ = display;
  displayDetail_ = displayDetail;
  getLabelStats_ = getLabelStats;
  getIndexLabelStats_ = getIndexLabelStats;
  getLabelStatsIncIndexes_ = getLabelStatsIncIndexes;
  getLabelStatsIncInternal_ = getLabelStatsIncInternal;
  getLabelStatsIncRelated_ = getLabelStatsIncRelated;
  getSchemaLabelStats_ = getSchemaLabelStats;
}

void ExeUtilMaintainObject::setControlParams(
     NABoolean disableReorgTable,
     NABoolean enableReorgTable,
     NABoolean disableReorgIndex,
     NABoolean enableReorgIndex,
     NABoolean disableUpdStatsTable,
     NABoolean enableUpdStatsTable,
     NABoolean disableUpdStatsMvs,
     NABoolean enableUpdStatsMvs,
     NABoolean disableRefreshMvs,
     NABoolean enableRefreshMvs,
     NABoolean disableReorgMvs,
     NABoolean enableReorgMvs,
     NABoolean resetReorgTable,
     NABoolean resetUpdStatsTable,
     NABoolean resetUpdStatsMvs,
     NABoolean resetRefreshMvs,
     NABoolean resetReorgMvs,
     NABoolean resetReorgIndex,
     NABoolean enableUpdStatsMvlog,
     NABoolean disableUpdStatsMvlog,
     NABoolean resetUpdStatsMvlog,
     NABoolean enableReorgMvsIndex,
     NABoolean disableReorgMvsIndex,
     NABoolean resetReorgMvsIndex,
     NABoolean enableRefreshMvgroup,
     NABoolean disableRefreshMvgroup,
     NABoolean resetRefreshMvgroup,
     NABoolean enableReorgMvgroup,
     NABoolean disableReorgMvgroup,
     NABoolean resetReorgMvgroup,
     NABoolean enableUpdStatsMvgroup,
     NABoolean disableUpdStatsMvgroup,
     NABoolean resetUpdStatsMvgroup,
     NABoolean enableTableLabelStats,
     NABoolean disableTableLabelStats,
     NABoolean resetTableLabelStats,
     NABoolean enableIndexLabelStats,
     NABoolean disableIndexLabelStats,
     NABoolean resetIndexLabelStats)
{
  disableReorgTable_ = disableReorgTable;
  enableReorgTable_ = enableReorgTable;
  disableReorgIndex_ = disableReorgIndex;
  enableReorgIndex_ = enableReorgIndex;
  disableUpdStatsTable_ = disableUpdStatsTable;
  enableUpdStatsTable_ = enableUpdStatsTable;
  disableUpdStatsMvs_ = disableUpdStatsMvs;
  enableUpdStatsMvs_ = enableUpdStatsMvs;
  disableRefreshMvs_ = disableRefreshMvs;
  enableRefreshMvs_ = enableRefreshMvs;
  disableReorgMvs_ = disableReorgMvs;
  enableReorgMvs_ = enableReorgMvs;
  resetReorgTable_ = resetReorgTable;
  resetUpdStatsTable_ = resetUpdStatsTable;
  resetUpdStatsMvs_ = resetUpdStatsMvs;
  resetRefreshMvs_ = resetRefreshMvs;
  resetReorgMvs_ = resetReorgMvs;
  resetReorgIndex_ = resetReorgIndex;
  enableUpdStatsMvlog_ = enableUpdStatsMvlog;
  disableUpdStatsMvlog_ = disableUpdStatsMvlog;
  resetUpdStatsMvlog_ = resetUpdStatsMvlog;
  enableReorgMvsIndex_ = enableReorgMvsIndex;
  disableReorgMvsIndex_ = disableReorgMvsIndex;
  resetReorgMvsIndex_ = resetReorgMvsIndex;
  enableRefreshMvgroup_ = enableRefreshMvgroup;
  disableRefreshMvgroup_ = disableRefreshMvgroup;
  resetRefreshMvgroup_ = resetRefreshMvgroup;
  enableReorgMvgroup_ = enableReorgMvgroup;
  disableReorgMvgroup_ = disableReorgMvgroup;
  resetReorgMvgroup_ = resetReorgMvgroup;
  enableUpdStatsMvgroup_ = enableUpdStatsMvgroup;
  disableUpdStatsMvgroup_ = disableUpdStatsMvgroup;
  resetUpdStatsMvgroup_ = resetUpdStatsMvgroup;
  enableTableLabelStats_ =  enableTableLabelStats;
  disableTableLabelStats_ =  disableTableLabelStats;
  resetTableLabelStats_ =  resetTableLabelStats;
  enableIndexLabelStats_ =  enableIndexLabelStats;
  disableIndexLabelStats_ =  disableIndexLabelStats;
  resetIndexLabelStats_ = resetIndexLabelStats;

}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilGetMetadataInfo
// -----------------------------------------------------------------------
ExeUtilGetMetadataInfo::ExeUtilGetMetadataInfo
(
     NAString &ausStr,
     NAString &infoType,
     NAString &iofStr,
     NAString &objectType,
     CorrName &objectName,
     NAString *pattern,
     NABoolean returnFullyQualNames_,
     NABoolean getVersion,
     NAString *param1,
     CollHeap *oHeap)
     : ExeUtilExpr(GET_METADATA_INFO_, CorrName("DUMMY"),
		   NULL, NULL, NULL, CharInfo::UnknownCharSet, oHeap),
       ausStr_(ausStr),
       infoType_(infoType), iofStr_(iofStr),
       objectType_(objectType), objectName_(objectName),
       pattern_((pattern ? *pattern : ""), oHeap),
       noHeader_(FALSE),
       returnFullyQualNames_(returnFullyQualNames_),
       getVersion_(getVersion),
       param1_((param1 ? *param1 : ""), oHeap),
       errorInParams_(FALSE),
       hiveObjs_(FALSE),
       hbaseObjs_(FALSE),
       cascade_(FALSE)
{
}


// -----------------------------------------------------------------------
// Member functions for class ExeUtilShowSet
// -----------------------------------------------------------------------
RelExpr * ExeUtilShowSet::copyTopNode(RelExpr *derivedNode,
				      CollHeap* outHeap)
{
  ExeUtilShowSet *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilShowSet(type_, ssdName_, outHeap);
  else
    result = (ExeUtilShowSet *) derivedNode;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilAQR
// -----------------------------------------------------------------------
ExeUtilAQR::ExeUtilAQR(
     AQRTask task,
     NAList<AQROption*> * aqrOptionsList,
     CollHeap *oHeap)
     : ExeUtilExpr(AQR_, CorrName("dummyName"), NULL, NULL, NULL, CharInfo::UnknownCharSet, oHeap),
       task_(task),
       sqlcode_(-1),
       nskcode_(-1),
       retries_(-1),
       delay_(-1),
       type_(-1)
{
  if (aqrOptionsList)
    {
      for (CollIndex i = 0; i < aqrOptionsList->entries(); i++)
	{
	  AQROption * o = (*aqrOptionsList)[i];
	  switch (o->option_)
	    {
	    case SQLCODE_:
	      sqlcode_ = o->numericVal_;
	      break;

	    case NSKCODE_:
	      nskcode_ = o->numericVal_;
	      break;

	    case RETRIES_:
	      retries_ = o->numericVal_;
	      break;

	    case DELAY_:
	      delay_ = o->numericVal_;
	      break;

	    case TYPE_:
	      type_ = o->numericVal_;
	      break;

	    default:
	      break;
	    }
	} // switch
    }
}

RelExpr * ExeUtilAQR::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilAQR *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilAQR(task_, NULL, outHeap);
  else
    result = (ExeUtilAQR *) derivedNode;

  result->sqlcode_ = sqlcode_;
  result->nskcode_ = nskcode_;
  result->retries_ = retries_;
  result->delay_   = delay_;
  result->type_    = type_;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilRegionStats
// -----------------------------------------------------------------------
ExeUtilRegionStats::ExeUtilRegionStats
(const CorrName &objectName,
 NABoolean summaryOnly,
 NABoolean isIndex,
 NABoolean forDisplay,
 NABoolean clusterView,
 RelExpr * child,
 CollHeap *oHeap)
     : ExeUtilExpr(REGION_STATS_, objectName,
		   NULL, child, NULL, CharInfo::UnknownCharSet, oHeap),
       summaryOnly_(summaryOnly),
       isIndex_(isIndex),
       displayFormat_(forDisplay),
       clusterView_(clusterView),
       errorInParams_(FALSE),
       inputColList_(NULL)
{
}

RelExpr * ExeUtilRegionStats::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilRegionStats *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilRegionStats(getTableName(),
                                              summaryOnly_, isIndex_, 
                                              displayFormat_,
                                              clusterView_,
                                              NULL,
                                              outHeap);
  else
    result = (ExeUtilRegionStats *) derivedNode;

  result->errorInParams_ = errorInParams_;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// member functions for class ExeUtilRegionStats
// -----------------------------------------------------------------------
RelExpr * ExeUtilRegionStats::bindNode(BindWA *bindWA)
{
  if (errorInParams_)
    {
      *CmpCommon::diags() << DgSqlCode(-4218) << DgString0("GET ");

      bindWA->setErrStatus();
      return this;
    }

  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  if ((NOT clusterView_) &&
      (getTableName().getQualifiedNameObj().getObjectName().isNull()))
    {
      *CmpCommon::diags() << DgSqlCode(-4218) << DgString0("REGION STATS");
      
      bindWA->setErrStatus();
      return this;
    }

  if ((! child(0)) &&
      (NOT getTableName().getQualifiedNameObj().getObjectName().isNull()))
    {
      NATable * naTable = bindWA->getNATable(getTableName());
      if ((!naTable) || (bindWA->errStatus()))
        return this;
    }

  RelExpr * childExpr = NULL;
  
  if (getArity() > 0)
    {
      childExpr = child(0)->bindNode(bindWA);
      if (bindWA->errStatus()) 
	return NULL;

      if ((childExpr->getRETDesc() == NULL) ||
	  (childExpr->getRETDesc()->getDegree() > 1) ||
	  (childExpr->getRETDesc()->getType(0).getTypeQualifier() != NA_CHARACTER_TYPE))
	{
	  *CmpCommon::diags() << DgSqlCode(-4218) << DgString0("REGION STATS ");
	  
	  bindWA->setErrStatus();
	  return this;
	}

      inputColList_ = childExpr->getRETDesc()->getValueId(0).getItemExpr();

      setChild(0, NULL);
    }

  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  if (childExpr)
    {
      RelExpr * re = new(PARSERHEAP()) Join
	(childExpr, boundExpr, REL_TSJ_FLOW, NULL);
      ((Join*)re)->doNotTransformToTSJ();
      ((Join*)re)->setTSJForWrite(TRUE);
      
      boundExpr = re->bindNode(bindWA);
      if (bindWA->errStatus()) 
	return NULL;
    }

  return boundExpr;
}

void ExeUtilRegionStats::recomputeOuterReferences()
{
  if (inputColList_)
    {
      ValueIdSet outerRefs = getGroupAttr()->getCharacteristicInputs(); 
      outerRefs += inputColList_->getValueId();
      
      getGroupAttr()->setCharacteristicInputs(outerRefs);
    }
} // ExeUtilRegionStats::recomputeOuterReferences()  

// -----------------------------------------------------------------------
// Member functions for class ExeUtilRegionStats
// -----------------------------------------------------------------------
ExeUtilLobInfo::ExeUtilLobInfo
(const CorrName &objectName,
 NABoolean  tableFormat,
 RelExpr * child,
 CollHeap *oHeap)
     : ExeUtilExpr(LOB_INFO_, objectName,
		   NULL, child, NULL, CharInfo::UnknownCharSet, oHeap),
       errorInParams_(FALSE),
       objectUID_(0)
{
  tableFormat_ = tableFormat;
}

RelExpr * ExeUtilLobInfo::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilLobInfo *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilLobInfo(getTableName(),
                                          FALSE,
                                          NULL,
                                          outHeap);
  else
    result = (ExeUtilLobInfo *) derivedNode;

  result->errorInParams_ = errorInParams_;
  result->objectUID_ = objectUID_;
  return ExeUtilExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// member functions for class ExeUtilLobInfo
// -----------------------------------------------------------------------
RelExpr * ExeUtilLobInfo::bindNode(BindWA *bindWA)
{
  if (errorInParams_)
    {
      *CmpCommon::diags() << DgSqlCode(-4218) << DgString0("GET ");

      bindWA->setErrStatus();
      return this;
    }

  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  if (getTableName().getQualifiedNameObj().getObjectName().isNull())
    {
      *CmpCommon::diags() << DgSqlCode(-4218) << DgString0("LOB INFO");
      
      bindWA->setErrStatus();
      return this;
    }

  
  NATable * naTable = bindWA->getNATable(getTableName());
  if ((!naTable) || (bindWA->errStatus()))
    return this;
    
 // Allocate a TableDesc and attach it to this.
  //
  setUtilTableDesc(bindWA->createTableDesc(naTable, getTableName()));
  if (bindWA->errStatus())
    return this;

  objectUID_ = naTable->objectUid().get_value();

  RelExpr * childExpr = NULL;
  
  if (getArity() > 0)
    {
      childExpr = child(0)->bindNode(bindWA);
      if (bindWA->errStatus()) 
	return NULL;

      if ((childExpr->getRETDesc() == NULL) ||
	  (childExpr->getRETDesc()->getDegree() > 1) ||
	  (childExpr->getRETDesc()->getType(0).getTypeQualifier() != NA_CHARACTER_TYPE))
	{
	  *CmpCommon::diags() << DgSqlCode(-4218) << DgString0("LOB INFO ");
	  
	  bindWA->setErrStatus();
	  return this;
	}

      

      setChild(0, NULL);
    }

  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  if (childExpr)
    {
      RelExpr * re = new(PARSERHEAP()) Join
	(childExpr, boundExpr, REL_TSJ_FLOW, NULL);
      ((Join*)re)->doNotTransformToTSJ();
      ((Join*)re)->setTSJForWrite(TRUE);
      
      boundExpr = re->bindNode(bindWA);
      if (bindWA->errStatus()) 
	return NULL;
    }

  return boundExpr;
}

void ExeUtilLobInfo::recomputeOuterReferences()
{
 
} // ExeUtilLobInfo::recomputeOuterReferences()  


// -----------------------------------------------------------------------
// Member functions for class ExeUtilLongRunning
// -----------------------------------------------------------------------
ExeUtilLongRunning::ExeUtilLongRunning(const CorrName    &name,
				       const char *     predicate,
                                       ULng32    predicateLen,
				       LongRunningType type,
				       ULng32 userSpecifiedCommitSize,
				       CollHeap *oHeap)
  : ExeUtilExpr(LONG_RUNNING_, name, NULL, NULL, NULL, CharInfo::UnknownCharSet, oHeap),
    type_(type),
    lruStmt_(NULL),
    lruStmtWithCK_(NULL)
{
  if (predicate)
    {
      predicateLen_ = predicateLen;
      predicate_ = new(CmpCommon::statementHeap()) char[predicateLen+1];
      strncpy(predicate_, predicate, predicateLen);
      predicate_[predicateLen] = '\0';
    }
  else
    {
      predicate_ = NULL;
      predicateLen_ = 0;
    }

  // take the Multi Commit Size, the user specified thro' the statement.
  // It takes precedence over all.
  multiCommitSize_ = userSpecifiedCommitSize;

  if (multiCommitSize_ == 0)  // the user did not specify the Multi Commit size thro' the statement.
    {

      // See if the MULTI COMMIT SIZE was specified thro' a SET TRANSACTION stmt.

      // If a previous SET TRANSACTION has set the multi commit size,
      // then use this size instead of the CQD.

      if (CmpCommon::transMode()->getMultiCommit() == TransMode::MC_ON_ &&
	  (CmpCommon::transMode()->getMultiCommitSize() != 0))
	{
	  multiCommitSize_ = CmpCommon::transMode()->getMultiCommitSize();
	}


      // If it's still zero, then the the user did not specify the commit size thro' a
      // statement or thro' SET TRANSACTION, then get the commit size from
      // the system defaults/CQD
      if (multiCommitSize_ == 0)
	{
	  multiCommitSize_ = (ULng32)CmpCommon::getDefaultNumeric(MULTI_COMMIT_SIZE);
	}

    } // (multiCommitSize == 0)
}

ExeUtilLongRunning::~ExeUtilLongRunning()
{
  if(predicate_)
    {
      NADELETEBASIC(predicate_, CmpCommon::statementHeap());
    }
}


// -----------------------------------------------------------------------
// Member functions for class ExeUtilLongRunning
// -----------------------------------------------------------------------
RelExpr * ExeUtilLongRunning::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilExpr *result;

  CMPASSERT(derivedNode == NULL);

  result = new (outHeap) ExeUtilLongRunning(getTableName(),
					    predicate_,
                                            (ULng32)predicateLen_,
					    LR_DELETE, // currently only delete is supported. So, LRU_DELETE_
					    multiCommitSize_,
					    outHeap);
  ((ExeUtilLongRunning *) result)->setLongRunningType(type_);

  return ExeUtilExpr::copyTopNode(result, outHeap);
}


NAString ExeUtilLongRunning::constructLRUDeleteStatement(NABoolean withCK)
{
  NAString lruDeleteStmt;
  NAString ckColumns;
  NAString mvNtriggerControl = "";
  NAString partClauseStr = "";

  ckColumns = getCKColumnsAsSelectList();

  lruDeleteStmt = "SELECT [LAST 1] ";
  lruDeleteStmt += ckColumns;

  // is MV NOMVLOG option specified
  if (isNoLogOperation_)
     mvNtriggerControl += "NOMVLOG ";


  lruDeleteStmt += "FROM (DELETE ";
  lruDeleteStmt += mvNtriggerControl;
  lruDeleteStmt += "[FIRST ";

  char commitSize[22];
  convertInt64ToAscii(multiCommitSize_, commitSize);
  lruDeleteStmt += commitSize;

  lruDeleteStmt += "] FROM TABLE (";
  // Do we need to specify the special table syntax for the IUD log table?
  if (getTableName().getSpecialType() != ExtendedQualName::IUD_LOG_TABLE)
    lruDeleteStmt += "TABLE ";
  else
    lruDeleteStmt += "IUD_LOG_TABLE ";
  lruDeleteStmt += getTableName().getQualifiedNameObj().getQualifiedNameAsAnsiString(TRUE);

  // is a signle partition specified
  PartitionClause partClause = getTableName().getPartnClause();
  if (!partClause.isEmpty())
  {
     if (partClause.partnNumSpecified())
     {
       partClauseStr += ", PARTITION NUMBER ";
       char buf[10];
       sprintf(buf, "%d", partClause.getPartitionNumber());
       partClauseStr += buf;
     }
     else if (partClause.partnNameSpecified())
     {
       partClauseStr += ", PARTITION ";
       partClauseStr += partClause.getPartitionName();
     }
     else if (!(partClause.getLocationName().isNull()))
     {
       partClauseStr += ", LOCATION ";
       partClauseStr += partClause.getLocationName();
     }
  }

  if (partClauseStr.length())
  {
     partClauseStr += " ";
     lruDeleteStmt += partClauseStr;
  }
  else
  {
    lruDeleteStmt += ", PARTITION NUMBER ";

    // Parameterize the partition number.
    lruDeleteStmt += " %d ";
  }
  lruDeleteStmt += ") ";

  // Parameter for predicate_
  if (predicate_!= NULL)
    lruDeleteStmt += " %s ";

  if(withCK)
    {
      if (predicate_!= NULL)
	{
	  // keyword WHERE is part of predicate_
	  lruDeleteStmt += " AND ";
	}
      else
	{
	lruDeleteStmt += " WHERE ";
	}

      lruDeleteStmt += "(";
      lruDeleteStmt += constructKeyRangeComparePredicate();
      lruDeleteStmt += ")";
    }

  lruDeleteStmt += ") AS MTS_TABLE;";

  return lruDeleteStmt;
}

NAString ExeUtilLongRunning::getCKColumnsAsSelectList()
{

  NAString ckAsSelectList;

  const ValueIdList &ckeys =
    getUtilTableDesc()->getClusteringIndex()->getClusteringKeyCols();

  for (CollIndex i = 0; i < ckeys.entries(); i++)
    {
      // add comma only if it's not the last column and first column in the select list.
      if (i > 0)
	ckAsSelectList += ", ";

      // All column names may be set in double quotes.
      // This will automatically preserve delimited column names.
      ckAsSelectList += '"';
      ckAsSelectList += ((NAColumn *) ckeys[i].getNAColumn())->getColName();
      ckAsSelectList += '"';

    }

    ckAsSelectList += " ";

    return ckAsSelectList;
}


NAString ExeUtilLongRunning::constructKeyRangeComparePredicate()
{

  NAString ckAsTypedParameters;

  const ValueIdList &ckeys =
    getUtilTableDesc()->getClusteringIndex()->getClusteringKeyCols();

  for (CollIndex i = 0; i < ckeys.entries(); i++)
    {
     NAColumn *column = (NAColumn *) ckeys[i].getNAColumn();

     const NAType *colType = column->getType();
     const char* colTypeNameNA = column->getType()->getTypeName();
     Lng32 precision = colType->getPrecision();
     Lng32 scale = colType->getScale();
     Lng32 varlen = colType->getVarLenHdrSize();
     Lng32 charSize = colType->getNominalSize();
     NAString sqlTypeName = colType->getTypeSQLname(TRUE);

     // Prepare the casting strings
     char * colTypeName = NULL;
     NABuiltInTypeEnum enumType = colType->getTypeQualifier();

     // add comma only if it's not the last or the first column in the list.
     if (i > 0)
       ckAsTypedParameters += ", ";

     switch(enumType)
       {
       case NA_NUMERIC_TYPE:

         if ( (strcmp(colTypeNameNA, "NUMERIC") == 0) ||
              (strcmp(colTypeNameNA, "BIG NUM") == 0) )
	   {
	     char precisionBuf[10];
	     char scaleBuf[10];

	     sprintf(precisionBuf,"%d", precision);
	     sprintf(scaleBuf,"%d", scale);

	     ckAsTypedParameters += "CAST (? AS ";
	     ckAsTypedParameters += "NUMERIC";
	     ckAsTypedParameters += "(";
	     ckAsTypedParameters += precisionBuf;


	     if (scale >0)
	       {
		 ckAsTypedParameters += ",";
		 ckAsTypedParameters += scaleBuf;
	       }

	     ckAsTypedParameters += ")) ";
	   }
	 else
	   {
	    if (strcmp(colTypeNameNA, "DOUBLE PRECISION") == 0 ||
                strcmp(colTypeNameNA, "REAL") == 0)
             {
              ckAsTypedParameters += "?";
             }
             else
             {
              ckAsTypedParameters += "CAST (? AS ";
              ckAsTypedParameters += colTypeNameNA;
              ckAsTypedParameters += ") ";
             }
 	   }
	 break;


       case NA_DATETIME_TYPE:


         char scaleBuf[10];
         sprintf(scaleBuf,"%d", scale);
         
         if ((strcmp(colTypeNameNA,"TIMESTAMP") == 0) || (strcmp(colTypeNameNA,"TIME") == 0))
	 {
            ckAsTypedParameters += "CAST (? AS ";
	    ckAsTypedParameters += colTypeNameNA;
	    ckAsTypedParameters += "(";
	    ckAsTypedParameters += scaleBuf;
            ckAsTypedParameters += ")) ";
	 }
         else
         {
           ckAsTypedParameters += "CAST (? AS ";
	   ckAsTypedParameters += colTypeNameNA;
	   ckAsTypedParameters += ") ";
	 }
	  
	 break;

       case NA_BOOLEAN_TYPE:

	 ckAsTypedParameters += "CAST (? AS ";
	 ckAsTypedParameters += colTypeNameNA;
	 ckAsTypedParameters += ") ";
	 break;

       case NA_CHARACTER_TYPE:

	 ckAsTypedParameters += "CAST (? AS ";
	 ckAsTypedParameters += sqlTypeName.data();
	 ckAsTypedParameters += ") ";

	 break;

       case NA_INTERVAL_TYPE:

	 ckAsTypedParameters += "CAST (? AS ";
	 ckAsTypedParameters += sqlTypeName.data();
	 ckAsTypedParameters += ") ";
	 break;

       default  : // Unknown column
      CMPASSERT(FALSE);  // generate real error msg here
       }
    }


  NAString keyRangeComparePredicate;

  keyRangeComparePredicate += "KEY_RANGE_COMPARE (CLUSTERING KEY >= (";
  keyRangeComparePredicate += ckAsTypedParameters;

  keyRangeComparePredicate += ")) ";

  return keyRangeComparePredicate;
}


// -----------------------------------------------------------------------
// Member functions for class ExeUtilGetUID
// -----------------------------------------------------------------------
ExeUtilGetUID::ExeUtilGetUID(
     const CorrName &name,
     enum ExeUtilMaintainObject::MaintainObjectType type,
     CollHeap *oHeap)
     : ExeUtilExpr(GET_UID_, name, NULL, NULL, NULL, CharInfo::UnknownCharSet, oHeap),
       type_(type),
       uid_(-1)
{
}

RelExpr * ExeUtilGetUID::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilGetUID *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilGetUID(getTableName(), type_, outHeap);
  else
    result = (ExeUtilGetUID *) derivedNode;
  
  result->uid_ = uid_;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilGetQID
// -----------------------------------------------------------------------
ExeUtilGetQID::ExeUtilGetQID(
                             NAString &statement,
                             CollHeap *oHeap)
  : ExeUtilExpr(GET_QID_, CorrName("dummy"), 
                NULL, NULL, NULL, CharInfo::UnknownCharSet, oHeap),
    statement_(statement)
{
}

RelExpr * ExeUtilGetQID::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilGetQID *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilGetQID(statement_);
  else
    result = (ExeUtilGetQID *) derivedNode;
  
  return ExeUtilExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilPopulateInMemStats
// -----------------------------------------------------------------------
ExeUtilPopulateInMemStats::ExeUtilPopulateInMemStats(
     const CorrName &inMemTableName,
     const CorrName &sourceTableName,
     const SchemaName * sourceStatsSchemaName,
     CollHeap *oHeap)
     : ExeUtilExpr(POP_IN_MEM_STATS_, CorrName("DUMMY"), 
		   NULL, NULL, NULL, CharInfo::UnknownCharSet, oHeap),
       inMemTableName_(inMemTableName),
       sourceTableName_(sourceTableName),
       uid_(-1)
{
  if (sourceStatsSchemaName)
    sourceStatsSchemaName_ = *sourceStatsSchemaName;
}

RelExpr * ExeUtilPopulateInMemStats::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilPopulateInMemStats *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilPopulateInMemStats(
	 getInMemTableName(),
	 getSourceTableName(),
	 (sourceStatsSchemaName_.getSchemaName().isNull() ? NULL : &sourceStatsSchemaName_));
  else
    result = (ExeUtilPopulateInMemStats *) derivedNode;
  
  result->uid_ = uid_;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

//////////////////////////////////////////////////////////////////////////
// bindNode methods for ExeUtil operators
//////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------
// member functions for class ExeUtilExpr
// -----------------------------------------------------------------------

RelExpr * ExeUtilExpr::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  if (NOT tableName_.isEmpty())
    tableName_.applyDefaults(bindWA, bindWA->getDefaultSchema());

  //
  // Bind the child nodes.
  //
  bindChildren(bindWA);
  if (bindWA->errStatus())
    return this;

  RelExpr * boundExpr = GenericUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus())
    return NULL;

  if (tableName_.getSpecialType()==ExtendedQualName::NORMAL_TABLE) 
    if (bindWA->violateAccessDefaultSchemaOnly(tableName_.getQualifiedNameObj()))
      return this;

  //
  // Bind the base class.
  //
  boundExpr = bindSelf(bindWA);
  if (bindWA->errStatus()) 
    return boundExpr;

  ValueIdSet ov;
  getPotentialOutputValues(ov);
  getGroupAttr()->addCharacteristicOutputs(ov);

  return boundExpr;
} // ExeUtilExpr::bindNode()

// -----------------------------------------------------------------------
// member functions for class ExeUtilDisplayExplain
// -----------------------------------------------------------------------
RelExpr * ExeUtilDisplayExplainComplex::bindNode(BindWA *bindWA)
{
  return NULL;
}

// -----------------------------------------------------------------------
// member functions for class DDLExpr
// -----------------------------------------------------------------------
RelExpr * DDLExpr::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  isCreate_ = FALSE;
  isCreateLike_ = FALSE;
  isDrop_ = FALSE;
  isAlter_ = FALSE;
  isCleanup_ = FALSE;
  isHbase_ = FALSE;
  isNative_ = FALSE;
  hbaseDDLNoUserXn_ = FALSE;
  isSchema_ = FALSE;

  NABoolean isSeq = FALSE;

  NABoolean alterAddCol = FALSE;
  NABoolean alterDropCol = FALSE;
  NABoolean alterDisableIndex = FALSE;
  NABoolean alterEnableIndex = FALSE;
  NABoolean alterHBaseOptions = FALSE;
  NABoolean otherAlters = FALSE;
  NABoolean isPrivilegeMngt = FALSE;
  NABoolean isCreateSchema = FALSE;
  NABoolean isDropSchema = FALSE;
  NABoolean isAlterSchema = FALSE;
  NABoolean isAuth = FALSE;
  NABoolean alterAddConstr = FALSE;
  NABoolean alterDropConstr = FALSE;
  NABoolean alterRenameTable = FALSE;
  NABoolean alterStoredDesc = FALSE;
  NABoolean alterIdentityCol = FALSE;
  NABoolean alterColDatatype = FALSE;
  NABoolean alterColRename = FALSE;
  NABoolean alterLibrary = FALSE;
  NABoolean externalTable = FALSE;
  NABoolean isVolatile = FALSE;
  NABoolean isRegister = FALSE;
  NABoolean isCommentOn = FALSE;
  NABoolean isHive = FALSE;


  NABoolean specialType = FALSE;
  if (isUstat())  // special DDLExpr node for an Update Stats statement
    {
      RelExpr * boundExpr = GenericUtilExpr::bindNode(bindWA);
      if (bindWA->errStatus())
        return NULL;
      
      return boundExpr;
      //      isHbase_ = TRUE;
    }
  else if (initAuth() || dropAuth() || cleanupAuth())
  {
    isHbase_ = TRUE;
    hbaseDDLNoUserXn_ = TRUE;
  }
  else if (initHbase() || dropHbase() || createMDViews() || dropMDViews() ||
      addSchemaObjects() || updateVersion())
  {
    isHbase_ = TRUE;
    hbaseDDLNoUserXn_ = TRUE;
  }
  else if (createLibmgr() || dropLibmgr() || upgradeLibmgr())
    {
      isHbase_ = TRUE;
      hbaseDDLNoUserXn_ = TRUE;
    }
  else if (createRepos() || dropRepos() || upgradeRepos())
    {
      isHbase_ = TRUE;
      hbaseDDLNoUserXn_ = TRUE;
    }
  else if (purgedata())
  {
    isHbase_ = TRUE;
    hbaseDDLNoUserXn_ = TRUE;      
  }
  else if (getExprNode() && getExprNode()->castToStmtDDLNode())
  {
    if (getExprNode()->castToStmtDDLNode()->castToStmtDDLCreateTable())
    {
      StmtDDLCreateTable * createTableNode =
        getExprNode()->castToStmtDDLNode()->castToStmtDDLCreateTable();

      if (createTableNode->isVolatile())
        isVolatile = TRUE;

      isCreate_ = TRUE;
      isTable_ = TRUE;

      if (createTableNode->getIsLikeOptionSpecified())
        isCreateLike_ = TRUE;

      objName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLCreateTable()->
        getTableName();

      qualObjName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLCreateTable()->
        getTableNameAsQualifiedName();

      if ((createTableNode->isInMemoryObjectDefn()) ||
          (createTableNode->isMultiSetTable()) ||
          (createTableNode->isSetTable()))
      {
        // these options not supported in open source
        *CmpCommon::diags() << DgSqlCode(-4222) << DgString0("InMemory/Set/Multiset");
        bindWA->setErrStatus();
        return NULL;
      }

      // Hive tables can only be specified as external and must be created
      // with the FOR clause
       if (createTableNode->isExternal())
         qualObjName_.applyDefaults(bindWA->getDefaultSchema());

      if (qualObjName_.isHive()) 
      {
        if (createTableNode->isExternal())
        {
          externalTable = TRUE;
          isHbase_ = TRUE;
        }
        else 
        {
          *CmpCommon::diags() << DgSqlCode(-3242) << DgString0("External tables supported on hive tables only.");
          bindWA->setErrStatus();
          return NULL;
        }
      }
 
      // if unique, ref or check constrs are specified, then dont start a transaction.
      // ddl with these clauses is executed as a compound create.
      // A compound create cannot run under a user transaction.
      if ((createTableNode->getAddConstraintUniqueArray().entries() > 0) ||
	  (createTableNode->getAddConstraintRIArray().entries() > 0) ||
	  (createTableNode->getAddConstraintCheckArray().entries() > 0))
        {
          if ((NOT createTableNode->ddlXns()) &&
              (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
            hbaseDDLNoUserXn_ = TRUE;
        }
    } // createTable
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLCreateHbaseTable())
    {
      StmtDDLCreateHbaseTable * createHbaseTableNode =
	getExprNode()->castToStmtDDLNode()->castToStmtDDLCreateHbaseTable();
      
      isCreate_ = TRUE;
      isTable_ = TRUE;
      isNative_ = TRUE;
      isHbase_ = TRUE;

      objName_ =
	getDDLNode()->castToStmtDDLNode()->castToStmtDDLCreateHbaseTable()->
	getTableName();
      
      qualObjName_ =
	getDDLNode()->castToStmtDDLNode()->castToStmtDDLCreateHbaseTable()->
	getTableNameAsQualifiedName();
    } // createHbaseTable
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLCreateIndex())
    {
      isCreate_ = TRUE;
      isIndex_ = TRUE;

      StmtDDLCreateIndex * createIndexNode =
        getExprNode()->castToStmtDDLNode()->castToStmtDDLCreateIndex();

      if (createIndexNode->isVolatile())
        isVolatile = TRUE;

      objName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLCreateIndex()->
        getIndexName();

      qualObjName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLCreateIndex()->
        getIndexNameAsQualifiedName();
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLPopulateIndex())
    {
      isCreate_ = TRUE;
      isIndex_ = TRUE;

      objName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLPopulateIndex()->
        getIndexName();

      qualObjName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLPopulateIndex()->
        getIndexNameAsQualifiedName();
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLDropTable())
    {
      isDrop_ = TRUE;
      isTable_ = TRUE;

      StmtDDLDropTable * dropTableNode =
        getExprNode()->castToStmtDDLNode()->castToStmtDDLDropTable();

      if (dropTableNode->isVolatile())
        isVolatile = TRUE;

      qualObjName_ = dropTableNode->getTableNameAsQualifiedName();

      // Normally, when a drop table is executed and DDL transactions is not
      // enabled, a user started transaction is not allowed.  However, when a
      // session ends, a call is made to drop a volatile table, this drop should
      // succeed. 
      if ((dropTableNode->isVolatile()) &&
          (NOT getExprNode()->castToStmtDDLNode()->ddlXns()))
        hbaseDDLNoUserXn_ = TRUE;

      if (dropTableNode->isExternal())
         qualObjName_.applyDefaults(bindWA->getDefaultSchema());

      // Drops of Hive and HBase external tables are allowed 
      if (qualObjName_.isHive() || (qualObjName_.isHbase()))
        {
          if (dropTableNode->isExternal())
            {
              isHbase_ = TRUE;
              externalTable = TRUE;
            }
          else
            {
              *CmpCommon::diags() << DgSqlCode(-4222) << DgString0("DDL");
              bindWA->setErrStatus();
              return NULL;
            }
        }
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLDropHbaseTable())
    {
      isDrop_ = TRUE;
      isTable_ = TRUE;
      isNative_ = TRUE;
      isHbase_ = TRUE;

      qualObjName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLDropHbaseTable()->
        getTableNameAsQualifiedName();
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLDropIndex())
    {
      StmtDDLDropIndex * dropIndexNode =
        getExprNode()->castToStmtDDLNode()->castToStmtDDLDropIndex();

      if (dropIndexNode->isVolatile())
        isVolatile = TRUE;

      isDrop_ = TRUE;
      isIndex_ = TRUE;

      qualObjName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLDropIndex()->
        getIndexNameAsQualifiedName();
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLAlterTable())
    {
      isAlter_ = TRUE;
      isTable_ = TRUE;

      if (getExprNode()->castToStmtDDLNode()->castToStmtDDLAlterTableAddColumn())
        alterAddCol = TRUE;
      else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLAlterTableDropColumn())
        alterDropCol = TRUE;
      else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLAlterTableDisableIndex())
        alterDisableIndex = TRUE;
      else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLAlterTableEnableIndex())
        alterEnableIndex = TRUE;
      else if ((getExprNode()->castToStmtDDLNode()->castToStmtDDLAddConstraintUnique()) ||
	       (getExprNode()->castToStmtDDLNode()->castToStmtDDLAddConstraintRI()) ||
	       (getExprNode()->castToStmtDDLNode()->castToStmtDDLAddConstraintCheck()))
         alterAddConstr = TRUE;
      else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLDropConstraint())
         alterDropConstr = TRUE;
      else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLAlterTableRename())
         alterRenameTable = TRUE;
      else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLAlterTableAlterColumnSetSGOption())
         alterIdentityCol = TRUE;
      else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLAlterTableAlterColumnDatatype())
         alterColDatatype = TRUE;
       else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLAlterTableAlterColumnRename())
         alterColRename = TRUE;
       else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLAlterTableHBaseOptions())
         alterHBaseOptions = TRUE;
       else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLAlterTableStoredDesc())
         alterStoredDesc = TRUE;
       else
        otherAlters = TRUE;

      qualObjName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLAlterTable()->
        getTableNameAsQualifiedName();
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLAlterSchema())
    {
      isAlter_ = TRUE;
      isSchema_ = TRUE;

      isAlterSchema = TRUE;

      qualObjName_ =
        QualifiedName(NAString("dummy"),
                      getExprNode()->castToStmtDDLNode()->castToStmtDDLAlterSchema()->getSchemaNameAsQualifiedName().getSchemaName(),
                      getExprNode()->castToStmtDDLNode()->castToStmtDDLAlterSchema()->getSchemaNameAsQualifiedName().getCatalogName());
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLAlterIndex())
    {
      isAlter_ = TRUE;
      isIndex_ = TRUE;
      if (getExprNode()->castToStmtDDLNode()->castToStmtDDLAlterIndexHBaseOptions())
        alterHBaseOptions = TRUE;
      else
        otherAlters = TRUE;

      qualObjName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLAlterIndex()->
        getIndexNameAsQualifiedName();
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLCreateView())
    {
      isCreate_ = TRUE;
      isView_ = TRUE;

      qualObjName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLCreateView()->
        getViewNameAsQualifiedName();
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLDropView())
    {
      isDrop_ = TRUE;
      isView_ = TRUE;

      qualObjName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLDropView()->
        getViewNameAsQualifiedName();
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLCreateSequence())
    {
      StmtDDLCreateSequence * createSeq =
	getExprNode()->castToStmtDDLNode()->castToStmtDDLCreateSequence();
      
      isCreate_ = TRUE;
      isSeq = TRUE;
      isHbase_ = TRUE;

      objName_ =
	getDDLNode()->castToStmtDDLNode()->castToStmtDDLCreateSequence()->
	getSeqName();
      
      qualObjName_ =
	getDDLNode()->castToStmtDDLNode()->castToStmtDDLCreateSequence()->
	getSeqNameAsQualifiedName();
    } // createSequence
   else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLDropSequence())
    {
      StmtDDLDropSequence * dropSeq =
	getExprNode()->castToStmtDDLNode()->castToStmtDDLDropSequence();
      
      isDrop_ = TRUE;
      isSeq = TRUE;
      isHbase_ = TRUE;

      objName_ =
	getDDLNode()->castToStmtDDLNode()->castToStmtDDLDropSequence()->
	getSeqName();
      
      qualObjName_ =
	getDDLNode()->castToStmtDDLNode()->castToStmtDDLDropSequence()->
	getSeqNameAsQualifiedName();
    } // dropSequence
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLRegisterUser())
    {
      isAuth = TRUE; 
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLAlterUser())
    {
      isAuth = TRUE; 
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLCreateRole())
    {
      isAuth = TRUE; 
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLRoleGrant())
    {
      isPrivilegeMngt = TRUE; 
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLRegisterComponent())
    {
      isPrivilegeMngt = TRUE; 
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLCreateComponentPrivilege())
    {
      isPrivilegeMngt = TRUE; 
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLDropComponentPrivilege())
    {
      isPrivilegeMngt = TRUE; 
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLGrantComponentPrivilege())
    {
      isPrivilegeMngt = TRUE; 
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLRevokeComponentPrivilege())
    {
      isPrivilegeMngt = TRUE; 
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLGiveAll())
    {
      isPrivilegeMngt = TRUE; 
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLGiveObject())
    {
      isPrivilegeMngt = TRUE; 
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLGiveSchema())
    {
      isPrivilegeMngt = TRUE; 
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLGrant())
    {
      isTable_ = TRUE;
      isPrivilegeMngt = TRUE;
      qualObjName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLGrant()->
        getGrantNameAsQualifiedName();
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLRevoke())
    {
      isTable_ = TRUE;
      isPrivilegeMngt = TRUE;
      qualObjName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLRevoke()->
        getRevokeNameAsQualifiedName();
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLDropSchema())
    {
      isDropSchema = TRUE;
      qualObjName_ =
        QualifiedName(NAString("dummy"),
                      getExprNode()->castToStmtDDLNode()->castToStmtDDLDropSchema()->getSchemaNameAsQualifiedName().getSchemaName(),
                      getExprNode()->castToStmtDDLNode()->castToStmtDDLDropSchema()->getSchemaNameAsQualifiedName().getCatalogName());

    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLCreateSchema())
    {
      isCreateSchema = TRUE;
      qualObjName_ =
        QualifiedName(NAString("dummy"),
                      getExprNode()->castToStmtDDLNode()->castToStmtDDLCreateSchema()->getSchemaNameAsQualifiedName().getSchemaName(),
                      getExprNode()->castToStmtDDLNode()->castToStmtDDLCreateSchema()->getSchemaNameAsQualifiedName().getCatalogName());
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLCreateLibrary())
    {
      isCreate_ = TRUE;
      isLibrary_ = TRUE;
      qualObjName_ = getExprNode()->castToStmtDDLNode()->
        castToStmtDDLCreateLibrary()->getLibraryNameAsQualifiedName();
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLDropLibrary())
    {
      isDrop_ = TRUE;
      isLibrary_ = TRUE;
      qualObjName_ = getExprNode()->castToStmtDDLNode()->
        castToStmtDDLDropLibrary()->getLibraryNameAsQualifiedName();
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLAlterLibrary())
    {
      isAlter_ = TRUE;
      isLibrary_ = TRUE;
      alterLibrary = TRUE ;
      qualObjName_ = getExprNode()->castToStmtDDLNode()->
	castToStmtDDLAlterLibrary()->getLibraryNameAsQualifiedName();
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLCreateRoutine())
    {
      isCreate_ = TRUE;
      isRoutine_ = TRUE;
      qualObjName_ = getExprNode()->castToStmtDDLNode()->
        castToStmtDDLCreateRoutine()->getRoutineNameAsQualifiedName();
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLDropRoutine())
    {
      isDrop_ = TRUE;
      isRoutine_ = TRUE;
      qualObjName_ = getExprNode()->castToStmtDDLNode()->
        castToStmtDDLDropRoutine()->getRoutineNameAsQualifiedName();
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLCleanupObjects())
    {
      isCleanup_ = TRUE;

      returnStatus_ = 
        getExprNode()->castToStmtDDLNode()->castToStmtDDLCleanupObjects()->getStatus();
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLRegOrUnregObject())
    {
      isRegister = TRUE;

      qualObjName_ = getExprNode()->castToStmtDDLNode()->
        castToStmtDDLRegOrUnregObject()->getObjNameAsQualifiedName();
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLCommentOn())
    {
      isCommentOn = TRUE;

      qualObjName_ = getExprNode()->castToStmtDDLNode()->
        castToStmtDDLCommentOn()->getObjectNameAsQualifiedName();
    }
    else if (getExprNode()->castToStmtDDLNode()->castToStmtDDLonHiveObjects())
    {
      if (getExprNode()->castToStmtDDLNode()->castToStmtDDLonHiveObjects()->getOper() != StmtDDLonHiveObjects::PASSTHRU_DDL_)
        {
          if (CmpCommon::getDefault(TRAF_DDL_ON_HIVE_OBJECTS) == DF_OFF)
            {
              *CmpCommon::diags() << DgSqlCode(-3242) << DgString0("DDL on Hive objects is not allowed from Trafodion interface. Use hive shell to perform this operation.");
              bindWA->setErrStatus();
              return NULL;
            }
        }

      isHive = TRUE;
      isHbase_ = TRUE;
    }

    if (isCleanup_)
      {
        if (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
          hbaseDDLNoUserXn_ = TRUE;
      }

    if ((isCreateSchema || isDropSchema || isAlterSchema) || isRegister || isCommentOn ||
        ((isTable_ || isIndex_ || isView_ || isRoutine_ || isLibrary_ || isSeq || isHive) &&
         (isCreate_ || isDrop_ || purgedata() || 
          (isAlter_ && (alterAddCol || alterDropCol || alterDisableIndex || alterEnableIndex || 
			alterAddConstr || alterDropConstr || alterRenameTable ||
                        alterStoredDesc ||
                        alterIdentityCol || alterColDatatype || alterColRename ||
                        alterHBaseOptions || alterLibrary || otherAlters)))))
      {
	if ((NOT isNative_) &&
            (NOT isVolatile))
	  {
	    qualObjName_.applyDefaults(bindWA->getDefaultSchema());
	    
	    if (qualObjName_.isSeabase())
	      {
		isHbase_ = TRUE;
	      }
	  }
	
        // volatile tables are traf tables
        if (isVolatile)
          {
            isHbase_ = TRUE;
          }

	if (isHbase_ && otherAlters)
	  {
	    *CmpCommon::diags() << DgSqlCode(-4222) << DgString0("ALTER");
	    bindWA->setErrStatus();
	    return NULL;
	  }

        if (isRegister || isCommentOn)
          {
            isHbase_ = TRUE;
          }

	// if a user ddl operation, it cannot run under a user transaction.
	// If an internal ddl request, like a CREATE internally issued onbehalf
	// of a CREATE LIKE, then allow it to run under a user Xn.
        if ((NOT getExprNode()->castToStmtDDLNode()->ddlXns()) &&
            (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
	  hbaseDDLNoUserXn_ = TRUE;
      }
    else if (isAuth || isPrivilegeMngt || isCleanup_)
      {
        isHbase_ = TRUE;
      }

    else if (NOT isHive)
      {
        if ((alterDropCol) || (alterEnableIndex))
          {
            // non-hbase tables not supported in open source
            *CmpCommon::diags() << DgSqlCode(-4222) << DgString0("Drop Column");
            bindWA->setErrStatus();
            return NULL;
            }

        // non-hbase tables not supported in open source
        *CmpCommon::diags() << DgSqlCode(-4222) << DgString0("DDL");
        bindWA->setErrStatus();
        return NULL;
      }
  }

  RelExpr * boundExpr = GenericUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus())
    return NULL;

  if (isHbase_ || externalTable || isVolatile || isHive)
    return boundExpr;

  if (isView_ && (isCreate_ || isDrop_))
    {
      if (qualObjName_.getCatalogName().isNull())
        *CmpCommon::diags() << DgSqlCode(-3242) << DgString0("This view cannot be created or dropped.");
      else
        *CmpCommon::diags() << DgSqlCode(-3242) << DgString0(NAString("This view cannot be created or dropped in the specified catalog '") + qualObjName_.getCatalogName() + "'.");
    }
  else if ((NOT qualObjName_.getCatalogName().isNull()) &&
           (NOT ((qualObjName_.getCatalogName() == TRAFODION_SYSCAT_LIT) ||
                 (qualObjName_.getCatalogName() == HBASE_SYSTEM_SCHEMA) ||
                 (qualObjName_.getCatalogName() == HIVE_SYSTEM_SCHEMA))))
    {
      *CmpCommon::diags() << DgSqlCode(-3242) << 
        DgString0(NAString("This DDL operation is not allowed in the specified catalog '" + qualObjName_.getCatalogName() + "'."));
    }
  else
    *CmpCommon::diags() << DgSqlCode(-3242) << DgString0("This DDL operation cannot be done.");

  bindWA->setErrStatus();
  return NULL;
}
  
// -----------------------------------------------------------------------
// member functions for class ExeUtilProcessVolatileTable
// -----------------------------------------------------------------------
RelExpr * ExeUtilProcessVolatileTable::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  isCreate_ = FALSE;
  isTable_  = FALSE;
  isIndex_  = FALSE;
  isSchema_ = FALSE;

  // set volTabName
  if (getDDLNode()->castToStmtDDLNode() &&
      getDDLNode()->castToStmtDDLNode()->castToStmtDDLCreateTable())
    {
      volTabName_ =
	getDDLNode()->castToStmtDDLNode()->castToStmtDDLCreateTable()->getTableNameAsQualifiedName();
      isCreate_ = TRUE;
      isTable_ = TRUE;
    }
  else if (getDDLNode()->castToStmtDDLNode() &&
	   getDDLNode()->castToStmtDDLNode()->castToStmtDDLDropTable())
    {
      volTabName_ =
	getDDLNode()->castToStmtDDLNode()->castToStmtDDLDropTable()->getTableNameAsQualifiedName();
      isTable_ = TRUE;
    }
  else if (getDDLNode()->castToStmtDDLNode() &&
      getDDLNode()->castToStmtDDLNode()->castToStmtDDLCreateIndex())
    {
      volTabName_ =
	getDDLNode()->castToStmtDDLNode()->castToStmtDDLCreateIndex()->getIndexNameAsQualifiedName();
      isCreate_ = TRUE;
      isIndex_  = TRUE;
    }
  else if (getDDLNode()->castToStmtDDLNode() &&
	   getDDLNode()->castToStmtDDLNode()->castToStmtDDLDropIndex())
    {
      volTabName_ =
	getDDLNode()->castToStmtDDLNode()->castToStmtDDLDropIndex()->getIndexNameAsQualifiedName();
      isIndex_  = FALSE;
    }
  else if (getDDLNode()->castToStmtDDLNode() &&
	   getDDLNode()->castToStmtDDLNode()->castToStmtDDLCreateSchema())
    {
      isCreate_ = TRUE;
      isSchema_ = TRUE;

      volTabName_ = 
	QualifiedName(NAString("dummy"),
		      getExprNode()->castToStmtDDLNode()->castToStmtDDLCreateSchema()->getSchemaNameAsQualifiedName().getSchemaName(),
		      getExprNode()->castToStmtDDLNode()->castToStmtDDLCreateSchema()->getSchemaNameAsQualifiedName().getCatalogName());
    }
  else if (getDDLNode()->castToStmtDDLNode() &&
	   getDDLNode()->castToStmtDDLNode()->castToStmtDDLDropSchema())
    {
      isCreate_ = FALSE;
      isSchema_ = TRUE;

      volTabName_ = 
	QualifiedName(NAString("dummy"),
		      getExprNode()->castToStmtDDLNode()->castToStmtDDLDropSchema()->getSchemaNameAsQualifiedName().getSchemaName(),
		      getExprNode()->castToStmtDDLNode()->castToStmtDDLDropSchema()->getSchemaNameAsQualifiedName().getCatalogName());
    }
    
  RelExpr * boundExpr = DDLExpr::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  isHbase_ = TRUE;

  return boundExpr;
}

// -----------------------------------------------------------------------
// member functions for class ExeUtilProcessExceptionTable
// -----------------------------------------------------------------------
RelExpr * ExeUtilProcessExceptionTable::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }


  RelExpr * boundExpr = DDLExpr::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;
  
  return boundExpr;
}

// -----------------------------------------------------------------------
// member functions for class ExeUtilLoadVolatileTable
// -----------------------------------------------------------------------
RelExpr * ExeUtilLoadVolatileTable::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // get the insert query
  NAWchar *inputStr = SQLTEXTW();
  Int32 start_pos = 0;

  CharInfo::CharSet targetCharSet = SqlParser_CurrentParser->charset_;
  if ( targetCharSet == CharInfo::UCS2 )
  {
    targetCharSet = CharInfo::UTF8;
  }
  
  insertQuery_ = unicodeToChar(
       &inputStr[start_pos],
       NAWstrlen(&inputStr[start_pos]),
       targetCharSet,
       bindWA->wHeap());
  
  // get the upd stats query
  updStatsQuery_ = 
    new(bindWA->wHeap()) NAString(bindWA->wHeap());
  *updStatsQuery_ += "UPDATE STATISTICS FOR TABLE ";
  *updStatsQuery_ += getTableName().getQualifiedNameObj().getQualifiedNameAsAnsiString(TRUE);
  *updStatsQuery_ += " ON EVERY KEY SAMPLE SET ROWCOUNT %Ld;";

  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  return boundExpr;
}

// -----------------------------------------------------------------------
// member functions for class ExeUtilCleanupVolatileTables
// -----------------------------------------------------------------------
RelExpr * ExeUtilCleanupVolatileTables::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  if ((type_ == OBSOLETE_TABLES_IN_DEFAULT_CAT) ||
      (type_ == ALL_TABLES_IN_ALL_CATS))
    catName_ = ActiveSchemaDB()->getDefaults().getValue(CATALOG);

  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  return boundExpr;
}

// -----------------------------------------------------------------------
// member functions for class ExeUtilCreateTableAs
// -----------------------------------------------------------------------
RelExpr * ExeUtilCreateTableAs::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  char * stmtText = getStmtText();
  NAString* tempstmt = 0;
  NAString visiQuery = " ";
  NAString attributeList = " ";
  NAString attrListEndPosStmt = " ";
  StmtDDLCreateTable * createTableNode = NULL;
  NABoolean upsertUsingLoadAllowed = TRUE;
  Int32 errorcode = 0;
  NAWcharBuf* wcbuf = 0;
      
  if ((getExprNode()) &&
      (getExprNode()->castToStmtDDLNode()) &&
      (getExprNode()->castToStmtDDLNode()->castToStmtDDLCreateTable()))
    createTableNode =
      getExprNode()->castToStmtDDLNode()->castToStmtDDLCreateTable();

  isVolatile_ = FALSE;
  if (createTableNode && (createTableNode->isVolatile()))
    {    
      isVolatile_ = TRUE;
    }

  CorrName savedTableName = getTableName();
  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  NABoolean isHive = FALSE;
  if ((getTableName().isHive()) &&
      (CmpCommon::getDefault(TRAF_DDL_ON_HIVE_OBJECTS) == DF_ON))
    {
      isHive = TRUE;
      upsertUsingLoadAllowed = FALSE;
    }

  if ((NOT isVolatile_) &&
      (NOT getTableName().isSeabase()) && // can only create traf tables
      (NOT isHive))
    {
      *CmpCommon::diags() << DgSqlCode(-3242) << 
        DgString0(NAString("This DDL operation is not allowed in the specified catalog '" + getTableName().getQualifiedNameObj().getCatalogName() + "'."));

      bindWA->setErrStatus();
      return NULL;
    }

  // open source path
  if ((createTableNode->isInMemoryObjectDefn()) ||
      (createTableNode->isMultiSetTable()) ||
      (createTableNode->isSetTable()))
    {
      return NULL;
    }

  NABoolean isSeabase = FALSE;
  if (getTableName().isSeabase())
    {
      isSeabase = TRUE;
    }

  if (isVolatile_)
    {    
      getTableName() = savedTableName;
    }

  Int32 scannedInputCharset = createTableNode->getCreateTableAsScannedInputCharset();
  Int32 isoMapping = createTableNode->getCreateTableAsIsoMapping();

  size_t asQueryPos = 0;
  if (createTableNode)
    {
      bindWA->setInCTAS(TRUE);
      createTableNode->getQueryExpression()->bindNode(bindWA);
      bindWA->setInCTAS(FALSE);
      if (bindWA->errStatus()) 
	return NULL;
      
      asQueryPos = createTableNode->getStartOfCreateTableQueryPosition();
      
      size_t attrListStartPos =
	createTableNode->getStartOfCreateTableAsAttrListPosition();
      size_t attrListEndPos =
	createTableNode->getEndOfCreateTableAsAttrListPosition();

      CMPASSERT(scannedInputCharset == (Int32)SQLCHARSETCODE_UTF8 &&
                ComGetNameInterfaceCharSet() == SQLCHARSETCODE_UTF8);

      // If column definitions were not explicitely specified, then create
      // column definitions of CREATE TABLE stmt from the AS SELECT query.
      // In addition to no col defns, if attribute list was explicitely specified,
      // then append that to the CREATE TABLE stmt.
      ElemDDLNode *pTableDefBody = 
	(createTableNode->getChild(0/*StmtDDLCreateTable::INDEX_TABLE_DEFINITION*/)
	 ? createTableNode->getChild(0)->castToElemDDLNode()
	 : NULL);
      if ((! pTableDefBody) ||
	  (createTableNode->ctaColumnsAreRenamed()))
	{
          if(createTableNode->isSetTable())
            {
              if(createTableNode->isVolatile())
                ctQuery_ = "CREATE SET VOLATILE TABLE ";
              else
                ctQuery_ = "CREATE SET TABLE ";
            }
          else if(createTableNode->isMultiSetTable() &&
                 (CmpCommon::getDefault(MODE_SPECIAL_1) == DF_ON))
            {
               if(createTableNode->isVolatile())
                 ctQuery_ = "CREATE MULTISET VOLATILE TABLE ";
               else
                 ctQuery_ = "CREATE MULTISET TABLE ";
            }
          else if (createTableNode->isVolatile())
	    ctQuery_ = "CREATE VOLATILE TABLE ";
	  else if ((isHive) && (createTableNode->isExternal()))
            ctQuery_ = "CREATE EXTERNAL TABLE ";
          else
	    ctQuery_ = "CREATE TABLE ";

          if (createTableNode->createIfNotExists())
            {
              ctQuery_ += "IF NOT EXISTS ";
            }

	  ctQuery_ += 
	    getTableName().getQualifiedNameObj().getQualifiedNameAsAnsiString();
	  ctQuery_ += " ";
	  
	  // column definition was not specified.
	  // create col defn from select list of AS query.
	  CMPASSERT(createTableNode->getQueryExpression()->getOperatorType() == REL_ROOT);
	  
	  RelRoot * queryRoot = (RelRoot *) createTableNode->getQueryExpression();
	  RETDesc * retDesc = queryRoot->getRETDesc();
	  CMPASSERT(retDesc->getDegree() > 0);

	  bindWA->setInCTAS(TRUE);
	  castComputedColumnsToAnsiTypes(bindWA, retDesc,
					 queryRoot->compExpr());
	  bindWA->setInCTAS(FALSE);
	  
	  ctQuery_ += "( ";

          NAString hiveType;
	  if (! pTableDefBody)
	    {
	      for (CollIndex i = 0; i < retDesc->getDegree(); i++)
		{
		  NAString colDef = "";
		  
		  const NAString 
		    colName(retDesc->getColRefNameObj(i).getColNameAsAnsiString());
		  if (colName.length() == 0)
		    {
		      // expression in the AS query select list.
		      // It must be renamed.
		      *CmpCommon::diags() << DgSqlCode(-1099)
					  << DgInt0(i+1);
		      bindWA->setErrStatus(); 
		      return NULL;
		    }
		  
		  colDef += ToAnsiIdentifier(colName);
	      
		  colDef += " ";
		  
		  NAType &colType = (NAType&)(queryRoot->compExpr()[i].getType());
                 if (isHive)
                    {
                      colType.getMyTypeAsHiveText(&hiveType);
                      colDef += hiveType;
                    }
                 else
                   colType.getMyTypeAsText(&colDef);

		  if (colType.isLob())
		    upsertUsingLoadAllowed = FALSE;

		  if (i < (retDesc->getDegree() -1))
		    colDef += ", ";
		  else 
		    colDef += " ";
		  
		  ctQuery_ += colDef;
		} // for
	    } // !pTableDefBody
	  else
	    {
	      CollIndex numColDefEntries = 0;
	      CollIndex numUntypedColDefEntries = 0;
	      Int32 j = 0;
	      NABoolean firstEntry = TRUE;
	      CollIndex entries = pTableDefBody->entries();
	      for (CollIndex i = 0; i < entries; i++)
		{
		  NAString colDef = "";
		  
		  ElemDDLNode * currListElem = (*pTableDefBody)[i]->castToElemDDLNode();;
		  if (currListElem->getOperatorType() == ELM_COL_DEF_ELEM)
		    {
		      if (NOT firstEntry)
			colDef += ", ";
		      else
			firstEntry = FALSE;
		      ElemDDLColDef *colDefNode = currListElem->castToElemDDLColDef();

		      numColDefEntries++;
		      if (colDefNode->getColumnDataType() != NULL)
			{
			  colDef += colDefNode->getColDefAsText();

			  if (colDefNode->getColumnDataType()->isLob())
			    upsertUsingLoadAllowed = FALSE;
			}
		      else
			{
			  numUntypedColDefEntries++;

			  if (numUntypedColDefEntries > retDesc->getDegree())
			    {
			      continue;
			    }
			  
                          if (NOT colDefNode->castToElemDDLColDef()->getColumnFamily().isNull())
                            {
                              colDef +=
                                ToAnsiIdentifier(colDefNode->castToElemDDLColDef()->
                                                 getColumnFamily());
                              colDef += ".";
                            }

			  colDef += 
			    ToAnsiIdentifier(colDefNode->castToElemDDLColDef()->
					     getColumnName());
	      
			  colDef += " ";
			  
			  NAType &colType = (NAType&)(queryRoot->compExpr()[j].getType());
			  colType.getMyTypeAsText(&colDef);

			  if (colType.isLob())
			    upsertUsingLoadAllowed = FALSE;
			}

		      j++;

		    } // if ColDefNode
		  else if (currListElem->getOperatorType() == ELM_CONSTRAINT_PRIMARY_KEY_ELEM)
		    {
		      if (NOT firstEntry)
			colDef += ", ";
		      else
			firstEntry = FALSE;
		      ElemDDLNode * pColRefList = 
			currListElem->castToElemDDLConstraintPK()
			->getColumnRefList();
		      CMPASSERT(pColRefList != NULL);

		      colDef += "PRIMARY KEY ( ";
		      for (CollIndex k = 0; k < pColRefList->entries(); k++)
			{
			  if (k > 0)
			    colDef += ", ";
			  colDef +=
			    (*pColRefList)[k]->castToElemDDLColRef()->getColumnName();
			}
		      colDef += " ) ";
		    } // primary key
		  else if (currListElem->getOperatorType() == ELM_CONSTRAINT_UNIQUE_ELEM)
		    {
		      if (NOT firstEntry)
			colDef += ", ";
		      else
			firstEntry = FALSE;
		      ElemDDLNode * pColRefList = 
			currListElem->castToElemDDLConstraintUnique()
			->getColumnRefList();
		      CMPASSERT(pColRefList != NULL);

		      colDef += "UNIQUE ( ";
		      for (CollIndex k = 0; k < pColRefList->entries(); k++)
			{
			  if (k > 0)
			    colDef += ", ";
			  colDef +=
			    (*pColRefList)[k]->castToElemDDLColRef()->getColumnName();
			}
		      colDef += " ) ";
		    } // unique constraint
		  ctQuery_ += colDef;
		} // for

	      if (numUntypedColDefEntries > retDesc->getDegree())
		{
		  *CmpCommon::diags() << DgSqlCode(-1108)
				      << DgInt0(numUntypedColDefEntries)
				      << DgInt1(retDesc->getDegree());

		  bindWA->setErrStatus(); 
		  return NULL;
		}

	      if ((CmpCommon::getDefault(COMP_BOOL_207) == DF_OFF) &&
		  ((numUntypedColDefEntries > 0) &&
		   (numUntypedColDefEntries < entries)))
		{
		  *CmpCommon::diags() << DgSqlCode(-1299);

		  bindWA->setErrStatus(); 
		  return NULL;
		}
	    } // else pTableDefBody

	  ctQuery_ += " ) ";
	  
	  // if attribute list is specified, append that to col definition.
	  if (createTableNode->getChild(1/*StmtDDLCreateTable::INDEX_ATTRIBUTE_LIST*/))
	    {
              ctQuery_ += " ";
              ctQuery_.append(&stmtText[attrListStartPos], 
                              attrListEndPos - attrListStartPos);
            }
	}
      else
	{
	  // column defns were explicitely specified. 
	  // Create the CREATE stmt from the original text of the query.
	  ctQuery_ = "";
	  ctQuery_.append(stmtText, attrListEndPos);
	}

      if (NOT createTableNode->getHiveOptions().isNull())
        {
          if (NOT isHive)
            {
              *CmpCommon::diags() << DgSqlCode(-3242) << 
                DgString0(NAString("WITH HIVE OPTIONS cannot be specified for non-Hive tables."));
              
              bindWA->setErrStatus();
              return NULL;
            }
          else
            {
              ctQuery_ += " ";
              ctQuery_ += createTableNode->getHiveOptions();
            }
        }
       
      if (createTableNode->isInMemoryObjectDefn())
	ctQuery_.append(" IN MEMORY ");

      if (createTableNode->isSetTable())
	upsertUsingLoadAllowed = FALSE;

      loadIfExists_ = createTableNode->loadIfExists();
      noLoad_ = createTableNode->noLoad();
      deleteData_ = createTableNode->deleteData();
    }

  // create a VSBB insert and a sidetree insert stmt, unless sidetree
  // inserts are not allowed.
  // At runtime, we do sidetree inserts if there is no user started
  // transaction. If there is, vsbb inserts are done.
  viQuery_ = "insert into ";
  viQuery_ += getTableName().getQualifiedNameObj().getQualifiedNameAsAnsiString();
  viQuery_ += " ";  

  // if insert column list was specified, append that.
  if (createTableNode->insertColumnsList())
    {
      viQuery_ += " ( ";

      for (CollIndex i = 0; i < createTableNode->insertColumnsList()->entries(); i++)
	{
	  ElemDDLNode * colNameNode = (*(createTableNode->insertColumnsList()))[i];
	  CMPASSERT(colNameNode->getOperatorType() ==  ELM_COL_VIEW_DEF_ELEM);
	  
	  viQuery_ += ToAnsiIdentifier(colNameNode->castToElemDDLColViewDef()->getColumnName());

	  if (i < createTableNode->insertColumnsList()->entries() - 1)
	    viQuery_ += ", ";
	}
      viQuery_ += " ) ";
    }

  viQuery_ += &stmtText[asQueryPos];

  if (upsertUsingLoadAllowed)
    {
      siQuery_ = "upsert using load into ";
      siQuery_ += getTableName().getQualifiedNameObj().getQualifiedNameAsAnsiString();
      siQuery_ += " ";

      // if insert column list was specified, append that.
      if (createTableNode->insertColumnsList())
	{
	  siQuery_ += " ( ";
	  
	  for (CollIndex i = 0; i < createTableNode->insertColumnsList()->entries(); i++)
	    {
	      ElemDDLNode * colNameNode = (*(createTableNode->insertColumnsList()))[i];
	      CMPASSERT(colNameNode->getOperatorType() ==  ELM_COL_VIEW_DEF_ELEM);
	      
	      siQuery_ += ToAnsiIdentifier(colNameNode->castToElemDDLColViewDef()->getColumnName());
	      
	      if (i < createTableNode->insertColumnsList()->entries() - 1)
		siQuery_ += ", ";
	    }
	  siQuery_ += " ) ";
	}

      siQuery_ += &stmtText[asQueryPos];
    }

  if (NOT isSeabase)
    {
      // get the upd stats query
      usQuery_ = "UPDATE STATISTICS FOR TABLE ";
      usQuery_ += getTableName().getQualifiedNameObj().getQualifiedNameAsAnsiString(TRUE);
      if (isHive)
        usQuery_ += " ON EVERY COLUMN SAMPLE SET ROWCOUNT %Ld;";
      else
        usQuery_ += " ON EVERY KEY SAMPLE SET ROWCOUNT %Ld;";        
    }

  return boundExpr;
}
// -----------------------------------------------------------------------
// member functions for class ExeUtilHiveTruncateLegacy
// -----------------------------------------------------------------------
RelExpr * ExeUtilHiveTruncateLegacy::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) 
    {
      bindWA->getCurrentScope()->setRETDesc(getRETDesc());
      return this;
    }

  bindChildren(bindWA);
  if (bindWA->errStatus()) 
    return this;

  NATable *naTable = NULL;

  // do not do override schema for this
  bindWA->setToOverrideSchema(FALSE);
  
  naTable = bindWA->getNATable(getTableName());
  if ((!naTable) || 
      (bindWA->errStatus()))
    return this;
 
  if ((NOT getTableName().isHive()) ||
      (!naTable->isHiveTable()))
    {
      *CmpCommon::diags() << DgSqlCode(-3242) 
                          << DgString0("Truncate is only allowed for hive tables.");
      bindWA->setErrStatus();
      return NULL;
    }

  // If the current user has been granted the Trafodion Hive/DB root role or
  // is DB__ROOT, allow the operation. 
  // If the current user has select and delete privileges, allow the operation
  if (bindWA->currentCmpContext()->isAuthorizationEnabled())
  {
    NABoolean found = FALSE;
    if (ComUser::isRootUserID() ||
        ComUser::currentUserHasRole(HIVE_ROLE_ID) ||
        ComUser::currentUserHasRole(ROOT_ROLE_ID))
      found = TRUE;

    if (!found)
    {
      PrivMgrUserPrivs *pPrivInfo = naTable->getPrivInfo();
      if (pPrivInfo &&
          pPrivInfo->hasPriv(SELECT_PRIV) &&
          pPrivInfo->hasPriv(DELETE_PRIV))
        found = TRUE;

      if (!found)
      {
        *CmpCommon::diags()
           << DgSqlCode( -1051 )
           << DgTableName(naTable->getTableName().getQualifiedNameAsAnsiString());
        bindWA->setErrStatus();
        return NULL;
      }
    }
  }

  const HHDFSTableStats* hTabStats = 
    naTable->getClusteringIndex()->getHHDFSTableStats();
  
  const char * hiveTablePath = (*hTabStats)[0]->getDirName();
  NAString hostName;
  Int32 hdfsPort;
  NAString tableDir;
  HHDFSDiags hhdfsDiags;
  
  NABoolean result = TableDesc::splitHiveLocation
    (hiveTablePath, hostName, hdfsPort, tableDir,
     CmpCommon::diags(), hTabStats->getPortOverride()) ;       
  if (!result) 
    {
      *CmpCommon::diags() << DgSqlCode(-4224)
                          << DgString0(hiveTablePath);
      bindWA->setErrStatus();
      return this;
    }
  
  hiveTableLocation_ = tableDir;
  hiveHostName_ = hostName;
  hiveHdfsPort_ = hdfsPort;
  hiveModTS_ = -1;

  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus())
    return NULL;

  return boundExpr;
}

// -----------------------------------------------------------------------
// member functions for class ExeUtilHiveTruncate
// -----------------------------------------------------------------------
RelExpr * ExeUtilHiveTruncate::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) 
    {
      bindWA->getCurrentScope()->setRETDesc(getRETDesc());
      return this;
    }

  bindChildren(bindWA);
  if (bindWA->errStatus()) 
    return this;

  NATable *naTable = NULL;

  // do not do override schema for this
  bindWA->setToOverrideSchema(FALSE);
  
  naTable = bindWA->getNATable(getTableName());
  if (getIfExists() && (! naTable))
    {
      setTableNotExists(TRUE);

      bindWA->resetErrStatus();
      CmpCommon::diags()->clear();
      RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
      if (bindWA->errStatus())
        return NULL;
      
      return boundExpr;
    }

  if ((!naTable) || 
      (bindWA->errStatus()))
    return this;
 
  if ((NOT getTableName().isHive()) ||
      (!naTable->isHiveTable()))
    {
      *CmpCommon::diags() << DgSqlCode(-3242) 
                          << DgString0("Truncate is only allowed for hive tables.");
      bindWA->setErrStatus();
      return NULL;
    }

  // if no security check is to be done, skip it.
  if (NOT noSecurityCheck_)
    {
      // In Hive, you need admin privs to truncate files.  At this time, we don't
      // know if the current user has admin privileges, return an error.
      char * sentryEnv = getenv("SENTRY_SECURITY_FOR_HIVE");
      if (sentryEnv && strcmp(sentryEnv, "TRUE") == 0)
        {
          *CmpCommon::diags() << DgSqlCode(-3242) 
                              << DgString0("Truncate must be performed through native Hive interface.");
          bindWA->setErrStatus();
          return NULL;
        }
      
      // If the current user has been granted the Trafodion Hive/DB root role or
      // is DB__ROOT, allow the operation. 
      // If the current user has select and delete privileges, allow the operation
      if (bindWA->currentCmpContext()->isAuthorizationEnabled())
        {
          NABoolean found = FALSE;
          if (ComUser::isRootUserID() ||
              ComUser::currentUserHasRole(HIVE_ROLE_ID) ||
              ComUser::currentUserHasRole(ROOT_ROLE_ID))
            found = TRUE;
          
          if (!found)
            {
              PrivMgrUserPrivs *pPrivInfo = naTable->getPrivInfo();
              if (pPrivInfo &&
                  pPrivInfo->hasPriv(SELECT_PRIV) &&
                  pPrivInfo->hasPriv(DELETE_PRIV))
                found = TRUE;
              
              if (!found)
                {
                  *CmpCommon::diags()
                    << DgSqlCode( -1051 )
                    << DgTableName(naTable->getTableName().getQualifiedNameAsAnsiString());
                  bindWA->setErrStatus();
                  return NULL;
                }
            }
        }
    }

  setHiveExternalTable(naTable->isHiveExternalTable());

  // Allocate a TableDesc and attach it to this.
  //
  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus())
    return NULL;

  return boundExpr;
}

// -----------------------------------------------------------------------
// member functions for class ExeUtilMaintainObject
// -----------------------------------------------------------------------
RelExpr * ExeUtilMaintainObject::bindNode(BindWA *bindWA)
{
  *CmpCommon::diags() << DgSqlCode(-4222) << DgString0("Maintain");

  bindWA->setErrStatus();
  return this;
}

// -----------------------------------------------------------------------
// member functions for class ExeUtilGetStatistics
// -----------------------------------------------------------------------
RelExpr * ExeUtilGetStatistics::bindNode(BindWA *bindWA)
{
  if (errorInParams_)
    {
      *CmpCommon::diags() << DgSqlCode(-4218) << DgString0("GET STATISTICS");

      bindWA->setErrStatus();
      return this;
    }

  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  return boundExpr;
}

// -----------------------------------------------------------------------
// member functions for class ExeUtilGetUID
// -----------------------------------------------------------------------
RelExpr * ExeUtilGetUID::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  NATable *naTable = NULL;
  if (type_ == ExeUtilMaintainObject::TABLE_)
    {
      naTable = bindWA->getNATable(getTableName());
      if (bindWA->errStatus()) 
	return this;
    }

  if (type_ == ExeUtilMaintainObject::INDEX_)
    {
      CorrName cn = getTableName();
      cn.setSpecialType(ExtendedQualName::INDEX_TABLE);

      naTable = bindWA->getNATable(cn);
      if (bindWA->errStatus()) 
	return this;

      if (naTable->getClusteringIndex()->notAvailable())
        {
          *CmpCommon::diags() << DgSqlCode(-4082) <<
            DgTableName(naTable->getTableName().getQualifiedNameAsAnsiString());
	  bindWA->setErrStatus();
	  return this;
        }
    }

  if (type_ == ExeUtilMaintainObject::MV_)
    {
      naTable = bindWA->getNATable(getTableName());
      if (bindWA->errStatus()) 
	return this;
      
      if (NOT naTable->isAnMV())
	{
	  *CmpCommon::diags() << DgSqlCode(-4219);
	  
	  bindWA->setErrStatus();
	  return this;
	}
    }

  if (! naTable)
      //      (NOT naTable->isInMemoryObjectDefn()))
    {
      *CmpCommon::diags() << DgSqlCode(-4219);
      
      bindWA->setErrStatus();
      return this;
    }

  uid_ = naTable->objectUid().get_value();

  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  return boundExpr;
}

// -----------------------------------------------------------------------
// member functions for class ExeUtilPopulateInMemStats
// -----------------------------------------------------------------------
RelExpr * ExeUtilPopulateInMemStats::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  const NATable *inMemNATable = bindWA->getNATable(getInMemTableName());
  if (bindWA->errStatus()) 
    return this;

  if (NOT inMemNATable->isInMemoryObjectDefn())
    {
      *CmpCommon::diags() << DgSqlCode(-1004) 
			  << DgTableName(inMemNATable->getTableName().getQualifiedNameAsAnsiString());
      bindWA->setErrStatus();
      return this;
    }

  uid_ = inMemNATable->objectUid().get_value();

  sourceTableName_.applyDefaults(bindWA, bindWA->getDefaultSchema());

  if (sourceStatsSchemaName_.getSchemaName().isNull())
    {
      sourceStatsSchemaName_ = sourceTableName_.getQualifiedNameObj().getSchemaName();
    }

  const NATable *sourceNATable = bindWA->getNATable(sourceTableName_);
  if (bindWA->errStatus()) 
    return this;

  // source and inMem table must be 'similar' before stats could be moved.
  // More checks, if needed, could be added later.
  if ((inMemNATable->getColumnCount() != sourceNATable->getColumnCount()) ||
      (inMemNATable->getRecordLength() != sourceNATable->getRecordLength()) ||
      (NOT (inMemNATable->getNAColumnArray() == 
	    sourceNATable->getNAColumnArray())) ||
      (NOT (inMemNATable->getClusteringIndex()->getIndexKeyColumns() == 
	    sourceNATable->getClusteringIndex()->getIndexKeyColumns())))
      //      (((NAFileSet&)(inMemNATable->getClusteringIndex()))->getKeyLength() != 
      //       ((NAFileSet&)(sourceNATable->getClusteringIndex()))->getKeyLength()))
    {
      *CmpCommon::diags() << DgSqlCode(-8579) 
			  << DgString0("InMemory and source table attributes are different.");
      bindWA->setErrStatus();
      return this;    
    }
      
  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  return boundExpr;
}

// -----------------------------------------------------------------------
// member functions for class ExeUtilWnrInsert
// -----------------------------------------------------------------------
ExeUtilWnrInsert::ExeUtilWnrInsert( const CorrName &name,
                                    RelExpr * child,
                                    CollHeap *oHeap)
            : ExeUtilExpr(WNR_INSERT_, name, NULL, child, 
            NULL, CharInfo::UnknownCharSet, oHeap)
{
}

RelExpr * ExeUtilWnrInsert::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // do not do override schema for this
  bindWA->setToOverrideSchema(FALSE);

  // Allocate a TableDesc and attach it to this.
  NATable *naTable = bindWA->getNATable(getTableName());
  if (bindWA->errStatus()) 
    return this;

  bindWA->getCurrentScope()->xtnmStack()->createXTNM();
  setUtilTableDesc(bindWA->createTableDesc(naTable, getTableName()));
  bindWA->getCurrentScope()->xtnmStack()->removeXTNM();
  if (bindWA->errStatus())
    return this;

  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  return boundExpr;
}

RelExpr * ExeUtilWnrInsert::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilWnrInsert *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilWnrInsert
      (getTableName(), NULL, outHeap);
  else
    result = (ExeUtilWnrInsert *) derivedNode;
  
  return ExeUtilExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// member functions for class ExeUtilGetMetadataInfo
// -----------------------------------------------------------------------
RelExpr * ExeUtilGetMetadataInfo::bindNode(BindWA *bindWA)
{
  if (errorInParams_)
    {
      *CmpCommon::diags() << DgSqlCode(-4218) << DgString0("GET ");

      bindWA->setErrStatus();
      return this;
    }

  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  return boundExpr;
}


// -----------------------------------------------------------------------
// member functions for class ExeUtilAQR
// -----------------------------------------------------------------------
RelExpr * ExeUtilAQR::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  return boundExpr;
}

// -----------------------------------------------------------------------
// member functions for class ExeUtilLongRunning
// -----------------------------------------------------------------------
RelExpr * ExeUtilLongRunning::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // do not do override schema for this
  bindWA->setToOverrideSchema(FALSE);

  // Allocate a TableDesc and attach it to this.
  NATable *naTable = bindWA->getNATable(getTableName());
  if (bindWA->errStatus()) 
    return this;

  // LRU is not allowed in a trigger action
  if (bindWA->isInTrigger() && bindWA->inDDL())
  {
     *CmpCommon::diags() << DgSqlCode(-11018) <<
     DgTableName(naTable->getTableName().getQualifiedNameAsAnsiString());
     bindWA->setErrStatus();
     return this;
  }

  // LRU is not allowed on views
  if (naTable->getObjectType() == COM_VIEW_OBJECT)
  {
     *CmpCommon::diags() << DgSqlCode(-4350) <<
     DgTableName(naTable->getTableName().getQualifiedNameAsAnsiString());
     bindWA->setErrStatus();
     return this;
  }
  
  setUtilTableDesc(bindWA->createTableDesc(naTable, getTableName()));
  if (bindWA->errStatus())
    return this;

  // predicateExpr_ and predicate_ are two representations of the same thing
  // So, if we have one, we better have the other.
  CMPASSERT((predicateExpr_ && predicate_) || (!predicateExpr_ && !predicate_));

  // If we have a predicate, do some sanity checks on it (both the
  // itemexpr tree version and the string version) so that we can
  // detect and report errors here, rather than at runtime.
  if(predicateExpr_)
    {
      ValueIdSet selPred;
      Parser parser(bindWA->currentCmpContext());

      // Expect the predicate string to start with 'WHERE'
      CMPASSERT((predicate_[0] == 'w' || predicate_[0] == 'W') &&
                (predicate_[1] == 'h' || predicate_[1] == 'H') &&
                (predicate_[2] == 'e' || predicate_[2] == 'E'));

      // See if we can parse the predicate string. (Skip over the 'WHERE')
      ItemExpr *pred = parser.getItemExprTree(&predicate_[5]);
      CMPASSERT(pred);

      // See if we can bind the predicate itemexpr tree
      bindWA->getCurrentScope()->context()->inWhereClause() = TRUE;
      predicateExpr_->convertToValueIdSet(selPred, bindWA, ITM_AND);
      bindWA->getCurrentScope()->context()->inWhereClause() = FALSE;

      if (bindWA->errStatus()) 
        {
          // Error binding pred tree, better quit now.
          return this;
        }

      if (selPred.containsSubquery())
        {
          // LRU Delete not supported with subquery in where clause
          *CmpCommon::diags() << DgSqlCode(-4139);

          bindWA->setErrStatus();
          return this;
        }
      if (selPred.containsUDF())
        {
          // LRU Delete not supported with UDF in where clause
          *CmpCommon::diags() << DgSqlCode(-4474)
                              << DgString0(((UDFunction *)selPred.containsUDF())->
                                            getFunctionName().getExternalName());	    
          bindWA->setErrStatus();
          return this;
        }
    }

  // Since we don't really need to project rows from this operator, 
  // allocate an empty RETDesc and attach it to this node and the BindScope.
  //
  setRETDesc(new (bindWA->wHeap()) RETDesc(bindWA));
  bindWA->getCurrentScope()->setRETDesc(getRETDesc());

  tableName_.applyDefaults(bindWA, bindWA->getDefaultSchema());

  // Bind the child nodes.
  bindChildren(bindWA);
  if (bindWA->errStatus())
    return this;
 
  RelExpr *boundExpr = bindSelf(bindWA);

  if (bindWA->errStatus()) 
    return NULL;

  // indicate that this is a an LRU statement
  bindWA->getTopRoot()->setContainsLRU(TRUE);

  return boundExpr;
}

/////////////////////////////////////////////////////////////////////////////
// Normalizer/transform methods for ExeUtil operators
/////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// OptPhysRelExpr methods for ExeUtil operators
//////////////////////////////////////////////////////////////////////////////
PhysicalProperty * ExeUtilLongRunning::synthPhysicalProperty(const Context * context,
                                                             const Lng32 planNumber,
                                                             PlanWorkSpace *pws)
{
   const ReqdPhysicalProperty* rppForMe = context->getReqdPhysicalProperty();

   if ( rppForMe->executeInDP2() )
      return NULL;

   // ---------------------------------------------------------
   // decide on my partitioning function
   // ---------------------------------------------------------
   PartitioningFunction *myPartFunc = 
         getUtilTableDesc()->getClusteringIndex()->getPartitioningFunction();

   if ( myPartFunc == NULL ) // single partition table case
   {
      //----------------------------------------------------------
      // Create a node map with a single, active, wild-card entry.
      //----------------------------------------------------------
      NodeMap* myNodeMap = new(CmpCommon::statementHeap())
                              NodeMap(CmpCommon::statementHeap(),
                                      1,
                                      NodeMapEntry::ACTIVE);

      //------------------------------------------------------------
      // The table is not partitioned. Do not need to start ESPs.
      // Synthesize a partitioning function with a single partition.
      //------------------------------------------------------------
      myPartFunc = new(CmpCommon::statementHeap())
                             SinglePartitionPartitioningFunction(myNodeMap);
   }

   // Decide on execution location: single partition table: in MASTER;
   //                               partitioned table: in ESP
   PlanExecutionEnum loc = ( myPartFunc->getCountOfPartitions() <= 1 ) ?
                           EXECUTE_IN_MASTER : EXECUTE_IN_ESP;


   PhysicalProperty * sppForMe = new(CmpCommon::statementHeap())
                                       PhysicalProperty(myPartFunc,
                                                        loc,
                                                        SOURCE_VIRTUAL_TABLE);
   // remove anything that's not covered by the group attributes
   sppForMe->enforceCoverageByGroupAttributes(getGroupAttr());
   return sppForMe;
}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilLobExtract
// -----------------------------------------------------------------------
RelExpr * ExeUtilLobExtract::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilLobExtract *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilLobExtract(NULL, NOOP_,
					     NULL, NULL, 0, 0, NULL, NULL, NULL,NULL,
					     outHeap);
  else
    result = (ExeUtilLobExtract *) derivedNode;

  result->handle_ = handle_;
  result->toType_ = toType_;
  result->bufAddr_ = bufAddr_;
  result->extractSizeAddr_ = extractSizeAddr_;
  result->intParam_ = intParam_;
  result->intParam2_ = intParam2_;
  result->stringParam_ = stringParam_;
  result->stringParam2_ = stringParam2_;
  result->stringParam3_ = stringParam3_;
  result->handleInStringFormat_ = handleInStringFormat_;
  result->withCreate_ = withCreate_;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

RelExpr * ExeUtilLobExtract::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // BindScope *currScope = bindWA->getCurrentScope();
 
  //
  // Bind the child nodes.
  //
  bindChildren(bindWA);
  if (bindWA->errStatus())
    return this;

  //  currScope = bindWA->getCurrentScope();
  // bindWA->getCurrentScope()->setRETDesc(getRETDesc());
  
  if (handle_)
    {
      handle_->bindNode(bindWA);
      if (bindWA->errStatus())
	return NULL;
    }

  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus())
    return NULL;

  return boundExpr;
}

void ExeUtilLobExtract::transformNode(NormWA & normWARef,
				      ExprGroupId & locationOfPointerToMe)
{
  RelExpr::transformNode(normWARef, locationOfPointerToMe);

  /*
  ExprValueId nePtr(handle_);                   // normalized expr.
   
  handle_->getReplacementExpr()->
    transformNode(normWARef, nePtr, locationOfPointerToMe,
		  getGroupAttr()->getCharacteristicInputs());			
  if ((const ItemExpr *)handle_ != nePtr)
    handle_ = nePtr;
  */
}

RelExpr * ExeUtilLobExtract::normalizeNode(NormWA & normWARef)
{
  
  return RelExpr::normalizeNode(normWARef);
}

void ExeUtilLobExtract::pushdownCoveredExpr(const ValueIdSet & outputExpr,
					    const ValueIdSet & newExternalInputs,
					    ValueIdSet & predicatesOnParent,
					    const ValueIdSet * setOfValuesReqdByParent,
					    Lng32 childIndex
					    )
{
  ValueIdSet exprOnParent;

  if (handle_) // && handle_->child(0))
    {
      exprOnParent += handle_->getValueId(); //child(0)->getValueId();
    }

  // ---------------------------------------------------------------------
  RelExpr::pushdownCoveredExpr(outputExpr,
                               newExternalInputs,
                               predicatesOnParent,
			       &exprOnParent,
                               childIndex
                               );

}


// -----------------------------------------------------------------------
// Member functions for class ExeUtilLobUpdate
// -----------------------------------------------------------------------
RelExpr * ExeUtilLobUpdate::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilLobUpdate *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilLobUpdate(NULL, NOOP_,
                                             0, 0,ERROR_IF_EXISTS_,NULL,
					     outHeap);
  else
    result = (ExeUtilLobUpdate *) derivedNode;

  result->handle_ = handle_;
  result->fromType_ = fromType_;
  result->bufAddr_ = bufAddr_;
  result->updateSize_ = updateSize_;
 
  return ExeUtilExpr::copyTopNode(result, outHeap);
}

RelExpr * ExeUtilLobUpdate::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // BindScope *currScope = bindWA->getCurrentScope();
 
  //
  // Bind the child nodes.
  //
  bindChildren(bindWA);
  if (bindWA->errStatus())
    return this;

  //  currScope = bindWA->getCurrentScope();
  // bindWA->getCurrentScope()->setRETDesc(getRETDesc());
  
  if (handle_)
    {
      handle_->bindNode(bindWA);
      if (bindWA->errStatus())
	return NULL;
    }

  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus())
    return NULL;

  return boundExpr;
}

void ExeUtilLobUpdate::transformNode(NormWA & normWARef,
				      ExprGroupId & locationOfPointerToMe)
{
  RelExpr::transformNode(normWARef, locationOfPointerToMe);

}

RelExpr * ExeUtilLobUpdate::normalizeNode(NormWA & normWARef)
{
  
  return RelExpr::normalizeNode(normWARef);
}

void ExeUtilLobUpdate::pushdownCoveredExpr(const ValueIdSet & outputExpr,
					    const ValueIdSet & newExternalInputs,
					    ValueIdSet & predicatesOnParent,
					    const ValueIdSet * setOfValuesReqdByParent,
					    Lng32 childIndex
					    )
{
  ValueIdSet exprOnParent;

  if (handle_) // && handle_->child(0))
    {
      exprOnParent += handle_->getValueId(); //child(0)->getValueId();
    }

  // ---------------------------------------------------------------------
  RelExpr::pushdownCoveredExpr(outputExpr,
                               newExternalInputs,
                               predicatesOnParent,
			       &exprOnParent,
                               childIndex
                               );

}
// -----------------------------------------------------------------------
// Member functions for class ExeUtilLobShowddl
// -----------------------------------------------------------------------
RelExpr * ExeUtilLobShowddl::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilLobShowddl *result;

  if (derivedNode == NULL)
    result = new (outHeap) 
      ExeUtilLobShowddl(getTableName(), sdOptions_, outHeap);
  else
    result = (ExeUtilLobShowddl *) derivedNode;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

RelExpr * ExeUtilLobShowddl::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  bindChildren(bindWA);
  if (bindWA->errStatus()) 
    return this;

  NATable *naTable = NULL;

  naTable = bindWA->getNATable(getTableName());
  if (bindWA->errStatus())
    return this;

  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus())
    return NULL;

  // Allocate a TableDesc and attach it to this.
  //
  setUtilTableDesc(bindWA->createTableDesc(naTable, getTableName()));
  if (bindWA->errStatus())
    return this;

  objectUID_ = naTable->objectUid().get_value();

  return boundExpr;
}


// -----------------------------------------------------------------------
// Member functions for class ExeUtilHbaseCoProcAggr
// -----------------------------------------------------------------------
RelExpr * ExeUtilHbaseCoProcAggr::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilHbaseCoProcAggr *result;

  if (derivedNode == NULL)
  {
    result = new (outHeap) 
      ExeUtilHbaseCoProcAggr(getTableName(), aggregateExpr(), outHeap);
    result->setEstRowsAccessed(getEstRowsAccessed());
  }
  else
    result = (ExeUtilHbaseCoProcAggr *) derivedNode;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

RelExpr * ExeUtilHbaseCoProcAggr::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  NATable *naTable = NULL;

  naTable = bindWA->getNATable(getTableName());
  if (bindWA->errStatus())
    return this;

  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus())
    return NULL;

  // Allocate a TableDesc and attach it to this.
  //
  setUtilTableDesc(bindWA->createTableDesc(naTable, getTableName()));
  if (bindWA->errStatus())
    return this;

  // BindWA keeps list of coprocessors used, so privileges can be checked.
  bindWA->insertCoProcAggr(this);

  Int32 retryLimitMilliSeconds = 5000; // use at most 5 seconds on retries
  Int32 errorCode = 0;
  Int32 breadCrumb = 0;
  CostScalar rowsAccessed(naTable->estimateHBaseRowCount(retryLimitMilliSeconds,errorCode /* out */,breadCrumb /* out */));
  setEstRowsAccessed(rowsAccessed);

  return boundExpr;
}

void ExeUtilHbaseCoProcAggr::getPotentialOutputValues(
						      ValueIdSet & outputValues) const
{
  outputValues.clear();
  
  outputValues += aggregateExpr();
} // HbaseAccessCoProcAggr::getPotentialOutputValues()

// -----------------------------------------------------------------------
// Member functions for class ExeUtilOrcFastAggr
// -----------------------------------------------------------------------
RelExpr * ExeUtilOrcFastAggr::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilOrcFastAggr *result;

  if (derivedNode == NULL)
    result = new (outHeap) 
      ExeUtilOrcFastAggr(getTableName(), aggregateExpr(), outHeap);
  else
    result = (ExeUtilOrcFastAggr *) derivedNode;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

RelExpr * ExeUtilOrcFastAggr::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  NATable *naTable = NULL;

  naTable = bindWA->getNATable(getTableName());
  if (bindWA->errStatus())
    return this;

  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus())
    return NULL;

  // Allocate a TableDesc and attach it to this.
  //
  setUtilTableDesc(bindWA->createTableDesc(naTable, getTableName()));
  if (bindWA->errStatus())
    return this;

  return boundExpr;
}

void ExeUtilOrcFastAggr::getPotentialOutputValues(
						      ValueIdSet & outputValues) const
{
  outputValues.clear();
  
  outputValues += aggregateExpr();
} // ExeUtilOrcFastAggr::getPotentialOutputValues()

// -----------------------------------------------------------------------
// Member functions for class ExeUtilHbaseDDL
// -----------------------------------------------------------------------
RelExpr * ExeUtilHbaseDDL::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilHbaseDDL *result;

  if (derivedNode == NULL)
    result = new (outHeap) 
      ExeUtilHbaseDDL(getTableName(), type_, csl_, outHeap);
  else
    result = (ExeUtilHbaseDDL *) derivedNode;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

// --------------------------------------------------------------------------------
// Member functions for class ExeUtilMetadataUpgrade
// --------------------------------------------------------------------------------
RelExpr * ExeUtilMetadataUpgrade::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilMetadataUpgrade *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilMetadataUpgrade(outHeap);
  else
    result = (ExeUtilMetadataUpgrade *) derivedNode;

  result->myFlags_ = myFlags_;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}
// -----------------------------------------------------------------------
// Member functions for class ExeUtilHbaseLoad
// -----------------------------------------------------------------------
RelExpr * ExeUtilHBaseBulkLoad::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilHBaseBulkLoad *result;

  if (derivedNode == NULL)
    result = new (outHeap)
      ExeUtilHBaseBulkLoad(getTableName(),
                      getExprNode(),
                      NULL, CharInfo::UnknownCharSet, pQueryExpression_, outHeap);
  else
    result = (ExeUtilHBaseBulkLoad *) derivedNode;

  result->keepHFiles_ = keepHFiles_;
  result->truncateTable_ = truncateTable_;
  result->noRollback_= noRollback_;
  result->continueOnError_ = continueOnError_ ;
  result->logErrorRowsLocation_= logErrorRowsLocation_;
  result->logErrorRows_ = logErrorRows_;
  result->noDuplicates_= noDuplicates_;
  result->rebuildIndexes_= rebuildIndexes_;
  result->hasUniqueIndexes_ = hasUniqueIndexes_;
  result->constraints_= constraints_;
  result->noOutput_= noOutput_;
  result->indexTableOnly_= indexTableOnly_;
  result->upsertUsingLoad_= upsertUsingLoad_;
  result->pQueryExpression_ = pQueryExpression_;
  result->maxErrorRows_= maxErrorRows_;
  return ExeUtilExpr::copyTopNode(result, outHeap);
}

RelExpr * ExeUtilHBaseBulkLoad::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  RelExpr * boundExpr = NULL;

  // If user does not have the MANAGE_LOAD component privilege, setup stoi for
  // privileges needed to perform load operation
  if (!checkForComponentPriv(SQLOperation::MANAGE_LOAD, bindWA))
  {
    // Load requires select and insert privileges
    // Load requires delete if TRUNCATE is specified
    SqlTableOpenInfo::AccessFlags privs;
    privs.select_ = 1;
    privs.insert_ = 1;
    privs.update_ = 0;
    privs.delete_ = truncateTable_;

    setupStoiForPrivs(privs, bindWA);
    if (bindWA->errStatus())
      return NULL;

    getQueryExpression()->bindNode(bindWA);
  }

  if (bindWA->errStatus())
    return NULL;

  boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus())
    return NULL;

  // Allocate a TableDesc and attach it to this.
  NATable *naTable = bindWA->getNATable(getTableName());
  if (bindWA->errStatus()) 
    return this;

  setUtilTableDesc(bindWA->createTableDesc(naTable, getTableName()));
  if (bindWA->errStatus())
    return this;
  if ((CmpCommon::getDefault(TRAF_LOAD_ALLOW_RISKY_INDEX_MAINTENANCE) == DF_OFF) && 
      getUtilTableDesc()->hasSecondaryIndexes() && 
      !(getUtilTableDesc()->isIdentityColumnGeneratedAlways() && 
        getUtilTableDesc()->hasIdentityColumnInClusteringKey()) &&
       !getUtilTableDesc()->getClusteringIndex()->getNAFileSet()->hasSyskey())
    setRebuildIndexes(TRUE) ;
  if (!getRebuildIndexes())
  setHasUniqueIndexes(getUtilTableDesc()->hasUniqueIndexes());

  return boundExpr;
}

const NAString ExeUtilHBaseBulkLoad::getText() const
{
  NAString result(CmpCommon::statementHeap());

  result = "HBASE_BULK_LOAD";

  return result;
}

short ExeUtilHBaseBulkLoad::setOptions(NAList<ExeUtilHBaseBulkLoad::HBaseBulkLoadOption*> *
    hBaseBulkLoadOptionList,   ComDiagsArea * da)
{
  if (!hBaseBulkLoadOptionList)
    return 0;

  for (CollIndex i = 0; i < hBaseBulkLoadOptionList->entries(); i++)
  {
    HBaseBulkLoadOption * lo = (*hBaseBulkLoadOptionList)[i];
    switch (lo->option_)
    {
      case NO_ROLLBACK_:
      {
        if (getNoRollback())
        {
          //4488 bulk load option $0~String0 cannot be specified more than once.
          *da << DgSqlCode(-4488)
                  << DgString0("NO ROLLBACK");
          return 1;
        }
        setNoRollback(TRUE);
      }
      break;
      case TRUNCATE_TABLE_:
      {
        if (getTruncateTable())
        {
          //4488 bulk load option $0~String0 cannot be specified more than once.
          *da << DgSqlCode(-4488)
                  << DgString0("TRUNCATE TABLE");
          return 1;
        }
        setTruncateTable(TRUE);
      }
      break;
      case INDEX_TABLE_ONLY_:
      {
        if (getIndexTableOnly())
        {
          //4488 bulk load option $0~String0 cannot be specified more than once.
          *da << DgSqlCode(-4488)
                  << DgString0("INDEX TABLE ONLY");
          return 1;
        }
        setIndexTableOnly(TRUE);
      }
      break;
      case NO_DUPLICATE_CHECK_:
      {
        if (!getNoDuplicates())
        {
          //4488 bulk load option $0~String0 cannot be specified more than once.
          *da << DgSqlCode(-4488)
                  << DgString0("SKIP DUPLICATES");
          return 1;
        }
        setNoDuplicates(FALSE);
      }
      break;
      case REBUILD_INDEXES_:
      {
        if (getRebuildIndexes())
        {
          //4488 bulk load option $0~String0 cannot be specified more than once.
          *da << DgSqlCode(-4488)
                  << DgString0("REBUILD INDEXES");
          return 1;
        }
        setRebuildIndexes(TRUE);
      }
      break;
      case CONSTRAINTS_:
      {
        *da << DgSqlCode(-4485)
        << DgString0(": CONSTRAINTS.");
        return 1;
      }
      break;
      case NO_OUTPUT_:
      {
        if (getNoOutput())
        {
          //4488 bulk load option $0~String0 cannot be specified more than once.
          *da << DgSqlCode(-4488)
                  << DgString0(" NO OUTPUT  ");
          return 1;
        }
        setNoOutput(TRUE);
      }
      break;
      case STOP_AFTER_N_ERROR_ROWS_:
      {
        if (getMaxErrorRows() != 0)
        {
          //4488 bulk load option $0~String0 cannot be specified more than once.
          *da << DgSqlCode(-4488)
                  << DgString0("MAX ERROR ROWS");
          return 1;
        }
        setMaxErrorRows(lo->numericVal_);
      }
      break;
      case LOG_ERROR_ROWS_:
      {
        if (getLogErrorRows())
        {
          //4488 bulk load option $0~String0 cannot be specified more than once.
          *da << DgSqlCode(-4488)
                  << DgString0("LOG ERROR ROWS");
          return 1;
        }
        setContinueOnError(TRUE);
        setLogErrorRows(TRUE);
        if (lo->stringVal_ == NULL)
           logErrorRowsLocation_ = CmpCommon::getDefaultString(TRAF_LOAD_ERROR_LOGGING_LOCATION);
        else
        {
           if (strlen(lo->stringVal_) > 512) {
              *da << DgSqlCode(-4487)
                  << DgString0(lo->stringVal_);
              return -1;
           }
           logErrorRowsLocation_ = lo->stringVal_;
        }
        //GIVE ERROR if EQUAL to BULKLOAD tmp location
      }
      break;
      case CONTINUE_ON_ERROR_:
      {
        if (getContinueOnError())
        {
          //4488 bulk load option $0~String0 cannot be specified more than once.
          *da << DgSqlCode(-4488)
                  << DgString0("CONTINUE ON ERROR");
          return 1;
        }
        setContinueOnError(TRUE);
      }
      break;
      case UPSERT_USING_LOAD_:
      {
        if (getUpsertUsingLoad())
        {
          //4488 bulk load option $0~String0 cannot be specified more than once.
          *da << DgSqlCode(-4488)
                  << DgString0("UPSERT USING LOAD");
          return 1;
        }
        setUpsertUsingLoad(TRUE);
      }
      break;
      case UPDATE_STATS_:
      {
        if (getUpdateStats())
        {
          //4488 bulk load option $0~String0 cannot be specified more than once.
          *da << DgSqlCode(-4488)
                  << DgString0("UPDATE STATISTICS");
          return 1;
        }
        setUpdateStats(TRUE);
      }
      break;
      default:
      {
        CMPASSERT(0);
        return 1;
      }

    }
  }

  // Update stats not allowed with upsert load.
  if (getUpdateStats() && getUpsertUsingLoad())
    {
      // 4492 BULK LOAD option UPDATE STATISTICS cannot be used with UPSERT USING LOAD option.
      *da << DgSqlCode(-4492);
      return 1;
    }

  if (getLogErrorRows() || getMaxErrorRows() > 0)
  {
    setContinueOnError(TRUE);
  }
  if (getIndexTableOnly())
  {
    // target table is index then : no output, no secondary index maintenance
    // and no constraint maintenance
    setNoOutput(TRUE);
    setRebuildIndexes(FALSE);
    setConstraints(FALSE);
  }

  return 0;

};

RelExpr * ExeUtilHBaseBulkLoadTask::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // Allocate a TableDesc and attach it to this.
  NATable *naTable = bindWA->getNATable(getTableName());
  if (bindWA->errStatus()) 
    return this;

  setUtilTableDesc(bindWA->createTableDesc(naTable, getTableName()));
  if (bindWA->errStatus())
    return this;

  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  return boundExpr;
}

RelExpr * ExeUtilHBaseBulkLoadTask::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilHBaseBulkLoadTask *result;

  if (derivedNode == NULL)
    result = new (outHeap)
    ExeUtilHBaseBulkLoadTask(getTableName(),
                            getExprNode(),
                            NULL,
                            CharInfo::UnknownCharSet,
                            NOT_SET_,
                            outHeap);
  else
    result = (ExeUtilHBaseBulkLoadTask *) derivedNode;

  result->taskType_ = taskType_;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}


const NAString ExeUtilHBaseBulkLoadTask::getText() const
{
  NAString result(CmpCommon::statementHeap());

  if (taskType_ == PRE_LOAD_CLEANUP_)
       result = "CLEANUP";
  else
       result = "COMPLETE HBASE LOAD";

  return result;
}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilHbaseLoad
// -----------------------------------------------------------------------
RelExpr * ExeUtilHBaseBulkUnLoad::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilHBaseBulkUnLoad *result;

  if (derivedNode == NULL)
    result = new (outHeap)
      ExeUtilHBaseBulkUnLoad(getTableName(),
                      getExprNode(),
                      NULL, CharInfo::UnknownCharSet, outHeap);
  else
    result = (ExeUtilHBaseBulkUnLoad *) derivedNode;

  result->emptyTarget_ = emptyTarget_;
  result->logErrors_ = logErrors_ ;
  //result->compress_ = compress_ ;
  result->noOutput_= noOutput_;
  result->oneFile_= oneFile_;
  result->compressType_ = compressType_;
  result->extractLocation_ = extractLocation_;
  result->mergePath_ = mergePath_;
  result->overwriteMergeFile_ = overwriteMergeFile_;
  result->snapSuffix_= snapSuffix_;
  result->scanType_ = scanType_;
  result->pQueryExpression_ = pQueryExpression_;
  return ExeUtilExpr::copyTopNode(result, outHeap);
}

RelExpr * ExeUtilHBaseBulkUnLoad::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  RelExpr * boundExpr = NULL;

  // If user does not have the MANAGE_LOAD component privilege, call the
  // binder to bind the associated query expression.  The binder returns
  // any privilege errors
  if (!checkForComponentPriv(SQLOperation::MANAGE_LOAD, bindWA))
  {
    // Verify that current user has privileges by binding the
    // query expression attached to request
    getQueryExpression()->bindNode(bindWA);
  }

  boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus())
    return NULL;

  return boundExpr;
}

const NAString ExeUtilHBaseBulkUnLoad::getText() const
{
  NAString result(CmpCommon::statementHeap());

  result = "HBASE_BULK_UNLOAD";

  return result;
}
void ExeUtilHBaseBulkUnLoad::buildWithClause(NAString & withStr, char * str)
{
  if (withStr.length() == 0 )
  {
    withStr = " WITH ";
    withStr += str;
  }
  else
  {
    withStr += " ";
    withStr += str;
  }
}

short ExeUtilHBaseBulkUnLoad::setOptions(NAList<UnloadOption*>  *
    hBaseBulkUnLoadOptionList,   ComDiagsArea * da)
{

  NAString delimiterSqlStr(CmpCommon::statementHeap());
  NAString nullStringSqlStr(CmpCommon::statementHeap());
  NAString recordSeparatorSqlStr(CmpCommon::statementHeap());
  NAString withClauseStr(CmpCommon::statementHeap());

  if (hBaseBulkUnLoadOptionList)
  {
    for (CollIndex i = 0; i < hBaseBulkUnLoadOptionList->entries(); i++)
    {
      UnloadOption * lo = (*hBaseBulkUnLoadOptionList)[i];
      switch (lo->option_)
      {
      case UnloadOption::DELIMITER_:
      {

        if (delimiterSqlStr.length() == 0)
        {
          delimiterSqlStr = " DELIMITER ";
          if (lo->numericVal_ == 0)
          {
            if (strlen(lo->stringVal_) != 1 )
            {
              char c;
              if (!PhysicalFastExtract::isSpecialChar(lo->stringVal_, c))
              {
                if (lo->stringVal_[0]== '\\')
                {
                  //if it start with '\' and not a valid escape character produce an error
                  //4374 - Invalid escape sequence specified as BULK UNLOAD field delimiter or record separator.
                  //       Only the following escape sequences are allowed: \a, \b, \f, \n, \r, \t, or \v.
                  *da << DgSqlCode(-4374);
                  return 1;
                }
                else
                {
                  // 4379 - Invalid BULK UNLOAD field delimiter or record separator.
                  //        A valid field delimiter or record separator must be a single character or an integer between 1 and 255.
                  *da << DgSqlCode(-4379);
                  return 1;
                }
              }
            }
            delimiterSqlStr.append(" '");
            delimiterSqlStr.append( lo->stringVal_);
            delimiterSqlStr.append("' ");
          }
          else
          {
            char buf[5];
            sprintf(buf,"%u",(unsigned char) lo->stringVal_[0]);
            delimiterSqlStr.append( buf);
            delimiterSqlStr.append(" ");
          }
          buildWithClause(withClauseStr,(char*)delimiterSqlStr.data());
        }
        else
        {
          *da << DgSqlCode(-4376) << DgString0("DELIMITER");
          return 1;
        }
      }
      break;
      case UnloadOption::NULL_STRING_:
      {
        if (nullStringSqlStr.length() == 0)
        {
          nullStringSqlStr = " NULL_STRING ";
          nullStringSqlStr.append(" '");
          nullStringSqlStr.append( lo->stringVal_);
          nullStringSqlStr.append("' ");
          buildWithClause(withClauseStr,(char*)nullStringSqlStr.data());
        }
        else
        {
          *da << DgSqlCode(-4376) << DgString0("NULL_STRING");
          return 1;
        }
      }
      break;
      case  UnloadOption::RECORD_SEP_:
      {
        if (recordSeparatorSqlStr.length() == 0)
        {
          recordSeparatorSqlStr = " RECORD_SEPARATOR ";
          if (lo->numericVal_ == 0)
          {
            if (strlen(lo->stringVal_) != 1 )
            {
              char c;
              if (!PhysicalFastExtract::isSpecialChar(lo->stringVal_, c))
              {
                if (lo->stringVal_[0]== '\\')
                {
                  //if it start with '\' and not a valid escape character produce an error
                  //4374 - Invalid escape sequence specified as BULK UNLOAD field delimiter or record separator.
                  //       Only the following escape sequences are allowed: \a, \b, \f, \n, \r, \t, or \v.
                  *da << DgSqlCode(-4374);
                  return 1;
                }
                else
                {
                  // 4379 - Invalid BULK UNLOAD field delimiter or record separator.
                  //        A valid field delimiter or record separator must be a single character or an integer between 1 and 255.
                  *da << DgSqlCode(-4379);
                  return 1;
                }
              }
            }
            recordSeparatorSqlStr.append(" '");
            recordSeparatorSqlStr.append( lo->stringVal_);
            recordSeparatorSqlStr.append("' ");
          }
          else
          {
            char buf[5];
            sprintf(buf,"%u",(unsigned char) lo->stringVal_[0]);
            recordSeparatorSqlStr.append( buf);
            recordSeparatorSqlStr.append(" ");
          }
          buildWithClause(withClauseStr,(char*)recordSeparatorSqlStr.data());
        }
        else
        {
          *da << DgSqlCode(-4376) << DgString0("RECORD_SEPARATOR");
          return 1;
        }
      }
      break;
      case  UnloadOption::EMPTY_TARGET_:
      {
        if (getEmptyTarget())
        {
          //4489 bulk unload option $0~String0 cannot be specified more than once.
          *da << DgSqlCode(-4489)
                          << DgString0("EMPTY TARGET");
          return 1;
        }
        setEmptyTarget(TRUE);
      }
      break;

      case  UnloadOption::NO_OUTPUT_:
      {
        if (getNoOutput())
        {
          //4489 bulk unload option $0~String0 cannot be specified more than once.
          *da << DgSqlCode(-4489)
                          << DgString0("NO OUTPUT");
          return 1;
        }
        setNoOutput(TRUE);
      }
      break;
      case  UnloadOption::LOG_ERRORS_:
      {
        *da << DgSqlCode(-4485)
                << DgString0(": ERROR LOGGING.");
        return 1;
      }
      break;
      case  UnloadOption::APPEND_:
      {
        *da << DgSqlCode(-4485)
                << DgString0(": APPEND.");
        return 1;
      }
      break;
      case  UnloadOption::HEADER_:
      {
        *da << DgSqlCode(-4485)
                << DgString0(": HEADER.");
        return 1;
      }
      break;
      case  UnloadOption::COMPRESS_:
      {
        if (getCompressType()!= NONE_)
        {
          //4489 bulk load option $0~String0 cannot be specified more than once.
          *da << DgSqlCode(-4489)
                          << DgString0("COMPRESS");
          return 1;
        }
        //only GZIP is supported for now
        setCompressType(GZIP_);
      }
      break;
      case  UnloadOption::ONE_FILE_:
      {
        if (getOneFile())
        {
          //4489 bulk unload option $0~String0 cannot be specified more than once.
          *da << DgSqlCode(-4489)
              << DgString0("ONE FILE");
          return 1;
        }
        setOneFile(TRUE);
        mergePath_ = lo->stringVal_;
        setOverwriteMergeFile(lo->numericVal_);

      }
      break;
      case UnloadOption::USE_SNAPSHOT_SCAN_:
      {
        if (scanType_ == REGULAR_SCAN_)
          scanType_ = (ScanType)lo->numericVal_;
        else
        {
          *da << DgSqlCode(-4376) << DgString0("SNAPSHOT SCAN");
          return 1;
        }

        if (lo->stringVal_ != NULL)
          snapSuffix_ = lo->stringVal_;
        else
          snapSuffix_ = CmpCommon::getDefaultString(TRAF_TABLE_SNAPSHOT_SCAN_SNAP_SUFFIX);
      }
      break;
      default:
      {
        CMPASSERT(0);
        return 1;
      }
      }
    }
    if (snapSuffix_.length() == 0)
      snapSuffix_ = CmpCommon::getDefaultString(TRAF_TABLE_SNAPSHOT_SCAN_SNAP_SUFFIX);

    if (getOneFile())
    {
      if (mergePath_.contains("/"))
      {
        NAString msg = "MERGE FILE '";
        msg = msg + mergePath_;
        msg = msg + "' cannot contain the '/' character.";
        //4487 Invalid file name
        *da << DgSqlCode(-4487)
            << DgString0((const char* )msg.data() );
        return 1;

      }
      if (getCompressType()!= NONE_)
      {
        // currently only gzip compression is supported
        //verify the merge path ends with .gz
        if (!(mergePath_.length() > 3 &&
            strcmp(&(mergePath_.data()[mergePath_.length() -3 ]), ".gz") == 0))
        {
          NAString msg = "MERGE FILE '";
          msg = msg + mergePath_;
          msg = msg + "' must end with the characters '.gz'.";
          //4487 Invalid file name
          *da << DgSqlCode(-4487)
              << DgString0((const char* )msg.data());
          return 1;
        }
      }
      if (extractLocation_.data()[extractLocation_.length()-1] != '/')
        mergePath_.prepend("/");
      mergePath_.prepend(extractLocation_);
    }
  }

  char * stmt = this->getStmtText();

  for ( char *s = stmt; *s ; s++ )
  {
    if (*s=='\n' || *s=='\t' || *s == '\r')
    {
      *s = ' ';
    }
  }

  NAString stmt1 = this->getStmtText();
  UInt32 pos = stmt1.index(" into ", 0, NAString::ignoreCase) ;
  pos += strlen(" into ");

  NAString uldQuery = " ";
  uldQuery.append((char*)&(stmt1.data()[pos]));
  uldQuery.prepend(" TO ");
  if (withClauseStr.length() != 0)
  {
    uldQuery.prepend(withClauseStr);
  }
  uldQuery.prepend("UNLOAD EXTRACT ");

  this->setStmtText((char*)uldQuery.data(), this->getStmtTextCharSet());

  return 0;
};


