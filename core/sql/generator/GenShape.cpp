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
 *****************************************************************************
 *
 * File:         GenShape.cpp
 * Description:  Generates a CONTROL QUERY SHAPE statement for a given query.
 *
 *
 * Created:      7/3/1998
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Sqlcomp.h"
#include "AllItemExpr.h"
#include "AllRelExpr.h"
#include "RelSequence.h"
#include "RelSample.h"
#include "RelPackedRows.h"
#include "Generator.h"
#include "mdamkey.h"
#include "GroupAttr.h"

void outputBuffer(Space * space, char * buf, char * newbuf, NAString * shapeStr = NULL)
{
  if(shapeStr)
  {
    (*shapeStr) += newbuf;
    return;
  }

  if ((strlen(buf) + strlen(newbuf)) > 75)
    {
      space->allocateAndCopyToAlignedSpace(buf, strlen(buf), sizeof(short));
      strcpy(buf, newbuf);
    }
  else
    {
      strcat(buf, newbuf);
    }
}

short RelExpr::generateShape(CollHeap * c, char * buf, NAString * shapeStr)
{
  Space * space = (Space *)c;
  char mybuf[100];

  mybuf[0] = 0;

  NABoolean addParens = TRUE;

  switch (getOperatorType())
    {
    case REL_SORT:
      {
	sprintf(mybuf, "sort(");
      }
    break;

    case REL_ORDERED_GROUPBY:
      {
	sprintf(mybuf, "sort_groupby(");
      }
    break;

    case REL_HASHED_GROUPBY:
      {
	sprintf(mybuf, "hash_groupby(");
      }
    break;
    case REL_SHORTCUT_GROUPBY:
      {
        sprintf(mybuf, "shortcut_groupby(");
      }
    break;

    case REL_MAP_VALUEIDS:
      {
	sprintf(mybuf, "expr(");
      }
    break;

    case REL_COMPOUND_STMT:
      {
	sprintf(mybuf, "compound_stmt(");
      }
    break;

   case REL_TUPLE:
      {
	sprintf(mybuf, "tuple");
	addParens = FALSE;
      }
    break;

   case REL_FAST_EXTRACT:
     {
       sprintf(mybuf, "fast_extract(");
     }
    break;

   case REL_HIVE_INSERT:
     {
       sprintf(mybuf, "hive_insert(");
     }
    break;

   case REL_LEAF_TABLE_MAPPING_UDF:
   case REL_UNARY_TABLE_MAPPING_UDF:
   case REL_BINARY_TABLE_MAPPING_UDF:
   case REL_TABLE_MAPPING_BUILTIN_LOG_READER:
   case REL_TABLE_MAPPING_BUILTIN_SERIES:
   case REL_TABLE_MAPPING_BUILTIN_TIMESERIES:
   case REL_TABLE_MAPPING_BUILTIN_JDBC:
     {
       if (getArity() == 0)
         {
           sprintf(mybuf, "tmudf");
           addParens = FALSE;
         }
       else
         sprintf(mybuf, "tmudf(");
     }
    break;

    case REL_TUPLE_LIST:
    case REL_STORED_PROC:
    case REL_INTERNALSP:
    case REL_UTIL_INTERNALSP:
    case REL_EXPLAIN:
    case REL_DDL:
    case REL_DESCRIBE:
    case REL_LOCK:
    case REL_UNLOCK:
    case REL_CONTROL_QUERY_SHAPE:
    case REL_CONTROL_QUERY_DEFAULT:
    case REL_TRANSACTION:
      {
	sprintf(mybuf, "anything");
	addParens = FALSE;
      }
    break;

    case REL_ROOT:
      {
	addParens = FALSE;
      }
    break;

    default:
      {
	addParens = FALSE;
      }
    break;

    } // switch

  if (mybuf)
    outputBuffer(space, buf, mybuf, shapeStr);

  for (Int32 i = 0; i < getArity(); i++)
    {
      if (i > 0)
	{
	  sprintf(mybuf, ",");
	  outputBuffer(space, buf, mybuf, shapeStr);
	}

      if (child(i))
	{
	  child(i)->generateShape(space, buf, shapeStr);
	}
    }

  if (addParens)
    {
      sprintf(mybuf, ")");
      outputBuffer(space, buf, mybuf, shapeStr);
    }

  return 0;
}

short Exchange::generateShape(CollHeap * c, char * buf, NAString * shapeStr)
{
  Space * space = (Space *)c;
  char mybuf[100];

  // ---------------------------------------------------------------------
  // copy important info from the properties into data members
  // ---------------------------------------------------------------------
  //storePhysPropertiesInNode();

  if (isDP2Exchange())
    {
      if (isAPAPA())
	sprintf(mybuf, "split_top_pa(");
      else
	sprintf(mybuf, "partition_access(");
    }
  else
    sprintf(mybuf, "esp_exchange(");
  outputBuffer(space, buf, mybuf, shapeStr);

  if ((isDP2Exchange()) && (isAPAPA()))
    child(0)->child(0)->generateShape(space, buf, shapeStr);
  else
    child(0)->generateShape(space, buf, shapeStr);

  const PartitioningFunction *bottomPartFunc =
    getBottomPartitioningFunction();

  mybuf[0] = 0;
  if (isDP2Exchange())
    {
      if (bottomPartFunc)
	{
	  const LogPhysPartitioningFunction *lpf =
	    bottomPartFunc->castToLogPhysPartitioningFunction();
	  if (lpf)
	    {
	      switch (lpf->getLogPartType())
		{
		case LogPhysPartitioningFunction::PA_PARTITION_GROUPING:
		  sprintf(mybuf, " ,group");
		  break;

		case LogPhysPartitioningFunction::LOGICAL_SUBPARTITIONING:
		case LogPhysPartitioningFunction::PA_GROUPED_REPARTITIONING:
		  sprintf(mybuf, " ,split");
		  break;

		default:
		  break;
		}

	      outputBuffer(space, buf, mybuf, shapeStr);
	    } // lpf !+ NULL
	} // bottomPartFunc
    } // DP2Exchange

  if ((bottomPartFunc) && (isDP2Exchange()) && (isAPAPA()))
    sprintf(mybuf, " ,%d", getBottomPartitioningFunction()->getCountOfPartitions());

  strcat(mybuf, ")");

  outputBuffer(space, buf, mybuf, shapeStr);

  return 0;
}

short Join::generateShape(CollHeap * c, char * buf, NAString * shapeStr)
{
  Space * space = (Space *)c;
  char mybuf[100];

  switch (getOperatorType())
    {
    case REL_NESTED_JOIN:
    case REL_LEFT_NESTED_JOIN:
    case REL_NESTED_SEMIJOIN:
    case REL_NESTED_ANTI_SEMIJOIN:
    case REL_NESTED_JOIN_FLOW:
      sprintf(mybuf, "nested_join(");
      break;

    case REL_MERGE_JOIN:
    case REL_LEFT_MERGE_JOIN:
    case REL_MERGE_SEMIJOIN:
    case REL_MERGE_ANTI_SEMIJOIN:
      sprintf(mybuf, "merge_join(");
      break;

    case REL_HASH_SEMIJOIN:
    case REL_HASH_ANTI_SEMIJOIN:
      sprintf(mybuf, "hash_join(");
      break;

    case REL_LEFT_HYBRID_HASH_JOIN:
    case REL_HYBRID_HASH_SEMIJOIN:
    case REL_HYBRID_HASH_ANTI_SEMIJOIN:
    case REL_FULL_HYBRID_HASH_JOIN:
      sprintf(mybuf, "hybrid_hash_join(");
      break;

    case REL_LEFT_ORDERED_HASH_JOIN:
    case REL_ORDERED_HASH_JOIN:
    case REL_ORDERED_HASH_SEMIJOIN:
    case REL_ORDERED_HASH_ANTI_SEMIJOIN:
      sprintf(mybuf, "ordered_hash_join(");
      break;

    case REL_HYBRID_HASH_JOIN:
      if (((HashJoin *)this)->isOrderedCrossProduct())
	sprintf(mybuf, "ordered_cross_product(");
      else
	sprintf(mybuf, "hybrid_hash_join(");
      break;

    default:
      sprintf(mybuf, "add_to_Join::generateShape(");
    }

  outputBuffer(space, buf, mybuf, shapeStr);

  child(0)->generateShape(space, buf, shapeStr);

  sprintf(mybuf, ",");
  outputBuffer(space, buf, mybuf, shapeStr);

  child(1)->generateShape(space, buf, shapeStr);

  // is it IndexJoin? if both children are scans and number of base tables is
  // 1, it is index join
  if (getGroupAttr()->getNumBaseTables() == 1)
  {
     NABoolean child0isScan = FALSE;
     NABoolean child1isScan = FALSE;
     RelExpr *lChild = child(0)->castToRelExpr();

     while (lChild->getArity() == 1)
     {
        lChild=lChild->child(0)->castToRelExpr();
     }

     switch( lChild->castToRelExpr()->getOperatorType())
     {
       case REL_SCAN:
       case REL_FILE_SCAN:
       case REL_HBASE_ACCESS:
       case REL_HDFS_SCAN:
	 child0isScan = TRUE;
     }

     RelExpr *rChild = child(1)->castToRelExpr();

     while (rChild->getArity() == 1)
     {
        rChild = rChild->child(0)->castToRelExpr();
     }
     switch (rChild->castToRelExpr()->getOperatorType())
     {
       case REL_SCAN:
       case REL_FILE_SCAN:
       case REL_HBASE_ACCESS:
       case REL_HDFS_SCAN:
         child1isScan = TRUE;
     }
     if (child0isScan && child1isScan)
     {
       sprintf(mybuf, ",INDEXJOIN)");
     }
     else
     {
       sprintf(mybuf, ")");
     }
  }
  else
  {
    sprintf(mybuf, ")");
  }

  outputBuffer(space, buf, mybuf, shapeStr);

  return 0;
}

short MergeUnion::generateShape(CollHeap * c, char * buf, NAString * shapeStr)
{
  Space * space = (Space *)c;
  char mybuf[100];

  sprintf(mybuf, "union(");
  outputBuffer(space, buf, mybuf, shapeStr);

  child(0)->generateShape(space, buf, shapeStr);

  sprintf(mybuf, ",");
  outputBuffer(space, buf, mybuf, shapeStr);

  child(1)->generateShape(space, buf, shapeStr);

  sprintf(mybuf, ")");
  outputBuffer(space, buf, mybuf, shapeStr);

  return 0;
}

short FileScan::generateShape(CollHeap * c, char * buf, NAString * shapeStr)
{
  Space * space = (Space *)c;
  char mybuf[1000];
  NAString fmtdStr1, fmtdStr2;

  if (getIndexDesc()->getNAFileSet()->getKeytag() == 0)
  {
     if (getTableName().getCorrNameAsString() == "")
     {
        ToQuotedString(fmtdStr1, 
                       getIndexDesc()->getNAFileSet()->
                        getExtFileSetName().data());
        snprintf(mybuf, 1000, "scan(path %s, ", fmtdStr1.data());
     }
     else
     {
        ToQuotedString(fmtdStr1,getTableName().getCorrNameAsString().data());
        ToQuotedString(fmtdStr2,getIndexDesc()->getNAFileSet()->
                                 getExtFileSetName().data());
        snprintf(mybuf, 1000, "scan(TABLE %s, path %s, ",
                  fmtdStr1.data(), fmtdStr2.data());
     }
  }
  else
  {
    if (getTableName().getCorrNameAsString() != "")
    {
      ToQuotedString(fmtdStr1,getTableName().getCorrNameAsString().data());
      ToQuotedString(fmtdStr2,getIndexDesc()->getNAFileSet()->
                               getExtFileSetName().data());
      snprintf(mybuf, 1000, "scan(TABLE %s, path %s, ",
                  fmtdStr1.data(), fmtdStr2.data());
    }
    else
    {
      ToQuotedString(fmtdStr1,getIndexDesc()->getNAFileSet()->
                               getExtFileSetName().data());
      snprintf(mybuf, 1000, "scan(path %s, ", fmtdStr1.data());
    }

  }

  outputBuffer(space, buf, mybuf, shapeStr);

  if (getReverseScan())
    strcpy(mybuf, "reverse");
  else
    strcpy(mybuf, "forward");

  outputBuffer(space, buf, mybuf, shapeStr);

  if (getNumberOfBlocksToReadPerAccess() > -1)
    {
      sprintf(mybuf, ", blocks_per_access %d ",
	      getNumberOfBlocksToReadPerAccess());
      outputBuffer(space, buf, mybuf, shapeStr);
    }

  if (getMdamKeyPtr() == NULL)
    strcpy(mybuf, ", mdam off)");
  else
    {
      // compute MAX stop column
      CollIndex maxStopColumn=0;
      CollIndex j = 0;
      for (j=0; j < getMdamKeyPtr()->getKeyDisjunctEntries(); j++)
        {
          if (getMdamKeyPtr()->getStopColumn(j) > maxStopColumn)
            {
              maxStopColumn = getMdamKeyPtr()->getStopColumn(j);
            }
        }

      if (maxStopColumn == getIndexDesc()->getIndexKey().entries()-1)
        {
          strcpy(mybuf, ", mdam forced, mdam_columns all(");
        }
        else
        {
          strcpy(mybuf, ", mdam forced, mdam_columns (");
        }

      outputBuffer(space, buf, mybuf, shapeStr);

      UInt32 maxCol = MINOF((maxStopColumn+1), getIndexDesc()->getIndexKey().entries());
      for (UInt32 i = 0; i < maxCol; i++)
	{
	  if (i > 0)
	    strcpy(mybuf, ", ");
	  else
	    mybuf[0] = 0;

	  if (getMdamKeyPtr()->isColumnSparse(i))
	    strcat(mybuf, "sparse");
	  else
	    strcat(mybuf, "dense");
	  outputBuffer(space, buf, mybuf, shapeStr);
	}
      strcpy(mybuf, "))");
    }

  outputBuffer(space, buf, mybuf, shapeStr);

  return 0;
}

short GenericUpdate::generateShape(CollHeap * c, char * buf, NAString * shapeStr)
{
  Space * space = (Space *)c;
  char mybuf[100];

  switch (getOperatorType())
    {
    default:
      {
	sprintf (mybuf, "anything");
      }
    break;
    }

  outputBuffer(space, buf, mybuf, shapeStr);

  return 0;
}

short PhysSequence::generateShape(CollHeap * c, char * buf, NAString * shapeStr)
{
  Space * space = (Space *)c;
  char mybuf[100];

  sprintf(mybuf, "sequence(");
  outputBuffer(space, buf, mybuf, shapeStr);

  child(0)->generateShape(space, buf, shapeStr);

  sprintf(mybuf, ")");
  outputBuffer(space, buf, mybuf, shapeStr);

  return 0;
}

short PhysTranspose::generateShape(CollHeap * c, char * buf, NAString * shapeStr)
{
  Space * space = (Space *)c;
  char mybuf[100];

  sprintf(mybuf, "transpose(");
  outputBuffer(space, buf, mybuf, shapeStr);

  child(0)->generateShape(space, buf, shapeStr);

  sprintf(mybuf, ")");
  outputBuffer(space, buf, mybuf, shapeStr);

  return 0;
}

short PhysUnPackRows::generateShape(CollHeap * c, char * buf, NAString * shapeStr)
{
  Space * space = (Space *)c;
  char mybuf[100];

  sprintf(mybuf, "unpack(");
  outputBuffer(space, buf, mybuf, shapeStr);

  child(0)->generateShape(space, buf, shapeStr);

  sprintf(mybuf, ")");
  outputBuffer(space, buf, mybuf, shapeStr);

  return 0;
}

short PhyPack::generateShape(CollHeap * c, char * buf, NAString * shapeStr)
{
  Space * space = (Space *)c;
  char mybuf[100];

  sprintf(mybuf, "pack(");
  outputBuffer(space, buf, mybuf, shapeStr);

  child(0)->generateShape(space, buf, shapeStr);

  sprintf(mybuf, ")");
  outputBuffer(space, buf, mybuf, shapeStr);

  return 0;
}

short PhysSample::generateShape(CollHeap * c, char * buf, NAString * shapeStr)
{
  Space * space = (Space *)c;
  char mybuf[100];

  sprintf(mybuf, "sample(");
  outputBuffer(space, buf, mybuf, shapeStr);

  child(0)->generateShape(space, buf, shapeStr);

  sprintf(mybuf, ")");
  outputBuffer(space, buf, mybuf, shapeStr);

  return 0;
}

short
IsolatedScalarUDF::generateShape(CollHeap * c, char * buf,
                           NAString * shapeStr)
{
  Space *space = (Space *)c;
  char mybuf[100];

  sprintf (mybuf, "isolated_scalar_udf");
  outputBuffer (space, buf, mybuf, shapeStr);

  if (getRoutineDesc() &&
      getRoutineDesc()->getNARoutine() &&
      getRoutineDesc()->getNARoutine()->getRoutineName())
  {
    NAString fmtdStr;
    ToQuotedString(fmtdStr, getRoutineDesc()->getNARoutine()->
                             getRoutineName()->getQualifiedNameObj().
                             getQualifiedNameAsAnsiString().data());
    snprintf (mybuf, 100, "(scalar_udf %s", fmtdStr.data());
    outputBuffer (space, buf, mybuf, shapeStr);
    if (getRoutineDesc()->isUUDFRoutine() &&
        getRoutineDesc()->getActionNARoutine() &&
        getRoutineDesc()->getActionNARoutine()->getActionName())
    {
      ToQuotedString(fmtdStr, getRoutineDesc()->getActionNARoutine()->
                               getActionName()->data());
      snprintf (mybuf, 100, ", udf_action %s", fmtdStr.data());
      outputBuffer (space, buf, mybuf, shapeStr);
    }
    strcpy(mybuf, ")");
    outputBuffer (space, buf, mybuf, shapeStr);
  }
  return 0;
}

short CallSP::generateShape(CollHeap * c, char * buf, NAString * shapeStr)
{
  Space *space = (Space *)c;
  char mybuf[100];

  sprintf (mybuf, "callsp");
  outputBuffer (space, buf, mybuf, shapeStr);

  return 0;

}

