/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2008-2014 Hewlett-Packard Development Company, L.P.
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
#include "OptimizerSimulator.h"
#include "charinfo.h"
#include "SqlParserGlobals.h"		// must be last #include
#include "ItmFlowControlFunction.h"
#include "HDFSHook.h"


NAWchar *SQLTEXTW();
void castComputedColumnsToAnsiTypes(BindWA *bindWA,
				    RETDesc *rd,
				    ValueIdList &compExpr);

#define TEXT_DISPLAY_LENGTH 1001


// -----------------------------------------------------------------------
// Member functions for class GenericUtilExpr
// -----------------------------------------------------------------------

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
  outputValues.clear(); // for now, it produces no output values
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
  //
  // Allocate an empty RETDesc and attach it to this node and the BindScope.
  //
  setRETDesc(new (bindWA->wHeap()) RETDesc(bindWA));
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
                                   CharInfo::UnknownCharSet, // CharInfo::CharSet ddlStmtTextCharSet
                                   FALSE, FALSE, FALSE, NULL, 0, outHeap);
  else
    result = (DDLExpr *) derivedNode;

  result->specialDDL() = specialDDL();

  result->ddlObjNATable_ = ddlObjNATable_;

  result->forShowddlExplain_ = forShowddlExplain_;
  result->internalShowddlExplain_ = internalShowddlExplain_;
  result->noLabelStats_ = noLabelStats_;
  result->explObjName_ = explObjName_;
  result->numExplRows_ = numExplRows_;

  result->objName_ = objName_;
  result->isVolatile_ = isVolatile_;
  result->isTable_ = isTable_;
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
  result->qualObjName_ = qualObjName_;
  result->purgedataTableName_ = purgedataTableName_;
  result->isHbase_ = isHbase_;
  result->isNative_ = isNative_;
  result->hbaseDDLNoUserXn_ = hbaseDDLNoUserXn_;
  result->initHbase_ = initHbase_;
  result->dropHbase_ = dropHbase_;
  result->updateVersion_ = updateVersion_;
  result->purgedataHbase_ = purgedataHbase_;
  result->addSeqTable_ = addSeqTable_;
  result->flags_ = flags_;

  return GenericUtilExpr::copyTopNode(result, outHeap);
}

const NAString DDLExpr::getText() const
{
  NAString result(CmpCommon::statementHeap());

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

void ExeUtilExpr::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  outputValues.clear();
  if ((getVirtualTableDesc()) &&
      (((ExeUtilExpr*)this)->producesOutput()))
    outputValues.insertList(getVirtualTableDesc()->getColumnList());
} // ExeUtilExpr::getPotentialOutputValues()

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

  return GenericUtilExpr::copyTopNode(result, outHeap);
}

const NAString ExeUtilExpr::getText() const
{
  NAString result(CmpCommon::statementHeap());

  switch (type_)
    {
    case LOAD_:
      result = "LOAD";
      break;

    case REORG_:
      result = "REORG";
      break;

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

    case FAST_DELETE_:
    {
      if (((ExeUtilFastDelete*)this)->isHiveTable())
      {
        result = "HIVE_TRUNCATE";
      }
      else
      {
        result = "FAST_DELETE";
      }
    }
      break;

    case GET_STATISTICS_:
      result = "GET_STATISTICS";
      break;

    case LONG_RUNNING_:
      result = "EXE_LONG_RUNNING";
      break;

    case USER_LOAD_:
      result = "USER_LOAD";
      break;

    case GET_METADATA_INFO_:
      result = "GET_METADATA_INFO";
      break;

    case GET_VERSION_INFO_:
      result = "GET_VERSION_INFO";
      break;

    case GET_DISK_LABEL_STATS_:
      result = "GET_DISK_LABEL_STATS_";
      break;

    case GET_DP2_STATS_:
      result = "GET_DP2_STATS";
      break;
      
    case GET_FORMATTED_DISK_STATS_:
      result = "GET_FORMATTED_DISK_STATS";
      break;
      
    case POP_IN_MEM_STATS_:  
      result = "POP_IN_MEM_STATS";
      break;

    case REPLICATE_:
      result = "REPLICATE";
      break;

    case ST_INSERT_:
      result = "ST_INSERT";
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
    case LOAD_:
      result += "Load statement";
      break;

    case REORG_:
      result += "Reorg statement";
      break;

    case DISPLAY_EXPLAIN_:
      result += "Explain command";
      break;

    case DISPLAY_EXPLAIN_COMPLEX_:
      result += "Explain complex command";
      break;

    case USER_LOAD_:
      result += "User Load statement";
      break;

    case LONG_RUNNING_:
      result += "Long Running statement";

    default:
      break;
    }
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
	  {
            shortFormat_ = TRUE;
	  }
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
// Member functions for class ExeUtilLoad
// -----------------------------------------------------------------------
RelExpr * ExeUtilLoad::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilLoad(getTableName(),
				       getExprNode(), NULL, CharInfo::UnknownCharSet, outHeap);
  else
    result = (ExeUtilLoad *) derivedNode;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilLoad
// -----------------------------------------------------------------------
RelExpr * ExeUtilUserLoad::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilUserLoad *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilUserLoad(getTableName(),
					   NULL, eodExpr_, partnNumExpr_,
					   NULL, outHeap);
  else
    result = (ExeUtilUserLoad *) derivedNode;

  result->maxRowsetSize_ = maxRowsetSize_;
  result->rwrsInputSizeExpr_ = rwrsInputSizeExpr_;
  result->rwrsMaxInputRowlenExpr_ = rwrsMaxInputRowlenExpr_;
  result->rwrsBufferAddrExpr_ = rwrsBufferAddrExpr_;
  
  result->excpTabNam_ = excpTabNam_;
  result->excpTabInsertStmt_ = excpTabInsertStmt_;

  result->excpTabDesc_ = excpTabDesc_;

  result->excpRowsPercentage_ = excpRowsPercentage_;
  result->excpRowsNumber_ = excpRowsNumber_;
  result->loadId_ = loadId_;

  result->ingestNumSourceProcesses_ = ingestNumSourceProcesses_;
  result->ingestTargetProcessIds_ = ingestTargetProcessIds_;

  result->flags_ = flags_;
  
  return ExeUtilExpr::copyTopNode(result, outHeap);
}

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
// Member functions for class ExeUtilFastDelete
// -----------------------------------------------------------------------
RelExpr * ExeUtilFastDelete::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilFastDelete *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilFastDelete(getTableName(),
					     getExprNode(), NULL, CharInfo::UnknownCharSet,
					     doPurgedataCat_,
					     noLog_,
					     ignoreTrigger_,
					     isPurgedata_,
					     outHeap,
					     isHiveTable_,
					     &hiveTableLocation_,
                                             &hiveHostName_,
                                             hiveHdfsPort_);
  else
    result = (ExeUtilFastDelete *) derivedNode;

  result->doParallelDelete_ = doParallelDelete_;
  result->doParallelDeleteIfXn_ = doParallelDeleteIfXn_;
  result->offlineTable_ = offlineTable_;
  result->doLabelPurgedata_ = doLabelPurgedata_;

  result->numLOBs_ = numLOBs_;
  result->lobNumArray_ = lobNumArray_;
  result->isHiveTable_ = isHiveTable_;
  result->hiveTableLocation_= hiveTableLocation_;
  result->hiveHostName_ = hiveHostName_;
  result->hiveHdfsPort_ = hiveHdfsPort_;


  return ExeUtilExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
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
// Member functions for class ExeUtilGetReorgStatistics
// -----------------------------------------------------------------------
ExeUtilGetReorgStatistics::ExeUtilGetReorgStatistics(NAString statementName,
						     char * optionsStr,
						     CollHeap *oHeap)
  : ExeUtilGetStatistics(statementName, NULL, 
			 oHeap, 
			 SQLCLI_STATS_REQ_QID, SQLCLI_SAME_STATS)
{
  compilerStats_ = FALSE;
  executorStats_ = FALSE;
  otherStats_    = FALSE;
  detailedStats_ = FALSE;
  
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

	  if (option == "DT")
	    {
	      detailedStats_ = TRUE;
	    }
	  else if (option == "TF")
	    {
	      tokenizedFormat_ = TRUE;
	    }

	  currIndex += 3;
	} // while
    }
}

RelExpr * ExeUtilGetReorgStatistics::copyTopNode(RelExpr *derivedNode,
						 CollHeap* outHeap)
{
  ExeUtilGetReorgStatistics *result;

  if (derivedNode == NULL)
    result = new (outHeap) 
      ExeUtilGetReorgStatistics(statementName_,
				(char*)(optionsStr_.data()),
				outHeap);
  else
    result = (ExeUtilGetReorgStatistics *) derivedNode;

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
       optionX_('n'), optionsStr_(NULL)
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

  result->optionX_ = optionX_;

  return ExeUtilExpr::copyTopNode(result, outHeap);
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

// -----------------------------------------------------------------------
// Member functions for class ExeUtilReorg
// -----------------------------------------------------------------------
ExeUtilReorg::ExeUtilReorg(
     ReorgObjectType type,
     const CorrName &name,
     NAList<ReorgOption*> * reorgOptionsList,
     QualNamePtrList * additionalTables,
     CollHeap *oHeap)
     : ExeUtilExpr(REORG_, name, NULL, NULL, NULL, CharInfo::UnknownCharSet, oHeap),
       type_(type),
       firstPartn_(0), lastPartn_(0),
       concurrency_(-2),
       rate_(40),  // default is 40
       dslack_(0),
       islack_(0),
       delay_(10),
       noDealloc_(FALSE),
       priority_(-1),
       partnName_(oHeap),
       displayOnly_(FALSE),
       noOutputMsg_(FALSE),
       returnSummary_(FALSE),
       returnDetailOutput_(FALSE),
       doCheck_(FALSE),
       doReorg_(FALSE),
       reorgIfNeeded_(FALSE),
       getStatus_(FALSE),
       statusAfterTS_(0),
       suspend_(FALSE),
       stop_(FALSE),
       newReorg_(FALSE),
       statusSummaryOptions_(0),
       doCompact_(FALSE),
       doOverride_(FALSE),
       updateReorgDB_(TRUE),
       useReorgDB_(FALSE),
       returnFast_(FALSE),
       rowsetSize_(100), // default
       firstTable_(-1), 
       lastTable_(-1),
       wherePred_(NULL),
       generateMaintainCommands_(FALSE),
       showMaintainCommands_(FALSE),
       runMaintainCommands_(FALSE),
       maxMaintainTables_(-1),
       reorgCheckAll_(FALSE),
       generateCheckCommands_(FALSE),
       concurrentCheckSessions_(-1),
       compressionType_(-1),
       continueOnError_(FALSE),
       systemObjectsOnly_(FALSE),
       debugOutput_(FALSE),
       initialize_(FALSE),
       reinitialize_(FALSE),
       drop_(FALSE),
       createView_(FALSE),
       dropView_(FALSE),
       cleanupReorgDB_(FALSE),
       cleanupToTS_(-1),
       maxTables_(100),
       verify_(FALSE),
       stoiList_(CmpCommon::statementHeap())
{
  Lng32 num;
  num = (Lng32)CmpCommon::getDefaultNumeric(MAINTAIN_REORG_PRIORITY);
  if (num >= 0)
    {
      priority_ = num;
    }

  num = (Lng32)CmpCommon::getDefaultNumeric(MAINTAIN_REORG_PRIORITY_DELTA);
  if (num != 0)
    {
      if (num < 0)
	priority_ = -1000 + num;
      else
	priority_ = 1000 + num;
    }

  num = (Lng32)CmpCommon::getDefaultNumeric(MAINTAIN_REORG_RATE);
  if (num >= 0)
    {
      rate_ = num;
    }

  num = (Lng32)CmpCommon::getDefaultNumeric(MAINTAIN_REORG_SLACK);
  if (num >= 0)
    {
      dslack_ = num;
      islack_ = num;
    }

  if (CmpCommon::getDefault(REORG_IF_NEEDED) == DF_ON)
    reorgIfNeeded_ = TRUE;
  else
    reorgIfNeeded_ = FALSE;

  NABoolean continueOnErrorSpecified = FALSE;
  updateDBspecified_ = FALSE;

  if (reorgOptionsList)
    {
      for (CollIndex i = 0; i < reorgOptionsList->entries(); i++)
	{
	  ReorgOption * fro = (*reorgOptionsList)[i];
	  switch (fro->option_)
	    {
	    case PARTITION_:
	      {
		if (fro->stringVal_)
		  {
		    partnName_ = fro->stringVal_;
		  }
		else
		  {
		    firstPartn_ = ((Lng32)fro->numericVal_ & 0xFFFF0000) >> 16;
		    lastPartn_ = (Lng32)fro->numericVal_ & 0xFFFF;
		  }
	      }
	    break;

	    case CONCURRENCY_:
	      concurrency_ = (Lng32)fro->numericVal_;
	      break;

	    case RATE_:
	      rate_ = (Lng32)fro->numericVal_;
	      break;

	    case SLACK_:
	      dslack_ = (Lng32)fro->numericVal_;
	      islack_ = (Lng32)fro->numericVal_;
	      break;

	    case DSLACK_:
	      dslack_ = (Lng32)fro->numericVal_;
	      break;

	    case ISLACK_:
	      islack_ = (Lng32)fro->numericVal_;
	      break;

	    case DELAY_:
	      delay_ = (Lng32)fro->numericVal_;
	      break;

	    case NO_DEALLOC_:
	      noDealloc_ = TRUE;
	      break;

	    case PRIORITY_:
	      priority_ = (Lng32)fro->numericVal_;
	      if ((priority_ > 200) ||
		  (priority_ <= 0))
		priority_ = -1;

	      break;

	    case PRIORITY_DELTA_:
	      {
		if (fro->numericVal_ < 0)
		  priority_ = -1000 + (Lng32)fro->numericVal_;
		else
		  priority_ = 1000 + (Lng32)fro->numericVal_;
	      }
	      break;

	    case DISPLAY_:
	      displayOnly_ = TRUE;
	      break;

	    case NO_OUTPUT_:
	      noOutputMsg_ = TRUE;
	      break;

	    case RETURN_SUMMARY_:
	      returnSummary_ = TRUE;

	      if (fro->stringVal_)
		statusSummaryOptionsStr_ = fro->stringVal_;
	      break;
	      
	    case RETURN_DETAIL_OUTPUT_:
	      returnDetailOutput_ = TRUE;
	      break;

	    case REORG_IF_NEEDED_:
	      if (fro->numericVal_ == 0)
		reorgIfNeeded_ = FALSE;
	      else
		reorgIfNeeded_ = TRUE;
	      break;

	    case CHECK_:

	      if (fro->numericVal_ > 0)
		{
		  // check all partitions
		  firstPartn_ = 1;
		  lastPartn_ = (Lng32)fro->numericVal_;
		}
	      else if (fro->numericVal_ == -1)
		{
		  // check all partitions.
		  firstPartn_ = -1;
		  lastPartn_ = -1;
		}
	      else
		{
		  // check default number of partitions. See ExExeUtilReorg
                  // in executor for logic to determine default number.
		  firstPartn_ = 0;
		  lastPartn_ = 0;
		}

	      doCheck_ = TRUE;
	      break;

	    case WHERE_:
	      wherePred_ = (void*)fro->numericVal_;
	      break;

	    case GET_TABLES_:
	      {
		firstTable_ = (Lng32)fro->numericVal_;
		lastTable_ = fro->numericVal2_;
	      }
	    break;

	    case UPDATE_DB_:
	      updateDBspecified_ = TRUE;

	      if (fro->numericVal_ == 0)
		updateReorgDB_ = FALSE;
	      else if (fro->numericVal_ == -1)
		updateReorgDB_ = TRUE;
	      else if (fro->numericVal_ == -2)
	        {
		  updateReorgDB_ = TRUE;
		  rowsetSize_ = 0; // dont use rowsets
		}
	      else if (fro->numericVal_ > 0)
	        {
		  updateReorgDB_ = TRUE;
		  rowsetSize_ = (Lng32)fro->numericVal_;
		}
	      
	      break;

	    case USE_DB_:
	      useReorgDB_ = TRUE;
	      break;

	    case RETURN_FAST_:
	      if (fro->numericVal_ == 0)
		returnFast_ = FALSE;
	      else if (fro->numericVal_ == -1)
		returnFast_ = TRUE;
	      break;

	    case GENERATE_MAINTAIN_COMMANDS_:
	      generateMaintainCommands_ = TRUE;
	      break;

	    case SHOW_MAINTAIN_COMMANDS_:
	      showMaintainCommands_ = TRUE;
	      break;

	    case RUN_MAINTAIN_COMMANDS_:
	      runMaintainCommands_ = TRUE;
	      break;

	    case MAX_MAINTAIN_TABLES_:
	      maxMaintainTables_ = (Lng32)fro->numericVal_;
	      break;

	    case GENERATE_CHECK_COMMANDS_:
	      generateCheckCommands_ = TRUE;
	      break;

	    case CONCURRENT_CHECK_SESSIONS_:
	      concurrentCheckSessions_ = (Lng32)fro->numericVal_;
	      break;

	    case MAX_TABLES_:
	      maxTables_ = fro->numericVal_;
	      break;

	    case GET_STATUS_:
	      getStatus_ = TRUE;

	      statusAfterTS_ = 0;
	      if (fro->stringVal_)
		{
		  statusAfterTS_ = -1;
		  UInt32 fractionPrec;
		  DatetimeValue ts
		    (fro->stringVal_,
		     REC_DATE_YEAR, REC_DATE_SECOND, fractionPrec);

		  if (ts.isValid())
		    {
		      statusAfterTS_ = 
			DatetimeType::julianTimestampValue
			((char*)ts.getValue(), ts.getValueLen(), fractionPrec);
							  
		      if (statusAfterTS_ <= 0)
			statusAfterTS_ = -1;
		    }
		}
	      
	      break;

	    case SUSPEND_:
	      suspend_ = TRUE;
	      break;

	    case STOP_:
	      stop_ = TRUE;
	      break;

	    case NEW_REORG_:
	      newReorg_ = TRUE;
	      break;

            case COMPACT_:
              doCompact_ = TRUE;
              break;

            case OVERRIDE_:
              doOverride_ = TRUE;
              break;
       
            case COMPRESS_:
              compressionType_ = (Lng32)fro->numericVal_;
              break;

	    case CONTINUE_ON_ERROR_:
	      {
		if (fro->numericVal_ != 0)
		  continueOnError_ = TRUE;
		else
		  continueOnError_ = FALSE;
	      }
	    break;

	    case SYSTEM_OBJECTS_ONLY_:
	      {
		systemObjectsOnly_ = TRUE;
	      }
	    break;

	    case DEBUG_OUTPUT_:
	      {
		debugOutput_ = TRUE;
	      }
	    break;

	    case INITIALIZE_DB_:
	      {
		initialize_ = TRUE;
	      }
	    break;

	    case REINITIALIZE_DB_:
	      {
		reinitialize_ = TRUE;
	      }
	    break;

	    case DROP_DB_:
	      {
		drop_ = TRUE;
	      }
	    break;

	    case CREATE_DB_VIEW_:
	      {
		createView_ = TRUE;
	      }
	    break;

	    case DROP_DB_VIEW_:
	      {
		dropView_ = TRUE;
	      }
	    break;
            
            case VERIFY_:
               verify_ = TRUE;
               break;

	    case CLEANUP_REORG_DB_:
	      {
		cleanupReorgDB_ = TRUE;

		if (fro->stringVal_)
		  cleanupToTS_ = *(Int64*)fro->stringVal_;
	      }
	    break;

	    default:
	      break;
	    } // switch
	} // for
    } // if

  if (NOT doCheck_)
  {
    doReorg_ = TRUE;
    rowsetSize_ = 0; 
  }

  if (generateMaintainCommands_)
    {
      if (NOT updateDBspecified_)
	updateReorgDB_ = FALSE;
    }

  if (noOutputMsg_)
    {
      returnSummary_ = FALSE;
      returnDetailOutput_ = FALSE;
    }
  else if ((NOT returnSummary_) && (NOT returnDetailOutput_))
    {
      returnDetailOutput_ = TRUE;
    }

  additionalTablesDescs_ = NULL;
  if (additionalTables)
    additionalTables_ = *additionalTables;
}

RelExpr * ExeUtilReorg::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilReorg *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilReorg(type_, getTableName(),
					NULL, NULL, outHeap);
  else
    result = (ExeUtilReorg *) derivedNode;

  result->setParams(concurrency_, firstPartn_, lastPartn_,
		    rate_, dslack_, islack_, delay_, 
		    noDealloc_,
		    priority_, partnName_,
		    displayOnly_, noOutputMsg_,
		    returnSummary_, returnDetailOutput_,
		    doCheck_, doReorg_, reorgIfNeeded_,
		    getStatus_,
		    suspend_, stop_, newReorg_, doCompact_, doOverride_,
                    compressionType_, updateReorgDB_, useReorgDB_,
		    returnFast_,
		    rowsetSize_, firstTable_, lastTable_,
		    wherePredStr_,
		    generateMaintainCommands_, 
		    showMaintainCommands_, runMaintainCommands_,
		    maxMaintainTables_,
		    reorgCheckAll_,
		    generateCheckCommands_, concurrentCheckSessions_,
                    continueOnError_, systemObjectsOnly_, debugOutput_,
                    verify_);
  
  result->updateDBspecified_ = updateDBspecified_;
  result->initialize_ = initialize_;
  result->reinitialize_ = reinitialize_;
  result->drop_ = drop_;
  result->createView_ = createView_;
  result->dropView_ = dropView_;
  result->cleanupReorgDB_ = cleanupReorgDB_;
  result->cleanupToTS_ = cleanupToTS_;
  result->maxTables_ = maxTables_;

  result->additionalTables_ = additionalTables_;
  result->additionalTablesDescs_ = additionalTablesDescs_;

  result->statusAfterTS_ = statusAfterTS_;
  result->statusSummaryOptions_ = statusSummaryOptions_;
  result->stoiList_ = stoiList_;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

void ExeUtilReorg::setParams(Lng32 concurrency,
			     Lng32 firstPartn, Lng32 lastPartn,
			     Lng32 rate, Lng32 dslack, Lng32 islack,
			     Lng32 delay,
			     NABoolean noDealloc,
			     Lng32 priority,
			     NAString &partnName,
			     NABoolean displayOnly, NABoolean noOutputMsg,
			     NABoolean returnSummary, NABoolean returnDetailOutput,
			     NABoolean doCheck, NABoolean doReorg,
			     NABoolean reorgIfNeeded,
			     NABoolean getStatus,
			     NABoolean suspend, NABoolean stop, 
                             NABoolean newReorg,
			     NABoolean doCompact, NABoolean doOverride,
                             Lng32 compressionType,
			     NABoolean updateReorgDB,
                             NABoolean useReorgDB,
			     NABoolean returnFast,
			     Lng32 rowsetSize,
			     Lng32 firstTable,
			     Lng32 lastTable,
			     NAString wherePredStr,
			     NABoolean generateMaintainCommands,
			     NABoolean showMaintainCommands,
			     NABoolean runMaintainCommands,
			     Lng32 maxMaintainTables,
			     NABoolean reorgCheckAll,
			     NABoolean generateCheckCommands,
			     Lng32 concurrentCheckSessions,
			     NABoolean continueOnError,
                             NABoolean systemObjectsOnly,
			     NABoolean debugOutput,
                             NABoolean verify)
{
  concurrency_ = concurrency;
  firstPartn_ = firstPartn;
  lastPartn_ = lastPartn;
  rate_ = rate;
  dslack_ = dslack;
  islack_ = islack;
  delay_ = delay;
  noDealloc_ = noDealloc;
  priority_ = priority;
  partnName_ = partnName;
  displayOnly_ = displayOnly;
  noOutputMsg_ = noOutputMsg;
  returnSummary_ = returnSummary_;
  returnDetailOutput_ = returnDetailOutput;
  doCheck_ = doCheck;
  doReorg_ = doReorg;
  reorgIfNeeded_ = reorgIfNeeded;
  getStatus_ = getStatus;
  suspend_ = suspend;
  stop_ = stop;
  newReorg_  = newReorg;
  doCompact_  = doCompact;
  doOverride_  = doOverride;
  compressionType_ = compressionType;
  updateReorgDB_ = updateReorgDB;
  useReorgDB_ = useReorgDB;
  returnFast_ = returnFast;
  rowsetSize_ = rowsetSize;
  firstTable_ = firstTable;
  lastTable_ = lastTable;
  wherePredStr_ = wherePredStr;
  generateMaintainCommands_ = generateMaintainCommands;
  showMaintainCommands_ = showMaintainCommands;
  runMaintainCommands_ = runMaintainCommands;
  maxMaintainTables_ = maxMaintainTables;
  reorgCheckAll_ = reorgCheckAll;
  generateCheckCommands_ = generateCheckCommands;
  concurrentCheckSessions_ = concurrentCheckSessions;
  continueOnError_ = continueOnError;
  systemObjectsOnly_ = systemObjectsOnly;
  debugOutput_ = debugOutput;
  verify_ = verify;
}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilReplicate
// -----------------------------------------------------------------------
void ExeUtilReplicate::setErrorInParams(NABoolean v, const char * reason)
{
  errorInParams_ = v;

  reason_ = " Reason: ";

  reason_ += reason;
}

ExeUtilReplicate::ExeUtilReplicate(
     NAList<ReplicateOption*> * replicateOptionsList,
     CollHeap *oHeap)
     : ExeUtilExpr(REPLICATE_, CorrName("DUMMY"), NULL, NULL, NULL, CharInfo::UnknownCharSet, oHeap),

       sourceType_(TABLE_),
       targetType_(TABLE_),

       purgedataTgt_(FALSE),
       incremental_(FALSE),

       srcObjsInList_(FALSE),

       notLikePattern_(FALSE),
       notInList_(FALSE),

       srcImpUnqIxPos_(0),

       rate_(-999999),  
       priority_(-999999),
       priorityDelta_(-999999),
       concurrency_(-999999),
       delay_(-999999),

       compress_(TRUE),
       compressSpecified_(FALSE),
       compressionTypeSpecified_(FALSE),
       compressionType_(COM_SOURCE_COMPRESSION),
       diskPoolSpecified_(FALSE),
       diskPool_(1), 
       getStatus_(FALSE),
       suspend_(FALSE),
       resume_(FALSE),
       abort_(FALSE),

       validateTgtDDL_(TRUE),
       validateDDL_(FALSE),
       validateDDLSpecified_(FALSE),
       validateAndPurgedata_(FALSE),
       validateData_(FALSE),

       onlySpecified_(FALSE),

       tgtObjsOnline_(FALSE), tgtObjsOffline_(FALSE),

       tgtObjsAudited_(FALSE), tgtObjsUnaudited_(FALSE),

       noValidateRole_(FALSE),

       returnPartnDetails_(FALSE),

       getDetails_(FALSE),

       debugTarget_(FALSE),

       disableSchemaCreate_(FALSE),

       numSrcPartns_(0),

       recoverType_(0),

       transform_(FALSE), noTransform_(FALSE), tgtActions_(FALSE),

       initialize_(FALSE), reinitialize_(FALSE), drop_(FALSE),

       addConfig_(FALSE), removeConfig_(FALSE),
       portNum_(0),

       cleanup_(FALSE),

       validatePrivs_(FALSE),

       statistics_(FALSE),
       tgtStatsType_(0),
       tgtDDLType_(0),

       copyDDL_(FALSE),
       copyData_(FALSE),
       copyBothDataAndStats_(FALSE),

       errorInParams_(FALSE),
       SQTargetType_(TRUE),
       waitedStartup_(FALSE),
       authids_(FALSE)
{
  Lng32 patternCount = 0;
  Lng32 numControlCommands = 0;
  Lng32 numSchCopyCommands = 0;

  NABoolean validateOrCreateSpecified = FALSE;

  version_ = (short)CmpCommon::getDefaultNumeric(REPLICATE_IO_VERSION); 

  if (replicateOptionsList)
    {
      for (CollIndex i = 0; i < replicateOptionsList->entries(); i++)
	{
	  ReplicateOption * fro = (*replicateOptionsList)[i];
	  switch (fro->option_)
	    {
	    case SOURCE_OBJECT_:
	      {
		sourceName_ = *(CorrName*)fro->stringVal_;
		if (sourceName_.isEmpty())
		  {
		    setErrorInParams(TRUE, "Invalid Source Name specified.");
		    return;
		  }
		  
		if (fro->numericVal_ == 1)
		  sourceType_ = TABLE_;
		else if (fro->numericVal_ == 2)
		  sourceType_ = INDEX_;
		else if (fro->numericVal_ == 3)
		  sourceType_ = MV_;
		else
		  {
		    setErrorInParams(TRUE, "Invalid Source object type specified.");
		    return;
		  }
	      }
	    break;

	    case SOURCE_OBJ_IN_LIST_:
	      {
		if (CmpCommon::getDefault(COMP_BOOL_222) == DF_OFF)
		  {
		    setErrorInParams(TRUE, "List of source tables not supported.");
		    return;
		  }
		  
		//		ConstStringList * csl = (ConstStringList*)fro->stringVal_;
		QualNamePtrList * csl = (QualNamePtrList*)fro->stringVal_;
		if (csl->entries() == 0)
		  {
		    setErrorInParams(TRUE, "Empty list of source tables.");
		    return;
		  }

		if (csl->entries() > 100)
		  {
		    setErrorInParams(TRUE, "Number of objects specified in the list exceed 100.");
		    return;
		  }

		if (fro->numericVal_ == 1)
		  sourceType_ = TABLE_;
		else if (fro->numericVal_ == 2)
		  sourceType_ = INDEX_;
		else if (fro->numericVal_ == 3)
		  sourceType_ = MV_;
		else
		  {
		    setErrorInParams(TRUE, "Invalid Source object type specified.");
		    return;
		  }

		srcInList_ = *csl;
	      }
	    break;

	    case TARGET_OBJECT_:
	      {
		if (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
		  {
		    setErrorInParams(TRUE, "Target object name not supported.");
		    return;
		  }
		  
		targetName_ = *(CorrName*)fro->stringVal_;
		if (targetName_.isEmpty())
		  {
		    setErrorInParams(TRUE, "Empty target name.");
		    return;
		  }

		if (fro->numericVal_ == 1)
		  targetType_ = TABLE_;
		else if (fro->numericVal_ == 2)
		  targetType_ = INDEX_;
		else if (fro->numericVal_ == 3)
		  targetType_ = MV_;
		else
		  {
		    setErrorInParams(TRUE, "Invalid target object type.");
		    return;
		  }
	      }
	    break;

	    case PURGEDATA_TARGET_:
	      {
		if (purgedataTgt_)
		  {
		    setErrorInParams(TRUE, "Duplicate Purgedata option specified.");
		    return;
		  }
		  
		if (incremental_)
		  {
		    setErrorInParams(TRUE, "Either Purgedata or Incremental can be specified"
		                           ", not both.");
		    return;
		  }
		  
		purgedataTgt_ = TRUE;
	      }
	      break;

	    case INCREMENTAL_:
	      {
	        
		if (incremental_)
		  {
		    setErrorInParams(TRUE, "Duplicate Incremental option specified.");
		    return;
		  }
		  
		if (copyDDL_)
		  {
		    setErrorInParams(TRUE, "Incremental option cannot be specified with Copy DDL");
		    return;
		  }
		  
		if (statistics_)
		  {
		    setErrorInParams(TRUE, "Incremental option cannot be specified with Copy Statistics");
		    return;
		  }
		  
		if (purgedataTgt_)
		  {
		    setErrorInParams(TRUE, "Either Purgedata or Incremental can be specified"
		                           ", not both.");
		    return;
		  }
		  
		if (transform_)
		  {
		    setErrorInParams(TRUE, "Either Transform or Incremental can be specified"
		                           ", not both.");
		    return;
		  }
		  
		incremental_ = TRUE;
	      }
	      break;

	    case SOURCE_SCHEMA_:
	      {
		if ((CmpCommon::getDefault(COMP_BOOL_222) == DF_OFF) &&
		    (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
		  {
		    setErrorInParams(TRUE, "Source Schema specification not supported.");
		    return;
		  }
		  
		sourceSchema_ = *(SchemaName*)fro->stringVal_;
		if (sourceSchema_.getSchemaName().isNull())
		  {
		    setErrorInParams(TRUE, "Empty source schema name.");
		    return;
		  }
		if ((NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
                    && (NOT targetSchema_.getSchemaName().isNull()))
                {
                   if (strcmp(sourceSchema_.getSchemaName(),
                          targetSchema_.getSchemaName()) == 0)
                      setErrorInParams(TRUE, "Source and Target schema can't be the same.");
                }
	      }
              break;
           case TARGET_SCHEMA_:
	      {
                if (!targetSchema_.getSchemaName().isNull())
                {
		   setErrorInParams(TRUE, "Target schema is already specified");
		   return;
                }
		targetSchema_ = *(SchemaName*)fro->stringVal_;
		if (targetSchema_.getSchemaName().isNull())
		{
		   setErrorInParams(TRUE, "Empty target schema name.");
		   return;
		}
                if ((NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
                  && (NOT sourceSchema_.getSchemaName().isNull()))
                {
                   if (strcmp(sourceSchema_.getSchemaName(),
                          targetSchema_.getSchemaName()) == 0)
                      setErrorInParams(TRUE, "Source and Target schema can't be the same.");
                }
	      }
              break;
            case AUTHORIZATION_:
              authids_ = TRUE;
              break;
	    case LIKE_PATTERN_:
	    case NOT_LIKE_PATTERN_:
	      {
		if (CmpCommon::getDefault(COMP_BOOL_222) == DF_OFF)
		  {
		    setErrorInParams(TRUE, "LIKE pattern not supported.");
		    return;
		  }
		  
		if (fro->stringVal_ == NULL)
		  {
		    setErrorInParams(TRUE, "Empty LIKE pattern.");
		    return;
		  }

		likePattern_ = "'";
		likePattern_ += (char*)fro->stringVal_;
		likePattern_ += "'";

		if (fro->stringVal2_ != NULL)
		  {
		    escChar_ = "'";
		    escChar_ += (char*)fro->stringVal2_;
		    escChar_ += "'";
		  }

		if (fro->option_ == NOT_LIKE_PATTERN_)
		  notLikePattern_ = TRUE;
		else 
		  notLikePattern_ = FALSE;

		patternCount++;
	      }
	    break;

	    case IN_LIST_:
	    case NOT_IN_LIST_:
	      {
		if (CmpCommon::getDefault(COMP_BOOL_222) == DF_OFF)
		  {
		    setErrorInParams(TRUE, "IN list not supported.");
		    return;
		  }
		  
		if (CmpCommon::getDefault(COMP_BOOL_223) == DF_OFF)
		  {
		    setErrorInParams(TRUE, "IN list not supported.");
		    return;
		  }

		ConstStringList * csl = (ConstStringList*)fro->stringVal_;
		if (csl->entries() == 0)
		  {
		    setErrorInParams(TRUE, "Empty IN list.");
		    return;
		  }

		if (csl->entries() > 100)
		  {
		    setErrorInParams(TRUE, "Number of objects specified in the list exceed 100.");
		    return;
		  }

		inList_ = "(";
		for (CollIndex i = 0; i < csl->entries(); i++)
		  {
		    inList_ += "'";
		    inList_ += *((*csl)[i]);
		    inList_ += "'";
		    
		    if (i < csl->entries()-1)
		      inList_ += ", ";
		  }
		inList_ += ")";

		patternCount++;

		if (fro->option_ == NOT_IN_LIST_)
		  notInList_ = TRUE;
		else
		  notInList_ = FALSE;
	      }
	    break;

	    case SOURCE_IMPLICIT_UNIQUE_INDEX_POSITION_:
	      {
		if (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
		  {
		    setErrorInParams(TRUE, "ImplicitUniqueIndexPosition option not supported.");
		    return;
		  }

		if (srcImpUnqIxPos_ != 0)
		  {
		    setErrorInParams(TRUE, "Duplicate ImplicitUniqueIndexPosition option specified.");
		    return;
		  }

		srcImpUnqIxPos_ = fro->numericVal_;
	      }
	    break;

	    case TARGET_SYSTEM_:
	      {
		if (NOT targetSystem_.isNull())
		  {
		    setErrorInParams(TRUE, "Duplicate Target System option specified.");
		    return;
		  }
		
		char * str = (char*)fro->stringVal_;

		if (str[0] == '\\')
                {
		  targetSystem_ = &str[1];
                  SQTargetType_ = FALSE;
                }
		else
                {
		  targetSystem_ = str;
                  SQTargetType_ = TRUE;
                }
		if (targetSystem_.isNull())
		{
		    setErrorInParams(TRUE, "Empty target system specified.");
		    return;
		}
                if (str_len(targetSystem_) >= 24)
                {
                   setErrorInParams(TRUE, "Target System name should be less than 24 characters");
                   return;
                }
	      }
	    break;

	    case COMPRESS_:
	      {
		if (compressSpecified_)
		  {
		    setErrorInParams(TRUE, "Duplicate Compress option specified.");
		    return;
		  }
		  
		compress_ = (fro->numericVal_ == 1 ? TRUE : FALSE);
		compressSpecified_ = TRUE;
	      }
	      break;
            case COMPRESSION_TYPE_:
            {
               if (compressionTypeSpecified_)
               {
                 setErrorInParams(TRUE, "Duplicate Compression Type  option specified.");
                 return;
               }
               compressionType_ = fro->numericVal_;
               compressionTypeSpecified_ = TRUE;
               break;
            }
            case DISK_POOL_:
            {
               if (diskPoolSpecified_)
               {
                 setErrorInParams(TRUE, "Duplicate Disk Pool option specified.");
                 return;
               }
               diskPool_ = fro->numericVal_;
               diskPoolSpecified_ = TRUE;
               break;
            }
	    case CONCURRENCY_:
	      {
		if (concurrency_ != -999999)
		  {
		    setErrorInParams(TRUE, "Duplicate Concurrency option specified.");
		    return;
		  }

		concurrency_ = fro->numericVal_;
	      }
	      break;

	    case RATE_:
	      {
		if ((CmpCommon::getDefault(COMP_BOOL_223) == DF_OFF) &&
		    (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
		  {
		    setErrorInParams(TRUE, "Rate option not supported.");
		    return;
		  }
		  
		if (rate_ != -999999)
		  {
		    setErrorInParams(TRUE, "Duplicate Rate option specified.");
		    return;
		  }
		
		rate_ = fro->numericVal_;
		
		if ((rate_ < 0) || (rate_ > 100))
		  {
		    setErrorInParams(TRUE, "Invalid Rate value specified.");
		    return;
		  }
	      }
	      break;

	    case PRIORITY_:
	      {
		if ((priority_ != -999999) ||
		    (priorityDelta_ != -999999))
		  {
		    setErrorInParams(TRUE, "Duplicate Priority option specified.");
		    return;
		  }

		priority_ = fro->numericVal_;

		if ((priority_ < 0) || (priority_ > 200))
		  {
		    setErrorInParams(TRUE, "Invalid priority value specified.");
		    return;
		  }
	      }
	      break;

	    case PRIORITY_DELTA_:
	      {
		if ((priorityDelta_ != -999999) ||
		    (priority_ != -999999))
		  {
		    setErrorInParams(TRUE, "Duplicate Priority Delta option specified.");
		    return;
		  }

		priorityDelta_ = fro->numericVal_;

		if ((priorityDelta_ < -200) || (priorityDelta_ > 200))
		  {
		    setErrorInParams(TRUE, "Invalid priority delta value specified.");
		    return;
		  }

		if (priorityDelta_ < 0)
		  priority_ = -1000 + priorityDelta_;
		else
		  priority_ = 1000 + priorityDelta_;
	      }
	      break;

	    case DELAY_:
	      {
		if (CmpCommon::getDefault(COMP_BOOL_223) == DF_OFF)
		  {
		    setErrorInParams(TRUE, "Delay option not supported.");
		    return;
		  }
		  
		if (delay_ != -999999)
		  {
		    setErrorInParams(TRUE, "Duplicate Delay option specified.");
		    return;
		  }
		
		delay_ = fro->numericVal_;
	      }
	      break;

	    case GET_STATUS_:
	      {
		if (getStatus_)
		  {
		    setErrorInParams(TRUE, "Duplicate Get Status option specified.");
		    return;
		  }
		  
		if (fro->stringVal_)
		  controlQueryId_ = (char*)fro->stringVal_;

		if (controlQueryId_.isNull())
		  {
		    setErrorInParams(TRUE, "Null query id was specified.");
		    return;
		  }

		numControlCommands++;
		getStatus_ = TRUE;
	      }
	    break;

	    case SUSPEND_:
	      {
		if (CmpCommon::getDefault(COMP_BOOL_223) == DF_OFF)
		  {
		    setErrorInParams(TRUE, "Suspend option not supported.");
		    return;
		  }
		  
		if (suspend_)
		  {
		    setErrorInParams(TRUE, "Duplicate Suspend option specified.");
		    return;
		  }

		if (fro->stringVal_)
		  controlQueryId_ = (char*)fro->stringVal_;

		numControlCommands++;
		suspend_ = TRUE;
	      }
	    break;

	    case RESUME_:
	      {
		if (CmpCommon::getDefault(COMP_BOOL_223) == DF_OFF)
		  {
		    setErrorInParams(TRUE, "Resume option not supported");
		    return;
		  }
		  
		if (resume_)
		  {
		    setErrorInParams(TRUE, "Duplicate Resume option specified.");
		    return;
		  }

		if (fro->stringVal_)
		  controlQueryId_ = (char*)fro->stringVal_;

		numControlCommands++;
		resume_ = TRUE;
	      }
	    break;

	    case ABORT_:
	      {
		if (abort_)
		  {
		    setErrorInParams(TRUE, "Duplicate Abort option specified.");
		    return;
		  }

		if (fro->stringVal_)
		  controlQueryId_ = (char*)fro->stringVal_;

		if (controlQueryId_.isNull())
		  {
		    setErrorInParams(TRUE, "Null query id was specified.");
		    return;
		  }

		numControlCommands++;

		abort_ = TRUE;
	      }
	    break;
            case VALIDATE_DDL_:
            {
               if (validateDDLSpecified_)
               {
                   setErrorInParams(TRUE, "Duplicate VALIDATE option specified.");
                   return;
               }
               validateDDL_ = TRUE; 
               validateDDLSpecified_ = TRUE;
            }
            break;
            case VALIDATE_AND_PURGEDATA_:
            {
              if (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
              {
                 setErrorInParams(TRUE, "VALIDATE AND PURGEDATA not supported.");
                 return;
               }
               validateAndPurgedata_ = TRUE;
            }
            break;
            case WAITED_STARTUP_:
               waitedStartup_ = TRUE;
               break;
	    case VALIDATE_TGT_DDL_:
	      {
		if (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
		  {
		    setErrorInParams(TRUE, "ValidateTgtDDL not supported.");
		    return;
		  }

		if (validateOrCreateSpecified)
		  {
		    setErrorInParams(TRUE, "Duplicate ValidateTgtDDL option specified.");
		    return;
		  }

		validateTgtDDL_ = TRUE;

		if (fro->stringVal_)
		  {
		    ddlInputStr_ = (char*)fro->stringVal_;
		  }

		if (fro->numericVal_ == 1)
		  onlySpecified_ = TRUE;

		validateOrCreateSpecified = TRUE;
	      }
	    break;

	    case NO_VALIDATE_TGT_DDL_:
	      {
		if (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
		  {
		    setErrorInParams(TRUE, "NoValidateTgtDDL not supported.");
		    return;
		  }

		if (validateOrCreateSpecified)
		  {
		    setErrorInParams(TRUE, "Duplicate NoValidateTgtDDL option specified.");
		    return;
		  }

		validateTgtDDL_ = FALSE;

		validateOrCreateSpecified = TRUE;
	      }
	    break;

	    case COPY_DDL_:
	      {
		if (CmpCommon::getDefault(COMP_BOOL_222) == DF_OFF)
		  {
		    setErrorInParams(TRUE, "Copy DDL option not supported.");
		    return;
		  }
		  
		if (copyDDL_)
		  {
		    setErrorInParams(TRUE, "Duplicate Copy DDL option specified.");
		    return;
		  }

		if (incremental_)
		  {
		    setErrorInParams(TRUE, "Incremental option cannot be specified with Copy DDL");
		    return;
		  }
		  
		copyDDL_ = TRUE;

		numSchCopyCommands++;
	      }
	    break;

	    case COPY_DATA_:
	      {
		if (CmpCommon::getDefault(COMP_BOOL_223) == DF_OFF)
		  {
		    setErrorInParams(TRUE, "Copy Data option not supported.");
		    return;
		  }
		  
		if (copyData_)
		  {
		    setErrorInParams(TRUE, "Duplicate Copy Data option specified.");
		    return;
		  }

		copyData_ = TRUE;

		tgtDDLType_ = 0;
		numSchCopyCommands++;
	      }
	    break;

            case COPY_BOTH_DATA_AND_STATISTICS_:
              {
                if (CmpCommon::getDefault(COMP_BOOL_222) == DF_OFF)
                  {
                    setErrorInParams(TRUE, "Copy Both Data And Statistics option not supported.");
                    return;
                  }

                if (copyBothDataAndStats_)
                  {
                    setErrorInParams(TRUE, "Duplicate Copy Both Data And Statistics option specified.");
                    return;
                  }

                if (copyData_)
                  {
                    setErrorInParams(TRUE, "Copy Data option cannot be specified with Copy Both Data And Statistics");
                    return;
                  }

                if (statistics_)
                  {
                    setErrorInParams(TRUE, "Copy Statistics option cannot be specified with Copy Both Data And Statistics");
                    return;
                  }

                if (incremental_)
                  {
                    setErrorInParams(TRUE, "Incremental option cannot be specified with Copy Both Data And Statistics");
                    return;
                  }

                copyBothDataAndStats_ = TRUE;
                numSchCopyCommands++;
              }
            break;

	    case COPY_STATISTICS_:
	      {
		if (CmpCommon::getDefault(COMP_BOOL_222) == DF_OFF)
		  {
		    setErrorInParams(TRUE, "Copy Statistics option not supported.");
		    return;
		  }

		if (copyBothDataAndStats_)
		  {
		    setErrorInParams(TRUE, "Copy Both Data And Statistics option cannot be specified with Copy Statistics");
		    return;
		  }

		if (statistics_)
		  {
		    setErrorInParams(TRUE, "Duplicate Copy Statistics option specified.");
		    return;
		  }

		if (incremental_)
		  {
		    setErrorInParams(TRUE, "Incremental option cannot be specified with Copy Statistics");
		    return;
		  }
		  
		statistics_ = TRUE;

		tgtStatsType_ = 0;
		numSchCopyCommands++;
	      }
	    break;

	    case TGT_STATISTICS_:
	      {
		if (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
		  {
		    setErrorInParams(TRUE, "TgtStatistics not supported.");
		    return;
		  }

		if (statistics_)
		  {
		    setErrorInParams(TRUE, "Duplicate Tgt Statistics option specified.");
		    return;
		  }

		statistics_ = TRUE;

		tgtStatsType_ = fro->numericVal_;

		if (fro->stringVal_)
		  {
		    ddlInputStr_ = (char*)fro->stringVal_;
		  }

		numSchCopyCommands++;
	      }
	    break;

	    case TGT_DDL_:
	      {
		if (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
		  {
		    setErrorInParams(TRUE, "TgtDDL option not supported.");
		    return;
		  }

		if (copyDDL_)
		  {
		    setErrorInParams(TRUE, "Duplicate Copy DDL option specified.");
		    return;
		  }

		copyDDL_ = TRUE;

		tgtDDLType_ = fro->numericVal_;

		if (fro->stringVal_)
		  {
		    ddlInputStr_ = (char*)fro->stringVal_;
		  }

		numSchCopyCommands++;
	      }
	    break;

	    case VALIDATE_DATA_:
	      {
		if (CmpCommon::getDefault(COMP_BOOL_223) == DF_OFF)
		  {
		    setErrorInParams(TRUE, "ValidateData option not supported.");
		    return;
		  }
		  
		if (validateData_)
		  {
		    setErrorInParams(TRUE, "Duplicate ValidateData option specified.");
		    return;
		  }

		validateData_ = TRUE;

		if (fro->numericVal_ == 1)
		  onlySpecified_ = TRUE;
	      }
	    break;

	    case TGT_OBJS_ONLINE_:
	      {
		if (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
		  {
		    setErrorInParams(TRUE, "TgtObjsOnline option not supported.");
		    return;
		  }

		if ((tgtObjsOnline_) || (tgtObjsOffline_))
		  {
		    setErrorInParams(TRUE, "Duplicate TgtObjsOnline option specified.");
		    return;
		  }

		tgtObjsOnline_ = TRUE;
	      }
	    break;

	    case TGT_OBJS_OFFLINE_:
	      {
		if (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
		  {
		    setErrorInParams(TRUE, "TgtObjsOffline option not supported.");
		    return;
		  }

		if ((tgtObjsOnline_) || (tgtObjsOffline_))
		  {
		    setErrorInParams(TRUE, "Duplicate TgtObjsOnline option specified.");
		    return;
		  }

		tgtObjsOffline_ = TRUE;
	      }
	    break;

	    case TGT_OBJS_UNAUDITED_:
	      {
		if (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
		  {
		    setErrorInParams(TRUE, "TgtObjsUnaudited option not supported.");
		    return;
		  }

		if ((tgtObjsUnaudited_) || (tgtObjsAudited_))
		  {
		    setErrorInParams(TRUE, "Duplicate TgtObjsUnaudited option specified.");
		    return;
		  }

		tgtObjsUnaudited_ = TRUE;
	      }
	    break;

	    case TGT_OBJS_AUDITED_:
	      {
		if (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
		  {
		    setErrorInParams(TRUE, "TgtObjsAudited option not supported.");
		    return;
		  }

		if ((tgtObjsUnaudited_) || (tgtObjsAudited_))
		  {
		    setErrorInParams(TRUE, "Duplicate TgtObjsAudited option specified.");
		    return;
		  }

		tgtObjsAudited_ = TRUE;
	      }
	    break;

	    case TGT_CAT_NAME_:
	      {
		if ((CmpCommon::getDefault(COMP_BOOL_223) == DF_OFF) &&
		    (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
		  {
		    setErrorInParams(TRUE, "TgtCatName option not supported.");
		    return;
		  }
		  
		if (NOT tgtCatName_.isNull())
		  {
		    setErrorInParams(TRUE, "Duplicate TgtCatName option specified.");
		    return;
		  }

		tgtCatName_ = (char*)fro->stringVal_;
		if (tgtCatName_.isNull())
		  {
		    setErrorInParams(TRUE, "Invalid TgtCatName specified.");
		    return;
		  }
	      }
	    break;

	    case TGT_RETURN_PARTITION_DETAILS_:
	      {
		if (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
		  {
		    setErrorInParams(TRUE, "TgtReturnPartitionDetails not supported.");
		    return;
		  }

		if (returnPartnDetails_)
		  {
		    setErrorInParams(TRUE, "Duplicate TgtReturnPartitionDetails option specified.");
		    return;
		  }

		returnPartnDetails_ = TRUE;
	      }
	    break;

	    case TRANSFORM_:
	      {
		if ((NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)) &&
		    (CmpCommon::getDefault(COMP_BOOL_222) == DF_OFF))
		  {
		    setErrorInParams(TRUE, "Transform option not supported.");
		    return;
		  }
		  
		if (transform_ || noTransform_)
		  {
		    setErrorInParams(TRUE, "Duplicate Transform option specified.");
		    return;
		  }

		if (incremental_)
		  {
		    setErrorInParams(TRUE, "Either Transform or Incremental can be specified"
		                           ", not both.");
		    return;
		  }
		  
		transform_ = TRUE;
	      }
	    break;

	    case NO_TRANSFORM_:
	      {
		if ((NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)) &&
		    (CmpCommon::getDefault(COMP_BOOL_222) == DF_OFF))
		  {
		    setErrorInParams(TRUE, "No Transform option not supported.");
		    return;
		  }
		  
		if (transform_ || noTransform_)
		  {
		    setErrorInParams(TRUE, "Duplicate Transform option specified.");
		    return;
		  }

		noTransform_ = TRUE;
	      }
	    break;

	    case ROLE_NAME_:
	      {
		if (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
		  {
		    setErrorInParams(TRUE, "User Id option not supported.");
		    return;
		  }

		if (NOT roleName_.isNull())
		  {
		    setErrorInParams(TRUE, "Duplicate Role Name specified.");
		    return;
		  }

		roleName_ = (char*)fro->stringVal_;
	      }
	    break;

	    case NO_VALIDATE_ROLE_:
	      {
		if (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
		  {
		    setErrorInParams(TRUE, "No Validate Role option not supported.");
		    return;
		  }

		if (noValidateRole_)
		  {
		    setErrorInParams(TRUE, "Duplicate No Validate Role option specified.");
		    return;
		  }

		noValidateRole_ = TRUE;
	      }
	    break;

	    case NUM_SRC_PARTNS_:
	      {
		if (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
		  {
		    setErrorInParams(TRUE, "NumSrcPartns option not supported.");
		    return;
		  }

		if (numSrcPartns_ != 0)
		  {
		    setErrorInParams(TRUE, "Duplicate NumSrcPartns specified.");
		    return;
		  }

		numSrcPartns_ = fro->numericVal_;
	      }
	    break;

	    case DEBUG_TARGET_:
	      {
		if (CmpCommon::getDefault(COMP_BOOL_223) == DF_OFF)
		  {
		    setErrorInParams(TRUE, "Debug Target option not supported.");
		    return;
		  }
		  
		if (debugTarget_)
		  {
		    setErrorInParams(TRUE, "Duplicate Debug Target option specified.");
		    return;
		  }

		debugTarget_ = TRUE;
	      }
	    break;

	    case FORCE_ERROR_:
	      {
		if ((NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)) &&
		    (CmpCommon::getDefault(COMP_BOOL_223) == DF_OFF))
		  {
		    setErrorInParams(TRUE, "Force Error option not supported.");
		    return;
		  }
		
		if (NOT forceError_.isNull())
		  {
		    setErrorInParams(TRUE, "Duplicate Force Error option specified.");
		    return;
		  }
		
		forceError_ = (char*)fro->stringVal_;
	      }
	    break;

	    case DISABLE_SCHEMA_CREATE_:
	      {
		if (disableSchemaCreate_)
		  {
		    setErrorInParams(TRUE, "Duplicate Disable Schema Create option specified.");
		    return;
		  }
		
		disableSchemaCreate_ = TRUE;
	      }
	    break;

	    case RECOVER_:
	      {
		/*if ((CmpCommon::getDefault(COMP_BOOL_223) == DF_OFF) &&
		    (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
		  {
		    setErrorInParams(TRUE, "Recover option not supported.");
		    return;
		  }
		  */

		if (recoverType_ != 0)
		  {
		    setErrorInParams(TRUE, "Duplicate Recover option specified.");
		    return;
		  }

		recoverType_ = fro->numericVal_;

		if (fro->stringVal_)
		  controlQueryId_ = (char*)fro->stringVal_;

		numControlCommands++;
	      }
	    break;


	    case GET_DETAILS_:
	      {
		if (getDetails_)
		  {
		    setErrorInParams(TRUE, "Duplicate Get Details option specified.");
		    return;
		  }

		// s: short format
		// m: medium format
		// l: long format
		if (fro->stringVal_)
		  {
		    formatOptions_ = (char*)fro->stringVal_;

		    formatOptions_ = formatOptions_.strip(NAString::both);
		    formatOptions_.toLower();
		    if ((formatOptions_.contains(" ")) ||
			((formatOptions_ != "s") &&
			 (formatOptions_ != "m") &&
			 (formatOptions_ != "l")))
		      {
			setErrorInParams(TRUE, "Invalid Get Details value specified.");
			return;
		      }
		  }

		getDetails_ = TRUE;
	      }
	    break;

	    case TGT_ACTIONS_:
	      {
		if (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
		  {
		    setErrorInParams(TRUE, "TgtActions option not supported.");
		    return;
		  }

		if (tgtActions_)
		  {
		    setErrorInParams(TRUE, "Duplicate TgtActions option specified.");
		    return;
		  }

		tgtActions_ = TRUE;
	      }
	    break;

	    case INITIALIZE_:
	      {
		initialize_ = TRUE;

		numControlCommands++;
	      }
	    break;

	    case REINITIALIZE_:
	      {
		reinitialize_ = TRUE;

		numControlCommands++;
	      }
	    break;

	    case DROP_:
	      {
		drop_ = TRUE;

		numControlCommands++;
	      }
	    break;

	    case ADD_CONFIG_:
	      {
		if (addConfig_)
		  {
		    setErrorInParams(TRUE, "Duplicate Add Config option specified.");
		    return;
		  }

		char * str = (char*)fro->stringVal_;

		if (str[0] == '\\')
		  targetSystem_ = &str[1];
		else
		  targetSystem_ = str;

                if (str_len(targetSystem_) >= 24)
                {
                   setErrorInParams(TRUE, "System name should be less than 24 characters");
                   return;
                }
		addConfig_ = TRUE;

		if (fro->stringVal2_)
		  {
		    ipAddr_ = (char*)fro->stringVal2_;
		  }

		if (fro->numericVal2_ > 0)
		  portNum_ = fro->numericVal2_;

		numControlCommands++;
	      }
	    break;

	    case REMOVE_CONFIG_:
	      {
		if (removeConfig_)
		{
		    setErrorInParams(TRUE, "Duplicate Remove Config option specified.");
		    return;
		}

		char * str = (char*)fro->stringVal_;

		if (str[0] == '\\')
		  targetSystem_ = &str[1];
		else
		  targetSystem_ = str;

                if (str_len(targetSystem_) >= 24)
                {
                   setErrorInParams(TRUE, "System name should be less than 24 characters");
                   return;
                }
		removeConfig_ = TRUE;
		numControlCommands++;
	      }
	    break;

	    case CLEANUP_:
	      {
		if (cleanup_)
		  {
		    setErrorInParams(TRUE, "Duplicate Cleanup option specified.");
		    return;
		  }

		cleanup_ = TRUE;

		if (fro->numericVal_ > 0)
		  {
		    str_cpy_all((char*)&cleanupFrom_,
				(char*)fro->stringVal_,
				fro->numericVal_);
		  }

		if (fro->numericVal2_ > 0)
		  {
		    str_cpy_all((char*)&cleanupTo_,
				(char*)fro->stringVal2_,
				fro->numericVal2_);
		  }

		if ((cleanupTo_ > 0) &&
		    (cleanupFrom_ > 0) &&
		    (cleanupTo_ < cleanupFrom_))
		  {
		    setErrorInParams(TRUE, "Invalid Cleanup value specified.");
		    return;
		  }

		numControlCommands++;
	      }
	      break;
            case VERSION_:
              {
	        if (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
		{
                   setErrorInParams(TRUE, "Invalid version option specified.");
                   return;
                }
                version_ = fro->numericVal_;
              }
              break;
	    default:
	      {
		setErrorInParams(TRUE, "Invalid option specified.");
		return;
	      }
	      break;
	    } // switch
	} // for

    } // if

  if (concurrency_ == -999999)
    concurrency_ = -2;

  if (initialize_ || reinitialize_ || drop_ || cleanup_ )
  {
  }
  else if (authids_)
  {
     // must specify target system
     if (targetSystem_.isNull()) 
     {
        setErrorInParams(TRUE, "Target System name must be specified.");
        return;
     }
     if (NOT sourceSchema_.getSchemaName().isNull() || NOT sourceName_.isEmpty() || srcInList_.entries() > 0)
     {
       setErrorInParams(TRUE, "Source option is not allowed in REPLICATE AUTHORIZATION command.");
       return;
     }
     if (incremental_)
     {
       setErrorInParams(TRUE, "Incremental option is not allowed in REPLICATE AUTHORIZATION command."); // may be allowed in the future
       return;
     }
     // Note that copyDDL_ is set to TRUE and tgtDDLType_ is set to 1 for the following
     // internally-generated statement:
     //   replicate authorization target system "LOCAL" , version 13 , no validate target ddl ,
     //   target ddl create using query id 
     //   target action ;
     // We do not want to issue the "Copy DDL option is not allowed ..." error message for
     // the above case.
     if (copyDDL_ && tgtDDLType_ == 0)
     {
       setErrorInParams(TRUE, "Copy DDL option is not allowed in REPLICATE AUTHORIZATION command.");
       return;
     }
     if (statistics_)
     {
       setErrorInParams(TRUE, "Copy Statistics option is not allowed in REPLICATE AUTHORIZATION command.");
       return;
     }
     if (copyBothDataAndStats_)
     {
       setErrorInParams(TRUE, "Copy Both Data And Statistics option is not allowed in REPLICATE AUTHORIZATION command.");
       return;
     }
     if (purgedataTgt_)
     {
       setErrorInParams(TRUE, "Purgedata Target option is not allowed in REPLICATE AUTHORIZATION command.");
       return;
     }
     if (transform_)
     {
       setErrorInParams(TRUE, "Transform option is not allowed in REPLICATE AUTHORIZATION command.");
       return;
     }
     if (noTransform_)
     {
       setErrorInParams(TRUE, "No Transform option is not allowed in REPLICATE AUTHORIZATION command.");
       return;
     }
  }
  else if (compressionTypeSpecified_ && ! copyDDL_)
  {
       setErrorInParams(TRUE, "COMPRESSION TYPE is allowed with Copy DDL option only.");
       return;
  }
  else if (diskPoolSpecified_ && ! copyDDL_)
  {
       setErrorInParams(TRUE, "DISK POOL is allowed with Copy DDL option only.");
       return;
  }
  else if (validateDDLSpecified_ && (copyDDL_ || statistics_))
  {
       setErrorInParams(TRUE, "VALIDATE is not allowed with COPY DDL or COPY STATISTICS option.");
       return;
         
  }
  else if (validateDDLSpecified_ && recoverType_ > 0)
  {
       setErrorInParams(TRUE, "RECOVER is not needed with VALIDATE.");
       return;
  }
  else if (ddlInputStr_.isNull())
    {
      // only table replication is supported at this time.
      targetType_ = sourceType_;
      if ((sourceType_ != TABLE_) &&
	  (sourceType_ != INDEX_) &&
	  (sourceType_ != MV_))
	{
	  setErrorInParams(TRUE, "Invalid source object type specified.");
	  return;
	}
      
      if ((sourceType_ == MV_) &&
	  ((NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)) &&
	   ((CmpCommon::getDefault(COMP_BOOL_223) == DF_OFF))))
	{
	  setErrorInParams(TRUE, "MV object not supported.");
	  return;
	}
      
      if ((sourceType_ == INDEX_) &&
	  (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
	{
	  setErrorInParams(TRUE, "Index object not supported.");
	  return;
	}

      // can only specify either a single source object or an IN list of objs
      if ((NOT sourceName_.isEmpty()) &&
	  (srcInList_.entries() > 0))
	{
	  setErrorInParams(TRUE, "Cannot specify source object name and IN list at the same time.");
	  return;
	}
	
      if ((NOT sourceSchema_.getSchemaName().isNull()) &&
	  (srcInList_.entries() > 0))
	{
	  setErrorInParams(TRUE, "Cannot specify source schema and IN list at the same time.");
	  return;
	}
	
      // must specify fully qualified source and target names, if schema is
      // not being replicated.
      if ((sourceSchema_.getSchemaName().isNull()) &&
	  (srcInList_.entries() == 0))
	{
	  if ((sourceName_.isEmpty()) &&
	      (numControlCommands == 0 ||
               (recoverType_ > 0 && controlQueryId_.isNull())))
	    {
	      setErrorInParams(TRUE, "Source object name must be specified.");
	      return;
	    }
	  
          if (targetName_.isEmpty())
	  {
	     targetName_ = sourceName_;
             if (NOT targetSchema_.getSchemaName().isNull())
                targetName_.getQualifiedNameObj().setSchemaName
                    (targetSchema_.getSchemaName());
	  }
	}
       
      if (incremental_ && NOT targetSchema_.getSchemaName().isNull())
      {
	  setErrorInParams(TRUE, "Incremental option cannot be specified when target schema is specified.");
	  return;
      }
      
      // cannot do schema replication if source name is specified
      if ((NOT sourceName_.isEmpty()) &&
	  (NOT sourceSchema_.getSchemaName().isNull()))
	{
	  setErrorInParams(TRUE, "Only one of source schema or source object name can be specified.");
	  return;
	}

      if ((srcInList_.entries() > 0) &&
	  ((sourceName_.isEmpty()) &&
	   (sourceSchema_.getSchemaName().isNull())))
	{
	  srcObjsInList_ = TRUE;
	}

      if ((srcObjsInList_) &&
	  ((NOT sourceName_.isEmpty()) ||
	   (NOT targetName_.isEmpty()) ||
	   (NOT sourceSchema_.getSchemaName().isNull())))
	{
	  setErrorInParams(TRUE, "Cannot specify source object or schema name if IN list of objects is specified.");
	  return;
	}
	
      if ((sourceSchema_.getSchemaName().isNull()) &&
	  (disableSchemaCreate_))
	{
	  setErrorInParams(TRUE, "Schema name must be specified.");
	  return;
	}

      // must specify target system
      if ((targetSystem_.isNull()) &&
	  (numControlCommands == 0) && 
	   (recoverType_ == 0) && (targetSchema_.getSchemaName().isNull()))
	{
	  setErrorInParams(TRUE, "Target System name must be specified.");
	  return;
	}
       
      char bdrClusterName[BDR_CLUSTER_NAME_LEN+1];
      Int16 error;
      char * sqlmxRegr = NULL;
   
      // Control commands don't need to have target system name set
      // except recover command

      if (numControlCommands == 0 || recoverType_ > 0)
      {
         if ((error = getBDRClusterName(bdrClusterName)) != XZFIL_ERR_OK)
         {
            sqlmxRegr = getenv("SQLMX_REGRESS");
            if (sqlmxRegr == NULL)
            {
               char errorMsg[100];
               sprintf(errorMsg, "BDR service is not started or unable to get the BDR cluster name. Error %d", error);
               setErrorInParams(TRUE, errorMsg);
               return;
            }
            else
               strcpy(bdrClusterName, "UNKNOWN");
         }

         if (NOT targetSchema_.getSchemaName().isNull())
         {
            if (targetSystem_.isNull())
               targetSystem_ = bdrClusterName;
         }
         else
         {
            if (targetSystem_.isNull())
            {
               setErrorInParams(TRUE, "Target System name must be specified.");
               return;
            }
            else
            if (strcmp(targetSystem_, bdrClusterName) == 0)
            {
                setErrorInParams(TRUE, "Source and Target System cannot be the same.");
                return;
            }
         }
      }

      if ((srcObjsInList_) &&
	  (recoverType_ != 0))
	{
	  setErrorInParams(TRUE, "Recover only supported for single object.");
	  return;
	}
    }

  // only one of like/not_like/in/not_in could be specified
  if (patternCount > 1)
    {
      setErrorInParams(TRUE, "Only one of LIKE or NOT LIKE options can be specified.");
      return;
    }
  
  // pattern can only be specified with source schema replication
  if ((patternCount > 0) &&
      (sourceSchema_.getSchemaName().isNull()))
    {
      setErrorInParams(TRUE, "Pattern can only be specified with source schema specification.");
      return;
    }
  
  if (numControlCommands > 1)
    {
      setErrorInParams(TRUE, "Only one of the specified options can be provided.");
      return;
    }
  
  if (NOT authids_ && numSchCopyCommands > 1)
    {
      if (NOT sourceSchema_.getSchemaName().isNull()) // REPLICATE SOURCE SCHEMA
        {
          setErrorInParams(TRUE, "Only one of the Schema DDL or Statistics copy options can be provided.");
          return;
        }
      else if (sourceType_ == TABLE_ && NOT sourceName_.isEmpty()) // REPLICATE SOURCE TABLE
        {
          setErrorInParams(TRUE, "Only one of the copy options can be provided.");
          return;
        }
    }
  
  // schema copy commands can only be specified with a schema specification
  // without any pattern.
  // Also, cannot specify list of source objects or a single source object.
  // Copy Statistics options is now allowed in REPLICATE SOURCE TABLE command
  if (numSchCopyCommands == 1)
    {
      if (((CmpCommon::getDefault(COMP_BOOL_224) == DF_ON) ||
	   (Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)) ||
	   (sourceType_ == TABLE_)) &&
	  (statistics_) &&
	  //(sourceSchema_.getSchemaName().isNull()) &&
	  (NOT sourceName_.isEmpty()))
	{
	  // single table statistics replication is supported.
	}
      else if (NOT authids_ && sourceSchema_.getSchemaName().isNull() &&
	       NOT sourceName_.isEmpty()) // REPLICATE SOURCE <objectType> <objectName>
	{
	  if (copyDDL_)
	    {
	      setErrorInParams(TRUE, "Copy DDL option only supported with Schema specification.");
	      return;
	    }
	  if (statistics_ && sourceType_ != TABLE_)
	    {
	      setErrorInParams(TRUE, "Copy Statistics option only supported with Schema or Table specification.");
	      return;
	    }
	  if (copyBothDataAndStats_ && sourceType_ != TABLE_)
	    {
	      // setErrorInParams(TRUE, "Copy Both Data And Statistics option only supported with Schema or Table specification."); // RFE
	      setErrorInParams(TRUE, "Copy Both Data And Statistics option only supported with Table specification.");
	      return;
	    }
	}

      if (patternCount > 0)
	{
	  setErrorInParams(TRUE, "DDL or Statistics Copy option not supported with pattern specification.");
	  return;
	}
      
      if (srcObjsInList_ && copyDDL_)
	{
	  setErrorInParams(TRUE, "DDL option not supported with IN list.");
	  return;
	}
 
      onlySpecified_ = TRUE;
    }

  if ((CmpCommon::getDefault(COMP_BOOL_222) == DF_OFF) &&
      (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      transform_ = FALSE;
      noTransform_ = TRUE;
    }

  if (priority_ == -999999)
    priority_ = -1;

  if (priorityDelta_ == -999999)
    priorityDelta_ = 0;

  // suspend/resume not supported yet
  if (suspend_)
    {
      setErrorInParams(TRUE, "Suspend option not supported.");
      return;
    }

  if (resume_)
    {
      setErrorInParams(TRUE, "Resume option not supported.");
      return;
    }

  //  if (rate_ == -999999)
  //    rate_ = 100;

  if (delay_ == -999999)
    delay_ = 10; // 10 seconds max delay

}

RelExpr * ExeUtilReplicate::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilReplicate *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilReplicate(NULL, outHeap);
  else
    result = (ExeUtilReplicate *) derivedNode;

  result->sourceName_       = sourceName_;
  result->sourceType_       = sourceType_;
  result->targetName_       = targetName_;
  result->targetType_       = targetType_;
  result->sourceSchema_     = sourceSchema_;
  result->targetSystem_     = targetSystem_;
  result->targetSchema_     = targetSchema_;
  result->purgedataTgt_     = purgedataTgt_;
  result->incremental_      = incremental_;

  result->likePattern_      = likePattern_;
  result->escChar_          = escChar_;
  result->notLikePattern_   = notLikePattern_;
  result->inList_           = inList_;
  result->notInList_        = notInList_;

  result->srcObjsInList_    = srcObjsInList_;
  result->srcInList_        = srcInList_;

  result->srcImpUnqIxPos_ = srcImpUnqIxPos_;

  result->concurrency_   = concurrency_;
  result->rate_          = rate_;
  result->priority_      = priority_;
  result->priorityDelta_ = priorityDelta_;
  result->delay_         = delay_;
  result->compress_      = compress_;
  result->compressSpecified_  = compressSpecified_;
  result->compressionType_  = compressionType_;
  result->compressionTypeSpecified_ = compressionTypeSpecified_;
  
  result->diskPool_  = diskPool_;
  result->diskPoolSpecified_ = diskPoolSpecified_;
  result->getStatus_      = getStatus_;
  result->suspend_        = suspend_;
  result->resume_         = resume_;
  result->abort_          = abort_;
  result->controlQueryId_ = controlQueryId_;

  result->validateTgtDDL_         = validateTgtDDL_;
  result->validateDDL_         = validateDDL_;
  result->validateDDLSpecified_   = validateDDLSpecified_;
  result->validateAndPurgedata_ = validateAndPurgedata_;
  result->validateData_           = validateData_;
  result->onlySpecified_          = onlySpecified_;

  result->statistics_   = statistics_;
  result->tgtStatsType_ = tgtStatsType_;
  result->tgtDDLType_   = tgtDDLType_;

  result->copyDDL_      = copyDDL_;
  result->copyData_     = copyData_;
  result->copyBothDataAndStats_ = copyBothDataAndStats_;

  result->ddlInputStr_ = ddlInputStr_;

  result->tgtObjsOnline_  = tgtObjsOnline_;
  result->tgtObjsOffline_ = tgtObjsOffline_;

  result->tgtObjsAudited_  = tgtObjsAudited_;
  result->tgtObjsUnaudited_ = tgtObjsUnaudited_;

  result->returnPartnDetails_ = returnPartnDetails_;

  result->transform_ = transform_;
  result->noTransform_ = noTransform_;
  result->tgtActions_ = tgtActions_;

  result->debugTarget_ = debugTarget_;

  result->forceError_ = forceError_;

  result->disableSchemaCreate_ = disableSchemaCreate_;

  result->numSrcPartns_ = numSrcPartns_;

  result->recoverType_ = recoverType_;

  result->noValidateRole_ = noValidateRole_;
  result->roleName_ = roleName_;

  result->getDetails_ = getDetails_;
  result->formatOptions_ = formatOptions_;

  result->initialize_ = initialize_;
  result->reinitialize_ = reinitialize_;
  result->drop_ = drop_;

  result->addConfig_ = addConfig_;
  result->removeConfig_ = removeConfig_;
  result->ipAddr_ = ipAddr_;
  result->portNum_ = portNum_;

  result->validatePrivs_ = validatePrivs_;

  result->cleanup_ = cleanup_;
  result->cleanupFrom_ = cleanupFrom_;
  result->cleanupTo_ = cleanupTo_;
  result->authids_  = authids_;
  return ExeUtilExpr::copyTopNode(result, outHeap);
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

	    case REORG_INDEX_:
	      {
		if (reorgIndex_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		explicitTask = TRUE;

		reorgIndex_ = TRUE;

		if (mto->stringVal1_)
		  {
		    reorgIndexOptions_ = " ";
		    reorgIndexOptions_ += mto->stringVal1_;
		  }

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

            case REORG_MVGROUP_:
	      {
		if (reorgMvgroup_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		explicitTask = TRUE;

		reorgMvgroup_ = TRUE;

		if (mto->stringVal1_)
		  {
		    reorgMvgroupOptions_ = " ";
		    reorgMvgroupOptions_ += mto->stringVal1_;
		  }

		maintainOptionsSpecified = TRUE;

	      }
	    break;

	    case REORG_MVS_:
	      {
		if (reorgMvs_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		explicitTask = TRUE;

		reorgMvs_ = TRUE;

		if (mto->stringVal1_)
		  {
		    reorgMvsOptions_ = " ";
		    reorgMvsOptions_ += mto->stringVal1_;
		  }

		maintainOptionsSpecified = TRUE;

	      }
	    break;

	    case REORG_MVS_INDEX_:
	      {
		if (reorgMvsIndex_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		explicitTask = TRUE;

		reorgMvsIndex_ = TRUE;

		if (mto->stringVal1_)
		  {
		    reorgMvsIndexOptions_ = " ";
		    reorgMvsIndexOptions_ += mto->stringVal1_;
		  }

		maintainOptionsSpecified = TRUE;

	      }
	    break;

	    case REORG_:
	      {
		if (reorg_)
		  {
		    errorInParams_ = TRUE;
		    return;
		  }

		reorg_ = TRUE;

		explicitTask = TRUE;

		if (mto->stringVal1_)
		  {
		    reorgTableOptions_ = " ";
		    reorgTableOptions_ += mto->stringVal1_;
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
       hbaseObjs_(FALSE)
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

// See ControlRunningQuery

// -----------------------------------------------------------------------
// Member functions for class ExeUtilGetFormattedDiskStats
// -----------------------------------------------------------------------
ExeUtilGetDiskLabelStats::ExeUtilGetDiskLabelStats
(RelExpr * child,
 char * optionsStr,
 NABoolean isIndex,
 NABoolean isIudLog,
 NABoolean isRangeLog,
 NABoolean isTempTable,
 CollHeap *oHeap)
     : ExeUtilExpr(GET_DISK_LABEL_STATS_, CorrName("DUMMY"), 
		   NULL, child, NULL, CharInfo::UnknownCharSet, oHeap),
       isIndex_(isIndex),
       isIudLog_(isIudLog),
       isRangeLog_(isRangeLog),
       isTempTable_(isTempTable),
       displayFormat_(FALSE),
       errorInParams_(FALSE)
{
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

	  if (option == "DF")
	    {
	      if (displayFormat_)
		{
		  errorInParams_ = TRUE;
		  return;
		}

	      displayFormat_ = TRUE;
	    }
	  else
	    {
	      errorInParams_ = TRUE;
	      return;
	    }

	  currIndex += 3;
	}
    }
}

RelExpr * ExeUtilGetDiskLabelStats::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilGetDiskLabelStats *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilGetDiskLabelStats(NULL, NULL, isIndex_, isIudLog_,isRangeLog_,isTempTable_,outHeap);
  else
    result = (ExeUtilGetDiskLabelStats *) derivedNode;

  result->optionsStr_ = optionsStr_;
  result->displayFormat_ = displayFormat_;

  result->errorInParams_ = errorInParams_;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilGetDiskStats
// -----------------------------------------------------------------------
RelExpr * ExeUtilGetDiskStats::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilGetDiskStats *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilGetDiskStats(NULL,
					       resetStats_,
					       outHeap);
  else
    result = (ExeUtilGetDiskStats *) derivedNode;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// Member functions for class ExeUtilGetFormattedDiskStats
// -----------------------------------------------------------------------
ExeUtilGetFormattedDiskStats::ExeUtilGetFormattedDiskStats
(const CorrName &cn,
 char * optionsStr,
 CollHeap *oHeap)
     : ExeUtilExpr(GET_FORMATTED_DISK_STATS_, CorrName("DUMMY"), 
		   NULL, NULL, NULL, CharInfo::UnknownCharSet, oHeap),
       cn_(cn),
       reset_(FALSE),
       shortFormat_(FALSE),
       longFormat_(FALSE),
       errorInParams_(FALSE)
{
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

	  if (option == "RS")
	    {
	      if (reset_)
		{
		  errorInParams_ = TRUE;
		  return;
		}

	      reset_ = TRUE;
	    }
	  else if (option == "SF")
	    {
	      if (shortFormat_ || longFormat_)
		{
		  errorInParams_ = TRUE;
		  return;
		}

	      shortFormat_ = TRUE;
	    }
	  else if (option == "LF")
	    {
	      if (shortFormat_ || longFormat_)
		{
		  errorInParams_ = TRUE;
		  return;
		}

	      longFormat_ = TRUE;
	    }
	  else
	    {
	      errorInParams_ = TRUE;
	      return;
	    }

	  currIndex += 3;
	}
    }
}

RelExpr * ExeUtilGetFormattedDiskStats::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilGetFormattedDiskStats *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilGetFormattedDiskStats(cn_, NULL, outHeap);
  else
    result = (ExeUtilGetFormattedDiskStats *) derivedNode;

  result->getDiskStatsStr_ = getDiskStatsStr_;

  result->optionsStr_ = optionsStr_;
  result->reset_ = reset_;
  result->shortFormat_ = shortFormat_;
  result->longFormat_ = longFormat_;

  result->errorInParams_ = errorInParams_;

  return ExeUtilExpr::copyTopNode(result, outHeap);
}


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

  if (getVirtualTableName())
    {
      CorrName corrName(getVirtualTableName());
      corrName.setSpecialType(ExtendedQualName::VIRTUAL_TABLE);
      NATable *naTable = bindWA->getSchemaDB()->getNATableDB()->
	get(&corrName.getExtendedQualNameObj());
      
      if (NOT naTable) {
	desc_struct *tableDesc = createVirtualTableDesc();
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
	  // no rows are returned from this operator. 
	  // Allocate an empty RETDesc and attach it to this and the BindScope.
	  //
	  setRETDesc(new (bindWA->wHeap()) RETDesc(bindWA));
	}
      bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    }
  
  if (tableName_.getSpecialType()==ExtendedQualName::NORMAL_TABLE) 
    if (bindWA->violateAccessDefaultSchemaOnly(tableName_.getQualifiedNameObj()))
      return this;

  //
  // Bind the base class.
  //
  RelExpr *boundExpr = bindSelf(bindWA);
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
short ExeUtilDisplayExplain::setOptionsX()
{
  // optionX_ default set to 'n' by constructor
  if (optionsStr_)                  // if specified by user, validate and save
    {
      char c = optionsStr_[0];      // pick up first char of input
      if (c != '\0')                // if an option is provided
        {
          if (optionsStr_[1] != '\0') {c = '\0'; } // check if more than one, force fail
        }
      switch(c)
        {
          case 'e' :                // expert mode
          case 'f' :                // summary mode
          case 'm' :                // machine readable mode
          case 'n' :                // normal mode
            break;

          default  :                // fail all else
            return -1;
        }
      optionX_ = c;                 // it is valid, save it
    }

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
      *CmpCommon::diags() << DgSqlCode(-15517);
      bindWA->setErrStatus();

      return NULL;
    }

  boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  return boundExpr;
}

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

  RelExpr * boundExpr = GenericUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus())
    return NULL;

  isCreate_ = FALSE;
  isCreateLike_ = FALSE;
  isDrop_ = FALSE;
  isAlter_ = FALSE;
  isHbase_ = FALSE;
  isNative_ = FALSE;
  hbaseDDLNoUserXn_ = FALSE;

  NABoolean isSeq = FALSE;

  NABoolean alterAddCol = FALSE;
  NABoolean alterDropCol = FALSE;
  NABoolean alterDisableIndex = FALSE;
  NABoolean alterEnableIndex = FALSE;
  NABoolean otherAlters = FALSE;
  NABoolean isPrivilegeMngt = FALSE;
  NABoolean isCreateSchema = FALSE;
  NABoolean isDropSchema = FALSE;
  NABoolean isAuth = FALSE;
  NABoolean alterAddConstr = FALSE;
  NABoolean alterDropConstr = FALSE;
  NABoolean alterRenameTable = FALSE;
  NABoolean alterIdentityCol = FALSE;

  NABoolean specialType = FALSE;
  if (isUstat())  // special DDLExpr node for an Update Stats statement
    return boundExpr;

  if (initAuthorization() || dropAuthorization())
  {
    isHbase_ = TRUE;
    hbaseDDLNoUserXn_ = TRUE;
    return boundExpr;
  }

  if (initHbase_ || dropHbase_ || createMDViews() || dropMDViews() ||
      addSeqTable() || updateVersion())
  {
    isHbase_ = TRUE;
    hbaseDDLNoUserXn_ = TRUE;
    return boundExpr;
  }
  else if (purgedataHbase_)
  {
    isHbase_ = TRUE;
    hbaseDDLNoUserXn_ = TRUE;      
  }
  else if (getExprNode() && getExprNode()->castToElemDDLNode())
  {
    if (getExprNode()->castToElemDDLNode()->castToStmtDDLCreateTable())
    {
      StmtDDLCreateTable * createTableNode =
        getExprNode()->castToElemDDLNode()->castToStmtDDLCreateTable();

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
        *CmpCommon::diags() << DgSqlCode(-4222) << DgString0("DDL");
        bindWA->setErrStatus();
        return NULL;
      }

      // if unique, ref or check constrs are specified, then dont start a transaction.
      // ddl with these clauses is executed as a compound create.
      // A compound create cannot run under a user transaction.
      if ((createTableNode->getAddConstraintUniqueArray().entries() > 0) ||
	  (createTableNode->getAddConstraintRIArray().entries() > 0) ||
	  (createTableNode->getAddConstraintCheckArray().entries() > 0))
	hbaseDDLNoUserXn_ = TRUE;
       
    } // createTable
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLCreateHbaseTable())
    {
      StmtDDLCreateHbaseTable * createHbaseTableNode =
	getExprNode()->castToElemDDLNode()->castToStmtDDLCreateHbaseTable();
      
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
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLCreateIndex())
    {
      isCreate_ = TRUE;
      isIndex_ = TRUE;

      objName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLCreateIndex()->
        getIndexName();

      qualObjName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLCreateIndex()->
        getIndexNameAsQualifiedName();
    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLPopulateIndex())
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
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLDropTable())
    {
      isDrop_ = TRUE;
      isTable_ = TRUE;

      qualObjName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLDropTable()->
        getTableNameAsQualifiedName();
    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLDropHbaseTable())
    {
      isDrop_ = TRUE;
      isTable_ = TRUE;
      isNative_ = TRUE;
      isHbase_ = TRUE;

      qualObjName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLDropHbaseTable()->
        getTableNameAsQualifiedName();
    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLDropIndex())
    {
      isDrop_ = TRUE;
      isIndex_ = TRUE;

      qualObjName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLDropIndex()->
        getIndexNameAsQualifiedName();
    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLAlterTable())
    {
      isAlter_ = TRUE;
      isTable_ = TRUE;

      if (getExprNode()->castToElemDDLNode()->castToStmtDDLAlterTableAddColumn())
        alterAddCol = TRUE;
      else if (getExprNode()->castToElemDDLNode()->castToStmtDDLAlterTableDropColumn())
        alterDropCol = TRUE;
      else if (getExprNode()->castToElemDDLNode()->castToStmtDDLAlterTableDisableIndex())
        alterDisableIndex = TRUE;
      else if (getExprNode()->castToElemDDLNode()->castToStmtDDLAlterTableEnableIndex())
        alterEnableIndex = TRUE;
      else if ((getExprNode()->castToElemDDLNode()->castToStmtDDLAddConstraintUnique()) ||
	       (getExprNode()->castToElemDDLNode()->castToStmtDDLAddConstraintRI()) ||
	       (getExprNode()->castToElemDDLNode()->castToStmtDDLAddConstraintCheck()))
         alterAddConstr = TRUE;
      else if (getExprNode()->castToElemDDLNode()->castToStmtDDLDropConstraint())
         alterDropConstr = TRUE;
      else if (getExprNode()->castToElemDDLNode()->castToStmtDDLAlterTableRename())
         alterRenameTable = TRUE;
      else if (getExprNode()->castToElemDDLNode()->castToStmtDDLAlterTableAlterColumnSetSGOption())
         alterIdentityCol = TRUE;
       else
        otherAlters = TRUE;

      qualObjName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLAlterTable()->
        getTableNameAsQualifiedName();
    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLCreateView())
    {
      isCreate_ = TRUE;
      isView_ = TRUE;

      qualObjName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLCreateView()->
        getViewNameAsQualifiedName();
    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLDropView())
    {
      isDrop_ = TRUE;
      isView_ = TRUE;

      qualObjName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLDropView()->
        getViewNameAsQualifiedName();
    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLCreateSequence())
    {
      StmtDDLCreateSequence * createSeq =
	getExprNode()->castToElemDDLNode()->castToStmtDDLCreateSequence();
      
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
   else if (getExprNode()->castToElemDDLNode()->castToStmtDDLDropSequence())
    {
      StmtDDLDropSequence * dropSeq =
	getExprNode()->castToElemDDLNode()->castToStmtDDLDropSequence();
      
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
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLRegisterUser())
    {
      isAuth = TRUE; 
    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLAlterUser())
    {
      isAuth = TRUE; 
    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLCreateRole())
    {
      isAuth = TRUE; 
    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLRoleGrant())
    {
      isPrivilegeMngt = TRUE; 
    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLRegisterComponent())
    {
      isPrivilegeMngt = TRUE; 
    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLCreateComponentPrivilege())
    {
      isPrivilegeMngt = TRUE; 
    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLDropComponentPrivilege())
    {
      isPrivilegeMngt = TRUE; 
    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLGrantComponentPrivilege())
    {
      isPrivilegeMngt = TRUE; 
    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLRevokeComponentPrivilege())
    {
      isPrivilegeMngt = TRUE; 
    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLGrant())
    {
      isTable_ = TRUE;
      isPrivilegeMngt = TRUE;
      qualObjName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLGrant()->
        getGrantNameAsQualifiedName();
    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLRevoke())
    {
      isTable_ = TRUE;
      isPrivilegeMngt = TRUE;
      qualObjName_ =
        getDDLNode()->castToStmtDDLNode()->castToStmtDDLRevoke()->
        getRevokeNameAsQualifiedName();
    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLDropSchema())
    {
      isDropSchema = TRUE;
      qualObjName_ =
        QualifiedName(NAString("dummy"),
                      getExprNode()->castToElemDDLNode()->castToStmtDDLDropSchema()->getSchemaNameAsQualifiedName().getSchemaName(),
                      getExprNode()->castToElemDDLNode()->castToStmtDDLDropSchema()->getSchemaNameAsQualifiedName().getCatalogName());

    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLCreateSchema())
    {
      isCreateSchema = TRUE;
      qualObjName_ =
        QualifiedName(NAString("dummy"),
                      getExprNode()->castToElemDDLNode()->castToStmtDDLCreateSchema()->getSchemaNameAsQualifiedName().getSchemaName(),
                      getExprNode()->castToElemDDLNode()->castToStmtDDLCreateSchema()->getSchemaNameAsQualifiedName().getCatalogName());
    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLCreateLibrary())
    {
      isCreate_ = TRUE;
      isLibrary_ = TRUE;
      qualObjName_ = getExprNode()->castToElemDDLNode()->
        castToStmtDDLCreateLibrary()->getLibraryNameAsQualifiedName();
    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLDropLibrary())
    {
      isDrop_ = TRUE;
      isLibrary_ = TRUE;
      qualObjName_ = getExprNode()->castToElemDDLNode()->
        castToStmtDDLDropLibrary()->getLibraryNameAsQualifiedName();
    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLCreateRoutine())
    {
      isCreate_ = TRUE;
      isRoutine_ = TRUE;
      qualObjName_ = getExprNode()->castToElemDDLNode()->
        castToStmtDDLCreateRoutine()->getRoutineNameAsQualifiedName();
    }
    else if (getExprNode()->castToElemDDLNode()->castToStmtDDLDropRoutine())
    {
      isDrop_ = TRUE;
      isRoutine_ = TRUE;
      qualObjName_ = getExprNode()->castToElemDDLNode()->
        castToStmtDDLDropRoutine()->getRoutineNameAsQualifiedName();
    }
    

    if ((isCreateSchema || isDropSchema) ||
        ((isTable_ || isIndex_ || isView_ || isRoutine_ || isLibrary_ || isSeq) &&
         (isCreate_ || isDrop_ || purgedataHbase_ ||
          (isAlter_ && (alterAddCol || alterDropCol || alterDisableIndex || alterEnableIndex || 
			alterAddConstr || alterDropConstr || alterRenameTable || 
                        alterIdentityCol || otherAlters)))))
      {
	if (NOT isNative_)
	  {
	    qualObjName_.applyDefaults(bindWA->getDefaultSchema());
	    
	    if (qualObjName_.isSeabase())
	      {
		isHbase_ = TRUE;
	      }
	  }
	
	if (isHbase_ && otherAlters)
	  {
	    *CmpCommon::diags() << DgSqlCode(-4222) << DgString0("ALTER");
	    bindWA->setErrStatus();
	    return NULL;
	  }
	
	// if a user ddl operation, it cannot run under a user transaction.
	// If an internal ddl request, like a CREATE internally issued onbehalf
	// of a CREATE LIKE, then allow it to run under a user Xn.
	if (NOT Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
	  hbaseDDLNoUserXn_ = TRUE;
      }
    else if (isAuth || isPrivilegeMngt)
      {
        isHbase_ = TRUE;
      }

    else
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

  if (isHbase_)
    return boundExpr;

  *CmpCommon::diags() << DgSqlCode(-4222) << DgString0("DDL");
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
		      getExprNode()->castToElemDDLNode()->castToStmtDDLCreateSchema()->getSchemaNameAsQualifiedName().getSchemaName(),
		      getExprNode()->castToElemDDLNode()->castToStmtDDLCreateSchema()->getSchemaNameAsQualifiedName().getCatalogName());
    }
  else if (getDDLNode()->castToStmtDDLNode() &&
	   getDDLNode()->castToStmtDDLNode()->castToStmtDDLDropSchema())
    {
      isCreate_ = FALSE;
      isSchema_ = TRUE;

      volTabName_ = 
	QualifiedName(NAString("dummy"),
		      getExprNode()->castToElemDDLNode()->castToStmtDDLDropSchema()->getSchemaNameAsQualifiedName().getSchemaName(),
		      getExprNode()->castToElemDDLNode()->castToStmtDDLDropSchema()->getSchemaNameAsQualifiedName().getCatalogName());
    }
    
  RelExpr * boundExpr = DDLExpr::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;
  
  isHbase_ = FALSE;
  if ((CmpCommon::getDefault(MODE_SEABASE) == DF_ON) &&
      (CmpCommon::getDefault(SEABASE_VOLATILE_TABLES) == DF_ON))
    {
      volTabName_.applyDefaults(bindWA->getDefaultSchema());
      if (volTabName_.isSeabase())
	{
	  isHbase_ = TRUE;
	}
    }
  
  if (NOT isHbase_)
    {
      // non-hbase tables not supported in open source
      *CmpCommon::diags() << DgSqlCode(-4222) << DgString0("DDL");
      bindWA->setErrStatus();
      return NULL;
    }

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

#pragma nowarn(1506)   // warning elimination 
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
#pragma warn(1506)  // warning elimination
  
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
  NABoolean sidetreeInsertAllowed = TRUE;
  Int32 errorcode = 0;
  NAWcharBuf* wcbuf = 0;
      
  if ((getExprNode()) &&
      (getExprNode()->castToElemDDLNode()) &&
      (getExprNode()->castToElemDDLNode()->castToStmtDDLCreateTable()))
    createTableNode =
      getExprNode()->castToElemDDLNode()->castToStmtDDLCreateTable();

  CorrName savedTableName = getTableName();
  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  // open source path
  if ((NOT getTableName().isSeabase()) ||
      (createTableNode->isInMemoryObjectDefn()) ||
      (createTableNode->isMultiSetTable()) ||
      (createTableNode->isSetTable()))
    {
      return NULL;
    }


  NABoolean isSeabase = FALSE;
  //  if (NATableDB::isHbaseTable(getTableName()))
  if (getTableName().isSeabase())
    {
      isSeabase = TRUE;

      // no sidetree inserts into hbase tables
      sidetreeInsertAllowed = FALSE;
    }

  if (createTableNode && (createTableNode->isVolatile()))
    {    
      getTableName() = savedTableName;
      isVolatile_ = TRUE;
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
	  else
	    ctQuery_ = "CREATE TABLE ";
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
		  colType.getMyTypeAsText(&colDef);

		  if (colType.isLob())
		    sidetreeInsertAllowed = FALSE;

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
			    sidetreeInsertAllowed = FALSE;
			}
		      else
			{
			  numUntypedColDefEntries++;

			  if (numUntypedColDefEntries > retDesc->getDegree())
			    {
			      continue;
			    }
			  
			  colDef += 
			    ToAnsiIdentifier(colDefNode->castToElemDDLColDef()->
					     getColumnName());
	      
			  colDef += " ";
			  
			  NAType &colType = (NAType&)(queryRoot->compExpr()[j].getType());
			  colType.getMyTypeAsText(&colDef);

			  if (colType.isLob())
			    sidetreeInsertAllowed = FALSE;
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
      
      if (createTableNode->isInMemoryObjectDefn())
	ctQuery_.append(" IN MEMORY ");

      if (createTableNode->isSetTable())
	sidetreeInsertAllowed = FALSE;

      //      ctQuery_ += ";";

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

  if (sidetreeInsertAllowed)
    {
      siQuery_ = "insert using sideinserts into ";
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
      usQuery_ += " ON EVERY KEY SAMPLE SET ROWCOUNT %Ld;";
    }

  return boundExpr;
}
// -----------------------------------------------------------------------
// member functions for class ExeUtilFastDelete
// -----------------------------------------------------------------------
RelExpr * ExeUtilFastDelete::bindNode(BindWA *bindWA)
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

  if (NOT doPurgedataCat_)
    {
      // do not do override schema for this
      bindWA->setToOverrideSchema(FALSE);

      naTable = bindWA->getNATable(getTableName());
      if (getTableName().isSeabase())
	{
	  if (bindWA->errStatus())
	    return this;

	  DDLExpr * ddlExpr = new(bindWA->wHeap()) DDLExpr(TRUE,
							   getTableName(),
							   getStmtText(),
							   CharInfo::UnknownCharSet);
	  RelExpr * boundExpr = ddlExpr->bindNode(bindWA);
	  return boundExpr;
	}
      
      if (bindWA->errStatus())
	{
	  naTable = NULL;
	  CmpCommon::diags()->clear();
	  bindWA->resetErrStatus();

	}
    }

  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus())
    return NULL;

  if ((! getTableName().isSeabase()) &&
      (! getTableName().isHbase()) &&
      (! getTableName().isHive()))
    {
      *CmpCommon::diags() << DgSqlCode(-4222) << DgString0("PURGEDATA");
      bindWA->setErrStatus();
      return NULL;
    }

  if (naTable && (!naTable->isHiveTable()))
    {
      *CmpCommon::diags() << DgSqlCode(-4222) << DgString0("PURGEDATA");
      bindWA->setErrStatus();
      return NULL;
    }
  return boundExpr;
}

// -----------------------------------------------------------------------
// member functions for class ExeUtilReorg
// -----------------------------------------------------------------------
RelExpr * ExeUtilReorg::bindNode(BindWA *bindWA)
{
  return NULL;
}

// -----------------------------------------------------------------------
// member functions for class ExeUtilReplicate
// -----------------------------------------------------------------------
NABoolean ExeUtilReplicate_checkReplicateCQD(Int32 userID)
{
  return FALSE;
}


NABoolean ExeUtilReplicate_checkUserHasReplicateAuthority(Int32 userID,
                                                          NATable *naTable)
{
  return TRUE;
}

RelExpr * ExeUtilReplicate::bindNode(BindWA *bindWA)
{
  *CmpCommon::diags() << DgSqlCode(-4222) << DgString0("Replicate");

  bindWA->setErrStatus();
  return this;
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
// member functions for class ExeUtilUserLoad
// -----------------------------------------------------------------------
ExeUtilUserLoad::ExeUtilUserLoad(const CorrName &name,
				 RelExpr * child,
				 ItemExpr * eodExpr,
				 ItemExpr * partnNumExpr,
				 NAList<UserLoadOption*> * userLoadOptionsList,
				 CollHeap *oHeap)
     : ExeUtilExpr(USER_LOAD_, name, NULL, child, NULL, CharInfo::UnknownCharSet, oHeap),
       eodExpr_(eodExpr),
       partnNumExpr_(partnNumExpr),
       maxRowsetSize_(0),
       rwrsInputSizeExpr_(NULL),
       rwrsMaxInputRowlenExpr_(NULL),
       rwrsBufferAddrExpr_(NULL),
       excpTabDesc_(NULL),
       excpRowsPercentage_(-1),
       excpRowsNumber_(-1),
       loadId_(-1),
       ingestNumSourceProcesses_(-1),
       flags_(0)
{
  setDoFastLoad(TRUE);

  if (userLoadOptionsList)
    {
      for (CollIndex i = 0; i < userLoadOptionsList->entries(); i++)
	{
	  UserLoadOption * fro = (*userLoadOptionsList)[i];
	  switch (fro->option_)
	    {
	    case OLD_LOAD_:
	      {
		setDoFastLoad(FALSE);
	      }
	    break;

	    case WITH_SORT_:
	      {
		setDoSortFromTop(TRUE);
	      }
	    break;

	    case EXCEPTION_TABLE_:
	      {
		excpTabNam_ = *((CorrName*)(fro->stringVal_));
		child->setTolerateNonFatalError(RelExpr::NOT_ATOMIC_);
	      }
	    break;

	    case EXCEPTION_ROWS_PERCENTAGE_:
	      {
		excpRowsPercentage_ = (Lng32)fro->numericVal_;
	      }
	    break;

	    case EXCEPTION_ROWS_NUMBER_:
	      {
		excpRowsNumber_ = (Lng32)fro->numericVal_;
	      }
	    break;

	    case LOAD_ID_:
	      {
		loadId_ = fro->numericVal_;
	      }
	    break;

	    case INGEST_MASTER_:
	      {
		setIngestMaster(TRUE);
		ingestNumSourceProcesses_ = (Lng32)fro->numericVal_;
	      }
	    break;

	    case INGEST_SOURCE_:
	      {
		setIngestSource(TRUE);
		ingestTargetProcessIds_ = (char*)fro->stringVal_;
	      }
	    break;

	    }
	} // for
    }
};

RelExpr * ExeUtilUserLoad::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // do not do override schema for this
  bindWA->setToOverrideSchema(FALSE);

  // Allocate a TableDesc and attach it to this.
  //
  NATable *naTable = bindWA->getNATable(getTableName());
  if (bindWA->errStatus()) 
    return this;

  NATable *excpNATable = NULL;
  if (NOT excpTabNam().isEmpty())
    {
      excpNATable = bindWA->getNATable(excpTabNam_);
      if (bindWA->errStatus()) 
	return this;

      bindWA->getCurrentScope()->xtnmStack()->createXTNM();
      excpTabDesc_ = bindWA->createTableDesc(excpNATable, excpTabNam_);
      bindWA->getCurrentScope()->xtnmStack()->removeXTNM();
      if (bindWA->errStatus())
	return this;
    }

  bindWA->getCurrentScope()->xtnmStack()->createXTNM();
  setUtilTableDesc(bindWA->createTableDesc(naTable, getTableName()));
  bindWA->getCurrentScope()->xtnmStack()->removeXTNM();
  if (bindWA->errStatus())
    return this;

  if (! eodExpr_)
    {
      CMPASSERT(eodExpr_);

      bindWA->setErrStatus();
      return this;
    }

  NAType * typ = new(bindWA->wHeap()) SQLInt(FALSE, FALSE); 
  ItemExpr * newExpr = new(bindWA->wHeap()) Cast(eodExpr_, typ);
  if (bindWA->errStatus()) 
    return this;

  eodExpr_ = newExpr->bindNode(bindWA);
  if (bindWA->errStatus()) 
    return this;

  if (partnNumExpr_)
    {
      NAType * typ = new(bindWA->wHeap()) SQLInt(FALSE, FALSE); 
      ItemExpr * newExpr = new(bindWA->wHeap()) Cast(partnNumExpr_, typ);
      if (bindWA->errStatus()) 
	return this;
      
      partnNumExpr_ = newExpr->bindNode(bindWA);
      if (bindWA->errStatus()) 
	return this;
    }

  // We get the list of input host vars, which is stored in the root of the
  // parse tree
  HostArraysWA *arrayArea = bindWA->getHostArraysArea();
  
  if ((arrayArea->rwrsMaxSize()->getOperatorType() != ITM_CONSTANT) ||
      (((ConstValue *)arrayArea->rwrsMaxSize())->getType()->getTypeQualifier()
       != NA_NUMERIC_TYPE))
    {
      // 30003 Rowset size must be an integer host variable or an
      //       integer constant
      *CmpCommon::diags() << DgSqlCode(-30003);
      bindWA->setErrStatus();
      return NULL;
    }
  
  maxRowsetSize_ = 
    (Lng32)((ConstValue *)arrayArea->rwrsMaxSize())->getExactNumericValue() ;
  
  typ = new(bindWA->wHeap()) SQLInt(FALSE, FALSE); 
  rwrsInputSizeExpr_ = 
    new(bindWA->wHeap()) Cast(arrayArea->inputSize(), typ);
  rwrsInputSizeExpr_ = rwrsInputSizeExpr_->bindNode(bindWA);
  if (bindWA->errStatus()) 
    return this;
  
  rwrsMaxInputRowlenExpr_ =
    new(bindWA->wHeap()) Cast(arrayArea->rwrsMaxInputRowlen(), typ);
  rwrsMaxInputRowlenExpr_ = rwrsMaxInputRowlenExpr_->bindNode(bindWA);
  if (bindWA->errStatus()) 
    return this;
  
#ifdef NA_64BIT
  typ = new(bindWA->wHeap()) SQLLargeInt(TRUE, FALSE); 
#endif
  rwrsBufferAddrExpr_ =
    new(bindWA->wHeap()) Cast(arrayArea->rwrsBuffer(), typ);
  rwrsBufferAddrExpr_ = rwrsBufferAddrExpr_->bindNode(bindWA);
  if (bindWA->errStatus()) 
    return this;

  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return NULL;

  if ((naTable->getViewFileName()) ||
      (naTable->isAnMV()) ||
      (naTable->isAnMVMetaData()) ||
      (naTable->isAnMPTableWithAnsiName()) ||
      (naTable->isUMDTable ()) ||
      (naTable->isSMDTable ()) ||
      (naTable->isMVUMDTable ()) ||
      (naTable->isTrigTempTable ()) ||
      (naTable->getClusteringIndex()->isAudited()))
    {
      *CmpCommon::diags() << DgSqlCode(-4219);
      bindWA->setErrStatus();
      return this;
    }

  // can't do fast load for audited tables.
  if (getUtilTableDesc()->getClusteringIndex()->getNAFileSet()->isAudited())
    setDoFastLoad(FALSE);

  if (NOT excpTabNam().isEmpty())
    {
      excpTabInsertStmt_ = "insert into ";
      excpTabInsertStmt_ += excpTabNam().getQualifiedNameAsString();
      excpTabInsertStmt_ += " values (";
      
      UInt32 colcount = excpNATable->getUserColumnCount();

      for (UInt32 i = 0; i < colcount-1; i++)
	{
	  excpTabInsertStmt_ += " ?, ";
	}

      excpTabInsertStmt_ += " ? );";
    }

  return boundExpr;
}

// -----------------------------------------------------------------------
// member functions for class ExeUtilSidetreeInsert
// -----------------------------------------------------------------------
ExeUtilSidetreeInsert::ExeUtilSidetreeInsert(const CorrName &name,
					     RelExpr * child,
					     char * stmtText,
					     CharInfo::CharSet stmtTextCharSet,
					     CollHeap *oHeap)
     : ExeUtilExpr(ST_INSERT_, name, NULL, child, 
		   stmtText, stmtTextCharSet, oHeap)
{
}

RelExpr * ExeUtilSidetreeInsert::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // do not do override schema for this
  bindWA->setToOverrideSchema(FALSE);

  // Allocate a TableDesc and attach it to this.
  //
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

RelExpr * ExeUtilSidetreeInsert::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilSidetreeInsert *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExeUtilSidetreeInsert
      (getTableName(),
       NULL, 
       NULL,
       CharInfo::UnknownCharSet, outHeap);
  else
    result = (ExeUtilSidetreeInsert *) derivedNode;
  
  return ExeUtilExpr::copyTopNode(result, outHeap);
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
// member functions for class ExeUtilGetDiskLabelStats
// -----------------------------------------------------------------------
RelExpr * ExeUtilGetDiskLabelStats::bindNode(BindWA *bindWA)
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
	  *CmpCommon::diags() << DgSqlCode(-4218) << DgString0("GET ");
	  
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

// -----------------------------------------------------------------------
// member functions for class ExeUtilGetDiskStats
// -----------------------------------------------------------------------
RelExpr * ExeUtilGetDiskStats::bindNode(BindWA *bindWA)
{
  if (nodeIsBound()) {
    bindWA->getCurrentScope()->setRETDesc(getRETDesc());
    return this;
  }

  // save the current parserflags setting
  ULng32 savedParserFlags = Get_SqlParser_Flags (0xFFFFFFFF);

  // allow funny characters in the tablename. This can happen if user
  // didn't specify a tablename and we used the internal MAINTAIN table
  // which contains a 'funny' character.
  Set_SqlParser_Flags(ALLOW_FUNNY_IDENTIFIER);

  RelExpr * boundExpr = ExeUtilExpr::bindNode(bindWA);

  // Restore parser flags settings to what they originally were
  Set_SqlParser_Flags (savedParserFlags);

  if (bindWA->errStatus()) 
    return NULL;

  return boundExpr;
}

// -----------------------------------------------------------------------
// member functions for class ExeUtilGetFormattedDiskStats
// -----------------------------------------------------------------------
RelExpr * ExeUtilGetFormattedDiskStats::bindNode(BindWA *bindWA)
{
  return NULL;
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
// -----------------------------------------------------------------------
// ExeUtilGetDiskLabelStats::recomputeOuterReferences()
// -----------------------------------------------------------------------
void ExeUtilGetDiskLabelStats::recomputeOuterReferences()
{
  ValueIdSet outerRefs = getGroupAttr()->getCharacteristicInputs(); 
  outerRefs += inputColList_->getValueId();

  getGroupAttr()->setCharacteristicInputs(outerRefs);
} // ExeUtilGetDiskLabelStats::recomputeOuterReferences()  


//////////////////////////////////////////////////////////////////////////////
// OptPhysRelExpr methods for ExeUtil operators
//////////////////////////////////////////////////////////////////////////////
PhysicalProperty * ExeUtilLongRunning::synthPhysicalProperty(const Context * context,
                                                                      const Lng32 planNumber)
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




PhysicalProperty*
ExeUtilGetDiskStats::synthPhysicalProperty(const Context* context,
					   const Lng32     planNumber)
{
  // ---------------------------------------------------------------------
  // Simply propogate child's physical property.
  // ---------------------------------------------------------------------
  const PhysicalProperty* const sppOfTheChild =
                    context->getPhysicalPropertyOfSolutionForChild(0);

  PhysicalProperty* sppForMe = new (CmpCommon::statementHeap())
    PhysicalProperty (*sppOfTheChild);

  sppForMe->setLocation(EXECUTE_IN_DP2);

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  return sppForMe;
}


// -----------------------------------------------------------------------
// Member functions for class ExeUtilHbaseCoProcAggr
// -----------------------------------------------------------------------
RelExpr * ExeUtilHbaseCoProcAggr::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExeUtilHbaseCoProcAggr *result;

  if (derivedNode == NULL)
    result = new (outHeap) 
      ExeUtilHbaseCoProcAggr(getTableName(), aggregateExpr(), outHeap);
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
                      NULL, CharInfo::UnknownCharSet, outHeap);
  else
    result = (ExeUtilHBaseBulkLoad *) derivedNode;

  result->keepHFiles_ = keepHFiles_;
  result->truncateTable_ = truncateTable_;
  result->noRollback_= noRollback_;
  result->logErrors_ = logErrors_ ;
  result->noDuplicates_= noDuplicates_;
  result->indexes_= indexes_;
  result->constraints_= constraints_;
  result->noOutput_= noOutput_;
  result->indexTableOnly_= indexTableOnly_;
  result->upsertUsingLoad_= upsertUsingLoad_;

  return ExeUtilExpr::copyTopNode(result, outHeap);
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
      case NO_POPULATE_INDEXES_:
      {
        if (!getIndexes())
        {
          //4488 bulk load option $0~String0 cannot be specified more than once.
          *da << DgSqlCode(-4488)
                  << DgString0("NO POPULATE INDEXES");
          return 1;
        }
        setIndexes(FALSE);
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
      case STOP_AFTER_N_ERRORS_:
      {
        *da << DgSqlCode(-4485)
        << DgString0(": STOP AFTER N ERRORS.");
        return 1;
      }
      break;
      case LOG_ERRORS_:
      {
        *da << DgSqlCode(-4485)
        << DgString0(": ERROR LOGGING.");
        return 1;
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
      default:
      {
        CMPASSERT(0);
        return 1;
      }

    }
  }

  if (getIndexTableOnly())
  {
    // target table is index then : no output, no secondary index maintenance
    // and no constraint maintenance
    setNoOutput(TRUE);
    setIndexes(FALSE);
    setConstraints(FALSE);
  }

  return 0;

};
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
  return ExeUtilExpr::copyTopNode(result, outHeap);
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
            delimiterSqlStr.append(" '");
            delimiterSqlStr.append( lo->stringVal_);
            delimiterSqlStr.append("' ");
          }
          else
          {
            char buf[5];
            sprintf(buf,"%u",(unsigned int) lo->stringVal_[0]);
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
            recordSeparatorSqlStr.append(" '");
            recordSeparatorSqlStr.append( lo->stringVal_);
            recordSeparatorSqlStr.append("' ");
          }
          else
          {
            char buf[5];
            sprintf(buf,"%u",(unsigned int) lo->stringVal_[0]);
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
      default:
      {
        CMPASSERT(0);
        return 1;
      }
      }
    }

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


