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
* File:         GenExplain.C
* Description:  addSpecificExplainInfo methods for classes deriving from RelExpr
*               addExplainInfo and addExplainPredicates for RelExpr itself
*
* Created:      5/8/96
* Language:     C++
*
*
******************************************************************************
*/

#include "AllRelExpr.h"
#include "Cost.h"
#include "Generator.h"
#include "GroupAttr.h"
#include "PhyProp.h"
#include "ComSmallDefs.h"
#include "ComTdb.h"
#include "ComTdbSplitBottom.h"
#include "ComTdbSendBottom.h"
#include "ComTdbExplain.h"
#include "ComTdbSort.h"
#include "ComTdbRoot.h"
#include "ComTransInfo.h"
#include "ControlDB.h"
#include "ExplainTuple.h"
#include "ExplainTupleMaster.h"
#include "CmpStatement.h"
#include "ComTdbSendTop.h"
#include "ComTdbExeUtil.h"
#include "ComTdbHashj.h"
#include "ComTdbHashGrby.h"
#include "ComTdbMj.h"
#include "ComTdbSequence.h"
#include "ComTdbCancel.h"
#include "HDFSHook.h"

#include "StmtDDLCreateTable.h"
#include "StmtDDLCreateIndex.h"
#include "StmtDDLonHiveObjects.h"
#include "ComDistribution.h"
#include "TrafDDLdesc.h"


void RelExpr::addExplainPredicates(ExplainTupleMaster * explainTuple,
				       Generator * generator)

{
  // exprList can use statement heap as the elements of the list are
  // on statement heap
  NAList<ExprNode *> exprList(generator->wHeap());

  // labelList is on global heap to avoid memory lekas; if it were on
  // statement heap, it would cause memory leak as the elements of the
  // list on global heap CR 10-010813-4515
  NAList<NAString>   labelList(generator->wHeap());

  ExprNode           *currExpr = NULL;
  NAString           unParsed((size_t)4096, generator->wHeap());

  //to be used in cleaning up the output and removing full ANSI names
  NAString tname = GenGetQualifiedName(getTableName());
  bool remTableNames = true;

  if (tname == GenGetQualifiedName(RelExpr::invalid))
	remTableNames = false;
  else
    tname += ".";

  addLocalExpr(exprList,labelList);

  // get the first element (and remove) until no more elements
  while(exprList.getFirst(currExpr) == TRUE)
  {
      // Possible to have a NULL entry
      if(currExpr)
      {
          labelList.getFirst(unParsed); //if labelList's entry is of certain kind,
                                        //should be different format of unparsing

          // added to avoid stack overflow when the statement has a very large
          // input variable list. This can happen if the statement is an
          // insert statement with a very large number of tuples. The issue
          // is compounded if the table has a large number of columns.
          // In this case we just skip the input variables expression
          if ((this->getOperatorType() == REL_ROOT) &&
              (unParsed == "input_variables") &&
              (((RelRoot *) this)->inputVars().entries() > CmpCommon::getDefaultLong(EXPLAIN_ROOT_INPUT_VARS_MAX))
             )
          {
             unParsed += ": List too Long - skipped";
          }
          else
          {
             unParsed += ": ";

             currExpr->unparse(unParsed, OPTIMIZER_PHASE, EXPLAIN_FORMAT);
             unParsed += " ";
             if (remTableNames)
             {
                //find if need to remove full table name and where from
                size_t index = unParsed.index(tname);
                while (index != NA_NPOS)
                {
                   unParsed.remove(index, tname.length());
                   index = unParsed.index(tname, index);  // start searching from the last position 
                                                          // where index was found
                }
             }
          }
          explainTuple->setDescription(unParsed);
      }
      else
      {
         // skip this label
         labelList.getFirst(unParsed); 
      }
  }

  // make sure all elements of the list are deleted CR 10-010813-4515
  labelList.deallocate();
}

// Default virtual routine for adding explain information
// to the TDB.  Most nodes will provide their own version
// of this routine.

ExplainTuple *
RelExpr::addExplainInfo(ComTdb * tdb,
			ExplainTuple *leftChild,
			ExplainTuple *rightChild,
			Generator *generator)
{

  if(generator->getExplainFragDirIndex() == NULL_COLL_INDEX)
    {
      // Create an Explain Fragment
      generator->setExplainFragDirIndex(
	   generator->getFragmentDir()->pushFragment(FragmentDir::EXPLAIN,0));
      generator->getFragmentDir()->popFragment();
    }

  CollIndex fragIndex = generator->getExplainFragDirIndex();

  Space *space = generator->getFragmentDir()->getSpace(fragIndex);

  ExplainDesc *explainDesc =
    (ExplainDesc *)generator->getFragmentDir()->getTopNode(fragIndex);

  ExplainTupleMaster *explainTuple =
    (ExplainTupleMaster *)new(space) ExplainTuple(leftChild,
						  rightChild,
						  explainDesc);

  Lng32 explainNodeId = generator->getNextExplainNodeId();

  NAString operText(getText());
  size_t blankIndex = operText.index(" ");

  // take the first word from getText() and upshift it
  if (blankIndex != NA_NPOS)
    operText.remove(blankIndex); // cuts off chars including space

  // upshift
  operText.toUpper();

  NABoolean doExplainSpaceOpt = FALSE;
  if (CmpCommon::getDefault(EXPLAIN_SPACE_OPT) == DF_ON)
    doExplainSpaceOpt = TRUE;
  explainTuple->init(space, doExplainSpaceOpt);

  explainTuple->setPlanId(generator->getPlanId());

  explainTuple->setOperator(operText);
  explainTuple->setSeqNum(explainNodeId);
  tdb->setExplainNodeId(explainNodeId);
  if (leftChild)
    explainTuple->setChildSeqNum(0,leftChild->getSeqNum());
  if (rightChild)
    explainTuple->setChildSeqNum(1,rightChild->getSeqNum());

  explainTuple->setCardinality((MINOF(getEstRowsUsed(), 1e32)).value());

  Cost const *operatorCost = getOperatorCost();
  Cost const *rollUpCost = getRollUpCost();
  NAString
    costDetail
    ,ocDetail  = "OPERATOR_COST: Zero cost"
    ,ruDetail  = "ROLLUP_COST: Zero cost"
    ;

  NABoolean explainForCalibration =
   ( CmpCommon::getDefault(EXPLAIN_DETAIL_COST_FOR_CALIBRATION)==DF_ON);

  if (operatorCost)
    {
      explainTuple->
        setOperatorCost((MINOF(operatorCost->displayTotalCost(), 1e32)).getValue());
      // Set detail info for calibration:
       if (explainForCalibration)
          {

            char line[256];
            sprintf(line, "OPERATOR_COST: ");
            ocDetail = line;
            ocDetail += operatorCost->getDetailDesc();
          }
      else
        {
          ocDetail = "";
        }
    }
  else
    {
      explainTuple->setOperatorCost(0.0);
    }

  if (rollUpCost)
    {
      explainTuple->
        setTotalCost((MINOF(rollUpCost->displayTotalCost(), 1e32)).getValue());

     // Set detail info for calibration:
      if (explainForCalibration)  
      {
        char line[256];
        sprintf(line, "ROLLUP_COST: ");
        ruDetail = line;
        ruDetail += rollUpCost->getDetailDesc();
      }
      else
        ruDetail = rollUpCost->getDetailDesc();
    }

  costDetail = ocDetail + ruDetail;
  explainTuple->setDetailCost((char *)(const char *)costDetail);

  // Add a tuple_format token to description
  // e.g. "tuple_format: 2:? 3:Aligned"
  ex_cri_desc *criUp = tdb->getCriDescUp();

  // If no CriDescUp, then don't generate token
  if( CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT_EXPLAIN)==DF_ON &&
      criUp && criUp->noTuples() > 2) {

    NABoolean hasTD = false;
    // Skip over constants and temps
    for(UInt16 i = 2; i < criUp->noTuples(); i++) {
      if(criUp->getTupleDescriptor(i)) {
        hasTD = true;
        break;
      }
    }
    if (hasTD) {
      char buf[20];
      strcpy(buf, "tuple_format: "); 
      explainTuple->setDescription(buf);

      // Skip over constants and temps
      for(UInt16 i = 2; i < criUp->noTuples(); i++) {
        ExpTupleDesc *tupleDesc = criUp->getTupleDescriptor(i);

        // If no tupleDesc for this tuple, use "?"
        const char *tf="?";
        if(tupleDesc) {
          tf = tupleDesc->tupleFormatStr();
        }
        snprintf(buf, 20, "%d:%s ", i, tf); 
        explainTuple->setDescription(buf);
      }
    }
  }

  if (tdb->doOltQueryOpt())
    {
      char oltopt[100];
      strcpy(oltopt, "olt_optimization: used ");
      explainTuple->setDescription(oltopt);
    }

  if ((tdb->doOltQueryOptLean()) &&
      (oltOptLean()))
    {
      char oltopt[100];

      strcpy(oltopt, "olt_opt_lean: used ");
      explainTuple->setDescription(oltopt);
    }

  //add the info on current explain fragment, its parent, its type

  CollIndex cix = generator->getFragmentDir()->getCurrentId();
  FragmentDir::FragmentTypeEnum fragtype = generator->getFragmentDir()->getType(cix);
  
  char buf[120];
  NAString fragdescr;

  NADefaults &defs = ActiveSchemaDB()->getDefaults();
  double mlimit = defs.getAsDouble(BMO_MEMORY_LIMIT_PER_NODE_IN_MB);
  double quotaPerBMO = defs.getAsDouble(EXE_MEM_LIMIT_PER_BMO_IN_MB);

  const char * memory_quota_str = "memory_quota_per_instance: %d MB " ;
  if ( generator->getEspLevel() == 0 ) 
    memory_quota_str = "memory_quota: %d MB " ;

  switch (tdb->getNodeType()) {
    case ComTdb::ex_ROOT:
       {
         if ( quotaPerBMO > 0 || mlimit == 0 ) 
            break;

         double BMOsMemory = 
              generator->getTotalBMOsMemoryPerNode().value() / (1024 * 1024);
         double nBMOsTotalMemory = 
              (generator->getTotalNBMOsMemoryPerNode()).value() / (1024 * 1024);
         snprintf(buf, 120, "est_memory_per_node: %.2f(Limit), %.2f(BMOs), %.2f(nBMOs) MB ", 
                                   mlimit, BMOsMemory, nBMOsTotalMemory); 
         fragdescr += buf;
       } 
       break;

    case ComTdb::ex_HASHJ:
       sprintf(buf, memory_quota_str, 
                     ((ComTdbHashj*)tdb)->memoryQuotaMB());
       fragdescr += buf;
       break;

    case ComTdb::ex_HASH_GRBY:
       {               
         ULng32 mqt = ((ComTdbHashGrby*)tdb)->memoryQuotaMB();
         if (mqt > 0) {
            sprintf(buf, memory_quota_str, mqt);
            fragdescr += buf;
         }
       }
       break;

    case ComTdb::ex_SORT:
       sprintf(buf, memory_quota_str, 
                    (((ComTdbSort*)tdb)->getSortOptions())->memoryQuotaMB());
       fragdescr += buf;
       break;

    case ComTdb::ex_MJ:
       { 
        Lng32 mjqp = defs.getAsLong(MJ_BMO_QUOTA_PERCENT);
        if ( mjqp != 0 ) {
          sprintf(buf, memory_quota_str, 
                         ((ComTdbMj*)tdb)->getQuotaMB());
          fragdescr += buf;
        }
       }
       break;

    case ComTdb::ex_SEQUENCE_FUNCTION:
       {               
         unsigned long mqt = ((ComTdbSequence*)tdb)->memoryQuotaMB();
         if (mqt > 0) {
            sprintf(buf, memory_quota_str, mqt);
            fragdescr += buf;
         }
       }
       break;

    default: 
       break;
  }

  sprintf(buf, "max_card_est: %g fragment_id: %d parent_frag: ", 
          MINOF(getMaxCardEst(), 1e32).value(), cix);
  fragdescr += buf;
  if (fragtype == FragmentDir::MASTER)
    fragdescr += "(none)";
  else 
  {
    CollIndex pix = generator->getFragmentDir()->getParentId(cix);
    sprintf(buf, "%d", pix);
    fragdescr += buf; 
  }
  fragdescr += " fragment_type: ";


  switch (fragtype)
  {
  case FragmentDir::MASTER:
    fragdescr += "master ";
    break;
  case FragmentDir::DP2:
    fragdescr += "dp2 ";
    break;
  case FragmentDir::ESP:
    fragdescr += "esp ";
    break;
  case FragmentDir::EXPLAIN:
    fragdescr += "explain(error) ";
    break;
  default:
    fragdescr += "unknown ";
    break;
  };
  explainTuple->setDescription(fragdescr);

  if (isRowsetIterator())
    {
      char rowsetIter[50];
      strcpy(rowsetIter, "rowset iterator: yes ");
      explainTuple->setDescription(rowsetIter);
    }

  if (getTolerateNonFatalError() == RelExpr::NOT_ATOMIC_)
    {
      char rowsetNAR[50];
      strcpy(rowsetNAR, "non-fatal error tolerated: yes ");
      explainTuple->setDescription(rowsetNAR);
    }

  if ( mlimit > 0 && quotaPerBMO == 0 )
  {
     // Report estimate memory usage per CPU (both BMOs and nBMOs)

     NABoolean reportMemoryEst = TRUE;
     ComTdb::ex_node_type nodeType = tdb->getNodeType();
     switch (nodeType) {
       case ComTdb::ex_HASH_GRBY:
         reportMemoryEst = ((ComTdbHashGrby*)tdb)->memoryQuotaMB() > 0;
         break;
       case ComTdb::ex_MJ:
         reportMemoryEst = FALSE; // MJ does not have an estimate routine to call
         break;
       default:
         break;
     }
     if ( reportMemoryEst == TRUE ) {
        if (nodeType == ComTdb::ex_HASH_GRBY || nodeType == ComTdb::ex_HASHJ
               || nodeType == ComTdb::ex_SORT) {
           double memUsage = tdb->getEstimatedMemoryUsage();
           if ( memUsage > 0 ) {
              sprintf(buf, "est_memory_per_instance: %.3f KB ", memUsage);
              explainTuple->setDescription(buf);
           }
        }
        else {
           double memUsage = getEstimatedRunTimeMemoryUsage(generator, TRUE).value()/1024;
           if ( memUsage > 0 ) {
              sprintf(buf, "est_memory_per_node: %.3f KB ", memUsage);
              explainTuple->setDescription(buf);
           }
        }
    }
  } 

  //calls virtual subclass-specific function
  addSpecificExplainInfo(explainTuple, tdb, generator);

  //finishes up the processing
  addExplainPredicates(explainTuple, generator);

  explainTuple->genExplainTupleData(space);

  return(explainTuple);

}

ExplainTuple *RelExpr::addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
					      ComTdb * tdb,
					      Generator *generator)
{
  return explainTuple;
}


ExplainTuple *
FileScan::addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
					      ComTdb * tdb,
					      Generator *generator)
{

  // accessOptions() belongs to class Scan in optimizer/RelScan.h
  // the possible values of lockMode() and accessType() can be
  // found in common/ComTransInfo.h

  NAString description("scan_type: " + getTypeText());

  // add object type
  if (getIndexDesc()->getNAFileSet()->isInMemoryObjectDefn())
    description += " object_type: inMemory ";
  else if (getTableName().isVolatile())
    description += " object_type: volatile ";
  else if (getTableDesc()->getNATable()->isHiveTable())
    {    
      if (getTableDesc()->getNATable()->getClusteringIndex()->getHHDFSTableStats()->isOrcFile())
        description += " object_type: Hive_Orc ";
      else if (getTableDesc()->getNATable()->getClusteringIndex()->getHHDFSTableStats()->isTextFile())
        description += " object_type: Hive_Text ";
      else if (getTableDesc()->getNATable()->getClusteringIndex()->getHHDFSTableStats()->isSequenceFile())
        description += " object_type: Hive_Sequence ";
    }
  // find direction

  description += " scan_direction: ";
  
  if (reverseScan_)
    {
      description += "reverse ";
    }
  else
    {
      description += "forward ";
    }

  // MDAM information
  if (getMdamKeyPtr())
  {
    description += " mdam: used ";  // key_type: mdam format dropped
  }

  description += " lock_mode: ";
  
  switch(accessOptions().lockMode()) {
  case SHARE_:     description += "shared ";
    break;
  case EXCLUSIVE_: description += "exclusive ";
    break;
  case LOCK_MODE_NOT_SPECIFIED_: description += "not specified, defaulted to lock cursor ";
    break;
  default:         description += "unknown ";
    break;
  };
  
  description += "access_mode: ";
    switch(accessOptions().accessType()) {
    case TransMode::READ_UNCOMMITTED_ACCESS_:     description += "read uncommitted ";
      break;
    case TransMode::SKIP_CONFLICT_ACCESS_: description += "skip conflict ";
      break;
    case TransMode::READ_COMMITTED_ACCESS_:      description += "read committed ";
      break;
    case TransMode::REPEATABLE_READ_ACCESS_: description += "repeatable read ";
      break;
    case TransMode::SERIALIZABLE_: description += "serializable ";
      break;
    case TransMode::ACCESS_TYPE_NOT_SPECIFIED_: description += "not specified, defaulted to read committed ";
      break;
    default:          description += "unknown ";
      break;
    }; 

  // now get columns_retrieved
  description += "columns_retrieved: ";
  char buf[27];
  sprintf(buf, "%d ", retrievedCols().entries());
  description += buf;

  // now get the probe counters
  if ( getProbes().getValue() > 0.0 ) {
    description += "probes: "; // total number of probes
    sprintf(buf, "%g ", getProbes().getValue());
    description += buf;
  }
  
  if ( getSuccessfulProbes().getValue() > 0.0 ) {
    description += "successful_probes: "; // # of probes returning data
    sprintf(buf, "%g ", getSuccessfulProbes().getValue());
    description += buf;
  }
  
  if ( getUniqueProbes().getValue() > 0.0 ) {
     description += "unique_probes: "; // # of probes returning 1 row
     sprintf(buf, "%g ", getUniqueProbes().getValue());
     description += buf;
  }
  
  if ( getDuplicatedSuccProbes().getValue() > 0.0 ) {
     description += "duplicated_succ_probes: "; // # of succ probes returning
     sprintf(buf, "%g ", getDuplicatedSuccProbes().getValue());  // more than 1 row
     description += buf;
  }
  
  if ( getEstRowsAccessed().getValue() ) {
     description += "rows_accessed: "; // #  rows accessed
     sprintf(buf, "%g ", getEstRowsAccessed().getValue());
     description += buf;
  }
  
  if (isRewrittenMV())
    {
      strcpy(buf, "mv_rewrite: used ");
      description += buf;
    }

  const NATable* natable = getIndexDesc()->getPrimaryTableDesc()->getNATable();
  if (natable->isPartitionNameSpecified())
    {
      // this is an index iud
      description += "partition_name: ";
      const NodeMapEntry* nmapentry = natable->getClusteringIndex()->getPartitioningFunction()->getNodeMap()->getNodeMapEntry(0);
      description += nmapentry->getGivenName();
      description += " ";
    }

  if (skipRowsToPreventHalloween_)
    {
      description += "self_referencing_update: dp2_locks ";
    }
  if (isExtraHub())
    {
      description += "project_only_cols: yes ";
    }
  if (minMaxHJColumns().entries() > 0)
    {
      ExprNode *minMaxCols = 
        minMaxHJColumns().rebuildExprTree(ITM_ITEM_LIST);
      
      description += "min_max_hashj_cols: ";
      minMaxCols->unparse(description, OPTIMIZER_PHASE, EXPLAIN_FORMAT);
      description += " ";
    }

  explainTuple->setDescription(description);

//  explainTuple->setTableName(getTableName());

  NAString corrName  = getTableName().getCorrNameAsString();
  NAString tableName = GenGetQualifiedName(getTableName(), TRUE);

  if (natable->getExtendedQualName().getSpecialType() == ExtendedQualName::TRIGTEMP_TABLE)
    tableName   += " [TT]";
  else if (natable->getExtendedQualName().getSpecialType() == ExtendedQualName::IUD_LOG_TABLE)
    tableName   += " [IL]";
  else if (natable->getExtendedQualName().getSpecialType() == ExtendedQualName::RANGE_LOG_TABLE)
    tableName   += " [RL]";

  if (corrName != "")
    explainTuple->setTableName(corrName + " (" + tableName+ ") ");
  else
    explainTuple->setTableName(tableName);

  return(explainTuple);
}

static void appendListOfColumns(Queue* listOfColNames,ComTdb *tdb, NAString& outNAString){

    if (((ComTdbHbaseAccess*)tdb)->sqHbaseTable()){// if trafodion table
      char buf[1000];

      listOfColNames->position();
      for (Lng32 j = 0; j < listOfColNames->numEntries(); j++)
        {
          char * currPtr = (char*)listOfColNames->getCurr();

          Lng32 currPos = 0;
          Lng32 jj = 0;
          short colNameLen = *(short*)currPtr;
          currPos += sizeof(short);
          char colFam[100];
          while (currPtr[currPos] != ':')
          {
            currPos++;
            jj++;
          }
          jj++;
          currPos++;
          snprintf(colFam,sizeof(colFam),"%.*s",jj,currPtr+sizeof(short));
          colNameLen -= jj;

          NABoolean withAt = FALSE;
          char * colName = &currPtr[currPos];
          if (colName[0] == '@')
        {
          colNameLen--;
          colName++;
          withAt = TRUE;
        }

          Int64 v;
          if (colNameLen == sizeof(char))
        v = *(char*)colName;
          else if (colNameLen == sizeof(unsigned short))
        v = *(UInt16*)colName;
          else if (colNameLen == sizeof(Lng32))
        v = *(ULng32*)colName;
          else
        v = 0;
          if (j==0)
              str_sprintf(buf, "%s%s%ld",
                  colFam,
                  (withAt ? "@" : ""),
                  v);
          else
              str_sprintf(buf, ",%s%s%ld",
                  colFam,
                  (withAt ? "@" : ""),
                  v);

          outNAString += buf;

          listOfColNames->advance();
        } // for
    }// trafodion tables
    else
    {// if hbase native tables
      char buf[1000];

      listOfColNames->position();
      for (Lng32 j = 0; j < listOfColNames->numEntries(); j++)
        {
          char * currPtr = (char*)listOfColNames->getCurr();

          char * colNamePtr = NULL;

          Lng32 currPos = 0;
          short colNameLen = *(short*)currPtr;
          currPos += sizeof(short);
          char colName[500];
          snprintf(colName,sizeof(colName),"%.*s",colNameLen,currPtr+sizeof(short));
          colNamePtr = colName;

          if (j==0)
              str_sprintf(buf, "%s",colNamePtr);
          else
              str_sprintf(buf, ",%s",colNamePtr);


          outNAString += buf;

          listOfColNames->advance();
        } // for

    }// hbase native table
    outNAString +=" ";
}

static void appendPushedDownExpression(ComTdb *tdb, NAString& outNAString){
    // in predicate pushdown V2, the hbaseCompareOps list contains a reverse polish set of operation, were operators are
    // AND or OR, the rest are operands. this function display the column, operator and replace any constant with ?. it keeps reverse polish format
    // this can be improved in the future for better readability.
    char buf[1000];
    Queue* reversePolishItems = ((ComTdbHbaseAccess *)tdb)->listOfHbaseCompareOps();
    Queue* pushedDownColumns = ((ComTdbHbaseAccess *)tdb)->listOfHbaseFilterColNames();
    reversePolishItems->position();
    pushedDownColumns->position();

    for (Lng32 j = 0; j < reversePolishItems->numEntries(); j++){
        char * currPtr = (char*)reversePolishItems->getCurr();
        char buf2[1000];
        if (strcmp(currPtr,"V2")!=0 && strcmp(currPtr,"AND")!=0 && strcmp(currPtr,"OR")!=0){//if an operand (not an operator or V2 marker), get the column name
            if (((ComTdbHbaseAccess*)tdb)->sqHbaseTable()){// if trafodion table
                char * currPtr2 = (char*)pushedDownColumns->getCurr();
                  Lng32 currPos = 0;
                  Lng32 jj = 0;
                  short colNameLen = *(short*)currPtr2;
                  currPos += sizeof(short);
                  char colFam[100];
                  while (currPtr2[currPos] != ':')
                  {
                    currPos++;
                    jj++;
                  }
                  jj++;
                  currPos++;
                  snprintf(colFam,sizeof(colFam),"%.*s",jj,currPtr2+sizeof(short));
                  colNameLen -= jj;

                  NABoolean withAt = FALSE;
                  char * colName = &currPtr2[currPos];
                  if (colName[0] == '@')
                {
                  colNameLen--;
                  colName++;
                  withAt = TRUE;
                }
                  Int64 v;
                  if (colNameLen == sizeof(char))
                v = *(char*)colName;
                  else if (colNameLen == sizeof(unsigned short))
                v = *(UInt16*)colName;
                  else if (colNameLen == sizeof(Lng32))
                v = *(ULng32*)colName;
                  else
                v = 0;
                  str_sprintf(buf2, "%s%s%ld",
                      colFam,
                      (withAt ? "@" : ""),
                      v);

            }else{//native hbase table
                 char * currPtr2 = (char*)pushedDownColumns->getCurr();
                  char * colNamePtr1 = NULL;
                  Lng32 currPos = 0;
                  short colNameLen = *(short*)currPtr2;
                  currPos += sizeof(short);
                  char colName[500];
                  snprintf(colName,sizeof(colName),"%.*s",colNameLen,currPtr2+sizeof(short));
                  colNamePtr1 = colName;
                  str_sprintf(buf2, "%s",colNamePtr1);
            }
            pushedDownColumns->advance();
        }


        char* colNamePtr = buf2;
        if(strcmp(currPtr,"EQUAL")==0){
             str_sprintf(buf, "(%s=?)",colNamePtr);
             outNAString += buf;
         }
         else if (strcmp(currPtr,"NOT_EQUAL")==0){
             str_sprintf(buf, "(%s!=?)",colNamePtr);
             outNAString += buf;
         }
         else if (strcmp(currPtr,"LESS")==0){
             str_sprintf(buf, "(%s<?)",colNamePtr);
             outNAString += buf;
         }
         else if(strcmp(currPtr,"LESS_OR_EQUAL")==0){
             str_sprintf(buf, "(%s<=?)",colNamePtr);
             outNAString += buf;
         }
         else if (strcmp(currPtr,"GREATER")==0){
             str_sprintf(buf, "(%s>?)",colNamePtr);
             outNAString += buf;
         }
         else if (strcmp(currPtr,"GREATER_OR_EQUAL")==0){
             str_sprintf(buf, "(%s>=?)",colNamePtr);
             outNAString += buf;
         }
         else if (strcmp(currPtr,"NO_OP")==0){//should never happen
             str_sprintf(buf, "(%s??)",colNamePtr);
             outNAString += buf;
         }
         else if (strcmp(currPtr,"EQUAL_NULL")==0){
             str_sprintf(buf, "(%s=.?)",colNamePtr);
             outNAString += buf;
         }
         else if (strcmp(currPtr,"NOT_EQUAL_NULL")==0){
             str_sprintf(buf, "(%s!=.?)",colNamePtr);
             outNAString += buf;
         }
         else if (strcmp(currPtr,"LESS_NULL")==0){
             str_sprintf(buf, "(%s<.?)",colNamePtr);
             outNAString += buf;
         }
         else if (strcmp(currPtr,"LESS_OR_EQUAL_NULL")==0){
             str_sprintf(buf, "(%s<=.?)",colNamePtr);
             outNAString += buf;
         }
         else if (strcmp(currPtr,"GREATER_NULL")==0){
             str_sprintf(buf, "(%s>.?)",colNamePtr);
             outNAString += buf;
         }
         else if (strcmp(currPtr,"GREATER_OR_EQUAL_NULL")==0){
             str_sprintf(buf, "(%s>=.?)",colNamePtr);
             outNAString += buf;
         }
         else if (strcmp(currPtr,"NO_OP_NULL")==0){
             str_sprintf(buf, "(%s?.?)",colNamePtr);//should never happen
             outNAString += buf;
         }
         else if (strcmp(currPtr,"IS_NULL")==0){
             str_sprintf(buf, "(%s is_null)",colNamePtr);
             outNAString += buf;
         }
         else if (strcmp(currPtr,"IS_NULL_NULL")==0){
             str_sprintf(buf, "(%s is_null.)",colNamePtr);
             outNAString += buf;
         }
         else if (strcmp(currPtr,"IS_NOT_NULL")==0){
             str_sprintf(buf, "(%s is_not_null)",colNamePtr);
             outNAString += buf;
         }
         else if (strcmp(currPtr,"IS_NOT_NULL_NULL")==0){
             str_sprintf(buf, "(%s is_not_null.)",colNamePtr);
             outNAString += buf;
         }
         else if (strcmp(currPtr,"AND")==0)
              outNAString += "AND";
         else if (strcmp(currPtr,"OR")==0)
              outNAString += "OR";


          reversePolishItems->advance();
        }
     outNAString +=' ';
    }



ExplainTuple *
HbaseAccess::addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
				    ComTdb * tdb,
				    Generator *generator)
{
  NAString description("scan_type: " + getTypeText());
  
  if (getTableDesc()->getNATable()->isSeabaseTable())
    {
      if (getTableDesc()->getNATable()->isSeabaseMDTable())
        description += " object_type: Trafodion_MD ";
      else
        description += " object_type: Trafodion ";
    }
  else if (getTableDesc()->getNATable()->isHbaseCellTable())
    description += " object_type: Hbase_Cell ";
  else if (getTableDesc()->getNATable()->isHbaseRowTable())
    description += " object_type: Hbase_Row ";

  // add HbaseSearch info

  NAString keyInfo(" ");
  if ( listOfUniqueRows_.entries() > 0 ) {
     keyInfo.append(listOfUniqueRows_.getText());

  } 

  if ( listOfRangeRows_.entries() > 0 ) {
     keyInfo.append(listOfRangeRows_.getText());
  }

  description += keyInfo;

  description += "cache_size: ";
  char cacheBuf[12];
  snprintf(cacheBuf, 12, "%u", ((ComTdbHbaseAccess *)tdb)->getHbasePerfAttributes()->numCacheRows());
  description += cacheBuf ;
  description += " " ;

  description += "cache_blocks: " ;
  if (!(((ComTdbHbaseAccess *)tdb)->getHbasePerfAttributes()->cacheBlocks()))
    description += "OFF " ;
  else
    description += "ON " ;

  if ((((ComTdbHbaseAccess *)tdb)->getHbasePerfAttributes()->useSmallScanner())) {
    description += "small_scanner: " ;
    description += "ON " ;
  }
  size_t TMP_BUFFER_SIZE=512;
  char buf[TMP_BUFFER_SIZE];

  if ((((ComTdbHbaseAccess *)tdb)->getHbasePerfAttributes()->dopParallelScanner())>0.0) {
     description += "parallel_scanner: " ;
     snprintf(buf, TMP_BUFFER_SIZE, "%g ", ((ComTdbHbaseAccess *)tdb)->getHbasePerfAttributes()->dopParallelScanner());
     description += buf;
  }

  if ( getProbes().getValue() > 0.0 ) {
    description += "probes: "; // total number of probes
    snprintf(buf, TMP_BUFFER_SIZE, "%g ", getProbes().getValue());
    description += buf;
  }

  if ( getSuccessfulProbes().getValue() > 0.0 ) {
    description += "successful_probes: "; // # of probes returning data
    snprintf(buf, TMP_BUFFER_SIZE, "%g ", getSuccessfulProbes().getValue());
    description += buf;
  }

  if ( getUniqueProbes().getValue() > 0.0 ) {
     description += "unique_probes: "; // # of probes returning 1 row
     snprintf(buf, TMP_BUFFER_SIZE, "%g ", getUniqueProbes().getValue());
     description += buf;
  }

  if ( getDuplicatedSuccProbes().getValue() > 0.0 ) {
     description += "duplicated_succ_probes: "; // # of succ probes returning
     snprintf(buf, TMP_BUFFER_SIZE, "%g ", getDuplicatedSuccProbes().getValue());  // more than 1 row
     description += buf;
  }

  if ( getEstRowsAccessed().getValue() ) {
     description += "rows_accessed: "; // #  rows accessed
     snprintf(buf, TMP_BUFFER_SIZE, "%g ", getEstRowsAccessed().getValue());
     description += buf;
  }
  if (((ComTdbHbaseAccess *)tdb)->getHbaseSnapshotScanAttributes()->getUseSnapshotScan())
  {
    description += "use_snapshot_scan: ";
    snprintf(buf, TMP_BUFFER_SIZE, "%s ", "TRUE" );
    description += buf;

    description += "full_table_name: ";
    snprintf(buf, TMP_BUFFER_SIZE, "%s ", ((ComTdbHbaseAccess *)tdb)->getTableName());
    description += buf;

    description += "snapshot_name: ";
    snprintf(buf, TMP_BUFFER_SIZE, "%s ", ((ComTdbHbaseAccess *)tdb)->getHbaseSnapshotScanAttributes()->getSnapshotName());
    description += buf;

    description += "snapshot_temp_location: ";
    snprintf(buf, TMP_BUFFER_SIZE, "%s ", ((ComTdbHbaseAccess *)tdb)->getHbaseSnapshotScanAttributes()->getSnapScanTmpLocation());
    description += buf;

  }

  // get column retrieved
  if (((ComTdbHbaseAccess *)tdb)->listOfFetchedColNames()){
      description += "column_retrieved: ";
      appendListOfColumns(((ComTdbHbaseAccess *)tdb)->listOfFetchedColNames(),tdb,description);
  }
  // get predicate pushed down in Reverse Polish Notation for the AND / OR operators.
  // could transform it standard notation for better readability, but good enough for now...
  // could also evaluate the constants instead of hard coded ?, but good enough for now...
  if (((ComTdbHbaseAccess *)tdb)->listOfHbaseFilterColNames()){
      description += "pushed_down_rpn: ";
     appendPushedDownExpression(tdb, description);
    }
  // get pushed down predicate



  /*
  // now get columns_retrieved
  description += "columns_retrieved: ";
  //sprintf(buf, "%d ", retrievedCols().entries());
  snprintf(buf, TMP_BUFFER_SIZE, "%d ", getIndexDesc()->getIndexColumns().entries());
  description += buf;
  */

  explainTuple->setDescription(description);

  /*
  NAString corrName  = getTableName().getCorrNameAsString();
  NAString tableName = GenGetQualifiedName(getTableName(), TRUE);

  if (corrName != "")
    explainTuple->setTableName(corrName + " (" + tableName+ ") ");
  else
    explainTuple->setTableName(tableName);
  */

  explainTuple->setTableName(getTableName().getQualifiedNameObj().getObjectName());

  return(explainTuple);
}

ExplainTuple *
HbaseDelete::addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
				    ComTdb * tdb,
				    Generator *generator)
{
  NAString description;

  GenericUpdate::addSpecificExplainInfo(explainTuple, tdb, generator, description);

  // add HbaseSearch info

  NAString keyInfo;
  if ( listOfDelUniqueRows_.entries() > 0 ) {
     keyInfo.append(listOfDelUniqueRows_.getText());

  } 

  if ( listOfDelSubsetRows_.entries() > 0 ) {
     keyInfo.append(listOfDelSubsetRows_.getText());
  }

  description += keyInfo;

  explainTuple->setDescription(description);

  //  explainTuple->setTableName(corrName_.getQualifiedNameObj().getObjectName());

  return(explainTuple);
}

ExplainTuple *
HbaseUpdate::addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
				    ComTdb * tdb,
				    Generator *generator)
{
  NAString description;

  GenericUpdate::addSpecificExplainInfo(explainTuple, tdb, generator, description);

  // add HbaseSearch info

  NAString keyInfo;
  if ( listOfUpdUniqueRows_.entries() > 0 ) {
     keyInfo.append(listOfUpdUniqueRows_.getText());

  } else
  if ( listOfUpdSubsetRows_.entries() > 0 ) {
     keyInfo.append(listOfUpdSubsetRows_.getText());
  }

  description += keyInfo;

  explainTuple->setDescription(description);

  //  explainTuple->setTableName(corrName_.getQualifiedNameObj().getObjectName());

  return(explainTuple);
}

ExplainTuple *
DDLExpr::addSpecificExplainInfo(ExplainTupleMaster *explainTuple, 
				ComTdb * tdb, 
				Generator *generator)
{
  char buf[200];
  NAString buffer;

  ExprNode *ddlNode = getDDLNode();
  if (ddlNode)
    {
      if (ddlNode->getOperatorType() == DDL_ON_HIVE_OBJECTS)
        {
          StmtDDLonHiveObjects * hddl =
            ddlNode->castToStmtDDLNode()->castToStmtDDLonHiveObjects();
          buffer = "explain_information: DDL on Hive object ";
          buffer += NAString("ddl_operation: ") + hddl->getOperStr() + " ";
          if (NOT hddl->getName().isNull())
            buffer += NAString("object_name: ") + hddl->getName() + " ";
          else
            buffer += "object_name: unknown ";
          buffer += NAString("object_type: ") + hddl->getTypeStr() + " ";
          if (NOT hddl->getHiveDDL().isNull())
            {
              if (NOT hddl->getHiveDefaultDB().isNull())
                buffer += NAString("hive_default_db: ") + hddl->getHiveDefaultDB() + " ";
              buffer += NAString("hive_ddl: ") + hddl->getHiveDDL() + " ";
            }
          else
            buffer += "hive_ddl: unknown ";
        }
    } // ddlNode

  if (buffer.isNull())
    buffer = "explain_information: not available.";

  explainTuple->setDescription(buffer);
  
  return(explainTuple);
}

ExplainTuple *
ExeUtilHiveTruncate::addSpecificExplainInfo(ExplainTupleMaster *explainTuple, 
                                            ComTdb * tdb, 
                                            Generator *generator)
{
  char buf[200];
  NAString buffer;

  ComTdbExeUtilHiveTruncate *ctdb = (ComTdbExeUtilHiveTruncate*)tdb;
  if (ctdb->getTableName() != NULL)
    buffer += NAString("table_name: ") + ctdb->getTableName() + " ";
  else
    buffer += "table_name: unknown ";
  //  buffer += NAString("object_type: ") + hddl->getTypeStr() + " ";
  if (NOT getHiveTruncQuery().isNull())
    buffer += NAString("hive_trunc_query: ") + getHiveTruncQuery() + " ";
  else
    buffer += "hive_trunc_query: unknown ";

  explainTuple->setDescription(buffer);
  
  return(explainTuple);
}

ExplainTuple*
GroupByAgg::addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
					      ComTdb * tdb,
					      Generator *generator)
{
  
  NAString buffer;
  if (CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT_EXPLAIN)==DF_ON &&
      tdb->getNodeType() == ComTdb::ex_HASH_GRBY)
  {
    buffer += "variable_length_tuples: ";
    if(((ComTdbHashGrby*)tdb)->useVariableLength())
    {
      buffer += "yes ";
    }
    else
    {
      buffer += "no ";
    }

    if (tdb->isCIFON())
    {
       buffer += "CIF: ON ";
    }
    else
    {
      buffer += "CIF: OFF ";
    }
  }

  if (isRollup())
    {
      buffer += "groupby_rollup: specified ";
    }

  explainTuple->setDescription(buffer);

  return(explainTuple);
}

ExplainTuple*
Union::addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
					      ComTdb * tdb,
					      Generator *generator)
{

 // getOperatorType() is located in common/ExprNode.h
 OperatorTypeEnum type = getOperatorType();

 NAString buffer = "union_type: ";

 if (type == REL_MERGE_UNION)
   buffer += "merge ";
 else
 if (type == REL_UNION)
   buffer += "physical ";
 else
   buffer += "unspecified ";

 explainTuple->setDescription(buffer);

 return(explainTuple);
}


ExplainTuple*
Join::addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
					      ComTdb * tdb,
					      Generator *generator)
{

 // getOperatorType() is located in common/ExprNode.h
 OperatorTypeEnum type = getOperatorType();
 Int32 parJoinType;
 Join::ParallelJoinTypeDetail parJoinDetail;

 NAString buffer = "join_type: ";

 if (isNaturalJoin())
   buffer += "natural ";
 else if (isInnerJoin() OR
          isAntiSemiJoin() OR
          (type == REL_TSJ_FLOW) OR
          (type == REL_NESTED_JOIN_FLOW))
   buffer += "inner ";
 else if (isLeftJoin() && !isRightJoin())
   buffer += "left ";
 else if (isRightJoin() && !isLeftJoin())
   buffer += "right ";
 else if (isLeftJoin() && isRightJoin())
   buffer += "full ";
 else if (type == REL_UNION_JOIN || type == REL_MERGE_UNION)
   buffer += "union ";
 else
   buffer += "unknown ";

 if (isSemiJoin())
   buffer += "semi ";
 else
 if (isAntiSemiJoin())
   buffer += "anti-semi ";

 // we need to determine the relationship of the remaining methods with
 // the remaining join types

 buffer += "join_method: ";

 if (isHashJoin())
 {
   if(((ComTdbHashj *)tdb)->isUniqueHashJoin())
     buffer += "unique-hash ";
   else
     buffer += "hash ";

   if (getEquiJoinPredicates().isEmpty())
     buffer += "(cross product) ";

   if (CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT_EXPLAIN)==DF_ON)
   {
     buffer += "variable_length_tuples: ";
     if(((ComTdbHashj *)tdb)->useVariableLength())
     {
       buffer += "yes ";
     }
     else
     {
       buffer += "no ";
     }
     if (tdb->isCIFON())
     {
        buffer += "CIF: ON ";
     }
     else
     {
       buffer += "CIF: OFF ";
     }
   }

 }
 else if (isNestedJoin())
 {
   NestedJoin* njOp = (NestedJoin*)this;
   if (njOp->probesInOrder())
     buffer += "in-order nested ";
   else
     buffer += "nested ";
 }
 else if (isMergeJoin())
   buffer += "merge ";
 else
   buffer += "unknown ";


 parJoinType = getParallelJoinType(&parJoinDetail);

 if (parJoinType > 0)
   {
     char tbuf[20];

     buffer += " parallel_join_type: ";
     sprintf(tbuf,"%d ",parJoinType);
     buffer += tbuf;

     switch (parJoinDetail)
       {
       case Join::PAR_SB:
         buffer += "(SkewBuster) ";
         break;

       case Join::PAR_OCB:
         buffer += "(OCB Outer Child Broadcast) ";
         break;

       case Join::PAR_OCR:
         buffer += "(OCR Outer Child Repartitioning) ";
         break;

       case Join::PAR_N2J:
         buffer += "(N2J Opens all inner partitions) ";
         break;

       default:
         break;
       }
   }

 if (isHashJoin())
 {
     if(((ComTdbHashj *)tdb)->useVariableLength())
       buffer += "variable_length_tuples: yes ";

     // If this Hash Join is configured for min/max optimization, then add
     // token and values for the min max system hostvars used.
     HashJoin *hj = (HashJoin *)this;
     if(hj->getMinMaxVals().entries()) 
       {
         ExprNode *minMaxCols = 
           hj->getMinMaxCols().rebuildExprTree(ITM_ITEM_LIST);
         ExprNode *minMaxExpr = 
           hj->getMinMaxVals().rebuildExprTree(ITM_ITEM_LIST);
  
         buffer += "min_max_cols: ";
         minMaxCols->unparse(buffer, OPTIMIZER_PHASE, EXPLAIN_FORMAT);
         buffer += " ";
         buffer += "min_max_expr: ";
         minMaxExpr->unparse(buffer, OPTIMIZER_PHASE, EXPLAIN_FORMAT);
         buffer += " ";
       } 
 }

 if (beforeJoinPredOnOuterOnly())
   buffer += "other_join_pred_first: yes ";

 explainTuple->setDescription(buffer);

 return(explainTuple);
}

// these CQDs are used for internal purpose and are set/sent by SQL.
// Do not display them.
static NABoolean isInternalCQD(DefaultConstants attr)
{
  if ((attr == SESSION_ID) ||
      (attr == SESSION_IN_USE) ||
      (attr == VOLATILE_CATALOG) ||
      (attr == VOLATILE_SCHEMA_IN_USE))
    return TRUE;
  else
    return FALSE;
}

// explain root displays user specified cqds.
// Some cqds are disabled or enabled in regress/tools/sbdefs during 
// regressions run. That can cause explain plan diffs.
// Specify those cqds in these methods so they are not displayed.
// This method is only called during regressions run.
NABoolean displayDuringRegressRun(DefaultConstants attr)
{
  if ((attr == TRAF_READ_OBJECT_DESC) ||
      (attr == TRAF_STORE_OBJECT_DESC) ||
      (attr == ALLOW_INCOMPATIBLE_ASSIGNMENT) ||
      (attr == ALLOW_INCOMPATIBLE_COMPARISON) ||
      (attr == ALLOW_INCOMPATIBLE_OPERATIONS) ||
      (attr == ALLOW_FIRSTN_IN_SUBQUERIES) ||
      (attr == ALLOW_ORDER_BY_IN_SUBQUERIES) ||
      (attr == GROUP_OR_ORDER_BY_EXPR))
    return FALSE;
  else
    return TRUE;
}

ExplainTuple *
RelRoot::addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
				ComTdb * tdb,
				Generator *generator)
{
  NAString statement;

  static NABoolean sqlmxRegress = (getenv("SQLMX_REGRESS") != NULL);

  // if regressions are running and this explain is for a DDL, then
  // do not return root specific explain info. This is done to avoid
  // differences in root explain information during regression runs 
  // on different systems.
  if ((child(0) && child(0)->castToRelExpr()->getOperatorType() == REL_DDL) &&
      (sqlmxRegress))
    {  
      explainTuple->setDescription(statement);

      return(explainTuple);
    }


  char buf[20];

  // For Adaptive Segmentation
  //
  statement += "affinity_value: ";

  ULng32 affinity = generator->getAffinityValueUsed();

  sprintf(buf, "%u ", affinity);

  statement += buf;

  ComTdbRoot *rootTdb = (ComTdbRoot *)tdb;

  NADefaults &defs = ActiveSchemaDB()->getDefaults();
  ULng32 mlimit = defs.getAsLong(BMO_MEMORY_LIMIT_PER_NODE_IN_MB);

  if (mlimit == 0 && rootTdb->getQueryCostInfo())
  {
    Lng32 memEst = (Lng32) rootTdb->getQueryCostInfo()->totalMem();
    if ( memEst > 0)
    {
      statement += "est_memory_per_cpu: ";
      char estMemVal[11];
      snprintf(estMemVal, 11, "%d KB ",memEst);
      statement += estMemVal;
      statement += " ";
    }
  }

  statement += "max_max_cardinality: ";
  char maxMaxCard[1024];
  sprintf(maxMaxCard, "%.0lf", CURRSTMT_OPTDEFAULTS->maxMaxCardinality());
  statement += maxMaxCard;
  statement += " ";

  FragmentDir *fragDir = generator->getFragmentDir();
  for (CollIndex i = 0; i < fragDir->entries(); i++) {
    if (fragDir->getPartitioningFunction(i) != NULL &&
        fragDir->getType(i) == FragmentDir::ESP)
      {

        NodeMap *nodeMap =
          (NodeMap *)fragDir->getPartitioningFunction(i)->getNodeMap();

        snprintf(buf, 20, "esp_%d_node_map: ", i);
        statement += buf;
        statement += nodeMap->getText();
        statement += " ";
      }
  }

  if (NOT (child(0) && child(0)->castToRelExpr()->getOperatorType() == REL_DDL))
    {
      statement += "statement: ";
      statement += generator->getStmtSource();
      statement += " ";
    }
  else
    {
      statement += "statement: DDL ";
    }

  if (generator->isTransactionNeeded())
    {
      if (generator->foundAnUpdate())
	{
	  statement += "upd_action_on_error: ";

	  if (generator->withNoRollbackUsed())
	  {
	    statement += "no_xn_rollback ";
	  }
	  else if (generator->updAbortOnError() == TRUE)
	    {
	      statement += "xn_rollback ";
	    }
	  else if (generator->updPartialOnError() == TRUE)
	    {
	      statement += "partial_upd ";
	    }
	  else if (generator->updErrorOnError() == FALSE)
	    {
	      if (generator->updSavepointOnError() == TRUE)
		{
		  statement += "savepoint ";
		}
	      else
		statement += "xn_rollback ";
	    }
	  else
	    {
	      statement += "return ";
	    }
	}

      if (generator->dp2XnsEnabled())
      {
	statement += "dp2_xns: enabled";
      }
      
    } // xn needed


  if (NOT generator->needsReadWriteTransaction())
  {
    statement += "xn_access_mode: read_only ";
  }

  Lng32 autoabort_in_seconds = rootTdb->getTransMode()->getAutoAbortIntervalInSeconds();
  if (autoabort_in_seconds == -2)
    autoabort_in_seconds = 0;
  if (autoabort_in_seconds == -3)
    autoabort_in_seconds = -1;

  statement += "xn_autoabort_interval: ";
  char stringAutoAbortInterval[10];
  sprintf(stringAutoAbortInterval, "%d",autoabort_in_seconds);
  statement += stringAutoAbortInterval;
  statement += " ";

  if (rootTdb->aqrEnabled())
    {
      statement += "auto_query_retry: enabled ";      
    }
  else if (rootTdb->aqrEnabledForSqlcode(-8734))
    {
      statement += "auto_query_retry: enabled for privilege checks ";
    }
  else 
    {
      statement += "auto_query_retry: disabled ";
    }

  statement += "plan_version: ";
  char stringPlanVersion[5];
  snprintf(stringPlanVersion, 5, "%d",rootTdb->getPlanVersion());
  statement += stringPlanVersion;
  statement += " ";
  if (rootTdb->isEmbeddedCompiler())
      {
	statement += "embedded_arkcmp: used ";
      } 
  else
    {
      statement += "embedded_arkcmp: not used  ";
    }
  statement += " ";
  switch(generator->getHalloweenProtection())
  {
    case Generator::DP2LOCKS:
      {
        statement += "self_referencing_update: dp2_locks ";
        break;
      }
    case Generator::FORCED_SORT:
      {
        statement += "self_referencing_update: forced_sort ";
        break;
      }
    case Generator::PASSIVE:
      {
        statement += "self_referencing_update: none_required ";
        break;
      }
    case Generator::NOT_SELF_REF:
      {
        break;
      }
    default:
      {
        GenAssert(0, "Invalid value for Generator::HalloweenProtectionType");
      }
   }

  if (rootTdb->getCpuLimit() != 0)
    {
      char str64[32];
      char *int64Str = str64;
      convertInt64ToAscii(rootTdb->getCpuLimit(), int64Str);
      statement += "limit_process_cpu_time: ";      
      statement += str64; 
      statement += " " ;
    }

  ControlDB *cdb = ActiveControlDB();

  //output control query defaults in effect for this statement
  //but skip the internal ones
  for (CollIndex i = 0; i < cdb->getCQDList().entries(); i++)
  {
    ControlQueryDefault * cqd = cdb->getCQDList()[i];
    if ((NOT isInternalCQD(cqd->getAttrEnum())) && (!cqd->reset()) &&
        (sqlmxRegress && displayDuringRegressRun(cqd->getAttrEnum())))
    {
      statement += cqd->getToken().data();
      statement += ": ";
      statement += cqd->getValue().data();
      statement += " ";
    }
  }

  //output the control query table settings now in effect
  for (CollIndex i = 0; i < cdb->getCTList().entries(); i++)
  {
    ControlTableOptions *cto = cdb->getCTList()[i];
      for (CollIndex j = 0; j < cto->numEntries(); j++)
      {
	statement += cto->getToken(j).data();
	statement += ": ";
	statement += cto->getValue(j).data();
	statement += " (for table ";
	statement += cto->tableName().data();
	statement += ") ";
      } // j
  } // i

  double optimizationBudget = CURRSTMT_OPTDEFAULTS->getOptimizationBudget();
  NABoolean explainStrategyParams =
    (CmpCommon::getDefault(EXPLAIN_STRATEGIZER_PARAMETERS) == DF_ON);
  NABoolean isTRoot = isTrueRoot();

  if( explainStrategyParams &&
     (optimizationBudget > 0) &&
     isTRoot)
  {
    char buffer[500];
    
    snprintf(buffer,500,"InitialCPUCost: %.8e InitialScanCost: %.8e InitialCost: %.8e BudgetFactor: %e OrigOptBudget: %e OptBudget: %e Pass1Tasks: %f TaskFactor: %e nComplexity: %f 2nComplexity: %f n2Complexity: %f n3Complexity: %f fullComplexity: %f enumPotential: %d ",
                 ((float)CURRSTMT_OPTDEFAULTS->getCpuCost()),
                 ((float)CURRSTMT_OPTDEFAULTS->getScanCost()),
                 ((float)CURRSTMT_OPTDEFAULTS->getTotalCost()),
                 ((float)CURRSTMT_OPTDEFAULTS->getBudgetFactor()),
                 ((float)CURRSTMT_OPTDEFAULTS->getOriginalOptimizationBudget()),
                 ((float)CURRSTMT_OPTDEFAULTS->getOptimizationBudget()),
                 ((float)CURRSTMT_OPTDEFAULTS->getPass1Tasks()),
                 ((float)CURRSTMT_OPTDEFAULTS->getTaskFactor()),
                 ((float)CURRSTMT_OPTDEFAULTS->getNComplexity()),
                 ((float)CURRSTMT_OPTDEFAULTS->get2NComplexity()),
                 ((float)CURRSTMT_OPTDEFAULTS->getN2Complexity()),
                 ((float)CURRSTMT_OPTDEFAULTS->getN3Complexity()),
                 ((float)CURRSTMT_OPTDEFAULTS->getExhaustiveComplexity()),
                 CURRSTMT_OPTDEFAULTS->getEnumPotentialThreshold());

    statement += buffer;

    /*                 
    statement += "IntialCPUCost: ";
    statement += CURRSTMT_OPTDEFAULTS->getCpuCost();
    statement += " ";
    statement += "IntialScanCost: ";
    statement += CURRSTMT_OPTDEFAULTS->getScanCost();
    statement += " ";
    statement += "InitialCost: ";
    statement += CURRSTMT_OPTDEFAULTS->getTotalCost();
    statement += " ";
    statement += "BudgetFactor: ";
    statement += CURRSTMT_OPTDEFAULTS->getBudgetFactor();
    statement += " ";
    statement += "OptimizationBudget: ";
    statement += CURRSTMT_OPTDEFAULTS->getOptimizationBudget();
    statement += " ";
    statement += "Pass1Tasks: ";
    statement += CURRSTMT_OPTDEFAULTS->getPass1Tasks();
    statement += " ";
    statement += "TaskFactor: ";
    statement += CURRSTMT_OPTDEFAULTS->getTaskFactor();
    statement += " ";
    statement += "nComplexity: ";
    statement += CURRSTMT_OPTDEFAULTS->getNComplexity();
    statement += " ";
    statement += "2nComplexity: ";
    statement += CURRSTMT_OPTDEFAULTS->get2NComplexity();
    statement += " ";
    statement += "n2Complexity: ";
    statement += CURRSTMT_OPTDEFAULTS->getN2Complexity();
    statement += " ";
    statement += "n3Complexity: ";
    statement += CURRSTMT_OPTDEFAULTS->getN3Complexity();
    statement += " ";
    statement += "exhaustiveComplexity: ";
    statement += CURRSTMT_OPTDEFAULTS->getExhaustiveComplexity();
    statement += " ";
    */
  }
  if (CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT_EXPLAIN)==DF_ON)
  {
    if (tdb->isCIFON())
    {
      statement += "CIF: ON ";
    }
    else
    {
      statement += "CIF: OFF ";
    }
  }

  SecurityInvKeyInfo * sik = rootTdb->getSikInfo();
  if (sik)
  {
    const ComSecurityKey * sikValue  = sik->getSikValues();
    statement += "Query_Invalidation_Keys: ";
    Int32 numSiks = sik->getNumSiks();
    for (Int32 i = 0; i < numSiks; i++)
    {
       char buf[64];
       char sikOperationLit[8];
       ComQIActionTypeEnumToLiteral(sikValue[i].getSecurityKeyType(),
                                    sikOperationLit);
       sikOperationLit[2] = '\0';
       str_sprintf(buf, "{%ld,%ld,%s}",
                  (Int64)sikValue[i].getSubjectHashValue(),
                  (Int64)sikValue[i].getObjectHashValue(),
                  sikOperationLit);
       statement += buf;
    }
    statement += " ";
  }

  const Int64 *objectUIDs = rootTdb->getObjectUIDs();
  if (objectUIDs)
  {
    char buf[64];
    str_sprintf(buf, "ObjectUIDs: %ld", objectUIDs[0]);
    statement += buf; 
    Int32 numO = rootTdb->getNumObjectUIDs();
    for (Int32 i = 1; i < numO; i++)
    {
      str_sprintf(buf, ", %ld", objectUIDs[i]);
      statement += buf;
    }
    statement += " ";
  }

  explainTuple->setDescription(statement);
  return(explainTuple);
}

// Here we deal with the cases when we either have the nodes split_top,
// send_top, send_bottom, or when we have an split_top and then a
// partition access.  These node define process boundaries and always
// have a top and bottom partitioning function
ExplainTuple *
Exchange::addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
					      ComTdb * tdb,
					      Generator *generator)
{
  Lng32 topPCount;               // save count of partitions here (parent)
  Lng32 botPCount;               // save count of partitions below (child)
  const PartitioningFunction* topPFuncPtr = getTopPartitioningFunction();
  const PartitioningFunction* botPFuncPtr = getBottomPartitioningFunction();
  NAString description = "";    // output string initialization
  char buf[20];                 // work to format small numbers

  ComTdbSplitBottom *splitBottomTdb = (ComTdbSplitBottom *) tdb;

  if (isEspExchange() && !isExtractConsumer_)
    { // case when we have the nodes split_top, send_top, send_bottom,
      // and split_bottom

      description = "buffer_size: ";
      ComTdbSendBottom *sendBottomTdb = splitBottomTdb->getSendBottomTdb();
      sprintf(buf, "%d ", sendBottomTdb->getReplyBufferSize());
      description += buf;
      description += "record_length: ";
      sprintf(buf, "%d ", sendBottomTdb->getUpRecordLength());
      description += buf;
    }

  // get and format the partition counts
  topPCount = topPFuncPtr->getCountOfPartitions();
  botPCount = botPFuncPtr->getCountOfPartitions();

  description += "parent_processes: ";
  sprintf(buf, "%d ", topPCount);
  description += buf;
  description += "child_processes: ";
  sprintf(buf, "%d ", botPCount);
  description += buf;

  //we do not expect anything but a LogPhysPartitioningFunction here
  //but check anyway and have an alternate action
  if (botPFuncPtr->isALogPhysPartitioningFunction())
  {
    LogPhysPartitioningFunction * lph = (LogPhysPartitioningFunction*)botPFuncPtr;

    description += "parent_partitioning_function: ";
    description += lph->getLogForSplitTop();
    description += " ";

    description += "child_partitioning_function: ";
    description += lph->getPhysForSplitTop();
    description += " ";
  }
  else
  {
    // Get the top and bottom partitioning function attributes.  
    if (topPCount > 1)
      {
	description += "parent_partitioning_function: ";
	description += topPFuncPtr->getText();
	description += " ";
      }
    if (botPCount > 1)
      {
	description += "child_partitioning_function: ";
	description += botPFuncPtr->getText();
	description += " ";
      }
  }

  if (isExtractProducer_)
  {
    description += "extract_producer: yes ";
    description += "extract_security_key: ";
    description += splitBottomTdb->getExtractSecurityKey();
    description += " ";
  }

  if (isExtractConsumer_)
  {
    ComTdbSendTop *sendTopTdb = (ComTdbSendTop *) tdb;

    description += "extract_consumer: yes ";
    description += "extract_esp: ";
    description += sendTopTdb->getExtractEsp();
    description += " ";
    
    description += "extract_security_key: ";
    description += sendTopTdb->getExtractSecurityKey();
    description += " ";
  }

  if (hash2RepartitioningWithSameKey_)
  {
    description += "same_key_hash2_repartition: yes ";
    description += " ";
  }

  if (forcedHalloweenProtection_)
  {
    description += "self_referencing_update: yes ";
    description += " ";
  }

  if (CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT_EXPLAIN)==DF_ON &&
      isEspExchange())
  {
    if (tdb->isCIFON())
    {
      description += "CIF: ON ";
    }
    else
    {
      description += "CIF: OFF ";
    }
   

  }

  // For SeaMonster
  // 
  // Generate two tokens. One to indicate whether the query uses SM
  // anywhere and one for this particular exchange node.
  // 
  // *** NOTES ***
  //
  // Expected files for SQL regressions do not recognize these exact
  // tokens. Instead a filter is added to ignore SeaMonster ON/OFF
  // information in explain output. See file
  // regress/tools/regress-filter-linux.
  //
  // The exchange-specific token is used to generate output for
  // "explain options 'f'". If the token changes, code in
  // ExExeUtilDisplayExplainTcb::FormatForF() needs to be kept in
  // sync. See file executor/ExExeUtilExplain.cpp.

  if (splitBottomTdb->getQueryUsesSM())
    description += "seamonster_query: yes ";

  if (splitBottomTdb->getExchangeUsesSM())
    description += "seamonster_exchange: yes ";

  explainTuple->setDescription(description);  // save what we have built

  return(explainTuple);
}

ExplainTuple *
Sort::addSpecificExplainInfo(ExplainTupleMaster *explainTuple, 
					      ComTdb * tdb, 
					      Generator *)
{
  ComTdbSort *sortTdb = (ComTdbSort *) tdb;
  NAString description = "sort_type:";

  if (sortTdb->partialSort())
  {
    description += " partial ";
  }
  else
  {
    description += " full ";
  }

  if (forcedHalloweenProtection_)
  {
    description += "self_referencing_update: forced_sort ";
  }

  if (sortTdb->getSortOptions()->resizeCifRecord())
  {
    description += "variable_length_tuples: yes ";
  }
  if (CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT_EXPLAIN)==DF_ON)
  {
    description += "variable_length_tuples: ";
    if(sortTdb->getSortOptions()->resizeCifRecord())
    {
      description += "yes ";
    }
    else
    {
      description += "no ";
    }
    if (tdb->isCIFON())
    {
      description += "CIF: ON ";
    }
    else
    {
      description += "CIF: OFF ";
    }
  }
  if (sortTdb->topNSortEnabled() && sortNRows())
    description += "topn_enabled: yes ";

  explainTuple->setDescription(description);  // save what we have built

  return(explainTuple);
}

short
GenericUpdate::addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
				      ComTdb * tdb,
				      Generator *generator,
				      NAString &description)
{

  const NATable* natable = getIndexDesc()->getPrimaryTableDesc()->getNATable();
  NABoolean indexIUD = FALSE;
  description += "iud_type: ";
  if (natable->getExtendedQualName().getSpecialType() == ExtendedQualName::INDEX_TABLE)
    {
      // this is an index iud
      description += "index_";
      indexIUD = TRUE;
    }

  NAString corrName  = getTableName().getCorrNameAsString();

  NAString tableName;
  if ((indexIUD) ||
      (getOptStoi() && getOptStoi()->getStoi()->isView()))
    // for index access, get the actual filename.
    // for view access, get the name of the base table.
    tableName = getIndexDesc()->getNAFileSet()->getExtFileSetName();
  else
    tableName = GenGetQualifiedName(getTableName(), TRUE);

  if (natable->getExtendedQualName().getSpecialType() == ExtendedQualName::TRIGTEMP_TABLE)
    tableName   += " [TT]";
  else if (natable->getExtendedQualName().getSpecialType() == ExtendedQualName::IUD_LOG_TABLE)
    tableName   += " [IL]";
  else if (natable->getExtendedQualName().getSpecialType() == ExtendedQualName::RANGE_LOG_TABLE)
    tableName   += " [RL]";

  if (corrName != "")
    explainTuple->setTableName(corrName + " (" + tableName+ ")");
  else
    explainTuple->setTableName(tableName);

  /*
     iud_type: index_unique_delete TI(T)
     iud_type: subset_update T
  */

  NAString operText(getText());
  size_t blankIndex = operText.index(" ");

  // take the first word from getText()
  if (blankIndex != NA_NPOS)
    operText.remove(blankIndex); // cuts off chars including space

  description += operText + " ";

  description += tableName;
 
  description += " ";

  // add object type
  if (getIndexDesc()->getNAFileSet()->isInMemoryObjectDefn())
    description += " object_type: inMemory ";
  else if (getTableName().isVolatile())
    description += " object_type: volatile ";

  if(getUpdateCKorUniqueIndexKey())
    description += " (implements update of clustering key or unique index key) " ;
  if (natable->isPartitionNameSpecified())
    {
      // this is an index iud
      description += "partition_name: ";
      const NodeMapEntry* nmapentry = natable->getClusteringIndex()->getPartitioningFunction()->getNodeMap()->getNodeMapEntry(0);

      description += nmapentry->getGivenName();

      description += " ";
    }

  if (natable->isSeabaseTable() && 
      (((ComTdbHbaseAccess *)tdb)->getTrafLoadFlushSize() > 0)) {
    char lbuf[20];
    description += "load_flush_size: " ;
    sprintf(lbuf, "%d ", ((ComTdbHbaseAccess *)tdb)->getTrafLoadFlushSize());
    description += lbuf;
  }

  if (natable->isSeabaseTable())
    {
      if (((ComTdbHbaseAccess *)tdb)->useRegionXn())
        description += "region_transaction: enabled ";
      else if (((ComTdbHbaseAccess *)tdb)->useHbaseXn())
        description += "hbase_transaction: used ";
    }

  return 0;
}

ExplainTuple *
GenericUpdate::addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
				      ComTdb * tdb,
				      Generator *generator)
{
  NAString description;

  addSpecificExplainInfo(explainTuple, tdb, generator, description);

  explainTuple->setDescription(description);
  return(explainTuple);
}

ExplainTuple *
ProbeCache::addSpecificExplainInfo( ExplainTupleMaster *explainTuple, 
                                    ComTdb *, 
                                    Generator *generator)
{
  char buf[256];
  
  NAString unParsed(generator->wHeap());
  
  ValueIdSet probeSet = child(0)->getGroupAttr()->getCharacteristicInputs();
  probeSet.addElement(generator->getOrAddStatementExecutionCount());
  ExprNode *probeExpr = probeSet.rebuildExprTree(ITM_ITEM_LIST);
  
  unParsed = "probe_columns: ";
  probeExpr->unparse(unParsed, OPTIMIZER_PHASE, EXPLAIN_FORMAT);
  unParsed += " ";
  explainTuple->setDescription(unParsed);

  sprintf(buf, "num_cache_entries: %d ", numCachedProbes_);
  explainTuple->setDescription(buf);

  sprintf(buf, "num_inner_tuples: %d ", numInnerTuples_);
  explainTuple->setDescription(buf);

  return(explainTuple);
}

ExplainTuple * ExeUtilWnrInsert::addSpecificExplainInfo( 
                ExplainTupleMaster *explainTuple, 
                ComTdb *tdb, 
                Generator *generator)
{
  char buf[1024];
  ComTdbExeUtilAqrWnrInsert * myTdb = 
    (ComTdbExeUtilAqrWnrInsert *)  tdb;
  
  sprintf(buf, "lock_target_table: %s ", 
          myTdb->doLockTarget() ? "ON" : "OFF");
  explainTuple->setDescription(buf);

  sprintf(buf, "target_table_name: %s ", myTdb->getTableName());
  explainTuple->setDescription(buf);

  return(explainTuple);
}

ExplainTuple * ExeUtilCreateTableAs::addSpecificExplainInfo( 
     ExplainTupleMaster *explainTuple, 
     ComTdb *tdb, 
     Generator *generator)
{

  Lng32 maxBufLen = 2000;
  maxBufLen = MAXOF(maxBufLen, ctQuery_.length());
  maxBufLen = MAXOF(maxBufLen, siQuery_.length());
  maxBufLen = MAXOF(maxBufLen, viQuery_.length());
  maxBufLen = MAXOF(maxBufLen, usQuery_.length());

  maxBufLen = MINOF(maxBufLen, 4000);
  maxBufLen++;

  char buf[maxBufLen];
  snprintf(buf, maxBufLen, "CreateQuery: %s ", 
           (ctQuery_.length() > 0 ? ctQuery_.data() : "NULL"));
  explainTuple->setDescription(buf);

  snprintf(buf, maxBufLen, "InsertQuery: %s ", 
           (viQuery_.length() > 0 ? viQuery_.data() : "NULL"));
  explainTuple->setDescription(buf);
          
  snprintf(buf, maxBufLen, "UpsertLoadQuery: %s ", 
           (siQuery_.length() > 0 ? siQuery_.data() : "NULL"));
  explainTuple->setDescription(buf);

  snprintf(buf, maxBufLen, "UpdStatsQuery: %s ", 
           (usQuery_.length() > 0 ? usQuery_.data() : "NULL"));
  explainTuple->setDescription(buf);

  return(explainTuple);
}

const char * ExplainFunc::getVirtualTableName()
//{ return "EXPLAIN__"; }
{return getVirtualTableNameStr();}

TrafDesc * ExplainFunc::createVirtualTableDesc()
{
  // Create a descriptor for this 'virtual' table called EXPLAIN__.
  // The table has 14 columns:
  //   SYSKEY                  INT not null,
  //   module_name             CHAR(60),
  //   statement_name          CHAR(60),
  //   plan_id                 LARGEINT,
  //   seq_num                 INT,
  //   operator                CHAR(30),
  //   left_child_seq_num      INT,
  //   right_child_seq_num     INT,
  //   tname                   CHAR(60),
  //   cardinality             REAL,
  //   operator_cost           REAL,
  //   total_cost              REAL,
  //   detail_cost             VARCHAR(200),
  //   description             VARCHAR(3000)
  //
  //

  // TrafAllocateDDLdesc() requires that HEAP (STMTHEAP) be used for new

  TrafDesc *tableDesc = TrafAllocateDDLdesc(DESC_TABLE_TYPE, NULL);
  const char  *tableName = getVirtualTableName();
  tableDesc->tableDesc()->tablename = new HEAP char[strlen(tableName)+1];
  strcpy(tableDesc->tableDesc()->tablename, tableName);

  TrafDesc *filesDesc = TrafAllocateDDLdesc(DESC_FILES_TYPE, NULL);
  tableDesc->tableDesc()->files_desc = filesDesc;

  TrafDesc *columnDesc;
  UInt32 offset = 0;
  Int32 colnumber = ComTdbExplain::getVirtTableNumCols();
  ComTdbVirtTableColumnInfo * vtci = ComTdbExplain::getVirtTableColumnInfo();

  Lng32 descLen = (Lng32) CmpCommon::getDefaultNumeric(EXPLAIN_DESCRIPTION_COLUMN_SIZE);
  if (descLen > 0) // explicitly specified, use it
    vtci[ EXPLAIN_DESCRIPTION_INDEX].length = descLen;
  else if (CmpCommon::getDefault(EXPLAIN_SPACE_OPT) == DF_OFF)
    {
      // no space opt, use default length of 3000 for backward compatibility
      vtci[EXPLAIN_DESCRIPTION_INDEX].length = 3000; 
    }
  else
    {
      // length will be what was specified in explain descriptor
      // in comexe/ComTdbExplain.h
    }
  columnDesc = Generator::createColDescs(getVirtualTableName(),
					 vtci, //ComTdbExplain::getVirtTableColumnInfo(),
   					 colnumber,
					 offset, NULL);
  columnDesc->columnsDesc()->colclass = 'S';
  //  CMPASSERT(colnumber == 14 && offset == 3492);  // Sanity check
  tableDesc->tableDesc()->columns_desc = columnDesc;
  tableDesc->tableDesc()->colcount = colnumber;
  tableDesc->tableDesc()->record_length = (Lng32) offset;

  ComUID comUID;
  comUID.make_UID();
  Int64 objUID = comUID.get_value();
  tableDesc->tableDesc()->objectUID = objUID;

  TrafDesc *indexDesc = TrafAllocateDDLdesc(DESC_INDEXES_TYPE, NULL);
  tableDesc->tableDesc()->indexes_desc = indexDesc;
  indexDesc->indexesDesc()->tablename = tableDesc->tableDesc()->tablename;
  indexDesc->indexesDesc()->indexname = tableDesc->tableDesc()->tablename;
  TrafDesc *indFilesDesc = TrafAllocateDDLdesc(DESC_FILES_TYPE, NULL);
  indexDesc->indexesDesc()->files_desc = indFilesDesc;

  indexDesc->indexesDesc()->keytag = 0;
  indexDesc->indexesDesc()->record_length = 4;
  indexDesc->indexesDesc()->colcount = 1;
  indexDesc->indexesDesc()->setUnique(TRUE);
  indexDesc->indexesDesc()->blocksize = 4096; // doesn't matter.

  TrafDesc *keyDesc = TrafAllocateDDLdesc(DESC_KEYS_TYPE, NULL);
  indexDesc->indexesDesc()->keys_desc = keyDesc;
  keyDesc->keysDesc()->keyseqnumber = 0;
  keyDesc->keysDesc()->tablecolnumber = 0;
  keyDesc->keysDesc()->setDescending(FALSE);

  return tableDesc;
}

ExplainTuple*
ExeUtilLongRunning::addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
					      ComTdb * tdb,
					      Generator *generator)
{

  char buf[256];
  NAString buffer = "multi_commit: ";
  sprintf(buf, "%s", "ON");
  buffer += buf;
  
  ULng32 multiCommitSize = ((ComTdbExeUtilLongRunning*)tdb)->getMultiCommitSize();
  sprintf(buf, " multi_commit_size: %d ", multiCommitSize);

  buffer += buf;

  if(predicate_) {
    buffer += "predicate: ";
    buffer += predicate_;
    buffer += " ";
  }

  explainTuple->setDescription(buffer);

  return(explainTuple);
}

ExplainTuple *
ControlRunningQuery::addSpecificExplainInfo( ExplainTupleMaster *explainTuple, 
                                    ComTdb *tdb, 
                                    Generator *generator)
{
  NAString buffer = "control_action: ";

  ComTdbCancel *crqTdb = (ComTdbCancel *) tdb;
  bool addQidToExplain = true;

  if (crqTdb->getAction() == ComTdbCancel::CancelByQid)
    buffer += "cancel by qid ";
  else if (crqTdb->getAction() == ComTdbCancel::CancelByPname)
  {
    buffer += "cancel by pname ";
    buffer += "process_name: ";
    buffer += crqTdb->getCancelPname();
    buffer += " ";
    addQidToExplain = false;
  }
  else if (crqTdb->getAction() == ComTdbCancel::CancelByNidPid)
  {
    buffer += "cancel by nid,pid ";
    char npBuf[80];
    sprintf(npBuf, "nid_pid: %d,%d ", 
            crqTdb->getCancelNid(), crqTdb->getCancelPid());
    buffer += npBuf;
    addQidToExplain = false;
  }
  else if (crqTdb->getAction() == ComTdbCancel::Activate)
    buffer += "activate ";
  else
  {
    GenAssert(crqTdb->getAction() == ComTdbCancel::Suspend,
              "Missing explain logic for ControlRunningQuery action_.");
    if (crqTdb->getForce() == ComTdbCancel::Force)
      buffer += "suspend (forced) ";
    else
      buffer += "suspend (safe) ";
  }

  if (addQidToExplain)
  {
    buffer += "qid: ";
    buffer += crqTdb->getQidText();
    buffer += " ";
  }

  if (comment_ != "" )
  {
    buffer += "comment: ";
    buffer += comment_;
    buffer += " ";
  }

  if ((crqTdb->getAction() == ComTdbCancel::CancelByPname) ||
      (crqTdb->getAction() == ComTdbCancel::CancelByNidPid))
  {
    char numBuf[32];
    sprintf(numBuf, "%d seconds", crqTdb->getCancelPidBlockThreshold());
    buffer += "min_blocking_interval: ";
    buffer += numBuf;
    buffer += " ";
  }
    
 
  explainTuple->setDescription(buffer);

  return(explainTuple);
}


ExplainTuple *ExeUtilHBaseBulkUnLoad::addSpecificExplainInfo(ExplainTupleMaster *explainTuple, ComTdb *tdb,
    Generator *generator)
{
  NAString description = "extract_location: ";
  description += extractLocation_;

  description += " empty_target: ";
  description += emptyTarget_ ? "true" : "false";

  if (oneFile_)
  {
    if (mergePath_.length() > 0)
    {
       description += " merge_path: ";
       description += mergePath_;
    }
    description += " overwrite_merge_file: ";
    description +=  overwriteMergeFile_ ? "true" : "false";
  }
  description += " compression: ";
  if (compressType_ ==1 )
    description += "GZIP";
  else
    description += "NONE";


  description += " extract_query: ";
  description += getStmtText();

  explainTuple->setDescription(description);

  return explainTuple;
}

ExplainTuple *ExeUtilHbaseCoProcAggr::addSpecificExplainInfo(
     ExplainTupleMaster *explainTuple, ComTdb *tdb, Generator *generator)
{
  NAString description;
  char buf[64];
  description += "rows_accessed: "; // #  rows accessed
  sprintf(buf, "%g ", getEstRowsAccessed().getValue());
  description += buf;
  
  if (!(((ComTdbHbaseCoProcAggr *)tdb)->
        getHbasePerfAttributes()->cacheBlocks())) {
    description += "cache_blocks: " ;
    description += "OFF " ;
  }

  NAString aggr("aggr on table ");
  aggr += NAString(((ComTdbHbaseCoProcAggr *)tdb)->getTableName());
  description += "coproc_type: ";
  description += aggr;

  explainTuple->setDescription(description);

  return explainTuple;
}


