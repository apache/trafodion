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
 * File:         hs_globals.C
 * Description:  For managing globals.
 * Created:      03/25/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#define HS_FILE "hs_globals"

#define   SQLPARSERGLOBALS_FLAGS				  
#include "SqlParserGlobalsCmn.h"

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <memory>
#include "ComDiags.h"
#include "hs_globals.h"
#include "hs_cli.h"
#include "hs_la.h"
#include "hs_auto.h"
#include "hs_parser.h"
#include "hs_faststats.h"
#include "ComCextdecs.h"
#include "NAString.h"
#include "wstr.h"
#include "Collections.h"
#include "NumericType.h"
#include "exp_datetime.h"
#include "DatetimeType.h"
#include "SchemaDB.h"
#include "CompException.h"
#include "SQLTypeDefs.h"
#include "csconvert.h"
#include "exp_clause_derived.h"  // convDoIt
#include "ExSqlComp.h" // for NAExecTrans()
#include "sql_id.h"
#include "parser.h"
#include "ComUser.h"
#include "CmpSeabaseDDL.h"
#include "PrivMgrDefs.h"
#include "PrivMgrComponentPrivileges.h"
#include "PrivMgrCommands.h"
#include "CmpDDLCatErrorCodes.h"
#include "HBaseClient_JNI.h"  // to get HBC_ERROR_ROWCOUNT_EST_EXCEPTION

#include <sys/stat.h>
#include <sys/types.h>
//#include <fcntl.h>

#include <sys/types.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include "NAClusterInfo.h"
#include <errno.h>
#include <fcntl.h>


#include "Globals.h"
#include "ExpHbaseInterface.h"


#define srand48 srand
#define lrand48 rand
#define MathPow(op1, op2, err) pow(op1, op2)
#define MathLog10(op, err) log10(op)

template <class T>
Int64 placeWidePivot(T* sortArr, Int64 lowInx, Int64 highInx, Int64 pivotInx,
                     Int64& pivotWidth);
template <class T>
void quicksort(T *sortArr, Int64 lowInx, Int64 highInx);

bool isInternalSortType(HSColumnStruct &col);

void formatFixedNumeric(Int64 value, Lng32 scale, char* buffer);

void getPreviousUECRatios(HSColGroupStruct *groupList);

THREAD_P float HSGlobalsClass::ISMemPercentage_ = 0;

Lng32 doSort(HSColGroupStruct *group);

THREAD_P NAString* HSGlobalsClass::defaultHiveCatName  = NULL;
THREAD_P NAString* HSGlobalsClass::defaultHbaseCatName = NULL;

Lng32 setBufferValue(MCWrapper& value, const HSColGroupStruct *mgroup, HSDataBuffer &boundary);

template <class T>
void createHistogram(HSColGroupStruct *group, Lng32 numIntervals, Int64 estRowCount, NABoolean usingSample, T* dummyPtr);

static Lng32 create_I(NAString& sampTblName);

static Lng32 drop_I(NAString& sampTblName);

//
// Initialize the GLOBAL instances of ISFixedChar and ISVarChar values.
// See the "as lightweight as possible" comments in hs_globals.h
//
THREAD_P Int32 ISFixedChar::length = 0;
THREAD_P NABoolean ISFixedChar::caseInsensitive = FALSE;
THREAD_P CharInfo::Collation ISFixedChar::colCollation = CharInfo::DefaultCollation;
THREAD_P CharInfo::CharSet ISFixedChar::charset = CharInfo::UnknownCharSet;

THREAD_P Int32 ISVarChar::declaredLength = 0;
THREAD_P NABoolean ISVarChar::caseInsensitive = FALSE;
THREAD_P CharInfo::Collation ISVarChar::colCollation  = CharInfo::DefaultCollation;
THREAD_P CharInfo::CharSet ISVarChar::charset = CharInfo::UnknownCharSet;

// Initialize the static member hash table that stores jit-log threshold values.
// It will be allocated in the HSGlobalsClass ctor for the first execution of
// an Update Stats statement.
THREAD_P JitLogHashType* HSGlobalsClass::jitLogThresholdHash = NULL;

// Global variables for maintaining buffers used by Collated_cmp()
THREAD_P Int32       lengthOfSortBufrs = 0;
THREAD_P char *    sortBuffer1 = NULL;
THREAD_P char *    sortBuffer2 = NULL;

// Initialize the GLOBAL instances for MCWrapper
THREAD_P MCIterator** MCWrapper::cols_ = NULL;
THREAD_P MCIterator** MCWrapper::allCols_ = NULL;
THREAD_P Int32 MCWrapper::numOfCols_ = 0;
THREAD_P Int32 MCWrapper::numOfAllCols_ = 0;
THREAD_P Int32 MCWrapper::nullCount_ = 0;

// Names (used for logging) corresponding to SortState enum values.
// Must match the enum.
const char* SortStateName[] = 
  {
    "UNPROCESSED",
    "PENDING",
    "PROCESSED",
    "DONT_TRY",
    "SKIP",
    "NO_STATS"
  };

extern THREAD_P NAString* ius_where_condition_text;

// This is from exp\exp_conv.cpp. We copied it here rather than try to figure
// out the linkage issues necessary to be able to use the existing one.
static short convFloat64ToAscii(char *target,
                                Lng32 targetLen,
                                double source,
                                // maximum # of fraction digits
                                Lng32 digits,
                                char * varCharLen,
                                Lng32 varCharLenSize,
                                NABoolean leftPad);

// If an NAHeap is given a request for more than 128MB minus a few bookkeeping
// bytes, it will trigger an assert failure, even if failureIsFatal is false.
// NOTE: This and all references to it can be removed once the fix to check this
//       in NAHeap has been released.
#define MAX_NAHEAP_SINGLE_ALLOC ((128 * 1024 * 1024) - 1024)

// The arrSz argument passed to this macro should not have side effects; it is
// evaluated twice in the macro expansion.
#define newObjArr(clsName, arrSz)                      \
    (sizeof(clsName)*(arrSz) > MAX_NAHEAP_SINGLE_ALLOC \
        ? 0                                            \
        : new clsName[(size_t)arrSz])
#define delObjArr(ptr, clsName) {delete [] (clsName*)(ptr);}
#ifdef _TEST_ALLOC_FAILURE
#define newObjArrX(clsName, arrSz, count)               \
    ((sizeof(clsName)*(arrSz) > MAX_NAHEAP_SINGLE_ALLOC \
              || HSColGroupStruct::allocFilter(count))    \
        ? 0                                             \
        : new clsName[(size_t)arrSz])
#endif

// The elemCount argument passed to this macro should not be an expression with
// side effects; it will be evaluated twice in the macro expansion. This macro
// should only be invoked from the HSGlobalsClass scope, because it uses a
// member variable (allocCount) when _TEST_ALLOC_FAILURE is defined. The
// _TEST_ALLOC_FAILURE version simulates memory allocation failure for testing
// purposes if allocCount equals one of a set of values specified via a CQD.
#ifdef _TEST_ALLOC_FAILURE
#define newBuiltinArr(elemType, elemCount)                         \
    ( HSColGroupStruct::allocFilter(allocCount++) ||                 \
            sizeof(elemType)*(elemCount) > MAX_NAHEAP_SINGLE_ALLOC \
        ? 0                                                        \
        : new (STMTHEAP) elemType[elemCount])
#else
#define newBuiltinArr(elemType, elemCount)                  \
    (sizeof(elemType)*(elemCount) > MAX_NAHEAP_SINGLE_ALLOC \
        ? 0                                                 \
        : STMTHEAP->allocateMemory(sizeof(elemType)*(elemCount), FALSE))
     //        : new (STMTHEAP, FALSE) elemType[elemCount])
#endif

void ISFixedChar::fail(const char* opName, Lng32 line)
{
  HSLogMan *LM = HSLogMan::Instance();
  LM->Log("INTERNAL ERROR (ISFixedChar):");
  sprintf(LM->msg, "Undefined operator type %s", opName);
  LM->Log(LM->msg);
  *CmpCommon::diags() << DgSqlCode(-UERR_GENERIC_ERROR)
                      << DgString0("ISFixedChar")
                      << DgString1("N/A")
                      << DgString2(LM->msg);
  throw CmpInternalException("failure in ISFixedChar",
                             __FILE__, line);
}

// Compare this object to rhs, returning negative value if less, 0 if equal,
// and positive value if greater.
Int32 ISFixedChar::compare(const ISFixedChar &rhs)
{
  // Note that case insensitive is not supported with non-binary collation.
  if (CollationInfo::isSystemCollation(colCollation))
      return Collated_cmp(content, rhs.content, length, colCollation,
                          sortBuffer1, sortBuffer2);
  // UCS2 cols not supported in MODE_SPECIAL_1 and do not support case insensitivity.
  if (!caseInsensitive)
    {
      if (charset != CharInfo::UNICODE)
        return memcmp(content, rhs.content, length);
      else  
        return na_wcsnncmp((const wchar_t *)content, length / sizeof(NAWchar), 
                            (const wchar_t *)rhs.content, length / sizeof(NAWchar));
    }
  else
    return hs_strncasecmp(content, rhs.content, length);
}

/*************************************************/
/* METHOD:   setupMCColumnIterator               */
/* PURPOSE:  sets up the iterator for a given    */
/*           MC column for the MCrapper class    */
/* PARAMS:   group: group represeting an MC col  */
/*           iter: all MC columns                */
/*           iter2: MC not all-null cols         */
/*           currentLoc: index in all cols       */
/*           notNullLoc: index in not null cols  */
/*           numRows: num of rows to process     */
/*                                               */
/* RETCODE:  0 for success and -1 for failure    */
/*************************************************/

Lng32 MCWrapper::setupMCColumnIterator (HSColGroupStruct *group, MCIterator** iter, MCIterator** iter2, 
                                        Int32 &currentLoc, Int32 &notNullLoc, Int32 numRows)
{
  Lng32 retcode = 0;
  char errtxt[100]={0};
  HSErrorCatcher errorCatcher(retcode, - UERR_INTERNAL_ERROR, errtxt, TRUE);
 
  HSLogMan *LM = HSLogMan::Instance();

  // declared early to make the compiler happy
  MCFixedCharIterator* MCFcharIter;
  MCVarCharIterator* MCVcharIter;

  // not nullable column if its type is not nullable or if nullable but has 
  // no null values
  NABoolean noNulls = (!group->colSet[0].nullflag || (group->nullCount == 0));

  switch (group->ISdatatype)
    {
      case REC_BIN8_SIGNED:
        iter[currentLoc] = new (STMTHEAP) MCNonCharIterator<char>((char *)group->mcis_data);
        break;

      case REC_BOOLEAN:
      case REC_BIN8_UNSIGNED:
        iter[currentLoc] = new (STMTHEAP) MCNonCharIterator<unsigned char>((unsigned char *)group->mcis_data);
        break;

      case REC_BIN16_SIGNED:
        iter[currentLoc] = new (STMTHEAP) MCNonCharIterator<short>((short *)group->mcis_data);
        break;

      case REC_BIN16_UNSIGNED:
        iter[currentLoc] = new (STMTHEAP) MCNonCharIterator<unsigned short>((unsigned short *)group->mcis_data);
        break;

      case REC_BIN32_SIGNED:
        iter[currentLoc] = new (STMTHEAP) MCNonCharIterator<Int32>((Int32 *)group->mcis_data);
        break;

      case REC_BIN32_UNSIGNED:
        iter[currentLoc] = new (STMTHEAP) MCNonCharIterator<UInt32>((UInt32 *)group->mcis_data);
        break;

      case REC_BIN64_SIGNED:
        iter[currentLoc] = new (STMTHEAP) MCNonCharIterator<Int64>((Int64 *)group->mcis_data);
        break;

      case REC_BIN64_UNSIGNED:
        iter[currentLoc] = new (STMTHEAP) MCNonCharIterator<UInt64>((UInt64 *)group->mcis_data);
        break;

      case REC_IEEE_FLOAT32:
        iter[currentLoc] = new (STMTHEAP) MCNonCharIterator<float>((float *)group->mcis_data);
        break;

      case REC_IEEE_FLOAT64:
        iter[currentLoc] = new (STMTHEAP) MCNonCharIterator<double>((double *)group->mcis_data);
        break;

      case REC_BYTE_F_ASCII: 
      case REC_BYTE_F_DOUBLE:
      case REC_BINARY_STRING:
        iter[currentLoc] = new (STMTHEAP) MCFixedCharIterator((char*)group->strData, group->ISlength);

        MCFcharIter = (MCFixedCharIterator*)(iter[currentLoc]);

        MCFcharIter->caseInsensitive = (group->colSet[0].caseInsensitive == 1);
        MCFcharIter->colCollation = group->colSet[0].colCollation;
        MCFcharIter->charset = group->colSet[0].charset;

        break;

      case REC_BYTE_V_ASCII: 
      case REC_BYTE_V_DOUBLE:
      case REC_VARBINARY_STRING:
        iter[currentLoc] = new (STMTHEAP) MCVarCharIterator((char*)group->strData);

        MCVcharIter = (MCVarCharIterator*)iter[currentLoc];
       
        // set row length
        MCVcharIter->rowLength = group->ISlength + VARCHAR_LEN_FIELD_IN_BYTES + (group->ISlength % 2);

        MCVcharIter->caseInsensitive = (group->colSet[0].caseInsensitive == 1);
        MCVcharIter->colCollation = group->colSet[0].colCollation;
        MCVcharIter->charset = group->colSet[0].charset;

        break;

      default:
        sprintf(errtxt, "MCsetIterator: unknown type %d", group->ISdatatype);
        sprintf(LM->msg, "MC INTERNAL ERROR: %s", errtxt);
        LM->Log(LM->msg);
        retcode = -1;
        HSHandleError(retcode);
        break;
    }

    // set the bit map null indicator
    if (iter[currentLoc] && group->mcis_nullIndBitMap)
      iter[currentLoc]->nullInd = group->mcis_nullIndBitMap;

    iter[currentLoc]->ISdatatype = group->ISdatatype;  

    if (group->nullCount == numRows)
    {
       if (LM->LogNeeded())
       {
          sprintf(LM->msg, "\tMC: in setupMCColumnIterator, skiping column (%s), all values are null", 
                  group->colSet[0].colname->data());
          LM->Log(LM->msg);
       }
    }
    else
    {
      iter2[notNullLoc] = iter[currentLoc];
      iter2[notNullLoc++]->nullInd = iter[currentLoc]->nullInd;

      if ((LM->LogNeeded()) && group->colSet[0].nullflag && (group->nullCount == 0))
      {
         sprintf(LM->msg, "\tMC: column (%s) is nullable but has no nulls", group->colSet[0].colname->data());
         LM->Log(LM->msg);
      }
    }

    return retcode;
}

/***********************************************/
/* METHOD:   setupMCIterators                  */
/* PURPOSE:  setup all MCWrapper iterators for */
/*           all columns used by the MC        */
/* PARAMS:   mgroup - the MC group to process  */
/*           numRows - number of rows to       */
/*                     process                 */
/* RETCODE:  none                              */
/***********************************************/

void MCWrapper::setupMCIterators(HSColGroupStruct *mgroup, Int32 numRows)
{
    HSColumnStruct   *col;
    HSColGroupStruct *sgroup;
    Int32 nonNullCols = 0;

    HSGlobalsClass *hs_globals = GetHSContext();

    Int32 colCount = mgroup->colCount;

    allCols_ = new (STMTHEAP) MCIterator*[colCount];
    cols_ = new (STMTHEAP) MCIterator*[colCount];

    for (Int32 j=0; j<colCount; j++)
    {
       col = &mgroup->colSet[j];
       sgroup = hs_globals->findGroup(col->colnum);
       setupMCColumnIterator (sgroup, allCols_, cols_, j, nonNullCols, numRows);
    }

    MCWrapper::numOfCols_ = nonNullCols;
    MCWrapper::numOfAllCols_ = colCount;
}


/***********************************************/
/* METHOD:   checkAllColsHaveSameNumOfRows     */
/* PURPOSE:  make sure all columns of the MC   */
/*           have read in the same number      */
/*           of columns                        */
/* PARAMS:   mgroup - the MC group to process  */
/*           numRows - number of rows to       */
/*                     process                 */
/* RETCODE:  TRUE/FALSE                        */
/***********************************************/

NABoolean checkAllColsHaveSameNumOfRows(HSColGroupStruct *mgroup, Int32 &numRows)
{
   numRows = -1;

   Int32 prevNumRows = -1;
   NABoolean goodRowCount = TRUE;

   HSGlobalsClass *hs_globals = GetHSContext();

   HSColumnStruct   *col;
   HSColGroupStruct *sgroup;

   Lng32 colCount = mgroup->colCount;
  
   for (Lng32 x=0; x < colCount; x++)
   {
      col = &mgroup->colSet[x];
      sgroup = hs_globals->findGroup(col->colnum);
      numRows = sgroup->mcis_rowsRead;

      if (x==0)
      {
        prevNumRows = numRows;
      }
      else if (numRows != prevNumRows)
      {
         goodRowCount = FALSE;
         break;
      }
   }

   return goodRowCount;
}

/***********************************************/
/* METHOD:   getCommonCols                     */
/* PURPOSE:  Find the common bits/columns      */
/*           between two bitmaps               */
/* PARAMS:   map1 - bitmap1                    */
/*           map2 - bitmap2                    */
/* RETCODE:  number of common bits/columns     */
/***********************************************/

Int32 getCommonCols (const NABitVector &map1, const NABitVector &map2)
{
  NABitVector common_bits(map1);

  common_bits.intersectSet(map2);

  return (Int32) common_bits.entries();
}

/***********************************************/
/* METHOD:   getMissingCols                    */
/* PURPOSE:  Find the missing bits/columns     */
/*           between two bitmaps               */
/* PARAMS:   map1 - bitmap1                    */
/*           map2 - bitmap2                    */
/*           missing_bits - bitmap3 that       */
/*             keeps track of missing bits     */
/* RETCODE:  number of bits/columns in map2    */
/*           that are not in map1 and not      */
/*           already in missing_bits           */
/***********************************************/

Int32 getMissingCols (const NABitVector &map1, const NABitVector &map2, NABitVector *missing_bits)
{
  NABitVector new_missing_bits(map2);

  // bits in map2 that are not in map1
  new_missing_bits.subtractSet(map1);

  CollIndex initial = missing_bits->entries();

  missing_bits->addSet(new_missing_bits);

  return (Int32) (missing_bits->entries() - initial);
}

// If an HBase table is very large, we risk time-outs because the
// sample scan doesn't return rows fast enough. In this case, we
// want to reduce the HBase row cache size to a smaller number to
// force more frequent returns. Experience shows that a value of
// '10' worked well with a 17.7 billion row table with 128 regions
// on six nodes (one million row sample). We'll assume a workable
// HBase cache size value scales linearly with the sampling ratio.
// That is, we'll assume the model:
//
//   workable value = (sample row count / actual row count) * c,
//   where c is chosen so that we get 10 when the sample row count
//   is 1,000,000 and the actual row count is 17.7 billion.
//
//   Solving for c, we get c = 10 * (17.7 billion/1 million).
//
// Note that the Generator does a similar calculation in
// Generator::setHBaseNumCacheRows. The calculation here is more
// conservative because we care more about getting UPDATE STATISTICS
// done without a timeout, trading off possible speed improvements
// by using a smaller cache size.
//
// Another issue is that it's been observed that time-outs also
// depend on system load. Through experimentation we've discovered
// that a value of 50 works well in loaded scenarios, assuming the
// table is not too large. So, we use a maximum of the workable
// value computed above and 50.
//
// Another note: If the user has already set HBASE_NUM_CACHE_ROWS_MAX,
// then we don't do anything here. We respect the user's choices
// instead.
//
// We had hoped that HBase 1.1, with its heartbeat protocol, would
// solve this time-out problem for good. But early testing seems
// to suggest that this is not the case.
//
// Input:
// sampleRatio -- Percentage of rows being sampled.
//
// Return:
// TRUE if the CQDs were altered, FALSE otherwise. The caller should use this
// information to reset the CQDs following execution of the sample query to
// avoid a performance penalty for subsequent queries (notably those that
// read the sample table).
NABoolean HSGlobalsClass::setHBaseCacheSize(double sampleRatio)
{
  double calibrationFactor = 10 * (17700000000/1000000);
  Int64 workableCacheSize = (Int64)(sampleRatio * calibrationFactor);
  if (workableCacheSize < 1)
    workableCacheSize = 1;  // can't go below 1 unfortunately
  else if (workableCacheSize > 50)
    workableCacheSize = 50; 

  // Do this only if the user didn't set the CQD in this session
  // (So, for example, if the CQD was set in the DEFAULTS table
  // but not in this session, we'll still override it.)
  NADefaults &defs = ActiveSchemaDB()->getDefaults();
  if (defs.getProvenance(HBASE_NUM_CACHE_ROWS_MAX) < 
      NADefaults::SET_BY_CQD)
    {
      char temp1[40];  // way more space than needed, but it's safe
      Lng32 wcs = (Lng32)workableCacheSize;
      sprintf(temp1,"'%d'",wcs);
      NAString minCQD = "CONTROL QUERY DEFAULT HBASE_NUM_CACHE_ROWS_MIN ";
      minCQD += temp1;
      HSFuncExecQuery(minCQD);
      NAString maxCQD = "CONTROL QUERY DEFAULT HBASE_NUM_CACHE_ROWS_MAX ";
      maxCQD += temp1;
      HSFuncExecQuery(maxCQD);
      hbaseCacheSizeCQDsSet_ = TRUE;
      return TRUE;
    }
  else
    return FALSE;
}


// If a Hive table has very long character columns, we might get
// a SQL error 8446 when scanning it. The way around that is to
// set CQD HIVE_MAX_STRING_LENGTH_IN_BYTES to the longest string
// length in the table. This is typically done by the user in
// the sqlci or trafci session. However we need to propagate
// it to our child tdm_arkcmp. This method does that.
NABoolean HSGlobalsClass::setHiveMaxStringLengthInBytes(void)
{
  NABoolean rc = FALSE;
  if (isHiveTable)
    {
      NADefaults &defs = ActiveSchemaDB()->getDefaults();
      if (defs.getProvenance(HIVE_MAX_STRING_LENGTH_IN_BYTES) >= 
          NADefaults::SET_BY_CQD)
        {
          char temp1[40];  // way more space than needed, but it's safe
          UInt32 hiveMaxStringLengthInBytes = 
            ActiveSchemaDB()->getDefaults().getAsULong(HIVE_MAX_STRING_LENGTH_IN_BYTES);

          sprintf(temp1,"'%u'",hiveMaxStringLengthInBytes);
          NAString theCQD = "CONTROL QUERY DEFAULT HIVE_MAX_STRING_LENGTH_IN_BYTES ";
          theCQD += temp1;
          HSFuncExecQuery(theCQD);

          hiveMaxStringLengthCQDSet_ = TRUE;
          rc = TRUE;
        }
    }

  return rc;
}


// If we set any CQDs in CollectStatistics that need to be
// reset when we are done, do that here.
void HSGlobalsClass::resetCQDs(void)
{
  if (hbaseCacheSizeCQDsSet_)
    {
      HSFuncExecQuery("CONTROL QUERY DEFAULT HBASE_NUM_CACHE_ROWS_MIN RESET");
      HSFuncExecQuery("CONTROL QUERY DEFAULT HBASE_NUM_CACHE_ROWS_MAX RESET");
      hbaseCacheSizeCQDsSet_ = FALSE;
    }
  if (hiveMaxStringLengthCQDSet_)
    {
      HSFuncExecQuery("CONTROL QUERY DEFAULT HIVE_MAX_STRING_LENGTH_IN_BYTES RESET");
      hiveMaxStringLengthCQDSet_ = FALSE;
    }
}


// rearrange the MCs so that the larger groups are listed first
// and the ones that will not be processed (not enough memory) are
// listed last, so to simplify the rest of the ordering algorithm
void HSGlobalsClass::reArrangeMCGroups()
{
   Int32 multiGroupCount = groupCount - singleGroupCount;
   HSColGroupStruct* s_m_group[multiGroupCount];

   HSColGroupStruct* mgroup = multiGroup;

   for (int i=0; i <multiGroupCount; i++)
   {
      if (i==0)
         s_m_group[i] = mgroup;
      else
      {
         int j=0;
         while((j<i) &&
               ((mgroup->colCount < s_m_group[j]->colCount) || (mgroup->state != UNPROCESSED)))
           j++;

         int k=i;
         while (k>j)
         {
            s_m_group[k] = s_m_group[k-1];
            k--;
         }
         s_m_group[k] = mgroup;
      }
      mgroup = mgroup->next;
   }

   s_m_group[0]->prev = NULL;
   s_m_group[multiGroupCount-1]->next = NULL;
   for (int i=0; i <multiGroupCount-1; i++)
   {
      s_m_group[i]->next = s_m_group[i+1];
      s_m_group[i+1]->prev = s_m_group[i];
   }

   multiGroup = s_m_group[0];
}

/***********************************************/
/* METHOD:  orderMCGroupsNeeded                */
/* PURPOSE: check if we need to re-order       */
/*          multi-column and single-column     */
/*          groups to maximize the number      */
/*          of multi-column stats that can     */
/*          be done in memory                  */
/* PARAMS:  none                               */
/* RETCODE:  1 - Memory is not enough to       */
/*               process all SC and MC         */
/*               together and at least one MC  */
/*               column can be processed in    */
/*               memory                        */
/*          -0 - otherwise                     */
/* ASSUMPTIONS: memory requirements for single */
/*              and multi-column groups have   */
/*              already been determined        */
/***********************************************/

NABoolean HSGlobalsClass::orderMCGroupsNeeded()
{
  // ordering is not necessary if:
  //   multiGroup is empty
  //   all SC and MC fit in memory together
  //   all MC don't fit in memory
  // return FALSE is any of the above is TRUE

  HSLogMan *LM = HSLogMan::Instance();
  Int64 memNeededForAllCols = 0;
  Int64 memAllowed = getMaxMemory();
  HSColGroupStruct* sgroup = singleGroup;
  HSColGroupStruct* mgroup = multiGroup;

  // no MC groups to process
  if (mgroup == NULL)
  {
    if (LM->LogNeeded())
    {
       sprintf(LM->msg, "MC: NO rearranging of sgroup for IS: mgroup is empty");
       LM->Log(LM->msg);
    }

    return FALSE;
  }

  // memory needed by all single-column groups
  while (sgroup)
  {
     memNeededForAllCols += sgroup->memNeeded;
     sgroup = sgroup->next;
  }

  // memory needed by all multi-column groups
  while (mgroup)
  {
     memNeededForAllCols += mgroup->memNeeded;
     mgroup = mgroup->next;
  }

  // all columns and MCs can fit in memory together, no need to rearrange them
  if (memNeededForAllCols < memAllowed)
  {
    if (LM->LogNeeded())
    {
       sprintf(LM->msg, "MC: NO rearranging of sgroup for IS: there is enough memory to compute all MC groups");
       LM->Log(LM->msg);
    }

     return FALSE;
  }

  // we have multi-column groups and memory is not enough to process all single and
  // multi-column groups together, we need to check if any of the multi-column
  // groups can be processed in memory

  sgroup = singleGroup;
  mgroup = multiGroup;

  HSColumnStruct* col;
  NABoolean atLeastOne = FALSE;
  while (mgroup)
  {
     // don't try this multi-column group if its memory 
     // requirement cannot be met
     if ((mgroup->state == UNPROCESSED) &&
         (mgroup->mcis_totalMCmemNeeded >= memAllowed))
     {
        mgroup->state = DONT_TRY;
        mgroup->memNeeded = 0;
        mgroup->mcis_totalMCmemNeeded = 0;
        for (Int32 i=0; i< mgroup->colCount; i++)
        {
           col = &mgroup->colSet[i];
           sgroup = findGroup(col->colnum);
           sgroup->mcs_usingme--;
        }

        if (LM->LogNeeded())
        {
           sprintf(LM->msg, "MC: NO memory available to compute MC (%s) using IS", mgroup->colNames->data());
           LM->Log(LM->msg);
        }
     }
     else if (mgroup->state == UNPROCESSED)
       atLeastOne = TRUE;

     mgroup = mgroup->next;
  }

  // all MCs are too large to fit in memory
  if (!atLeastOne)
  {
    if (LM->LogNeeded())
    {
       sprintf(LM->msg, "MC: NO rearranging of sgroup for IS: "
                        "either all MC groups are already processed or too large for memory");
       LM->Log(LM->msg);
    }

    return FALSE;
  }

  // we at least have one multi-column group that can be processed in memory
  if (LM->LogNeeded())
  {
    sprintf(LM->msg, "\tMC: proceeding with rearranging sgroup for IS: memory available for at"
                     " least one MC but not enough to process all eligible MCs together");
    LM->Log(LM->msg);
  }

  // rearrange the MCs so that the larger groups are listed first
  // and the ones that will not be processed (not enough memory) are
  // listed last, so to simplify the rest of the ordering algorithm
  reArrangeMCGroups();

  return TRUE;
}


// helper function to orderMCGroups
// set the number of columns that are only used by this MC
// this method also initializes mcis_groupHead 
void HSGlobalsClass::computeSingleUsedCols()
{
   HSLogMan *LM = HSLogMan::Instance();
   HSColGroupStruct* mgroup = multiGroup;
   while (mgroup != NULL)
   {
       if (mgroup->state != UNPROCESSED)
       {
          mgroup = mgroup->next;
          continue;
       }

       // initialize all candidate groups as group heads
       mgroup->mcis_groupHead = TRUE;
       mgroup->mcis_groupWeight.w = 0;

       HSColGroupStruct *sgroup;
       HSColumnStruct* col;
       for (Int32 i=0; i<mgroup->colCount; i++)
       {
          col = &mgroup->colSet[i];
          sgroup = findGroup(col->colnum);
          if (sgroup->mcs_usingme == 1)
            mgroup->mcis_groupWeight.w++;
       }

       mgroup = mgroup->next;
   }
}

// helper function to orderMCGroups
// this method computes the wieght (u,v) of an
// MC. See below for definition of the weight
void HSGlobalsClass::computeMCGroupsWeight()
{
   HSColGroupStruct* m_group = multiGroup;
   HSColGroupStruct* remaining_mgroup = NULL;

   // compute the weight of every multi-column group 
   // except for the ones that cannot fit into memory
   // or the ones that are already processed
   // weight of multi-group MCi is a vector (u,v) where: 
   //     u: sum of multi-columns MCj that have common columns 
   //        with MCi
   //     v: sum of |MCi| - |interset(MCi,MCj)| for all MCj
   //        that have common columns with MCi. A column 
   //        is not counted twice
   //     (u1,v1) > (u2,v2) if:
   //     u1 > u2 or
   //     u1==u2 and v1 < v2

   while (m_group && m_group->next)
   {
      Int32 i = 0;
      Int32 commonCols = 0;
      if ((m_group->state == DONT_TRY) || (m_group->state == PROCESSED))
      {
         m_group->mcis_groupWeight.clear();
         m_group = m_group->next;
         continue;
      }

      remaining_mgroup = m_group->next;
      while (remaining_mgroup)
      {
         if ((remaining_mgroup->state != DONT_TRY) && 
             (remaining_mgroup->state != PROCESSED) &&
             (commonCols = getCommonCols (*(m_group->mcis_colsUsedMap), *(remaining_mgroup->mcis_colsUsedMap))) > 0 )
         {
            m_group->mcis_groupWeight.u++;

            remaining_mgroup->mcis_groupWeight.u++;

            m_group->mcis_groupWeight.v += 
                     getMissingCols (*(m_group->mcis_colsUsedMap), 
                                     *(remaining_mgroup->mcis_colsUsedMap), 
                                     m_group->mcis_colsMissingMap);
            remaining_mgroup->mcis_groupWeight.v += 
                     getMissingCols (*(remaining_mgroup->mcis_colsUsedMap), 
                                     *(m_group->mcis_colsUsedMap), 
                                     remaining_mgroup->mcis_colsMissingMap);

            // do this here to avoid another traversal
            // A group HEAD is a group that has common columns
            // (or no common columns at all) with other groups
            // and has the highest weight
            if (remaining_mgroup->mcis_groupWeight <= m_group->mcis_groupWeight)
            {
                remaining_mgroup->mcis_groupHead = FALSE;
            }
           
         }
         remaining_mgroup = remaining_mgroup->next;
      }

      m_group = m_group->next;
   }
}

// helper function to orderMCGroups
// order the MC groups in an assending order
// by their weight
void HSGlobalsClass::reorderMCGroupsByWeight()
{
   Int32 multiGroupCount = groupCount - singleGroupCount;
   HSColGroupStruct* s_m_group[multiGroupCount];

   HSColGroupStruct* mgroup = multiGroup;

   for (int i=0; i <multiGroupCount; i++)
   {
      if (i==0)
         s_m_group[i] = mgroup;
      else
      {
         int j=0;
         while((j<i) && (mgroup->mcis_groupWeight < s_m_group[j]->mcis_groupWeight))
           j++;

         int k=i;
         while (k>j)
         {
            s_m_group[k] = s_m_group[k-1];
            k--;
         }
         s_m_group[k] = mgroup;
      }
      mgroup = mgroup->next;
   }

   s_m_group[0]->prev = NULL;
   s_m_group[multiGroupCount-1]->next = NULL;
   for (int i=0; i <multiGroupCount-1; i++)
   {
      s_m_group[i]->next = s_m_group[i+1];
      s_m_group[i+1]->prev = s_m_group[i];
   }

   multiGroup = s_m_group[0];
}

// helper function to orderMCGroups
// form group sets and connect group heads to their neighbors
// using the mcis_next pointers
void HSGlobalsClass::formGroupSets()
{
   HSLogMan *LM = HSLogMan::Instance();

   // get the list of neighbors to the group with the highest weight
   HSColGroupStruct* mgroup_set = multiGroup;
   HSColGroupStruct* mgroup =  NULL;

   while (mgroup_set)
   {
      if (mgroup_set->state == DONT_TRY)
      {
         if (LM->LogNeeded())
         {
            *LM << "\tMC: GROUP (" << mgroup_set->colNames->data() << ") has state DONT_TRY, is skipped";
            LM->FlushToLog();
         }
      }
      else if ((mgroup_set->mcis_groupHead) && (mgroup_set->state != PROCESSED))
      {
         mgroup_set->mcis_next = mgroup_set;

         if (LM->LogNeeded())
         {
            *LM << "\tMC: GROUP (" << mgroup_set->colNames->data() << ") is a HEAD GROUP ";
            LM->FlushToLog();
         }

         mgroup =  multiGroup;
   
         Int32 i = 0;
         while (mgroup != NULL)
         {
            if((!mgroup->mcis_groupHead) && 
               (mgroup->state == UNPROCESSED) &&
               getCommonCols (*(mgroup_set->mcis_colsUsedMap), *(mgroup->mcis_colsUsedMap)))
            {

               if (mgroup->mcis_next == NULL)
               {
                   HSColGroupStruct *myNextNeighbpr = mgroup_set;
                   if (myNextNeighbpr->mcis_next == NULL)
                   {
                       myNextNeighbpr->mcis_next = mgroup;
                       mgroup->mcis_next = mgroup_set;
                   }
                   else
                   {
                      while ((myNextNeighbpr->mcis_next != mgroup_set) && 
                             (mgroup->mcis_groupWeight.w < myNextNeighbpr->mcis_next->mcis_groupWeight.w))
                          myNextNeighbpr = myNextNeighbpr->mcis_next;
                      
                       mgroup->mcis_next = myNextNeighbpr->mcis_next;
                       myNextNeighbpr->mcis_next = mgroup;
                   }
               }
            }
      
            mgroup = mgroup->next;
         }
      
         if (LM->LogNeeded())
         {
            HSColGroupStruct *myNextNeighbpr = mgroup_set;
         
            while(myNextNeighbpr->mcis_next && (myNextNeighbpr->mcis_next != mgroup_set))
            {
              myNextNeighbpr = myNextNeighbpr->mcis_next;

              *LM << "\tMC: GROUP (" << myNextNeighbpr->colNames->data() << 
                     ") is neighbor of HEAD GROUP (" << mgroup_set->colNames->data() << ")"; 
              LM->FlushToLog();
            }

            if (mgroup_set->mcis_groupWeight.isNull())
            {
              *LM << "\tMC: GROUP (" << mgroup_set->colNames->data() << ") has no neighbors"; 
              LM->FlushToLog();
            }
         }
      }
     
      mgroup_set = mgroup_set->next;
   }
}

// helper function to orderMCGroups
// reorder the single groups used by the multi-groups by listing the columns 
// used by the multi-groups with highest weight first
// this is done so to not alter the IS logic that schedules the processing of
// single column groups
// this method proceeds as follows:
//   1- identify the SC that are used by a head group and its neighbors
//   2- order the SC so that the columns used by the head group are listed first
void HSGlobalsClass::reorderSingleGroupsByWeight (HSColGroupStruct* s_group_back[],
                                                  Int32 colsOrder[], Int32 &headGroupCols)
{
   HSLogMan *LM = HSLogMan::Instance();

   HSColGroupStruct* mgroup_set = multiGroup;
   HSColGroupStruct* sgroup = NULL;
   HSColGroupStruct* s_m_group[singleGroupCount];

   Int32 i = 0;
   NABitVector seenCols;
   while (mgroup_set != NULL)
   {
       if ((mgroup_set->state != PROCESSED) && 
           (mgroup_set->mcis_groupHead) && 
           (mgroup_set->mcis_next != NULL))
       {
          HSColGroupStruct* start = mgroup_set;

          if (start->mcis_groupWeight.w < start->mcis_next->mcis_groupWeight.w)
             start = start->mcis_next;

          HSColGroupStruct* end = start;

          while(start->mcis_next && (start->mcis_next != end))
          {
            NABitVector new_cols(*start->mcis_colsUsedMap);

            new_cols.subtractSet(seenCols);

            for (CollIndex j=0; new_cols.nextUsed(j); j++)
              colsOrder[i++] = j;

            seenCols.addSet(*start->mcis_colsUsedMap);
            start = start->mcis_next;
          }

       }
       mgroup_set = mgroup_set->next;
   }

   headGroupCols = i;
   // fill in the remaining single columns that are not used by any
   // multi-group
   for (CollIndex j=0; j<singleGroupCount; j++)
     if (!seenCols.testBit(j))
        colsOrder[i++] = j;

   if (LM->LogNeeded())
   {
      sprintf(LM->msg, "\tMC: order of single group columns BEFORE reorder");
      LM->Log(LM->msg);
   
      // get current single columns order
      sgroup = singleGroup;
      while (sgroup != NULL)
      {
        sprintf(LM->msg, "\tMC: SINGLE has mcs_usingme (%d) col: (%s)",
                sgroup->mcs_usingme, sgroup->colSet[0].colname->data());
        LM->Log(LM->msg);
        sgroup = sgroup->next;
      }
   }

   // do the actual reordering and rearrange the single groups list
   i = 0;
   while (i < singleGroupCount)
   {
      s_m_group[i] = s_group_back[colsOrder[i]];
      if (i == 0)
        s_m_group[i]->prev = NULL;
      else
        s_m_group[i-1]->next = s_m_group[i];
      i++;
   }

   s_m_group[singleGroupCount-1]->next = NULL;

   singleGroup = s_m_group[0];

   if (LM->LogNeeded())
   {
      sprintf(LM->msg, "\tMC: order of single columns groups AFTER reorder");
      LM->Log(LM->msg);
   
      // get current single columns order
      sgroup = singleGroup;
      while (sgroup != NULL)
      {
        sprintf(LM->msg, "\tMC: SINGLE has mcs_usingme (%d) col: (%s)",
                sgroup->mcs_usingme, sgroup->colSet[0].colname->data());
        LM->Log(LM->msg);
        sgroup = sgroup->next;
      }
   }
}

/***********************************************/
/* METHOD:  freeMCISmemory                     */
/* PURPOSE: helper function to orderMCGroups   */
/*          free up (remove from memory) any   */
/*          columns that are not being used    */
/*          by the candidate multi-group       */
/*          (the one with the highest weight)  */
/* PARAMS:  s_group_back: array of pointers    */
/*               to the initial list           */
/*               (order) of SC                 */
/*          colsOrder: array of indexes to     */
/*               the s_group_back              */
/*          headGroupCol: number of columns in */
/*               head MC and its neighbors     */
/* RETCODE: none                               */
/* ASSUMPTIONS: none                           */
/***********************************************/

void HSGlobalsClass::freeMCISmemory(HSColGroupStruct* s_group_back[], 
                                    Int32 colsOrder[], Int32 &headGroupCols)
{
   HSLogMan *LM = HSLogMan::Instance();
   HSColGroupStruct* sgroup = singleGroup;
   while (sgroup != NULL)
   {
      // column still in memory but will not be used
      // by head group or its neighbors
      if ((sgroup->state == PROCESSED) && !sgroup->mcis_memFreed)
      {
         Int32 i = 0;
         while (i < headGroupCols)
         {
            if (s_group_back[colsOrder[i]] == sgroup)
              break;
            i++;
         }

         // group was not found in the list of used cols
         if (i >= headGroupCols)
         {
            sgroup->freeISMemory();
            sgroup->mcis_readAsIs = TRUE;
            sgroup->state = UNPROCESSED;

            if (LM->LogNeeded())
            {
               sprintf(LM->msg, "MC: col: (%s) memory is released using freeMCISmemory", 
                                 sgroup->colSet[0].colname->data());
               LM->Log(LM->msg);
            }
         }
      }

      sgroup = sgroup->next;
   }
}

/***********************************************/
/* METHOD:  orderMCGroups                      */
/* PURPOSE: re-order multi-column and single   */    
/*          column groups to maximize the      */
/*          number of multi-column stats that  */
/*          can be done in memory              */
/* PARAMS:  s_group_back: array of pointers    */
/*          to the initial list (order) of SC  */
/* RETCODE: none                               */
/* ASSUMPTIONS: memory requirements for single */
/*              and multi-column groups have   */
/*              already been determined and    */
/*              multi-columns that cannot be   */
/*              processed in memory has        */
/*              already been identified and    */
/*              flagged                        */
/***********************************************/

void HSGlobalsClass::orderMCGroups(HSColGroupStruct* s_group_back[])
{
   // check if reorder of multi/single groups is needed
   if (!orderMCGroupsNeeded())
      return;

     /*================================================================
        The algorithm to identify the order of processing for all MCs.  
        Let MC1, MC2, MC3, . MCn denote n MCs to build.

        0- identify all MC group that cannot be processed in 
           memory (flagged as PROCESSED or DONT_TRY)
        1- For every remaining eligible MCi 
	   a- Let MCj denote another MC where MCi and MCj have 
              common single columns 
	   b- For each such MCj, compute the number of columns to 
              read into memory after MCi is processed. 
              This number is |MCj| - |overlap(MCi,MCj)|. 
	   c- Compute a weight vector (u,v),  
              u= # of MCjs that can be processed after MCi, 
              v= total # of columns in these MCj to read.
	2- Denote MCk as the MCi with the largest u and smallest v
	   a- Remove columns from memory that are not needed to build 
              MCk and its neighbors.
	   b-Reading in columns necessary to built MCk, build MCk
	   c-For each MCj connected to MCk, reading in columns necessary 
             to build MCj, build MCj  
	3- Go back to step 1
     =================================================================*/

   HSLogMan *LM = HSLogMan::Instance();

   LM->StartTimer("MC: orderMCGroups");

   // compute how many columns of each MC that are only used by that MC
   computeSingleUsedCols();

   // compute the weight (u,v) of each MC
   computeMCGroupsWeight();

   // reorder the MC groups to get the MC with the largest weight (u,v)
   reorderMCGroupsByWeight();

   // connect group heads to their neighbors
   formGroupSets();

   // arary of indexes to the s_group_back
   Int32 colsOrder[singleGroupCount];
   // number of columns in the head MC and its neighbors
   Int32 headGroupCols = 0;

   // reorder the single column groups so that all columns used by 
   // the head MC (MC with the largest weight) are listed first, followed
   // by the columns used by the neighbors MCs
   reorderSingleGroupsByWeight(s_group_back, colsOrder, headGroupCols);

   // free up (remove from memory) any columns that are not being used
   // by the candidate multi-group (the one with the highest weight)
   // or its neighbors
   freeMCISmemory(s_group_back, colsOrder, headGroupCols);

   LM->StopTimer();
}

void MCWrapper::fail(const char* opName, Lng32 line)
{
  HSLogMan *LM = HSLogMan::Instance();
  LM->Log("INTERNAL ERROR (MCWrapper):");
  sprintf(LM->msg, "Undefined operator type %s", opName);
  LM->Log(LM->msg);
  *CmpCommon::diags() << DgSqlCode(-UERR_GENERIC_ERROR)
                      << DgString0("MCWrapper")
                      << DgString1("N/A")
                      << DgString2(LM->msg);
  throw CmpInternalException("failure in MCWrapper",
                             __FILE__, line);
}


void ISVarChar::fail(const char* opName, Lng32 line)
{
  HSLogMan *LM = HSLogMan::Instance();
  LM->Log("INTERNAL ERROR (ISVarChar):");
  sprintf(LM->msg, "Undefined operator type %s", opName);
  LM->Log(LM->msg);
  *CmpCommon::diags() << DgSqlCode(-UERR_GENERIC_ERROR)
                      << DgString0("ISVarChar")
                      << DgString1("N/A")
                      << DgString2(LM->msg);
  throw CmpInternalException("failure in ISVarChar",
                             __FILE__, line);
}

// Compare this object to rhs, returning negative value if less, 0 if equal,
// and positive value if greater.
Int32 ISVarChar::compare(const ISVarChar &rhs)
{
  Int32 result;
  Int16 lhsLen = *(short*)content;
  Int16 rhsLen = *(short*)rhs.content;
  Int16 minLen = MINOF(lhsLen, rhsLen);
  Int16 diffLen;
  char* diffPtr;

  // Note that case insensitive is not supported with non-binary collation.
  if (CollationInfo::isSystemCollation(colCollation))
      return Collated_cmp(content+VARCHAR_LEN_FIELD_IN_BYTES,
                          rhs.content+VARCHAR_LEN_FIELD_IN_BYTES,
                          MAXOF(*((short*)content), *((short*)rhs.content)),
                          colCollation, sortBuffer1, sortBuffer2);

  // UCS2 cols not supported in MODE_SPECIAL_1 and do not support case insensitivity.
  if (!caseInsensitive) {
    if (charset != CharInfo::UNICODE)
    {
      result = memcmp(content+VARCHAR_LEN_FIELD_IN_BYTES,
                    rhs.content+VARCHAR_LEN_FIELD_IN_BYTES,
                    minLen);
      if (result != 0 || lhsLen == rhsLen)
        return result;
      else
      {
        if (minLen == lhsLen)
        {
          diffPtr = rhs.content + VARCHAR_LEN_FIELD_IN_BYTES + minLen;
          diffLen = rhsLen - minLen;
        }
        else
        {
          diffPtr = content + VARCHAR_LEN_FIELD_IN_BYTES + minLen;
          diffLen = lhsLen - minLen;
        }
        for (int i = 0; i < diffLen; i++)
        {
          if (*diffPtr++ != ' ')
            return (minLen == lhsLen ? -1 : 1);
        }
        return 0;
      }
    }
    else  
      return compareWcharWithBlankPadding((const wchar_t*)(content+VARCHAR_LEN_FIELD_IN_BYTES),
                          *((short*)content) / sizeof(NAWchar), 
                          (const wchar_t*)(rhs.content+VARCHAR_LEN_FIELD_IN_BYTES),
                          *((short*)rhs.content) / sizeof(NAWchar));
  }
  else                  
    return hs_strncasecmp(content+VARCHAR_LEN_FIELD_IN_BYTES,
                          rhs.content+VARCHAR_LEN_FIELD_IN_BYTES,
                          MAXOF(*((short*)content), *((short*)rhs.content)));
}

Int32 ISVarChar::operator==(const ISVarChar &rhs)
{
  return !compare(rhs);  // returns 1 if equal, 0 if not
}

void IUSFixedChar::operator=(const HSDataBuffer& buff)
{
  Int16 bytesPerChar = (charset == CharInfo::UNICODE ? sizeof(NAWchar) : 1);
  content = new (STMTHEAP) char[length * bytesPerChar];
  size_t buffLenBytes = (size_t)buff.length();
  const char* buffData = buff.data();

  // MFV for interval 0 is always empty without enclosing quotes, so we have to
  // check for that before removing quotes.
  if (buffLenBytes > 0)
    {
      buffLenBytes -= (2 * sizeof(NAWchar));
      buffData += sizeof(NAWchar);
    }

  if (charset == CharInfo::UNICODE)
    {
      memmove(content, buffData, buffLenBytes);
      for (NAWchar* p=(NAWchar*)(content+buffLenBytes);
            p<((NAWchar*)content)+length;
            p++)
        *p = L' ';
    }
  else
    {
      na_wcstombs(content, (const NAWchar*)buffData, buffLenBytes);
      for (char* p=content+(buffLenBytes/sizeof(NAWchar)); p<content+length; p++)
        *p = ' ';
    }
}


void IUSVarChar::operator=(const HSDataBuffer& buff)
{
  size_t sizeFieldBytes = sizeof(Int16);
  size_t buffLenBytes = (size_t)buff.length();
  const char* buffData = buff.data();

  // MFV for interval 0 is always empty without enclosing quotes, so we have to
  // check for that before removing quotes.
  if (buffLenBytes > 0)
    {
      buffLenBytes -= (2 * sizeof(NAWchar));
      buffData += sizeof(NAWchar);
    }

  size_t destBytes = (charset == CharInfo::UNICODE 
                       ? buffLenBytes
                       : buffLenBytes / sizeof(NAWchar));
  content = new (STMTHEAP) char[sizeFieldBytes + destBytes];
  *(Int16*)content = destBytes;
  if (charset == CharInfo::UNICODE)
    memmove(content+sizeFieldBytes, buffData, buffLenBytes);
  else
    na_wcstombs(content+sizeFieldBytes, (const NAWchar*)buffData, destBytes);
}

#ifdef _TEST_ALLOC_FAILURE
Int32 HSColGroupStruct::allocCount = 1;
#endif

// -----------------------------------------------------------------------
// Constructor and destructor.
// -----------------------------------------------------------------------
HSColGroupStruct::HSColGroupStruct()
      : colSet(STMTHEAP), colCount(0), clistr(new(STMTHEAP) NAString(STMTHEAP)),
        oldHistid(0), newHistid(0), colNames(new(STMTHEAP) NAString(STMTHEAP)),
        groupHist(NULL), next(NULL), prev(NULL), state(UNPROCESSED), 
        memNeeded(0), strMemAllocated(0),
        data(NULL), nextData(NULL), strData(NULL), strNextData(NULL),
        strDataConsecutive(TRUE),  // only becomes false if data sets merged for IUS
        varcharFetchBuffer(NULL),
        mcis_data(NULL), mcis_nextData(NULL), mcs_usingme(0), //for MC
        nullIndics(NULL), nullCount(0), mcis_rowsRead(0),
        eligibleForVarCharCompaction(FALSE),
        ISdatatype(-1), ISlength(-1), ISvcLenUsed(-1), ISprecision(-1), ISscale(-1),
        ISSelectExpn(STMTHEAP), prevRowCount(0), prevUEC(0),
        reason(HS_REASON_UNKNOWN), newReason(HS_REASON_MANUAL),
        colSecs(0), coeffOfVar(0), oldAvgVarCharSize(-1), rowsRead(0), sumSize(0),
        avgVarCharSize(-1), skewedValuesCollected(FALSE),
        mcis_nullIndBitMap(NULL), mcis_colsUsedMap(NULL),
        mcis_colsMissingMap(NULL), mcis_memFreed(FALSE),
        mcis_totalMCmemNeeded(0), mcis_groupHead(TRUE), mcis_next(NULL), mcis_readAsIs (FALSE),
        delayedRead(FALSE), cbf(NULL),
        boundaryValues(NULL), MFVValues(NULL), allKeysInsertedIntoCBF(FALSE),
        backwardWarningCount(0)
  {
    strcpy(readTime, "0001-01-01 00:00:00");  // default if new
#ifdef _TEST_ALLOC_FAILURE
    initFilter();
#endif
  }

HSColGroupStruct::~HSColGroupStruct()
  {
    freeISMemory();  // do this first, as it dereferences colNames for logging
    delete clistr;
    delete colNames;
    delete groupHist;
    delete next;
  }

/**
 * Sets the length of the IS type of the column, and the estimated average
 * length if the mapped type is varchar and compacted varchars are in use.
 *
 * @param len Natural length of the column type as represented for IS.
 * @param maxCharColumnLengthInBytes Maximum length character string limit
 *    imposed by UPDATE STATS
 */
void HSColGroupStruct::setISlength(Lng32 len, Lng32 maxCharColumnLengthInBytes)
{
  ISlength = MINOF(len, maxCharColumnLengthInBytes);
  if (!DFS2REC::isAnyVarChar(ISdatatype))
    return;

  if (eligibleForVarCharCompaction)
  {
    // If average varchar size is known from older histograms
    // use that; otherwise use a rule of thumb estimate.

    if (oldAvgVarCharSize >= 1)
      ISvcLenUsed = oldAvgVarCharSize + 4;  // + 4 to allow a little growth
    else
    {
      // In the absence of older histograms, assume the average
      // length is about one half the maximum length. (After all,
      // the user presumably chose varchar to save some space.)
      // Note: This code path can only be taken on the first call
      // to this method. Later calls happen only when we overran
      // buffer space, but in that case oldAvgVarCharSize will 
      // have been calculated.
      double ruleOfThumbEstimate = len/2;
      if (ruleOfThumbEstimate < 4)
        ruleOfThumbEstimate = 4;
      ISvcLenUsed = ruleOfThumbEstimate;
    }

    if (ISvcLenUsed > ISlength)
      ISvcLenUsed = ISlength;  // don't allow it to exceed the actual size!

    HSLogMan *LM = HSLogMan::Instance();
    if (LM->LogNeeded())
    {
      sprintf(LM->msg, "Considering compaction on varchar column %s:", colNames->data());
      LM->Log(LM->msg);
      sprintf(LM->msg, "    Declared len: %d, estimated avg len: %d", ISlength, ISvcLenUsed);
      LM->Log(LM->msg);
      sprintf(LM->msg, "    Compaction%schosen", ISvcLenUsed == len ? " not " : " ");
      LM->Log(LM->msg);
    }
  }
  else
    ISvcLenUsed = ISlength;
}

/**
 * Determines the number of bytes to allocate for strData, the buffer holding
 * all the data for a char/varchar column with internal sort. The overall memory
 * requirement for the column has already been calculated. Here, we just need to
 * determine how much of it is for the data buffer. The other parts are the array
 * of objects (ISFixedChar or ISVarChar) that reference the content, and for
 * a compacted varchar, the buffer that the uncompacted varchar data is read into.
 *
 * @param rows Number of rows being retrieved to calculate stats on.
 * @return Number of bytes to allocate to hold the char or varchar content.
 */
size_t HSColGroupStruct::strDataMemNeeded(Int64 rows)
{
  size_t result = memNeeded;
  HS_ASSERT(DFS2REC::isAnyCharacter(ISdatatype));
  if (DFS2REC::isAnyVarChar(ISdatatype))
  {
    result -= (rows * sizeof(ISVarChar));  // deduct space for ptrs to content
    if (isCompacted())
      result -= (inflatedVarcharContentSize() * MAX_ROWSET);  // deduct pre-compaction fetch buffer
  }
  else
    result -= (rows * sizeof(ISFixedChar));

  return result;
}


// Allocates memory necessary for internal sort for the group. If an allocation
// failure occurs, free any memory already allocated for the current column and
// exit.
//
// Parameters:
//   rows -- number of rows the allocation is based on.
//   allocStrData -- allocate strData array as well as data array for char types.
//                   If false, don't change strData ptr; if non-null, it is
//                   still in use. This parameter defaults to TRUE.
//   recalcMemNeeded -- if TRUE, recalculate memNeeded based on the number of rows.
//                      This happens when mergeDatasetsForIUS() calls this fn to
//                      allocate for the original number of rows + inserted rows.
//                      This parameter defaults to FALSE.
//
// Return value:
//   TRUE if all went well, FALSE if a memory allocation request failed.
//
NABoolean HSColGroupStruct::allocateISMemory(Int64 rows,
                                             NABoolean allocStrData,
                                             NABoolean recalcMemNeeded)
{
  HSLogMan *LM = HSLogMan::Instance();
  NAWchar* wptr;
  NABoolean allAllocated = TRUE;

  try
    {
     // Get new value for memNeeded if necessary. This is the case when IUS
     // datasets are being merged and extra room may be needed for the insert
     // dataset. Null indicators are not affected by this.
     if (recalcMemNeeded)
       {
         // Null indicators should already have been allocated.
         HS_ASSERT(nullIndics || !colSet[0].nullflag);
         GetHSContext()->getMemoryRequirementsForOneGroup(this, rows);
       }
     else if (colSet[0].nullflag)
       {
         // Allocate enough null indicators for a single rowset -- they are
         // processed after each rowset read. If it is a NOT NULL column, do
         // not allocate the null indicator space.
         //
         nullIndics = (short*)newBuiltinArr(short, MAX_ROWSET);
         if (!nullIndics)
           throw ISMemAllocException();

         // setup the null indicator bit for MC IS if this column is used
         // by MCs
         if (mcs_usingme > 0)
         {
             if (!mcis_nullIndBitMap)  
             {
                mcis_nullIndBitMap = new (STMTHEAP) NABitVector (STMTHEAP);
                if (!mcis_nullIndBitMap)
                   throw ISMemAllocException();
             }
         }
       }
     else
       nullIndics = NULL;

     if (DFS2REC::isAnyCharacter(ISdatatype))
       {
         // For all char types, data and nextData will have ptrs to pool of char
         // data read into strData, assigned after each rowset fetch. Blank out
         // entire varchar allocation to allow simple blank-padded comparison
         // of different-length strings.
         //
         // memNeeded includes length for ptrs; subtract it from amount of space
         // to allocate for the strings themselves.
         size_t strMemNeeded = strDataMemNeeded(rows);
         // round up to next multiple of sizeof(short)
         strMemNeeded = sizeof(short) * ( (strMemNeeded + sizeof(short) - 1) / sizeof(short) );
         strMemAllocated = strMemNeeded;  // remember for overrun checking


         if (DFS2REC::isAnyVarChar(ISdatatype))
           {
             if (allocStrData)
               {
                 // Allocate as short to ensure proper alignment for length field.
                 strData = (char*)(newBuiltinArr(short, strMemNeeded / sizeof(short)));
                 if (!strData)
                   throw ISMemAllocException();

                 // Unless varchar values are compacted after being read, blank out the
                 // entire varchar allocation to allow simple blank-padded comparison
                 // of different-length strings.
                 if (isCompacted())
                 {
                   size_t fetchMemNeeded = (inflatedVarcharContentSize() * MAX_ROWSET);
                   fetchMemNeeded = sizeof(short) * 
                                    ( (fetchMemNeeded + sizeof(short) - 1) / sizeof(short) );
                   varcharFetchBuffer =
                         (char*)(newBuiltinArr(short, fetchMemNeeded / sizeof(short)));
                   if (!varcharFetchBuffer)
                     throw ISMemAllocException();
                 }
                 else
                 {
                   if (ISdatatype == REC_BYTE_V_DOUBLE)
                     {
                       Int64 uvCharCount = strMemNeeded / sizeof(NAWchar);
                       wptr = (NAWchar*)strData + uvCharCount - 1;
                       while (uvCharCount--)
                         *wptr-- = L' ';
                     }
                   else
                     memset(strData, ' ', strMemNeeded);
                 }
               }
#ifdef _TEST_ALLOC_FAILURE
             data = newObjArrX(ISVarChar, rows, allocCount++);
#else
             data = newObjArr(ISVarChar, rows);
#endif
             if (!data)
               throw ISMemAllocException();
           }
         else
           {
             if (allocStrData)
               {
                 strData = newBuiltinArr(char, strMemNeeded);
                 if (!strData)
                   throw ISMemAllocException();
               }
#ifdef _TEST_ALLOC_FAILURE
             data = newObjArrX(ISFixedChar, rows, allocCount++);
#else
             data = newObjArr(ISFixedChar, rows);
#endif
             if (!data)
               throw ISMemAllocException();
           }

         strNextData = strData;
       }
     else
       {
         data = newBuiltinArr(char, memNeeded);
         if (!data)
           throw ISMemAllocException();

         // for MC IS copy the data to an MC structure to be used later by
         // MCs. We need to copy the non-char data because the IS logic
         // for single columns sort this data in place
         if (mcs_usingme > 0)
         {
           mcis_data = newBuiltinArr(char, memNeeded);
           if (!mcis_data)
             throw ISMemAllocException();
         }
       }
    }
  catch(ISMemAllocException&)
    {
      allAllocated = FALSE;
      freeISMemory();   // get rid of partial allocation
    }

  return allAllocated;
}

// freeStrData defaults to TRUE, and causes the memory allocated for char types
// to be freed (for char types, the data array contains simple objects that
// reference the actual strings, which are in strData). mergeDatasetsForIUS()
// allocates a new data array, but reuses ptrs into strData, so we need to
// avoid freeing strData when this fn is called to remove the old data array.
void HSColGroupStruct::freeISMemory(NABoolean freeStrData, NABoolean freeMCData)
{
  HSLogMan *LM = HSLogMan::Instance();
  if (LM->LogNeeded())
    {
      *LM << "Freeing IS memory for column " << colNames->data();
      LM->FlushToLog();
    }

  // used by MC in-memory since a column might have been processed but kept
  // in memory to be used by MCs
  mcis_memFreed = TRUE;

  NADELETEBASIC(nullIndics, STMTHEAP);
  nullIndics = NULL;

  if (DFS2REC::isAnyCharacter(ISdatatype))
    {
      if (DFS2REC::isAnyVarChar(ISdatatype))
        {
          delObjArr(data, ISVarChar)
          if (freeStrData && freeMCData)
            {
              NADELETEBASIC((short*)strData, STMTHEAP);
              strData = NULL;
              if (isCompacted())
                {
                  NADELETEBASIC((short*)varcharFetchBuffer, STMTHEAP);
                  varcharFetchBuffer = NULL;
                }
            }
        }
      else
        {
          delObjArr(data, ISFixedChar);
          if (freeStrData && freeMCData)
            {
              NADELETEBASIC((char*)strData, STMTHEAP);
              strData = NULL;
            }
        }
    }
  else
  {
     NADELETEBASIC((char *)data, STMTHEAP);
     if (freeMCData && mcis_data)
        NADELETEBASIC((char *)mcis_data, STMTHEAP);
  }

  data = NULL;
  nextData = NULL;
  strNextData = NULL;

  if (freeMCData)
  {
     mcis_data = NULL;
     mcis_nextData = NULL;
  }
}

// Copy constructor
HSColumnStruct::HSColumnStruct(const HSColumnStruct &src, NAMemory *h)
  {
    colname         = new (h) NAString(src.colname->data(), h);
    externalColumnName = new (h) NAString(src.externalColumnName->data(), h);
    colnum          = src.colnum;
    position        = src.position;
    datatype        = src.datatype;
    nullflag        = src.nullflag;
    charset         = src.charset;
    length          = src.length;
    precision       = src.precision;
    scale           = src.scale;
    caseInsensitive = src.caseInsensitive;
    colCollation    = src.colCollation;
  }


HSColumnStruct::~HSColumnStruct()
  {
    if (colname != NULL)
      {
        delete colname;
        colname = NULL;
      }
    if (externalColumnName != NULL)
      {
        delete externalColumnName;
        externalColumnName = NULL;
      }
  }

// Assignment operator
HSColumnStruct& HSColumnStruct::operator=(const HSColumnStruct& rhs)
  {
    if (this == &rhs)
        return *this;

    NABasicObject::operator=(rhs);

    // Do not do the delete of "colname"; this may cause delete of data that is
    // already deleted; colname is on the STMTHEAP and will be destructed at the
    // end of the statement. [SOL 10-070822-6995]
    colname         = new (STMTHEAP) NAString(rhs.colname->data(), STMTHEAP);
    externalColumnName = new (STMTHEAP) NAString(rhs.externalColumnName->data(), STMTHEAP);
    colnum          = rhs.colnum;
    position        = rhs.position;
    datatype        = rhs.datatype;
    nullflag        = rhs.nullflag;
    charset         = rhs.charset;
    length          = rhs.length;
    precision       = rhs.precision;
    scale           = rhs.scale;
    caseInsensitive = rhs.caseInsensitive;
    colCollation    = rhs.colCollation;

    return *this;
  }

NABoolean HSColumnStruct::operator==(const HSColumnStruct& other) const
  {
    return ( colnum == other.colnum );
  }

//
// METHOD:  addTruncatedColumnReference()
//
// PURPOSE: Generates a column reference or a SUBSTRING
//          on a column reference which truncates the
//          column to the maximum length allowed in
//          UPDATE STATISTICS.
//
// INPUT:   'qry' - the SQL query string to append the 
//          reference to.
//          'colInfo' - struct containing datatype info
//          about the column.
//
void HSColumnStruct::addTruncatedColumnReference(NAString & qry)
  {
    HSGlobalsClass *hs_globals = GetHSContext();
    Lng32 maxLengthInBytes = hs_globals->maxCharColumnLengthInBytes;
    bool isOverSized = DFS2REC::isAnyCharacter(datatype) &&
                           (length > maxLengthInBytes);
    if (isOverSized)
      {
        // Note: The result data type of SUBSTRING is VARCHAR, always.
        // But if the column is CHAR, many places in the ustat code are not
        // expecting a VARCHAR. So, we stick a CAST around it to convert
        // it back to a CHAR in these cases.

        NABoolean isFixedChar = DFS2REC::isSQLFixedChar(datatype);
        if (isFixedChar)
          qry += "CAST(";
        qry += "SUBSTRING(";
        qry += externalColumnName->data();
        qry += " FOR ";
        
        char temp[20];  // big enough for "nnnnnn)"
        sprintf(temp,"%d)", maxLengthInBytes / CharInfo::maxBytesPerChar(charset));
        qry += temp;
        if (isFixedChar)
          {
            qry += " AS CHAR(";
            qry += temp;
            qry += ")";
          }
        qry += " AS ";
        qry += externalColumnName->data();
      }
    else
      qry += externalColumnName->data();
  }


HSInterval::HSInterval()
      : rowCount_(0), uecCount_(0), gapMagnitude_(0), highFreq_(FALSE),
        MFVrowCount_(0), MFV2rowCount_(0), squareCntSum_(0), origUec_(0),
        origRC_(0), origMFV_(0)
  {}


HSInterval::~HSInterval()
  {
  }

FrequencyCounts::FrequencyCounts() 
{ 
  for (ULng32 i=0; i<FC_NUM_HT_BUCKETS; i++)
    bigfiHT_[i].next_ = 0;

  reset();
}

FrequencyCounts::~FrequencyCounts()
{
  for (ULng32 i=0; i<FC_NUM_HT_BUCKETS; i++)
    {
      struct entry *ent = bigfiHT_[i].next_;
      
      while (ent)
        {
          struct entry *next = ent->next_;
          NADELETEBASIC(ent, STMTHEAP);
          ent = next;
        }
    }
}

FrequencyCounts& FrequencyCounts::operator=(const FrequencyCounts& rhs)
{
  memmove(fiArr_, rhs.fiArr_, sizeof rhs.fiArr_);
  for (Int32 i=0; i<FC_NUM_HT_BUCKETS; i++)
    {
      bigfiHT_[i].ix_ = rhs.bigfiHT_[i].ix_;
      bigfiHT_[i].value_ = rhs.bigfiHT_[i].value_;

      entry* rhsNextEntry = rhs.bigfiHT_[i].next_;
      entry** nextEntryAddr = &bigfiHT_[i].next_;
      while (rhsNextEntry)
        {
          *nextEntryAddr = newEntry(rhsNextEntry->ix_, rhsNextEntry->value_);
          nextEntryAddr = &(*nextEntryAddr)->next_;
          rhsNextEntry = rhsNextEntry->next_;
        }
      *nextEntryAddr = NULL;
    }
  
  return *this;
}

void FrequencyCounts::reset()
{
  for (ULng32 i=0; i<FC_NUM_STORED_VALUES; i++) fiArr_[i] = 0;
  resetHT();
}

void FrequencyCounts::increment(Int64 i, ULng32 val)
{
  if (i > 0)
    {
      UInt32 ix = 
        (UInt32) ((i > UINT_MAX) ? UINT_MAX : i);
      
      if (ix < FC_NUM_STORED_VALUES) 
        fiArr_[ix] += val;
      else
        incrementHT(ix, val);
    }
}

ULng32 FrequencyCounts::operator[](Int64 i)
{
  if (i==0) return 0;
  
  UInt32 ix = 
    (UInt32) ((i > UINT_MAX) ? UINT_MAX : i);
  
  if (ix < FC_NUM_STORED_VALUES) 
    return fiArr_[ix];
  else
    return lookupHT(ix);
}

void FrequencyCounts::mergeTo(FrequencyCounts &f)
{
  ULng32 i;

  for (i=0; i<FC_NUM_STORED_VALUES; i++)
    f.increment(i, fiArr_[i]);

  for (i=0; i<FC_NUM_HT_BUCKETS; i++)
    if (bigfiHT_[i].ix_>0)
      {
        struct entry *ent = &(bigfiHT_[i]);
        while (ent)
          {
            f.increment(ent->ix_, ent->value_);
            ent = ent->next_;
          }
      }
}

FrequencyCounts::entry *FrequencyCounts::hashToBucket(ULng32 ix)
{
  return &(bigfiHT_[ix % FC_NUM_HT_BUCKETS]);
}

ULng32 FrequencyCounts::lookupHT(ULng32 ix)
{
  struct entry *ent = hashToBucket(ix);
  while (ent)
    {
      if (ix == ent->ix_) return ent->value_;
      ent = ent->next_;
    }

  return 0;
}
 
void FrequencyCounts::resetHT()
{
  for (ULng32 i=0; i<FC_NUM_HT_BUCKETS; i++)
    {
      struct entry *ent = &(bigfiHT_[i]);
      while (ent)
        {
          ent->ix_ = 0;
          ent->value_ = 0;
          ent = ent->next_;
        }
    }
}

FrequencyCounts::entry *FrequencyCounts::newEntry(ULng32 ix, ULng32 value)
{
  struct entry *ent = new (STMTHEAP) struct entry;

  ent->ix_ = ix;
  ent->value_ = value;
  ent->next_ = 0;

  return ent;
}

void FrequencyCounts::incrementHT(ULng32 ix, ULng32 val)
{
  struct entry *ent = hashToBucket(ix);
  
  while (ent)
    {
      if (ix == ent->ix_) 
        {
          ent->value_ += val;
          return;
        }
      
      if (ent->next_)
        ent = ent->next_;
      else 
        {
          ent->next_ = newEntry(ix, 1);
          return;
        }
    }
}

GapKeeper::GapKeeper(Int32 gapsToKeep)
  :gapsToKeep_(gapsToKeep)
{
  gaps_ = new (STMTHEAP) double[gapsToKeep];
  for (Int32 i=0; i<gapsToKeep; i++)
    gaps_[i] = 0;
}

GapKeeper::~GapKeeper()
  {
  }

double GapKeeper::smallest()
{
  Int32 i=gapsToKeep_ - 1;
  while (i >= 0 && gaps_[i] == 0)
    i--;
  return (i<0 ? 0 : gaps_[i]);
}

Int32 GapKeeper::qualifyingGaps(double minAcceptableGap)
{
  Int32 count = 0;
  for (Int32 i=0; i<gapsToKeep_; i++)
    {
      if (gaps_[i] >= minAcceptableGap)
        count++;
    }
  return count;
}

/**************************************************************************/
/* METHOD:  insert()                                                      */
/* PURPOSE: If the passed gap is one of the gapsToKeep_ highest gaps, add */
/*          it to the gaps_ array in the proper order, else do nothing.   */
/* PARAMS:  gap(in) -- Gap magnitude to (possibly) add to ordered array.  */
/* RETCODE: TRUE if the value was inserted, FALSE otherwise.              */
/**************************************************************************/
NABoolean GapKeeper::insert(double gap)
{
  // Gap magnitudes are stored in the array in ascending order. Go through the
  // array until you find the proper location for the new value (if any), insert
  // it, and push everything after it down.
  Int32 i = 0;
  while (i<gapsToKeep_ && gaps_[i] >= gap)
    i++;
  if (i < gapsToKeep_)
    {
      for (Int32 j=gapsToKeep_ - 2; j>=i; j--)
        gaps_[j+1] = gaps_[j];
      gaps_[i] = gap;
      return TRUE;
    }
  else
    return FALSE;
}


HSHistogram::HSHistogram(Lng32 intcount,
                         Int64 rowcount,
                         Lng32 gapIntervals,      // target # of gap intervals
                         Lng32 highFreqIntervals, // # extra for high frequency values
                         NABoolean sampleUsed,
                         NABoolean singleIntervalPerUec)
      : gapKeeper_(gapIntervals), intCount_(intcount), currentInt_(0), remRows_(rowcount),
        hasNull_(FALSE), fi_(0), gapIntCount_(0),
        targetGapIntervals_(gapIntervals),
        highFreqIntervalsAllotted_(highFreqIntervals), highFreqIntervalsUsed_(0),
        singleIntervalPerUec_(singleIntervalPerUec),
        maxStddev_(0)
  {
    if(singleIntervalPerUec_) // single distinct value per histogram interval
      {
        step_ = 1;
      }
    else                      // more than one distinct value per histogram interval
      {
        // Don't use intervals provided for high frequency values in step calculation),
        // and also account for gap intervals, and the intervals that precede them,
        // which will be half empty on average.
        step_ = originalStep_ 
              = rowcount / (intcount - highFreqIntervals - (Lng32)(gapIntervals * 1.5));
      }

    // Calculate frequency count required to establish a separate interval for
    // a single distinct value.
    highFreqThreshold_ = 
        (Int64)(rowcount * (CmpCommon::getDefaultNumeric(USTAT_FREQ_SIZE_PERCENT) / 100));

    // In addition to the number of intervals requested, we need 2 more for
    // interval 0 and possibly the NULL interval. We also add extras based
    // on the number of gap intervals we want to keep. These will be used
    // as necessary to maintain a stable step size as low-frequency intervals
    // are used for gaps.
    maxAllowedInts_ = intCount_ + 10 * gapIntervals;
    // Put on global heap, NABasicObject and MX heaps do not
    // work with arrays.
    intArry_ = new HSInterval[maxAllowedInts_ + 2];

    if (sampleUsed &&
        CmpCommon::getDefault(USTAT_FORCE_MOM_ESTIMATOR) == DF_OFF)
      {
        // sampling is being used and the new estimator, so we
        // need to maintain frequency counts
        // Put on global heap, NABasicObject and MX heaps do not
        // work with arrays.
        fi_ = new FrequencyCounts[maxAllowedInts_ + 2]; 
      }

    // If handling gaps, set up the gap multiplier that determines which gaps
    // are big enough to consider.
    if (targetGapIntervals_ > 0)
      {
        HSGlobalsClass *hs_globals = GetHSContext();
        HSLogMan *LM = HSLogMan::Instance();
        float sampleRatio = (float) hs_globals->samplePercentX100 / 10000;
          // Sample percent is stored in HISTOGRAMS table as percent * 100.
        gapMultiplier_ = CmpCommon::getDefaultNumeric(USTAT_GAP_SIZE_MULTIPLIER);
        if (sampleRatio > 0 && sampleRatio < 1)
          gapMultiplier_ += ((double)(1 - sampleRatio));
        if (LM->LogNeeded())
          {
            sprintf(LM->msg, "%f used as gap multiplier", gapMultiplier_);
            LM->Log(LM->msg);
          }
      }
  }

HSHistogram::~HSHistogram()
  {
    if (intArry_ != NULL)
      {
        delete [] intArry_;
        intArry_ = NULL;
      }
    deleteFiArray();
  }

void HSHistogram::deleteFiArray()
  {
    if (fi_ != NULL)
      {
        delete [] fi_;
        fi_ = NULL;
      }
  }
     

Lng32 HSHistogram::getLowValue(HSDataBuffer &lval, NABoolean addParen)
  {
    Lng32 retcode = 0;

    lval = intArry_[0].boundary_;
    if (addParen)
      {
        retcode = lval.addParenthesis();
        HSHandleError(retcode);
      }

    return retcode;
  }

Lng32 HSHistogram::getHighValue(HSDataBuffer &hval, NABoolean addParen)
  {
    Lng32 i;
    Lng32 retcode = 0;

    //The high value for the histogram table may not be NULL. Although,
    //it is OK for histogram_interval table.
    if (hasNull_)
      i = currentInt_- 1;
    else
      i = currentInt_;

    hval = intArry_[i].boundary_;
    if (addParen)
      {
        retcode = hval.addParenthesis();
        HSHandleError(retcode);
      }

    return retcode;
  }


/***********************************************/
/* METHOD:  processIntervalValues()            */
/* PURPOSE: processes entire rowset, while     */
/*          maintaining the INTERVAL boundary  */
/*          information. Each interval will    */
/*          have the #rows, #uec, low/hi values*/
/* PARAMS:  boundaryRowSet - rowset containing */
/*                           data values.      */
/*          group - group intervals are being  */
/*                  processed for.             */
/*                       external format string*/
/*          rowsInSet - Accumulation of how    */
/*                      many data records have */
/*                      been processed.        */
/*          currentGapAvg -- Average gap       */
/*                           size so far.      */
/* RETCODE:  0 - successful                    */
/*          -1 - failure                       */
/* ASSUMPTIONS: The data is SORTED(increasing) */
/* NOTES:   bndry:   )[----](----]...(----]    */
/*          int#     0   1     2  ...   n      */
/***********************************************/
Lng32 HSHistogram::processIntervalValues(boundarySet<myVarChar>* boundaryRowSet,
                                        HSColGroupStruct *group,
                                        Int64 &rowsInSet,
                                        double currentGapAvg)
  {
    Lng32 retcode = 0;
    ULng32 sumSize = 0;
    NABoolean maxLongLimit = FALSE;
    NABoolean computeVarCharSize = FALSE;
    HSLogMan *LM = HSLogMan::Instance();

    // is it single group and data type is varchar? set it to TRUE.
    if (group->computeAvgVarCharSize())
      computeVarCharSize = TRUE;
    rowsInSet = 0;
    Lng32 indexOfLastNonNullRow =
                  (boundaryRowSet->nullInd[boundaryRowSet->size-1] == -1
                    ? boundaryRowSet->size - 2
                    : boundaryRowSet->size - 1);
    Lng32 colCount = group->colCount;
    NABoolean allColumnValsNULLs = FALSE;
    if(colCount > 1)
    {
	char * value = (char *)&(boundaryRowSet->data[boundaryRowSet->size-1]);
	value += sizeof(short);

	NAWString boundaryVal((NAWchar *)value);
	NAWString nullStr(L"NULL");
	for (Lng32 colNum=1; colNum < colCount; colNum++)
		nullStr.append(L",NULL");
	if(boundaryVal == nullStr)
	{
		allColumnValsNULLs = TRUE;
		indexOfLastNonNullRow = boundaryRowSet->size - 2;
	}
    }

    for (Lng32 i=0; i < boundaryRowSet->size; i++)
      {
        rowsInSet += boundaryRowSet->dataSum[i];
        // varchar type AND max long limit not reached.
        if (computeVarCharSize AND (!maxLongLimit) )
        {
          // Not a NULL value. 
          if (boundaryRowSet->avgVarCharNullInd[i] != -1)
          {
            sumSize = sumSize + (ULng32)(boundaryRowSet->avgVarCharSize[i] *
                                 boundaryRowSet->dataSum[i]);
            if (sumSize >= INT_MAX)
            {
              maxLongLimit = TRUE;
              group->avgVarCharSize = (double)sumSize / (double)rowsInSet;
            }
          }
        } // computeVarCharSize
        
        if ((boundaryRowSet->nullInd[i] == -1) ||
                (allColumnValsNULLs && i == boundaryRowSet->size-1))
          {
            //10-030703-7604: It is expected that once a NULL value has been
            //processed, no other rows follow.
            HS_ASSERT(NOT hasNullInterval());
            addNullInterval(boundaryRowSet->dataSum[i], group->colCount);
          }
        else
          {
            // Determine the gap magnitude associated with this distinct value
            // and whether or not it is a significant one, and include this in
            // the information passed to addIntervalData. Note that if an interval
            // qualifies as high frequency, it will not be considered as a gap
            // interval.
            double currGapMagnitude = boundaryRowSet->gapMagnitude[i];
            NABoolean bigGap = currGapMagnitude > currentGapAvg * gapMultiplier_
                                 && boundaryRowSet->dataSum[i] <= highFreqThreshold_
                                 && gapKeeper_.insert(currGapMagnitude);
            retcode = addIntervalData(boundaryRowSet->data[i],
                                      group,
                                      boundaryRowSet->dataSum[i],
                                      bigGap,
                                      currGapMagnitude,
                                      i == indexOfLastNonNullRow);
          }
      }
      
    if ( (computeVarCharSize) AND (group->avgVarCharSize == -1) )
    {
      group->avgVarCharSize = (double)sumSize / (double)rowsInSet;

    }
    if (LM->LogNeeded())
      {
        char rowStr[30];
        
        convertInt64ToAscii(rowsInSet, rowStr);
        sprintf(LM->msg, "\tProcessed %5s rows, %5d uec, retcode = %5d",
                             rowStr,
                             boundaryRowSet->size,
                             retcode);
        LM->Log(LM->msg);
      }

    return retcode;
  }

void HSHistogram::addNullInterval(const Int64 nullCount, const Lng32 colCount)
  {
    NAWString nullTxt = WIDE_("NULL");
    if(colCount > 1)
    {
      for(int i = 1; i < colCount; i++)
        nullTxt.append(WIDE_(", NULL"));
    }

    HSDataBuffer nullVal(nullTxt.data());

    if (currentInt_ == 0)          // first datapoint
      intArry_[0].boundary_ = nullVal;
    currentInt_++;
    double nullCountd                   = (double) nullCount;
    intArry_[currentInt_].rowCount_     = nullCount;
    intArry_[currentInt_].uecCount_     = 1;
    intArry_[currentInt_].squareCntSum_ = nullCountd * nullCountd;
    intArry_[currentInt_].boundary_     = nullVal;
    if (fi_) fi_[currentInt_].increment(nullCount);

    hasNull_ = TRUE;
  }

Lng32 HSHistogram::updateMCInterval(const HSDataBuffer &lowval,
                                   const HSDataBuffer &hival)
  {
    intArry_[0].boundary_ = lowval;
    intArry_[1].boundary_ = hival;
    return 0;
  }

/***********************************************/
/* METHOD:  getTotalCounts()                   */
/* PURPOSE: Determine the SUM(rowcount) and the*/
/*          SUM(uec).                          */
/* IN/OUT:  rowCount = SUM(rowcount)           */
/*          uecCount = SUM(uec)                */
/* NOTES:   bndry:   )[----](----]...(----]    */
/*          int#     0   1     2  ...   n      */
/***********************************************/
void HSHistogram::getTotalCounts(Int64 &rowCount, Int64 &uecCount)
  {
    rowCount = 0;
    uecCount = 0;

    //Since interval 0 is exclusive, the iterator must
    //begin at 1. Since the last interval is inclusive,
    //we must process the last interval.
    for (Lng32 i=1; i <= currentInt_; i++)
      {
        rowCount += intArry_[i].rowCount_;
        uecCount += intArry_[i].uecCount_;
      }
  }


// Get the original total counts
void HSHistogram::getOrigTotalCounts(Int64 &rowCount, Int64 &uecCount)
  {
    rowCount = 0;
    uecCount = 0;

    //Since interval 0 is exclusive, the iterator must
    //begin at 1. Since the last interval is inclusive,
    //we must process the last interval.
    for (Lng32 i=1; i <= currentInt_; i++)
      {
        rowCount += getIntOrigRC(i);
        uecCount += getIntOrigUec(i);
      }
  }



/***********************************************/
/* METHOD:  getTotalUec()                      */
/* PURPOSE: Determine the SUM(uec)             */
/* RETURN:  SUM(uec)                           */
/* NOTES:   bndry:   )[----](----]...(----]    */
/*          int#     0   1     2  ...   n      */
/***********************************************/
Int64 HSHistogram::getTotalUec()
  {
    Int64 uec = 0;

    //Since interval 0 is exclusive, the iterator must
    //begin at 1. Since the last interval is inclusive,
    //we must process the last interval.
    for (Lng32 i=1; i <= currentInt_; i++)
      {
        uec += intArry_[i].uecCount_;
      }
    return uec;
  }


/***********************************************/
/* METHOD:  getTotalRowCount()                 */
/* PURPOSE: Determine the SUM(rowcount)        */
/* RETURN:  SUM(rowcount)                      */
/* NOTES:   bndry:   )[----](----]...(----]    */
/*          int#     0   1     2  ...   n      */
/***********************************************/
Int64 HSHistogram::getTotalRowCount()
  {
    Int64 rowCount = 0;

    //Since interval 0 is exclusive, the iterator must
    //begin at 1. Since the last interval is inclusive,
    //we must process the last interval.
    for (Lng32 i=1; i <= currentInt_; i++)
      {
        rowCount += intArry_[i].rowCount_;
      }
    return rowCount;
  }

Lng32 HSHistogram::getParenthesizedIntBoundary(Lng32 intNum, HSDataBuffer &intBoundary)
  {
    Lng32 retcode = 0;
    HS_ASSERT(intNum >= 0);
    HS_ASSERT(intNum <= currentInt_);

    intBoundary = intArry_[intNum].boundary_;
    retcode = intBoundary.addParenthesis();
    HSHandleError(retcode);

    return retcode;
  }

Lng32 HSHistogram::getParenthesizedIntMFV(Lng32 intNum, HSDataBuffer &intMostFreqVal)
  {
    Lng32 retcode = 0;
    HS_ASSERT(intNum >= 0);
    HS_ASSERT(intNum <= currentInt_);

    intMostFreqVal = intArry_[intNum].mostFreqVal_;
    retcode = intMostFreqVal.addParenthesis();
    HSHandleError(retcode);

    return retcode;
  }

// initialize static variables of HSGlobalsClass
THREAD_P COM_VERSION HSGlobalsClass::schemaVersion = COM_VERS_UNKNOWN;   
THREAD_P Lng32 HSGlobalsClass::autoInterval = 0;

Int64 HSGlobalsClass::getMinRowCountForSample()
{
  return (Int64)CmpCommon::getDefaultLong(USTAT_MIN_ROWCOUNT_FOR_SAMPLE);
}

Int64 HSGlobalsClass::getMinRowCountForLowSample()
{
  return (Int64)CmpCommon::getDefaultLong(USTAT_MIN_ROWCOUNT_FOR_LOW_SAMPLE);
}

void HSHistogram::logAll(const char* title)
{
  HSLogMan *LM = HSLogMan::Instance();

  if (!LM->LogNeeded()) return;

  LM->Log(title);

  char buf[50];

  sprintf(buf, "Total RC=%d, Total UEC=%d", (Lng32)getTotalRowCount(), (Lng32)getTotalUec());
  LM->Log(buf);

  logIntervals();
}

/****************************************************************************/
/* METHOD:  logIntervals()                                                  */
/* PURPOSE: Logs the current state of the intervals of the histogram. If    */
/*          curr and lookahead are non-negative, the function is being used */
/*          to trace the intermediate steps in merging undersized gap       */
/*          intervals into adjacent intervals (removeLesserGapIntervals()). */
/*          In this case, the current and lookahead interval for the merge  */
/*          procedure are indicated in addition to displaying the basic     */
/*          information for each interval.                                  */
/* PARAMS:  curr(in)      -- Index of current interval, -1 if not needed.   */
/*          lookahead(in) -- Index of interval being evaluated, -1 if not   */
/*                           needed.                                        */
/****************************************************************************/
void HSHistogram::logIntervals(Lng32 curr, Lng32 lookahead)
{
  HSLogMan *LM = HSLogMan::Instance();

  if (!LM->LogNeeded()) 
     return;

  const Int32 MAX_CHARS = 20;
  char boundary[MAX_CHARS+1];
  char mostFreqVal[MAX_CHARS+1];

  sprintf(LM->msg, "   |int|  rc | uec | boundary | mfv#|2mfv#| mfv value|  gap size|");
  LM->Log(LM->msg);

  Int32 numBoundaryChars;
  wchar_t* wchPtr;
  for (Lng32 i=0; i<=getNumIntervals(); i++)
    {
      wchPtr = (wchar_t*)(intArry_[i].boundary_.data());
      numBoundaryChars = MINOF(MAX_CHARS, intArry_[i].boundary_.numChars());
      for (Lng32 k=0;k<numBoundaryChars; k++)
        boundary[k] = (char)(*wchPtr++);
      boundary[numBoundaryChars] = '\0';

      wchPtr = (wchar_t*)(intArry_[i].mostFreqVal_.data());
      numBoundaryChars = MINOF(MAX_CHARS, intArry_[i].mostFreqVal_.numChars());
      for (Lng32 k=0;k<numBoundaryChars; k++)
        mostFreqVal[k] = (char)(*wchPtr++);
      mostFreqVal[numBoundaryChars] = '\0';

      // Note that if curr and lookahead are negative, they will match no intvl
      // value and the C-> and L-> indicators will not appear in the output.
      sprintf(LM->msg, "%3s|%3d|%5d|%5d|%10s|%5d|%5d|%10s|%10.2f|",
             i == curr ? "C->" : (i == lookahead ? "L->" : " "),
             i,
             (Int32)intArry_[i].rowCount_,  // blows up without the cast
             (Int32)intArry_[i].uecCount_,
             boundary,
             (Int32)intArry_[i].MFVrowCount_,
             (Int32)intArry_[i].MFV2rowCount_,
             mostFreqVal,
             intArry_[i].gapMagnitude_
             );
      LM->Log(LM->msg);
    }
  LM->Log("==================================================");
}

void HSGlobalsClass::logDiagArea(const char* title)
{
  HSLogMan *LM = HSLogMan::Instance();
  if (!LM->LogNeeded())
    return;

  Lng32 warnings = diagsArea.getNumber(DgSqlCode::WARNING_);
  Lng32 errors = diagsArea.getNumber(DgSqlCode::ERROR_);

  sprintf(LM->msg, "%s, HSGlobalsClass::diagsArea, #errors=%d, #warnings=%d", title, errors, warnings);
  LM->Log(LM->msg);
}

/**************************************************************************/
/* METHOD:  sumFrequencies()                                              */
/* PURPOSE: For each value count up to FC_NUM_STORED_VALUES (larger counts are*/
/*          in a hash table instead of an array indexed by count), sum    */
/*          the frequencies in the fi array into a single FrequencyCounts */
/*          object that spans all intervals. Each element in fi           */
/*          represents an interval, and stores an array in which the      */
/*          value at each index is the number of values that occur that   */
/*          many times. For example, fi[4][3] == 8 indicates that in      */
/*          interval 4, there are 8 values that occur 3 times.            */
/*          NOTE: This function is only used for logging and is called    */
/*          before and after the merging of intervals for gap encoding    */
/*          as a rough check (frequency counts over FC_NUM_STORED_VALUES are  */
/*          not validated, and accurate distribution of counts within     */
/*          intervals is not determined) on the correctness of merging    */
/*          frequency counts as intervals are merged.                     */
/* PARAMS:  fc(out)         -- FrequencyCounts object that accumulates the*/
/*                             sum of the frequencies in fi.              */
/*          maxInterval(in) -- Index of interval being evaluated.         */
/*          fi(in)          -- Array of frequency counts to sum.          */
/**************************************************************************/
static void sumFrequencies(FrequencyCounts& fc, Int32 maxInterval, FrequencyCounts* fi)
{
  if (!fi)
    return;

  for (Int32 i=1; i<=maxInterval; i++)
    for (Int32 valCount=0; valCount<FC_NUM_STORED_VALUES; valCount++)
      {
        fc.increment(valCount, fi[i][valCount]);
      }
}

/**************************************************************************/
/* METHOD:  compareFC()                                                   */
/* PURPOSE: Compare the two frequency counts to determine if for each     */
/*          incidence count, there are the same number of values in each  */
/*          that occur that many times. See the comments for the          */
/*          sumFrequencies() function above.                              */
/* PARAMS:  fc1,fc2(in) -- The FrequencyCounts objects to compare.        */
/* RETURN:  TRUE if a difference is found.                                */
/**************************************************************************/
NABoolean compareFC(FrequencyCounts& fc1, FrequencyCounts& fc2)
{
  HSLogMan *LM = HSLogMan::Instance();
  if (LM->LogNeeded())
    {
      sprintf(LM->msg,
              "Comparing overall (sum across intervals) frequency counts (only frequencies up to %d)",
              FC_NUM_STORED_VALUES);
      LM->Log(LM->msg);
    }
  NABoolean differenceFound = FALSE;
  for (Int32 valCount=0; valCount<FC_NUM_STORED_VALUES; valCount++)
    {
      if (fc1[valCount] != fc2[valCount])
        {
          differenceFound = TRUE;
          if (LM->LogNeeded())
            {
              sprintf(LM->msg,
                      "Difference in number of values occurring %d times: %d vs. %d",
                      valCount, fc1[valCount], fc2[valCount]);
              LM->Log(LM->msg);
            }
        }
    }
  if (!differenceFound && LM->LogNeeded())
    {
      sprintf(LM->msg,
              "No difference found in FrequencyCounts objects comparing frequencies up to %d",
              FC_NUM_STORED_VALUES);
      LM->Log(LM->msg);
    }

  return differenceFound;
}

/**************************************************************************/
/* METHOD:  removeLesserGapIntervals()                                    */
/* PURPOSE: Now that the true overall gap average is known, revisit the   */
/*          gap intervals we created, and keep the gapIntCount ones with  */
/*          the greatest gap magnitude. The rest are merged into adjacent */
/*          intervals, unless that would create an interval of excessive  */
/*          height.                                                       */
/* PARAMS:  trueGapAvg(in) -- The actual gap average, after all gaps have */
/*                            been seen.                                  */
/**************************************************************************/
void HSHistogram::removeLesserGapIntervals(double trueGapAvg)
{
  HSLogMan *LM = HSLogMan::Instance();
  Int32 log_mergedGaps=0;             // for logging use only
  FrequencyCounts *FCbeforePtr = 0; // logging only; heavyweight object, so use ptr
                                    //   to avoid instantiation unless necessary

  // The minimum acceptable gap magnitude is found using the multiplier specified
  // by USTAT_GAP_SIZE_MULTIPLIER. However, we bump up the minimum enough to
  // limit the intervals selected to the number we want to keep.
  double minGapToKeep = gapMultiplier_ * trueGapAvg;
  if (LM->LogNeeded())
    {
      sprintf(LM->msg, "min acceptable gap based on gap average = %f", minGapToKeep);
      LM->Log(LM->msg);
      if (fi_)
        {
          // FCbeforePtr is used to compare overall frequency counts with those
          // obtained after merging intervals later in this function, and is
          // deleted immediately after that.  
          // Create on global heap - FrequencyCounts is not NABasicObject.
          FCbeforePtr = new FrequencyCounts;
          sumFrequencies(*FCbeforePtr, currentInt_, fi_);
        }
    }
  if (gapKeeper_.smallest() > minGapToKeep)
    {
      minGapToKeep = gapKeeper_.smallest();
      if (LM->LogNeeded())
        {
          sprintf(LM->msg, "min acceptable gap to accommodate number of gap intervals  = %f", minGapToKeep);
          LM->Log(LM->msg);
        }
    }
  if (LM->LogNeeded() && targetGapIntervals_ < gapIntCount_)
    {
      sprintf(LM->msg, "Too many gaps collected; will try to merge back the "
                        "%d that are less than %f", 
                        gapIntCount_ - gapKeeper_.qualifyingGaps(minGapToKeep), minGapToKeep);
      LM->Log(LM->msg);
    }
  
  Lng32 keptIntInx = 1;
  Lng32 lookaheadIntInx = 2;  // intArry_[1] can't be gap interval
  // currentInt_ is the index of the highest interval used.
  Lng32 highestIntInx = currentInt_;
  Lng32 mergeTargetIntInx;
  double gapMagnitudeOfLookahead;
  NABoolean firstTime = TRUE;   // 1st time through loop
  while (lookaheadIntInx <= highestIntInx)
    {
      // If more than 25 intervals, only log the initial interval state (output
      // too voluminous otherwise).
      if (LM->LogNeeded() && (currentInt_ <= 25 || firstTime))
        {
          firstTime = FALSE;
          logIntervals(keptIntInx, lookaheadIntInx);
        }
      gapMagnitudeOfLookahead = intArry_[lookaheadIntInx].gapMagnitude_;
      //@ZXgap -- try merging undersized non-gap intervals as well as small gap intervals
      //if (gapMagnitudeOfLookahead > 0 && gapMagnitudeOfLookahead < minGapToKeep)
      if (gapMagnitudeOfLookahead < minGapToKeep    // includes non-gap intervals...
          && !intArry_[lookaheadIntInx].highFreq_)  // ...unless they are high frequency
        {
          // Merge this small gap with an adjacent interval, unless neither
          // adjacent interval is suitable.
          mergeTargetIntInx = mergeInterval(lookaheadIntInx, keptIntInx, minGapToKeep);
          if (mergeTargetIntInx > 0)
            {
              // Also merge frequency counts if being maintained.
              if (fi_)
                fi_[lookaheadIntInx].mergeTo(fi_[mergeTargetIntInx]);
              log_mergedGaps++;
              lookaheadIntInx++;
              continue;
            }
        }

      // If the code above did not find an undersized gap and successfully
      // merge it with another, we reach this point. This code handles non-gap
      // intervals, including the null interval if it exists, large gap intervals
      // that will be kept, and small gap intervals that could not be merged.
      keptIntInx++;
      if (keptIntInx < lookaheadIntInx)
        {
          intArry_[keptIntInx] = intArry_[lookaheadIntInx];
          if (fi_)
            fi_[keptIntInx] = fi_[lookaheadIntInx];
        }
      lookaheadIntInx++;
#if 0 // Set to #if 1 for more logging info for gaps.
      if (LM->LogNeeded())
        {
          if (lookaheadIntInx-1 == highestIntInx && hasNullInterval())
            {
              sprintf(LM->msg, "Null interval moved from %d to %d",
                               lookaheadIntInx-1, keptIntInx);
              LM->Log(LM->msg);
            }
          else if (gapMagnitudeOfLookahead >= minGapToKeep)
            {
              sprintf(LM->msg, "Big gap interval moved from %d to %d",
                               lookaheadIntInx-1, keptIntInx);
              LM->Log(LM->msg);
            }
          else if (gapMagnitudeOfLookahead == 0)
            {
              sprintf(LM->msg, "Non-gap interval moved from %d to %d",
                               lookaheadIntInx-1, keptIntInx);
              LM->Log(LM->msg);
            }
          else
            {
              sprintf(LM->msg, "Unmerged small gap interval moved from %d to %d",
                               lookaheadIntInx-1, keptIntInx);
              LM->Log(LM->msg);
            }
        }
#endif
    }

#if 0 // Set to #if 1 for more logging info for gaps.
    if (LM->LogNeeded())
    {
      logIntervals(keptIntInx, lookaheadIntInx);
      sprintf(LM->msg, "Of %d original gap intervals, %d were under the gap "
                       "magnitude threshold. %d intervals were able to be "
                       "merged into adjacent intervals (some of which may have "
                       "also been undersized gap intervals)",
                       gapIntCount_, gapIntCount_ - targetGapIntervals_, log_mergedGaps);
      LM->Log(LM->msg);
      sprintf(LM->msg, "After eliminating smaller gap intervals, there are "
                       "now %d instead of %d total intervals",
                       keptIntInx + 1, highestIntInx + 1); // 1 extra for interval 0
      LM->Log(LM->msg);
      if (fi_)
        {
          FrequencyCounts FCafter;
          sumFrequencies(FCafter, keptIntInx, fi_);
          compareFC(*FCbeforePtr, FCafter);
        }
    }
#endif
  delete FCbeforePtr;

  // Update the stored interval count to reflect the removed intervals.
  currentInt_ = keptIntInx;

  GetHSContext()->checkTime("after adjusting/eliminating gap intervals");
}


/**************************************************************************/
/* METHOD:  mergeInterval()                                               */
/* PURPOSE: Merges the specified interval into an adjacent one if         */
/*          possible. It will not be merged if it cannot be done without  */
/*          producing an interval that exceeds the target bucket height   */
/*          In addition, it cannot be merged with a gap interval that we  */
/*          intend to keep, or a special high frequency interval.         */
/* PARAMS:  intervalToMerge(in) -- Index of the interval we intend to     */
/*                                 merge into another.                    */
/*          prevInterval(in)    -- Index of the interval preceding the    */
/*                                 one to merge. Won't necessarily be 1   */
/*                                 less, there may be one or more removed */
/*                                 intervals between them.                */
/*          gapThreshold(in)    -- Only allow merges with gap intervals   */
/*                                 having gap magnitude less than this.   */
/* RETCODE: Index of the interval that was merged into, or 0 if a merge   */
/*          could not be done.                                           */
/**************************************************************************/
Lng32 HSHistogram::mergeInterval(const Lng32 intervalToMerge,
                                const Lng32 prevInterval,
                                const double gapThreshold)
{
  HSLogMan *LM = HSLogMan::Instance();
  Lng32 intervalToMergeInto;
  Lng32 nextInterval = intervalToMerge + 1;

  // Set highest interval to currentInt_ less the NULL interval.
  Lng32 highestInterval = currentInt_;
  if (hasNull_) highestInterval--; // Can't merge NULL interval.
  if (intervalToMerge > highestInterval) return 0;
  // Determine whether the two candidate intervals are eligible.
  NABoolean excludeNext = nextInterval > highestInterval ||
                          intArry_[nextInterval].gapMagnitude_ >= gapThreshold ||
                          intArry_[nextInterval].highFreq_;
  NABoolean excludePrev = prevInterval < 1 ||  // actually, should never be <1
                          intArry_[prevInterval].gapMagnitude_ >= gapThreshold ||
                          intArry_[prevInterval].highFreq_;

  // Pick the adjacent interval that meets the criteria, and would have the
  // lowest resulting row count.
  if (excludeNext)
    {
      if (excludePrev)
        {
          // Neither previous nor next interval is suitable for merging; leave
          // the small gap interval as is.
          if (LM->LogNeeded())
            {
              sprintf(LM->msg,
                      "Could not merge interval %d into either adjacent interval",
                      intervalToMerge);
              LM->Log(LM->msg);
            }
          return 0;  // neither adjacent interval can be merged into
        }
      else
        // Can't merge into next, but previous is ok.
        intervalToMergeInto = prevInterval;
    }
  else if (excludePrev)
    // Can't merge into previous, but next is ok.
    intervalToMergeInto = nextInterval;
  else if (intArry_[prevInterval].rowCount_ <= intArry_[nextInterval].rowCount_)
    // Previous and next are both ok, but previous is smaller.
    intervalToMergeInto = prevInterval;
  else
    // Previous and next are both ok, but next is smaller.
    intervalToMergeInto = nextInterval;

  // Do the actual merge here, but only if combined interval would not exceed
  // our bucket height.
  Int64 combinedRowCount = intArry_[intervalToMergeInto].rowCount_ +
                           intArry_[intervalToMerge].rowCount_;
  if (combinedRowCount > originalStep_)
    {
      if (LM->LogNeeded())
        {
          sprintf(LM->msg, "Could not merge interval %d into interval %d; "
                           "combined interval too large",
                           intervalToMerge, intervalToMergeInto);
          LM->Log(LM->msg);
        }
      return 0;
    }
  else
    {
      intArry_[intervalToMergeInto].rowCount_ = combinedRowCount;
      intArry_[intervalToMergeInto].uecCount_ += intArry_[intervalToMerge].uecCount_;
      intArry_[intervalToMergeInto].squareCntSum_ += intArry_[intervalToMerge].squareCntSum_;
      intArry_[intervalToMergeInto].gapMagnitude_ = 0;
      mergeMFVs(intervalToMergeInto, intervalToMerge);
      if (intervalToMergeInto < intervalToMerge)
        intArry_[intervalToMergeInto].boundary_ = intArry_[intervalToMerge].boundary_;
      if (LM->LogNeeded())
        {
          sprintf(LM->msg, "Merged interval %d into interval %d",
                           intervalToMerge, intervalToMergeInto);
          LM->Log(LM->msg);
        }
      return intervalToMergeInto;
    }
}

/**************************************************************************/
/* METHOD:  mergeMFVs()                                                   */
/* PURPOSE: Merges the most frequent values of two intervals.             */
/* PARAMS:  to(in)   -- Index of the interval to be merged into           */
/*          from(in) -- Index of the interval being merged from.          */
/**************************************************************************/
void HSHistogram::mergeMFVs(const Lng32 to, const Lng32 from)
{
  if (intArry_[from].MFVrowCount_ > intArry_[to].MFVrowCount_)
  {
    // Interval being merged from has larger MFV.
    Int64 saveMfvCount = intArry_[to].MFVrowCount_;
    intArry_[to].MFVrowCount_ = intArry_[from].MFVrowCount_;
    intArry_[to].mostFreqVal_ = intArry_[from].mostFreqVal_;

    // Now check if MFV2 of 'from' interval larger than MFV of 'to'.
    // do the 2mfv update only the rowCount_ > uecCount_ in the 'to' interval. 
    if ( intArry_[to].rowCount_ > intArry_[to].uecCount_ ) {
       if (intArry_[from].MFV2rowCount_ > saveMfvCount)
            intArry_[to].MFV2rowCount_ = intArry_[from].MFV2rowCount_;
       else intArry_[to].MFV2rowCount_ = saveMfvCount;
    }
  }
  else
  {
    // Interval being merged into has larger MFV. Check if MFV2 of 'to' 
    // should be changed to MFV of 'from'. Do the 2mfv update only when 
    // rowCount_ > uecCount_ in the 'to' interval. 
    if ( intArry_[to].rowCount_ > intArry_[to].uecCount_ &&
         intArry_[to].MFV2rowCount_ < intArry_[from].MFVrowCount_ 
       )
         intArry_[to].MFV2rowCount_ = intArry_[from].MFVrowCount_;
  }
}

// Adjustmen of MVV and 2MFV for interval i, when the rowcount and uec of the
// interval are about to be set to newEstRow and newEstUec respectively.
void HSHistogram::adjustMFVand2MFV(const Lng32 i, double newEstRow, double newEstUec)
{
   if ( newEstRow > newEstUec && newEstUec > 1.0 ) {
     double ratio = newEstRow / getIntOrigRC(i);
     setIntMFVRowCount(i, 
                           MAXOF(Int64(newEstRow / newEstUec) + 1,              // Set MFV to the max of the lower bound of the MFV
                                 (Int64)((double)getIntOrigMFV(i)*ratio) // and the scaled original MFV (by the ratio 
                                                                         // of increase in rowcount of the interval).
                                )
                      );
     setIntMFV2RowCount(i, (Int64)((double)getIntMFVRowCount(i)/2.0));   // Following the Zipf's law, 2MFV set to half of the MFV.
   } else
     if ( newEstUec == 1.0 )
       setIntMFV2RowCount(i, 0);
}

void HSHistogram::maintainEndIntervalForIUS(float avgRCPerInterval, Lng32 intNum)
{
}

// Hash function used for jitLogThresholdHash.
static ULng32 hashString(const NAString& str) { return str.hash(); }

THREAD_P NABoolean HSGlobalsClass::performISForMC_ = FALSE;

HSGlobalsClass::HSGlobalsClass(ComDiagsArea &diags)
  : catSch(new(STMTHEAP) NAString(STMTHEAP)),
    isHbaseTable(FALSE),
    isHiveTable(FALSE),
    hasOversizedColumns(FALSE),
    user_table(new(STMTHEAP) NAString(STMTHEAP)),
    numPartitions(0),
    hstogram_table(new(STMTHEAP) NAString(STMTHEAP)),
    hsintval_table(new(STMTHEAP) NAString(STMTHEAP)),
    hsperssamp_table(new(STMTHEAP) NAString(STMTHEAP)),
    hssample_table(new(STMTHEAP) NAString(STMTHEAP)),
    statstime(new(STMTHEAP) NAString(STMTHEAP)),

    externalSampleTable(FALSE),
    tableType(UNKNOWN_TYPE),tableFormat(UNKNOWN),

    actualRowCount(0), sampleRowCount(0),
    rowChangeCount(-1),   // -1 indicates not determined yet
    intCount(0), dupGroup(0), 
    errorFile(STMTHEAP),

    groupCount(0), singleGroupCount(0), singleGroup(NULL), multiGroup(NULL),
    parserError(ERROR_SYNTAX), errorCount(0), diagsArea(diags),
    sampleTableUsed(FALSE),
    samplingUsed(FALSE),

    optFlags(0), sampleOption(new(STMTHEAP) NAString(STMTHEAP)),
    sampleValue1(0), sampleValue2(0), sampleOptionUsed(FALSE),
    unpartitionedSample(FALSE), objDef(NULL), 
    statsNeeded_(TRUE),
    
    // histogram automation
    sampleSeconds(0), columnSeconds(0), samplePercentX100(0),
    allMissingStats(FALSE),
    // other
    requestedByCompiler(FALSE),

    //iusSampleInMem(NULL),
    iusSampleDeletedInMem(NULL),
    iusSampleInsertedInMem(NULL),
    sampleIExists_(FALSE),
    PST_IUSrequestedSampleRows_(NULL),
    PST_IUSactualSampleRows_(NULL),
    sampleRateAsPercetageForIUS(0),
    minRowCtPerPartition_(-1),
    sample_I_generated(FALSE),
    PSRowUpdated(FALSE),
    jitLogThreshold(0),
    stmtStartTime(0),
    jitLogOn(FALSE),
    isUpdatestatsStmt(FALSE),
    maxCharColumnLengthInBytes(ActiveSchemaDB()->getDefaults().
               getAsLong(USTAT_MAX_CHAR_COL_LENGTH_IN_BYTES)),
    hbaseCacheSizeCQDsSet_(FALSE),
    hiveMaxStringLengthCQDSet_(FALSE)
  {
    // Must add the context first in the constructor.
    contID_ = AddHSContext(this);

    // Save parserflags
    SQL_EXEC_GetParserFlagsForExSqlComp_Internal(savedParserFlags);

    // Special SQLParser flags to deal with namespaces and funny signs like '@'
    // and security
    SQL_EXEC_SetParserFlagsForExSqlComp_Internal(
      //dmALLOW_SPECIALTABLETYPE | dmALLOW_PHONYCHARACTERS | dmINTERNAL_QUERY_FROM_EXEUTIL);
      dmINTERNAL_QUERY_FROM_EXEUTIL);

    // On first ustat statement of session, allocate and fill the static hash
    // table of table-specific elapsed-time thresholds.
    if (!jitLogThresholdHash)
      {
        jitLogThresholdHash = new(CTXTHEAP) JitLogHashType(hashString, 77, TRUE, CTXTHEAP);
        initJITLogData();
      }

    performISForMC_ = FALSE;
  }

HSGlobalsClass::~HSGlobalsClass()
{
  // If this was an IUS execution, make sure the row for the source table in
  // SB_PERSISTENT_SAMPLES is modified to reflect that an IUS operation is no
  // longer in progress.
  if (PSRowUpdated)
    end_IUS_work();

  // Used in end_IUS_work(), must call it first.
  NADELETEBASIC(PST_IUSrequestedSampleRows_, STMTHEAP);
  NADELETEBASIC(PST_IUSactualSampleRows_, STMTHEAP);

  // reset the parser flags that were set in the constructor
  SQL_EXEC_ResetParserFlagsForExSqlComp_Internal(savedParserFlags);

  HSColGroupStruct *group = singleGroup;
  while (group) 
  {
    if (group->groupHist) delete group->groupHist; // Delete single object from ColHeap
    group = group->next;
  }
  group = multiGroup;
  while (group)
  {
    if (group->groupHist) delete group->groupHist; // Delete single object from ColHeap
    group = group->next;
  }

  // If just-in-time logging was activated, turn logging back off.
  if (jitLogOn)
    {
      HSLogMan* LM = HSLogMan::Instance();
      sprintf(LM->msg, "***** End of Just-In-Time logging session for table %s *****\n",
              user_table->data());
      LM->Log(LM->msg);
      LM->StopLog();
    }

  // Must delete the context last in the destructor.
  DeleteHSContext(contID_);
}


// -----------------------------------------------------------------------
// Initialize stats schema for Hive or native HBase tables if needed
// -----------------------------------------------------------------------
Lng32 HSGlobalsClass::InitializeStatsSchema()
  {
    Lng32 retcode = 0;

                                              /*==============================*/
                                              /*   CREATE HIVE STATS SCHEMA   */
                                              /*==============================*/
    if (isHiveCat(objDef->getCatName()))
      {
        HSTranMan *TM = HSTranMan::Instance(); // Must have transaction around this.
        TM->Begin("Create schema for hive stats.");
        NAString ddl = "CREATE SCHEMA IF NOT EXISTS ";
        ddl.append(HIVE_STATS_CATALOG).append('.').append(HIVE_STATS_SCHEMA).
            append(" AUTHORIZATION DB__ROOT");
        retcode = HSFuncExecQuery(ddl, -UERR_INTERNAL_ERROR, NULL,
                                  "Creating schema for Hive statistics", NULL,
                                  NULL);
        HSHandleError(retcode);
        TM->Commit(); // In case if there is an error, the commit will log the error (if
                      // ULOG is enabled. Otherwise, the method will commit the tranaction.
      }
                                              /*=====================================*/
                                              /*   CREATE HBASE STATS SCHEMA         */
                                              /*   typically as trafodion.hbasestats */
                                              /*=====================================*/
    if (isNativeHbaseCat(objDef->getCatName()))
      {
        HSTranMan *TM = HSTranMan::Instance(); // Must have transaction around this.
        TM->Begin("Create schema for native hbase stats.");
        NAString ddl = "CREATE SCHEMA IF NOT EXISTS ";
        ddl.append(HBASE_STATS_CATALOG).append('.').append(HBASE_STATS_SCHEMA).
            append(" AUTHORIZATION DB__ROOT");
        retcode = HSFuncExecQuery(ddl, -UERR_INTERNAL_ERROR, NULL,
                                  "Creating schema for native HBase statistics", NULL,
                                  NULL);
        HSHandleError(retcode);
        TM->Commit(); // In case if there is an error, the commit will log the error (if
                      // ULOG is enabled. Otherwise, the method will commit the tranaction.
      }
                                              /*==============================*/
                                              /*    CREATE HISTOGRM TABLES    */
                                              /*==============================*/
    retcode = CreateHistTables(this);
    HSHandleError(retcode);

    return retcode;
  }



// -----------------------------------------------------------------------
//
// -----------------------------------------------------------------------
Lng32 HSGlobalsClass::Initialize()
  {
    Lng32 retcode = 0;
    NAString query;
    HSCursor cursor;
    Int64 xSampleSet = 0;
    char intStr[30], intStr2[30];
    Int64 inserts, deletes, updates;
    HSLogMan *LM = HSLogMan::Instance();
    HSGlobalsClass *hs_globals = GetHSContext();

    // Seed the random number generator used in quicksort().
    srand(time(NULL));
    // Set the default catalog names for Hive and HBase.
    if (defaultHiveCatName == NULL)
       defaultHiveCatName = new (GetCliGlobals()->exCollHeap()) NAString("");
    else
      (*defaultHiveCatName) = "";

    CmpCommon::getDefault(HIVE_CATALOG, (*defaultHiveCatName), FALSE);
    (*defaultHiveCatName).toUpper();

    if (defaultHbaseCatName == NULL)
       defaultHbaseCatName = new (GetCliGlobals()->exCollHeap()) NAString("");
    else
      (*defaultHbaseCatName) = "";

    CmpCommon::getDefault(HBASE_CATALOG, (*defaultHbaseCatName), FALSE);
    (*defaultHbaseCatName).toUpper();

    // initialize stats schema if this is a Hive or native HBase table
    retcode = InitializeStatsSchema();
    HSHandleError(retcode);    

                                             /*==============================*/
                                             /*   CREATE UNDOCUMENTED VIEW   */
                                             /*==============================*/
    if (optFlags & VIEWONLY_OPT)
      {
        if (isHiveCat(objDef->getCatName()))
          {
            *CmpCommon::diags() << DgSqlCode(-UERR_NO_VIEWONLY) << DgString0("hive");
            return -1;
          }
        else if (isHbaseCat(objDef->getCatName()))
          {
            *CmpCommon::diags() << DgSqlCode(-UERR_NO_VIEWONLY) << DgString0("HBase");
            return -1;
          }
        LM->Log("\tCREATE HISTOGRAM VIEW");
        retcode = CreateHistView(this);
        HSHandleError(retcode);
        return 0;
      }
                                              /*==============================*/
                                              /*=      DETERMINE #ROWS       =*/
                                              /*==============================*/
    Int64 userSuppliedRowCount = actualRowCount;

    sample_I_generated = FALSE;

    LM->StartTimer("getRowCount()");
    Int32 errorCode = 0;
    Int32 breadCrumb = 0;
    if ((optFlags & ROWCOUNT_OPT) == 0)
      actualRowCount = objDef->getRowCount(currentRowCountIsEstimate_,
                                           inserts, deletes, updates,
                                           numPartitions,
                                           minRowCtPerPartition_,
                                           errorCode /* out */,
                                           breadCrumb /* out */,
                                           optFlags & (SAMPLE_REQUESTED | IUS_OPT));
    else
      {
        // skip the potentially expensive step of determining the row
        // count, if it was specified by the user
        actualRowCount = userSuppliedRowCount;
        currentRowCountIsEstimate_ = FALSE;
        inserts = deletes = updates = 0;
        numPartitions = 1;
        minRowCtPerPartition_ = actualRowCount;
        if (LM->LogNeeded())
          {
            convertInt64ToAscii(actualRowCount, intStr);
            sprintf(LM->msg, "\t\t\tUser provided rowcount: rows=%s", intStr);
            LM->Log(LM->msg);
          }
      }
    LM->StopTimer();
    if (LM->LogNeeded())
      {
        sprintf(LM->msg, "\tcurrentRowCountIsEstimate_=%d from getRowCount()", currentRowCountIsEstimate_);
        LM->Log(LM->msg);
        sprintf(LM->msg, "\terrorCode=%d, breadCrumb=%d", errorCode, breadCrumb);
        LM->Log(LM->msg);
        if (errorCode == HBC_ERROR_ROWCOUNT_EST_EXCEPTION)
          {
            const char * jniErrorStr = HSFuncGetJniErrorStr();
            if (strlen(jniErrorStr) > 0)
              {
                LM->Log("\tJNI exception info:");
                LM->Log(jniErrorStr);
              }
          }
      }

    if (errorCode == HBC_ERROR_ROWCOUNT_EST_EXCEPTION)
      {
        *CmpCommon::diags() << DgSqlCode(-UERR_BAD_EST_ROWCOUNT) << DgInt0(errorCode) 
                            << DgInt1(breadCrumb) << DgString0(HSFuncGetJniErrorStr());
        return -1;
      }
    else if (errorCode)
      {
        *CmpCommon::diags() << DgSqlCode(-UERR_BAD_EST_ROWCOUNT) << DgInt0(errorCode) 
                            << DgInt1(breadCrumb) << DgString0("");
        return -1;
      }

    // We only allow an estimate when sampling, and then only if the
    // estimated row count is at least ustat_min_estimate_for_rowcount (CQD),
    // because estimation error is high for small or fragmented tables.
    // Otherwise a SELECT COUNT(*) is used to get the actual row count in
    // place of the estimate, unless the user supplied his own row count..
    if (currentRowCountIsEstimate_ && !(optFlags & CLEAR_OPT))
      {
        if (convertInt64ToDouble(actualRowCount) <   // may be 0 (no estimate) or -1 (error doing estimation)
                     CmpCommon::getDefaultNumeric(USTAT_MIN_ESTIMATE_FOR_ROWCOUNT))
          {
            if (LM->LogNeeded() && actualRowCount > 0)
            {
              sprintf(LM->msg, "Estimated row count " PF64 " rejected (below size threshhold).",
                      actualRowCount);
              LM->Log(LM->msg);
            }
            LM->StartTimer("Execute query to get row count");
            query  = "SELECT COUNT(*) FROM ";
            query += getTableName(user_table->data(), nameSpace);
            query += " FOR SKIP CONFLICT ACCESS";
            retcode = cursor.fetchNumColumn(query, NULL, &actualRowCount);
            LM->StopTimer();
            HSHandleError(retcode);
            currentRowCountIsEstimate_ = FALSE;
            if (LM->LogNeeded())
              {
                convertInt64ToAscii(actualRowCount, intStr);
                sprintf(LM->msg, "\n\t\tUsing select count(*): rows=%s", intStr);
                LM->Log(LM->msg);
              }
          }
      }
    else  // row change counts won't be accurate if estimation was used
      rowChangeCount = inserts + deletes + updates;

    HS_ASSERT(actualRowCount >= 0);

    Int64 youWillLikelyBeSorry = 
      ActiveSchemaDB()->getDefaults().getAsDouble(USTAT_YOULL_LIKELY_BE_SORRY);
    if ((actualRowCount >= youWillLikelyBeSorry) &&
       !(optFlags & CLEAR_OPT) &&
       !(optFlags & SAMPLE_REQUESTED) &&
       !(optFlags & IUS_OPT))
      {
        // attempt to do UPDATE STATISTICS on a big table without sampling,
        // which could take a really long time
        if ((optFlags & NO_SAMPLE) == 0)  // if explicit NO SAMPLE is missing
          {
            // raise an error on the chance that omitting the SAMPLE clause
            // was accidental
            HSFuncMergeDiags(-UERR_YOU_WILL_LIKELY_BE_SORRY);
            retcode = -1;
            HSHandleError(retcode);
          }
      }


                                             /*===================================*/
                                             /*=  DETERMINE "NECESSARY" COLUMNS  =*/
                                             /*===================================*/

    // Determining which columns are implied by the NECESSARY clause, if present,
    // has to be deferred until this point so the row count is known (determined
    // immediately above). The row count is used in determining obsolescence of
    // existing histograms.
    if (optFlags & NECESSARY_OPT)
      {
        // Sporadic failures of AddNecessaryColumns() have been observed,
        // so we use retry-with-delay to try to circumvent the problem. Note that
        // the retry limit is specified by a CQD that is specific to this fn rather
        // than the one used for other ustat retries, although the same delay is used.
        Int32 centiSecsDelay = getDefaultAsLong(USTAT_RETRY_DELAY);
        Int32 retryLimit = getDefaultAsLong(USTAT_RETRY_NEC_COLS_LIMIT);

        // Save the state of the group lists, so they can be restored in between retries.
        HSColGroupStruct* oldSingleGroup = singleGroup;
        HSColGroupStruct* oldMultiGroup = multiGroup;
        Lng32 oldGroupCount = groupCount;

        ComDiagsArea& diagsArea = hs_globals->diagsArea;
        Lng32 diagMark = diagsArea.mark();

        NABoolean groupStateOK = TRUE;
        Int32 retry;
        for (retry = 0; retry <= retryLimit && groupStateOK; retry++)
          {
            retcode = AddNecessaryColumns();
            if (retcode < 0)
              {
                // An error occurred.
                if (retry < retryLimit &&
                    (groupStateOK = removeGroups(groupCount - oldGroupCount,  // intentional = (not ==)
                                                 oldSingleGroup, oldMultiGroup)))
                  {
                    DELAY_CSEC(centiSecsDelay); // short delay before next attempt
                    if (LM->LogNeeded())
                      LM->Log("!!! Retrying AddNecessaryColumns() !!!");
                  }
              }
            else
              break;  // successful execution, exit retry loop
          }

        // If we found errors, but retried successfully, rewind past the errors
        // on the failed attempts. Otherwise, error messages will be displayed
        // in spite of successful execution.
        if (retry > 0 && retcode >= 0)
          diagsArea.rewind(diagMark, TRUE);
        if (retcode == HS_EOF)
          retcode = 0;
        HSHandleError(retcode);
      }

                                             /*=============================*/
                                             /*=  SEE IF NEW STATS NEEDED  =*/
                                             /*=============================*/
    // This can't be done before checking columns implied by NECESSARY (above).
    if (groupCount == 0 ||                        // no grouplist specified
        (optFlags & CLEAR_OPT))                   // CLEAR option used
      {
        statsNeeded_ = FALSE;                     // do not collect statistics
        return 0;
      }
                                             /*==============================*/
                                             /*=  DETERMINE SAMPLING RATIO  =*/
                                             /*==============================*/
    if (actualRowCount > 0)
      {
        if (CmpCommon::getDefault(USTAT_USE_BACKING_SAMPLE) == DF_ON)
          {
            if (optFlags & SAMPLE_REQUESTED)
              LM->Log("SAMPLE OPTION IGNORED. USING BACKING SAMPLE TABLE.");
            sampleRowCount = actualRowCount / 100;  //@ZXtemp -- assume 1% sample for now
          }
        //If the number of rows are less than the minimum for which we do sampling,
        //then ignore the sampling option. The table is small enough for a full
        //table scan. We make an exception for this if a persistent sample (IUS)
        //was requested.
        else if ((optFlags & SAMPLE_REQUESTED) &&
            !(optFlags & IUS_PERSIST) &&
            (convertInt64ToDouble(actualRowCount) < getMinRowCountForSample()))
          {
            LM->Log("SAMPLE OPTION IGNORED. SMALL TABLE SPECIFIED");
          }
        else
          {
            // Set sample size and percent based on sampling options.
            switch (optFlags & SAMPLE_REQUESTED)
              {
                case SAMPLE_BASIC_0:         /*==    BASIC: NO OPTIONS     ==*/
                  // READ BASE TABLE USING A RATIO OF HIST_DEFAULT_SAMPLE_RATIO
                  // BUT NO MORE THAN HIST_DEFAULT_SAMPLE_MAX ROWS.  THE MIN SIZE
                  // OF SAMPLE IS THE NUMBER OF PARTITIONS OF THE TABLE * 2.
                {
                  if (CmpCommon::getDefault(USTAT_USE_SLIDING_SAMPLE_RATIO) == DF_ON)
                    xSampleSet = getDefaultSlidingSampleSize(actualRowCount);
                  else
                    xSampleSet = getDefaultSampleSize(actualRowCount);
                  xSampleSet = MAXOF(xSampleSet, numPartitions*2);

                  // multiply by 100.0001 instead of 100 so that rounding
                  // errors are limited.
                  sampleTblPercent = convertInt64ToDouble(xSampleSet) / 
                                     convertInt64ToDouble(actualRowCount) * 100.0001;
                      
                  if (sampleTblPercent < 100.0)
                    {
                       sampleOptionUsed = TRUE;                     
                       sampleRowCount = xSampleSet;
                    }
                  break;
                }
              
                case SAMPLE_BASIC_1:         /*==     BASIC: ROWS ONLY     ==*/
                // READ NUMBER OF ROWS SPECIFIED - NO MORE THAN ACTUAL ROWCOUNT
                {
                  if ((optFlags & ROWCOUNT_OPT) &&
                      (sampleValue1 > actualRowCount))
                    {
                      HSFuncMergeDiags(- UERR_INVALID_OPTION,
                                       "SAMPLE ROWS",
                                       "less than or equal to ROWCOUNT");
                      retcode = -1;
                      HSHandleError(retcode);
                    }
                  else
                    {
                      xSampleSet = MINOF(sampleValue1, actualRowCount);
                      // multiply by 100.0001 instead of 100 so that rounding
                      // errors are limited.
                      sampleTblPercent = convertInt64ToDouble(xSampleSet) / 
                                         actualRowCount * 100.0001;

                      if (sampleTblPercent < 100)
                        {
                          sampleOptionUsed = TRUE;
                               
                          //estimate the number of rows in the temporary sample
                          //table. This will help determine how many partitions
                          //to create for MX tables.
                          sampleRowCount = xSampleSet;
                        }
                    }
                  break;
                }
              
                case SAMPLE_RAND_1:          /*==     RANDOM: PERCENT      ==*/
                {
                  sampleTblPercent = convertInt64ToDouble(sampleValue1) / 10000;
                  xSampleSet = (Int64)ceil(convertInt64ToDouble(actualRowCount) * 
                               sampleTblPercent / 100) ;
	      
                  if (sampleTblPercent < 100 || (optFlags & IUS_PERSIST))
                    {
                      sampleOptionUsed = TRUE;
                           
                      //estimate the number of rows in the temporary sample
                      //table. This will help determine how many partitions
                      //to create for MX tables.
                      sampleRowCount = xSampleSet;
                    }
                  break;
                }
              
                case SAMPLE_RAND_2:          /*==RANDOM: PERCENT w/ CLUSTER==*/
                {
                  sampleTblPercent = convertInt64ToDouble(sampleValue1) / 10000;
                  xSampleSet = (Int64)ceil(convertInt64ToDouble(actualRowCount) * 
                               sampleTblPercent / 100) ;
                  if (sampleTblPercent < 100)
                    {
                      sampleOptionUsed = TRUE;
                        
                      //estimate the number of rows in the temporary sample
                      //table. This will help determine how many partitions
                      //to create for MX tables.
                      sampleRowCount = xSampleSet;
                    }
                  break;
                }
              
                case SAMPLE_PERIODIC:        /*==        PERIODIC          ==*/
                {
                  if ((optFlags & ROWCOUNT_OPT) &&
                      (sampleValue2 > actualRowCount))
                    {
                      HSFuncMergeDiags(- UERR_INVALID_OPTION,
                                       "SAMPLE PERIOD",
                                       "less than or equal to ROWCOUNT");
                      retcode = -1;
                      HSHandleError(retcode);
                    }
                  else
                    {
                      sampleOptionUsed = TRUE;

                      //estimate the number of rows in the temporary sample
                      //table. This will help determine how many partitions
                      //to create for MX tables.
                      sampleRowCount = (Int64)ceil((1-((double)(sampleValue2 - sampleValue1) / (double)sampleValue2)) * (double)actualRowCount);
                    }
                  break;
                }
              
                default:                             /*== NO SAMPLING: 100%==*/
                {
                }
              }
            if (sampleTblPercent > 100) sampleTblPercent=100;
            if (sampleTblPercent < 0)   sampleTblPercent=0;
            samplePercentX100 = (short) (sampleTblPercent * 100); 
               // saved for automation: percent * 100.  
          }
      }
    else
      {                                           /* empty table             */
        statsNeeded_ = FALSE;                     /*do not collect statistics*/
      }

                                             /*==============================*/
                                             /*=    DETERMINE #INTERVALS    =*/
                                             /*==============================*/
    if (optFlags & INTERVAL_OPT)
      {
        if (LM->LogNeeded())
          {
            sprintf(LM->msg, "\t\tUSER REQUESTED %d INTERVALS", intCount);
            LM->Log(LM->msg);
          }
      }
    else
      {
        intCount = MAXOF(CmpCommon::getDefaultLong(HIST_DEFAULT_NUMBER_OF_INTERVALS), 1);
        intCount = MINOF(intCount, HS_MAX_INTERVAL_COUNT);

        if (LM->LogNeeded())
          {
            sprintf(LM->msg, "\t\tSYSTEM GENERATED %d INTERVALS", intCount);
            LM->Log(LM->msg);
          }
      }

    if (LM->LogNeeded())
      {
        convertInt64ToAscii(actualRowCount, intStr);
        convertInt64ToAscii(sampleRowCount, intStr2);
        sprintf(LM->msg, "\tTotal #rows= %s, sample rows=%s, IntervalCount = %d", 
                intStr, intStr2, intCount);
        LM->Log(LM->msg);
      }


    return 0;
  }

// *****************************************************************************
// *                                                                           *
// * Function: HSGlobalsClass::isAuthorized                                    *
// *                                                                           *
// *   This member function determines if a user has authority to perform:     *
// *     UPDATE STATISTICS or                                                  *
// *     SHOWSTATS                                                             *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <isShowStats>                NABoolean                          In       *
// *    TRUE if request is coming from a showstat request                      *
// *                                                                           *
// * returns:                                                                  *
// *   TRUE - current user has privilege                                       *
// *   FALSE - current user has no privilege or unexpected error occurred      *
// *                                                                           *
// * ComDiags is loaded with any unexpected errors                             *
// *                                                                           *
// *****************************************************************************
NABoolean HSGlobalsClass::isAuthorized(NABoolean isShowStats)
{
  if (!CmpCommon::context()->isAuthorizationEnabled())
    return TRUE;

  HS_ASSERT (objDef->getNATable());

  // Let keep track of how long authorization takes
  HSLogMan *LM = HSLogMan::Instance();
  LM->LogTimeDiff("Entering: HSGlobalsClass::isAuthorized");

  // Root user is authorized for all operations.  
  NABoolean authorized = ComUser::isRootUserID();

  // If the current user owns the target object, they have full DDL authority 
  // on the object.
  if (!authorized)
    {
      if (LM->LogNeeded())
        {
          sprintf(LM->msg, "Authorization: check for object owner");
          LM->Log(LM->msg);
        }

      Int32 objOwner = objDef->getNATable()->getOwner();
      authorized = (ComUser::getCurrentUser() == objDef->getNATable()->getOwner());
    }

  // See if user has MANAGE_STATISTICS component priv
  NAString privMgrMDLoc = 
         NAString(CmpSeabaseDDL::getSystemCatalogStatic()) +
         NAString(".\"") +
         NAString(SEABASE_PRIVMGR_SCHEMA) +
         NAString("\"");
  PrivMgrComponentPrivileges componentPrivileges(std::string(privMgrMDLoc.data()),&diagsArea);

  if (!authorized)
    {

      if (LM->LogNeeded())
        {
          sprintf(LM->msg, "Authorization: check for MANAGE_STATISTICS component privilege");
          LM->Log(LM->msg);
        }

      authorized = (componentPrivileges.hasSQLPriv(ComUser::getCurrentUser(),
                                                   SQLOperation::MANAGE_STATISTICS,
                                                   true));
    }

  // For SHOW STATS command, check for additional privileges
  if (!authorized && isShowStats)
    {
      // check for SHOW component privilege
      if (LM->LogNeeded())
        {
          sprintf(LM->msg, "Authorization: check for SHOW component privilege");
          LM->Log(LM->msg);
        }

      authorized = (componentPrivileges.hasSQLPriv(ComUser::getCurrentUser(),
                                                    SQLOperation::SHOW,
                                                    true));
     }
   
   // Allow operation if requester has SELECT priv
   if (!authorized)
     {
       if (LM->LogNeeded())
         {
           sprintf(LM->msg, "Authorization: check for SELECT object privilege");
           LM->Log(LM->msg);
         }

       // check for SELECT privilege
       PrivMgrUserPrivs *privs = objDef->getNATable()->getPrivInfo();
       if (privs == NULL)
         {
           *CmpCommon::diags() << DgSqlCode(-1034);
            authorized = FALSE;
         }

       // Requester must have at least select privilege
       else if ( privs->hasSelectPriv() )
         authorized = TRUE;
       else
         {
           *CmpCommon::diags()
            << DgSqlCode( -4481 )
            << DgString0( "SELECT or MANAGE_STATISTICS" )
            << DgString1( objDef->getNATable()->getTableName().getQualifiedNameAsAnsiString() );
            authorized = FALSE;
          }
       }

   LM->LogTimeDiff("Exiting: HSGlobalsClass::isAuthorized");
   return authorized;
}

// Read the file, if present, containing table names and their execution
// elapsed time thresholds. Store the thresholds in a hash table keyed by
// the fully-qualified table names.
void HSGlobalsClass::initJITLogData()
{
  char* sqlogs = getenv("TRAF_LOG");
  if (!sqlogs)
    return;
  
  NAString filePath = sqlogs;
  filePath.append("/jit_ulog_params");
  FILE* jitParamFile = fopen(filePath.data(), "r");
  if (!jitParamFile)
    return;

  char buf[200];
  double* elapsedTimeThresholdPtr;
  Int32 itemsScanned = 0;
  while (itemsScanned != EOF)
    {
      // Use context heap instead of statement heap for keys and values placed
      // in the hash table. It is used across multiple statements.
      elapsedTimeThresholdPtr = new(CTXTHEAP) double;
      itemsScanned = fscanf(jitParamFile, "%s %lf\n",
                            buf, elapsedTimeThresholdPtr);
      if (itemsScanned == 2)
        {
          jitLogThresholdHash->insert(new(CTXTHEAP) NAString(buf),
                                      elapsedTimeThresholdPtr);
        }
      else
        {
          NADELETEBASIC(elapsedTimeThresholdPtr, CTXTHEAP);
        }
    }

#if 0
  HSLogMan *LM = HSLogMan::Instance();
  if (LM->LogNeeded())
    {
      NAHashDictionaryIterator<NAString, double> iter(*jitLogThresholdHash);
      NAString* key;
      double* value;
      for (CollIndex i=0; i<iter.entries(); i++) 
        {
          iter.getNext(key, value); 
          sprintf(LM->msg, "Threshold for %s is %f", key->data(), *value);
          LM->Log(LM->msg);
        } 
    }
#endif
}

// Activate logging in response to detecting that the ustat statement has run
// far longer than expected for the source table.
// Params:
//   checkPointName -- text describing the point at which the logging is kicking in.
//   elapsedSeconds -- time the stmt had been running when logging was activated.
void HSGlobalsClass::startJitLogging(const char* checkPointName, Int64 elapsedSeconds)
{
  HSLogMan *LM = HSLogMan::Instance();

  // Turn logging on 
  LM->StartLog(TRUE);
  jitLogOn = TRUE;

  // Write introductory information to log; name of table and columns being
  // processed, elapsed time and the threshold value it exceeded, name describing
  // the point at which logging was activated.
  sprintf(LM->msg,
          "***** Just-In-Time logging activated due to excessive elapsed time for table %s *****",
          user_table->data());
  LM->Log(LM->msg);
  sprintf(LM->msg, "Activation triggered %s", checkPointName); 
  LM->Log(LM->msg);
  sprintf(LM->msg, "Current elapsed time = " PF64 " seconds", elapsedSeconds); 
  LM->Log(LM->msg);
  sprintf(LM->msg, "Tolerance threshold = " PF64 " seconds", (Int64)jitLogThreshold); 
  LM->Log(LM->msg);
  LM->Log("Column groups being processed:");
  HSColGroupStruct* group = singleGroup;
  while (group)
    {
      sprintf(LM->msg, "    %s (%s)",
              group->colNames->data(), SortStateName[group->state]);
      LM->Log(LM->msg);
      group = group->next;
    }
  group = multiGroup;
  while (group)
    {
      sprintf(LM->msg, "    %s", group->colNames->data());
      LM->Log(LM->msg);
      group = group->next;
    }
}

// The optimal degree of parallelism for a LOAD or UPSERT is the number of
// partitions of the source table. This forces that by setting the cqd
// PARALLEL_NUM_ESPS. Note that when the default for AGGRESSIVE_ESP_ALLOCATION_PER_CORE
// is permanently changed to 'ON', we may be able to remove this.
// tblDef -- ptr to HSTableDef from which to get the catalog and schema name of
//           the source table.
// tblName -- unqualified name of the source table. If NULL, then the source
//            table is the one represented by tblDef.
// Returns TRUE if the cqd was successfully set, FALSE otherwise. If TRUE is returned,
// then resetEspParallelism() may be called to reset the cqd.
static NABoolean setEspParallelism(HSTableDef* tblDef, const char* tblName = NULL)
{
  HSLogMan *LM = HSLogMan::Instance();
  Lng32 retcode = 0;
  Lng32 numPartitions = 0;
  if (!tblName)
    numPartitions = tblDef->getNumPartitions();
  else
    {
      HSCursor cursor;
      NAString numPartitionsQuery;
      numPartitionsQuery.append("select t.num_salt_partns from \"_MD_\".OBJECTS O, \"_MD_\".TABLES T where o.catalog_name = '")
                        .append(tblDef->getCatName())
                        .append("' and o.schema_name = '")
                        .append(tblDef->getSchemaName())
                        .append("' and o.object_name = '")
                        .append(tblName)
                        .append("' and o.object_uid = t.table_uid");
      retcode = cursor.fetchNumColumn(numPartitionsQuery, &numPartitions, NULL);
      if (retcode != 0)
        {
          if (LM->LogNeeded())
            {
              sprintf(LM->msg, "PARALLEL_NUM_ESPS not set; query to get # partitions received sqlcode %d", retcode);
              LM->Log(LM->msg);
            }
          return FALSE;
        }
    }

  NABoolean espCQDUsed = FALSE;
  if (numPartitions > 1)
    {
      char temp[25];
      sprintf(temp, "'%d'", numPartitions);
      NAString espsCQD = "CONTROL QUERY DEFAULT PARALLEL_NUM_ESPS ";
      espsCQD += temp;
      retcode = HSFuncExecQuery(espsCQD);
      if (retcode < 0)
        {
          if (LM->LogNeeded())
            {
              sprintf(LM->msg, "cqd statement to set PARALLEL_NUM_ESPS failed with %d", retcode);
              LM->Log(LM->msg);
            }
        }
      else
        {
          espCQDUsed = TRUE;
          if (LM->LogNeeded())
            {
              sprintf(LM->msg, "PARALLEL_NUM_ESPS was set to %d", numPartitions);
              LM->Log(LM->msg);
            }
        }
    }
  else if (LM->LogNeeded())
    {
      sprintf(LM->msg, "PARALLEL_NUM_ESPS not set; # partitions reported as %d", numPartitions);
      LM->Log(LM->msg);
    }

  return espCQDUsed;
}

// Reset the cqd PARALLEL_NUM_ESPS. This is the other bookend for setEspParallelism(),
// which returns TRUE if the cqd is actually set within that function. If so, this
// function can be called to reset it.
static void resetEspParallelism()
{
  HSLogMan *LM = HSLogMan::Instance();
  Lng32 retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT PARALLEL_NUM_ESPS RESET");
  if (retcode)
  {
    HSLogMan *LM = HSLogMan::Instance();
    if (LM->LogNeeded())
      {
        HSLogMan *LM = HSLogMan::Instance();
        sprintf(LM->msg, "cqd statement to reset PARALLEL_NUM_ESPS returned %d", retcode);
        LM->Log(LM->msg);
      }
  }
}

/**********************************************/
/* METHOD: createSampleOption()               */
/* PURPOSE: create text for use in sample     */
/*          query.                            */
/* INPUT:  sampleType - the sample option type*/
/*           set when parsing.                */
/*         samplePercent - %                  */
/*         sampleValue1, sampleValue2 - set   */
/*          when parsing.                     */
/* OUTPUT: sampleOpt - sample text option.    */
/**********************************************/
void createSampleOption(Lng32 sampleType, double samplePercent, NAString &sampleOpt,
                        Int64 sampleValue1, Int64 sampleValue2)
{
  char floatStr[30], intStr[30];
  switch (sampleType)
  {
    case SAMPLE_BASIC_0:         // BASIC: NO OPTIONS
    case SAMPLE_BASIC_1:         // BASIC: ROWS ONLY
    case SAMPLE_RAND_1:          // RANDOM: PERCENT
    case SAMPLE_RAND_2:          // RANDOM: PERCENT W/ CLUSTERS
      sprintf(floatStr, "%f", samplePercent);
      sampleOpt  = " SAMPLE RANDOM ";
      sampleOpt += floatStr;
      sampleOpt += " PERCENT ";
              
      if (sampleType == SAMPLE_RAND_2 && sampleValue2 > 0) // Only set for SAMPLE_RAND_2
      {
        convertInt64ToAscii(sampleValue2, intStr);
        sampleOpt += " CLUSTERS OF ";
        sampleOpt += intStr;
        sampleOpt += " BLOCKS ";
      }
      break;
            
    case SAMPLE_PERIODIC:        // PERIODIC
      sampleOpt  = " SAMPLE PERIODIC ";
      convertInt64ToAscii(sampleValue1, intStr);
      sampleOpt += intStr;
      sampleOpt += " ROWS EVERY ";
      convertInt64ToAscii(sampleValue2, intStr);
      sampleOpt += intStr;
      sampleOpt += " ROWS ";
      break;
    default:                     // Invalid option.
      HS_ASSERT(FALSE);
      break;
  }
}


/***********************************************/
/* METHOD:  HSSample makeTableName() member    */
/* PURPOSE: Creates a unique sample table name */
/*          based on source table UID and      */
/*          current time.                      */
/* RETCODE: none.                              */
/***********************************************/
void HSSample::makeTableName(NABoolean isPersSample)
{
  NABoolean unpartitionedSample = FALSE;
  if (objDef->getObjectFormat() == SQLMX)
  {
    //The naming convention used for the temporary sample table is 'TRAF_SAMPLE_'
    //followed by the object_uid of the source table and a portion of 
    //the timestamp. The object_uid ensures no collisions with update stats
    //for other tables, while the timestamp chars help avoid collision when
    //two update stats are running against same table.

    // Check for PUBLIC_ACCESS_SCHEMA.  This should always exist, except
    // on NT.  So, we go ahead and create it on NT.  If for some reason
    // it does not, then the current schema will be used.
    // If the sample table is for a volatile table, then create it in the
    // same volatile schema as the source table instead of in the
    // public_access_schema.
    // Creating it in the volatile schema will make it a volatile table
    // and all attributes of a volatile table, like nullable primary keys,
    // will apply.
    // There are no security issues with a volatile table since it is
    // always created by the same user and in the same session where the
    // update statistics command is being issued from.
    // For SeaQuest (COM_VERSION >= 2500), the PUBLIC_ACCESS_SCHEMA is guaranteed
    // to exist, so don't call the function that checks for it.
    if (!objDef->isVolatile() && objDef->publicSchemaExists())
    {
      sampleTable  = objDef->getCatName(HSTableDef::EXTERNAL_FORMAT);
      sampleTable += ".PUBLIC_ACCESS_SCHEMA.";
    }
    else
    {
      sampleTable  = objDef->getHistLoc(HSTableDef::EXTERNAL_FORMAT);
      sampleTable += ".";
    }

#ifdef _DEBUG
    // If the cqd USTAT_SAMPLE_TABLE_NAME_CREATE has a value, use it as the
    // sample table's name instead of deriving one from the uid of the sampled
    // table and the current timestamp.
    NAString cqdSampleTableName;
    CmpCommon::getDefault(USTAT_SAMPLE_TABLE_NAME_CREATE, cqdSampleTableName, 0);
    if (!IsNAStringSpaceOrEmpty(cqdSampleTableName))
      {
        sampleTable += cqdSampleTableName;
        makeAccessible_ = TRUE;  // Avoid making offline and undroppable
      }
    else
#endif  /* _DEBUG */
      {
        char objectIDStr[30]; // room for 64-bit integer (20 digits max)
        char zeroPaddedObjectIDStr[30];
        char timestampStr[20]; // room for _<10 digits>_<6 digits>
        NA_timeval tv;
        NA_gettimeofday(&tv, 0);

        // When constructing the sample table name, we use a fixed-length
        // representation to minimize non-determinism in test logs. (We
        // have found that variable-length representations are subject to
        // havoc with line-wrapping semantics; it's harder to filter out
        // non-determinism that wraps across lines.)

        // convert object UID to a fixed-length string (with leading zeroes)
        // note: object UIDs are always positive today
        convertInt64ToAscii(objDef->getObjectUID(), objectIDStr);
        strcpy(zeroPaddedObjectIDStr,"000000000000000000000000"); // 20 zeroes
        strcpy(zeroPaddedObjectIDStr+(20-strlen(objectIDStr)),objectIDStr);
        
        // use fixed length strings for the time stamp parts too
        sprintf(timestampStr, "_%010u_%06u", (UInt32)tv.tv_sec, (UInt32)tv.tv_usec);
        sampleTable += TRAF_SAMPLE_PREFIX;  // "TRAF_SAMPLE_"
        sampleTable += zeroPaddedObjectIDStr;
        sampleTable += timestampStr;

        // This is FALSE by default; we only set it here defensively in case it
        // is somehow set to TRUE elsewhere.
        makeAccessible_ = FALSE;
      }
  }
  else
  {
    //Allow user to specify volume location for temporary table
    //through HIST_SCRATCH_VOL.
    NAString userLocation = getTempTablePartitionInfo(unpartitionedSample, isPersSample);
    if (userLocation.isNull())
      sampleTable = objDef->getCatName();
    else
    {
      sampleTable  = objDef->getNodeName();
      sampleTable += ".";
      sampleTable += userLocation.data();
    }
    sampleTable += ".ZZMXTEMP.";
    sampleTable += objDef->getObjectName(HSTableDef::EXTERNAL_FORMAT);
  }
}

/***********************************************/
/* METHOD:  HSSample make() member function    */
/* PURPOSE: Create a sample table for          */
/*          sampling purposes only. Histograms */
/*          will be determined with data from  */
/*          this table only.                   */
/* OUTPUT:  sampleTableName - name of table.   */
/*          tableRowCnt - only assigned if     */
/*              rowCountIsEstimate = TRUE.     */
/* INPUT:   sampleRowCnt - the size of the     */
/*              sample table to create.        */
/* RETCODE:  0 - successful                    */
/*           non-zero otherwise                */
/***********************************************/
Lng32 HSSample::make(NABoolean rowCountIsEstimate, // input
                    NAString &sampleTableName,    // output
                    Int64 &tableRowCnt,           // output - rowcount of original table
                    Int64 &sampleRowCnt,          // input/output
                    NABoolean isPersSample,       // input. Default value is FALSE
                    NABoolean unpartitionedSample,// input. Default value is TRUE 
                    Int64 minRowCtPerPartition
                   )
  {
    Lng32 retcode = 0;
    NAString dml, insertType, sampleOption;
    char intStr[30];
    NABoolean forceNoPartitioning = TRUE;

    HSTranMan *TM = HSTranMan::Instance();
    HSLogMan  *LM = HSLogMan::Instance();
    HSGlobalsClass *hs_globals = GetHSContext();

    LM->StartTimer("Create/populate sample table");
    (void)getTimeDiff(TRUE);

    NABoolean EspCQDUsed = FALSE;
    NABoolean HBaseCQDsUsed = FALSE;
     
    sampleRowCount = sampleRowCnt;  // Save sample row count for HSSample object.

    // Create sample option based on sampling type, using 'samplePercent'.
    if (hs_globals)
      createSampleOption(sampleType, samplePercent, sampleOption,
                         hs_globals->sampleValue1, hs_globals->sampleValue2);
    else
      createSampleOption(sampleType, samplePercent, sampleOption);

    //Normally, we want to create an AUDITED scratch table. Although, for
    //performance and TMF timeouts, we should use a NON-AUDITED table and
    //SIDETREE inserts.
    //The default is to use SIDETREE inserts, unless a transaction is active.
    //You could use cqd USTAT_USE_SIDETREE_INSERT to control this feature.
    //NOTE: before making changes to this code, you must consider TMF
    //AUTOABORT time. This process may take a long time and blow away
    //your transaction.
    LM->StartTimer("Create sample table");
    retcode = create(unpartitionedSample, isPersSample);
    LM->StopTimer();
    if (retcode == -HS_PKEY_FLOAT_ERROR) {
      // If creation of sample table fails with -1120, then the primary key
      // has a float datatype and can't be partitioned.  Turn off POS.
      LM->StartTimer("Create sample table (no partitions)");
      retcode = create(forceNoPartitioning, isPersSample);
      LM->StopTimer();
    }
    if (retcode) TM->Rollback();
    HSHandleError(retcode);

    if (LM->LogNeeded())
      {
        sprintf(LM->msg, "\tSAMPLE TABLE = %s", sampleTable.data());
        LM->Log(LM->msg);
      }

    // If a transaction is running then the table was created as audited and we
    // need to use a vanilla INSERT statement. Otherwise, we can use SIDETREE
    // INSERTS for better performance. A current bug in the HBase interface
    // requires the use of Upsert.
    // For Hive tables the sample table used is a Trafodion table
    if (hs_globals->isHbaseTable || hs_globals->isHiveTable)
      {
        EspCQDUsed = setEspParallelism(hs_globals->objDef);

        // Set CQDs controlling HBase cache size (number of rows returned by HBase
        // in a batch) to avoid scanner timeout. Reset these after the sample query
        // has executed.
        if (hs_globals->isHbaseTable)
          HBaseCQDsUsed = hs_globals->setHBaseCacheSize(samplePercent);

        if (CmpCommon::getDefault(TRAF_LOAD_USE_FOR_STATS) == DF_ON)
          {
            insertType = "LOAD WITH NO OUTPUT, NO RECOVERY, NO POPULATE INDEXES, NO DUPLICATE CHECK INTO ";
          }
        else
          {
            insertType = "UPSERT USING LOAD INTO ";
          }
      }
    else if (TM->InTransaction())
      insertType = "INSERT INTO ";
    else
      {
        insertType = "INSERT INTO ";
        //10-040706-7608: a workaround for this solution is to turn cqd    //Workaround: 10-040706-7608
        //PLAN_STEALING OFF. Once the compiler fixes their plan stealing   //Workaround: 10-040706-7608
        //plan issues, the cqd should be removed.                          //Workaround: 10-040706-7608
        HSFuncExecQuery("CONTROL QUERY DEFAULT PLAN_STEALING 'OFF'");      //Workaround: 10-040706-7608
      }

    // If the insert is trying to insert values into an IDENTITY
    // column which is a GENERATED ALWAYS AS IDENTITY column,
    // then a special override is required to allow the insertion
    // of values into the IDENTITY column.

    HSFuncExecQuery("CONTROL QUERY DEFAULT OVERRIDE_GENERATED_IDENTITY_VALUES 'ON'");    

    dml  = insertType;
    dml += sampleTable;
    dml += " SELECT ";

    // Generate the select list. Truncate any over-long char/varchar columns
    // by using SUBSTRING calls. Omit any LOB columns.
    objDef->addTruncatedSelectList(dml);

    dml += " FROM ";

    NAString hiveSrc = CmpCommon::getDefaultString(USE_HIVE_SOURCE);
    if (! hiveSrc.isNull())
      {
        dml += "HIVE.HIVE.";
        dml += objDef->getObjectName();
        dml += hiveSrc;
      }
    else
      dml += getTableName(objDef->getObjectFullName(), objDef->getNameSpace());

    char cardHint[50];
    sprintf(cardHint, " <<+ cardinality %e >> ", (double)hs_globals->actualRowCount);
    dml += cardHint;

    dml += sampleOption;
    dml += " FOR SKIP CONFLICT ACCESS";
    const Int32 hsALLOW_SPECIALTABLETYPE = 0x1;
    if (objDef->getNameSpace() == COM_IUD_LOG_TABLE_NAME)
    {
       SQL_EXEC_SetParserFlagsForExSqlComp_Internal(hsALLOW_SPECIALTABLETYPE);
    }

    // on very busy system, some "update statistics" implementation steps like
    // "Process_Query" step in HSSample::make() that calls HSFuncExecQuery
    // may experience failures resulting in a flurry of callcatcher error 9200 
    // events that show up in genesis solutions like 10-110320-6751, etc. 
    // we suspect some of these errors may be transient 
    // failures that may succeed if retried enough times.
    // 2 cqds allow user control of these retries.
    Int32 centiSecs = getDefaultAsLong(USTAT_RETRY_DELAY);
    Int32 limit = getDefaultAsLong(USTAT_RETRY_LIMIT);
    Int64 printPlan = 1;
    if (limit < 1 || centiSecs < 1) // user does not want any retry
    {
      LM->StartTimer("Populate sample table");
      retcode = HSFuncExecQuery(dml, - UERR_INTERNAL_ERROR, &sampleRowCount, 
                                HS_QUERY_ERROR, &printPlan , objDef);
      LM->StopTimer();
    }
    else // user wants retry
    { 
      // use AQR
      HSFuncExecQuery("CONTROL QUERY DEFAULT AUTO_QUERY_RETRY 'ON'");
      LM->StartTimer("Populate sample table (with possible retry)");
      retcode = HSFuncExecQuery(dml, - UERR_INTERNAL_ERROR, &sampleRowCount, 
                                HS_QUERY_ERROR, &printPlan, objDef);
      LM->StopTimer();
      HSFuncExecQuery("CONTROL QUERY DEFAULT AUTO_QUERY_RETRY RESET");
    }

    sampleRowCnt = sampleRowCount;

    if (objDef->getNameSpace() == COM_IUD_LOG_TABLE_NAME)
    {
      SQL_EXEC_ResetParserFlagsForExSqlComp_Internal(hsALLOW_SPECIALTABLETYPE);
    }

    // On SQ, alter the sample table to audit afterwards. There are performance 
    // issues with non-audited tables on SQ. For Trafodion, however, this alter
    // is not supported, so skip it.
     if (!hs_globals->isHbaseTable && !hs_globals->isHiveTable)
       {
         LM->StartTimer("Set audit attribute on sample table");
         SQL_EXEC_SetParserFlagsForExSqlComp_Internal(hsALLOW_SPECIALTABLETYPE);

         NAString alterStmt;
         alterStmt = "ALTER TABLE ";
         alterStmt += sampleTable;
         alterStmt += " attribute audit" ;
         retcode = HSFuncExecDDL(alterStmt,
                                 -UERR_GENERIC_ERROR,
                                 NULL,
                                 sampleTable, objDef);
         SQL_EXEC_ResetParserFlagsForExSqlComp_Internal(hsALLOW_SPECIALTABLETYPE);

         // Don't invoke HSHandleError, which returns if there was an error; need to
         // reset cqds below.
         if (retcode)
           {
             HSFilterWarning(retcode);
             HSFilterError(retcode);
           }
         LM->StopTimer();
       }

    // RESET CQDS:
    //10-040706-7608: a workaround for this solution is to turn cqd        //Workaround: 10-040706-7608
    //PLAN_STEALING OFF. Once the compiler fixes their plan stealing       //Workaround: 10-040706-7608
    //plan issues, the cqd should be removed.                              //Workaround: 10-040706-7608
    if (!(TM->InTransaction()))                                            //Workaround: 10-040706-7608
      HSFuncExecQuery("CONTROL QUERY DEFAULT PLAN_STEALING RESET");        //Workaround: 10-040706-7608
    
    HSFuncExecQuery("CONTROL QUERY DEFAULT POS RESET");
    HSFuncExecQuery("CONTROL QUERY DEFAULT POS_NUM_OF_PARTNS RESET");
    
    // Reset the IDENTITY column override CQD
    HSFuncExecQuery("CONTROL QUERY DEFAULT OVERRIDE_GENERATED_IDENTITY_VALUES RESET");                                                                      

    if (HBaseCQDsUsed)
      {
        hs_globals->resetCQDs();
      }
    if (EspCQDUsed)
      {
        resetEspParallelism();
      }

    if (retcode) TM->Rollback();
    else         TM->Commit();

    if (LM->LogNeeded())
      {
        convertInt64ToAscii(sampleRowCount, intStr);
        sprintf(LM->msg, "\t\t\tSAMPLE TABLE SIZE = %s", intStr);
        LM->Log(LM->msg);
      }

 
    double sampleRatio = samplePercent / 100;
    double tableRowCntDbl = ((double)sampleRowCount) / sampleRatio;
    if (!isnormal(tableRowCntDbl))  // if we get NaN, infinity etc, just use original row count
      tableRowCntDbl = (double)tableRowCnt;

    // TEMP: ignore empty sample set if bulk load is on as rowcount is currently not 
    // being returned by bulk load.
    if ((sampleRowCount == 0) &&                    // sample set is empty;
        (CmpCommon::getDefault(USTAT_USE_BULK_LOAD) == DF_OFF))
      {                                          // cannot generate histograms
        drop();  // drop the sample table we created
        HSFuncMergeDiags(- UERR_SAMPLE_SET_IS_ZERO);
        retcode = -1;
      }
    else if (hs_globals)
      {
      hs_globals->sampleTableUsed = TRUE;
      hs_globals->samplingUsed    = TRUE;
      hs_globals->sampleSeconds   = getTimeDiff();
      // If (a) current row count is estimate
      //    (b) user has not specified the rowcount and
      //    (c) CLUSTER sampling not used
      // we set the actualRowCount to the value inferred by the number of sample
      // rows and the sampling ratio.
      if (rowCountIsEstimate &&
          !(hs_globals->optFlags & ROWCOUNT_OPT) &&
          (hs_globals->optFlags & SAMPLE_REQUESTED) != SAMPLE_RAND_2) 
        {
          tableRowCnt = (Int64)tableRowCntDbl;
          if (LM->LogNeeded())
          {
            convertInt64ToAscii(tableRowCnt, intStr);
            sprintf(LM->msg, "\tThe actual row count from statistics = %s", intStr);
            LM->Log(LM->msg);
          }
        }
      }
    else if (rowCountIsEstimate) 
      tableRowCnt = (Int64)tableRowCntDbl;

    LM->StopTimer();

    sampleTableName = sampleTable;
    if (hs_globals)
      hs_globals->checkTime("after creating and populating sample table");
    return retcode;
  }


/************************************************************************/
/*                         MakeAllHistid()                              */
/*                                                                      */
/* FUNCTION:  Generates new unique histogram ids                        */
/*            for histograms to be updated.  Performs:                  */
/*              CURSOR103_... (via groupListFromTable() - reads from    */
/*                            HISTOGRAM for SERIALIZABLE ACCESS.        */
/*                                                                      */
/*            For existing histograms,                                  */
/*              the new ids will be generated by DualHistid(old id).    */
/*                                                                      */
/*            For a new histogram,                                      */
/*              if there is any old histogram,                          */
/*                its id will be the maximum id (prevHistId) + 5.       */
/*              if there is no old histogram,                           */
/*                its id is Julian Timestamp masked with 0x7FFFFFFF.    */
/*                                                                      */
/************************************************************************/
Lng32 HSGlobalsClass::MakeAllHistid()
  {
    Lng32 retcode = 0;
    ULng32 prevHistId = 0;
    NABoolean processMultiGroups;
    NAString missingHistograms = "";

    HSColGroupStruct *tableGroupList;
    HSColGroupStruct *tableGroup;
    HSColGroupStruct *group;
    NABoolean msgTruncated = FALSE;
    HSLogMan *LM = HSLogMan::Instance();

    // Create a list of histograms that already exist for this table
    // in HISTOGRAMS table.  This function also puts any duplicates in
    // a list at HSGlobalsClass::dupGroup.
    retcode = groupListFromTable(tableGroupList, FALSE, TRUE /* exclusive mode */); // Reads from HISTOGRAMS table
    HSHandleError(retcode);
    if (LM->LogNeeded())
      {
        LM->Log("\t\tEXISTING HISTOGRAMS");
        if (tableGroupList != NULL)
          tableGroupList->print();
      }

    //PASS 1: Loop through histograms that already exist in HISTOGRAMS table.
    //        For those that are to be updated as part of this update stats,
    //        create new hist id by inverting LSB of old hist id.  
    //        Any old histogram that is not being updated, add to list
    //        so it will be reported in warning to user.  
    LM->StartTimer("Change hist ids of existing histograms");
    tableGroup = tableGroupList;
    while (tableGroup != NULL)
      {
        prevHistId = MAXOF(prevHistId, tableGroup->oldHistid);
        group = findGroup(tableGroup); // find histogram entry in command list.
        if (group == NULL)
          {
            if (missingHistograms.length() <= HS_MAX_MSGTOK_LEN)
              {
                if  (tableGroup->reason != HS_REASON_EMPTY)
                  {
                    missingHistograms += "(";
                    missingHistograms += tableGroup->colNames->data();
                    missingHistograms += "),";
                  }
              }
            else if (NOT msgTruncated)
              {
                missingHistograms += "(...),";
                msgTruncated = TRUE;
              }
          }
        else
          {
            if (group->oldHistid == 0) {
              // Only assign the following fields if not already assigned.
              // These fields are assigned with NECESSARY, ... syntax.
              group->oldHistid = tableGroup->oldHistid;
              group->reason = tableGroup->reason;
            }
            // Flip LSB to generate new histid.  Must use oldHistid from 
            // tableGroup since this is what was just read from the HISTOGRAMS
            // table with groupListFromTable() function.  Preserve READ_TIME.
            group->newHistid = DualHistid(tableGroup->oldHistid);
            strncpy(group->readTime, tableGroup->readTime, TIMESTAMP_CHAR_LEN);
            group->readTime[TIMESTAMP_CHAR_LEN] = '\0';
          }
        tableGroup = tableGroup->next;
      }
    LM->StopTimer();  // Change hist ids of existing histograms

    // Create 9202 warning if some existing histograms are not getting regenerated
    // but not if the NECESSARY keyword is being used.
    if (missingHistograms.length() > 0 && !(optFlags & NECESSARY_OPT))
      {
        missingHistograms.remove(missingHistograms.length() - 1);       // remove last comma
        HSFuncMergeDiags(UERR_DOWN_LEVEL_HISTOGRAMS, missingHistograms.data());

        if (LM->LogNeeded())
          {
            LM->Log("WARNING: missing GroupLists ");
            LM->Log(missingHistograms.data());
          }
      }

    //PASS 2: Loop through histograms that are being updated.  Histograms that
    //        did not previously exist in the HISTOGRAMS table should have
    //        oldHistid = 0 and newHistid = 0.
    if (!(optFlags & CLEAR_OPT))
      {
        LM->StartTimer("Check histograms that are being updated");
        if (singleGroup)
          {
            group = singleGroup;                  /* process single-columns  */
            processMultiGroups = TRUE;            /* then multi-columns      */
          }
        else
          {
            group = multiGroup;                   /* process multi-columns   */
            processMultiGroups = FALSE;           /* only once               */
          }

        while (group != NULL)
          {
            if ((group->oldHistid == 0) &&
                (group->newHistid == 0))
              {
                if (prevHistId == 0)
                  {
                    prevHistId = (ULng32) 
                      (NA_JulianTimestamp() & 
                       ColStats::USTAT_HISTOGRAM_ID_THRESHOLD);
                    group->newHistid = prevHistId;
                  }
                else
                  {
                    prevHistId = prevHistId + 5;
                    group->newHistid = prevHistId;
                  }
              }

            group = group->next;
            if (group == NULL && processMultiGroups)
              {
                processMultiGroups = FALSE;
                group = multiGroup;
              }
          }
        LM->StopTimer();   // Check histograms that are being updated
      }

    if (LM->LogNeeded())
      {
        LM->Log("\t\tREQUESTED GROUPLIST(s)");
        if (singleGroup != NULL)
          singleGroup->print();
        if (multiGroup != NULL)
          multiGroup->print();
      }

    if (tableGroupList != NULL)
      {
        delete tableGroupList;
        tableGroupList = NULL;
      }

    return retcode;
  }

/**************************************************************************/
/* METHOD:  getAdjustedIntervalCount()                                    */
/* PURPOSE: Make adjustments to the number of intervals that will be used */
/*          based on column type, single- or multi-column, and number of  */
/*          gap and high frequency intervals desired.                     */
/* PARAMS:  group(in) -- Group to adjust the interval count for.          */
/*          intCount(in) -- Current number of intervals.                  */
/*          rowCount(in) -- Number of rows in the table.                  */
/*          rowsetSize(in) -- Number of rows in the current rowset.       */
/*          singleIntervalPerUec(out) -- TRUE if this function determines */
/*                                       we need a separate interval for  */
/*                                       each distinct value.             */
/*          gapIntCount(out) -- Number of intervals to be used for gaps.  */
/*          highFreqIntCount(out) -- Number of intervals for high         */
/*                                   frequency values.                    */
/* RETCODE: The number of intervals to use, after adjustment.             */
/**************************************************************************/
Lng32 HSGlobalsClass::getAdjustedIntervalCount(
                              HSColGroupStruct *group,
                              Lng32 intCount,
                              Int64 rowCount,
                              Lng32 rowsetSize,
                              NABoolean &singleIntervalPerUec,
                              Lng32 &gapIntCount,      // get #intvls for gaps
                              Lng32 &highFreqIntCount) // get #intvls for high freq 
{
  HSLogMan *LM = HSLogMan::Instance();
  Lng32 adjIntCount = intCount;

  //For some reason character data requires more intervals. This code has
  //been here before my time. My best guess is that when we encode 
  //char->double, using CharType::encodeString(), the numeric result may be
  //inaccurate. Therefore, if we use more intervals, the error becomes less
  //visible. Regardless, if the user requests GENERATE x INTERVALS, we should
  //not make any adjustments.
  if ( NOT (optFlags & INTERVAL_OPT) &&
       DFS2REC::isAnyCharacter(group->colSet[0].datatype))
    adjIntCount = MAXOF(intCount, 62);

  //#intervals should not be greater than the number of rows in table
  if (rowCount < (Int64) INT_MAX)
    adjIntCount = MINOF(adjIntCount, (Lng32)(rowCount));

  //If all the rows in the table have been exhausted AND
  //   UEC <= LOW_UEC_THRESHOLD or UEC <= #intervals
  // adjust the #intervals so that only 1 uec per interval.
  if ( ((rowsetSize <= CmpCommon::getDefaultLong(HIST_LOW_UEC_THRESHOLD))
                                    &&
         NOT (optFlags & INTERVAL_OPT)
       )
                                    ||
       ((optFlags & INTERVAL_OPT) && (rowsetSize <= intCount))
     )
    {
      adjIntCount = MAXOF(rowsetSize, 1);
      singleIntervalPerUec = TRUE;
    }

  // Set multi-column histograms to 1 interval.  The optimizer can only use
  if (group->colCount > 1 && !group->skewedValuesCollected)  
    adjIntCount = 1;

  // If gap processing is to be done (numeric single-column groups and not single
  // interval for each distinct value), set the number of intervals we want to
  // use for gaps. Add that same number of extra intervals so we can be more
  // inclusive in creating gap intervals before we have seen all the gaps and
  // know their actual distribution. We will merge the lesser ones with other
  // intervals if we select too many.
  if (CmpCommon::getDefault(USTAT_PROCESS_GAPS) == DF_OFF)  // disabled by CQD
    {
      gapIntCount = 0;  // forget about gaps
      if (LM->LogNeeded())
        LM->Log("Gap processing is disabled.");
    }
  else if (!singleIntervalPerUec && (adjIntCount > 1) && (group->colCount <= 1) 
                                 && DFS2REC::isNumeric(group->colSet[0].datatype))
    {
      gapIntCount = (Lng32)(adjIntCount 
                       * (CmpCommon::getDefaultNumeric(USTAT_GAP_PERCENT) / 100));
      adjIntCount += gapIntCount;
    }
  else
    {
      gapIntCount = 0;  // forget about gaps
      if (LM->LogNeeded() && group->colCount == 1)
        {
          sprintf(LM->msg, "No gap processing for column %s: ",
                           group->colSet[0].colname->data());
          if (singleIntervalPerUec)
            sprintf(LM->msg + strlen(LM->msg), "using single interval per uec");
          else if (!DFS2REC::isNumeric(group->colSet[0].datatype))
            sprintf(LM->msg + strlen(LM->msg), "not a numeric column");
          else
            sprintf(LM->msg + strlen(LM->msg), "** reason unknown **");  // shouldn't happen
          LM->Log(LM->msg);
        }
    }

  // Add some intervals for high frequency values. These won't be used in the
  // calculation of the step size. highFreqIntCount is returned and passed to
  // HSHistogram ctor.
  if (!singleIntervalPerUec && adjIntCount > 1)
    {
      float freqSizePercent = (float) CmpCommon::getDefaultNumeric(USTAT_FREQ_SIZE_PERCENT);
      // avoid a divide by zero.  Also, a percentage should not be negative
      if(freqSizePercent<=0)
        highFreqIntCount = 1000;
      else
        highFreqIntCount = MINOF(1000,(Lng32)(100.0 / freqSizePercent));
      adjIntCount = MINOF(adjIntCount + highFreqIntCount, HS_MAX_INTERVAL_COUNT);
      if (LM->LogNeeded())
        {
          sprintf(LM->msg, "Allotting %d intervals for high frequency values",
                  highFreqIntCount);
          LM->Log(LM->msg);
        }
    }
  else
    highFreqIntCount = 0;  // don't need them

  return adjIntCount;
}

// Return the name of the integral type to map an interval type to, based
// on the largest integer value possible when a value of the interval type
// is cast to its rightmost (most precise) field. The returned name is used
// in the CAST clause in the query to retrieve the interval value. For example,
// INTERVAL HOUR(2) TO MINUTE could represent a duration up to 5999 minutes,
// and can be cast to a smallint.
//
NAString* getIntTypeForInterval(HSColGroupStruct *group, Int64 maxIntValue)
{
  NAString* typeName;
  group->ISprecision = 0;
  group->ISscale = 0;
  if (maxIntValue <= CHAR_MAX)
    {
      group->ISdatatype = REC_BIN8_SIGNED;
      group->ISlength = 1;
      typeName = &LiteralTinyInt;  // from NumericTypes.h
    }
  else if (maxIntValue <= SHRT_MAX)
    {
      group->ISdatatype = REC_BIN16_SIGNED;
      group->ISlength = 2;
      typeName = &LiteralSmallInt;  // from NumericTypes.h
    }
  else if (maxIntValue <= INT_MAX)
    {
      group->ISdatatype = REC_BIN32_SIGNED;
      group->ISlength = 4;
      typeName = &LiteralInteger;   // from NumericTypes.h
    }
  else
    {
      group->ISdatatype = REC_BIN64_SIGNED;
      group->ISlength = 8;
      typeName = &LiteralLargeInt;  // from NumericTypes.h
    }

  return typeName;
}

// Return the type name that an interval containing a seconds field with
// fractional seconds precision will be cast to. Also set up the other
// type fields for the result of the cast. For example, INTERVAL MINUTE(2)
// TO SECOND(6) would be cast as NUMERIC(10,6) to hold the number of seconds
// in a value of the interval.
//
NAString* getNumericTypeForInterval(HSColGroupStruct *group, 
                                    Lng32 secondsPrecision,
                                    Lng32 fractionalPrecision,
                                    char* precCommaScale)
{
  NAString *typeName = new (STMTHEAP) NAString("numeric(", STMTHEAP);
  typeName->append(precCommaScale).append(")");
  group->ISprecision = secondsPrecision + fractionalPrecision;
  group->ISscale = fractionalPrecision;
  if (group->ISprecision < 5)
    {
      group->ISdatatype = REC_BIN16_SIGNED;
      group->ISlength = 2;
    }
  else if (group->ISprecision < 10)
    {
      group->ISdatatype = REC_BIN32_SIGNED;
      group->ISlength = 4;
    }
  else
    {
      group->ISdatatype = REC_BIN64_SIGNED;
      group->ISlength = 8;
    }

  return typeName;
}

// Form the select list expression for each column handled through internal sort.
// For columns of an SQL type having a corresponding C type, the expression is
// simply the column name.  For other types, the expression may perform an
// order-preserving transformation into an efficiently sorted C type.
//
// This function also sets the type information for the sorted column.  If the
// column is not transformed before sorting, these type fields (ISdatatype,
// ISlength, ISprecision, ISscale) are the same as the column's original type
// information.
//
// The forHive parameter indicates whether this is called when using a Hive
// sample table that was created by the bulk loader. If so, we generate column
// names as "col" with an integer appended that is the ordinal position of the
// column in the table. This avoids all problems we would otherwise have with
// delimited ids due to the restricted set of characters currently allowed in
// Hive names and lack of case-sensitivity.
//
static void mapInternalSortTypes(HSColGroupStruct *groupList, NABoolean forHive = FALSE)
{
  HSColGroupStruct *group = groupList;
  NAString* typeName;
  HSLogMan *LM = HSLogMan::Instance();
  char sbuf[40];
  while (group)
    {
     HSColumnStruct &col = group->colSet[0];
     NAString columnName, dblQuote="\"";
     group->ISSelectExpn = "";

     // If retrieving from the Hive backing sample for a table, positional names
     // are used. This avoids any issues with Trafodion delimited ids that do
     // not map to valid Hive column names.
     if (forHive)
       {
         columnName = "col";
         sprintf(sbuf, "%d", col.colnum+1);
         columnName.append(sbuf);
       }
     // Surround column name with double quotes, if not already delimited.
     else if (group->colNames->data()[0] == '"')
       columnName=group->colNames->data();
     else
       columnName=dblQuote+group->colNames->data()+dblQuote;

     *(col.externalColumnName) = columnName;

     switch (col.datatype)
     {
      case REC_DECIMAL_LSE:
      case REC_DECIMAL_UNSIGNED:
      case REC_DECIMAL_LS:
        if (col.precision < 5)
          {
            if (col.datatype == REC_DECIMAL_UNSIGNED)
              group->ISdatatype = REC_BIN16_UNSIGNED;
            else
              group->ISdatatype = REC_BIN16_SIGNED;
            group->ISlength = 2;
            typeName = &LiteralSmallInt;  // from NumericTypes.h
          }
        else if (col.precision < 10)
          {
            if (col.datatype == REC_DECIMAL_UNSIGNED)
              group->ISdatatype = REC_BIN32_UNSIGNED;
            else
              group->ISdatatype = REC_BIN32_SIGNED;
            group->ISlength = 4;
            typeName = &LiteralInteger;   // from NumericTypes.h
          }
        else
          {
            // Max precision is 18. Largeint can't be unsigned.
            group->ISdatatype = REC_BIN64_SIGNED;
            group->ISlength = 8;
            typeName = &LiteralLargeInt;  // from NumericTypes.h
          }
        group->ISprecision = 0;
        group->ISscale = 0;
        formatFixedNumeric((Int64)pow(10, col.scale), 0, sbuf); 
        group->ISSelectExpn.append("cast(")
            .append(columnName)
            .append("*")
            .append(sbuf)
            .append(" as ")
            .append(*typeName);
        if (col.datatype == REC_DECIMAL_UNSIGNED)
          group->ISSelectExpn.append(" unsigned");
        group->ISSelectExpn.append(")");
        break;

      case REC_DATETIME:
        switch (col.precision)
          {
            case REC_DTCODE_DATE:
              group->ISdatatype = REC_BIN32_SIGNED;
              group->ISlength = 4;
              group->ISprecision = 0;
              group->ISscale = 0;
              group->ISSelectExpn.append("datediff(day, date'0001-01-01', ")
                  .append(columnName)
                  .append(")");
              break;
        
            // time(0) is treated as an integer, while time(n) is treated as
            // numeric(5+n,n).
            case REC_DTCODE_TIME:
              if (col.scale > 0)  // Max scale (fractional seconds) is 6
                {
                  group->ISprecision = 5 + col.scale; // 5 digits for #seconds/day
                  group->ISscale = col.scale;
                  if (col.scale <= 4)
                    {
                      group->ISdatatype = REC_BIN32_SIGNED;
                      group->ISlength = 4;
                    }
                  else
                    {
                      group->ISdatatype = REC_BIN64_SIGNED;
                      group->ISlength = 8;
                    }
                }
              else
                {
                  group->ISdatatype = REC_BIN32_SIGNED;
                  group->ISlength = 4;
                  group->ISprecision = 0;
                  group->ISscale = 0;
                }
              // CLI will treat this expression as a largeint regardless of the
              // fractional seconds precision. Since we are assuming it will be
              // a 4-byte int if scale < 5, we have to add an extra cast so the
              // value is returned in the format we are expecting.
              if (col.scale > 0 && col.scale <= 4)
                group->ISSelectExpn.append("cast(");
              group->ISSelectExpn.append("hour(").append(columnName).append(")*3600+minute(")
                  .append(columnName).append(")*60+second(")
                  .append(columnName).append(")");
              if (col.scale > 0 && col.scale <= 4)
                {
                  sprintf(sbuf, " as numeric(%d,%d))", group->ISprecision, group->ISscale);
                  group->ISSelectExpn.append(sbuf);
                }
              break;

            case REC_DTCODE_TIMESTAMP:
              group->ISdatatype = REC_BIN64_SIGNED;
              group->ISlength = 8;
              group->ISprecision = 0;
              group->ISscale = 0;
              group->ISSelectExpn.append("juliantimestamp(")
                  .append(columnName)
                  .append(")");
              break;

            default:
              LM->Log("INTERNAL ERROR (mapInternalSortTypes):");
              sprintf(LM->msg, "Undefined datetime type %d", col.precision);
              LM->Log(LM->msg);
              *CmpCommon::diags() << DgSqlCode(-UERR_GENERIC_ERROR)
                                  << DgString0("mapInternalSortTypes()")
                                  << DgString1("N/A")
                                  << DgString2(LM->msg);
              throw CmpInternalException("failure in mapInternalSortTypes()",
                                         __FILE__, __LINE__);
          }
        break;

      // col.precision is the leading field (decimal) precision for an
      // interval type. Using this as an exponent of 10 gives an upper bound
      // for the interval value cast as an integer.
      case REC_INT_YEAR:
      case REC_INT_MONTH:
      case REC_INT_DAY:
      case REC_INT_HOUR:
      case REC_INT_MINUTE:
        typeName = getIntTypeForInterval(group, (Int64)pow(10, col.precision));
        group->ISSelectExpn.append("cast(")
            .append(columnName)  // interval column
            .append(" as ")
            .append(*typeName)   // smallint, int, etc., depending on interval precision
            .append(")");
        break;

      case REC_INT_SECOND:
        if (col.scale > 0)
          {
            // The casts to seconds and to numeric will both have (prec,scale)
            // components, but prec includes scale for numeric, while for seconds
            // it does not.
            sprintf(sbuf, "%d,%d", col.precision+col.scale, col.scale);
            typeName = getNumericTypeForInterval(group, col.precision, col.scale, sbuf);
            sprintf(sbuf, "%d,%d", col.precision, col.scale); // for seconds cast below
            }
        else
          typeName = getIntTypeForInterval(group, (Int64)pow(10, col.precision));
        group->ISSelectExpn.append("cast(")
            .append(columnName)
            .append(" as ")
            .append(*typeName)
            .append(")");
        break;

      case REC_INT_YEAR_MONTH:
        sprintf(sbuf, "%d", col.precision+2); // required precision for single-field interval
        typeName = getIntTypeForInterval(group, 12 * (Int64)pow(10, col.precision));
        group->ISSelectExpn.append("cast(cast(")
            .append(columnName)
            .append(" as interval month(")
            .append(sbuf)
            .append(")) as ")
            .append(*typeName)
            .append(")");
        break;

      case REC_INT_DAY_HOUR:
        sprintf(sbuf, "%d", col.precision+2);
        typeName = getIntTypeForInterval(group, 24 * (Int64)pow(10, col.precision));
        group->ISSelectExpn.append("cast(cast(")
            .append(columnName)
            .append(" as interval hour(")
            .append(sbuf)
            .append(")) as ")
            .append(*typeName)
            .append(")");
        break;

      case REC_INT_HOUR_MINUTE:
        sprintf(sbuf, "%d", col.precision+2);
        typeName = getIntTypeForInterval(group, 60 * (Int64)pow(10, col.precision));
        group->ISSelectExpn.append("cast(cast(")
            .append(columnName)
            .append(" as interval minute(")
            .append(sbuf)
            .append(")) as ")
            .append(*typeName)
            .append(")");
        break;

      case REC_INT_DAY_MINUTE:
        sprintf(sbuf, "%d", col.precision+4); // required precision for single-field interval
        typeName = getIntTypeForInterval(group, 24 * 60 * (Int64)pow(10, col.precision));
        group->ISSelectExpn.append("cast(cast(")
            .append(columnName)
            .append(" as interval minute(")
            .append(sbuf)
            .append(")) as ")
            .append(*typeName)
            .append(")");
        break;

      case REC_INT_MINUTE_SECOND:
        if (col.scale > 0)
          {
            // The casts to seconds and to numeric will both have (prec,scale)
            // components, but prec includes scale for numeric, while for seconds
            // it does not.
            sprintf(sbuf, "%d,%d", col.precision+2+col.scale, col.scale);
            typeName = getNumericTypeForInterval(group, col.precision+2, col.scale, sbuf);
            sprintf(sbuf, "%d,%d", col.precision+2, col.scale); // for seconds cast below
          }
        else
          {
            sprintf(sbuf, "%d,0", col.precision+2); // for seconds cast below
            typeName = getIntTypeForInterval(group, 60 * (Int64)pow(10, col.precision));
          }
        group->ISSelectExpn.append("cast(cast(")
            .append(columnName)
            .append(" as interval second(")
            .append(sbuf)
            .append(")) as ")
            .append(*typeName)
            .append(")");
        break;

      case REC_INT_HOUR_SECOND:
        if (col.scale > 0)
          {
            // The casts to seconds and to numeric will both have (prec,scale)
            // components, but prec includes scale for numeric, while for seconds
            // it does not.
            sprintf(sbuf, "%d,%d", col.precision+4+col.scale, col.scale);
            typeName = getNumericTypeForInterval(group, col.precision+4, col.scale, sbuf);
            sprintf(sbuf, "%d,%d", col.precision+4, col.scale); // for seconds cast below
          }
        else
          {
            sprintf(sbuf, "%d,0", col.precision+4); // for seconds cast below
            typeName = getIntTypeForInterval(group, 60 * 60 * (Int64)pow(10, col.precision));
          }
        group->ISSelectExpn.append("cast(cast(")
            .append(columnName)
            .append(" as interval second(")
            .append(sbuf)
            .append(")) as ")
            .append(*typeName)
            .append(")");
        break;

      case REC_INT_DAY_SECOND:
        if (col.scale > 0)
          {
            // The casts to seconds and to numeric will both have (prec,scale)
            // components, but prec includes scale for numeric, while for seconds
            // it does not.
            sprintf(sbuf, "%d,%d", col.precision+5+col.scale, col.scale);
            typeName = getNumericTypeForInterval(group, col.precision+5, col.scale, sbuf);
            sprintf(sbuf, "%d,%d", col.precision+5, col.scale); // for seconds cast below
          }
        else
          {
            sprintf(sbuf, "%d,0", col.precision+5); // for seconds cast below
            typeName = getIntTypeForInterval(group, 24 * 60 * 60 * (Int64)pow(10, col.precision));
          }
        group->ISSelectExpn.append("cast(cast(")
            .append(columnName)
            .append(" as interval second(")
            .append(sbuf)
            .append(")) as ")
            .append(*typeName)
            .append(")");
        break;

      // Either this type not handled by IS yet, in which case the assigned
      // values won't matter, or no encoding is needed, in which case the
      // fields for the sorted type are the same as those for the original type.
      default:
        {
          HSGlobalsClass *hs_globals = GetHSContext();
          group->ISdatatype = col.datatype;
          //group->ISlength = col.length;
          group->setISlength(col.length,hs_globals->maxCharColumnLengthInBytes);
          group->ISprecision = col.precision;
          group->ISscale = col.scale;
          // the method below handles adding SUBSTRING for over-size char/varchars
          col.addTruncatedColumnReference(group->ISSelectExpn);
        }
        break;
     } // switch
     group = group->next;
    }  // while
}

// For each multi-column for which a histograms are to be created, determine
// the extra memory needed for this multi-column to be computed in memory
void HSGlobalsClass::getMCMemoryRequirements(HSColGroupStruct* mgroup, Int64 rows)
{
  HSLogMan *LM = HSLogMan::Instance();

  if (LM->LogNeeded())
  {
     sprintf(LM->msg, "MC: Memory estimates for multi-group based on " PF64 " rows", rows);
     LM->Log(LM->msg);
  }

  while (mgroup)
  {
      getMemoryRequirementsForOneMCGroup(mgroup, rows);
      mgroup = mgroup->next;
  }
}

//
// Get the number of bytes required for one multi-column group.
//
void HSGlobalsClass::getMemoryRequirementsForOneMCGroup(HSColGroupStruct* mgroup, Int64 rows)
{
  HSLogMan *LM = HSLogMan::Instance();

  Int64 memNeededForAllCols = 0;
  Int64 memAllowed = getMaxMemory();

  HSColGroupStruct *sgroup;
  HSColumnStruct* col;
  for (Int32 i=0; i< mgroup->colCount; i++)
  {
     col = &mgroup->colSet[i];
     sgroup = findGroup(col->colnum);
     memNeededForAllCols += sgroup->memNeeded;
     // to simplify coding take the size of the largest iterator
     mgroup->memNeeded += sizeof(MCFixedCharIterator);
     if (sgroup->colSet[0].nullflag)
     {
        mgroup->memNeeded += sizeof(NABitVector);
        mgroup->memNeeded += ceil(rows/8);
     }
  }

  mgroup->memNeeded += sizeof(MCWrapper)*rows;
  mgroup->mcis_totalMCmemNeeded = mgroup->memNeeded+memNeededForAllCols;

  if (LM->LogNeeded())
  {
    *LM << "MC: Group with columns " << mgroup->colNames->data() << " requires (" 
        << mgroup->mcis_totalMCmemNeeded 
        << ") bytes of memory for internal sort including ("
        << mgroup->memNeeded << ") bytes for MC processing.";
    LM->FlushToLog();
  }
}

// For each column for which histograms are to be created, determine the
// number of bytes required to store all the values that will be used (all
// column values or the size of the sample to be collected).
//
void HSGlobalsClass::getMemoryRequirements(HSColGroupStruct* group, Int64 rows)
{
  HSLogMan *LM = HSLogMan::Instance();
  
  if (LM->LogNeeded())
    {
      sprintf(LM->msg, "Memory estimates for single group based on " PF64 " rows", rows);
      LM->Log(LM->msg);
    }

  while (group)
    {
      getMemoryRequirementsForOneGroup(group, rows);
      group = group->next;
    }
}

//
// Get the number of bytes required for one single group.
//
void HSGlobalsClass::getMemoryRequirementsForOneGroup(HSColGroupStruct* group, Int64 rows)
{
  HSLogMan *LM = HSLogMan::Instance();
  Int32 elementSize=0;
  
      switch (group->ISdatatype)
        {
          case REC_BOOLEAN:
          case REC_BIN8_SIGNED:
          case REC_BIN8_UNSIGNED:
            elementSize = 1;
            break;

          case REC_BIN16_SIGNED:
          case REC_BIN16_UNSIGNED:
            elementSize = 2;
            break;

          case REC_BIN32_SIGNED:
          case REC_BIN32_UNSIGNED:
          case REC_IEEE_FLOAT32:
            elementSize = 4;
            break;

          case REC_BIN64_SIGNED:
          case REC_BIN64_UNSIGNED:
          case REC_IEEE_FLOAT64:
            elementSize = 8;
            break;

          case REC_BYTE_F_ASCII:
          case REC_BYTE_F_DOUBLE:
          case REC_BINARY_STRING:
            // Length is in bytes, not chars. Add size for object that references
            // the string, which is stored in a separate array.
            elementSize = group->ISlength + sizeof(ISFixedChar);
            break;

          case REC_BYTE_V_ASCII:
          case REC_BYTE_V_DOUBLE:
          case REC_VARBINARY_STRING:
            elementSize = group->varcharContentSize() + sizeof(ISVarChar);
            break;

          default:
            // Check to see if the column's type is supposed to be handled
            // by internal sort.  If so, this is a problem.
            if (isInternalSortType(group->colSet[0]))
              {
                LM->Log("INTERNAL ERROR (getInternalSortMemoryRequirements):");
                sprintf(LM->msg, "Undefined datatype %d", group->ISdatatype);
                LM->Log(LM->msg);
                *CmpCommon::diags() << DgSqlCode(-UERR_GENERIC_ERROR)
                                    << DgString0("getInternalSortMemoryRequirements()")
                                    << DgString1("N/A")
                                    << DgString2(LM->msg);
                throw CmpInternalException("failure in getInternalSortMemoryRequirements()",
                                           __FILE__, __LINE__);
              }
            elementSize = 0;
            break;
        }

      Int64 i64MemNeeded = rows * elementSize;
      if (group->isCompacted())  // varchar only
        {
          i64MemNeeded += (MAX_ROWSET * group->inflatedVarcharContentSize());
        }
      group->memNeeded = (i64MemNeeded <= UINT_MAX ? (size_t)i64MemNeeded : 0);
      if (LM->LogNeeded())
        {
          if (group->memNeeded == 0)
            sprintf(LM->msg, "Column %s requires too much memory for internal sort",
                             group->colSet[0].colname->data());
          else
            sprintf(LM->msg, "Column %s requires " PFSZ " bytes of memory for internal sort.",
                             group->colSet[0].colname->data(), group->memNeeded);
          LM->Log(LM->msg);
        }
}

// The number of rows the allocation is based on is returned as the function
// result. This is necessary because sampleRowCount may change (become larger)
// subsequent to this -- if we are unable to read the entire sample directly
// into memory, a sample table is created and populated, and sampleRowCount
// is changed from an estimate to the actual number of rows read into the
// sample table. The maximum number of rows we will actually read must be based
// on the amount of memory allocated to hold their values.
//
Int64 HSGlobalsClass::getInternalSortMemoryRequirements(NABoolean performISForMC)
{
  HSLogMan *LM = HSLogMan::Instance();
  Int64 rows;
  if (sampleRowCount > 0)
    rows = sampleRowCount;
  else
    rows = actualRowCount;

  // get memory requirements for single column groups first
  getMemoryRequirements(singleGroup, rows);

  if ( performISForMC ) 
  {
     // now get memory requirements for multi-column groups
     getMCMemoryRequirements(multiGroup, rows);
  }

  return rows;
}

// Parse a simple query using the Where clause specified for an IUS statement
// with the parser flag PARSING_IUS_WHERE_CLAUSE set. This will return an
// error for any language constructs used within the Where clause that are not
// appropriate in this context. Any ordinary syntax error is ignored; we let
// these be reported later so they will be shown in the context of an actual
// statement generated for IUS.
Lng32 HSGlobalsClass::validateIUSWhereClause()
{
  Lng32 retcode = 0;

  // use QualifiedName constructor to correctly handle delimited names.
  QualifiedName qualTableName(user_table->data(), 1);
  NAString tableNameStr = qualTableName.getUnqualifiedObjectNameAsAnsiString();
  NAString query = "select count(*) from ";
  query.append(tableNameStr);
  query.append(" where ").append(getWherePredicateForIUS());

  // set PARSING_IUS_WHERE_CLAUSE bit in Sql_ParserFlags; return it to
  // its entry value on exit
  PushAndSetSqlParserFlags savedParserFlags(PARSING_IUS_WHERE_CLAUSE);

  Parser parser(CmpCommon::context());
  Lng32 diagsMark = diagsArea.mark();

  // We have to make the table name used in the From clause unqualified, or it
  // will be flagged as a violation of the IUS Where clause restriction against
  // qualification. To make sure the correct table is used, we temporarily
  // replace the default catalog and schema with the ones for the table we
  // are using.
  SchemaName& sch = const_cast<SchemaName&>(ActiveSchemaDB()->getDefaultSchema());
  NAString oldCatName = sch.getCatalogNameAsAnsiString();
  NAString oldSchName = sch.getUnqualifiedSchemaNameAsAnsiString();
  const char* period = strchr(catSch->data(), '.');
  NAString tempCatName(catSch->data(), period - catSch->data());
  NAString tempSchName(period+1);
  sch.setCatalogName(tempCatName);
  sch.setSchemaName(tempSchName);
  ExprNode* tree = parser.getExprTree(query.data(),
                                      query.length(),
                                      CharInfo::ISO88591);
  sch.setCatalogName(oldCatName);
  sch.setSchemaName(oldSchName);

  if (!tree)
    {
      // If an ordinary syntax error occurs, don't report it here, let it
      //surface through parsing of an actual statement generated by IUS.
      if (diagsArea.contains(-UERR_SYNTAX_ERROR))
        diagsArea.rewind(diagsMark, TRUE);
      else
        retcode = diagsArea.mainSQLCODE();
    }

  return retcode;
}

/***********************************************/
/* METHOD:  CollectStatistics()                */
/* PURPOSE: Generate histograms based on data  */
/*          read from table. If sampling is    */
/*          requested, the data is read from   */
/*          sample table. Otherwise, data is   */
/*          read from the base table.          */
/* NOTES:   Single-column histograms are       */
/*          directly generated from data read. */
/*          Multi-Columns are later computed.  */
/* RETCODE:  0 - successful                    */
/*          -1 - failure                       */
/***********************************************/
Lng32 HSGlobalsClass::CollectStatistics()
  {
    Lng32 retcode = 0;
    HSColGroupStruct *group = singleGroup;
    HSCursor *cursor;
    NAString textForColumnCast;
    NAString columnName, dblQuote="\"";
    const Lng32 maxCharBoundaryLen = 
      (Lng32) CmpCommon::getDefaultNumeric(USTAT_MAX_CHAR_BOUNDARY_LEN);
    HSLogMan *LM = HSLogMan::Instance();
    NABoolean useSampling = sampleOptionUsed == TRUE ||
                            CmpCommon::getDefault(USTAT_FORCE_TEMP) == DF_ON;
    HSSample sampleTable(objDef, optFlags & SAMPLE_REQUESTED, sampleTblPercent);
      // Initialize variables for sample table.  May not be used.

    // set CQD for Hive if needed
    setHiveMaxStringLengthInBytes();

    /*======================================================================*/
    /* Perform internal sort if enabled.                                    */
    /*======================================================================*/
    // If internal sort is enabled (it is by default), each iteration of the
    // loop below reads in as many columns from the table as will fit in the
    // amount of memory we determine we can afford to use, and creates
    // histograms using internal sort on those columns.
    // selectSortBatch() chooses the columns to process in a query and marks them
    // PENDING.  These columns are marked PROCESSED in createStats() so they
    // will not be further considered. If any columns are unable to be processed
    // by internal sort due to lack of memory (resulting from fluctuating memory
    // availability or a miscalculation of memory available), they will be
    // processed in the "old" way.
    Int32 numColsSelected = 0;
    Int32 numColsToProcess = 0;
    NABoolean internalSortWhenBetter = FALSE;

    // Check to see if sample table specified via CQD (for debugging and testing).
    // CQD must specify the fully qualified name and must use 'SAMPLE xxx ROWS'
    // where xxx is the size of the specified sample table.
    NAString sampleTableFromCQD;
    CmpCommon::getDefault(USTAT_SAMPLE_TABLE_NAME, sampleTableFromCQD, FALSE);
    NABoolean useBackingSample = (CmpCommon::getDefault(USTAT_USE_BACKING_SAMPLE) == DF_ON);
    if (useBackingSample)
      {
        externalSampleTable = TRUE;
        sampleTableUsed     = TRUE;
        samplingUsed        = TRUE;
        *hssample_table = "HIVE.HIVE.";
        NAString hiveSampleTableName = user_table->data();
        TrafToHiveSampleTableName(hiveSampleTableName);
        hssample_table->append(hiveSampleTableName);
        snprintf(LM->msg, sizeof(LM->msg), "Using external sample table %s.",
                 hssample_table->data());
        LM->Log(LM->msg);
      }
    else if (! IsNAStringSpaceOrEmpty(sampleTableFromCQD))
      {
        *hssample_table = sampleTableFromCQD;
        externalSampleTable = TRUE;
        sampleTableUsed     = TRUE;
        samplingUsed        = TRUE;
      }
    else if (optFlags & IUS_PERSIST)  // PERSIST keyword given with sample clause
      {
        // If all the user columns in the table are LOB columns, our sample table
        // would have zero columns. So return an error for that case.
        if (objDef->allUserColumnsAreLOBs())
          {
            HSFuncMergeDiags(- UERR_ALL_LOB_COLUMNS);
            return -1;
          }

        // Create a persistent sample table. It will be used for this non-IUS
        // execution of Update Stats, and updated incrementally for subsequent
        // IUS operations.
        HSPersSamples *sampleList = HSPersSamples::Instance(objDef->getCatName(),
                                                            objDef->getSchemaName());
        if (!sampleList)
          return -1;  // sample list didn't exist and failed creation
        retcode = sampleList->createAndInsert(objDef, 
                                    *hssample_table, 
                                    sampleRowCount, actualRowCount, 
                                    TRUE, /* isEstimate */
                                    'I',  /* incremental update stats */
                                    TRUE, /* create addtional D and I 
                                            tables for IUS, used by
                                            algorithm 2.  */
                                    minRowCtPerPartition_
                                              );
        if (retcode == 0)
          {
            externalSampleTable = TRUE;
            sampleTableUsed     = TRUE;
            samplingUsed        = TRUE;
          }
        else
          return retcode;
      }
    else   
      externalSampleTable = FALSE;

    if (useBackingSample)
      {
        return CollectStatisticsWithFastStats();
      }
    else if (canDoIUS())
      {
        // Use IUS and use output param 'done' to indicate if we need to carry
        // on with RUS code below, or if IUS did it all and we can return.
        NABoolean done = FALSE;
        retcode = doIUS(done);
        HSHandleError(retcode);
        if (done)
          return retcode;

        // Set sampling parameters to do an RUS corresponding to the existing
        // persistent sample.
        useSampling = TRUE;
        externalSampleTable = TRUE;
        sampleTblPercent = sampleRateAsPercetageForIUS * 100;  // used for scaling results
      }

    NAString internalSortCQDValue = ActiveSchemaDB()->getDefaults().getValue(USTAT_INTERNAL_SORT);

    if (internalSortCQDValue == "OFF" )
      {
        // internal sort disabled
        if (LM->LogNeeded()) LM->Log("Internal sort is disabled");
        if (useSampling && !externalSampleTable)
          retcode = sampleTable.make(currentRowCountIsEstimate_,
                                     *hssample_table,
                                     actualRowCount, sampleRowCount);
            // hssample_table assigned, actualRowCount and sampleRowCount may get adjusted.
        else if (!externalSampleTable)
          {
            *hssample_table = getTableName(user_table->data(), nameSpace);
            sampleRowCount = actualRowCount;
          }
        HSHandleError(retcode);
      }
    else  // internal sort is enabled 
      {
        // Figure out which groups are eligible for varchar compaction:
        // A varchar column is eligible if CQD USTAT_COMPACT_VARCHARS is 'ON'
        // and that column is not referenced by any multi-column group.
        // The reason for the latter condition is we want to avoid the
        // possibility of doing a second full table sample scan in the event
        // that we attempt to do multi-column histograms in-memory, and
        // we underestimate the memory needed for internal sort.

        NABoolean varcharCompactionFeasible = FALSE;
        if (CmpCommon::getDefault(USTAT_COMPACT_VARCHARS) == DF_ON)
          {
            NABitVector * refdColumns = new (STMTHEAP) NABitVector (STMTHEAP);
            for (HSColGroupStruct * mcgrp = multiGroup; mcgrp; mcgrp = mcgrp->next)
              {
                for (Int32 i = 0; i < mcgrp->colCount; i++)
                  {
                    HSColumnStruct *c = &mcgrp->colSet[i];
                    refdColumns->setBit(c->colnum);
                  }
              }

            for (HSColGroupStruct * sgrp = singleGroup; sgrp; sgrp = sgrp->next)
              {
                if (!refdColumns->contains(sgrp->colSet[0].colnum))
                  {
                    sgrp->eligibleForVarCharCompaction = TRUE;
                    varcharCompactionFeasible = TRUE;
                  }
              }

            delete refdColumns;
          }

        // Get percentage of available memory to recommend. If an allocation
        // for memory for a column fails, this percentage will be reduced
        // for subsequent selection of column batches.
        //
        ISMemPercentage_ = (float)CmpCommon::getDefaultNumeric(USTAT_IS_MEMORY_FRACTION);

        NABoolean trySampleTableBypassForIS = useSampling && externalSampleTable == FALSE;

        // If we are considering varchar compaction and IS, get previous histogram
        // information here.
        if (varcharCompactionFeasible)
          getPreviousUECRatios(singleGroup);

        mapInternalSortTypes(singleGroup);
        Int64 maxRowsToRead = getInternalSortMemoryRequirements(TRUE); 

        if (trySampleTableBypassForIS && multiGroup ) {

              if (CmpCommon::getDefault(USTAT_USE_INTERNAL_SORT_FOR_MC) == DF_ON &&
                  allGroupsFitInMemory(maxRowsToRead)) 
              {
                // if both single and MC groups can fit in memory, turn on
                // performing MC in memory flag.
                //
                // Do not have to set the flag for next call to this function 
                // (HSGlobalsClass::CollectStatistics()) since it is not static and
                // therefore a new HSGlobalsClass object will be constructed.
                // In HSGlobalsClass::HSGlobalsClass(), the flag is set to FALSE.
                setPerformISForMC(TRUE);

              } else {
                // othereise, don't bypass sampling for internal sort.
                trySampleTableBypassForIS = FALSE;
              } 
         }

        if (useSampling && sampleRowCount <= getMinRowCountForSample())
          internalSortWhenBetter = FALSE;                              // always use internal sort.
        else 
          internalSortWhenBetter = (internalSortCQDValue == "HYBRID"); // use best method.

        if (LM->LogNeeded())
        {
           sprintf(LM->msg, "Internal sort is enabled with value (%s) and internalSortWhenBetter is (%s)",
                            internalSortCQDValue.data(), internalSortWhenBetter ? "TRUE" : "FALSE");
           LM->Log(LM->msg);
        }

        // Set CQDs for internal sort:
        // Do not limit precision, this can cause internal sort failure.
        retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT LIMIT_MAX_NUMERIC_PRECISION 'OFF'");
        HSHandleError(retcode);

        // identify the columns that are used in MC
        HSColGroupStruct *sgroup = NULL;
        HSColGroupStruct *mgroup = multiGroup;
        // backup of the original order of columns in the single column group list
        HSColGroupStruct* s_group_back[singleGroupCount];
        HSColumnStruct   *col;

        if ( performISForMC() ) 
        {
           while (mgroup != NULL)
           {
              mgroup->mcis_colsUsedMap = new (STMTHEAP) NABitVector (STMTHEAP);
              mgroup->mcis_colsMissingMap = new (STMTHEAP) NABitVector (STMTHEAP);
              Int32 colCount = mgroup->colCount;
              Int32 pos = 0;
              for (Int32 i=0; i<colCount; i++)
              {
                 col = &mgroup->colSet[i];
                 sgroup = findGroupAndPos(col->colnum, pos);
                 sgroup->mcs_usingme++;
                 mgroup->mcis_colsUsedMap->setBit(pos);
              }
              mgroup = mgroup->next;
           }
           mgroup = multiGroup;
 
           sgroup = singleGroup;
           Int32 i = 0;
           while (sgroup != NULL)
           {
              s_group_back[i++] = sgroup;
              sgroup = sgroup->next;
           }
           sgroup = NULL;
        }

        // If we need UEC ratios info and we haven't already read it, do so now
        if (internalSortWhenBetter && !varcharCompactionFeasible)
          getPreviousUECRatios(singleGroup);  // used to decide when to use IS

        
        if ( performISForMC() ) 
        {
           // reorder the single group columns if any MC group can be computed in memory
           orderMCGroups(s_group_back);
           mgroup = multiGroup;
        }

        numColsToProcess = getColsToProcess(maxRowsToRead,
                                              internalSortWhenBetter,
                                              trySampleTableBypassForIS);
        NABoolean hbaseCQDsUsed = FALSE;

        if (trySampleTableBypassForIS && numColsToProcess == singleGroupCount)
          {
              // This is not performed when there are MC stats to process.
              if (LM->LogNeeded())
                LM->Log("Internal sort: reading sample directly from base table; no sample table created");
              *hssample_table = getTableName(user_table->data(), nameSpace);
                // sampleTblPercent and sampleRowCount may get adjusted.
              createSampleOption(optFlags & SAMPLE_REQUESTED,
                                 sampleTblPercent, *sampleOption,
                                 sampleValue1, sampleValue2);
              sampleTableUsed = FALSE;
              samplingUsed    = TRUE;

              // Set CQDs controlling HBase cache size (number of rows returned by HBase
              // in a batch) to avoid scanner timeout. Reset these after the sample query
              // has executed.
              if (isHbaseTable)
                hbaseCQDsUsed = HSGlobalsClass::setHBaseCacheSize(sampleTblPercent);
          }
        else
          {
              if (useSampling && !externalSampleTable)
              {
                // free column memory, to allow sample table load to use it
                deallocatePendingMemory();
                
                // create and populate the sample table
                retcode = sampleTable.make(currentRowCountIsEstimate_,
                                           *hssample_table,
                                           actualRowCount, sampleRowCount);
                // hssample_table assigned, actualRowCount and sampleRowCount may get adjusted.
                
                HSHandleError(retcode);

                // reallocate column memory
                numColsToProcess = getColsToProcess(maxRowsToRead,
                                              internalSortWhenBetter,
                                              trySampleTableBypassForIS);
              }
              else if (!externalSampleTable)
              {
                *hssample_table = getTableName(user_table->data(), nameSpace);
                sampleRowCount = actualRowCount;
              }          
          }

        while (numColsToProcess > 0)
          {
            HSCursor cursor;
            (void)getTimeDiff(TRUE);

            LM->StartTimer("RUS: Read/sort data before creating STATISTICS");
   
            retcode = readColumnsIntoMem(&cursor, maxRowsToRead);
            HSHandleError(retcode);
            checkTime("after reading pending columns into memory for internal sort");
            columnSeconds = getTimeDiff() / numColsToProcess;  // saved for automation


            if (sampleRowCount == 0) // cannot generate histograms
            {   
              HSFuncMergeDiags(-UERR_SAMPLE_SET_IS_ZERO);
              retcode = -1;
              HSHandleError(retcode);
            }

            retcode = sortByColInMem();
            HSHandleError(retcode);

            LM->StopTimer();

            LM->StartTimer("RUS: Create statistics for internal sort");
            retcode = createStats(maxRowsToRead);
            HSHandleError(retcode);
            LM->StopTimer();

            if ( performISForMC() &&
                !allMCGroupsProcessed(TRUE)
               )
            {
               LM->StartTimer("MC: Compute MC stats using Internal Sort");
               retcode = ComputeMCStatistics(TRUE);
               LM->StopTimer();
               if ( performISForMC() && !allMCGroupsProcessed(TRUE)
                  )
               {
                  if (LM->LogNeeded())
                  {
                     sprintf(LM->msg, "\tMC: re-invoking the orderMCGroups logic");
                     LM->Log(LM->msg);
                  }
                  orderMCGroups(s_group_back);
                  mgroup = multiGroup;
               }
            }

            numColsToProcess = getColsToProcess(maxRowsToRead, internalSortWhenBetter);
          }

        HSFuncExecQuery("CONTROL QUERY DEFAULT LIMIT_MAX_NUMERIC_PRECISION RESET");
      }

                                          /*=================================*/
                                          /*         READ / GENERATE         */
                                          /*    SINGLE-COLUMN HISTOGRAMS     */
                                          /*=================================*/
    //All columns handled using internal sort above have been marked as "processed".
    //Remaining columns are handled with a separate query for each column, which
    //does the sorting and grouping.
    //The query generated is:
    //      SELECT column, COUNT(*) FROM table GROUP BY column ORDER BY column
    //The result will always be a VARCHAR(len) CHARACTER SET UCS2
    //In most cases, this will reduce the number of fetches.

    if (CmpCommon::getDefault(USTAT_ATTEMPT_ESP_PARALLELISM) == DF_OFF)
      HSFuncExecQuery("CONTROL QUERY DEFAULT ATTEMPT_ESP_PARALLELISM 'OFF'");

    group = singleGroup;
    if (singleGroup && LM->LogNeeded())
      LM->StartTimer("Query sort/group for individual columns");
    while (group != NULL)
      {
        if (group->state == PROCESSED)
          {
            group = group->next;
            continue;
          }

        HS_ASSERT(group->state != PENDING);

        if (LM->LogNeeded())
          {
            sprintf(LM->msg, "Using query sort/group for column %s",
                             group->colSet[0].colname->data());
            LM->Log(LM->msg);
          }

        // Surround column name with double quotes, if not already delimited.
        if (group->colNames->data()[0] == '"') 
          columnName=group->colNames->data();
        else                                   
          columnName=dblQuote+group->colNames->data()+dblQuote;

        //We must use TRANSLATE to convert non-unicode character strings
        //to unicode
        NABoolean isVarChar = group->computeAvgVarCharSize();
        // For long character strings, we'll truncate (trading off some
        // UEC accuracy for performance and also avoiding engine bugs
        // pertaining to very long varchars).
        HSColumnStruct &col = group->colSet[0];
        bool isOverSized = DFS2REC::isAnyCharacter(col.datatype) &&
              (col.length > maxCharColumnLengthInBytes);

        if (isVarChar)
         *group->clistr = "SELECT FMTVAL, SUMVAL, AVGVAL FROM (SELECT ";
        else
         *group->clistr = "SELECT FMTVAL, SUMVAL FROM (SELECT ";
        group->clistr->append(columnName.data());
        group->clistr->append(group->generateTextForColumnCast());
        if (isVarChar) 
        {
          group->clistr->append(", COUNT(*), AVG(OCTET_LENGTH(");
          group->clistr->append(columnName.data());
          group->clistr->append(")) FROM ");
        }
        else
         group->clistr->append(", COUNT(*) FROM ");
   
        Int64 hintRowCount =  0;

        if (sampleTableUsed)
        {
          hintRowCount = sampleRowCount;
        }
        else
        {
          hintRowCount = actualRowCount;
        }

        char cardHint[50];
        sprintf(cardHint, " <<+ cardinality %e >> ", (double)hintRowCount);

        if (isOverSized)
        {
          // Stick in a nested select that truncates the string.
          // We have to do it here so the truncated string is
          // the grouping column below.
          char temp[20];  // long enough for 32-bit integer
          sprintf(temp,"%d",maxCharColumnLengthInBytes);

          group->clistr->append("(SELECT SUBSTR(");
          group->clistr->append(columnName.data());
          group->clistr->append(" FOR ");
          group->clistr->append(temp);
          group->clistr->append(") AS ");
          group->clistr->append(columnName.data());
          group->clistr->append(" FROM ");
          group->clistr->append(hssample_table->data());
          group->clistr->append(cardHint);
          group->clistr->append(") AS T1");
        }
        else
        {
          group->clistr->append(hssample_table->data());
          group->clistr->append(cardHint);
        }

        group->clistr->append(" GROUP BY ");
        group->clistr->append(columnName.data());
        group->clistr->append(" FOR SKIP CONFLICT ACCESS) T(");
        group->clistr->append(columnName.data());
        if (isVarChar)
        {
          group->clistr->append(", FMTVAL, SUMVAL, AVGVAL) ORDER BY ");
        }
        else
         group->clistr->append(", FMTVAL, SUMVAL) ORDER BY ");
        group->clistr->append(columnName.data());

        cursor = new(STMTHEAP) HSCursor;
       
        char msg_buf[1000];
        sprintf(msg_buf, "RUS: create Single-column stats: fetchBoundaries() for %s", 
                group->colSet[0].colname->data());
        LM->StartTimer(msg_buf);
        (void)getTimeDiff(TRUE);
        retcode = cursor->fetchBoundaries(group,
                                          sampleRowCount,
                                          intCount,
                                          samplingUsed);
        group->colSecs = getTimeDiff();
        LM->StopTimer();
        delete cursor;
        HSHandleError(retcode);

        //10-030702-7560: There is no need to continue processing columns when
        //an empty table is detected.
        if (sampleRowCount == 0)
          {
            actualRowCount = 0;
            break;
          }
                                          /*=================================*/
                                          /*       FIX SAMPLING COUNTS       */
                                          /* sampled UEC -> est UEC          */
                                          /* sampled ROWCOUNT -> est ROWCOUNT*/
                                          /*=================================*/
        if (samplingUsed && sampleRowCount > 0 && actualRowCount > sampleRowCount)
          {
            LM->StartTimer("fix sample row counts");
            retcode = FixSamplingCounts(group);
            HSHandleError(retcode);
            LM->StopTimer();
            if (group->groupHist) group->groupHist->deleteFiArray();
          }
        group = group->next;
      }

    // If the current row count for an Hbase table is an estimate, then
    // actualRowCount is the estimate of the row count given by HBase. This
    // estimate can sometimes be inaccurate. Now that we have actually read
    // the data, we can improve the estimate. If we used sampling, we can
    // divide our sampleRowCount by the sampling ratio. If we did not use
    // sampling, the sampleRowCount is the true row count.

    // Note: After a recent code change, we no longer do an estimate
    // when not doing sampling. So the "else" case below is actually dead
    // code. But I'm leaving the code here on the chance that we change
    // our minds about estimates in the non-sampling case.

    if (isHbaseTable && currentRowCountIsEstimate_)
      {
        if (samplingUsed)
          {  
            HS_ASSERT(sampleTblPercent > 0 && sampleTblPercent <= 100.00);
            Int64 newActualRowCount = (Int64)((100 * sampleRowCount) / sampleTblPercent);
            if (LM->LogNeeded())
              {
                sprintf(LM->msg, "Re-estimating actualRowCount (was " PF64 ") as " PF64,
                                 actualRowCount,newActualRowCount);
                LM->Log(LM->msg);
              }
            actualRowCount = newActualRowCount;           
          }
        else
          {
            if (LM->LogNeeded())
              {
                sprintf(LM->msg, "Correcting actualRowCount (was " PF64 ") from sampleRowCount (" PF64 ")",
                                 actualRowCount,sampleRowCount);
                LM->Log(LM->msg);
              }
            actualRowCount = sampleRowCount;
          }
      }         

    if (singleGroup && LM->LogNeeded())
      LM->StopTimer();

                                          /*=================================*/
                                          /*             COMPUTE             */
                                          /*     MULTI-COLUMN HISTOGRAMS     */
                                          /*=================================*/
    //10-040220-3429
    //Since Multi-Column statistics are dependent on Single-Column statistics,
    //we must make sure that Single-Column statistics exist. This can easily be
    //done by checking that the rowcount is greater than zero.
    if (multiGroup != NULL && actualRowCount > 0)
      {


        if (!allMCGroupsProcessed())
        {
           LM->StartTimer("MC: Compute MC stats using SQL");
           retcode = ComputeMCStatistics();
           LM->StopTimer();
        }
        else if (LM->LogNeeded())
        {
           LM->Log("MC: No MCs to compute using SQL, all processed using Internal Sort");
        }


        HSHandleError(retcode);

        LM->StartTimer("MC: fix MC stats");

        if (samplingUsed && sampleRowCount > 0 && actualRowCount > sampleRowCount)
          {
            group = multiGroup;
            while (group != NULL)
            {
              retcode = FixSamplingCounts(group);
              HSHandleError(retcode);
              if (group->groupHist) group->groupHist->deleteFiArray();
              group = group->next;
            }
          }
        LM->StopTimer();
      }

    if (CmpCommon::getDefault(USTAT_ATTEMPT_ESP_PARALLELISM) == DF_OFF)
      HSFuncExecQuery("CONTROL QUERY DEFAULT ATTEMPT_ESP_PARALLELISM RESET");

    if (samplingUsed || (optFlags & ROWCOUNT_OPT))
      {
        //In combination of SAMPLE and "SELECT <col>, COUNT(*)", we may read
        //more rows than actually specified. We need to make sure that the
        //rowcount is no less than the number of rows actually read.
        //For example: The table has 18 rows. The statement
        //"SAMPLE 10 ROWS SET ROWCOUNT 10", will read 100% of table. Since the
        //rowcount was specified, actualrowcount=10. Although, the actual
        //number of rows read is 18(samplerowcount). Rowcount must always be
        //greater than or equal to samplerowcount.
        actualRowCount = MAXOF(actualRowCount, sampleRowCount);
      }

    // Reset sampling variables.
    sampleTableUsed = FALSE;
    samplingUsed = FALSE;

    return retcode;
  }

// Do the setup work for IUS, and call either doFullIUS() or prepareToUsePersistentSample(),
// depending on whether the USTAT_INCREMENTAL_UPDATE_STATISTICS cqd is ON or
// SAMPLE. In the latter case prepareToUsePersistentSample() is called to update
// the persistent sample incrementally, and then the normal Update Stats algorithm
// executes using the persistent sample table.
//
// The 'done' parameter is returned with a value of TRUE if stats are  completely
// handled by doFullIUS(). If prepareToUsePersistentSample() is called instead,
// or if doFullIUS() is unable to incrementally update the stats for one or more
// columns (e.g., shape test failure), 'done' will be set to FALSE.
Lng32 HSGlobalsClass::doIUS(NABoolean& done)
{
  done = FALSE;  // set to TRUE if IUS successfully updates the stats for all columns
  Lng32 retcode = 0;

  // Make sure the Where clause doesn't contain any constructs we don't allow
  // in the context of an IUS statement.
  retcode = validateIUSWhereClause();
  HSHandleError(retcode);

  retcode = begin_IUS_work();
  HSHandleError(retcode);

  Int64 currentSampleSize = 0;
  Int64 futureSampleSize = 0;

  retcode = computeSampleSizeForIUS(currentSampleSize, futureSampleSize);
  HSHandleErrorIUS(retcode);

  DefaultToken iusOption = CmpCommon::getDefault(USTAT_INCREMENTAL_UPDATE_STATISTICS);
  if (iusOption == DF_ON)
    return doFullIUS(currentSampleSize, futureSampleSize, done);
  else if (iusOption == DF_SAMPLE)
    // Leave 'done' FALSE; prepareToUsePersistentSample() updates the persistent sample
    // table in preparation for use by RUS.
    return prepareToUsePersistentSample(currentSampleSize, futureSampleSize);
  else
    {
      // Exception will be thrown, ~HSGlobalsClass will call end_IUS_work().
      HS_ASSERT(false);
      return -1;  // avoid 'no return' warning
    }
}

// Try to incrementally update existing histograms using in-memory tables and
// CBFs. If reversion to RUS is required for one or more columns, the 'done'
// output parameter will be set to FALSE.
Lng32 HSGlobalsClass::doFullIUS(Int64 currentSampleSize,
                                Int64 futureSampleSize,
                                NABoolean& done)
{
  done = FALSE;  // unless IUS handles all columns
  HSLogMan* LM = HSLogMan::Instance();
  Lng32 retcode = 0;
  ISMemPercentage_ = (float)CmpCommon::getDefaultNumeric(USTAT_IS_MEMORY_FRACTION);

  // Setup the memory requirement for singlegroup in each memNeeded field,
  // Since singleGroup will receive the computed new stats, we compute
  // the memory needs by using futureSampleSize.
  mapInternalSortTypes(singleGroup);
  getMemoryRequirements(singleGroup, futureSampleSize);

  // =====================================================================
  // create the inMemory delete table, the cstr also setups the memory
  // requirement utilizing the currentSampleSize as #rows.
  // =====================================================================
  iusSampleDeletedInMem = new(STMTHEAP)
                             HSInMemoryTable(*hssample_table,
                                         getWherePredicateForIUS(),
                                         currentSampleSize
                                        );

  // =====================================================================
  // Similarly, create the inMemory insert table.
  // =====================================================================
  NAString sampleTable_I(*hssample_table);
  sampleTable_I.append("_I");

  iusSampleInsertedInMem = new(STMTHEAP)
                            HSInMemoryTable(sampleTable_I,
                                         getWherePredicateForIUS(),
                                         futureSampleSize,// worse case
                                         sampleRateAsPercetageForIUS);

  // =====================================================================
  // Create and populate the sample _I table
  // =====================================================================

  if (!sampleIExists_)
    {
      retcode = create_I(*hssample_table);
      HSHandleError(retcode);
      sampleIExists_ = TRUE;  // so we remember to drop it
    }

  if (!sample_I_generated)
    {
      retcode = generateSampleI(currentSampleSize, futureSampleSize);
      HSHandleError(retcode);
      sample_I_generated = TRUE;
    } 

  // =====================================================================
  // Find out which group has persistent CBFs and set the delayedRead flag
  // for it. Do it for all groups.
  // =====================================================================
  detectPersistentCBFsForIUS(*hssample_table, singleGroup);



  Int32 colsSelected = 0;
  NABoolean ranOutOfMem = FALSE;

  while ( moreColsForIUS() > 0 ) {

     // Select a set of columns for IUS, based on the availability
     // of memory. Selected columns will be marked in PENDING state.
     // The number of columns selected is returned.
     //
     // If a particular column has persistent CBF, its column
     // structure's delayedRead is set to TRUE.
     //
     retcode = selectIUSBatch(currentSampleSize, futureSampleSize, ranOutOfMem, colsSelected);
     HSHandleErrorIUS(retcode);
     checkTime("after selecting batch of columns for IUS");

     //
     // Require at least one column in the persistent sample table
     // to be read into memory in one batch
     //
     if ( colsSelected == 0 ) {
       if ( ranOutOfMem ) {
         if (LM->LogNeeded())
           {
             // only do the warning diagnostic if logging is enabled
             diagsArea << DgSqlCode(UERR_WARNING_IUS_INSUFFICIENT_MEMORY)
                       << DgInt0(moreColsForIUS());
           }        
       } else {
         if (LM->LogNeeded())
           LM->Log("Empty IUS batch, not because of memory.");
       }
       break;  // Let RUS handle the rest
     }


     // process column groups that are in PENDING state
     // Data from columns with delayedRead set to FALSE (i.e., no
     // corresponding persistent CBFs) will be read in.
     //
     // The rows to be deleted in one shot later on will be read in
     // from the sample table.
     //
     retcode = CollectStatisticsForIUS(currentSampleSize, futureSampleSize);
     HSHandleErrorIUS(retcode);

     //
     // Fall back to let internal sort based RUS to take care of any
     // groups failing to be updated via IUS. These groups are in PENDING
     // state. If all groups are processed, no more read and we are done!
     //
     HSColGroupStruct *group = singleGroup;
     Int32 cols = 0;
     Int32 colsToRead = 0;

     // First mask out those groups that already have data read in.
     // These groups should have delayedRead flag set to FALSE.
     //
     // We do need to properly merge the data from S(i-1), D and I
     // together, so that group->data points at the merged data and
     // group->nextdata points at the end+1 of the merged data.
     //
     // The merge algorithm:
     //
     // Allocate a temp. buffer of size (|S(i-1)| + |I|)
     // For all data item v in S(i-1) and I do
     //    if ( v in cbf ) {
     //       append data to the temp. buffer
     //       remove one instance of v from cbf
     //    }
     // delete group->data
     // set group->data = temp. buffer
     // set group->nextdata = temp. buffer's size + 1

     while (group) {
       if (group->state==PENDING) {

          if ( group->delayedRead == FALSE ) {
             group->state=SKIP;
          } else
             colsToRead++;

          cols++;
       }
       group = group->next;
     }


     if ( cols > 0 ) {

       if (LM->LogNeeded())
         LM->StartTimer("IUS: process failed groups with RUS");

       // First we need to read in column data if it has not been
       // read previously (i.e., those with delayedRead is TRUE, or
       // it has persistent CBF).
       if ( colsToRead > 0 )
       {
          HSCursor cursor;
          // Read from the persistent sample table and use smplGroup to
          // hold the data read. All columns in PENDING state
          // will be read in.
          retcode = readColumnsIntoMem(&cursor, currentSampleSize);

          HSHandleErrorIUS(retcode);
          checkTime("after reading pending columns from persistent sample table into memory for IUS->RUS reversion");

       }

       group = singleGroup;
       while (group) {
         if (group->state==SKIP) {
            group->state=PENDING;
         }
         group = group->next;
       }

       // Fill each group (in PENDING state)'s data area with data merged from
       // cbf, S(I-1) and I.
       retcode = mergeDatasetsForIUS();
       HSHandleErrorIUS(retcode);
       checkTime("after merging datasets for IUS->RUS reversion");

       group = singleGroup;
       while (group) {
         if (group->state==PENDING) {

            // Delete the histogram allocated for IUS. The RUS step
            // below will recompute groupHist.
            delete group->groupHist;
            group->groupHist = NULL;
         }
         group = group->next;
       }


       // Remove all persistent CBFs with PENDING state
       // because RUS may generates a histogram with different
       // #intervals than that recorded in CBF! If the CBFs
       // are left undeleted, the encoded intervals (as
       // # of buckets) in CBF can be in conflict with
       // the actual # of intervals computed by RUS.
       retcode = deletePersistentCBFsForIUS(*hssample_table, singleGroup, PENDING);
       HSHandleErrorIUS(retcode);

       retcode = sortByColInMem();
       HSHandleErrorIUS(retcode);

       retcode = createStats(0 /* dummy argument */);
       HSHandleErrorIUS(retcode);

       if (LM->LogNeeded())
         LM->StopTimer();
     }
  }  // while ( moreColsForIUS() > 0 )

  // The _I table can be dropped after using it to update the persistent sample
  // table, which must be done before doing RUS on any unprocessed columns (RUS
  // will use the updated persistent sample).
  retcode = UpdateIUSPersistentSampleTable(currentSampleSize, futureSampleSize, sampleRowCount);
  HSHandleErrorIUS(retcode);
  if (sampleIExists_) {
    retcode = drop_I(*hssample_table);
    HSHandleErrorIUS(retcode);
    sampleIExists_ = FALSE;  // only try to drop it once
  }

  Int32 iusUnprocessed  = 0;
  // Reverse the NO_STATS state to UNPROCESSED and count
  // total unprocessed
  HSColGroupStruct *group = singleGroup;
  while (group != NULL) {
    if (group->state == NO_STATS ) {
       group->state = UNPROCESSED;
       iusUnprocessed ++;
    } else
    if (group->state == UNPROCESSED )
       iusUnprocessed ++;

      group = group->next;
  }

  // Leave the 'done' parameter FALSE so we continue with the RUS code upon return
  // if there are unprocessed columns (not enough memory or no prior stats as a
  // base to compute IUS), or if there are MCs to process.
  if ( iusUnprocessed > 0 )  {
    // Remove all persistent CBFs with UNPROCESSED state because RUS may generate
    // a histogram with different #intervals than that recorded in CBF! If the CBFs
    // are left undeleted, the encoded intervals (as # of buckets) in CBF can be in
    // conflict with the actual # of intervals computed by RUS.
    retcode = deletePersistentCBFsForIUS(*hssample_table, singleGroup, UNPROCESSED);
    HSHandleErrorIUS(retcode);
  } else if (multiGroup) { // not done if there are MCs to process
  } else {
    done = TRUE;  // no need to use RUS code
  }

  return retcode;
}

// This function makes all preparations for doing RUS using an updated IUS
// persistent sample table. The sample table is updated, and obsolete CBFs
// are discarded,
Lng32 HSGlobalsClass::prepareToUsePersistentSample(Int64 currentSampleSize,
                                                   Int64 futureSampleSize)
{
  Lng32 retcode = 0;
  retcode = UpdateIUSPersistentSampleTable(currentSampleSize, futureSampleSize, sampleRowCount);
  HSHandleErrorIUS(retcode);

  // If there are existing CBFs, they will be obsolete once the current operation
  // completes.
  retcode = deletePersistentCBFsForIUS(*hssample_table, singleGroup, UNPROCESSED);
  HSHandleErrorIUS(retcode);

  return retcode;
}

//
// A help function to generate a SQL timestamp constant. Example: '2012-01-01 23:59:00'
//
// bdt: input argument representing a break-down time value
// timestamp: output argument representing the SQL timestamp constant 
//
void genSQLTimestampConstant(struct tm * bdt, NAString& timestamp)
{
  char buf[100];

  // year
  str_itoa(bdt->tm_year+1900, buf); timestamp = buf;

  timestamp += "-";

  // tm_mon is in  [0, 11]
  if ( bdt->tm_mon < 9 ) timestamp += "0"; // < rather than <= since we add one in the next line
  str_itoa(bdt->tm_mon+1, buf); timestamp += buf;

  timestamp += "-";

  // tm_mday is in [1, 31]
  if ( bdt->tm_mday <= 9 ) timestamp += "0";
  str_itoa(bdt->tm_mday, buf); timestamp += buf;

  timestamp += " ";

  // tm_hour is in [0, 23]
  if ( bdt->tm_hour <= 9 ) timestamp += "0";
  str_itoa(bdt->tm_hour, buf); timestamp += buf;

  timestamp += ":";

  // tm_min is in [0, 59]
  if ( bdt->tm_min <= 9 ) timestamp += "0";
  str_itoa(bdt->tm_min, buf); timestamp += buf;

  timestamp += ":";

  // tm_sec is in [0, 59]
  if ( bdt->tm_sec <= 9 ) timestamp += "0";
  str_itoa(bdt->tm_sec, buf); timestamp += buf; 

}

void genArkcmpInfo(NAString& nidpid)
{
  char buf[100];

  str_itoa(((NAClusterInfoLinux*)gpClusterInfo)->get_nid(), buf);
  nidpid = buf;

  nidpid += ":";

  str_itoa(((NAClusterInfoLinux*)gpClusterInfo)->get_pid(), buf);
  nidpid += buf;

  nidpid += " (nid:pid)";
}

//
// This method starts a long-running (relatively) transaction to serialize IUS work 
// against a target table. The transaction is established by updating the row in the 
// PERSISTENT_SAMPLES table about the persistent sample table used by the IUS:
//   1. UPDATE_DATE field is populated with the current timestamp;
//   2. UPDATER_INFO field is populated with the SQ node ID and process ID
//      of the tdm_arkcmp process performing the IUS work
// The method will return 0 after the above successful updates, indicating that the IUS
// work can proceed.
//
// The transaction can fail to establish when another long-running IUS transaction
// is working against the same target table. The condition can be detected by
// querying the UPDATE_DATA field about the sample table. A non-zero timestamp 
// value (call it P1) indicates an on-going IUS transaction. When P1 
// is sufficiently close to the current timestamp P2 (P2-P1 <= 
// CQD(USTAT_IUS_MAX_TRANSACTION_DURATION)), the ongoing transaction is 
// considered legitimate, and the current call to the method will return an error
// indicating that a concurrent IUS is in progress.
// ius_update_history_buffer will be filled with the string read from the
// UPDATER_INFO column.
// 
// When P2-P1 > CQD(USTAT_IUS_MAX_TRANSACTION_DURATION), the on-going transaction is
// considered over-due and will be discarded. The method proceeds as if there was
// no IUS transaction pending (see the 1st paragraph above) and will return a retcode
// of 0, indicating success.
//
// The querying of these two fields against PERSISTENT_SAMPLE table must be 
// protected by a serializable transaction.
//
// The CQD USTAT_IUS_MAX_TRANSACTION_DURATION specifies the max transaction
// duration allowed with the unit in minutes. The default value is 720 minutes (12 hours).

Lng32 HSGlobalsClass::begin_IUS_work()
{
   sampleIExists_ = FALSE;  // keep track of whether a _I table needs to be dropped

#ifdef _DEBUG
   if (CmpCommon::getDefault(USTAT_IUS_NO_BLOCK) == DF_ON)
     return 0;
#endif 

   HSPersSamples *sampleList = HSPersSamples::Instance(objDef->getCatName(),
                                                       objDef->getSchemaName());
   if ( !sampleList ) return -1;

   Int64 updTimestamp = 0;

   HSTranMan *TM = HSTranMan::Instance();
   TM->Begin("READ AND UPDATE THE UPDATE DATE AND HISTORY from PERSISTENT SAMPLE TABLE");
   
   char ius_update_history_buffer[129];
   Lng32 retcode =
     sampleList->readIUSUpdateInfo(objDef, ius_update_history_buffer, &updTimestamp);
   if (retcode == 100)
     {
       HSFuncMergeDiags(- UERR_IUS_NO_PERSISTENT_SAMPLE,
                        objDef->getObjectFullName().data());
       retcode = -1;
       HSHandleError(retcode);
     }
   else
     {
       HSHandleError(retcode);
     }

   time_t t;

   //
   // A timestamp of 0 means the time stored is the epoch time: 00:00:00 on 
   // January 1, 1970, Coordinated Universal Time (UTC), which is a value
   // indicating no IUS operation is in progress. 
   // 

   time(&t); // Obtain the current time as a timestamp since epoch

   if ( updTimestamp != 0 ) {
      //
      // Assign the value in seconds as the column UPDATE_DATE in 
      // CAT.PUBLIC_ACCESS_SCHEMA.PERSISTENT_SAMPLES has no fractional part 
      // (defined as TIMESTAMP(0)).
      //
      Int32 maxDeltaSeconds = 
        ActiveSchemaDB()->getDefaults().getAsULong(USTAT_IUS_MAX_TRANSACTION_DURATION) * 60;

      if ( (Int64)(t) - updTimestamp < maxDeltaSeconds ) {
        // A legitimate instance of IUS is running. Return error.
        diagsArea << DgSqlCode(-UERR_IUS_IN_PROGRESS) 
                  << DgString0(ius_update_history_buffer);
        return -UERR_IUS_IN_PROGRESS;
      }
   }

   // If we reach here, it means either there is no IUS operation in progress or 
   // a previous IUS operation has spent more time than USTAT_IUS_MAX_TRANSACTION_DURATION,
   // we will update the PST entry with my IUS instance's information and the current
   // timestamp and proceed normally.

   // Break down the current timestamp t into a structure with year, month, day etc
   struct tm * bdt = gmtime(&t);

   NAString updTimestampStr;
   genSQLTimestampConstant(bdt, updTimestampStr);

   NAString nid_pid_str;
   genArkcmpInfo(nid_pid_str);

   retcode =
     sampleList->updIUSUpdateInfo(objDef,
                                  (char*)nid_pid_str.data(),
                                  (char*)updTimestampStr.data(),
                                  0 /* don't write where condition now */);

   if (retcode == 100)
     {
       HSFuncMergeDiags(- UERR_IUS_NO_PERSISTENT_SAMPLE,
                        objDef->getObjectFullName().data());
       retcode = -1;
     }

   HSHandleError(retcode);

   //
   // If we reach here, we have successfully stored the bdt and our process info
   // into the UPDATE_DATE and UPDATER_INFO columns of SB_PERSISTENT_SAMPLES.
   //
   PSRowUpdated = TRUE;
   return 0;
}

//
// This method completes the relatively long-running transaction by updating the
// row describing the persistent sample table in PERSISTENT_SAMPLE table 
// as follows.
//
//   1. UPDATE_DATE field is reset to a timestamp representing epoch time;
//   2. UPDATER_INFO field is reset to an empty string.

Lng32 HSGlobalsClass::end_IUS_work()
{
   Lng32 retcode = 0;
   if (sampleIExists_)
     {
       retcode = drop_I(*hssample_table);
       // ignore retcode; we want to try the rest of this method as well
       sampleIExists_ = FALSE;
     }

#ifdef _DEBUG
   if (CmpCommon::getDefault(USTAT_IUS_NO_BLOCK) == DF_ON)
     return 0;
#endif

   HSPersSamples *sampleList = HSPersSamples::Instance(objDef->getCatName(),
                                                       objDef->getSchemaName());
   if ( !sampleList ) return -1;

   // The epoch time
   time_t t = 0;

   // Break down the timestamp t into a structure with year, month, day etc.
   // Should be '1970-01-01 00:00:00'!
   struct tm * bdt = gmtime(&t);

   NAString updTimestampStr;
   genSQLTimestampConstant(bdt, updTimestampStr);

   retcode = 
     sampleList->updIUSUpdateInfo(objDef,
                                  (char*)"",
                                  (char*)updTimestampStr.data(),
                                  getWherePredicateForIUS(),
                                  PST_IUSrequestedSampleRows_,
                                  PST_IUSactualSampleRows_);
   HSHandleError(retcode);

   return 0;
}

Int32 HSGlobalsClass::moreColsForIUS()
{
   Int32 count = 0;
   HSColGroupStruct *group = singleGroup;

   while (group != NULL)
    {
      if (group->state == UNPROCESSED)
          count++;

       group = group->next;
    }
   return count;

}

//
// Prepare for IUS by performing the following tasks:
//
//   1. read the persistent samples table to find the
//      #sample rows, the requested rows for IUS, and sample rate
//   2. Find the size of the sample table of the previous IUS operation
//      and assign it to argument 'currentSampleSize'
//   2. Compute the size of the sample table for the new IUS operation
//      and assign it to argument 'futureSampleSize'
//   3. set the memNeeded field for each group use the size from 2)
//
Lng32 HSGlobalsClass::computeSampleSizeForIUS(Int64& currentSampleSize, Int64& futureSampleSize)
{
  Lng32 retcode = 0;
  Int64 requestedRowsForIUS = 0;
  currentSampleSize = 0;

  if ( getPersistentSampleTableForIUS(*hssample_table,
                  requestedRowsForIUS, currentSampleSize,
                  sampleRateAsPercetageForIUS) )
    {
      // found the persistent sample table name, actual row-count, sample row-count,
      // and sample rate from persistent_samples table.
      externalSampleTable = TRUE;
      sampleTableUsed     = TRUE;
      samplingUsed        = TRUE;
    }
  else
    {
      HSFuncMergeDiags(- UERR_IUS_NO_PERSISTENT_SAMPLE,
                        objDef->getObjectFullName().data());
      retcode = -1;
      HSHandleError(retcode);
    }

   sampleRowCount = currentSampleSize;

   // Meta-info about the previous IUS iteration is found, compute the # of rows
   // for the new IUS iteration. "actualRowCount" is the size of the
   // source table now. Call it futureSampleSize.
   futureSampleSize = (Int64)(sampleRateAsPercetageForIUS * actualRowCount);

   // keep the sample table monotonically increasing.
   if ( futureSampleSize < currentSampleSize )
      futureSampleSize = currentSampleSize;

   return 0;
}

// Before we have the PERSISTENT_DATA table available to us, we will
// save the CBFs as binary files on disk. One CBF maps to one binary file.
// The path of the directory for these files is specified in CQD
// USTAT_IUS_PERSISTENT_CBF_PATH, and the cbf file name is
// sampleTableName + '.' + 'colName'. This function builds the common initial
// text of the path for all columns in the same table, and assigns it to the
// output parameter filePrefix.
void HSGlobalsClass::getCBFFilePrefix(NAString& sampleTableName, NAString& filePrefix)
{
  filePrefix = ActiveSchemaDB()->getDefaults().getValue(USTAT_IUS_PERSISTENT_CBF_PATH);
  filePrefix.append("/")
            .append(sampleTableName)
            .append(".");
}

void
HSGlobalsClass::detectPersistentCBFsForIUS(NAString& sampleTableName, 
                                           HSColGroupStruct *group)
{
   NAString cbfFilePrefix;
   getCBFFilePrefix(sampleTableName, cbfFilePrefix);
   struct stat sts;
   while (group) {
      NAString cbfFile(cbfFilePrefix);
      cbfFile.append(group->cbfFileNameSuffix());

      if (stat(cbfFile, &sts) == -1 && errno == ENOENT)
        group->delayedRead = FALSE; 
      else
        group->delayedRead = TRUE;
      
      group = group->next;
   }
}

Lng32 HSGlobalsClass::prepareForIUSAlgorithm1(Int64& rows)
{
  Lng32 retcode = 0;
   
  HSLogMan *LM = HSLogMan::Instance();

  if (LM->LogNeeded())
    {
      char intStr[30];
      convertInt64ToAscii(rows, intStr);
      sprintf(LM->msg, "IUS in ::prepareForIUS(): |S(i-1)|=%s rows", intStr);
      LM->Log(LM->msg);
      LM->StartTimer("IUS: compute S(i-1)-D");
    }



  // Update the sample table in two separate transactions.
  Int64 xRows;
 
  // first delete the old rows 
    
  HSHandleError(retcode);
  
  // temp. for now to generate the delQuery, set #rows to 1
  iusSampleDeletedInMem = new(STMTHEAP) 
                              HSInMemoryTable(*hssample_table,
                                               getWherePredicateForIUS(),
                                               1 // rows
                                              );

  NABoolean transactional = (CmpCommon::getDefault(USTAT_DELETE_NO_ROLLBACK) == DF_OFF);
  
  NAString delQuery;
  generateIUSDeleteQuery(*hssample_table, delQuery, transactional);

  if (transactional)
    {
      retcode = HSFuncExecTransactionalQueryWithRetry(delQuery, -UERR_INTERNAL_ERROR, 
                              &xRows,"IUS S(i-1)-D operation",
                              NULL, NULL);
    }
  else
    {
      retcode = HSFuncExecQuery(delQuery, -UERR_INTERNAL_ERROR, 
                              &xRows,"IUS S(i-1)-D operation",
                              NULL, NULL);
    }

  HSHandleError(retcode);

  if (LM->LogNeeded())
     LM->StopTimer();

  rows -= xRows;

   if (LM->LogNeeded())
    {
      char intStr[30];
      convertInt64ToAscii(rows, intStr);
      sprintf(LM->msg, "IUS in ::prepareForIUS(): |S(i-1)-D|=%s rows", intStr);
      LM->Log(LM->msg);
    }



  if (LM->LogNeeded())
     LM->StartTimer("IUS: compute S(i-1) + I");

  { // start a new scope for the trasaction 

    HSTranController TC("IUS: Update S with I", &retcode);
    HSHandleError(retcode);
  
    // temp. for now to generate the insQuery, set #rows to 1
    iusSampleInsertedInMem = new(STMTHEAP)
                              HSInMemoryTable(*user_table,
                                               getWherePredicateForIUS(),
                                               1, // rows,
                                               sampleRateAsPercetageForIUS);
  
    NAString insQuery;
    iusSampleInsertedInMem->generateInsertQuery(*hssample_table, *user_table, insQuery, FALSE);  
  
    // Note that we don't retry the insert
    retcode = HSFuncExecQuery(insQuery, -UERR_INTERNAL_ERROR, 
                               &xRows, "IUS S(i-1)-D+I operation",
                               NULL, NULL);
    HSHandleError(retcode);
  }
  
  if (LM->LogNeeded())
     LM->StopTimer();


  rows += xRows;

   if (LM->LogNeeded())
    {
      char intStr[30];
      convertInt64ToAscii(rows, intStr);
      sprintf(LM->msg, "IUS in ::prepareForIUS(): size of |Si|=%s rows", intStr);
      LM->Log(LM->msg);
    }

  return 0;
}

static Lng32 create_I(NAString& sampTblName)
{
  HSLogMan *LM = HSLogMan::Instance();
  if (LM->LogNeeded())
     LM->StartTimer("IUS: create _I table");

  NAString createI("create table ");
  createI += sampTblName;
  createI += "_I LIKE ";
  createI += sampTblName;
  createI += " WITH PARTITIONS";
  Lng32 retcode = HSFuncExecTransactionalQueryWithRetry(createI, -UERR_INTERNAL_ERROR,
                                  NULL, "IUS create I",
                                  NULL, NULL);
  if (LM->LogNeeded())
     LM->StopTimer();

  HSHandleError(retcode);
  return retcode;
}


Lng32 HSGlobalsClass::generateSampleI(Int64 currentSampleSize,
                                      Int64 futureSampleSize)
{
  Lng32 retcode = 0;
  Int64 xRows;

  HSLogMan *LM = HSLogMan::Instance();
  if (LM->LogNeeded())
     LM->StartTimer("IUS: select-insert data set I");

    // performing: 
    //
    //      upsert using load into <sample_I> 
    //        (select * from <sourceTable> where <where> sample);
    //

    NAString sampleTable_I(*hssample_table);
    sampleTable_I.append("_I");

    HSTranMan *TM = HSTranMan::Instance();
    NABoolean transStarted = (TM->Begin("IUS clean data set I") == 0);

    NAString insertSelectIQuery;
    iusSampleInsertedInMem->generateInsertSelectIQuery(sampleTable_I, 
                        *user_table, insertSelectIQuery,
                        hasOversizedColumns, objDef, 
                        currentSampleSize, futureSampleSize,
                        actualRowCount);

    NABoolean needEspParReset = setEspParallelism(objDef);
    // note that we can't do a retry on non-transactional upsert using load + sample
    // note also the most likely error is a bad WHERE clause
    retcode = HSFuncExecQuery(insertSelectIQuery, 
                              -UERR_IUS_BAD_WHERE_CLAUSE, &xRows,
                              "IUS data set I creation",
                              NULL, NULL,
                              0, TRUE);  // check for MDAM usage

    if (needEspParReset)
      resetEspParallelism();

    if (retcode) TM->Rollback();

    HSHandleError(retcode);

    TM->Commit();

  if (LM->LogNeeded())
    {
      LM->StopTimer();
      sprintf(LM->msg, "the size of data set I is " PF64" rows", xRows);
      LM->Log(LM->msg);
    }

  return 0;
}

static Lng32 drop_I(NAString& sampTblName)
{
  HSLogMan *LM = HSLogMan::Instance();
  if (LM->LogNeeded())
    LM->StartTimer("IUS: drop _I table");

  NAString cleanupI("drop table if exists ");
  cleanupI.append(sampTblName).append("_I");
  Lng32 retcode = HSFuncExecTransactionalQueryWithRetry(cleanupI, -UERR_INTERNAL_ERROR,
                                  NULL, "IUS cleanup I",
                                  NULL, NULL);
  if (LM->LogNeeded())
    LM->StopTimer();
  HSHandleError(retcode);
  return retcode;
}

Lng32 HSGlobalsClass::CollectStatisticsForIUS(Int64 currentSampleSize, 
                                              Int64 futureSampleSIze)
{
  Lng32 retcode = 0;
  Int64    xRows = 0;

  HSLogMan *LM = HSLogMan::Instance();

  NABoolean havePending = FALSE;
  HSColGroupStruct *group = singleGroup;
  while (group)
    {
      if (group->state == PENDING)
        {
          if (group->delayedRead)
            group->state = SKIP; // temp. set so that the column data is not to be read
          else
            havePending = TRUE;
        }
      group = group->next;
    }

  // Only read in Si if there is at least one column in this batch that doesn't
  // already have a persistent CBF.
  if (havePending)
    {
      if (LM->LogNeeded())
        LM->StartTimer("IUS: read in Si");

      // Populate the data areas for the PENDING columns from the persistent
      // sample table.
      HSCursor cursor;  // on block exit, dtor will close/dealloc stmt and descriptors
      retcode = readColumnsIntoMem(&cursor, currentSampleSize);
      HSHandleError(retcode);
      checkTime("after reading pending columns into memory for IUS");

      if (LM->LogNeeded())
        LM->StopTimer();
    }
  else if (LM->LogNeeded())
    LM->Log("IUS: skipped reading in Si; delayedRead true for all columns in batch");

   // restore the state field to PENDING for any skipped groups.
   group = singleGroup;
   while (group) {
       if (group->delayedRead && group->state == SKIP)
           group->state = PENDING;  
       group = group->next;
   }

  // Read in CBFs for groups that are PENDING and delayedRead 
  retcode = readCBFsIntoMemForIUS(*hssample_table,singleGroup);
  HSHandleError(retcode);
  checkTime("after reading CBFs into memory for IUS");


  if (LM->LogNeeded()) 
    LM->StartTimer("IUS: read in data set D");

  // ================================================================
  // This section of code is only needed to support Algorithm 2.
  // ================================================================
  //
  // First loading data set D 
  NAString selectDQuery;
  iusSampleDeletedInMem->generateSelectDQuery(*hssample_table, 
                                               selectDQuery);

  retcode = iusSampleDeletedInMem->populate(selectDQuery);
  HSHandleError(retcode);

  if (LM->LogNeeded()) 
    LM->StopTimer();
 

  if (LM->LogNeeded()) 
    LM->StartTimer("IUS: read in data set I");

  // Next loading data set I. The I sample was generated by our caller.

  NAString selectIQuery;
  iusSampleInsertedInMem->generateSelectIQuery(*hssample_table, 
                                               selectIQuery);
  retcode = iusSampleInsertedInMem->populate(selectIQuery);
  HSHandleError(retcode);
  checkTime("after populating tables for IUS");

  if (LM->LogNeeded()) 
    LM->StopTimer();

  //
  // ================================================================
  // End of the section of code is only needed to support Algorithm 2.
  // ================================================================


  if (LM->LogNeeded()) 
    LM->StartTimer("IUS: compute CBFs and estimate UECs");

  //
  // Handle incremental update of the histograms selected.
  // Only those columns marked as STATE == PENDING will be processed.
  // Updated histograms are written to the histograms/intervals table inside 
  // indirect calling function UpdateStats().
  //
  retcode = incrementHistograms();
  HSHandleError(retcode);
  checkTime("after incrementing histograms for IUS");

  if (LM->LogNeeded()) 
    LM->StopTimer();

  // Write CBFs for groups that are PROCESSED and delayedRead back to disk.
  writeCBFstoDiskForIUS(*hssample_table, singleGroup);

  return retcode;
}

// Returns the table component of the fully qualified name passed in, using the
// catalog and schema name from tblDef to determine where it starts (the table
// is in the same schema as the one referenced by tblDef). This avoids problems
// in parsing the fully qualified name posed by the possibility of periods within
// delimited identifiers.
static void extractTblName(const NAString& fullyQualifiedName,
		           HSTableDef* tblDef, NAString & out)
{
  Lng32 tblNameOffset = tblDef->getCatName().length() +
                        tblDef->getSchemaName().length() +
                        2;  // 2 dot separators
  out = fullyQualifiedName.data() + tblNameOffset;
}

// Update the persistent sample table and determine its new cardinality.
//   1) Delete rows in the persistent sample satisfying the IUS predicate.
//   2) Insert the rows from <sampleTblName>_I into the persistent sample
//      if the _I table was created. If building histograms from scratch
//      using the persistent sample rather than incrementally changing them
//      (USTAT_INCREMENTAL_UPDATE_STATISTICS is DF_SAMPLE), insert sampled
//      rows satisfying the IUS where clause directly from the source table
//      In either case, these rows constitute a random sample of rows from
//      the source table that satisfy the IUS predicate.
//   3) From the prior cardinality of the sample table (oldSampleSize), subtract
//      the number of rows deleted, add the number of rows inserted, and return
//      the result in the newSampleSize parameter.
//
// This can't be done as part of end_IUS_work(), because that is called even
// when the IUS fails; its purpose is just to modify the SB_PERSISTENT_SAMPLES
// table to indicate that IUS is no longer in progress on the source table.
// The persistent sample table itself is only modified if IUS is successful.
Lng32 HSGlobalsClass::UpdateIUSPersistentSampleTable(Int64 oldSampleSize,
                                                     Int64 requestedSampleSize,
                                                     Int64& newSampleSize)
{
  Lng32 retcode = 0;
  Int64 rowsAffected;
  HSLogMan *LM = HSLogMan::Instance();
  newSampleSize = oldSampleSize;  // before deleting/adding rows

  HSFuncExecQuery("CONTROL QUERY DEFAULT ALLOW_DML_ON_NONAUDITED_TABLE 'ON'");

  HSHandleError(retcode);

  // step 1  - delete the affected rows from PS
  NABoolean transactional = (CmpCommon::getDefault(USTAT_DELETE_NO_ROLLBACK) == DF_OFF);
  NAString deleteQuery;
  generateIUSDeleteQuery(*hssample_table, deleteQuery, transactional);

  if (LM->LogNeeded()) {
    LM->Log("query to delete from PS:");
    LM->Log(deleteQuery.data());
    LM->StartTimer("IUS: execute query to delete from PS");
  }

  rowsAffected = 0;

  // The most likely error on the DELETE would be due to a bad WHERE clause.
  // (When CQD USTAT_INCREMENTAL_UPDATE_STATISTICS is set to 'SAMPLE', this is
  // the first place that we attempt to use the user's WHERE clause.)
  if (transactional)
    {
      retcode = HSFuncExecTransactionalQueryWithRetry(deleteQuery, -UERR_IUS_BAD_WHERE_CLAUSE,
                            &rowsAffected,
                            "IUS delete from PS where",
                            NULL, NULL);
    }
  else
    {
      retcode = HSFuncExecQuery(deleteQuery, -UERR_IUS_BAD_WHERE_CLAUSE,
                            &rowsAffected,
                            "IUS delete from PS where",
                            NULL, NULL);
    }

  if (LM->LogNeeded()) {
    LM->StopTimer();
    sprintf(LM->msg, PF64 " rows deleted from persistent sample table.", rowsAffected);
    LM->Log(LM->msg);
  }
  HSHandleError(retcode);
  newSampleSize -= rowsAffected;

  // step 2  - add all rows from _I to PS, or sampled from source table,
  // depending on USTAT_INCREMENTAL_UPDATE_STATISTICS value.
  NAString selectInsertQuery;
  generateIUSSelectInsertQuery(*hssample_table, *user_table, selectInsertQuery);

  if (LM->LogNeeded()) {
    LM->Log("query to insert into PS:");
    LM->Log(selectInsertQuery.data());
    LM->StartTimer("IUS: execute query to insert into PS");
  }

  rowsAffected = 0;
  NAString insSourceTblName;
  extractTblName(*hssample_table + "_I", objDef, insSourceTblName /* out */);
  NABoolean needEspParReset = setEspParallelism(objDef, insSourceTblName.data());
 
  // can't retry this one, as it uses non-transactional upsert using load + random
  // select; a retry might add *another* random sample to a partial sample from
  // the previous attempt
  retcode = HSFuncExecQuery(selectInsertQuery, -UERR_INTERNAL_ERROR,
                            &rowsAffected,
                            "IUS insert into PS (select from _I)",
                            NULL, NULL, 0,
                            // check mdam usage if reading incremental sample directly from source table
                            CmpCommon::getDefault(USTAT_INCREMENTAL_UPDATE_STATISTICS) == DF_SAMPLE);  //checkMdam
  if (LM->LogNeeded()) {
    LM->StopTimer();
    sprintf(LM->msg, PF64 " rows inserted into persistent sample table.", rowsAffected);
    LM->Log(LM->msg);
  }
  if (needEspParReset)
    resetEspParallelism();
  HSHandleError(retcode);
  newSampleSize += rowsAffected;

  // Save sample count values to update row in SB_PERSISTENT_SAMPLES table.
  PST_IUSrequestedSampleRows_ = new(STMTHEAP) Int64;
  *PST_IUSrequestedSampleRows_ = requestedSampleSize;
  PST_IUSactualSampleRows_ = new(STMTHEAP) Int64;
  *PST_IUSactualSampleRows_ = newSampleSize;

  HSFuncExecQuery("CONTROL QUERY DEFAULT ALLOW_DML_ON_NONAUDITED_TABLE reset");

  checkTime("after updating persistent sample table for IUS");
  return retcode;
}

// Read in CBFs for groups that are PENDING and delayedRead flag is TRUE 
Int32 HSGlobalsClass::readCBFsIntoMemForIUS(NAString& sampleTableName, 
                                            HSColGroupStruct* group
     )
{
   NAString cbfFilePrefix;
   getCBFFilePrefix(sampleTableName, cbfFilePrefix);

   Lng32 sz;
   Lng32 bufSz = 0;
   char* bufptr = NULL;
   struct stat sts;

   while (group) {

      if ( group->delayedRead && group->state == PENDING ) {

        // Reset to FALSE here to assume some problem reading in CBF.
        // Once the CBF does read in successfully, we set the flag to TRUE below.
        group->delayedRead = FALSE;

        NAString cbfFile(cbfFilePrefix);
        cbfFile.append(group->cbfFileNameSuffix());

        if (stat(cbfFile, &sts) == 0) {
           if ( bufSz < sts.st_size ) {
              NADELETEBASIC(bufptr, STMTHEAP);
              bufSz = sts.st_size;
              bufptr = new (STMTHEAP) char[bufSz];
           }

           Lng32 fd = open(cbfFile.data(), O_RDONLY);
           if ( fd != -1 ) {
              // Use a different buffer point because unpackBuffer() will
              // advance the buffer argument! 
              char* buffer = bufptr;
              sz = read(fd, buffer, bufSz);
              if ( sz == sts.st_size ) {
                 group->cbf = new (STMTHEAP) 
                        CountingBloomFilterWithKnownSkews(STMTHEAP);
                 group->cbf->unpackBuffer(buffer);
                 group->delayedRead = TRUE;
              }
              close(fd);
           }
        }
      }

      group = group->next;
   }

   return 0;
}



int file_select(const struct direct *entry)
{
   if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
     return 0;
   else
     return 1;
}


// Compute total disk space in 512kb blocks used by persistent CBFs contained in 
// dir USTAT_IUS_PERSISTENT_CBF_PATH
UInt32 computeTotalCBFDiskSpaceInBlocks(NAString& cbf_path)
{
   struct direct **files = NULL;


   Int32 count = scandir(cbf_path.data(), &files, file_select, 
                         (int(*)(const dirent**, const dirent**))alphasort);

   UInt32 sum = 0;
   struct stat buf;

   // Sum up the amount of disk spaces used by each cbf file
   for (Int32 i=0; i<count; i++) {

      NAString fullpath(cbf_path);
      fullpath += "/";
      fullpath += files[i]->d_name;

      if ( !stat(fullpath, &buf) ) {
        sum += buf.st_blocks;
      }
   }

   // Variable 'sum' has # of units in 512 KB. 
   //
   // Quote from Linux man page on stats():
   //    The st_blocks  field  indicates  the  number  of blocks allocated to 
   //    the file, 512-byte units.  

   return sum;
}

// Compute total disk space in 512k blocks and pass it in totalSpace.
// Return TRUE when there is no issue found.
NABoolean getTotalDiskSizeInBlocks(NAString& cbf_path, UInt64& totalSpace)
{
   struct statvfs fsstats;

   if ( !statvfs(cbf_path.data(), &fsstats) ) {
     totalSpace = (UInt64)(fsstats.f_bsize) * fsstats.f_blocks  / 512;
     return TRUE;
   } else 
     return FALSE;
}

// Check if one more cbf 'cbf' can be added to the cbf_path dir
NABoolean hasSpaceTostoreCBF(NAString& cbf_path, 
                             CountingBloomFilter* cbf,
                             UInt64 totalAllowedInBlocks
                            )
{
   UInt32 totalOnDisk = computeTotalCBFDiskSpaceInBlocks(cbf_path);

   if ( totalOnDisk + cbf->getTotalMemSize() / 512 <= totalAllowedInBlocks )
     return TRUE;
   else
     return FALSE;
}



// Write to disk for CBFs for groups that are PROCESSED and cbf ptr is not NULL
Int32 HSGlobalsClass::writeCBFstoDiskForIUS(NAString& sampleTableName, 
                                            HSColGroupStruct* group
     )
{
   HSLogMan *LM = HSLogMan::Instance();
   if (LM->LogNeeded())
     LM->StartTimer("IUS: write CBF files to disk");

   NAString path =
     ActiveSchemaDB()->getDefaults().getValue(USTAT_IUS_PERSISTENT_CBF_PATH);

   ULng32 totalCBFsizeInMB =
     ActiveSchemaDB()->getDefaults().getAsULong(USTAT_IUS_MAX_PERSISTENT_DATA_IN_MB);

   float percentage = (float)
     ActiveSchemaDB()->getDefaults().getAsDouble(USTAT_IUS_MAX_PERSISTENT_DATA_IN_PERCENTAGE);

   UInt64 totalSpaceInBlocks = 0;

   if ( !getTotalDiskSizeInBlocks(path, totalSpaceInBlocks) ) {
     if (LM->LogNeeded())
       LM->StopTimer();
     return 0;
   }
   
   UInt64 totalAllowedInBlocks = MINOF(totalCBFsizeInMB * 1024 / 2, 
                                       totalSpaceInBlocks * percentage);

   NAString cbfFilePrefix;
   getCBFFilePrefix(sampleTableName, cbfFilePrefix);

   Lng32 sz;
   Lng32 bufSz = 0;
   char* bufptr = NULL;

   Int32 count = 0;

   while (group) {

      if ( group->cbf && group->state == PROCESSED ) {

        NAString cbfFile(cbfFilePrefix);
        cbfFile.append(group->cbfFileNameSuffix());

        Lng32 cbfSz = group->cbf->getTotalMemSize();

        if ( !hasSpaceTostoreCBF(path, group->cbf, totalAllowedInBlocks) ) {
           group=group->next;
           continue;
        }

        if ( bufSz < cbfSz ) {

           if ( bufptr )
              NADELETEBASIC(bufptr, STMTHEAP);

           bufSz = cbfSz + 100;
           bufptr = new (STMTHEAP) char[bufSz];
        }

        Lng32 fd = open(cbfFile.data(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
        if ( fd != -1 ) {

           char* buffer = bufptr;
           sz = group->cbf->packIntoBuffer(buffer, FALSE /* no bytes swapping */ );
           HS_ASSERT( sz <= bufSz);
           HS_ASSERT( sz <= buffer - bufptr);

           ssize_t wsz = write(fd, bufptr, sz);

           if ( wsz != sz ) {
              // TBD. Need to remove the file written (if exist) 
           } else
              count++;

           close(fd);

        }
       
        // Make sure we don't write it again on next batch.
        delete group->cbf;
        group->cbf = NULL;
      }

      group = group->next;
   }
   NADELETEBASIC(bufptr, STMTHEAP);

   if (LM->LogNeeded())
     LM->StopTimer();
   return count;
}

Int32 HSGlobalsClass::deletePersistentCBFsForIUS(NAString& sampleTableName, 
                                                 HSColGroupStruct* group,
                                                 SortState stateToDelete)
{
   NAString cbfFilePrefix;
   getCBFFilePrefix(sampleTableName, cbfFilePrefix);

   while (group) {

      if ( group->cbf && group->state == stateToDelete ) {

        NAString cbfFile(cbfFilePrefix);
        cbfFile.append(group->cbfFileNameSuffix());

        remove(cbfFile.data());

        // Make sure this unused CBF does not get persisted.
        delete group->cbf;
        group->cbf = NULL;
      }

      group = group->next;
   }

   return 0;
}

Lng32 HSGlobalsClass::selectIUSBatch(Int64 currentRows, Int64 futureRows, NABoolean& ranOut, Int32& colsSelected)
{
  HSLogMan *LM = HSLogMan::Instance();
  if (LM->LogNeeded())
    LM->StartTimer("IUS: selectIUSBatch()");

  colsSelected = 0;
  Int32 colsSuggested = 0;  // number of cols we try to allocate
  iusSampleDeletedInMem->depopulate();
  iusSampleInsertedInMem->depopulate();
  Int64 memAllowed = getMaxMemory();
  Int64 memLeft = memAllowed;

  ranOut = FALSE;  // set to true if not enough memory for all cols

  Lng32 retcode = 0;
  Int64 tableUID = objDef->getObjectUID();
  char UIDStr[30];
  convertInt64ToAscii(tableUID,UIDStr);
  Lng32 colnum = 0;

  UInt32 histID = 0;
  Int16 intvlCount = 0;
  Int64 totalRowCount = 0;
  Int64 totalUEC = 0;
  Int64 v2;

  // SELECT HISTOGRAM_ID, INTERVAL_COUNT, ROWCOUNT, TOTAL_UEC, V2
  // FROM SB_HISTOGRAMS
  // WHERE TABLE_UID = tableUID AND COLCOUNT = 1 AND COLUMN_NUMBER = CAST(? AS INTEGER)

  HSErrorCatcher errorCatcher(retcode, - UERR_INTERNAL_ERROR, "HSGlobalsClass::selectIUSBatch", TRUE);

  NAString query = "SELECT HISTOGRAM_ID, INTERVAL_COUNT, ROWCOUNT, TOTAL_UEC, V2 FROM ";
  query += *hstogram_table;
  query += " WHERE TABLE_UID = ";
  query += UIDStr;
  query += " AND COLCOUNT = 1 AND COLUMN_NUMBER = CAST(? AS INTEGER)"; // single column histograms only

  HSCursor histCursor;
  histCursor.prepareQuery(query.data(), 1, 5); // 1 input parameter, 5 output

  // Memory required by RUS has been estimated in prepareForIUS().
  // Here we need to add the extra amount needed by IUS. Do it for each group below.

  // Visit all unprocessed items looking for ones that fit. No early loop exit if memLeft
  // is 0, because it is very unlikely to hit zero exactly. If trySampleInMemory
  // is set, we ignore whether columns have uec too low to perform with internal
  // sort. If all column samples won't fit in memory, we do it over and take that
  // into account the second time.

  float false_prob = (float)CmpCommon::getDefaultNumeric(USTAT_INCREMENTAL_FALSE_PROBABILITY);


  if (LM->LogNeeded()) {
    LM->Log("In ::selectIUSBatch()");
  }

  HSColGroupStruct *group = singleGroup;
  HSColGroupStruct* delGroup = iusSampleDeletedInMem->getColumns();
  HSColGroupStruct* insGroup = iusSampleInsertedInMem->getColumns();

  while (group != NULL)
    {
      if ( group->state == NO_STATS || group->state != UNPROCESSED ||
           group->memNeeded == 0  // was set to 0 if exceeds address space
         )
      {
         if (group->memNeeded == 0)
           ranOut = TRUE;
         group = group->next;
         delGroup = delGroup->next;
         insGroup = insGroup->next;
         continue;
      }
              
      colnum = group->colSet[0].colnum;
      retcode = histCursor.open(1, (void*)&colnum);
      HSHandleError(retcode);
      retcode = histCursor.fetch(5, (void*)&histID, (void*)&intvlCount,
                                    (void*)&totalRowCount, (void*)&totalUEC,
                                    (void*)&v2);

       if ( retcode == 0 ) {

         retcode = histCursor.close();

         group->avgVarCharSize = (double)v2;
         Lng32 maxHashsToUse =
          (ActiveSchemaDB()->getDefaults()).
               getAsLong(USTAT_IUS_MAX_NUM_HASH_FUNCS);

         group->groupHist = new(STMTHEAP) HSHistogram(intvlCount,
                                                        totalRowCount, // Not needed here
                                                        0, 0,
                                                        TRUE, FALSE);
         // Interval count passed to ctor is max # intervals; need to call the
         // following to set actual number of intervals used.
         group->groupHist->setCurrentInt(intvlCount);
         retcode = initIUSIntervals(group, delGroup, insGroup, histID, intvlCount);
         HSHandleError(retcode);

         // group->groupHist->maxStddev_ now contains the max of the stddev of freq
         // at each interval. 

         double maxStddev = group->groupHist->getMaxStddev();

         //group->groupHist->logAll(title);


         UInt64 memForCBF = CountingBloomFilterWithKnownSkews::estimateMemoryInBytes(

                        maxHashsToUse,

                        // The expected number of distinct keys
                        // @WARN: Have to do a narrowing cast here, ctor only takes a UInt32.

                        // Worse case assumption is that all rows in 
                        // the old and new sample are distinct.  We take the sum of that 
                        // minus the size of Di. Subject the result to the MAX of total UEC. 
                        MAXOF((UInt32)(currentRows -
                                       iusSampleDeletedInMem->getNumRows()+ 
                                       iusSampleInsertedInMem->getNumRows()),
                                      (UInt32)totalUEC),

                        // probability of false positives, from CQD
                        false_prob,

                        // take the 3x max stddev among intervals
                        UInt32(totalRowCount / totalUEC) + 3*UInt32(ceil(maxStddev)),

                        // expected # of keys with high frequency. +1 to include the
                        // dummy interval.
                        (UInt32)((intvlCount+1)  * 2),

                        intvlCount+1 
                      );


         Int64 totMemNeeded = Int64(group->memNeeded
                               + delGroup->memNeeded 
                               + insGroup->memNeeded 
                               + memForCBF); 

         if ( totMemNeeded < memLeft )
         {
           group->state = PENDING;
           delGroup->state = PENDING;
           insGroup->state = PENDING;


           colsSuggested++;
           memLeft -= totMemNeeded;

         } else {
           // Not enough memory for the group. Leave the group in UNPROCESSED state,
           // and get rid of the HSHistogram object we created for it.
           ranOut = TRUE;
           delete group->groupHist;
           group->groupHist = NULL;
           if (LM->LogNeeded()) {
              sprintf(LM->msg, "Not enough memory for %s: memLeft=" PF64 " totMemNeeded=", group->colNames->data(), memLeft);
              formatFixedNumeric((Int64)totMemNeeded, 0, LM->msg+strlen(LM->msg));
              LM->Log(LM->msg);
              sprintf(LM->msg, "group->memNeeded=" PF64"", group->memNeeded); 
              LM->Log(LM->msg);
              sprintf(LM->msg, "delGroup->memNeeded=" PF64"", delGroup->memNeeded); 
              LM->Log(LM->msg);
              sprintf(LM->msg, "insGroup->memNeeded=" PF64"", insGroup->memNeeded); 
              LM->Log(LM->msg);
              sprintf(LM->msg, "memForCBF=");
              formatFixedNumeric((Int64)memForCBF, 0, LM->msg+strlen(LM->msg));
              LM->Log(LM->msg);
           }
         }

      } else if ( retcode == 100 ) {
          // Ignore the group if there is no stats for it!

          if (LM->LogNeeded()) {
              sprintf(LM->msg, "No stats: histTableName=%s, tableUid=" PF64", colnum=%d", 
                               (char*)hstogram_table->data(),
                               tableUID,
                               group->colSet[0].colnum);
              LM->Log(LM->msg);
          }

          retcode = histCursor.close();
          HSHandleError(retcode);
        
          diagsArea << DgSqlCode(UERR_IUS_NO_EXISTING_STATS)
                    << DgString0(group->colSet[0].colname->data());

          // No stats exist for the group. Change the process state 
          // to NO_STATS so that we can skip it again in next call
          // to this routine and accurately report missing stats for each
          // column once.
          group->state = NO_STATS;

       } else {
          histCursor.close();
          HSHandleError(retcode);
       }

       group = group->next;
       delGroup = delGroup->next;
       insGroup = insGroup->next;
    }

   // Now allocate memory for singleGroup, inMemDelete and inMemInsert table,
   // for each column in PENDING state.
   colsSelected = allocateMemoryForIUSColumns(singleGroup, futureRows,
                               iusSampleDeletedInMem->getColumns(),
                               iusSampleDeletedInMem->getNumRows(),
                               iusSampleInsertedInMem->getColumns(),
                               iusSampleInsertedInMem->getNumRows());

   if (LM->LogNeeded())
    {
      LM->Log("Columns selected by selectIUSBatch():");
      group = singleGroup;
      while (group != NULL)
        {
          if (group->state == PENDING)
            {
              sprintf(LM->msg, "    %s (" PF64" bytes)",
                               group->colSet[0].colname->data(),
                               group->memNeeded);
              LM->Log(LM->msg);
            }
          group = group->next;
        }
     sprintf(LM->msg, "return from selectIUSBatch(): columns originally selected = %d, "
                      "columns able to allocate = %d", colsSuggested, colsSelected);
     LM->Log(LM->msg);
     LM->StopTimer();
    }

  return retcode;
}

// Use In-memory tables to update histograms incrementally.
Lng32 HSGlobalsClass::incrementHistograms()
{
  HSLogMan *LM = HSLogMan::Instance();

  Lng32 retcode = 0;

  HSColGroupStruct* group = singleGroup;
  HSColGroupStruct* delGroup = iusSampleDeletedInMem->getColumns();
  HSColGroupStruct* insGroup = iusSampleInsertedInMem->getColumns();

  while (group)
    {
       if ( group->state == PENDING ) {

         retcode = processIUSColumn(group, delGroup, insGroup);

         if ( retcode > 0 ) {
           // IUS is not successful. Keep the group 
           // in PENDING state so that it can be dealt with in the internal sort code path.

           retcode = 0;
         } else {
           if ( retcode == 0 ) {
              group->state = PROCESSED; // IUS successful. 
              group->freeISMemory();
              delGroup->state = PROCESSED; // IUS successful.
              delGroup->freeISMemory();
              insGroup->state = PROCESSED; // IUS successful.
              insGroup->freeISMemory();
           } else
              HSHandleError(retcode);
         }
      }

      group = group->next;
      delGroup = delGroup->next;
      insGroup = insGroup->next;
    }

  return retcode;
}

Int32 HSGlobalsClass::processIUSColumn(HSColGroupStruct* smplGroup,
                                       HSColGroupStruct* delGroup,
                                       HSColGroupStruct* insGroup)
{
  Int32 retcode = 0;
  Lng32 datatype = smplGroup->ISdatatype;

  // Only need to handle types used for IS/IUS. Datetime/interval types and
  // non-integral fixed numerics are all converted to one of these types.
  switch (datatype)
    {
      case REC_BIN8_SIGNED:
        return processIUSColumn((Int8*)smplGroup->data, L"%hd", smplGroup, delGroup, insGroup);
        break;
      case REC_BOOLEAN:
      case REC_BIN8_UNSIGNED:
        return processIUSColumn((UInt8*)smplGroup->data, L"%hu", smplGroup, delGroup, insGroup);
        break;
      case REC_BIN16_SIGNED:
        return processIUSColumn((Int16*)smplGroup->data, L"%hd", smplGroup, delGroup, insGroup);
        break;
      case REC_BPINT_UNSIGNED:
      case REC_BIN16_UNSIGNED:
        return processIUSColumn((UInt16*)smplGroup->data, L"%hu", smplGroup, delGroup, insGroup);
        break;
      case REC_BIN32_SIGNED:
        return processIUSColumn((Int32*)smplGroup->data, L"%d", smplGroup, delGroup, insGroup);
        break;
      case REC_BIN32_UNSIGNED:
        return processIUSColumn((UInt32*)smplGroup->data, L"%u", smplGroup, delGroup, insGroup);
        break;
      case REC_BIN64_SIGNED:
        return processIUSColumn((Int64*)smplGroup->data, L"%lld", smplGroup, delGroup, insGroup);
        break;
      case REC_BIN64_UNSIGNED:
        return processIUSColumn((UInt64*)smplGroup->data, L"%llu", smplGroup, delGroup, insGroup);
        break;
      case REC_FLOAT32:
        return processIUSColumn((Float32*)smplGroup->data, L"%f", smplGroup, delGroup, insGroup);
        break;
      case REC_FLOAT64:
        return processIUSColumn((Float64*)smplGroup->data, L"%lf", smplGroup, delGroup, insGroup);
        break;
      case REC_BYTE_F_ASCII:
      case REC_BYTE_F_DOUBLE:
      case REC_BINARY_STRING:
        {
          // Create an object to be used with the value iterator; does not own its content.
          // In setting length, take into account that length in IUSFixedChar is in
          // characters instead of bytes.
          IUSFixedChar fixedChar(FALSE);
          if (DFS2REC::isDoubleCharacter(smplGroup->ISdatatype))
            IUSFixedChar::setLength(smplGroup->ISlength / 2);
          else
            IUSFixedChar::setLength(smplGroup->ISlength);
          IUSFixedChar::setCaseInsensitive(smplGroup->colSet[0].caseInsensitive == 1);
          IUSFixedChar::setColCollation(smplGroup->colSet[0].colCollation);
          IUSFixedChar::setCharSet(smplGroup->colSet[0].charset);
          return processIUSColumn(&fixedChar, L"", smplGroup, delGroup, insGroup);
        }
        break;
      case REC_BYTE_V_ASCII:
      case REC_BYTE_V_DOUBLE:
      case REC_VARBINARY_STRING:
        {
          // Create an object to be used with the value iterator; does not own its content.
          // In setting length, take into account that length in IUSFixedChar is in
          // characters instead of bytes.
          IUSVarChar varChar(FALSE);
          if (DFS2REC::isDoubleCharacter(smplGroup->ISdatatype))
            IUSVarChar::setDeclaredLength(smplGroup->ISlength / 2);
          else
            IUSVarChar::setDeclaredLength(smplGroup->ISlength);
          IUSVarChar::setCaseInsensitive(smplGroup->colSet[0].caseInsensitive == 1);
          IUSVarChar::setColCollation(smplGroup->colSet[0].colCollation);
          IUSVarChar::setCharSet(smplGroup->colSet[0].charset);
          return processIUSColumn(&varChar, L"", smplGroup, delGroup, insGroup);
        }
        break;

      default:
        retcode = -1;
        HSHandleError(retcode);
    } // switch

  return retcode;
}

Lng32 HSGlobalsClass::initIUSIntervals(HSColGroupStruct* smplGroup,
                                       HSColGroupStruct* delGroup,
                                       HSColGroupStruct* insGroup,
                                       UInt32 histID,
                                       Int16 numIntervals)
{
  HSLogMan *LM = HSLogMan::Instance();
  if (LM->LogNeeded())
    LM->StartTimer("IUS: initIUSIntervals()");
  typedef Int16 LenType;
  Lng32 retcode = 0;
  Int64 tableUID = objDef->getObjectUID();
  char UIDStr[30];
  convertInt64ToAscii(tableUID,UIDStr);

  Int64 rowCount;
  Int16 intvlNum;
  Int64 uec;
  double max_stddev = 0.0; // max stddev per histogram 
  double stddev; // stddev per interval
  Int64 v1,v2;   // read but not used
  NAWchar boundarySpec[HS_MAX_UCS_BOUNDARY_CHAR + 1];  // +1 for 2-byte count
  NAWchar MFV[HS_MAX_UCS_BOUNDARY_CHAR + 1];

  char histIDStr[20];
  sprintf(histIDStr,"%u",histID);

  // SELECT several columns
  // FROM SB_HISTOGRAM_INTERVALS
  // WHERE TABLE_UID = tableUID AND HISTOGRAM_ID = histID
  // ORDER BY INTERVAL_NUMBER

  NAString query = "SELECT INTERVAL_NUMBER, INTERVAL_ROWCOUNT, INTERVAL_UEC,"
                   " INTERVAL_BOUNDARY, STD_DEV_OF_FREQ, V1, V2, V5 FROM ";
  query += *hsintval_table;
  query += " WHERE TABLE_UID = ";
  query += UIDStr;
  query += " AND HISTOGRAM_ID = ";
  query += histIDStr;
  query += " ORDER BY INTERVAL_NUMBER";

  HSCursor intvlCursor(STMTHEAP,HS_INTERVAL_STMT_ID);
  intvlCursor.prepareQuery(query.data(), 0, 8); // no input parameters, 8 output

  retcode = intvlCursor.open();
  HSHandleError(retcode);

  HSHistogram* hist = smplGroup->groupHist;
  Int16 fetchCount = 0;

  while (TRUE)
    {
      retcode = intvlCursor.fetch(8, (void*)&intvlNum,
                                     (void*)&rowCount,                                   
                                     (void*)&uec,
                                     (void*)boundarySpec,
                                     (void*)&stddev, (void*)&v1, (void*)&v2,
                                     (void*)MFV);
      if (retcode == HS_EOF)
        {
          retcode = 0;
          break;
        }
      else 
        if ( retcode != 0 ) {
        }
      HSHandleError(retcode);

      if ( stddev > max_stddev )
        max_stddev = stddev;

      fetchCount++;

      hist->setIntRowCount(intvlNum, rowCount);
      hist->setIntUec(intvlNum, uec);

      // The boundary and MFV values stored in the histogram_intervals table
      // include closing parens that we need to remove in the in-memory version.
      hist->setIntBoundary(intvlNum,
                           ((char*)boundarySpec) + sizeof(LenType) + sizeof(NAWchar),
                           *(LenType*)boundarySpec - (2 * sizeof(NAWchar)));
      hist->setIntMFVValue(intvlNum,
                           ((char*)MFV) + sizeof(LenType) + sizeof(NAWchar),
                           *(LenType*)MFV - (2 * sizeof(NAWchar)));

      hist->setIntMFVRowCount(intvlNum, v1);
      hist->setIntMFV2RowCount(intvlNum, v2);
    }

  retcode = intvlCursor.close();
  HSHandleError(retcode);

  hist->setMaxStddev(max_stddev);

  // 0th interval used only to hold lowest value and is not included in count of
  // intervals, so we should have fetched 1 more than numIntervals.
  HS_ASSERT(fetchCount == numIntervals+1);

  // Interval to count nulls, if present, is last one. The last interval is at
  // index numIntervals even though the array is 0-based, because the 0th interval
  // is not included in the count stored in the Histograms table.
  if (!na_wcscmp((const NAWchar*)hist->getIntBoundary(numIntervals).data(), L"NULL"))
    hist->setHasNull(TRUE);
  else
    {
      // Histogram for this sample table column has no null interval; should not
      // be any nulls for the column in rows deleted from sample. If there are
      // nulls for the column in rows to be added to the sample, we must create
      // a null interval in the histogram.

      HS_ASSERT(delGroup->nullCount == 0);
      if (insGroup->nullCount > 0)
        // Nulls will be included in this histogram where none were before.
        hist->addNullInterval(insGroup->nullCount, insGroup->colCount);  //@ZX -- may do more than we want

    }

  if (LM->LogNeeded())
    LM->StopTimer();
  return retcode;
}

/*************************************************/
/* METHOD:  FlushStatistics()                    */
/* PURPOSE: Final histogram processing stage.    */
/*          Based on what user requested, it     */
/*          will finalize the request. An        */
/*          HSTranController is declared at the  */
/*          start of this function, which causes */
/*          the body of the fn to be executed in */
/*          a transaction.                       */
/* PARAMS:  statsWritten(out) - tells if stats   */
/*                              were actually    */
/*                              written to db.   */
/* RETCODE:  0 - successful                      */
/*          -1 - failure                         */
/*************************************************/
Lng32 HSGlobalsClass::FlushStatistics(NABoolean &statsWritten)
  {
    HSLogMan *LM = HSLogMan::Instance();
    Lng32 retcode = 0;
    statsWritten = FALSE;

                                          /*=================================*/
                                          /*        CLEAR OPTION USED        */
                                          /*=================================*/
    if (optFlags & CLEAR_OPT)
      {
        // Execute in a transaction.  Transaction will rollback or commit based
        // on retcode, once this block is left.
        HSTranController TC("FLUSH STATISTICS", &retcode);
        HSHandleError(retcode);
        if (groupCount == 0)                 /*== No groups specified      ==*/
          {                                       /* delete all histograms   */
            retcode = ClearAllHistograms();       /* generated for table     */
            HSHandleError(retcode);
          }
        else
          {                                  /*== Group list specified     ==*/
        	LM->StartTimer("MakeAllHistid (for CLEAR)");
            retcode = MakeAllHistid();         /* determine histogram ids    */
                                               /* Performs SERIALIZABLE read */
            LM->StopTimer();
            HSHandleError(retcode);   
            retcode = ClearSelectHistograms();   /*delete selected histograms*/
            HSHandleError(retcode);               /* generated for table     */
          }

        //When a SQL/MP table is dropped, the histograms are not automatically
        //deleted. Whenever the CLEAR option is used, we always delete
        //orphan histograms - whether they exist or not. Having orphan
        //histograms does not hurt anything.
        if (tableFormat == SQLMP)
          DeleteOrphanHistograms();
      }
                                          /*=================================*/
                                          /*  COLLECT FILE-LEVEL STATISTICS  */
                                          /*=================================*/
    /* no histograms generated only collect file level statistics */
    else if (groupCount == 0) 
      {
         if (CmpCommon::getDefault(USTAT_COLLECT_FILE_STATS) == DF_ON)
         {
           // Execute in a transaction.  Transaction will rollback or commit based
           // on retcode, once this block is left.
           HSTranController TC("FLUSH STATISTICS", &retcode);
           HSHandleError(retcode);
           LM->StartTimer("collectFileStatistics()");
           retcode = objDef->collectFileStatistics();
           LM->StopTimer();
           HSHandleError(retcode);
         }

         // Absence of column groups to update could be due to either
         // not specifying ON clause, or using EXISTING when there are
         // no existing histograms or NECESSARY when there are no obsolete
         // or ungenerated histograms.
         if (optFlags & EXISTING_OPT)
           diagsArea << DgSqlCode(UERR_WARNING_NO_EXISTING_HISTOGRAMS);
         else if (optFlags & NECESSARY_OPT)
           diagsArea << DgSqlCode(UERR_WARNING_NO_OBSOLETE_HISTOGRAMS);
         else 
           diagsArea << DgSqlCode(UERR_WARNING_FILE_STATISTICS);
      }
                                          /*=================================*/
                                          /*     UPDATE HISTOGRAM TABLES     */
                                          /*=================================*/
    else
      {
        // Execute in a transaction.
        { // New block to ensure transaction termination via HSTranController dtor.
          HSTranController TC("FLUSH STATISTICS", &retcode);
          HSHandleError(retcode);

          // IUS work: Keep warnings unless there are errors.
          if ( CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) > 0 )
              CmpCommon::diags()->clear();

          LM->StartTimer("MakeAllHistid()");
          retcode = MakeAllHistid();      /* Determine histogram ids       */
          LM->StopTimer();
          HSHandleError(retcode);         /* Performs SERIALIZABLE read    */

          LM->StartTimer("WriteStatistics()");
          retcode = WriteStatistics();    /* Write new histograms using    */
          LM->StopTimer();
          HSHandleError(retcode);         /* precomp queries, then dels    */
                                          /* old hists using dyn query.    */
          statsWritten = TRUE;            /* no error writing hist stats   */
        } // block for flush stats transaction

        // Separate transaction for collection of file statistics.
        if (CmpCommon::getDefault(USTAT_COLLECT_FILE_STATS) == DF_ON)
        { // New block to ensure transaction termination via HSTranController dtor.
          HSTranController TC("GET FILE STATISTICS", &retcode);
          HSHandleError(retcode);
          retcode = objDef->collectFileStatistics(); /* collect file level statistics */
          if (retcode < 0)
            {
              // Update of histograms completed ok, but the collection of file
              // statistics failed, possibly due to contention for the Partitions
              // metadata table. Convert error diagnostics to warnings, and add
              // an explanatory one.
              NegateAllErrors(&diagsArea);
              HSFuncMergeDiags(UERR_WARNING_FILESTATS_FAILED);
            }
        }

      }

    if (retcode == 0)
      {
        // invalidate any cached histograms in the cluster
        CmpSeabaseDDL::invalidateStats(objDef->getObjectUID());
      }

    return retcode;
  }


/*****************************************************************************/
/* METHOD:  WriteStatistics()                                                */
/* PURPOSE: Inserts into the HISTOGRAM table using INSERT101_... precompiled */
/*          query with unique histogram ids obtained from MakeAllHistIds.    */
/*          Then inserts into the HISTOGRAM_INTERVAL table using INSERT201.. */
/*          precompiled query.  Finally, deletes old histograms using        */
/*          dynamic queries.                                                 */
/* RETCODE:  0 - successful                                                  */
/*          -1 - failure                                                     */
/*****************************************************************************/
Lng32 HSGlobalsClass::WriteStatistics()
  {
    Lng32     retcode = 0;
    char     tempStr[2000];
    char     uidStr[30];
    char     compileStatsReason=0;
    NAString oldHistList, dupHistList;
    NAString stmt;
    NABoolean processMultiGroups;
    HSColGroupStruct *group;
    HSColumnStruct   *col;
    HSLogMan *LM = HSLogMan::Instance();
    HSErrorCatcher errorCatcher(retcode, - UERR_INTERNAL_ERROR, "FLUSH_STATISTICS", TRUE);
    Lng32 i;

    // Use auto_ptr to ensure that histRS and histintRS are deleted when we exit
    // this function. Clearing of STMTHEAP when the statement ends wouldn't be
    // good enough; the dtors must be executed so the CLI statements will be
    // deallocated. Otherwise, a "statement already in use" error occurs for the
    // next instance of HSinsertHist or HSinsertHistint.
    std::auto_ptr<HSinsertHist> histRS(NULL);
    std::auto_ptr<HSinsertHistint> histintRS(NULL);

    //DEFAULT VALUES FOR HISTOGRAM TABLE.
    //These values should only be used when base table is empty. Otherwise, we
    //should get values from the generated histogram.
    Lng32 numInts    = 1;
    Int64 totalUec  = 0;
    Int64 avgVarCharSize  = 0;
    HSDataBuffer lval(WIDE_("()"));
    HSDataBuffer hval(WIDE_("()"));
    short readCount     = 0;
    //Int64 hv3           = 0;
    //Int64 hv4           = 0;
    //HSDataBuffer hv5(L"");
    //HSDataBuffer hv6(L"");

    //DEFAULT VALUES FOR HISTOGRAM_INTERVALS TABLE.
    //These values should only be used when base table is empty. Otherwise, we
    //should get values from the generated histogram.
    Int64 intRowCount = 0;
    Int64 intUEC      = 0;
    HSDataBuffer bound(WIDE_("()"));
    double stdDevOfFreq = 0;
    Int64 intMFVRowCount   = 0;
    Int64 intMFV2RowCount  = 0;
    HSDataBuffer mostFreqVal(L"()");
    //Int64 v3 = 0;
    //Int64 v4 = 0;
    //HSDataBuffer v6(L"");

    convertInt64ToAscii(objDef->getObjectUID(), uidStr);

    // If these are compile time stats and the desired sample is less than the
    // default would have been, set reason to SMALL_SAMPLE.
    if (requestedByCompiler)
    {
      // These are compile time stats.
      Int64 defaultSampRows;
      if (actualRowCount >= getMinRowCountForSample())
           defaultSampRows = getDefaultSampleSize(actualRowCount); 
      else defaultSampRows = actualRowCount;

      // Set reason to small sample if the actual sample table had less then 90% of the
      // rows that a default sample would have had.  That is, record these as full blown
      // stats if the sample rowcount is pretty close to what the default would have had.
      if (sampleRowCount < defaultSampRows * 0.9)
           compileStatsReason = HS_REASON_SMALL_SAMPLE; // sample size is small.
      else compileStatsReason = HS_REASON_AUTO_INIT;    // sample size similar to default.
    }

    if (singleGroup)
      {
        group = singleGroup;                      /* process single-columns  */
        processMultiGroups = TRUE;                /* then multi-columns      */
      }
    else
      {
        group = multiGroup;                       /* process multi-columns   */
        processMultiGroups = FALSE;               /* only once               */
      }

    // histRS and histintRS are instantiations of the std::auto_ptr template;
    // do NOT try to allocate them from STMTHEAP, or a core will occur when
    // they go out of scope and their underlying ptrs are deleted.
    if (tableFormat == SQLMX)
      {
        // histogram versioning
        if (HSGlobalsClass::schemaVersion >= COM_VERS_2300) 
        {
#ifdef NA_USTAT_USE_STATIC  // use static query defined in module file
          histRS.reset(new HSinsertHist("INSERT101_MX_2300",
                                        hstogram_table->data()));
          histintRS.reset(new HSinsertHistint("INSERT201_MX_2300",
                                              hsintval_table->data()));
#else // NA_USTAT_USE_STATIC not defined, use dynamic query
          histRS.reset(new HSinsertHist(hstogram_table->data()));
          histintRS.reset(new HSinsertHistint(hsintval_table->data()));
#endif // NA_USTAT_USE_STATIC not defined
        }
        else
        {
          histRS.reset(new HSinsertHist("INSERT101_MX", hstogram_table->data()));
          histintRS.reset(new HSinsertHistint("INSERT201_MX", hsintval_table->data()));
        }

        if (LM->LogNeeded())
           {
             sprintf(LM->msg, "writeStatistics() via INSERT201_MX_2300");
             LM->Log(LM->msg);
           }
      }
    else
      {
        histRS.reset(new HSinsertHist("INSERT101_MP", hstogram_table->data()));
        histintRS.reset(new HSinsertHistint("INSERT201_MP", hsintval_table->data()));
      }
    LM->StartTimer("initialize rowset for Histograms");
    retcode = histRS->initialize();       //initialize ROWSET for HISTOGRAMS
    LM->StopTimer();
    HSHandleError(retcode);
    LM->StartTimer("initialize ROWSET for HISTOGRAM_INTERVALS");
    retcode = histintRS->initialize();    //initialize ROWSET for HISTOGRAM_INTERVALS
    LM->StopTimer();
    HSHandleError(retcode);

    LM->StartTimer("Create new histograms and histogram intervals");
    while (group != NULL)
      {
        if (group->oldHistid != 0)
          {
                                          /*=================================*/
                                          /*   GATHER OLD HISTOGRAM VALUES   */
                                          /*=================================*/
            sprintf(tempStr, " %u,", group->oldHistid);
            oldHistList += tempStr;
            if (group->oldHistidList != "")
              oldHistList += group->oldHistidList;
          }

        //Histograms have been generated. We need to overwrite the default
        //values with actual values from the generated histogram.
        if (actualRowCount != 0 && group->groupHist != NULL)
          {
            numInts     = group->groupHist->getNumIntervals();
            totalUec    = group->groupHist->getTotalUec();

            if (group->colCount == 1) // processing single group.
            {
              if (DFS2REC::isAnyVarChar(group->colSet[0].datatype))
              {
                // scale up by 100 to get fraction.
                avgVarCharSize = (Int64) (group->avgVarCharSize * 100.0);
                // If all values are empty strings, just add 1 byte to 
                // distinguish  from 0 (zero). A zero varchar size means 
                // stats are from R2.4 and early releases.
                avgVarCharSize = MAXOF(avgVarCharSize, 1);
              }
              else
                avgVarCharSize = -1; // non-varchar data types.
            }
            retcode = group->groupHist->getLowValue(lval);
            HSHandleError(retcode);
            retcode = group->groupHist->getHighValue(hval);
            HSHandleError(retcode);
          }

        // Set the reason for this histogram to small sample.
        if (compileStatsReason) group->newReason = compileStatsReason;

        for (i = 0; i < group->colCount; i++)
          {
            col = &group->colSet[i];
                                          /*=================================*/
                                          /*     CREATE NEW HISTOGRAMS       */
                                          /*=================================*/

            // Don't add the row if its new histid is 0 - this would only happen
            // if there was a duplicate histogram for a given column. Instead,
            // log it and proceed. The old histid for the duplicate will still be
            // part of the list of histograms to delete, so the duplicate will
            // be eliminated.
            if (group->newHistid)
              retcode = histRS->addRow(objDef->getObjectUID(),
                                      group->newHistid,
                                      col->position,
                                      col->colnum,
                                      group->colCount,
                                      (short)numInts,
                                      actualRowCount,
                                      totalUec,
                                      (char*)statstime->data(),
                                      lval,
                                      hval,
                                      // the following are columns for automation
                                      group->readTime,
                                      readCount,
                                      sampleSeconds,
                                      group->colSecs + columnSeconds,
                                      samplePercentX100,
                                      group->coeffOfVar,
                                      group->newReason,
                                      // the following is used by compile time stats
                                      compileStatsReason ? sampleRowCount : 0,
                                      avgVarCharSize
                                      // v3-v6 currently unused.
                                      );
            else if (LM->LogNeeded())
              {
                sprintf(LM->msg, 
                        "WARNING: Duplicate histogram %d found (will be removed).",
                        group->oldHistid);
                LM->Log(LM->msg);
              }

            HSHandleError(retcode);
          }

        for (i = 0; i <= numInts; i++)
          {
                                          /*=================================*/
                                          /* CREATE NEW HISTOGRAM_INTERVALS  */
                                          /*=================================*/
            //Histograms have been generated. We need to overwrite the default
            //values with actual values from the generated histogram.
            if (actualRowCount != 0 && group->groupHist != NULL)
              {
                intRowCount = group->groupHist->getIntRowCount(i);
                intUEC      = group->groupHist->getIntUec(i);
                retcode = group->groupHist->getParenthesizedIntBoundary(i, bound);
                HSHandleError(retcode);
                intMFVRowCount  = group->groupHist->getIntMFVRowCount(i);
                intMFV2RowCount = group->groupHist->getIntMFV2RowCount(i);
                retcode = group->groupHist->getParenthesizedIntMFV(i, mostFreqVal);
                HSHandleError(retcode);

               // Calculate standard deviation of frequency for this interval.
               if (intUEC > 1) {
                 double intSumSquare = group->groupHist->getIntSquareSum(i);
                 Int64 intOrigUec    = 0;
                 double intAvg    = 0;
                 intOrigUec = group->groupHist->getIntOrigUec(i);
                 if (intOrigUec != 0  AND sampleRowCount != 0 )
                 {
                   Int64 intOrigRc = 0;
                   Int64 upscale_for_rc = actualRowCount / sampleRowCount;
                   intOrigRc = intRowCount / upscale_for_rc;
                   intAvg = (double)intOrigRc / (double)intOrigUec;
                 }
                 else
                 {
                   intAvg      = (double)intRowCount/(double)intUEC;
                   intOrigUec  = intUEC;
                 }
                 double result = (intSumSquare/double(intOrigUec)) -
                                 ((double)intAvg*intAvg);
                 // make sure result is a positive number
                 result = MAXOF(result, 0.0);
                 stdDevOfFreq = sqrt(result);
               }
               else
                 stdDevOfFreq = 0;
            }

            // Don't add intvl with id 0 -- it is from a duplicate histogram
            // that has snuck in somehow (see comment above for histogram call
            // to addRow()).
            if (group->newHistid)
              retcode = histintRS->addRow(objDef->getObjectUID(),
                                          group->newHistid,
                                          (short)i,
                                          intRowCount,
                                          intUEC,
                                          bound, 
                                          stdDevOfFreq,
                                          intMFVRowCount,  // v1
                                          intMFV2RowCount, // v2
                                          0, 0,            // v3, v4 unused
                                          mostFreqVal      // v5
                                          // v6 unused
                                          );
            HSHandleError(retcode);
          }

        checkTime("after creating new histograms");
        group = group->next;
        if (group == NULL && processMultiGroups)
          {
            processMultiGroups = FALSE;
            group = multiGroup;
          }
      } // while
    LM->StopTimer();  // Create new histograms and histogram intervals

    LM->StartTimer("Write out new histograms");
    retcode = histRS->flush();
    LM->StopTimer();
    HSHandleError(retcode);

    LM->StartTimer("Write out new histogram intervals");
    retcode = histintRS->flush();
    LM->StopTimer();
    HSHandleError(retcode); 
                                          /*=================================*/
                                          /*   REMOVE DUPLICATE HISTOGRAMS   */
                                          /*=================================*/
    LM->StartTimer("Remove duplicate histograms");
    HSColGroupStruct *dup = dupGroup;
    while (dup != NULL)
      {
        sprintf(tempStr, " %u,", dup->oldHistid);
        dupHistList += tempStr;
        dup = dup->next;
      }
    retcode = HSGlobalsClass::removeHists(dupHistList, uidStr, "DELETE DUPLICATE HISTOGRAMS");
    LM->StopTimer();
    LM->StopTimer();  // Remove duplicate histograms
    HSHandleError(retcode);
                                          /*=================================*/
                                          /*      REMOVE OLD HISTOGRAMS      */
                                          /*=================================*/
    LM->StartTimer("Remove old histograms");
    retcode = HSGlobalsClass::removeHists(oldHistList, uidStr, "DELETE OLD HISTOGRAMS"); 
    LM->StopTimer();
    return retcode;
  }


/*=================================*/
/* Remove histograms.              */
/*=================================*/
Lng32 HSGlobalsClass::removeHists(NAString &hists, char *uid, const char *operation)
{
  Lng32 retcode = 0;
  if (hists.length() > 0)
    {

      HSLogMan *LM = HSLogMan::Instance();
      NAString stmt;
      Int64    xRows = 0;
      char     rowCountStr[30];

      LM->StartTimer("Delete old rows from Histograms table");
      hists.remove(hists.length() - 1);      // remove last comma
      stmt = "DELETE FROM ";
      stmt += hstogram_table->data();
      stmt += " WHERE TABLE_UID = ";
      stmt += uid;
      stmt += " AND HISTOGRAM_ID IN (";
      stmt += hists;
      stmt += ")";
      // Note that this can't be done with retry since we are
      // part of a larger transaction started by FlushStatistics
      retcode = HSFuncExecQuery(stmt, -UERR_INTERNAL_ERROR,
                                 &xRows, operation, NULL, NULL);
      LM->StopTimer();
      HSHandleError(retcode);
      if (LM->LogNeeded())
        {
          convertInt64ToAscii(xRows, rowCountStr);
          sprintf(LM->msg, "\t\t\t%s ROWS DELETED", rowCountStr);
          LM->Log(LM->msg);
        }

      LM->StartTimer("Delete old rows from Histogram_Intervals table");
      stmt = "DELETE FROM ";
      stmt += hsintval_table->data();
      stmt += " WHERE TABLE_UID = ";
      stmt += uid;
      stmt += " AND HISTOGRAM_ID IN (";
      stmt += hists;
      stmt += ")";
      // Note that this can't be done with retry since we are
      // part of a larger transaction started by FlushStatistics
      retcode = HSFuncExecQuery(stmt, -UERR_INTERNAL_ERROR, 
                                &xRows, operation, NULL, NULL);
      LM->StopTimer();
      HSHandleError(retcode);
      if (LM->LogNeeded())
        {
          convertInt64ToAscii(xRows, rowCountStr);
          sprintf(LM->msg, "\t\t\t%s ROWS DELETED", rowCountStr);
          LM->Log(LM->msg);
        }
    }
  checkTime("during removal of histograms");
  return retcode;
}


// Perform CURSOR103_... to obtain histograms.  Make list of
// histograms and return as groupList.  For each histogram
// read, check to see if it is a duplicate.  If so, add to list
// HSGlobalsClass::dupGroup and do not put on groupList.   The
// dupGroup will be removed in WriteStatistics (during regular
// update statistics).
// arguments
//   - skipEmpty: is EXISTING keyword specified
//   - exclusive: do we need exclusive locks on the accessed rows
Lng32 HSGlobalsClass::groupListFromTable(HSColGroupStruct*& groupList,
                                         NABoolean skipEmpty,
                                         NABoolean exclusive)
  {
    HSLogMan *LM = HSLogMan::Instance();
    LM->StartTimer("Read histograms, return as HSColGroupStruct list (groupListFromTable())");
    HSColGroupStruct *group = NULL, **lastDupGroup = &dupGroup;
    HSColumnStruct   col;
    NAString columnName;
    ULng32 histid;
    Lng32 colPos, colNum, colCount;
    Int64 objID = objDef->getObjectUID();
    Lng32 retcode = 0;
    char readTime[TIMESTAMP_CHAR_LEN+1];
    char reason = HS_REASON_UNKNOWN;

    // Initialize the pointer to the group list we will build.
    groupList = NULL;
    
    // if showstats for a native hbase table,hive table or table under seabase schema, 
    // need to check if the table SB_HISTOGRAMS exist
    NAString schemaName;
    if (strcmp(hstogram_table->data(), "TRAFODION.\"_HBASESTATS_\".SB_HISTOGRAMS") == 0)
      schemaName = "_HBASESTATS_";
    else if (strcmp(hstogram_table->data(), "TRAFODION.\"_HIVESTATS_\".SB_HISTOGRAMS") == 0)
      schemaName = "_HIVESTATS_";
    else if (strcmp(hstogram_table->data(), "TRAFODION.SEABASE.SB_HISTOGRAMS") == 0)
      schemaName = "SEABASE";
    if (!schemaName.isNull())
      {
        NAString queryStr = "SELECT count(*) FROM TRAFODION.\"_MD_\".OBJECTS WHERE SCHEMA_NAME='" +
                            schemaName + 
                            "' AND OBJECT_NAME='SB_HISTOGRAMS' AND OBJECT_TYPE='BT';";
        HSCursor cursor;
        retcode = cursor.prepareQuery(queryStr.data(), 0, 1);
        HSHandleError(retcode);
        retcode = cursor.open();
        HSHandleError(retcode);
        ULng32 cnt;
        retcode = cursor.fetch (1, (void *)&cnt);
        HSHandleError(retcode);
        if (cnt == 0)
          {
            LM->StopTimer();
            return 0;
          }
      }

#ifdef NA_USTAT_USE_STATIC  // use static query defined in module file
    HSCliStatement::statementIndex stmt;
    if (tableFormat == SQLMX)
      if (HSGlobalsClass::schemaVersion >= COM_VERS_2300)
      {
      
        if (exclusive)
          stmt = HSCliStatement::CURSOR103_MX_2300_X;
        else
          stmt = HSCliStatement::CURSOR103_MX_2300;
      }
      else
        stmt = HSCliStatement::CURSOR103_MX;
    else
      stmt = HSCliStatement::CURSOR103_MP;

    HSCliStatement cursor103( stmt,
                           (char *)hstogram_table->data(),
                           (char *)&objID
                         );
#else // NA_USTAT_USE_STATIC not defined, use dynamic query
    char sbuf[25];
    NAString qry = "SELECT HISTOGRAM_ID, COL_POSITION, COLUMN_NUMBER, COLCOUNT, "
                          "cast(READ_TIME as char(19) character set iso88591), REASON "
                   "FROM ";
    qry.append(hstogram_table->data());
    qry.append(    " WHERE TABLE_UID = ");
    sprintf(sbuf, PF64, objID);
    qry.append(sbuf);
    qry.append(    " ORDER BY TABLE_UID, HISTOGRAM_ID, COL_POSITION ");
    qry.append(    " FOR READ COMMITTED ACCESS");

    HSCursor cursor103;
    retcode = cursor103.prepareQuery(qry.data(), 0, 6);
    HSHandleError(retcode);
#endif // NA_USTAT_USE_STATIC not defined

    retcode = cursor103.open();
    HSHandleError(retcode);

    while (retcode == 0)
        {
          if (tableFormat == SQLMX && HSGlobalsClass::schemaVersion >= COM_VERS_2300)
            retcode = cursor103.fetch (6,
                                      (void *)&histid, (void *)&colPos,
                                      (void *)&colNum, (void *)&colCount,
                                      (void *)&readTime, (void *)&reason
                                      );
          else
            retcode = cursor103.fetch (4,
                                      (void *)&histid, (void *)&colPos,
                                      (void *)&colNum, (void *)&colCount
                                      );
          // Don't read any more (break out of loop) if fetch did not succeed.
          HSFilterWarning(retcode);
          if (retcode)
            break;
          // If EXISTING keyword specified and REASON field is EMPTY, skip.
          if (skipEmpty && reason == HS_REASON_EMPTY)
            continue;

          col = objDef->getColInfo(colNum); // colNum is position in table
          col.colnum = colNum;
          col.position = colPos;

          columnName = objDef->getColName(colNum);

          if (groupList == NULL)  // FIRST GROUP ENTRY
            {
              groupList = new(STMTHEAP) HSColGroupStruct;
              group = groupList;
              group->prev = NULL;
            }
          else                      // APPEND GROUP ENTRY
            {
              group->next = new(STMTHEAP) HSColGroupStruct;
              group->next->prev = group;
              group = group->next;
            }
          group->colSet.insert(col);
          group->oldHistid = histid;
          strncpy(group->readTime, readTime, TIMESTAMP_CHAR_LEN);
          group->readTime[TIMESTAMP_CHAR_LEN] = '\0';
          group->reason = reason;
          group->colCount = colCount;
          *group->colNames += ToAnsiIdentifier(columnName);
          *group->colNames += ",";

          for (Int32 i=1; i < colCount; i++)  // GET ALL RELATED ROWS
            {
              if (tableFormat == SQLMX && HSGlobalsClass::schemaVersion >= COM_VERS_2300)
                retcode = cursor103.fetch (6,
                                          (void *)&histid, (void *)&colPos,
                                          (void *)&colNum, (void *)&colCount,
                                          (void *)&readTime,(void *)&reason
                                          );
              else
                retcode = cursor103.fetch (4,
                                          (void *)&histid, (void *)&colPos,
                                          (void *)&colNum, (void *)&colCount
                                          );
              HS_ASSERT(retcode == 0);
              HS_ASSERT(group->oldHistid == histid);

              col = objDef->getColInfo(colNum); // colNum is position in table
              col.colnum = colNum;
              col.position = colPos;
              group->colSet.insert(col);
              columnName = objDef->getColName(colNum);

              *group->colNames += " ";
              *group->colNames += ToAnsiIdentifier(columnName);
              *group->colNames += ",";
            }
          group->colNames->remove(group->colNames->length() - 1); // remove last comma

          if (findDuplicate(group, groupList)) 
          {
             // Add to HSGlobalsClass removal list.
             *lastDupGroup = group;
             lastDupGroup = &(group->next);
             *lastDupGroup = NULL; // NULL end of dup list.

             group=group->prev; // remove this group.
             group->next = NULL;
          }
        }

// For the dynamic case, the HSCursor dtor closes the cursor when cursor103
// goes out of scope.
#ifdef NA_USTAT_USE_STATIC
    // Don't overwrite the return code if an error has occurred, but attempt to
    // close the cursor anyway (in case it was successfully opened).
    if (retcode < 0)
      cursor103.close();
    else
      retcode = cursor103.close();
#else
    // Caller may use HSHandleError, which bails out for nonzero retcode.
    if (retcode == 100)
      retcode = 0;
#endif

    LM->StopTimer();
    return retcode;
  }

// Determine whether entry is a duplicate within list.  The first occurrence
// of any duplicated entry will always return FALSE.  All others will return
// TRUE.
NABoolean HSGlobalsClass::findDuplicate(const HSColGroupStruct *entry,
                                              HSColGroupStruct *list)
  {
    NABoolean retval = FALSE;
    if (!entry || !list) return retval;

    // Loop through all items that are before entry on the list.  This means
    // that the first of any duplicate entries will always return FALSE, 
    // leaving it to be used normally.  This also reduces the amount of 
    // comparisons required.
    HSColGroupStruct *listItem = list;
    while (listItem != NULL && listItem != entry)
      {
        if (entry->colSet == listItem->colSet)
          {
            retval = TRUE;
            break;
          }
        listItem = listItem->next;
      }
    return retval;
  }

HSColGroupStruct* HSGlobalsClass::findGroup(const HSColGroupStruct *tableGroup)
  {
    HS_ASSERT(tableGroup != NULL);
    HSColGroupStruct *group = NULL;
    NABoolean processMultiGroups;

    if (singleGroup)
      {
        group = singleGroup;                      /* process single-columns  */
        processMultiGroups = TRUE;                /* then multi-columns      */
      }
    else
      {
        group = multiGroup;                       /* process multi-columns   */
        processMultiGroups = FALSE;               /* only once               */
      }

    // Find histogram from HISTOGRAMS table in list of histograms specified
    // at command line.  Return NULL if a histogram in HISTOGRAMS table is
    // not specified for regeneration.
    while (group != NULL)
      {
        if (group->colSet == tableGroup->colSet)
          break;

        group = group->next;
        if (group == NULL && processMultiGroups)
          {
            processMultiGroups = FALSE;
            group = multiGroup;
          }
      }

    return group;
  }


HSColGroupStruct* HSGlobalsClass::findGroup(const Lng32 colnum)
  {
    HSColGroupStruct *group = singleGroup;

    while (group != NULL)
      {
        if (colnum == group->colSet[0].colnum)
          break;

        group = group->next;
      }

    return group;
  }

//  return the group with the given column number also return the position of
//  the group in the group list
HSColGroupStruct* HSGlobalsClass::findGroupAndPos(const Lng32 colnum, Int32 &pos)
  {
    HSColGroupStruct *group = singleGroup;

    pos = 0;
    while (group != NULL)
      {
        if (colnum == group->colSet[0].colnum)
          break;

        pos++;
        group = group->next;
      }

    return group;
  }

// has all MC groups be processed and computed
// forIS is used to indicated that the method was called from
// the MC IS processing code
NABoolean HSGlobalsClass::allMCGroupsProcessed(NABoolean forIS)
{
    HSColGroupStruct *mgroup = multiGroup;

    while (mgroup != NULL)
    {
       if ((forIS && (mgroup->state == UNPROCESSED)) ||
          (!forIS && (mgroup->state != PROCESSED)))
       {
          return FALSE;
       }

       mgroup = mgroup->next;

    }

    return TRUE;
}


// This function is called by the HS_ASSERT macro to take care of some things
// before triggering an assertion failure:
//   - Log the assertion failure if logging is enabled.
//   - Roll back transaction if one is in progress.
//   - Put an assertion error in the diagnostics area. This is supposed to be
//     done by code executed due to the macro HS_ASSERT invokes for the assertion
//     failure, but it does not always work properly. Doing it here prevents it
//     from being attempted downstream.
// The parameters are the text of the assertion, and the file and line at which
// it occurred.
void HSGlobalsClass::preAssertionFailure(const char* condition,
                                         const char* fileName,
                                         Lng32 lineNum)
{
  HSTranMan *TM = HSTranMan::Instance();
  HSLogMan *LM = HSLogMan::Instance();
  if (LM->LogNeeded())
    {
      sprintf(LM->msg, "***[ERROR] INTERNAL ASSERTION (%s) AT %s:%i", condition, fileName, lineNum);
      LM->Log(LM->msg);
    }
  if (TM->StartedTransaction())
    TM->Rollback();
  diagsArea << DgSqlCode(arkcmpErrorAssert)
            << DgString0(condition)
            << DgString1(fileName)
            << DgInt0(lineNum);
}


/****************************************************************/
/* METHOD:  getRetcodeFromDiags()                               */
/* PURPOSE: Derive the appropriate return code from the contents*/
/*          of the diagnostics area.                            */
/* PARAMS:  none                                                */
/* RETURN:  The return code implied by the errors/warnings in   */
/*          the diagnostics area.                               */
/****************************************************************/
Lng32 HSGlobalsClass::getRetcodeFromDiags()
{
  // Quick check for empty diagnostics area.
  if (diagsArea.getNumber() == 0)
    return 0;
    
  Lng32 i;
  Lng32 retcode = 0;
  for (i=1; i<=diagsArea.getNumber(DgSqlCode::ERROR_); i++)
    {
      retcode = diagsArea.getErrorEntry(i)->getSQLCODE();
      HSFilterError(retcode);
      // If we find an error that the filter did not change to -1, return it
      // now (prefer it to one mapped to the generic -1 value).
      if (retcode < -1)
        return retcode;
    }
    
  if (retcode < 0)
    return retcode;
    
  // No errors if we get this far. Look at the warnings, and return the first
  // one that HSFilterWarning() does not tell us to ignore.
  for (i=1; i<=diagsArea.getNumber(DgSqlCode::WARNING_); i++)
    {
      retcode = diagsArea.getWarningEntry(i)->getSQLCODE();
      HSFilterWarning(retcode);
      if (retcode)  // if filter didn't replace it with 0
        return retcode;
    }

   HS_ASSERT(!retcode);
   return retcode;
}


/****************************************************************/
/* METHOD:  addGroup()                                          */
/* PURPOSE: Add the passed group to either the single-column    */
/*          group or the multicolumn group, depending on its    */
/*          column count.                                       */
/* PARAMS:  group(in) -- Pointer to the group to add to one of  */
/*                       the lists.                             */
/* ASSUMPTIONS: The group is a detached node and not part of a  */
/*              list.                                           */
/****************************************************************/
void HSGlobalsClass::addGroup(HSColGroupStruct *group)
  {
    HS_ASSERT(group->next == NULL && group->prev == NULL);

    // Link the group in at the front of the appropriate list.
    if (group->colCount == 1)
      {
        if (singleGroup == NULL)   // first group entry
            singleGroup = group;
        else                       // append to front of list
          {
            group->next = singleGroup;
            singleGroup->prev = group;
            singleGroup = group;
          }
        singleGroupCount++;    // count of single-col groups
      }
    else
      {
        if (multiGroup == NULL)    // first group entry
          multiGroup = group;      // no separate count for MC groups
        else                       // append to front of list
          {
            group->next = multiGroup;
            multiGroup->prev = group;
            multiGroup = group;
          }
      }

    groupCount++;                  // overall group count
  }

// Remove the passed group from the appropriate group list, and deallocate it.
void HSGlobalsClass::removeGroup(HSColGroupStruct* groupToRemove)
{
  if (!groupToRemove)
    return;

  HSColGroupStruct* group = (groupToRemove->colCount == 1 ? singleGroup : multiGroup);
  while (group && group != groupToRemove)
    group = group->next;

  // If the group was found, unlink it from list and deallocate.
  if (group)
    {
      if (group->next)
        group->next->prev = group->prev;

      if (group->prev)
        group->prev->next = group->next;
      else if (group->colCount == 1)
        {
          HS_ASSERT(singleGroup == group);
          singleGroup = group->next;
        }
      else
        {
          HS_ASSERT(multiGroup == group);
          multiGroup = group->next;
        }

      // Make group isolated from list, or deleting it will cause further
      // deletions.
      group->next = group->prev = NULL;

      // Group has been detached from list, now delete it.
      delete group;
    }
}

/****************************************************************************/
/* METHOD:  removeGroups()                                                  */
/* PURPOSE: Remove groups from the front (most recently added) of both the  */
/*          single- and multi-group lists until arriving at the passed stop */
/*          nodes. This rolls back the state of the lists to a point before */
/*          a set of new nodes were added.                                  */
/* PARAMS:  numGroupsToRemove -- Number of groups to remove. This is not    */
/*                  strictly necessary, but provides a correctness check.   */
/*          oldSingle -- Node of singleGroup at which to stop. This will be */
/*                       the new head of the list of single-column groups.  */
/*          oldMulti -- Node of multiGroup at which to stop. This will be   */
/*                      the new head of the list of multiple-column groups. */
/*                      the lists.                                          */
/* RETURN:  TRUE if successful, FALSE otherwise                             */
/****************************************************************************/
NABoolean HSGlobalsClass::removeGroups(Lng32 numGroupsToRemove,
                                       HSColGroupStruct* oldSingle,
                                       HSColGroupStruct* oldMulti)
  {
    HSLogMan *LM = HSLogMan::Instance();
    HSColGroupStruct* groupToRemove;

    // Remove up to numGroupsToRemove nodes from front of single-group list
    // until stop node is encountered.
    while (singleGroup && singleGroup != oldSingle && numGroupsToRemove > 0)
      {
        groupToRemove = singleGroup;
        singleGroup = singleGroup->next;
        if (singleGroup)
          singleGroup->prev = NULL;
        groupToRemove->next = NULL;  // dtor will try to delete next
        delete groupToRemove;
        numGroupsToRemove--;
        groupCount--;
        singleGroupCount--;
      }

    // Remove nodes from front of multi-group list.
    while (multiGroup && multiGroup != oldMulti && numGroupsToRemove > 0)
      {
        groupToRemove = multiGroup;
        multiGroup = multiGroup->next;
        if (multiGroup)
          multiGroup->prev = NULL;
        groupToRemove->next = NULL;  // dtor will try to delete next
        delete groupToRemove;
        numGroupsToRemove--;
        groupCount--;
      }

    // We should have counted down to 0 groups to remove, and the new list heads
    // should match the targets that were passed in. If either of these conditions
    // is not met, return FALSE.
    if (numGroupsToRemove > 0)
      {
        if (LM->LogNeeded())
          {
            sprintf(LM->msg, "removeGroups() failed: %d groups not removed",
                             numGroupsToRemove);
            LM->Log(LM->msg);
          }
        return FALSE;
      }
    else if (oldSingle != singleGroup || oldMulti != multiGroup)
      {
        if (LM->LogNeeded())
          LM->Log("removeGroups() failed: not enough groups deleted");
        return FALSE;
      }

    return TRUE;
  }

static Int32 recDepth = 0;
static Int32 maxRecDepth = 0;


/************************************************/
/* METHOD:   updateMCMFV                        */
/* PURPOSE:  we have a new distinct value       */
/*           update the uec and MFV values      */
/* PARAMS:   mfvc1 - number of rows for mfv1    */
/*           mfvc2 - number of rows for mfv2    */
/*           nRowsSqr - row count square for    */
/*                      all distinct values     */
/*           uec - uec value                    */
/*           mfvi - index of mfv value          */
/*           newCount - row count of ditinct    */
/*                      value                   */
/*           newIndex - index of distinct value */
/* RETCODE:  none                               */
/************************************************/

void updateMCMFV (Int32& mfvc1, Int32& mfvc2, double& nRowsSqr, 
                  Int64& uec, Int32& mfvi, Int32 newCount, Int32 newIndex)
{
    nRowsSqr += newCount*newCount;
    uec++;

    if (newCount > mfvc1)
    {
       mfvc2 = mfvc1;
       mfvc1 = newCount;
       mfvi = newIndex;
    }
    else if (newCount > mfvc2)
       mfvc2 = newCount;
}

/*************************************************/
/* METHOD:   computeMCISuec                      */
/* PURPOSE:  a fast path way of computing stats  */
/*           for an MC                           */
/* PARAMS:   mgroup  - MC to compute stats for   */
/*           MCrows  - the data of the MC        */
/*                     encapsulated by MCWrapper */
/*                     objects                   */
/*           allRows -   number of rows to       */
/*                       process                 */
/*           numIntervals - number if intervals  */
/*           samplingUsed - is sampling used     */
/*                          to create            */
/* RETCODE:  none                                */
/*************************************************/

void computeMCISuec(HSColGroupStruct *mgroup, MCWrapper* MCrows, NABoolean samplingUsed, 
                    Int32 allRows, Int32 numIntervals)
{
   Int64 uec = 0;
   Int32 cnt = 1;
   double nRowsSqr = 0;

   HSLogMan *LM = HSLogMan::Instance();

   // most frequest values
   Int32 mfvc1 = 0;
   Int32 mfvc2 = 0;
   Int32 mfvi = 0;
   
   Int32 nulls = mgroup->nullCount;
   Int32 nRows = allRows - nulls;

   // with data already sorted, get the distinct values and compute
   // uec and MFV values
   if (nRows >  0)
   {
      Int32 i = 0;
      for (; i < nRows - 1; i++)
      {
         if (MCrows[i].index_ != MCrows[i+1].index_)
         {
           updateMCMFV (mfvc1, mfvc2, nRowsSqr, uec, mfvi, cnt,i);
           cnt = 0;
         }
         cnt++;
      }

      updateMCMFV (mfvc1, mfvc2, nRowsSqr, uec, mfvi, cnt,i);
   }

   // include nulls in MFV computation, if all nulls used the last row in the sorted list 
   // as the MFV since nulls are sorted higher
   if (nulls)
   {
      updateMCMFV (mfvc1, mfvc2, nRowsSqr, uec, mfvi, nulls,allRows - 1);
   }

   if (LM->LogNeeded())
   {
      sprintf(LM->msg, "\tMC: using IS: allrowcount is (%d) null row count is (%d)"
                       " mfvc1 is (%d) mvfc2 is (%d) hv index is (%d) uec is (%ld)", 
                       allRows, nulls, mfvc1, mfvc2, mfvi, uec);
      LM->Log(LM->msg);
   }

   // for MC the number of intervals is "1"
   Int32 intCount = 1;
   mgroup->groupHist = new(STMTHEAP) HSHistogram(intCount, nRows+MCrows->nullCount_, 0, 0, samplingUsed, FALSE);

   NAWString wStr = WIDE_("");
   myVarChar vc = myVarChar();
   vc.len = wStr.length()*2;
   memmove(vc.val, wStr.data(), vc.len);

   // create  one interval with dummy values
   mgroup->groupHist->addIntervalData(vc, mgroup, nulls, 0, 0, FALSE);

   mgroup->groupHist->setIntSquareSum(1, nRowsSqr);
   mgroup->groupHist->setIntRowCount(1, allRows);
   mgroup->groupHist->setIntMFVRowCount(1, mfvc1);
   mgroup->groupHist->setIntMFV2RowCount(1, mfvc2);
   mgroup->groupHist->setIntUec(1, uec);

   HSDataBuffer mfvBoundary;
   setBufferValue(MCrows[mfvi], mgroup, mfvBoundary);
   mgroup->groupHist->setIntMFVValue(1, mfvBoundary.data(), mfvBoundary.length());
}

/***********************************************/
/* METHOD:  ComputeMCStatistics()              */
/* PURPOSE: Estimates the UEC for multi-columns*/
/* PARAMS:  usingIS - was this method called   */
/*                    from the IS logic so IS  */ 
/*                    can be attempted to      */
/*                    compute MC stats         */
/* RETCODE: 0 - successful                     */
/* ASSUMPTIONS: For every multi-column group   */
/*        there must be a corresponding SC     */
/*        group. For example, For MC(A,B) the  */
/*        corresponding SC(A) and SC(B) must   */
/*        exist.                               */
/*        If sampling was used, the SC uec and */
/*        rowcount must have been extrapolated */
/*        prior to calling this method.        */
/***********************************************/
Lng32 HSGlobalsClass::ComputeMCStatistics(NABoolean usingIS)
{
    NABoolean collectMCSkewedValues = FALSE;
    Lng32 retcode = 0;
    HSColGroupStruct *mgroup = multiGroup;
    HSColumnStruct   *col;
    HSColGroupStruct *sgroup;
    NAWString low, hi;
    HSDataBuffer boundLo, boundHi;
    HSCursor *cursor;
    HSLogMan *LM = HSLogMan::Instance();
   
    while (mgroup != NULL)
    {
        // is this MC covered by columns in memory so IS can be used
        NABoolean coveredByIS = TRUE;

        // skip the group if we already processed it
        // or Internal Sort (IS) is used and this group cannot be used with IS
        if ((mgroup->state == PROCESSED) ||
            (usingIS && (mgroup->state == DONT_TRY)))
        {
           mgroup = mgroup->next;
           continue;
        }

        Lng32 colCount = mgroup->colCount;

        // Check if skewValuesCollected flag is set via syntax.
        collectMCSkewedValues = (mgroup->skewedValuesCollected ||
                         (CmpCommon::getDefault(USTAT_COLLECT_MC_SKEW_VALUES) == DF_ON));

        if(collectMCSkewedValues)
        {        
          const Lng32 maxCharBoundaryLen = (Lng32) CmpCommon::getDefaultNumeric(USTAT_MAX_CHAR_BOUNDARY_LEN);
          Lng32 totalBoundaryLen = (HS_MAX_UCS_BOUNDARY_CHAR - (colCount -1) * 3);
          if(colCount * maxCharBoundaryLen >= totalBoundaryLen)
            collectMCSkewedValues = FALSE;

          // Dont collect skewed values if the column
          // group consists of a member of the decimal class, the interval class
          // or the float class, since these classes are not supported by SB.
          for (Int32 i=0; i<colCount; i++)
          {
            HSColumnStruct &col = mgroup->colSet[i];
            if (DFS2REC::isInterval(col.datatype) ||
                DFS2REC::isFloat(col.datatype))
            {
              collectMCSkewedValues = FALSE;
              break;
            }
          }
          mgroup->skewedValuesCollected = collectMCSkewedValues; 
        }

        if ( performISForMC() && usingIS)
        {
          // check if the data needed by all columns in the MC is already in
          // memory so IS can be used
          HSColGroupStruct *sgroup2;
          for (Int32 i=0; i<colCount; i++)
          {
             col = &mgroup->colSet[i];
             sgroup2 = findGroup(col->colnum);
             if ((sgroup2->state != PROCESSED) || (sgroup2->mcis_memFreed == TRUE))
             {
               coveredByIS = FALSE;
               break;
             }
          }
        }
        else
          coveredByIS = FALSE;
  
        // compute the MC stats using Internal Sort
        if (coveredByIS)
        {
            if (LM->LogNeeded())
            {
               char title[1000];
               sprintf(title, "MC: Compute using Internal Sort (%.940s) with%sskew",
                               mgroup->colNames->data(), collectMCSkewedValues? " " : " NO ");
               LM->StartTimer(title);
               (void)getTimeDiff(TRUE);
            }

            mgroup->state = PENDING;
  
            Int32 numRows = 0;
            //make sure that all columns have the same number of rows and get back this number
            NABoolean rowCountMatch = checkAllColsHaveSameNumOfRows(mgroup, numRows);
  
            // number of rows in each group should match
            HS_ASSERT(rowCountMatch);

            MCWrapper::setupMCIterators(mgroup, numRows);
            MCWrapper* tempData = newObjArr(MCWrapper, numRows);
  
            if (!tempData)
               throw ISMemAllocException();
  
            // are columns forming the MC nullable
            NABoolean allColsNullable = tempData->areAllMCColsNullable();
 
            // reset the number of nulls
            tempData->nullCount_ = 0;

            Int32 effectiveNumRows = 0;
            // setup the index value for the MCWrapper objects
            for (Int32 i =0; i < numRows; i++)
            {
               // MC null processing
               // increment the number of nulls if this is a null row
               // and skip it from the sort phase
               if (allColsNullable && tempData->areAllMCColsNull(i))
               {
                  tempData->nullCount_++;
                  // place the null rows at the end and don't sort them
                  tempData[numRows-tempData->nullCount_].setIndex(i);
                  
               }
               else
                 tempData[effectiveNumRows++].setIndex(i);
            }

            mgroup->nullCount = tempData->nullCount_;

            if (LM->LogNeeded())
            {
               sprintf(LM->msg, "\tMC: After set up and null processing: total rows (%d) nulls (%d) non nulls (%d)", 
                       numRows, tempData->nullCount_, effectiveNumRows);
               LM->Log(LM->msg);
            }

            // Initiate sort for specific type by calling the quicksort template function.
            recDepth = maxRecDepth = 0;

            quicksort((MCWrapper*)tempData, 0, effectiveNumRows-1);
            recDepth = maxRecDepth = 0;

            mgroup->data = tempData;
            mgroup->nextData = tempData + numRows;

            if (mgroup->skewedValuesCollected)
            {
               if (LM->LogNeeded())
               {
                  sprintf(LM->msg, "\tMC: generating histogram after Internal sort - skew values ARE collected");
                  LM->Log(LM->msg);
               }

               if (effectiveNumRows)
                 createHistogram(mgroup, intCount, numRows, samplingUsed, (MCWrapper*)NULL);
               else
               {
                 // If the column has all NULLs, then groupHist will not have been allocated
                 // in the call to CreateHistogram.  So, create it here.
                   mgroup->groupHist = new(STMTHEAP) HSHistogram(intCount,
                                                                 mgroup->nullCount,
                                                                 0, // numGapIntervals
                                                                 0, // numHighFreqIntervals
                                                                 samplingUsed,
                                                                 FALSE //singleIntervalPerUec
                                                                );
                   mgroup->groupHist->addNullInterval(mgroup->nullCount, mgroup->colCount);
                   // NOTE: add a new method for MC so the MFV and nMFV are also captured
               }
            }
            else
            {
               if (LM->LogNeeded())
               {
                  sprintf(LM->msg, "\tMC: generating histogram after Internal sort - skew values are NOT collected");
                  LM->Log(LM->msg);
               }

               // temporary CQD used for testing to make sure that fast path stats computation is
               // correct - should remove after validation
               if ( performISForMC() )
                  computeMCISuec (mgroup, tempData, samplingUsed, numRows, intCount);
               else
               {
                  createHistogram(mgroup, 1, numRows, samplingUsed, (MCWrapper*)NULL);
               }
            }

            // free up memory used by the MC group to support IS
            tempData->freeColsMem();
            delObjArr(tempData, MCWrapper);

            // free-up the single col memory
            HSColGroupStruct *sgroup2;
            for (Int32 i=0; i<mgroup->colCount; i++)
            {
               HSColumnStruct &col = mgroup->colSet[i];
               sgroup2 = findGroup(col.colnum);
               sgroup2->mcs_usingme--;
               if (sgroup2->mcs_usingme <= 0)
                  sgroup2->freeISMemory(TRUE, TRUE);
            }

            // flag the group as processed by IS
            mgroup->state = PROCESSED;
            mgroup->mcis_groupWeight.clear();

            if (LM->LogNeeded())
            {
               LM->StopTimer();
            }

            // done computing MC stats using IS
        }
        else if (!usingIS) // regular path to compute stats using SQL
        {
            if (LM->LogNeeded())
            {
               char title[1000];
               sprintf(title, "MC: Compute using SQL (%.940s) with%sskew",
                               mgroup->colNames->data(), collectMCSkewedValues? " " : " NO ");
               LM->StartTimer(title);
               (void)getTimeDiff(TRUE);
            }

            NAString columnName = "", mgroupColNames = "",dblQuote="\"";;
	    for (Int32 i=0; i<mgroup->colCount; i++)
	    {
	      HSColumnStruct &col = mgroup->colSet[i];
	      columnName = ToAnsiIdentifier(col.colname->data());
	      // Surround column name with double quotes, if not already delimited.
	      if (columnName.data()[0] != '"') 
	        columnName=dblQuote+columnName+dblQuote;
	      mgroupColNames.append(columnName);
	      if(i < colCount-1)
	        mgroupColNames.append(",");	    
	    }
    
            /*=================================*/
            /*   CALCULATE MULTI-COLUMN UEC    */
            /*=================================*/
            mgroup->clistr->append("SELECT FMTVAL, SUMVAL FROM (SELECT ");
            mgroup->clistr->append(mgroupColNames);
            if(collectMCSkewedValues)
            {
              mgroup->clistr->append(mgroup->generateTextForColumnCast());
              mgroup->clistr->append(", COUNT(*) FROM ");
            }
            else
              mgroup->clistr->append(", _ucs2'unused', COUNT(*) FROM ");
            mgroup->clistr->append(hssample_table->data());

            Int64 hintRowCount = 0;
            if (sampleTableUsed)
            {
              hintRowCount = sampleRowCount;
            }
            else
            {
              hintRowCount = actualRowCount;
            }

            char cardHint[50];
            sprintf(cardHint, " <<+ cardinality %e >> ", (double)hintRowCount);
            mgroup->clistr->append(cardHint);

            if (samplingUsed && !sampleTableUsed)
               mgroup->clistr->append(sampleOption->data());

            mgroup->clistr->append(" GROUP BY ");
            mgroup->clistr->append(mgroupColNames);
            mgroup->clistr->append(" FOR SKIP CONFLICT ACCESS) T(");
            mgroup->clistr->append(mgroupColNames);
            mgroup->clistr->append(", FMTVAL, SUMVAL)");
            if(collectMCSkewedValues)
            {
              mgroup->clistr->append(" ORDER BY ");
              mgroup->clistr->append(mgroupColNames);
            }
    
            cursor = new(STMTHEAP) HSCursor;
            retcode = cursor->fetchBoundaries(mgroup,
                                              sampleRowCount,
                                              intCount,
                                              samplingUsed);
            mgroup->colSecs = getTimeDiff();
            if (LM->LogNeeded())
            {
               LM->StopTimer();
            }
            delete cursor;
            HSHandleError(retcode);
    
        }

        // Determine boundary values based on single column histograms for MC Group
        // if actual boundary values were not collected.
        if(((!usingIS) || (coveredByIS)) && !collectMCSkewedValues)
        {
                                                 /*==============================*/
                                                 /*= DETERMINE BOUNDARY VALUES  =*/
                                                 /*==============================*/
            low = WIDE_("");
            hi  = WIDE_("");
            NABoolean lowIsFull = FALSE;
            NABoolean hiIsFull = FALSE;
            for (Int32 i=0; i<mgroup->colCount && !(lowIsFull && hiIsFull); i++)
            {
               col = &mgroup->colSet[i];

               sgroup = findGroup(col->colnum);
               HS_ASSERT(sgroup != NULL);
               retcode = sgroup->groupHist->getLowValue(boundLo, FALSE);
               HSHandleError(retcode);

               retcode = sgroup->groupHist->getHighValue(boundHi, FALSE);
               HSHandleError(retcode);
            
               //if BigNum, cast to double
               //because a boundary can have only 250 characters 
               //and it is not large enough to save 2 BigNum numbers with 128 precision.
               //each double precision number needs only 25 characters.
               if (DFS2REC::isBigNum(sgroup->colSet[0].datatype))
               {
                 myVarChar vc = myVarChar();
                 vc.len = boundLo.length();
                 memmove(vc.val, boundLo.data(), vc.len);
                 retcode = doubleToHSDataBuffer(ucsToDouble(&vc), boundLo);
                 HSHandleError(retcode);
   
                 vc.len = boundHi.length();
                 memmove(vc.val, boundHi.data(), vc.len);
                 retcode = doubleToHSDataBuffer(ucsToDouble(&vc), boundHi);
                 HSHandleError(retcode);
               }
   
               //10-031023-0696: make sure that the final boundary value is within
               //the length constraint of the HISTOGRAM tables. Three extra
               //characters must be considered:
               //               1 - comma separator
               //               2 - open and close parenthesis
               if (!lowIsFull && ((low.length() + boundLo.numChars() + 3) < HS_MAX_UCS_BOUNDARY_CHAR))
               {
                   low.append((NAWchar*)boundLo.data(), boundLo.length() / sizeof(NAWchar));
                   low.append(WIDE_(","));
               }
               else
                  lowIsFull = TRUE;
               if (!hiIsFull && ((hi.length() + boundHi.numChars() + 3) < HS_MAX_UCS_BOUNDARY_CHAR))
               {
                   hi.append((NAWchar*)boundHi.data(), boundHi.length() / sizeof(NAWchar));
                   hi.append(WIDE_(","));
               }
               else
                  hiIsFull = TRUE;

            } //end for: column in group

            //remove extra comma separator between column names and copy result
            //to low and high boundaries
            low.remove(low.length() - 1);
            boundLo = low.data();
            hi.remove(hi.length() - 1);
            boundHi = hi.data();
    
            HS_ASSERT(mgroup->groupHist != NULL);
            retcode = mgroup->groupHist->updateMCInterval(boundLo, boundHi);
            HSHandleError(retcode);
        }

        mgroup = mgroup->next;
    }//end while: group

    return retcode;
}



/***********************************************/
/* METHOD:  FixSamplingCounts()                */
/* PURPOSE: When sampling is used, this        */
/*          method is called to estimate the   */
/*          UEC for intervals and histograms.  */
/* RETCODE: 0 - successful                     */
/* PROCESS: Basically, the UEC and ROWCOUNT of */
/*          every interval is increase by some */
/*          extrapolated value. There are two  */
/*          extrapolation methods that are used*/
/*          linear and root extrapolation.     */
/*          Newton-Raphson Method is used for  */
/*          root extrapolation.                */
/***********************************************/
Lng32 HSGlobalsClass::FixSamplingCounts(HSColGroupStruct *group)
  {
    Lng32 retcode = 0;
    Lng32 i, j;
    Lng32 lastInterval;
    Int64 preSumRC, preSumUec, postSumRC, postSumUec, rowAdj, rows, uec;
    Lng32 intJoinCount = 0;
    Lng32 start = -1;
    double combRows  = 0, combUec   = 0, combUecRatio;
    double estRow    = 0, estUec    = 0;
    double estSubRow = 0, estSubUec = 0;
    double lower     = 0;
    const double UEC_FRACTION_UPPER = 0.975;
    const double FRACTION_HIGH = 0.981;
    const double UPSCALE_FOR_ROWS = convertInt64ToDouble(actualRowCount) / sampleRowCount;
    const Lng32 MAX_INTERVAL_JOIN = 4;
    NABoolean processMultiGroups = TRUE;

    HSHistogram *groupHist = NULL;
    char rowCountStr[30];
    char uecStr[30];
    HSLogMan *LM = HSLogMan::Instance();

    // get defaults that tell us which estimator to use, and
    // the max Dsh parameter for the LWC estimator
    //
    NABoolean useLWCEstimator = 
      (CmpCommon::getDefault(USTAT_FORCE_MOM_ESTIMATOR) == DF_OFF);
    double DshMax = CmpCommon::getDefaultNumeric(USTAT_DSHMAX);

    if (LM->LogNeeded())
      {
        LM->Log("\nFixing sampling counts");

        if (useLWCEstimator)
          {
            sprintf(LM->msg, "Using LWC estimator, DshMax %5.1f", DshMax);
            LM->Log(LM->msg);
            LM->Log("Logging both MOM and LWC UEC estimates\n");
          }
        else
          {
            LM->Log("Using MOM estimator");
            LM->Log("Logging only MOM UEC estimates\n");
          }
      }

        groupHist = group->groupHist;
        HS_ASSERT(groupHist != NULL);

        double uecMomTot=0, uecLwcTot=0, uecUjTot=0, uecShTot=0, coeffOfVarTot=0;

        if (LM->LogNeeded())
          {
            sprintf(LM->msg, "Estimating UEC for column group [%s]\n", 
                    group->colNames->data());
            LM->Log(LM->msg);
            LM->Log("Interval(s)    Dlwc        Dsh        Duj    CofV         Dmom");
            LM->Log("-----------  ---------  ---------  --------- ---- --------------------");
          }

        lastInterval = groupHist->getNumIntervals();
        groupHist->getTotalCounts(preSumRC, preSumUec);

        Int32 estimateCnt=0; // Maintain count of times that UEC was estimated.  This
                           // count is used to obtain an average skew (CV) across the
                           // entire histogram.

        for (i=1; i <= lastInterval; i++) {
            groupHist->setIntOrigRC(i, groupHist->getIntRowCount(i));
            groupHist->setIntOrigMFV(i, groupHist->getIntMFVRowCount(i));
        }

        for (i=1; i <= lastInterval; i++)
          {
            // -----------------------------------------------------------------
            // Loop through the intervals for a histogram.  When a sampled interval
            // has UEC > (UEC_FRACTION_UPPER * # rows in sample interval), perform
            // UEC estimation across multiple intervals.  Continue combining intervals
            // while this is true AND the # of intervals combined has not exceeded
            // MAX_INTERVAL_JOIN.
            // -----------------------------------------------------------------

            groupHist->setIntOrigUec(i, groupHist->getIntUec(i));

            combRows += convertInt64ToDouble(groupHist->getIntRowCount(i)); // Not upscaled.
            combUec  += convertInt64ToDouble(groupHist->getIntUec(i));
            intJoinCount++;

            // Scale up all rowcounts for this interval. For most frequent 
            // values, don't scale up if UEC = row count.
            if (groupHist->getIntUec(i) != groupHist->getIntRowCount(i)) {
              groupHist->setIntMFVRowCount(i, (Int64)((double)groupHist->getIntMFVRowCount(i)*UPSCALE_FOR_ROWS));
              groupHist->setIntMFV2RowCount(i, (Int64)((double)groupHist->getIntMFV2RowCount(i)*UPSCALE_FOR_ROWS));
            }

            // Attempt to combine intervals for UEC estimation.
            // If possible, then will immediately loop back.
            if ((i < (lastInterval - 1)) &&
                (intJoinCount < MAX_INTERVAL_JOIN) &&
                (combUec >= (UEC_FRACTION_UPPER * combRows)))
              {
                if (start < 1)
                  start = i;
                continue;
              }

            // If the interval after this one is the last non-null interval,
            // and that interval has UEC > (UEC_FRACTION_UPPER * 
            // # rows in sample interval), then combine this one with that one
            // (and we'll ignore the MAX_INTERVAL_JOIN limit for that one).
            if ( ( ((i == lastInterval - 2) && groupHist->hasNullInterval()) ||
                   ((i == lastInterval - 1) && !groupHist->hasNullInterval())  ) &&
                 (groupHist->getIntUec(i+1) > UEC_FRACTION_UPPER * groupHist->getIntRowCount(i+1))  )
              {
                if (start < 1)
                  start = i;
                continue;            
              }

            intJoinCount = 0;

            // Cannot combine any more intervals, adjust current interval with calculated ratio.
            combUecRatio = combUec / combRows;
            estRow = UPSCALE_FOR_ROWS * combRows;

            // -----------------------------------------------------------------
            // SPECIAL CASE: Processing the interval that contains NULLs
            //               Do not adjust UEC
            // -----------------------------------------------------------------
            if (i == lastInterval && groupHist->hasNullInterval())
              {
                groupHist->setIntRowCount(i, (Int64)estRow);

                if (LM->LogNeeded())
                  {
                    // log UEC estimate for this interval.  the uec estimate in
                    // the full table interval is the same as the uec in the 
                    // sample interval, because this is the special last interval
                    // that contains only null values.  in other words, if we found
                    // one unique value (NULL) in the sample, we expect one unique
                    // value (NULL) to be in the full table as well.
                    //
                    sprintf(LM->msg, "   %5d    (last interval) contains NULLs, not changing UEC", i);
                    LM->Log(LM->msg);
                  }
              }
            else // Not processing the last interval as a NULL interval.
                 // The general case.
              {
                double Duj=0;
                double Dsh=0;
                double Dlwc=0;
                double Dmom=0;
                double intCoeffOfVar=0;

                NABoolean DlwcAvailable = false;
                NABoolean DmomAvailable = false;
                NABoolean DmomEstCalled = false;

                estimateCnt++;  

                // Estimate the UEC for this set of intervals.  There are 2 cases:
                //  1. UEC of sample < sampled rows in set of intervals.
                //  2. UEC of sample = sampled rows in set of intervals.
                if (combUec < combRows)
                  {
                    // If the total sampled UEC in this set of intervals is not equal 
                    // to the sampled number of rows for these intervals:
                    // estimate the number of distinct values in the entire table for
                    // the current interval (or intervals), based on the row count and 
                    // number of distinct values in the current sample interval(s). 

                    if ((LM->LogNeeded() || useLWCEstimator) && groupHist->fi(0))
                      {
                        // Use a UEC estimator that is a linear weighted combination
                        // of the unsmoothed jackknife and Shlosser methods.  
                        //
                        FrequencyCounts *f;
                        
                        if (start < 1)
                          // single interval estimation; use interval i's fi counts
                          //
                          f = groupHist->fi(i);
                        else 
                          {
                            // multiple interval estimation; merge fi counts
                            // of intervals start..i into work fi (f0)
                            //
                            f = groupHist->fi(0);
                            f->reset();
                            for (j=start; j<=i; j++)
                              groupHist->fi(j)->mergeTo(*f);
                          }
                        
                        estUec = lwcUecEstimate(combUec, combRows, estRow, f, 
                                                DshMax, intCoeffOfVar, Duj, Dsh);

                        Dlwc = estUec;
                        DlwcAvailable = true;
                        uecLwcTot += estUec;
                        uecUjTot += Duj;
                        uecShTot += Dsh;
                        coeffOfVarTot += intCoeffOfVar;
                      }
                    
                    if (LM->LogNeeded() || !useLWCEstimator)
                      {
                        estUec = (estRow / combRows) * combUec;
                        
                        //Experimentation using TPCD2X ORDERS and LINEITEM tables,
                        //tells us that if the UEC ratio is between 0.1 and 0.5, we
                        //should NOT call xValue() to get the root value. It will
                        //produce very low UECs. We have added two new CQD for
                        //flexibility.
                        if (combUecRatio >= CmpCommon::getDefaultNumeric(USTAT_UEC_HI_RATIO) ||
                            combUecRatio <= CmpCommon::getDefaultNumeric(USTAT_UEC_LOW_RATIO))
                          {
                            lower  = xValue(combUec, combRows);
                            DmomEstCalled = true;
                            
                            if (combUec <= (combRows * FRACTION_HIGH) &&
                                lower < estUec)
                              estUec = lower;
                          }
                        uecMomTot += estUec;
                        
                        Dmom = estUec;
                        DmomAvailable = true;
                      }

                    // reset estUec to Dlwc, if Dlwc is available and we are
                    // supposed to use it
                    //
                    if (DlwcAvailable && useLWCEstimator) 
                      estUec = Dlwc;

                    if (LM->LogNeeded())
                      {
                        //
                        // log UEC estimates for this interval, or intervals.
                        // for the MOM estimator, also indicate whether we actually
                        // called the estimator (denoted "mom est" in the output),
                        // or if we just scaled up the sample UEC by the sample
                        // fraction (denoted "scaleup" in the output).
                        //
                        char DlwcStr[16], DshStr[16], DujStr[16];
                        if (DlwcAvailable)
                          {
                            sprintf(DlwcStr,"%10.0f",Dlwc);
                            sprintf(DshStr,"%10.0f",Dsh);
                            sprintf(DujStr,"%10.0f",Duj);
                          }
                        else
                          {
                            sprintf(DlwcStr,"%10s"," ");
                            sprintf(DshStr,"%10s"," ");
                            sprintf(DujStr,"%10s"," ");
                          }
                        char DmomStr[32];
                        if (DmomAvailable)
                          sprintf(DmomStr,"%10.0f (%s)",Dmom, 
                                  DmomEstCalled ? "mom est" : "scaleup");
                        else
                          sprintf(DmomStr,"%10s"," ");

                        if (start < 1)
                          sprintf(LM->msg, "   %5d    %10s %10s %10s %4.2f %10s", 
                                  i, DlwcStr, DshStr, DujStr, intCoeffOfVar, DmomStr);
                        else
                          sprintf(LM->msg, "   %2d-%2d    %10s %10s %10s %4.2f %10s", 
                                  start, i, DlwcStr, DshStr, DujStr, intCoeffOfVar, DmomStr);
                        LM->Log(LM->msg);
                      }
                  } // End UEC of sample != rows of sample
                else
                  {
                    // UEC of sample = rows of sample
                    estUec = estRow;
                    lower  = estRow;

                    uecLwcTot += estUec;
                    uecMomTot += estUec;

                    if (LM->LogNeeded())
                      {
                        // 
                        // log UEC estimates for this interval, or intervals. the
                        // estimation that is done here involves setting the estimated
                        // UEC in the full table to the estimated rowCount in the full
                        // table, because the uec in the sample for this interval was
                        // the same as the row count in this sample interval.  in other
                        // words, the values in the sample interval were unique, so we
                        // conclude that the values in the full table interval will be
                        // unique as well
                        //
                        if (start < 1)
                          sprintf(LM->msg, "   %5d    uec==rc, no est; uec: %10.0f", 
                                  i, estUec);
                        else
                          sprintf(LM->msg, "   %2d-%2d    uec==rc, no est; uec: %10.0f", 
                                  start, i, estUec);
                        LM->Log(LM->msg);
                      }
                  }
                                              /*==============================*/
                                              /*    ADJUST SINGLE INTERVAL    */
                                              /*==============================*/

                if (start < 1)
                  {
                    if (estUec > estRow)
                      {
                        estUec = MINOF(estRow, convertInt64ToDouble(LLONG_MAX));
                        if (LM->LogNeeded())
                          {
                            sprintf(LM->msg, "            Adjusted UEC (estUec >estRow): %4.0f", estUec);
                            LM->Log(LM->msg);
                          }
                      }

                    // Adjust MFV and 2MFV for intervals where the original RC and UEC are the same. 
                    // Both MFV and 2MFV have been adjusted at the beginning of the loop when the original
                    // RC and UEC are not the same.
                    if ( groupHist->getIntOrigRC(i) ==  groupHist->getIntOrigUec(i) ) 
                      groupHist->adjustMFVand2MFV(i, estRow, estUec);

                    groupHist->setIntRowCount(i, (Int64)estRow);
                    groupHist->setIntUec(i, (Int64)estUec);

                  }
                                              /*==============================*/
                                              /*  ADJUST MULTIPLE INTERVALS   */
                                              /*==============================*/
                else
                  {
                    for (j = start; j <= i; j++)
                      {
                        estSubRow = (estRow / combRows) * convertInt64ToDouble(groupHist->getIntRowCount(j));
                        estSubUec = (estUec / combUec) * convertInt64ToDouble(groupHist->getIntUec(j));

                        if (estSubUec > estSubRow)
                          estSubUec = MINOF(estSubRow, convertInt64ToDouble(LLONG_MAX));

                        if (LM->LogNeeded())
                          {
                            sprintf(LM->msg, "            Adjusted UEC interval %2d: %4.0f", j, estSubUec);
                            LM->Log(LM->msg);
                          }

                        if ( groupHist->getIntOrigRC(j) ==  groupHist->getIntOrigUec(j)) 
                          groupHist->adjustMFVand2MFV(j, estSubRow, estSubUec);

                        groupHist->setIntRowCount(j, (Int64)estSubRow);
                        groupHist->setIntUec(j, (Int64)estSubUec);
                      }
                  }
              } // End processing interval UEC
            start = -1;
            combUec = combRows = 0;
          } // END LOOP -- for (i=1; i <= lastInterval; i++)

        if (estimateCnt != 0)
          group->coeffOfVar = coeffOfVarTot / estimateCnt; // Assign the coeffOfVar for this histogram
                                                           // now that all intervals have been processed.
        if (LM->LogNeeded())
          {
            char coeffOfVarStr[16],coeffOfVarTotStr[16];
            sprintf(coeffOfVarStr,"%10.0f",group->coeffOfVar);
            sprintf(coeffOfVarTotStr,"%10.0f",coeffOfVarTot);
            sprintf(LM->msg, "\tAverage CofV for column: %s = %s/%d (groups)", 
                    coeffOfVarStr, coeffOfVarTotStr, estimateCnt);
            LM->Log(LM->msg);
          }
                                              /*==============================*/
                                              /*  ROUNDING ERROR ADJUSTMENTS  */
                                              /*==============================*/
        groupHist->getTotalCounts(postSumRC, postSumUec); /* new totals       */
        rowAdj = actualRowCount - postSumRC;
        if (rowAdj != 0)
          {
            // Adjust intervals from loInt to hiInt.  Intervals are numbered 1 to lastInterval.
            //   When (lastInterval == 1 or 2) then (loInt,hiInt) = (1, 1)
            //   When (lastInterval >  2)      then (loInt,hiInt) = (2, lastInterval-1)
            Int32 loInt = (lastInterval <= 2) ? 1 : 2;               // Set to 1 if 1 or 2 intervals.
            Int32 hiInt = (lastInterval <= 2) ? 1 : lastInterval-1;  // Set to 1 if 1 or 2 intervals.

            // Check to see if adjusting all intervals within the range and with >=10000 rows 
            // up by 1 row will accommodate all of rowAdj.  If more intervals are required, 
            // intervals with >=1000 rows are checked.  This continues for no more than 5 passes
            // down to intervals with >=1 row.  Once this limit is chosen, a final pass uses
            // the limit to adjust the intervals.
            // UEC adjustment occurs only if UEC = rowcount and UEC != 1.  The check for UEC != 1
            // will hopefully avoid an invalid situation e.g. UEC=2 and boundary="(X)",
            // where previous boundary="(X-1)". 
            // Since the first and last intervals can be columns used often for join predicates,
            // they will only be used if total # intervals < 3.
            Int32   limit = 10000;  // # of rows required for an interval to be updated.
            Int64 ints  = 0;      // Count of intervals to update.
            while (limit > 1)
              {
                for (i=loInt; i<=hiInt; i++) if (groupHist->getIntRowCount(i)>=limit) ints++;
                if (ints >= rowAdj) break;
                limit /= 10;
                ints = 0;
              }
            
            // NOTE: The while loop attempts to ensure that the final row adjustment is correct.
            ints = 0;
            NABoolean assigned = TRUE; // flag to avoid infinite loop.
            while (ints<rowAdj && assigned)
              {
                assigned = FALSE;
                for (i=loInt; i<=hiInt && ints<rowAdj; i++)
                  if ((rows=groupHist->getIntRowCount(i)) >= limit) 
                    {
                      groupHist->setIntRowCount(i, rows+1);
                      if ((uec=groupHist->getIntUec(i)) == rows && uec != 1) groupHist->setIntUec(i, uec+1);
                      ints++;                  
                      assigned = TRUE;
                    }
              }

            if (LM->LogNeeded())
              {
                sprintf(LM->msg, "\tRounding error was " PF64 " rows.  Added " PF64 " rows "
                        "from interval " "%d to %d.", 
                                 rowAdj, ints, loInt, hiInt);
                LM->Log(LM->msg);
                sprintf(LM->msg, "\tUEC values adjusted if UEC=rows and UEC not 1."); 
                LM->Log(LM->msg);
              }
          }

        if (LM->LogNeeded())
          {
            convertInt64ToAscii(actualRowCount, rowCountStr);
            convertInt64ToAscii(sampleRowCount, uecStr);
            sprintf(LM->msg, "\t[%s] ESTIMATED ROWCOUNT & UEC (ROWCOUNT=%s SAMPLED=%s)", group->colNames->data(), rowCountStr, uecStr);
            LM->Log(LM->msg);
            convertInt64ToAscii(preSumRC, rowCountStr);
            convertInt64ToAscii(preSumUec, uecStr);
            sprintf(LM->msg,"\t\t\tBEFORE: rowcount = %s, uec = %s", rowCountStr, uecStr);
            LM->Log(LM->msg);
            groupHist->getTotalCounts(postSumRC, postSumUec); /* reset - possible adjustment */
            convertInt64ToAscii(postSumRC, rowCountStr);
            convertInt64ToAscii(postSumUec, uecStr);
            sprintf(LM->msg,"\t\t\tAFTER:  rowcount = %s, uec = %s", rowCountStr, uecStr);
            LM->Log(LM->msg);
            sprintf(LM->msg,"\t\t\tLWC:    uec = %4.0f", uecLwcTot);
            LM->Log(LM->msg);
            sprintf(LM->msg,"\t\t\tUJ:     uec = %4.0f", uecUjTot);
            LM->Log(LM->msg);
            sprintf(LM->msg,"\t\t\tSh:     uec = %4.0f", uecShTot);
            LM->Log(LM->msg);
            sprintf(LM->msg,"\t\t\tMOM:    uec = %4.0f", uecMomTot);
            LM->Log(LM->msg);
          }

    checkTime("after fixing sampling counts and performing estimation");
    return retcode;
  }


/***********************************************/
/* METHOD:  ClearAllHistograms()               */
/* PURPOSE: Delete all histograms that were    */
/*          generated for base table           */
/* RETCODE:  0 - successful                    */
/*          -1 - failure                       */
/* DEPENDENCY: Pre-compiled statements in      */
/*             SQLHIST.mdf                     */
/* ASSUMPTIONS: A transaction has already been */
/*              started.                       */
/***********************************************/
// Alternate function definitions -- first for static
// query, then for dynamic query.
#ifdef NA_USTAT_USE_STATIC  // use static query defined in module file
Lng32 HSGlobalsClass::ClearAllHistograms()
  {
    Lng32 retcode = 0;
    char UIDstr[30];
    Int64 xObjectUID = objDef->getObjectUID();
    convertInt64ToAscii(xObjectUID, UIDstr);
    HSCliStatement::statementIndex stmt;
    HSLogMan *LM = HSLogMan::Instance();

    if (LM->LogNeeded())
      {
        sprintf(LM->msg, "\t\t\tDELETE101 (%s, %s)", hstogram_table->data(), UIDstr);
        LM->Log(LM->msg);
      }

    if (tableFormat == SQLMP)       
      stmt = HSCliStatement::DELETE101_MP;
    else
      if (HSGlobalsClass::schemaVersion >= COM_VERS_2300)
        stmt = HSCliStatement::DELETE101_MX_2300;
      else
        stmt = HSCliStatement::DELETE101_MX;

    HSCliStatement delete101(stmt,
                             (char *)hstogram_table->data(),
                             (char *)&xObjectUID);
    retcode = delete101.execFetch("DELETE101()");
    HSHandleError(retcode);

    if (LM->LogNeeded())
      {
        sprintf(LM->msg, "\t\t\tDELETE201 (%s, %s) via DELETE201_MX_2300", hsintval_table->data(), UIDstr);
        LM->Log(LM->msg);
      }

    if (tableFormat == SQLMP)               
      stmt = HSCliStatement::DELETE201_MP; 
    else
      if (HSGlobalsClass::schemaVersion >= COM_VERS_2300)
        stmt = HSCliStatement::DELETE201_MX_2300;
      else
        stmt = HSCliStatement::DELETE201_MX;

    HSCliStatement delete201(stmt,
                             (char *)hsintval_table->data(),
                             (char *)&xObjectUID);
    retcode = delete201.execFetch("DELETE201()");
    HSHandleError(retcode);

    return retcode;
  }

#else // NA_USTAT_USE_STATIC not defined, define version of function to use dynamic query

Lng32 HSGlobalsClass::ClearAllHistograms()
  {
    HS_ASSERT(schemaVersion >= COM_VERS_2300);

    HSLogMan *LM = HSLogMan::Instance();
    Lng32 retcode = 0;
    char UIDstr[30];
    convertInt64ToAscii(objDef->getObjectUID(), UIDstr);

    // Delete from Histograms table.

    NAString stmtText = "DELETE FROM ";
    stmtText.append(hstogram_table->data())
            .append(" WHERE TABLE_UID = ")
            .append(UIDstr);

    retcode = HSFuncExecQuery(stmtText.data());
    HSHandleError(retcode);
    if (LM->LogNeeded())
      {
        sprintf(LM->msg, "\t\t\tDELETE101 (%s, %s) via dynamic query", hstogram_table->data(), UIDstr);
        LM->Log(LM->msg);
      }

    // Delete from Histogram_Intervals table.

    stmtText = "DELETE FROM ";
    stmtText.append(hsintval_table->data())
            .append(" WHERE TABLE_UID = ")
            .append(UIDstr);

    retcode = HSFuncExecQuery(stmtText.data());
    HSHandleError(retcode);
    if (LM->LogNeeded())
      {
        sprintf(LM->msg, "\t\t\tDELETE201 (%s, %s) via dynamic query", hsintval_table->data(), UIDstr);
        LM->Log(LM->msg);
      }

    return retcode;
  }

#endif // NA_USTAT_USE_STATIC not defined

/***********************************************/
/* METHOD:  ClearSelectHistograms()            */
/* PURPOSE: Delete user-selected histograms    */
/*          that were generated for base table */
/* RETCODE:  0 - successful                    */
/*          -1 - failure                       */
/* ASSUMPTIONS: A transaction has already been */
/*              started.                       */
/***********************************************/
Lng32 HSGlobalsClass::ClearSelectHistograms()
  {
    Lng32     retcode = 0;
    Int64    xRows = 0;
    NAString oldHistList;
    NAString stmt;
    char     tempStr[30];
    char     uidStr[30];
    HSColGroupStruct* group;
    NABoolean processMultiGroups;
    HSLogMan *LM = HSLogMan::Instance();

    convertInt64ToAscii(objDef->getObjectUID(), uidStr);

    if (singleGroup)
      {
        group = singleGroup;                      /* process single-columns  */
        processMultiGroups = TRUE;                /* then multi-columns      */
      }
    else
      {
        group = multiGroup;                       /* process multi-columns   */
        processMultiGroups = FALSE;               /* only once               */
      }

    while (group != NULL)
      {
        if (group->oldHistid != 0)
          {
                                          /*=================================*/
                                          /*     DETERMINE OLD HISTOGRAM     */
                                          /*             VALUES              */
                                          /*=================================*/
            sprintf(tempStr, " %u,", group->oldHistid);
            oldHistList += tempStr;
            if (group->oldHistidList != "")
              oldHistList += group->oldHistidList;
          }
        group = group->next;
        if (group == NULL && processMultiGroups)
          {
            processMultiGroups = FALSE;
            group = multiGroup;
          }
      }

    retcode = HSGlobalsClass::removeHists(oldHistList, uidStr, "CLEAR HISTOGRAMS");
    return retcode;
  }

/***********************************************/
/* METHOD:  DeleteOrphanHistograms()           */
/* PURPOSE: Deletes obsolete histograms that   */
/*          may exist. For SQL/MP tables,      */
/*          histograms are not automatically   */
/*          deleted when table is dropped.     */
/* RETCODE:  0 - successful                    */
/*           all error are ignored because it  */
/*           does not hurt or corrupt anything */
/*           if these obsolete histograms exist*/
/* ASSUMPTIONS: A transaction has already been */
/*              started.                       */
/***********************************************/
Lng32 HSGlobalsClass::DeleteOrphanHistograms()
  {
    Lng32     retcode = 0;
    Int64    rows = 0;
    NAString query;
    char     rowCountStr[30];
    HSLogMan *LM = HSLogMan::Instance();

    if (tableFormat == SQLMP)
      {
        query = "DELETE FROM ";
        query += hstogram_table->data();
        query += " WHERE TABLE_UID NOT IN (SELECT CREATETIME FROM ";
        query += objDef->getCatalogLoc(HSTableDef::EXTERNAL_FORMAT);
        query += ".TABLES)";
        retcode = HSFuncExecTransactionalQueryWithRetry
          (query, -UERR_INTERNAL_ERROR, &rows, "CLEAR_ORPHANS",
           NULL, NULL);
        if (LM->LogNeeded())
          {
            convertInt64ToAscii(rows, rowCountStr);
            sprintf(LM->msg, "\t\t\t%s ROWS DELETED FROM %s", rowCountStr, hstogram_table->data());
            LM->Log(LM->msg);
          }

        query = "DELETE FROM ";
        query += hsintval_table->data();
        query += " WHERE TABLE_UID NOT IN (SELECT CREATETIME FROM ";
        query += objDef->getCatalogLoc(HSTableDef::EXTERNAL_FORMAT);
        query += ".TABLES)";
        retcode = HSFuncExecTransactionalQueryWithRetry
          (query, -UERR_INTERNAL_ERROR, &rows, "CLEAR_ORPHANS",
           NULL, NULL);
        if (LM->LogNeeded())
          {
            convertInt64ToAscii(rows, rowCountStr);
            sprintf(LM->msg, "\t\t\t%s ROWS DELETED FROM %s", rowCountStr, hsintval_table->data());
            LM->Log(LM->msg);
          }
      }
    return 0;
  }

/***********************************************/
/* METHOD:  GetStatistics()                    */
/* PURPOSE:                                    */
/*                                             */
/*                                             */
/* RETCODE:  0 - successful                    */
/*          -1 - failure                       */
/* ASSUMPTIONS: Parser has set all options     */
/*    and set up the singleGroup and           */
/*    multiGroup lists.                        */
/*                                             */
/***********************************************/
// General Logic:
// 
// The parser has set up the lists singleGroup and multiGroup.
// Those contain the names of all of the histograms by column name
// that were just requested.  The column name(s) will be taken from
// each entry.  The table group list will be walked looking for matching
// names.  Once the name is found the histogram id will be taken from the
// oldHistid entry. 
//
// That histogram id will be used with a query to get the histogram data
// for that id.

Lng32 HSGlobalsClass::GetStatistics(NAString& displayData, Space& space)
{
    Lng32 retcode = 0;
    NABoolean gotOne = false;

    HSColGroupStruct *tableGroupList;
    HSColGroupStruct *tableGroup;
    HSColGroupStruct *listedGroup;
    retcode = groupListFromTable(tableGroupList); // Reads from HISTOGRAMS table
                                                  // for SERIALIZABLE ACCESS.
    HSHandleError(retcode);
                                          /*=================================*/
                                          /*   WRITE TABLE ID ONCE FOR       */
                                          /*   FOR EACH SHOWSTATS COMMAND    */
                                          /*=================================*/
    if (optFlags & DETAIL_OPT)
       displayData += "Detailed ";
    displayData += "Histogram data for Table ";
    displayData += user_table->data();

    char uidAsChar[20];
    Int64 objID = objDef->getObjectUID();
    convertInt64ToAscii(objID, uidAsChar);
    displayData += "\nTable ID: ";displayData.append(uidAsChar);
    displayData += "\n";

    if (!(optFlags & DETAIL_OPT)){
        displayData += "\n   Hist ID # Ints    Rowcount         UEC Colname(s)";
        displayData += "\n========== ====== =========== =========== ===========================\n";
    }

	                                      /*=================================*/
	                                      /*   If individual histograms      */
	                                      /*   were requested then they are  */
	                                      /*   listed in singleGroup and     */
	                                      /*   multigroup in reverse order.  */
	                                      /*=================================*/
    for(Int32 twice=0;twice<2;twice++){
        if(twice == 0)
            listedGroup = ReverseList(singleGroup);
        else
            listedGroup = ReverseList(multiGroup);
        while (listedGroup != NULL && !retcode)  
	                                      /*=================================*/
	                                      /*   All histograms are listed in  */
	                                      /*   tableGroupList so it is   */
	                                      /*   searched for each one.        */
	                                      /*=================================*/			
        {
            tableGroup = tableGroupList;
            while (tableGroup != NULL && !retcode)
            { 
                if(tableGroup->colSet == listedGroup->colSet && // order doesn't matter
                   tableGroup->reason != HS_REASON_EMPTY)
                {
                    retcode = DisplayHistograms(displayData, space,
                        tableGroup->oldHistid, tableGroup->colNames->data()); 
                    gotOne = true;
                    break;
                }				
                tableGroup = tableGroup->next;
            }
            listedGroup = listedGroup->prev;
        }
    }

    if(!gotOne && !retcode)
        displayData += "\nNo Histograms exist for the requested columns or groups\n";

    return retcode;
}

// Copied from cli/Cli.cpp.
inline NABoolean isERROR(Lng32 retcode)
{
  return (retcode < 0);
}

Lng32 HSGlobalsClass::DisplayHistograms(NAString& displayData, Space& space, 
                                        const ULng32 oldHistId, const char* colnames)                        
{
    Lng32 retcode = 0;
    HSErrorCatcher errorCatcher(retcode, - UERR_INTERNAL_ERROR, "DISPLAY HISTOGRAMS", TRUE);

    // HISTOGRAMS table columns:
    Lng32 colpos = 0;  
    Lng32 colcount = 0;  
    short intcount = 0;  
    Int64 rowcount = 0;  
    Int64 totaluec = 0;  
    Int64 mc_rowcount = 0;  
    Int64 mc_totaluec = 0;  
    double stdv = 0;
    Int64 mfv = 0;
    Int64 mfv2 = 0;
    NAWchar* lowval_;    
    NAWchar* highval_;   
    NAWchar* mfval_;   
    lowval_ =  new(STMTHEAP) NAWchar[HS_MAX_BOUNDARY_LEN + 1];
    highval_ =  new(STMTHEAP) NAWchar[HS_MAX_BOUNDARY_LEN + 1];
    mfval_ =  new(STMTHEAP) wchar_t[HS_MAX_BOUNDARY_LEN + 1];

    short intnumber = 0;   // Matches intcount type.
    
    NABoolean dispMFV = FALSE;
    if (CmpCommon::getDefault(USTAT_SHOW_MFV_INFO) == DF_ON)
      dispMFV = TRUE;

    ULng32 histID = oldHistId;      // Assign HISTOGRAM_ID for query.
    Int64 objID  = objDef->getObjectUID(); // Assign TABLE_UID for query.
    
    char numbuffer[2*HS_MAX_BOUNDARY_LEN];    //  used to write histogram lines to output.
    char numbuffer1[2*HS_MAX_BOUNDARY_LEN];    //  used to write histogram lines to output.
    char mfvbuffer[2*HS_MAX_BOUNDARY_LEN];    //  used to write mfv info  to output.
    char mfvbuffer1[2*HS_MAX_BOUNDARY_LEN];    //  used to write mfv info  to output.
    NAString colnamesStr(colnames);
    char *dummyFirstUntranslatedChar;
    unsigned int outputDataLen;
    
#ifdef NA_USTAT_USE_STATIC  // use static query defined in module file
    HSCliStatement::statementIndex stmt;

    if (tableFormat == SQLMX) 
       if (HSGlobalsClass::schemaVersion >= COM_VERS_2300)
         stmt = HSCliStatement::SHOWHIST_MX_2300;
       else
         stmt = HSCliStatement::SHOWHIST_MX;
    else                      
      stmt = HSCliStatement::SHOWHIST_MP;

    HSCliStatement histData( stmt,
                              (char *)hstogram_table->data(),
                              (char *)&objID,
                              (char *)&histID
                            );

#else // NA_USTAT_USE_STATIC not defined, use dynamic query

    HS_ASSERT(schemaVersion >= COM_VERS_2300);
    char sbuf[25];
    sprintf(sbuf, PF64, objID);
    NAString qry = "SELECT COL_POSITION, COLCOUNT, INTERVAL_COUNT, ROWCOUNT, "
                          "TOTAL_UEC, LOW_VALUE, HIGH_VALUE "
                   "FROM ";
    qry.append(hstogram_table->data())
       .append(   " WHERE TABLE_UID = ")
       .append(sbuf);
    qry.append(   "   AND HISTOGRAM_ID = ");
    sprintf(sbuf, "%d", histID);
    qry.append(sbuf)
       .append(   "   AND REASON <> ' '")
       .append(   " ORDER BY TABLE_UID, HISTOGRAM_ID, COL_POSITION");

    HSCursor histData(STMTHEAP, "DISPLAY_HIST");
    retcode = histData.prepareQuery(qry.data(), 0, 7);
    HSHandleError(retcode);

#endif // NA_USTAT_USE_STATIC not defined

    if (isERROR(retcode=histData.open())) {
        displayData += "\nThe requested histogram does not exist\n";
        NADELETEBASIC(lowval_, STMTHEAP);
        NADELETEBASIC(highval_, STMTHEAP);
        return retcode;
    }

    retcode = histData.fetch(7,
                       (char *)&colpos, (char *)&colcount,
                       (char *)&intcount, (char *)&rowcount,(char *)&totaluec,
                       (char *)&lowval_[0], (char *)&highval_[0]);
    if (isERROR(retcode)) {
        displayData += "\nUnable to fetch the histogram data for the table.\n";
        histData.close();
        return retcode;
    }

    NABoolean hideIntervalInfoForMCGroup = TRUE;
    if(singleGroup || (multiGroup && CmpCommon::getDefault(USTAT_SHOW_MC_INTERVAL_INFO) == DF_ON))
      hideIntervalInfoForMCGroup = FALSE;
    if(hideIntervalInfoForMCGroup)
    {
      mc_rowcount = rowcount;
      mc_totaluec = totaluec;
    }

    // Now format the data to the stream
    if (optFlags & DETAIL_OPT)
    {
        displayData += "\nHist ID:    " + Int64ToNAString(uint32ToInt64(histID)) + 
                       "\nColumn(s):  " + colnamesStr                    +
                       "\nTotal Rows: " + Int64ToNAString(rowcount)      +
                       "\nTotal UEC:  " + Int64ToNAString(totaluec)      +
                       "\nLow Value:  ";
        char *dummyFirstUntranslatedChar;
        unsigned int outputDataLen;

        LocaleToUTF8(cnv_version1,
                     (const char *) &lowval_[1],
                     (short)lowval_[0],
                     numbuffer,
                     sizeof(numbuffer),
                     cnv_UTF16,
                     dummyFirstUntranslatedChar,
                     &outputDataLen,
                     TRUE);
        displayData += numbuffer;
        LocaleToUTF8(cnv_version1,
                     (const char *) &highval_[1],
                     (short)highval_[0],
                     numbuffer,
                     sizeof(numbuffer),
                     cnv_UTF16,
                     dummyFirstUntranslatedChar,
                     &outputDataLen,
                     TRUE);

	    displayData += "\nHigh Value: ";
        displayData += numbuffer;
        displayData += "\nIntervals:  "; 
	    if(hideIntervalInfoForMCGroup)
	      displayData += LongToNAString((Lng32)1);
	    else
          displayData += LongToNAString((Lng32)intcount);
    }
    else
    {
        sprintf(numbuffer, "%10u %6d %11s %11s ", histID, intcount, 
                Int64ToNAString(rowcount).data(), Int64ToNAString(totaluec).data());
        displayData += numbuffer + colnamesStr;
    }

    // Flush the space after each histogram.  allocateAndCopyToAlignedSpace will encode the string
    // like varchar (with an embedded length of size short).  Call for each histogram in order to
    // avoid a buffer overflow in MXCI.  MUST BE CALLED ON LINE BOUNDARY.
    space.allocateAndCopyToAlignedSpace(displayData, displayData.length(), sizeof(short));
    displayData.resize(0);  // clear what was just copied.

    histData.close(); // finished writing information for this histogram.

    // Go ahead to write information for intervals of this histogram if DETAIL
    // option was specified, else return now.
    HSFilterWarning(retcode);  // clean up any warnings before possible return
    if (!(optFlags & DETAIL_OPT))
        return 0;

#ifdef NA_USTAT_USE_STATIC  // use static query defined in module file
    if (tableFormat == SQLMX) 
      if (HSGlobalsClass::schemaVersion >= COM_VERS_2300)
        stmt = HSCliStatement::SHOWINT_MX_2300;
      else
        stmt = HSCliStatement::SHOWINT_MX;
    else                                    
        stmt = HSCliStatement::SHOWINT_MP;

    HSCliStatement intData( stmt,
        (char *)hsintval_table->data(),
        (char *)&objID,
        (char *)&histID);

#else // NA_USTAT_USE_STATIC not defined, use dynamic query

    sprintf(sbuf, PF64, objID);
    qry =       "SELECT INTERVAL_ROWCOUNT, INTERVAL_NUMBER, INTERVAL_UEC, INTERVAL_BOUNDARY,"
                       "STD_DEV_OF_FREQ, V1, V2, V5"
               " FROM ";
    qry.append(hsintval_table->data())
       .append(" WHERE TABLE_UID = ")
       .append(sbuf)
       .append(  " AND HISTOGRAM_ID = ");
    sprintf(sbuf, "%d", histID);
    qry.append(sbuf);
    qry.append(" ORDER BY TABLE_UID, HISTOGRAM_ID, INTERVAL_NUMBER"
               " FOR READ UNCOMMITTED ACCESS");

    HSCursor intData(STMTHEAP, "DISPLAY_HISTINT");
    retcode = intData.prepareQuery(qry.data(), 0, 8);
    HSHandleError(retcode);

#endif // NA_USTAT_USE_STATIC not defined

    if (isERROR(retcode=intData.open())) {
        displayData += "\nUnable to open the Intervals Table.\n";
        return retcode;
    }
    displayData +=   "\nNumber    Rowcount         UEC Boundary";
    if (dispMFV)
    {
      displayData +=   "                                     Stdev MFVRowCount 2ndMFVRowCount MFV";
    }
    displayData +=   "\n====== =========== =========== ======================================";

    if (dispMFV)
      displayData +=   " =========== =========== ============== ======================================";

    // Flush the data after interval header.
    space.allocateAndCopyToAlignedSpace(displayData, displayData.length(), sizeof(short));
    displayData.resize(0);  // clear what was just copied.

    for(short jj=0; jj<=intcount; jj++){
        retcode = intData.fetch(8,
            (void *)&rowcount,(void *)&intnumber,(void *)&totaluec,
            (void *)&highval_[0],(void *)&stdv, (void *)&mfv,(void *)&mfv2,
            (void *)&mfval_[0]);
        if(isERROR(retcode)) {
            displayData += "\nInterval Not Found in the Intervals Table.\n";
            intData.close();
            return retcode;
        }

	    if(hideIntervalInfoForMCGroup)
	    {
          if(jj == intcount)
          {
            rowcount = mc_rowcount;
            totaluec = mc_totaluec;
          }
          else if(jj != 0)
            continue;
        }

        // This will be a max of 5+1+21+1+21+1 = 50 chars.
	    sprintf(numbuffer, "%6d %11s %11s ", (hideIntervalInfoForMCGroup && intnumber) ? 1 : intnumber,
                                              Int64ToNAString(rowcount).data(), 
                                              Int64ToNAString(totaluec).data());

        displayData += numbuffer;

        LocaleToUTF8(cnv_version1,
                      (const char *) &highval_[1],
                      (short)highval_[0],
                      numbuffer1,
                      sizeof(numbuffer1),
                      cnv_UTF16,
                      dummyFirstUntranslatedChar,
                      &outputDataLen,
                      TRUE);
        sprintf(numbuffer, "%-38s", numbuffer1);

        displayData += numbuffer;

        if (dispMFV)
        {
          sprintf(mfvbuffer, "%11.2f ", stdv);
          displayData += mfvbuffer;
          sprintf(mfvbuffer, " %11s %14s ", Int64ToNAString(mfv).data(), Int64ToNAString(mfv2).data());
          displayData += mfvbuffer;
          LocaleToUTF8(cnv_version1,
                        (const char *) &mfval_[1],
                        (short)mfval_[0],
                        mfvbuffer1,
                        sizeof(mfvbuffer1),
                        cnv_UTF16,
                        dummyFirstUntranslatedChar,
                        &outputDataLen,
                        TRUE);
          sprintf(mfvbuffer, "%-38s", mfvbuffer1);
          displayData += mfvbuffer;
        }

        TrimNAStringSpace(displayData, false, true);

        // Flush the data after each interval line to avoid overflowing buffers in MXCI. 
        space.allocateAndCopyToAlignedSpace(displayData, displayData.length(), sizeof(short));
        displayData.resize(0);  // clear what was just copied.
    }
    intData.close();
    displayData += "\n";

    HSFilterWarning(retcode);  // filter out any warnings so HSErrorCatcher doesn't act up

    return 0;
}

HSColGroupStruct* HSGlobalsClass::ReverseList(HSColGroupStruct* list)
{
   HSColGroupStruct *saveGroup;

   if(list==NULL)
      return list;
   saveGroup = list;
   list = list->next;
   while(list != NULL){
       list->prev = saveGroup;
       saveGroup = list;
       list = list->next;
   }
   return saveGroup;
}


/***********************************************/
/* METHOD:  print()                            */
/* PURPOSE: print histogram information to log.*/
/***********************************************/
void HSColGroupStruct::print()
  {
    HSLogMan *LM = HSLogMan::Instance();

    if (this != NULL)
      {
        *LM << "\t\t\t" << colCount << ", " << oldHistid << ", " << newHistid << ", (" << colNames->data() << ")";
        LM->FlushToLog();
        if (this->next != NULL)
          this->next->print();
      }
  }


// This function is called for each rowset that is read for internal sort.
// The null indicators for the data just read are checked starting at the
// front of the array, and when one is found to be set, the corresponding
// value in the data array is overwritten with the first non-null value
// found scanning backwards from the end of the data array. This compacts
// the non-null values in the data array without excessive data movement.
// Nulls are counted as they are found, so the null interval in the histogram
// can be set up properly.
//
template <class T>
void processNullsForColumn(HSColGroupStruct *group, Lng32 rowsRead, T* dummyPtr)
{
  short *frontNull, *backNull;
  T     *frontData, *backData;

  // copy data for MC
  if ( HSGlobalsClass::performISForMC() && (group->mcs_usingme > 0) &&
      (group->ISdatatype != REC_BYTE_F_ASCII) && 
      (group->ISdatatype != REC_BINARY_STRING) && 
      (group->ISdatatype != REC_BYTE_F_DOUBLE) &&
      (group->ISdatatype != REC_BYTE_V_ASCII) && 
      (group->ISdatatype != REC_VARBINARY_STRING) && 
      (group->ISdatatype != REC_BYTE_V_DOUBLE)   
     )

  {
      memcpy (group->mcis_nextData, group->nextData, rowsRead * sizeof(T)); 
      group->mcis_nextData = (T*)group->mcis_nextData + rowsRead;
  }

  if (!group->nullIndics)
    {
      group->nextData = (T*)group->nextData + rowsRead;
      group = group->next; //@ZX -- looks wrong; modifies by-value param right before return?
      return;
    }

  // set the MC bitmap null indicator
  if (group->mcs_usingme > 0)
  {
     Int32 start = ((T*)group->nextData - (T*)group->data) + group->nullCount;
     Int32 end = start + rowsRead;
     frontNull = group->nullIndics;
     for (Int32 idx= start; idx < end; idx++)
     {
        if (*frontNull == -1) 
           group->mcis_nullIndBitMap->setBit(idx);
        frontNull++;
     }
  }

  frontData = (T*)group->nextData;
  backData  = frontData + rowsRead - 1;
  frontNull = group->nullIndics;
  backNull  = frontNull + rowsRead - 1;
  while (frontNull < backNull)
    {
      while (*frontNull != -1 && frontNull < backNull)
        {
          frontNull++;
          frontData++;
        }
      
      while (*backNull == -1 && frontNull < backNull)
        {
          backNull--;
          backData--;
          group->nullCount++;
        }
      
      if (frontNull < backNull)
        {
          *frontData++ = *backData--;
          frontNull++;
          backNull--;
          group->nullCount++;
        }
    }

  if (frontNull == backNull)
    { 
      if (*frontNull == -1)
        group->nullCount++;
      else
        frontData++;
   }

  group->nextData = frontData;
}

// For each column currently being processed for internal sort (denoted by
// state=PENDING, invoke the template function that removes and counts
// nulls from the data array. A dummy null pointer cast to the appropriate
// type is passed to the template function, so the right template
// instantiation will be used.
// In this method, for VARCHAR columns, we do something other than what this 
// method is supposed to do. Probably this new code should be moved out of this
// method to a separate new method in future. 
// For varchar columns, we compute average column data size and store it in 
// group->avgVarCharSize member.
// 
// Return code: 0 on success, -1 on failure.
//
Lng32 HSGlobalsClass::processInternalSortNulls(Lng32 rowsRead, HSColGroupStruct *group)
{
  HSLogMan *LM = HSLogMan::Instance();
  ISFixedChar *chPtr;
  ISVarChar *vchPtr;
  ULng32 sumSize = 0;
  NABoolean computeVarCharSize = FALSE;
  NABoolean maxLongLimit = FALSE;
  short *nullInd = NULL;
  Int32 vcInflatedLen, vcCompactLen;
  char* inflatedDataPtr;
  char *dataPtr, errtxt[100]={0};
  Int32 i;
  Lng32 retcode=0;
  HSErrorCatcher errorCatcher(retcode, - UERR_INTERNAL_ERROR, errtxt, TRUE);

  while (group)
    {
      if (group->state != PENDING && group->state != OVERRAN)
        {
          group = group->next;
          continue;
        }

      // keep track of total number of rows read - used by MC IS
      group->mcis_rowsRead += rowsRead;

      switch (group->ISdatatype)
        {
          case REC_BIN8_SIGNED:
            processNullsForColumn(group, rowsRead, (Int8*)NULL);
            break;

          case REC_BOOLEAN:
          case REC_BIN8_UNSIGNED:
            processNullsForColumn(group, rowsRead, (UInt8*)NULL);
            break;

          case REC_BIN16_SIGNED:
            processNullsForColumn(group, rowsRead, (short*)NULL);
            break;

          case REC_BIN16_UNSIGNED:
            processNullsForColumn(group, rowsRead, (unsigned short*)NULL);
            break;

          case REC_BIN32_SIGNED:
            processNullsForColumn(group, rowsRead, (Int32*)NULL);
            break;

          case REC_BIN32_UNSIGNED:
            processNullsForColumn(group, rowsRead, (UInt32*)NULL);
            break;

          case REC_BIN64_SIGNED:
            processNullsForColumn(group, rowsRead, (Int64*)NULL);
            break;

          case REC_BIN64_UNSIGNED:
            processNullsForColumn(group, rowsRead, (UInt64*)NULL);
            break;

          case REC_IEEE_FLOAT32:
            processNullsForColumn(group, rowsRead, (float*)NULL);
            break;

          case REC_IEEE_FLOAT64:
            processNullsForColumn(group, rowsRead, (double*)NULL);
            break;

          case REC_BYTE_F_ASCII:
          case REC_BYTE_F_DOUBLE:
          case REC_BINARY_STRING:
            // Set up elements of data array, which are pointers to char values.
            chPtr = (ISFixedChar*)group->nextData;
            dataPtr = (char*)group->strNextData;
            for (i=0; i<rowsRead; i++)
              {
                chPtr->setContent(dataPtr);
                dataPtr += group->ISlength;
                chPtr++;
              }
            group->strNextData = dataPtr;  // not affected by null processing
            ISFixedChar::setLength(group->ISlength);
            processNullsForColumn(group, rowsRead, (ISFixedChar*)NULL);
            break;

          case REC_BYTE_V_ASCII:
          case REC_BYTE_V_DOUBLE:
          case REC_VARBINARY_STRING:
            {
              // Set up elements of data array, which are pointers to varchar
              // values (2-byte length field followed by string). The length
              // we advance the ptr by includes an extra byte for alignment if
              // the varchar length is odd.
              // We also compute average varchar data size here which is not
              // something processInternalSortNulls() method should be doing.
              // This new code (computing avg size) will be moved to a separate
              // new method in the future.
              vcInflatedLen = group->inflatedVarcharContentSize();
              vchPtr = (ISVarChar*)group->nextData;
              dataPtr = (char*)group->strNextData;
              inflatedDataPtr = (char*)group->varcharFetchBuffer;
              nullInd = group->nullIndics;
              NABoolean compacted = group->isCompacted();
              Int64 nulls = 0;  // Number of nulls in this batch of values
              for (i=0; i<rowsRead; i++)
                {
                  if (nullInd && *nullInd == -1)
                    {
                      nulls++;
                      if (compacted)
                        inflatedDataPtr += vcInflatedLen;
                      else
                        dataPtr += vcInflatedLen;
                    }
                  else
                  {
                    if (compacted)
                      {
                        vcCompactLen = HSColGroupStruct::varcharContentSize(*(Int16*)inflatedDataPtr);
                        group->sumSize += vcCompactLen;
                        if (group->state == PENDING)  // that is, not OVERRAN
                          {

                            if (dataPtr + vcCompactLen > (char *)group->strData + group->strMemAllocated)
                              {
                                // We underestimated the space needed for compacted varchars,
                                // so don't save anymore. We'll continue to compute the actual
                                // average varchar length though.
                                group->state = OVERRAN; 
                                if (LM->LogNeeded())
                                  {
                                    sprintf(LM->msg, "Exhausted varchar compaction memory for column %s", group->colNames->data());
                                    LM->Log(LM->msg);
                                  }
                              }
                            else
                              { 
                                memcpy(dataPtr, inflatedDataPtr, (ULng32)vcCompactLen);
                                inflatedDataPtr += vcInflatedLen;
                                vchPtr->setContent(dataPtr);
                                dataPtr += vcCompactLen;
                              }
                          }
                      }
                    else
                      {
                        vchPtr->setContent(dataPtr);
                        dataPtr += vcInflatedLen;
                        group->sumSize += vchPtr->getLength();
                      }
                    
                    vchPtr++;
                  }

                  if (nullInd)
                    nullInd++;
                }

              group->nullCount += nulls;
              group->rowsRead += rowsRead;
              // compute average varchar size from running rows count and running sum of varchar sizes
              if (group->rowsRead > 0)
                group->avgVarCharSize = (double)group->sumSize / (double)group->rowsRead;
              group->nextData = vchPtr;
              group->strNextData = dataPtr;
            }
            break;

          default:
            sprintf(errtxt, "processInternalSortNulls(): unknown type %d", 
                            group->ISdatatype);
            sprintf(LM->msg, "INTERNAL ERROR: %s", errtxt);
            LM->Log(LM->msg);
            retcode=-1;
            HSHandleError(retcode);
            break;
        }

      group = group->next;
    }
    return retcode;
}

// Return the maximum amount of memory we want to allocate for internal sort
// at any given time; or -1 if an error occurred
Int64 HSGlobalsClass::getMaxMemory()
{
    // the NAHeap will never exceed around 1.3GB.
    Int64       mem;
    HSLogMan   *LM = HSLogMan::Instance();

    mem = Int64(sysconf(_SC_AVPHYS_PAGES)) * Int64(sysconf(_SC_PAGE_SIZE));

    if (LM->LogNeeded())
      {
        LM->Log("Entering getMaxMemory()");
        sprintf(LM->msg, "The amount of physical memory currently available is %ld", mem);
        LM->Log(LM->msg);

        sprintf(LM->msg, "Will use up to %.7f percent of the min of the available physical memory, and CQD USTAT_NAHEAP_ESTIMATED_MAX",
                         ISMemPercentage_ * 100);
        LM->Log(LM->msg);
      }

  Int64 NAHEAP_ESTIMATED_MAX = (Int64)
                (CmpCommon::getDefaultNumeric(USTAT_NAHEAP_ESTIMATED_MAX) *
                1024 * 1024 * 1024);

  // Limit the amount of assumed available memory by the estimated max heap size.
  if (mem > NAHEAP_ESTIMATED_MAX)
    mem = NAHEAP_ESTIMATED_MAX;

  // Restrict available memory to the percentage we want to use.
  mem = (Int64)(mem * ISMemPercentage_);

  if (LM->LogNeeded())
    {
      // Use function to convert mem to string -- can't use sprintf with %I64d
      // because %I64d doesn't work as a format specifier on NSK.
      strcpy(LM->msg, "getMaxMemory: returned ");
      formatFixedNumeric(mem, 0, LM->msg + strlen(LM->msg));
      LM->Log(LM->msg);
    }

  return mem;
}

// Indicates whether the column can be handled by internal sort.
// Internal sort can handle all current international char sets supported by
// SQL/MX.  There are two encodings for char sets, UCS2 and ISO88591, both
// of these are sorted in a binary manner (there is no special handling) and
// no embedded NULLs (2/26/08).
//
bool isInternalSortType(HSColumnStruct &col)
{
  HSLogMan *LM = HSLogMan::Instance();

  switch (col.datatype)
    {
      case REC_BOOLEAN:
      case REC_BIN8_SIGNED:
      case REC_BIN8_UNSIGNED:
      case REC_BIN16_SIGNED:
      case REC_BIN16_UNSIGNED:
      case REC_BIN32_SIGNED:
      case REC_BIN32_UNSIGNED:
      case REC_BIN64_SIGNED:
      case REC_BIN64_UNSIGNED:
      case REC_DECIMAL_LSE:
      case REC_DECIMAL_UNSIGNED:
      case REC_DECIMAL_LS:
      case REC_IEEE_FLOAT32:
      case REC_IEEE_FLOAT64:
      case REC_BYTE_F_ASCII:
      case REC_BYTE_V_ASCII:
      case REC_BINARY_STRING:
      case REC_VARBINARY_STRING:
        return true;

      case REC_BYTE_F_DOUBLE:
      case REC_BYTE_V_DOUBLE:
        if (col.charset == CharInfo::UNICODE)
          {
            NAString MS1v = ActiveSchemaDB()->getDefaults().getValue(MODE_SPECIAL_1);
            if ( MS1v == "ON" )
              {
                 return false; //In these modes, Internal Sort won't work on UCS2 columns.
              }
          }
        return true;

      case REC_DATETIME:
        switch (col.precision)
          {
            case REC_DTCODE_DATE:
            case REC_DTCODE_TIME:
            case REC_DTCODE_TIMESTAMP:
              return true;
            default:
              LM->Log("INTERNAL ERROR (isInternalSortType):");
              sprintf(LM->msg, "Undefined datetime precision type %d", col.precision);
              LM->Log(LM->msg);
              *CmpCommon::diags() << DgSqlCode(-UERR_GENERIC_ERROR)
                                  << DgString0("isInternalSortType()")
                                  << DgString1("N/A")
                                  << DgString2(LM->msg);
              throw CmpInternalException("failure in isInternalSortType()",
                                         __FILE__, __LINE__);
          }

      case REC_INT_YEAR:
      case REC_INT_MONTH:
      case REC_INT_YEAR_MONTH:
      case REC_INT_DAY:
      case REC_INT_HOUR:
      case REC_INT_DAY_HOUR:
      case REC_INT_MINUTE:
      case REC_INT_HOUR_MINUTE:
      case REC_INT_DAY_MINUTE:
      case REC_INT_SECOND:
      case REC_INT_MINUTE_SECOND:
      case REC_INT_HOUR_SECOND:
      case REC_INT_DAY_SECOND:
        return true;

      default:
        if (LM->LogNeeded())
          {
            sprintf(LM->msg, "Type %d not handled by internal sort: column %s",
                             col.datatype,
                             col.colname->data());
            LM->Log(LM->msg);
          }
        return false;
    }
}

// Determines whether internal sort is the most efficient method for the column,
// based on type and percentage of values that are distinct (using information
// from the column's existing histogram). If there is no existing histogram for
// the column, the values used default to 0, and internal sort will not be used.
//
NABoolean isInternalSortEfficient(Int64 rows, HSColGroupStruct *group)
{
  HSLogMan *LM = HSLogMan::Instance();
  Lng32 dataType = group->ISdatatype;
  NABoolean returnVal;
  double uecRate;
  double uecRateMinForIS = 0;
  
  if (group->prevRowCount == 0)
    uecRate = 0.0;
  else
    uecRate = group->prevUEC / (double)group->prevRowCount;

  // always use IS when there is no existing histogram for the column
  if ((uecRate == 0) && CmpCommon::getDefault(USTAT_USE_IS_WHEN_NO_STATS) == DF_ON)
    {
       returnVal = TRUE;
    }
  else if ((group->mcs_usingme > 0) &&  
           (CmpCommon::getDefault(USTAT_IS_IGNORE_UEC_FOR_MC) == DF_ON)) 
    {
       // if this column is used by MC and MCIS is ON
       // then compute this column using IS regardless of UEC
       returnVal = TRUE;
    }
  else if ((dataType >= REC_MIN_BINARY_NUMERIC &&
       dataType <= REC_MAX_BINARY_NUMERIC)||
       dataType == REC_DECIMAL_LSE ||
       dataType == REC_DECIMAL_UNSIGNED ||
       dataType == REC_DECIMAL_LS)
    {
      // For integral types, number of distinct values must be at least 
      // USTAT_MIN_DEC_BIN_UEC_FOR_IS of total (default 3%).
      uecRateMinForIS = CmpCommon::getDefaultNumeric(USTAT_MIN_DEC_BIN_UEC_FOR_IS);
      returnVal = (uecRate >= uecRateMinForIS);
    }
  else if (DFS2REC::isAnyCharacter(dataType))
    {
      // For char types, if the total amount of data (rows * length) is less than 
      // USTAT_MAX_CHAR_DATASIZE_FOR_IS (default to 1000 MB), use IS. Otherwise
      // the number of distinct values must be at least 
      // USTAT_MIN_CHAR_UEC_FOR_IS of total (default 20%).
      if ( rows * group->ISlength < 1024*1024*CmpCommon::getDefaultNumeric(USTAT_MAX_CHAR_DATASIZE_FOR_IS) )
        returnVal = TRUE;
      else {
        uecRateMinForIS = CmpCommon::getDefaultNumeric(USTAT_MIN_CHAR_UEC_FOR_IS);
        returnVal = (uecRate >= uecRateMinForIS);
      }
    }
  else
    returnVal = TRUE;  // No threshold established yet for other types; use IS

  if (LM->LogNeeded())
    {
      if ((group->mcs_usingme > 0) && (CmpCommon::getDefault(USTAT_IS_IGNORE_UEC_FOR_MC) == DF_ON))
        sprintf(LM->msg, "MCIS is ON and column used by MC, skipping uec check for %s; internal sort will be used",
                         group->colSet[0].colname->data());
      else if (uecRate == 0.0)
        sprintf(LM->msg, "No existing histogram for %s; internal sort will not be used",
                         group->colSet[0].colname->data());
      else if (returnVal)
        sprintf(LM->msg, "%d%% of values for column %s are distinct; internal sort will be used",
                         (Int32)(uecRate * 100 + .5),
                         group->colSet[0].colname->data());
      else
        sprintf(LM->msg, "Only %d%% of values for column %s are distinct; min UEC to use IS is %d%%; "
                         "internal sort will NOT be used",
                         (Int32)(uecRate * 100 + .5),
                         group->colSet[0].colname->data(), 
                         (Int32)(uecRateMinForIS * 100 + .5));
      LM->Log(LM->msg);
    }

  return returnVal;
}

NABoolean HSGlobalsClass::allGroupsFitInMemory(Int64 rows)
{
  Int64 memLeft = getMaxMemory();

  // account for at least one multi-column memory if MC IS is used
   HSColGroupStruct *mgroup = multiGroup;
   while (mgroup && memLeft > 0)
   {
      memLeft -= mgroup->memNeeded;
      mgroup = mgroup->next;
   }
  
  Int32 count = 0;
  HSColGroupStruct *group = singleGroup;
  while (group && memLeft > 0)
    {
      if (group->memNeeded > 0 &&        // was set to 0 if exceeds address space
          group->memNeeded < memLeft &&
          isInternalSortType(group->colSet[0]) &&
          isInternalSortEfficient(rows, group))  
        {
          count++;
          memLeft -= group->memNeeded;
        }
      group = group->next;
    }

  HSLogMan *LM = HSLogMan::Instance();
  if (LM->LogNeeded())
    {
      sprintf(LM->msg, 
         "HSGlobalsClass::allGroupsFitInMemory(): count of single groups that fit =%d, total single count=%d", count, singleGroupCount);
      LM->Log(LM->msg);
    }

  return count == singleGroupCount;
}

// Determines a set of columns to process with internal sort, based on
// available memory. selectSortBatch() is called in a loop, which will
// typically be executed only once. However, when the call to
// allocateMemoryForColumns() returns 0, meaning that memory could not be
// allocated for any columns in the selected batch, we loop again, the
// memory allocation routine having adjusted things to be more conservative
// in the amount of memory we request.
// 
// Parameters:
//   rows - Number of rows we are allocating for. Needed for deleting arrays
//          if we can't allocate all memory needed.
//   internalSortWhenBetter - If TRUE, choose internal sort or standard sort
//          based on expected performance.
//   trySampleTableBypass - If TRUE, read directly into memory instead of sample
//          table if possible.
//
// Return value:
//   The number of columns to process in the next batch.
//
Int32 HSGlobalsClass::getColsToProcess(Int64 rows,
                                     NABoolean internalSortWhenBetter,
                                     NABoolean trySampleTableBypass)
{
  HSLogMan *LM = HSLogMan::Instance();
  Int32 numColsSelected, numColsToProcess;

  do
    {
      numColsSelected = selectSortBatch(rows, internalSortWhenBetter,
                                        trySampleTableBypass);
      if (numColsSelected > 0)
        numColsToProcess = allocateMemoryForInternalSortColumns(rows);
      else
        numColsToProcess = 0;
    }
  while (numColsSelected > 0 && numColsToProcess == 0);

  // If we had to throw some columns back, log the final list of ones to be
  // processed in this batch.
  if (LM->LogNeeded() && numColsSelected > numColsToProcess)
    {
      LM->Log("Columns retained by getColsToProcess:");
      HSColGroupStruct *group = singleGroup;
      while (group != NULL)
        {
          if (group->state == PENDING)
            {
              sprintf(LM->msg, "    %s (" PFSZ " bytes)",
                               group->colSet[0].colname->data(),
                               group->memNeeded);
              LM->Log(LM->msg);
            }
          group = group->next;
        }
    }

  return numColsToProcess;
}

// If we decide to create and load a sample table, deallocate column memory
// and reset PENDING group states back to UNPROCESSED before creating and
// loading the sample table. We'll call getColsToProcess to reallocate it
// again afterwards.
void HSGlobalsClass::deallocatePendingMemory(void)
{
  for (HSColGroupStruct *group = singleGroup; group; group = group->next)
    {
      if (group->state == PENDING)
        {
          group->freeISMemory(TRUE,TRUE);
          group->state = UNPROCESSED;
        }
    }
}

// Select a set of columns for internal sort based on the amount of memory req'd, type,
// and whether the column has already been processed. If ISonlyWhenBetter is true,
// data from the existing histogram is consulted to see if the column is expected
// to perform well with internal sort. However, if trySampleInMemory is true, we
// select all columns if they will all fit in memory at once, and only consider
// expected individual column performance if this can't be done.
//
Int32 HSGlobalsClass::selectSortBatch(Int64 rows, 
                                      NABoolean ISonlyWhenBetter,
                                      NABoolean trySampleInMemory)
{
  HSLogMan *LM = HSLogMan::Instance();

  HSColGroupStruct *group = singleGroup;
  Int32 count = 0;
  Int64 memAllowed = getMaxMemory();
  Int64 memLeft = memAllowed;
  Int64 mcMemUsed = 0;

  // account for at least one multi-column memory if MC IS is used
  if ( performISForMC() )
  {
     HSColGroupStruct *mgroup = multiGroup;
     while (mgroup)
     {
        if (mgroup->state == UNPROCESSED)
        {
           mcMemUsed = mgroup->memNeeded;
           memLeft -= mcMemUsed;
           break;
        }
        mgroup = mgroup->next;
     }
  }
  
  // Visit all unprocessed items looking for ones that fit. No early loop exit if memLeft
  // is 0, because it is very unlikely to hit zero exactly. If trySampleInMemory
  // is set, we ignore whether columns have uec too low to perform with internal
  // sort. If all column samples won't fit in memory, we do it over and take that
  // into account the second time.
  while (group != NULL)
    {
      if (group->state == OVERRAN)
        {
          group->state = UNPROCESSED;

          group->freeISMemory(TRUE,TRUE);  // free old memory

          // recalculate group->memNeeded based on what we now know about
          // average varchar size
 
          group->oldAvgVarCharSize = group->avgVarCharSize;
          group->avgVarCharSize = -1;  // to force a new computation
          group->setISlength(group->ISlength,maxCharColumnLengthInBytes);

          Int64 rows;
          if (sampleRowCount > 0)
            rows = sampleRowCount;
          else
            rows = actualRowCount;
       
          getMemoryRequirementsForOneGroup(group,rows);
        }

      if (group->state == UNPROCESSED &&
          group->memNeeded > 0 &&        // was set to 0 if exceeds address space
          group->memNeeded < memLeft &&
          isInternalSortType(group->colSet[0]) &&
          (trySampleInMemory || !ISonlyWhenBetter || 
           isInternalSortEfficient(rows, group)))  //@ZXuec
        {
          group->state = PENDING;
          count++;
          memLeft -= group->memNeeded;
          group = group->next;
        }
      else if (trySampleInMemory)
        {
          trySampleInMemory = false;
          if (LM->LogNeeded())
            LM->Log("Internal sort: could not fit entire sample in memory");
          if (ISonlyWhenBetter)
            {
              // Can't fit entire sample in memory and USTAT_INTERNAL_SORT was set
              // to HYBRID; reset everything and try again, choosing only columns
              // expected to perform well with internal sort.
              count = 0;
              memLeft = memAllowed;
              group = singleGroup;
              while (group != NULL)
                {
                  if (group->state == PENDING)
                    group->state = UNPROCESSED;
                  group = group->next;
                }
              group = singleGroup;  // restart at beginning
            }
          else
            group = group->next;
        }
      else
        group = group->next;
    }

  if (LM->LogNeeded())
    {
      LM->Log("Columns selected by selectSortBatch:");
      group = singleGroup;
      while (group != NULL)
      {
          if (group->state == PENDING)
            {
              sprintf(LM->msg, "    %s (" PFSZ " bytes)",
                               group->colSet[0].colname->data(),
                               group->memNeeded);
              LM->Log(LM->msg);
            }
          group = group->next;
      }

      if (performISForMC() && (mcMemUsed > 0))
      {
         sprintf(LM->msg, "    multi-column groups (" PFSZ " bytes)", multiGroup->memNeeded);
         LM->Log(LM->msg);
      }
    }


  return count;
}

// Reduce the percentage of physical memory to limit internal sort to, following
// an allocation failure that proved our previous estimate too high. We arbitrarily
// reduce it to 90% of what it was. We could get smarter about this, and base the
// reduction on how much of what we recommended was successfully allocated before
// the failure.
void HSGlobalsClass::memReduceAllowance()
{
  HSLogMan *LM = HSLogMan::Instance();

  ISMemPercentage_ *= .9f;

  if (LM->LogNeeded())
    {
      sprintf(LM->msg, "Reducing ISMemPercentage_ to %f", ISMemPercentage_);
      LM->Log(LM->msg);
    }
}

// Takes corrective action when a memory allocation for internal sort could not
// be made. Remove the offending column and remaining unallocated columns from
// the current internal sort batch.
//
// Parameters:
//   failedGroup - The single-col group which could not be allocated for.
//   firstFailed - TRUE if the allocation failure was on the first column of the batch.
//   rows - Number of rows the allocation is based on. Used for deleting object
//          arrays.
//
void HSGlobalsClass::memRecover(HSColGroupStruct* failedGroup,
                                NABoolean firstFailed,
                                Int64 rows,
                                HSColGroupStruct* mgr)
{
  HSLogMan *LM = HSLogMan::Instance();

  if (LM->LogNeeded())
    {
      LM->Log("<<<Recovering from failed memory allocation for internal sort");
      sprintf(LM->msg, "Memory allocation failed for %s (" PF64 " rows)",
                       failedGroup->colSet[0].colname->data(), rows);
      LM->Log(LM->msg);
    }

  // Reset this and all subsequent pending columns -- no memory for them.
  // If the allocation failure was on the first column attempted, mark it and
  // other columns of equal or greater size as DONT_TRY. While it is only
  // necessary to give up on the first one to ensure that we don't get "stuck"
  // trying the same thing over and over, we shouldn't be bumping up so close
  // to the memory limit, so we avoid trying anything that big again.
  HSColGroupStruct* grp = failedGroup;
  do
    {
      if (firstFailed &&
          grp->memNeeded >= failedGroup->memNeeded &&
          (grp->state == PENDING || grp->state == UNPROCESSED))
        {
          grp->state = DONT_TRY;

          if (LM->LogNeeded())
            {
              sprintf(LM->msg, "Setting column to DONT_TRY: name=%s, memNeeded=" PFSZ,
                                grp->colSet[0].colname->data(),
                                grp->memNeeded);
              LM->Log(LM->msg);
            }

          // if MC in memory is enabled, we should also mark all MCs using this
          // column as DONT_TRY since we won't be able to compute them in memory
          if (performISForMC())
          {
              HSColGroupStruct* mgroup = mgr;

              while (mgroup != NULL)
              {
                  if (mgroup->state != DONT_TRY)
                  {
                     HSColGroupStruct *sgroup;
                     HSColumnStruct* col;
                     for (Int32 i=0; i<mgroup->colCount; i++)
                     {
                        if (grp->colSet[0].colnum == mgroup->colSet[i].colnum)
                        {
                           mgroup->state = DONT_TRY;
                           if (LM->LogNeeded())
                           {
                               sprintf(LM->msg, "MC: Setting MC to DONT_TRY: columns=(%s)",
                                                 mgroup->colSet[0].colname->data());
                               LM->Log(LM->msg);
                           }
                           break;
                        }
                     }
                  }
                  mgroup = mgroup->next;
              }
          }
        }
      else if (grp->state == PENDING)
        {
          grp->state = UNPROCESSED;
          if (LM->LogNeeded())
            {
              sprintf(LM->msg, "Setting column to UNPROCESSED: name=%s, memNeeded=" PFSZ,
                                grp->colSet[0].colname->data(),
                                grp->memNeeded);
              LM->Log(LM->msg);
            }
        }
    }
  while (grp = grp->next);

  if (LM->LogNeeded())
    LM->Log(">>>Finished recovery from failed memory allocation for internal sort");
}

// The data read into memory for IUS consists not only of the primary data
// (from the existing persistent sample), but also the data for the rows to
// be removed from the sample, and those to be inserted into the sample. The
// allocation of memory for these three sets of data for a column must be
// kept consistent, such that on a given pass, the three data sets for a
// column should either all be in memory, or none of them.
//
// This function attempts to allocate the needed memory for all three data
// sets for a given column selected for an IUS batch. If an allocation failure
// occurs for one of the data sets, it ensures that any prior allocation for
// a corresponding data set is undone, and that the state of corresponding
// groups are all the same (typically removing the column from the current
// batch by changing its state from PENDING to UNPROCESSED).
Int32 HSGlobalsClass::allocateMemoryForIUSColumns(HSColGroupStruct* group,
                                                  Int64 rows,
                                                  HSColGroupStruct* delGroup,
                                                  Int64 delRows,
                                                  HSColGroupStruct* insGroup,
                                                  Int64 insRows)
{
  HSLogMan *LM = HSLogMan::Instance();
  if (LM->LogNeeded())
    LM->StartTimer("Allocate storage for IUS columns");

  Int32 numCols = 0;
  HSColGroupStruct* firstPendingGroup = NULL;

  // To simplify the logic of keeping the three groups in sync, place them
  // and their row counts in arrays. On each iteration the array elements
  // are updated to point to the next group in the respective list.
  HSColGroupStruct* groupArr[] = {group, delGroup, insGroup};
  Int64 rowsArr[] = {rows, delRows, insRows};

  NABoolean gotMemory = TRUE;

  // Create storage for query results.
  do
  {
    if (groupArr[0]->state != PENDING)
      {
    	// Skip column not selected for this batch by advancing all 3 groups.
        for (Int16 i=0; i<3; i++)
          groupArr[i] = groupArr[i]->next;
        continue;
      }

    if (!firstPendingGroup)
      firstPendingGroup = groupArr[0];

    // Allocate all memory needed for storing values of each group. If unable
    // to do so for all groups, make necessary adjustments to group states and
    // set flag indicating memory shortfall.
    for (Int16 i=0; i<3 && gotMemory; i++)
      {
        if (!groupArr[i]->allocateISMemory(rowsArr[i]))
          {
        	// Recover from failed allocation (free any partial allocation and
        	// reset the group's state to UNPROCESSED). Also do this for any
        	// groups already allocated, e.g., if the allocation fails for
        	// delGroup, back out the allocation for the primary group for the
        	// column in question.
            Int16 j;
            HSColGroupStruct *grpi, *grpj;
            for (j=i; j>=0; j--)
              {
                memRecover(groupArr[j], groupArr[0] == firstPendingGroup, rowsArr[j], NULL);
                if (j < i)                     // free memory for groups in this set
                  groupArr[j]->freeISMemory(); //   that were already allocated
              }

            // For groups not allocated yet, don't need to free anything, but
            // make sure the states of corresponding groups are the same.
            // memRecover() will have changed the PENDING state of some of the
            // columns, and if the corresponding delGroup and/or insGroup are
            // not changed, they will be part of the queries to read the sample
            // decrement/increment, for columns that are not being processed in
            // this batch.
            for (j=i+1; j<3; j++)
              {
                grpi = groupArr[i];
                grpj = groupArr[j];
                while (grpi)
                {
                  grpj->state = grpi->state;
                  grpi = grpi->next;
                  grpj = grpj->next;
                }
              }
            gotMemory = FALSE;
            memReduceAllowance();
          }
        else
          {
        	// Allocation was successful.
            groupArr[i]->nextData = groupArr[i]->data;
            groupArr[i]->mcis_nextData = groupArr[i]->mcis_data;
          }
      }

    // If the allocation was successful, increment the count of columns for
    // which memory was allocated, and advance to the next element in each
    // sequence of groups (primary, delete, and insert). If the allocation
    // was not successful, the loop will exit (recovery from the allocation
    // failure has been performed within the loop).
    if (gotMemory)
      {
        for (Int16 i=0; i<3; i++)
          groupArr[i] = groupArr[i]->next;
        numCols++;
      }

  } while (gotMemory && groupArr[0]);

  if (LM->LogNeeded())
    LM->StopTimer();

  return numCols;
}

// Allocates memory needed for internal sort for all columns marked as PENDING.
//
// Parameters:
//   rows - Number of rows to base allocation on.
//
// Return value:
//   Number of columns memory was successfully allocated for.
//
Int32 HSGlobalsClass::allocateMemoryForColumns(HSColGroupStruct* group,
                                               Int64 rows,
                                               HSColGroupStruct* mgr)
{
  HSLogMan *LM = HSLogMan::Instance();
  Int32 numCols = 0;
  HSColGroupStruct *firstPendingGroup = NULL;

  if (LM->LogNeeded())
    LM->StartTimer("Allocate storage for columns");

  // Create storage for query results.
  do
  {
     if (group->state != PENDING)
       continue;

     if (!firstPendingGroup)
       firstPendingGroup = group;

     // Allocate all memory needed for internal sort of the column. If unable,
     // to do so, make necessary adjustments to group states and bail out.
     if (!group->allocateISMemory(rows))
       {
         memRecover(group, group == firstPendingGroup, rows, mgr);
         memReduceAllowance();
         break;
       }
     //trafodion-2978
     //group->mcis_memFreed may be set TRUE in HSColGroupStruct::freeISMemory
     //so if allocate memory success,set group->mcis_memFreed to FALSE agin.
     if(group->mcis_memFreed)
         group->mcis_memFreed = FALSE;
     //trafodion-2978
     group->nextData = group->data;
     group->mcis_nextData = group->mcis_data;
     numCols++;
  } while (group = group->next);

  if (LM->LogNeeded())
    LM->StopTimer();

  return numCols;
}

Int32 HSGlobalsClass::allocateMemoryForInternalSortColumns(Int64 rows)
{
  return allocateMemoryForColumns(singleGroup, rows, multiGroup);
}

Lng32 HSGlobalsClass::prepareToReadColumnsIntoMem(HSCursor *cursor, Int64 rows)
{
  HSLogMan *LM = HSLogMan::Instance();
  Lng32 retcode = 0;
  HSColGroupStruct *group = singleGroup;
  NAString internalSortQuery;

  HSErrorCatcher errorCatcher(retcode, - UERR_INTERNAL_ERROR, 
                              "PREPARE_TO_READ_COLS_INTO_MEM", TRUE);

  // Create query to get data for the desired columns.
  internalSortQuery = "SELECT ";
  bool firstExpn = true;
  Int32 ct = 0;
  do
    {
      if (group->state == PENDING)  
        {
          group->rowsRead = 0;
          group->sumSize = 0;

          if (firstExpn)
            firstExpn = false;
          else
            internalSortQuery.append(", ");
          internalSortQuery.append(group->ISSelectExpn);

          ct++;
        }
    }
  while (group = group->next);

  if ( ct == 0 ) return 0;

  internalSortQuery.append(" FROM ");
  // hssample_table->data() will be the real table name if a sample table is not used.
  internalSortQuery.append(hssample_table->data());

  Int64 hintRowCount = 0;
  if (sampleTableUsed)
  {
     hintRowCount = sampleRowCount;
  }
  else
  {
     hintRowCount = actualRowCount;
  }

  char cardHint[50];
  sprintf(cardHint, " <<+ cardinality %e >> ", (double)hintRowCount);
  internalSortQuery.append(cardHint);

  if (samplingUsed && !sampleTableUsed)
     internalSortQuery.append(sampleOption->data());

  internalSortQuery.append(" FOR SKIP CONFLICT ACCESS");  

  LM->Log("Preparing rowset...");
  // Allocate descriptors and statements for CLI and prepare rowset by
  // assigning location for results to be written.
  // prepareRowset may do retries
  retcode = cursor->prepareRowset(internalSortQuery.data(), FALSE, singleGroup,
       (Lng32)MINOF(MAX_ROWSET, rows));
  if (retcode < 0) HSHandleError(retcode) else retcode=0; // Set to 0 for warnings.
  LM->Log("...rowset prepared");

  return retcode;
}


/***********************************************/
/* METHOD: readColumnsIntoMem()                */
/* PURPOSE: reads a set of columns from a      */
/*          table into memory so they can be   */
/*          sorted internally.                 */
/* PARAMETERS:                                 */
/*   cursor -- Cursor to use to read rows from */
/*             the table (may be a temporary   */
/*             sample table).                  */
/*   rows -- Maximum number of rows to read.   */
/*           Memory for the column values has  */
/*           been allocated based on this, so  */
/*           don't exceed it.                  */
/* RETURN CODE: 0 on success.                  */
/*              -1 on failure.                 */
/***********************************************/
Lng32 HSGlobalsClass::readColumnsIntoMem(HSCursor *cursor, Int64 rows)
{
  HSLogMan *LM = HSLogMan::Instance();
  Lng32 retcode = 0;
  Int64 rowsLeft = rows;

  HSErrorCatcher errorCatcher(retcode, - UERR_INTERNAL_ERROR,
                              "READ_COLS_INTO_MEM", TRUE);

  prepareToReadColumnsIntoMem(cursor, rows);

  LM->Log("fetching rowsets...");
  if (LM->LogNeeded())
    LM->StartTimer("Fetching rowsets for internal sort");
  sampleRowCount = 0;
  while (retcode >= 0          // allow warnings
         && retcode != HS_EOF  // exit if no more data
         && rowsLeft > 0)      // internal CLI error if 0 used for # rows to read
    {
      retcode = cursor->fetchRowset();
      if (retcode == 0)  // 1 or more rows successfully read
        {
          sampleRowCount += cursor->rowsetSize();
          rowsLeft = rows - sampleRowCount;
          // We also compute average data size for VarChar columns, which is not
          // something processInternalSortNulls() method should be doing.
          // This new code (computing avg size) will be moved from 
          // processInternalSortNulls() to a separate new method in the future.
          retcode = processInternalSortNulls(cursor->rowsetSize(), singleGroup);
          HSHandleError(retcode);
          Lng32 rowsetSize = (Lng32)MINOF(MAX_ROWSET, rowsLeft);
          if (rowsetSize > 0)
            retcode = cursor->setRowsetPointers(singleGroup,rowsetSize);
        }
    }

  // Deallocate buffer uncompacted varchars were read into prior to being compacted.
  HSColGroupStruct* group = singleGroup;
  while (group != NULL)
  {
    if (group->state == PENDING && group->isCompacted())
    {
      NADELETEBASIC((short*)(group->varcharFetchBuffer), STMTHEAP);
      group->varcharFetchBuffer = NULL;
    }
    group = group->next;
  }

  if (retcode < 0) HSHandleError(retcode) else retcode=0; // Set to 0 for warnings.

  // some post-reading to memory processing to support MC in-memory computation
  if ( performISForMC() )
  {
     group = singleGroup;
     do
     {
        if (group->state == PENDING) 
        { 
           // free memory used by null bitmap if column has no null values
           if ((group->mcis_nullIndBitMap != NULL) && (group->nullCount == 0))
           {
              NADELETEBASIC(group->mcis_nullIndBitMap, STMTHEAP);
              group->mcis_nullIndBitMap = NULL;
           }

           // in case we are reading this column again to memory after it was already 
           // sorted because of MC IS, we need to set the group as PROCESSED so
           // it does not go through the sorting logic again
           if (group->mcis_readAsIs == TRUE)
              group->state = PROCESSED;
        }
     }
     while (group = group->next);

  }

  if (LM->LogNeeded())
    {
      LM->StopTimer();

      char intStr[30];
      convertInt64ToAscii(sampleRowCount, intStr);
      sprintf(LM->msg, "HSGlobalsClass::readColumnsIntoMem(): %s rows read in", intStr);
      LM->Log(LM->msg);
    }

  LM->Log("...done fetching rowsets");
  return retcode;
}

// Invoke the quicksort template instantiation for the passed column's type.
// Return code: 0 on success, -1 on failure.
Lng32 doSort(HSColGroupStruct *group)
{
  Lng32 retcode = 0;
  char errtxt[100]={0};
  HSErrorCatcher errorCatcher(retcode, - UERR_INTERNAL_ERROR, errtxt, TRUE);

  HSLogMan *LM = HSLogMan::Instance();

  // Sort routine can't handle empty array. group->data is ptr to start of
  // array of values, group->nextData points to 1st address after end of array.
  // If group->nextData is not greater, there is no data to sort.
  if (group->nextData <= group->data)
    return retcode;
  
  // Initiate sort for specific type by calling the quicksort template function.
  recDepth = maxRecDepth = 0;

  if (LM->LogNeeded())
    {
      sprintf(LM->msg, "Do quicksort for column %s",
                       group->colSet[0].colname->data());
      LM->StartTimer(LM->msg);
    }

  switch (group->ISdatatype)
    {
      case REC_BIN8_SIGNED:
        quicksort((Int8*)group->data, 0,
                  (Int8*)group->nextData - (Int8*)group->data - 1);
        break;

      case REC_BOOLEAN:
      case REC_BIN8_UNSIGNED:
        quicksort((UInt8*)group->data, 0,
                  (UInt8*)group->nextData - (UInt8*)group->data - 1);
        break;

      case REC_BIN16_SIGNED:
        quicksort((short*)group->data, 0,
                       (short*)group->nextData - (short*)group->data - 1);
        break;

      case REC_BIN16_UNSIGNED:
        quicksort((unsigned short*)group->data, 0,
                       (unsigned short*)group->nextData - (unsigned short*)group->data - 1);
        break;

       case REC_BIN32_SIGNED:
        quicksort((Int32*)group->data, 0,
                       (Int32*)group->nextData - (Int32*)group->data - 1);
        break;

      case REC_BIN32_UNSIGNED:
        quicksort((UInt32*)group->data, 0,
                       (UInt32*)group->nextData - (UInt32*)group->data - 1);
        break;

      case REC_BIN64_SIGNED:
        quicksort((Int64*)group->data, 0,
                       (Int64*)group->nextData - (Int64*)group->data - 1);
        break;

      case REC_BIN64_UNSIGNED:
        quicksort((UInt64*)group->data, 0,
                       (UInt64*)group->nextData - (UInt64*)group->data - 1);
        break;

      case REC_IEEE_FLOAT32:
        quicksort((float*)group->data, 0,
                       (float*)group->nextData - (float*)group->data - 1);
        break;

      case REC_IEEE_FLOAT64:
        quicksort((double*)group->data, 0,
                       (double*)group->nextData - (double*)group->data - 1);
        break;

      case REC_BYTE_F_ASCII:
      case REC_BYTE_F_DOUBLE:
      case REC_BINARY_STRING:
      {
        //
        // Set the GLOBAL ISFixedChar instance with this column's values
        //
        ISFixedChar::setLength(group->ISlength);
        ISFixedChar::setCaseInsensitive(group->colSet[0].caseInsensitive == 1);
        ISFixedChar::setColCollation(group->colSet[0].colCollation);
        ISFixedChar::setCharSet(group->colSet[0].charset);

        // Allocate buffers for operator == and operator < to use.
        Int16 nPasses = CollationInfo::getCollationNPasses(group->colSet[0].colCollation);
        Int32   encodeKeyBufLen = group->ISlength * nPasses + 2 + nPasses;

        if ( encodeKeyBufLen > lengthOfSortBufrs )
        {
           //free memory for smaller buffers
           if ( sortBuffer1 ) NADELETEBASIC(sortBuffer1, STMTHEAP);
           if ( sortBuffer2 ) NADELETEBASIC(sortBuffer2, STMTHEAP);

           sortBuffer1 = new (STMTHEAP) char[encodeKeyBufLen];
           sortBuffer2 = new (STMTHEAP) char[encodeKeyBufLen];
           lengthOfSortBufrs = encodeKeyBufLen ;
        }

        quicksort((ISFixedChar*)group->data, 0,
                       (ISFixedChar*)group->nextData - (ISFixedChar*)group->data - 1);
        break;
      }
      case REC_BYTE_V_ASCII:
      case REC_BYTE_V_DOUBLE:
      case REC_VARBINARY_STRING:
      {
        //
        // Set the GLOBAL ISVarChar instance with this column's values
        //
        ISVarChar::setCaseInsensitive(group->colSet[0].caseInsensitive == 1);
        ISVarChar::setColCollation(group->colSet[0].colCollation);
        ISVarChar::setCharSet(group->colSet[0].charset);

        Int16 nPasses = CollationInfo::getCollationNPasses(group->colSet[0].colCollation);
        Int32 encodeKeyBufLen = group->ISlength * nPasses + 2 + nPasses;

        if ( encodeKeyBufLen > lengthOfSortBufrs )
        {
           //free memory for smaller buffers
           if ( sortBuffer1 ) NADELETEBASIC(sortBuffer1, STMTHEAP);
           if ( sortBuffer2 ) NADELETEBASIC(sortBuffer2, STMTHEAP);

           sortBuffer1 = new (STMTHEAP) char[encodeKeyBufLen];
           sortBuffer2 = new (STMTHEAP) char[encodeKeyBufLen];
           lengthOfSortBufrs = encodeKeyBufLen ;
        }

        quicksort((ISVarChar*)group->data, 0,
                       (ISVarChar*)group->nextData - (ISVarChar*)group->data - 1);
        break;
      }
      default:
        sprintf(errtxt, "doSort(): unknown type %d", group->ISdatatype);
        sprintf(LM->msg, "INTERNAL ERROR: %s", errtxt);
        LM->Log(LM->msg);
        retcode = -1;
        HSHandleError(retcode);
        break;
    }

  if (LM->LogNeeded())
    {
      LM->StopTimer();
      sprintf(LM->msg, "Maximum recursion depth for %s was %d",
              group->colSet[0].colname->data(), maxRecDepth);
      LM->Log(LM->msg);
    }
  if (recDepth != 0) 
    {
      sprintf(errtxt, "doSort(): Recursion depth should be 0.");
      sprintf(LM->msg, "INTERNAL ERROR: %s", errtxt);
      LM->Log(LM->msg);
      retcode = -1;
      HSHandleError(retcode);
    }
  return retcode;
}

/************************************************/
/* METHOD: sortByColInMem()                     */
/* PURPOSE: Iterate through single-column group */
/*          list, and call doSort() for those   */
/*          that are in the current internal    */
/*          sort batch.                         */
/* RETURN CODE: 0 on success, -1 on failure.    */
/************************************************/
Lng32 HSGlobalsClass::sortByColInMem()
{
  HSColGroupStruct *group = singleGroup;
  HSLogMan *LM = HSLogMan::Instance();
  Lng32 retcode = 0;

  // Sort for each column
  LM->StartTimer("Do internal sort for single-columns");
  do
  {
    if (group->state == PENDING)
      {
        if (LM->LogNeeded())
          {
            sprintf(LM->msg, "sortByColInMem: Starting sort of column %s",
                             group->colSet[0].colname->data());
            LM->Log(LM->msg);
          }

        (void)getTimeDiff(TRUE);
        retcode = doSort(group);
        HSHandleError(retcode);
        checkTime("after sorting column in memory");
        group->colSecs = getTimeDiff();  // saved for automation

        if (LM->LogNeeded())
          {
            sprintf(LM->msg, "sortByColInMem: Finished sort of column %s",
                             group->colSet[0].colname->data());
            LM->Log(LM->msg);
          }
      }
  } while (group = group->next);
  LM->StopTimer();
  return retcode;
}


// Do an in-place quicksort on sortArr, using wide pivots.
//
template <class T>
void quicksort(T* sortArr, Int64 lowInx, Int64 highInx)
{
  Int64 pivotWidth;
  Int64 pivotInx = (Int64)(lowInx + ((double)rand() / RAND_MAX) * (highInx - lowInx));

  recDepth++;
  if (recDepth > maxRecDepth)
    maxRecDepth = recDepth;

  pivotInx = placeWidePivot(sortArr, lowInx, highInx,
                            pivotInx, pivotWidth);

  if (pivotInx > -1)
    {
      if (lowInx < pivotInx - 1)
        quicksort(sortArr, lowInx, pivotInx - 1);
      if (pivotInx + pivotWidth < highInx)
        quicksort(sortArr, pivotInx + pivotWidth, highInx);
    }
  recDepth--;
}

// The wide pivot optimization implemented by this function correctly places
// all instances of the designated pivot value in a single call. Instances of
// the pivot value are counted as they are encountered, and overwritten with
// a value from the end of the list. When the correct location for the pivot
// value is found, the n occurrences of the value are written starting at
// that location, after moving the current data at those locations to the
// vacated spots at the end of the list.
//
// Input params:
//   sortArr -- array of values containing data to sort.
//   lowInx, highInx -- boundaries of the portion of the array to sort.
//   pivotInx -- index within the array of the value to use as the pivot.
// Output param:
//   pivotWidth -- number of occurrences of the pivot value found and placed.
// Return value:
//   The index at which the first of the pivot values was placed.
//
template <class T>
Int64 placeWidePivot(T* sortArr, Int64 lowInx, Int64 highInx, Int64 pivotInx,
                     Int64& pivotWidth)
{
  // endPtr is the final position in the array, while lastPtr is the position
  // of the rearmost value that hasn't been used to overwrite an instance of
  // the pivot value. It is decremented each time a value from the end is
  // moved to the location of a discovered pivot instance.
  T *endPtr, *currPtr, *storePtr, *lastPtr;
  T temp;
  
  // Can't use reference for this, because the array element will be moved.
  // Have to use copy ctor to create new object for char wrapper classes.
  const T pivot = sortArr[pivotInx];

  pivotWidth = 0;
  currPtr = sortArr + lowInx;
  storePtr = sortArr + lowInx;
  lastPtr = sortArr + highInx;
  while (currPtr <= lastPtr)
    {
      // If the next value to look at is less than the pivot, swap it with the
      // value at storePtr and increment storePtr -- all items prior to storePtr
      // must be less than the pivot value.
      if (*currPtr < pivot)
        {
          temp = *storePtr;
          *storePtr = *currPtr;
          *currPtr = temp;
          storePtr++;
          currPtr++;
        }
      else if (*currPtr == pivot)
        {
          // Don't increment currPtr here, we need to check the new value moved into
          // this location. Just increment the number of pivot values found, and
          // overwrite it with the value at the end of the list.
          pivotWidth++;
          *currPtr = *lastPtr--;
        }
      else
        currPtr++;
    }

  // All values have been checked, and all those less than the pivot value have
  // been moved to the left of storePtr, so storePtr is the location we want to
  // start writing the pivotWidth instances of the pivot value. First, we clear
  // space for the pivotWidth values starting at storePtr to the locations we
  // previously vacated at the end of the array.
  endPtr = sortArr + highInx;
  currPtr = MINOF(lastPtr, storePtr+pivotWidth-1);
  while (currPtr >= storePtr)
    {
      *endPtr-- = *currPtr--;
    }

  // Now move the pivot value instances into the proper location, which begins
  // at storePtr.
  currPtr = storePtr;
  while (currPtr < storePtr+pivotWidth)
    {
      *currPtr++ = pivot;
    }

  // If pivotWidth == high - low, then there is a single non-pivot value, but
  // it is guaranteed to be in the right place, so we still return -1 to
  // indicate no further recursion is required for this partition.
  //
  return (pivotWidth >= highInx - lowInx ? -1 : storePtr - sortArr);
}


template <class T>
void checkForBackwardness(HSGlobalsClass * hsGlobals, HSColGroupStruct *group, T * listitem1, T * listitem2)
{
  if (*listitem1 > *listitem2)
    {
      group->backwardWarningCount++;
      if (group->backwardWarningCount < 5)  // report this warning at most 5 times per column
        {
          // raise a warning that we found data in backwards order, which means
          // we might get out-of-order histograms, which is bad
          hsGlobals->diagsArea << DgSqlCode(UERR_UNEXPECTED_BACKWARDS_DATA)
               << DgString0(group->colSet[0].colname->data()); 
        }    
    }
}

template < >
void checkForBackwardness(HSGlobalsClass * hsGlobals, HSColGroupStruct *group, 
MCWrapper * listitem1, MCWrapper * listitem2)
{
  // TODO: write this method when necessary; it's a no-op for now
}


// The data in the column's data array has been sorted but not grouped.
// Iterate over the values, counting duplicates. When a new value is
// encountered, create a new group, consisting of a distinct value and
// its occurrence count. An initial number of groups must be stored up
// until we have enough to apply the required adjustment to the number of
// intervals to use. Ultimately, each group (value/count pair) is passed to
// addIntervalData() to be incorporated into an interval.
// The purpose of dummyPtr is to cause the correct template instantiation
// to be used.
// 
template <class T>
void createHistogram(HSColGroupStruct *group, Lng32 numIntervals, 
                     Int64 estRowCount, NABoolean usingSample, T* dummyPtr)
{
  Int64 numValues = (T*)group->nextData - (T*)group->data;
  NABoolean singleIntervalPerUec = FALSE;
  NABoolean allGroupsSeen = FALSE;
  Int32 adjustedIntervalCount;
  Lng32 numGapIntervals;
  Lng32 numHighFreqIntervals;

  // This should not happen unless all the rows in the table are deleted between
  // the time we check the row count and the time the data is actually read, but
  // we have to watch out for it or an infinite loop could occur.
  if (numValues <= 0) 
    return;

  HSGlobalsClass *hsGlobals = GetHSContext();
  boundarySet<T> *distinctValues = new (STMTHEAP) boundarySet<T>;

  T *listitem1 = (T*)group->data;  // adjacent sorted values
  T *listitem2 = listitem1+1;
  Int32 numRows;                     // number of rows with same value
  Int64 valueIndex = 0;            // index of current raw value
  Int32 valueCountIndex;             // index of distinct (grouped) value

  NABoolean firstRowset = TRUE;
  double gapAvgSoFar = 0;
  Int64 gapCountSoFar = 0;
  double gapMultiplier = 0;
  NABoolean bigGap;
  double currGapMagnitude;

do
 {
  valueCountIndex = 0;
  numRows = 0;
  while (valueCountIndex < MAX_ROWSET && valueIndex < numValues-1)
    {
      numRows++;
      if (*listitem1 != *listitem2)
        {
          // Do this for each distinct value.
          distinctValues->data[valueCountIndex] = *listitem1;
          distinctValues->dataSum[valueCountIndex] = numRows;
          distinctValues->nullInd[valueCountIndex] = 0; // nulls already handled
          valueCountIndex++;
          numRows=0;
        }
      checkForBackwardness(hsGlobals,group,listitem1,listitem2);
      listitem1++;
      listitem2++;
      valueIndex++;
    }

  // If we exited the above loop because we ran out of values before running out
  // of room for them, add the final distinct value to the list (loop exits with
  // 1 distinct value outstanding). In the rare case that the number of distinct
  // values is a multiple of MAX_ROWSET, and the final distinct value is a
  // singleton, both conditions of the above loop become true at the same time,
  // and the following 'if' will be false. In that case there is a single value
  // with a single occurrence left, and it will be handled in the next iteration
  // of the do...while(!allGroupsSeen) loop. In that final iteration, the while
  // loop above will not be entered because the condition on valueIndex is still
  // true, and the 'if' below will be true this time and process the final value.
  if (valueIndex == numValues - 1 && valueCountIndex < MAX_ROWSET)
    {
      numRows++;
      distinctValues->data[valueCountIndex] = *listitem1;
      distinctValues->dataSum[valueCountIndex] = numRows;
      distinctValues->nullInd[valueCountIndex] = 0; // nulls already handled
      valueCountIndex++;
      allGroupsSeen = TRUE;
    }

  distinctValues->size = valueCountIndex;

  // Need to determine that we have a certain number of intervals before the
  // interval count can be adjusted, and have to have the adjusted interval
  // count before instantiating the HSHistogram object. Do this only after
  // forming the first set of groups.
  if (firstRowset)
    {
      adjustedIntervalCount = 
          hsGlobals->getAdjustedIntervalCount(group, 
                                              numIntervals,
                                              estRowCount,
                                              valueCountIndex,
                                              singleIntervalPerUec,
                                              numGapIntervals,
                                              numHighFreqIntervals);

      // Now that we have adjusted the interval count, the HSHistogram can be
      // created.
      //@ZX add nullCount to non-null count to get same results as existing code,
      //    although we get a better dispersal of values by omitting nulls from the
      //    calculation (they go into their own interval).
      group->groupHist = new (STMTHEAP) HSHistogram(adjustedIntervalCount,
                                               numValues + group->nullCount,
                                               numGapIntervals,
                                               numHighFreqIntervals,
                                               usingSample,
                                               singleIntervalPerUec);
      gapMultiplier = group->groupHist->getGapMultiplier();
    }

  if (numGapIntervals > 0)
    profileGaps(group, distinctValues, gapAvgSoFar, gapCountSoFar,
                firstRowset);

  firstRowset = FALSE;

  // Pass the buffered distinct value/count pairs one at a time to the function
  // that forms intervals, and then proceed reading values and forming groups. Note
  // that if the buffer contains all the groups (i.e., the sorted data was exhausted
  // while forming the buffered groups), we pass TRUE as the last argument to
  // addIntervalData() for the final group, since there will no more groups passed
  // to it below.
  for (Int32 i=0; i<valueCountIndex; i++)
    {
      currGapMagnitude = distinctValues->gapMagnitude[i];
      bigGap = currGapMagnitude > gapAvgSoFar * gapMultiplier
                  // high frequency intervals not considered as possible gap intervals
               && distinctValues->dataSum[i] <= group->groupHist->getHighFreqThreshold()
               && group->groupHist->gapKeeper_.insert(currGapMagnitude);
      group->groupHist->addIntervalData(distinctValues->data[i],
                                        group,
                                        distinctValues->dataSum[i],
                                        bigGap,
                                        currGapMagnitude,
                                        allGroupsSeen && i == valueCountIndex-1);
    }

 } while (!allGroupsSeen);

 hsGlobals->checkTime("after forming intervals for a single column");

 // Now that all distinct values and their frequencies have been seen, we
 // know the actual gap average. Revisit the gap intervals we created,
 // and keep the gapIntCount ones with the highest gap magnitude. The rest
 // are merged into adjacent intervals, unless that would create an interval
 // of excessive height.
 if (numGapIntervals > 0)
   group->groupHist->removeLesserGapIntervals(gapAvgSoFar);
}


// For each column in the current internal sort batch, call the template
// function to create a histogram from the sorted column values.
//
// Parameters:
//   rowsAllocated - Number of rows memory allocation was based on. Used to
//                   delete object arrays allocated for the columns just
//                   processed.
// Return code: 0 on success, -1 on failure.
//
Lng32 HSGlobalsClass::createStats(Int64 rowsAllocated)
{
  Lng32 retcode = 0;

  HSColGroupStruct *group = singleGroup;
  // Create histogram for each column in this batch (denoted by state=PENDING).
  do
  {
    if (group->state != PENDING)
      continue;

    retcode = createStatsForColumn(group, rowsAllocated);
    group->groupHist->logIntervals();

  } while (group = group->next);

  return retcode;
}

Lng32 HSGlobalsClass::createStatsForColumn(HSColGroupStruct *group, Int64 rowsAllocated)
{
  Lng32 retcode = 0;
  char errtxt[100]={0};
  HSErrorCatcher errorCatcher(retcode, - UERR_INTERNAL_ERROR, errtxt, TRUE);

  HSLogMan *LM = HSLogMan::Instance();
  
  // Create histogram for the group.

    // Invoke template function to create histogram for the column's type.
    switch (group->ISdatatype)
    {
      case REC_BIN8_SIGNED:
        createHistogram(group, intCount, sampleRowCount, samplingUsed, (Int8*)NULL);
        break;

      case REC_BOOLEAN:
      case REC_BIN8_UNSIGNED:
        createHistogram(group, intCount, sampleRowCount, samplingUsed, (UInt8*)NULL);
        break;

       case REC_BIN16_SIGNED:
        createHistogram(group, intCount, sampleRowCount, samplingUsed, (short*)NULL);
        break;

      case REC_BIN16_UNSIGNED:
        createHistogram(group, intCount, sampleRowCount, samplingUsed, (unsigned short*)NULL);
        break;

     case REC_BIN32_SIGNED:
        createHistogram(group, intCount, sampleRowCount, samplingUsed, (Int32*)NULL);
        break;

      case REC_BIN32_UNSIGNED:
        createHistogram(group, intCount, sampleRowCount, samplingUsed, (UInt32*)NULL);
        break;

      case REC_BIN64_SIGNED:
        createHistogram(group, intCount, sampleRowCount, samplingUsed, (Int64*)NULL);
        break;

      case REC_BIN64_UNSIGNED:
        createHistogram(group, intCount, sampleRowCount, samplingUsed, (UInt64*)NULL);
        break;

      case REC_BYTE_F_ASCII:
      case REC_BYTE_F_DOUBLE:
      case REC_BINARY_STRING:
        //
        // Set the GLOBAL ISFixedChar instance with this column's values
        //
        ISFixedChar::setLength(group->ISlength);
        ISFixedChar::setCaseInsensitive(group->colSet[0].caseInsensitive == 1);
        ISFixedChar::setColCollation(group->colSet[0].colCollation);
        ISFixedChar::setCharSet(group->colSet[0].charset);

        createHistogram(group, intCount, sampleRowCount, samplingUsed, (ISFixedChar*)NULL);
        break;

      case REC_BYTE_V_ASCII:
      case REC_BYTE_V_DOUBLE:
      case REC_VARBINARY_STRING:
        //
        // Set the GLOBAL ISVarChar instance with this column's values
        //
        ISVarChar::setCaseInsensitive(group->colSet[0].caseInsensitive == 1);
        ISVarChar::setColCollation(group->colSet[0].colCollation);
        ISVarChar::setCharSet(group->colSet[0].charset);

        createHistogram(group, intCount, sampleRowCount, samplingUsed, (ISVarChar*)NULL);
        break;

      case REC_IEEE_FLOAT32:
        createHistogram(group, intCount, sampleRowCount, samplingUsed, (float*)NULL);
        break;

      case REC_IEEE_FLOAT64:
        createHistogram(group, intCount, sampleRowCount, samplingUsed, (double*)NULL);
        break;

      default:
        sprintf(errtxt, "createStats(): unknown type %d", group->ISdatatype);
        sprintf(LM->msg, "INTERNAL ERROR: %s", errtxt);
        LM->Log(LM->msg);
        retcode=-1;
        HSHandleError(retcode);
        break;
    }

    if (LM->LogNeeded())
      {
        sprintf(LM->msg, PF64 " nulls found for column %s", group->nullCount,
                         group->colSet[0].colname->data());
        LM->Log(LM->msg);
      }
    if (group->nullCount) 
    {
      // If the column has all NULLs, then groupHist will not have been allocated
      // in the call to CreateHistogram.  So, create it here.
      if (!group->groupHist) 
        group->groupHist = new(STMTHEAP) HSHistogram(intCount,
                                                     group->nullCount,
                                                     0, // numGapIntervals
                                                     0, // numHighFreqIntervals
                                                     samplingUsed,
                                                     FALSE //singleIntervalPerUec
                                                    );
      group->groupHist->addNullInterval(group->nullCount, group->colCount);
    }

    // Upscale rowcounts and estimate UECs when sampling.
    if (samplingUsed && sampleRowCount > 0 && actualRowCount > sampleRowCount)
    {
      retcode = FixSamplingCounts(group);
      HSHandleError(retcode);
      if (group->groupHist) group->groupHist->deleteFiArray();
    }

    // This is the final step of processing for internal sort, so mark the
    // column as PROCESSED so it won't be considered again.
    group->state = PROCESSED;

    // Free up the column's data now that we're done with it. This would be
    // done for us at end of statement, but we are dealing with large lumps
    // of memory for possibly long times, so we free it as soon as possible.
    // If MC IS is ON and there are MCs using this column, then keep this
    // column in memory until all MCs using it are properly computed
    if ( !HSGlobalsClass::performISForMC() || group->mcs_usingme == 0)
    {
       group->freeISMemory(TRUE,(group->mcs_usingme==0));
    }

  return retcode;
}

/************************************************/
/* METHOD:  log()                               */
/* PURPOSE: Write selected information to the   */
/*          log. This is done for just-in-time  */
/*          to collect information about the    */
/*          Update Stats statement at the point */
/*          of failure.                         */
/* INPUT:   Pointer to the Log Manager instance.*/
/************************************************/
void HSGlobalsClass::log(HSLogMan* LM)
{
  // Show table stats are being collected for.
  sprintf(LM->msg, "Updating stats for table %s",
                    objDef->getObjectFullName().data());
  LM->Log(LM->msg);

  // Show actual and sample row counts.
  sprintf(LM->msg, "Actual row count = " PF64, actualRowCount);
  LM->Log(LM->msg);
  sprintf(LM->msg, "Sample row count = " PF64, sampleRowCount);
  LM->Log(LM->msg);

  // Whether or not the statement is due to a request from compiler.
  sprintf(LM->msg, "Requested by compiler?: %s",
                   requestedByCompiler ? "Yes" : "No");
  LM->Log(LM->msg);

  // Show the single-column groups this statement is creaing histograms for.
  HSColGroupStruct* group = singleGroup;
  LM->Log("\nSingle-Column Groups");
  LM->Log(  "--------------------");
  while (group)
    {
      if (group->colNames)
        LM->Log(group->colNames->data());
      else
        LM->Log("<name not available>");
      group = group->next;
    }

  // Show the multi-column groups this statement is creaing histograms for.
  group = multiGroup;
  LM->Log("\nMulti-Column Groups");
  LM->Log(  "-------------------");
  while (group)
    {
      if (group->colNames)
        {
          sprintf(LM->msg, "(%s)", group->colNames->data());
          LM->Log(LM->msg);
        }
      else
        LM->Log("<names not available>");
      group = group->next;
    }
}

NABoolean HSGlobalsClass::wherePredicateSpecifiedForIUS()
{
  return  optFlags & IUS_OPT;
}

NAString& HSGlobalsClass::getWherePredicateForIUS()
{
   if (ius_where_condition_text == NULL)
      ius_where_condition_text = new(CTXTHEAP) NAString("");

   return (*ius_where_condition_text);
}

// Return the following string in the queryText parameter:
// delete from <smplTable> where <whereCondition>
void HSGlobalsClass::generateIUSDeleteQuery(const NAString& smplTable,
                                            NAString& queryText,
                                            NABoolean transactional)
{
  if (transactional)
    queryText = "DELETE FROM ";
  else
    queryText = "DELETE WITH NO ROLLBACK FROM ";

  queryText.append(smplTable.data());

  NAString& whereClause = getWherePredicateForIUS();
  if (whereClause.length() > 0) {
    queryText.append(" WHERE ");
    queryText.append(whereClause);
  }
}

// Create statement to add rows to the IUS persistent sample table.
//   upsert using load into into <smplTable>...
//
// If doing full IUS, the new sample table rows are already in the temporary
// _I table, and the source for the upsert is
//       (select * from <smplTable>_I)
//
// If a limited IUS (update persistent sample table and generate histograms),
// the source for the upsert is the source table with the IUS where predicate
// and sampling rate applied:
//       (select * from <sourceTable> where <predicate> sample random <sampleRate> percent)
void HSGlobalsClass::generateIUSSelectInsertQuery(const NAString& smplTable,
                                                  const NAString& sourceTable,
                                                  NAString& queryText)
{
  queryText.append("UPSERT USING LOAD INTO "); // for algorithm 1
  queryText.append(smplTable.data());
  queryText.append(" (SELECT ");
  
  // Generate the select list. Truncate any over-long char/varchar columns
  // by using SUBSTRING calls. Omit any LOB columns.
  objDef->addTruncatedSelectList(queryText);

  queryText.append(" FROM ");

  if (CmpCommon::getDefault(USTAT_INCREMENTAL_UPDATE_STATISTICS) == DF_ON)
    {
      queryText.append(smplTable.data());
      queryText.append("_I)");
    }
  else
    {
      queryText.append(sourceTable.data());
      queryText.append(" where ");
      queryText.append(getWherePredicateForIUS());
      NAString sampleOpt;
      createSampleOption(SAMPLE_RAND_1,
                         sampleRateAsPercetageForIUS * 100.0,
                         sampleOpt, 0, 0);
      queryText.append(sampleOpt);
      queryText.append(")");
    }
}

NABoolean HSGlobalsClass::okToPerformIUS()
{
  return CmpCommon::getDefault(USTAT_INCREMENTAL_UPDATE_STATISTICS) != DF_OFF;
}

// If IUS cqd is set to full ON position, use IUS to incrementally update
// histograms as well as persistent sample table.
NABoolean HSGlobalsClass::useIUSForHistograms()
{
  return CmpCommon::getDefault(USTAT_INCREMENTAL_UPDATE_STATISTICS) == DF_ON;
}

NABoolean HSGlobalsClass::getPersistentSampleTableForIUS(NAString& tableName, 
                Int64 &requestedRows, Int64 &sampleRows, double &sampleRate,
                NABoolean forceToFetch) 
{
  HSLogMan *LM = HSLogMan::Instance();
  LM->StartTimer("IUS: Read from persistent_samples table to find S(i-1)");

  if ( !okToPerformIUS() )
     return FALSE;

  // Fetch the IUS sample table name from SB_PERSISTENT_SAMPLES.

  HSPersSamples *sampleList = HSPersSamples::Instance(objDef->getCatName(),
                                                      objDef->getSchemaName());
  if ( !sampleList ) return FALSE;

  Lng32 retcode = sampleList->find(objDef, 'I', tableName,
                                   requestedRows, sampleRows, sampleRate
                                  );

  if ( retcode == 0 )
    sampleRate /= 100; // Remove the 100x factor from the percent rate value
                       // obtained from the persistent_samples meta-data table.

  LM->StopTimer();

  return ( retcode == 0 && tableName.length() > 0);
}

template <class T>
double delta(T x, T y)
{
  return (double)((x>y) ? x-y : y-x);
}
              
template <class T>
T HSGlobalsClass::convertToISdatatype(T* dummy,  // just so compiler can instantiate
                                      const HSDataBuffer& valToConvert,
                                      HSColGroupStruct* group)
{
  // First check for empty source value. This happens for MFV of 1st interval.
  if (valToConvert.length() == 0)
    return 0;

  T val;
  HSColumnStruct& col = group->colSet[0];
  ComDiagsArea diagsArea;
  ComDiagsArea *diagsAreaPtr = &diagsArea; 
  Lng32 dataConversionErrorFlag;
  if (col.datatype == REC_DATETIME ||
      col.datatype >= REC_MIN_INTERVAL && col.datatype <= REC_MAX_INTERVAL)
    {
      // We have to extract the body of the datetime or interval literal value,
      // because convDoIt() doesn't handle the full literal syntax. For example,
      // if the value is INTERVAL'20 6' YEAR TO MONTH, we have to pass "20 6".
      // convDoIt() doesn't produce datetime values that are consistent with the
      // internal representation we use for IS/IUS, so we have specialized
      // conversions for those.
      NAWchar* litBodyPtr = (NAWchar*)(valToConvert.data());
      NAWchar* lastCharPtr = litBodyPtr + valToConvert.numChars() - 1;
      while (litBodyPtr < lastCharPtr && *litBodyPtr != L'\'')
        litBodyPtr++;
      HS_ASSERT(litBodyPtr < lastCharPtr);
      litBodyPtr++;  // Go past single quote to 1st char of literal body
      while (lastCharPtr > litBodyPtr && *lastCharPtr != L'\'')
        lastCharPtr--;
      HS_ASSERT(lastCharPtr > litBodyPtr);

      if (col.datatype == REC_DATETIME)
        {
          // Datetime returned by convDoIt is a sequence of integer fields: 2 bytes
          // for year. 1 each for month/day/hour/minute/second, 4 bytes for
          // fractional precision.
          static const Int16 DFV_LEN = sizeof(Int16) + 5 + sizeof(Int32);
          char dateFieldValues[DFV_LEN];
          const char* dfvPtr = dateFieldValues;
          convDoIt((char*)litBodyPtr, (lastCharPtr - litBodyPtr) * sizeof(NAWchar), REC_BYTE_F_DOUBLE, 0, 0,
                  dateFieldValues, DFV_LEN, col.datatype, col.precision, col.scale,
                  NULL, 0, STMTHEAP, &diagsAreaPtr,
                  CONV_UNKNOWN,
                  &dataConversionErrorFlag,
                  0);
          switch (col.precision)
            {
              case REC_DTCODE_DATE:
                {
                  // Copy year to ensure alignment.
                  Int16 year;
                  memcpy(&year, dfvPtr, sizeof year);
                  val = (T)ExpDatetime::getTotalDays(year,
                                                     *(dfvPtr+2),  // month
                                                     *(dfvPtr+3)); // day
                }
                break;
              case REC_DTCODE_TIME:
                {
                  val = (T)(*dfvPtr * 3600 +
                            *(dfvPtr+1) * 60 +
                            *(dfvPtr+2));
                  if (col.scale)
                    {
                      // Copy fractional seconds field for alignment.
                      Int32 fracSec;
                      val *= (T)pow(10, col.scale);
                      memcpy(&fracSec, dfvPtr+3, sizeof fracSec);
                      val += fracSec;
                    }
                }
                break;
              case REC_DTCODE_TIMESTAMP:
                val = (T)DatetimeType::julianTimestampValue
                                         (dfvPtr,
                                          col.scale ? DFV_LEN : DFV_LEN - sizeof(Int32),
                                          col.scale);
                break;
              default:
                HS_ASSERT(FALSE);
                break;
            }
        }
      else  // an interval of some sort
        {
          convDoIt((char*)litBodyPtr, (lastCharPtr - litBodyPtr) * sizeof(NAWchar), REC_BYTE_F_DOUBLE, 0, 0,
                  (char*)&val, sizeof(T), col.datatype, col.precision, col.scale,
                  NULL, 0, STMTHEAP, &diagsAreaPtr,
                  CONV_UNKNOWN,
                  &dataConversionErrorFlag,
                  0);
        }
    }
  else if (col.datatype >= REC_MIN_DECIMAL && col.datatype <= REC_MAX_DECIMAL ||
           col.datatype >= REC_MIN_BINARY_NUMERIC && col.datatype <= REC_MAX_BINARY_NUMERIC)    //scale > 0, per caller
    {
      // The fractional part has been normalized to use the full number of scale
      // digits (e.g., 123.1 is represented as "123.100" for a Numeric(6,3)).
      T intPart = 0, fracPart = 0;
      if (sizeof(T) > 4)
        na_swscanf((const NAWchar*)valToConvert.data(), L"%lld.%lld", &intPart, &fracPart);
      else
        na_swscanf((const NAWchar*)valToConvert.data(), L"%d.%d", &intPart, &fracPart);
      val = intPart * (T)pow(10, col.scale) + fracPart;
    }
  else
    {
      // Don't know about this type -- should have been detected earlier and not
      // used with IUS.

      HS_ASSERT(FALSE);
    }

  return val;
}

Int32 computeKeyLengthInfo(Lng32 datatype)
{
  // Only need to handle types used for IS/IUS. Datetime/interval types and
  // non-integral fixed numerics are all converted to one of these types.
  switch (datatype)
    {
      case REC_BIN16_SIGNED:
      case REC_BPINT_UNSIGNED:
      case REC_BIN16_UNSIGNED:
        return ExHDPHash::SWAP_TWO;

      case REC_BIN32_SIGNED:
      case REC_BIN32_UNSIGNED:
      case REC_FLOAT32:
        return ExHDPHash::SWAP_FOUR;

      case REC_BIN64_SIGNED:
      case REC_BIN64_UNSIGNED:
      case REC_FLOAT64:
        return ExHDPHash::SWAP_EIGHT;

      default:
        return ExHDPHash::NO_FLAGS;
    }
    return ExHDPHash::NO_FLAGS;
}

template <class T>
void IUSValueIterator<T>::init(HSColGroupStruct* group)
{
  // Strings must be contiguous in the strData buffer for this iterator to
  // work correctly.
  HS_ASSERT(group->strDataConsecutive);
  vp = (T*)group->data;
}

template <class T>
Int32 HSGlobalsClass::processIUSColumn(T* ptr,
                      const NAWchar* format,
                      HSColGroupStruct* smplGroup,
                      HSColGroupStruct* delGroup,
                      HSColGroupStruct* insGroup)
{
  Int32 retcode = 0;
  HSHistogram* hist = smplGroup->groupHist;
  Lng32 numIntervals = hist->getNumIntervals();
  Lng32 numNonNullIntervals = hist->hasNullInterval()
                                    ? numIntervals - 1
                                    : numIntervals;

  // If the existing histogram is all nulls, and the incremental sample contains
  // any non-nulls, fall back to RUS so new intervals can be created correctly.
  if (numNonNullIntervals == 0 &&
      iusSampleInsertedInMem->getNumRows() > insGroup->nullCount)
    return UERR_WARNING_IUS_NO_LONGER_ALL_NULL;

  Int64 insertFailCount = 0;   // count attempted insertions into CBF that fail
  char title[100];

  HSLogMan *LM = HSLogMan::Instance();

  if (LM->LogNeeded()) {
     sprintf(LM->msg, "IUS: process column %s", smplGroup->colSet[0].colname->data());
     LM->StartTimer(LM->msg);
  }

  // Allocate and initialize 2 arrays: array of type-specific boundary
  // values to use for binary search for the interval that values
  // belong to, and array of MFV values. Include a space for the special
  // low-value interval (index 0).
  T* boundaryValues = new T[numNonNullIntervals+1];
  T* MFVValues = new T[numNonNullIntervals+1];

  // transfer to group for potential use within mergeDatasetsforIUS().
  smplGroup->boundaryValues = boundaryValues;
  smplGroup->MFVValues = MFVValues;

  // If there is only a null interval, the code to extract the boundary value of
  // the low-value interval (which is "(NULL)") as an instance of type T will get
  // an assertion failure. So skip this code if that is the case; these 2 arrays
  // won't be used anyway in this case because the existing sample contains no
  // non-nulls to place in intervals, and if there are non-nulls in _I, we will
  // have returned above with UERR_WARNING_IUS_NO_LONGER_ALL_NULL.
  if (numNonNullIntervals > 0) {
    for (Lng32 i=0; i<=numNonNullIntervals; i++) {
        convertBoundaryOrMFVValue(hist->getIntBoundary(i), smplGroup,
                                  i, boundaryValues, format);
        convertBoundaryOrMFVValue(hist->getIntMFV(i), smplGroup,
                                  i, MFVValues, format);
    }
  }

  if (LM->LogNeeded()) {
     LM->StopTimer();
  }

  // Use a counting bloom filter to track frequency information. Call its
  // insert() method for each value of smplGroup and insGroup, and its
  // remove() method for each value of delGroup.
  // 

  // TBD: to get the max std deviation u of frequency per interval and use
  // it to adjust the average frequency of keys with low frequency as
  // totalRC / totalUec + 3*u, to be used as the 4th argument in CBF cstr below.
  // for ( Lng32 idx = 1; idx<numIntervals; idx++ ) {
  // }


  float false_prob = (float)CmpCommon::getDefaultNumeric(USTAT_INCREMENTAL_FALSE_PROBABILITY);

  if (LM->LogNeeded()) {
     sprintf(title, "IUS: setup CBF");
     LM->StartTimer(title);
  }

   Lng32 maxHashsToUse =
    (ActiveSchemaDB()->getDefaults()).getAsLong(USTAT_IUS_MAX_NUM_HASH_FUNCS);


   CountingBloomFilter* cbf = smplGroup->cbf;

  Int64 rowInx;

  // Create a value iterator that encapsulates the necessary operations to
  // manipulate the values being processed in the in-memory table, and move
  // from one to the next regardless of the underlying column type.
  IUSValueIterator<T> valIter(ptr);

  Int32 intervalIdx;

  // Array to use for interval counts as a backup to CBF (only used for logging
  // and comparison to CBF counts).
  Int64* intvlRC = NULL;
  if (LM->LogNeeded()) {
    intvlRC = new(STMTHEAP) Int64[numNonNullIntervals+1];
    memset(intvlRC, 0, sizeof(Int64) * (numNonNullIntervals+1));
  }
     
  if ( cbf == NULL ) {


      smplGroup->cbf = cbf = new (STMTHEAP)
                CountingBloomFilterWithKnownSkews(
                        STMTHEAP,

                        (UInt32)maxHashsToUse,

                        // The expected number of distinct keys
                        // @WARN: Have to do a narrowing cast here, ctor only takes a UInt32.
                        MAXOF(
                            (UInt32)(sampleRowCount -
                                     iusSampleDeletedInMem->getNumRows() +
                                     iusSampleInsertedInMem->getNumRows()),
                             UInt32(hist->getTotalUec())
                             ),

                        // probability of false positives, from CQD
                        false_prob,

                        // averate frequency plus the max stddev times 3
                        UInt32(hist->getTotalRowCount() / hist->getTotalUec())
                            + 3*UInt32(ceil(hist->getMaxStddev())),

                        // expected # of keys with high frequency. 
                        // +1 so that interval# maps to bucket# directly
                        (UInt32)((hist->getNumIntervals()+1) * 2),

                        // # of intervals that keys mapped to. 
                        // +1 so that interval# maps to bucket# directly
                        hist->getNumIntervals()+1
                                                 );


     if ( LM->LogNeeded() ) {
        sprintf(LM->msg, "currentSampleSize=" PF64 ", deleteSize=" PF64 ", insertSize=" PF64 " ", 
                     sampleRowCount, iusSampleDeletedInMem->getNumRows(),
                     iusSampleInsertedInMem->getNumRows());
        LM->Log(LM->msg);

        cbf->setLogFile((char*)(LM->logFileName()->data()));
        logCBF("before forming S(i-1)", cbf);
     }



     cbf->setKenLengthInfo( computeKeyLengthInfo(smplGroup->ISdatatype) );

     if (LM->LogNeeded()) {
        sprintf(title, "IUS: insert into CBF " PF64 " keys for S(i-1)", sampleRowCount);
        LM->StartTimer(title);
     }
   
     // Insert into cbf the values from the column in the sample table
     Int64 numSmplRows = sampleRowCount;
     valIter.init(smplGroup);

     insertFailCount = 0;


     for (rowInx=1; rowInx<=numSmplRows - smplGroup->nullCount; rowInx++)
       {
         intervalIdx = findInterval(numNonNullIntervals, boundaryValues, valIter.val());
         if (intvlRC) intvlRC[intervalIdx]++;  // for logging
 
               

         CountingBloomFilter::INSERT_ENUM insert_status =
          cbf->insert((char*)valIter.dataRepPtr(), valIter.size(), intervalIdx,
                     (valIter.val() == MFVValues[intervalIdx]) ? cbf_key::MFV : cbf_key::NONE);


         // non-mfv value overflows to mfv. bail out.
         if (insert_status == CountingBloomFilter::NEW_MFV) {

#if 0
           if ( LM->LogNeeded() ) {
               sprintf(LM->msg, "rowIdx=%d, interval=%d, valSize=%d", rowInx, intervalIdx, valIter.size());
               LM->Log(LM->msg);

               memcpy(LM->msg, (char*)valIter.dataRepPtr(), valIter.size());
               LM->Log(LM->msg);
               logCBF("after the above key, cbf is:", cbf);
            }
#endif
  
            if (LM->LogNeeded()) {
              // only issue the warning if logging is turned on
              diagsArea << DgSqlCode(UERR_IUS_INSERT_NONMFV_OVERFLOW)
                << DgString0(smplGroup->colSet[0].colname->data());
              LM->Log("NONMFV overflow");
              LM->StopTimer();  // Need both of these; there are
              LM->StopTimer();  //   2 outstanding timer events
            }
            return UERR_IUS_INSERT_NONMFV_OVERFLOW;
         }

            
         // non-mfv value can not find a slot in CBF, record the failure and continue
         if (insert_status == CountingBloomFilter::NO_SLOT ||
             insert_status == CountingBloomFilter::PARAM_ERROR) 
           insertFailCount++;
 
#if 0
         if ( LM->LogNeeded() && intervalIdx  == 44 ) {
             sprintf(LM->msg, "key=%d, interval=%d", *(int*)valIter.dataRepPtr(), intervalIdx);
             LM->Log(LM->msg);
             logCBF("after the above key, cbf is:", cbf);
         }
#endif

         valIter.next();
   
       }


     if (LM->LogNeeded()) {
        logCBF("after s(i-1) insertion , cbf is:", cbf);
        LM->StopTimer();
        if (insertFailCount > 0) {
           sprintf(LM->msg, "For S(i-1), " PF64 " failures out of " PF64 " CBF insertions.", 
                   insertFailCount, numSmplRows - smplGroup->nullCount);
           LM->Log(LM->msg);
        }
     }

  } else { // cbf already exists
     // 1 more bucket than interval so that interval# maps to bucket# directly
     HS_ASSERT(cbf->numBuckets() == hist->getNumIntervals() + 1);
     cbf->setKenLengthInfo( computeKeyLengthInfo(smplGroup->ISdatatype) );
     sampleRowCount = cbf->totalFreqForAll();
  }

  //logCBF("after forming S(i-1)", cbf);
  

  if (LM->LogNeeded()) {
    LM->StopTimer();
  }


  Int64 numDelRows = 0;
  Int64 numInsRows = 0;

  //
  // Algorithm2:
  //
  // delete from cbf those values from the column in the deleted row inMem table

  numDelRows = iusSampleDeletedInMem->getNumRows();

  if (LM->LogNeeded()) {
     sprintf(title, "IUS: delete D from CBF (" PF64 " keys)", numDelRows);
     LM->StartTimer(title);
  }

  valIter.init(delGroup);
  for (rowInx=1; rowInx<=numDelRows - delGroup->nullCount; rowInx++)
    {


/*
      if ( LM->LogNeeded() && strcmp(smplGroup->colSet[0].colname->data(), "COLSINT") == 0 ) {

unsigned char* x = (unsigned char*)valIter.dataRepPtr();
if ( x[0] == (unsigned char)255 && x[1] == (unsigned char)127 ) {
         sprintf(LM->msg, "key=%d, interval=%d", *(short*)valIter.dataRepPtr(), intervalIdx);
         LM->Log(LM->msg);
}
      }
*/


      intervalIdx = findInterval(numNonNullIntervals, boundaryValues, valIter.val());
      if (intvlRC) intvlRC[intervalIdx]--;  // for logging
      cbf->remove((char*)valIter.dataRepPtr(), valIter.size(), intervalIdx, 
                  (valIter.val() == MFVValues[intervalIdx]) ? cbf_key::MFV : cbf_key::NONE);

#if 0
      if ( LM->LogNeeded() ) {
         sprintf(buf, "key=%d, interval=%d", *(int*)valIter.dataRepPtr(), intervalIdx);
         LM->Log(buf);
      }
#endif

      valIter.next();
    }

  if (LM->LogNeeded()) {
    LM->StopTimer();
  }

  //logCBF("after deleting D", cbf);

  // Insert into cbf the values from the column in the inserted row inMem table
  numInsRows = iusSampleInsertedInMem->getNumRows();

  if (LM->LogNeeded()) {
     sprintf(title, "IUS: insert I into CBF (" PF64 " keys)", numInsRows);
     LM->StartTimer(title);
  }

  // An object to keep track of any new lowest and highest values so we
  // can tweak interval boundaries if need be
  HSHiLowValues<T> hiLowVal;

  insertFailCount = 0;
  valIter.init(insGroup);
  for (rowInx=1; rowInx<=numInsRows - insGroup->nullCount; rowInx++)
    {
      intervalIdx = findInterval(numNonNullIntervals, boundaryValues, valIter.val());
      if (intvlRC) intvlRC[intervalIdx]++;  // for logging

      hiLowVal.findHiLowValues(valIter.val());

      CountingBloomFilter::INSERT_ENUM insert_status =
          cbf->insert((char*)valIter.dataRepPtr(), valIter.size(), intervalIdx, 
                     (valIter.val() == MFVValues[intervalIdx]) ? cbf_key::MFV : cbf_key::NONE);

      // non-mfv value overflows to mfv. bail out.
      if (insert_status == CountingBloomFilter::NEW_MFV) {
         if (LM->LogNeeded())
           {
             // only issue warning if logging is turned on
             diagsArea << DgSqlCode(UERR_IUS_INSERT_NONMFV_OVERFLOW)
                       << DgString0(smplGroup->colSet[0].colname->data());
           }
         LM->StopTimer();
         return UERR_IUS_INSERT_NONMFV_OVERFLOW;
      }

      // non-mfv value can not find a slot in CBF, record the failure and conintue
      if (insert_status == CountingBloomFilter::NO_SLOT ||
          insert_status == CountingBloomFilter::PARAM_ERROR)
         insertFailCount++;

#if 0
      if ( LM->LogNeeded() ) {
         sprintf(buf, "key=%d, interval=%d", *(int*)valIter.dataRepPtr(), intervalIdx);
         LM->Log(buf);
      }
#endif

      valIter.next();
    }
         
  smplGroup->allKeysInsertedIntoCBF = TRUE;

  if (LM->LogNeeded()) {
     LM->StopTimer();
     if (insertFailCount > 0) {
         sprintf(LM->msg, "For I, " PF64 " failures out of " PF64 " CBF insertions.", 
                 insertFailCount, numInsRows - insGroup->nullCount);
         LM->Log(LM->msg);
     }
     logCBF("after inserting I", cbf);
  }

  //
  // end of Algorithm 2
  //

  // 
  // Compute the new scale factor: the ratio of the RC of the table to
  // that of the final sample.
  // 
  double scaleFactor = 
       (double) actualRowCount / (sampleRowCount - numDelRows + numInsRows);


  if (LM->LogNeeded()) {
     sprintf(LM->msg, "actualRC=" PF64 " sampleRC=" PF64 " delRC= " PF64 ", InsRC=" PF64 ", scaleFactor=%f", 
               actualRowCount, sampleRowCount, numDelRows, numInsRows, scaleFactor);
     LM->Log(LM->msg);
  }

  // nullCount is the scaled version of the number of inserted nulls minus the number
  // of deleted ones; this value, which could be negative, will be added to the
  // row count for the original null interval.
  Int32 nullCount = (Int32)((insGroup->nullCount - delGroup->nullCount) * scaleFactor) ;
  retcode = estimateAndTestIUSStats(smplGroup, delGroup, insGroup,
                                    hist, cbf, numNonNullIntervals,
                                    scaleFactor, nullCount, intvlRC);

  // If the adjusted histogram is judged worthy by estimateAndTestIUSStats, we next
  // tweak the highest and lowest interval boundaries. If the sample data produced a
  // value higher than the highest interval, we extend that interval's boundary to
  // include it. Similarly on the low end. We don't attempt to shrink the intervals,
  // though, if sample values were deleted. Instead we count on reversion to RUS
  // if the rowcounts of those intervals get too small.
  if ((retcode == 0) && (hiLowVal.seenAtLeastOneValue_))
    {
      T convertedBoundaryValue[1];  // a target for convertBoundaryOrMFVValue
      
      // highest interval -- the highest boundary is stored in 
      // interval numNonNullIntervals
  
      // convert boundary to data type T
      const HSDataBuffer & hiBoundary = hist->getIntBoundary(numNonNullIntervals);
      convertBoundaryOrMFVValue(hiBoundary,
                                smplGroup,
                                0,
                                convertedBoundaryValue,  
                                format);
          
      if (hiLowVal.hiValue_ > convertedBoundaryValue[0])
        {
          // convert T back to what would be stored in the histogram
          HSDataBuffer newHiBoundary;
          Lng32 convertRC = setBufferValue(hiLowVal.hiValue_,
                                           smplGroup,
                                           newHiBoundary);
          hist->setIntBoundary(numNonNullIntervals,newHiBoundary);
        }

      // lowest interval -- the low boundary is stored in interval 0
      
      // convert boundary to data type T
      const HSDataBuffer & lowBoundary = hist->getIntBoundary(0);
      convertBoundaryOrMFVValue(lowBoundary,
                                smplGroup,
                                0,
                                convertedBoundaryValue,  
                                format);
          
      if (hiLowVal.lowValue_ < convertedBoundaryValue[0])
        {
          // convert T back to what would be stored in the histogram
          HSDataBuffer newLowBoundary;
          Lng32 convertRC = setBufferValue(hiLowVal.lowValue_,
                                           smplGroup,
                                           newLowBoundary);
          hist->setIntBoundary(0,newLowBoundary);
        }
    }

  if (intvlRC)
    NADELETEBASIC(intvlRC, STMTHEAP);
  return retcode;
}

Int32 HSGlobalsClass::logCBF(const char* title, CountingBloomFilter* cbf)
{
  char buf1[30];
  char buf2[30];

  HSLogMan *LM = HSLogMan::Instance();

  sprintf(LM->msg, "====================================");
  LM->Log(LM->msg);
  LM->Log(title);

  cbf->outputParams(LM->msg);
  LM->Log(LM->msg);

  for ( UInt32 b = 1; b<cbf->numBuckets(); b++ ) {
      UInt64 rc  = cbf->totalFreq(b);
      convertInt64ToAscii(rc, buf1);

      UInt64 uec = cbf->uec(b);
      convertInt64ToAscii(uec, buf2);

      sprintf(LM->msg, "%5d | %s | %s\n",  b, buf1, buf2);
      LM->Log(LM->msg);

     VarUIntArray& lowf2s = cbf->lowF2s(b);
     for (UInt32 i=1; i < lowf2s.entries(); i++)
      {
        if (lowf2s[i] >0) {
          sprintf(LM->msg, "\tf%d=%d\n",
                 i, lowf2s[i]);
          LM->Log(LM->msg);
        }
      }

  }

   cbf->computeOverflowF2s();
   UInt32 highF2s = cbf->getOverflowEntries();
   sprintf(LM->msg, "\nHigh freq area:\n");
   LM->Log(LM->msg);

   for ( CollIndex i = 0 ; i < highF2s; i++) {
     UInt64 freq;
     UInt32 bucket;
     UInt64 f2 = cbf->highF2(i, freq, bucket);

     if ( freq > 0 ) {
        sprintf(LM->msg, "In bucket %d, f%d=%d\n",
                      bucket, (UInt32)freq, (UInt32)f2);
        LM->Log(LM->msg);
     }
   }


  sprintf(LM->msg, "====================================");

  LM->Log(LM->msg);

  sprintf(LM->msg, "====================================");
  LM->Log(LM->msg);
  return 0;
}

double HSGlobalsClass::computeAvgCharLengthForIUS(HSColGroupStruct* smplGroup,
                                                  HSColGroupStruct* delGroup,
                                                  HSColGroupStruct* insGroup)
{
  Int64 delRows = iusSampleDeletedInMem->getNumRows();
  Int64 insRows = iusSampleInsertedInMem->getNumRows();

  // smplGroup->avgVarCharSize is the avg varchar size of the column represented
  // by smplGroup in the persistent sample, retrieved from the Histograms table
  // in selectIUSBatch(). The result of the current function replaces that value.
  Int64 oldSum = (Int64)(sampleRowCount * smplGroup->avgVarCharSize);
  Int64 newSum = (Int64)(oldSum - (delGroup->avgVarCharSize * delRows)
                         + (insGroup->avgVarCharSize * insRows));
  Int64 newRows = sampleRowCount - delRows + insRows;
  return (double)newSum / (double)newRows;
}

Int32 HSGlobalsClass::estimateAndTestIUSStats(HSColGroupStruct* group,
                                              HSColGroupStruct* delGroup,
                                              HSColGroupStruct* insGroup,
                                              HSHistogram* hist,
                                              CountingBloomFilter* cbf,
                                              Lng32 numNonNullIntervals,
                                              double scaleFactor,
                                              Int32 nullCount,
                                              Int64* intvlRC)
{
  Int32 retcode = 0;


  ///////////////////////////////////////////////////
  // fetch uec and rowcount per interval from cbf. 
  ///////////////////////////////////////////////////
  UInt64* sampledIntvlRCs = new(STMTHEAP) UInt64[cbf->numBuckets()];
  UInt64* sampledIntvlUECs = new(STMTHEAP) UInt64[cbf->numBuckets()];


  HSLogMan *LM = HSLogMan::Instance();

  if (LM->LogNeeded()) {
    sprintf(LM->msg, "IUS: estimateAndTestIUSStats() for column %s",
                     group->colSet[0].colname->data());
    LM->StartTimer(LM->msg);
    hist->logAll("The existing histogram is:");
    sprintf(LM->msg, "Total #intervals=%d, scaleFactor=%f,nullCount=%d", 
                      numNonNullIntervals, scaleFactor, nullCount);
    LM->Log(LM->msg);

  }

  // low frequency area first, skip the lowest special interval
  for ( UInt32 b = 1; b<cbf->numBuckets(); b++ ) {

      // RC and UEC stats collected for each interval (bucket)
      sampledIntvlRCs[b] = cbf->totalFreq(b);
      sampledIntvlUECs[b] = cbf->uec(b);
  }



  ///////////////////////////////////////////////
  // estimate the uec per interval
  ///////////////////////////////////////////////
  //UInt64* estIntvlUECs = new(STMTHEAP) UInt64[numIntervals+1];
  double DshMax = CmpCommon::getDefaultNumeric(USTAT_DSHMAX);
  double coeffOfVar;  
  double Duj;
  double Dsh;

  double totalAvgRC = 0;

  UInt64 totalUEC = 0;
  UInt64 totalRC = 0; 

  
  // Populate fi for high frequency data
  cbf->computeOverflowF2s();
               
  if (LM->LogNeeded()) {
     sprintf(LM->msg, "#high freq entries in cbf=%d", cbf->getOverflowEntries());
     LM->Log(LM->msg);
  }

  HS_ASSERT(cbf->canHandleArbitrarySkewedValue() == FALSE); 

  // save the total RC/UEC before during the processing of each of the interval
  // below, the interval RC and UEC will be updated.
  Int64 origTotalRC= 0;
  Int64 origTotalUEC = 0;
  hist->getTotalCounts(origTotalRC, origTotalUEC);
  
  double rcIntChangeThreshold =
    CmpCommon::getDefaultNumeric(USTAT_IUS_INTERVAL_ROWCOUNT_CHANGE_THRESHOLD);

  double uecIntChangeThreshold =
    CmpCommon::getDefaultNumeric(USTAT_IUS_INTERVAL_UEC_CHANGE_THRESHOLD);


  // skip the 0th interval, and go over all not-null intervals. idx is the
  // bucket number for CBF. Buckets are 0-index in CBF.
  //
  // If a shape test fails for an interval, we exit the following loop
  // immediately and return a result that causes the caller to bail out of IUS,
  // unless logging is enabled, in which case all intervals are processed
  // regardless of shape test failures. This allows us to view all problems in
  // the distribution of data after IUS when looking at the log. In this case,
  // the original shape test failure is remembered and used as the basis of
  // the return result and the information entered into the diagnostics area.
  USTAT_ERROR_CODES shapeTestError = UERR_NO_ERROR;
  char shapeFlags[10];
  for ( Lng32 idx = 1; idx<=numNonNullIntervals; idx++ ) {

      // This is an annotation for each interval in log file indicating which,
      // if any, shape tests failed for that interval.
      shapeFlags[0] = '\0';

      // get fi for i=1,2,...
      FrequencyCounts fi;


      // Populate fi for low frequent values first
      for ( UInt32 j = 1; j<cbf->lowF2s(idx).entries(); j++ ) {
         UInt32 f2 = cbf->lowF2s(idx)[j]; // fi=f2

         if ( f2 > 0 ) {

           if (LM->LogNeeded() && getenv("lf") ) {
               sprintf(LM->msg, "   low freq: f%d=%d", j, f2);
               LM->Log(LM->msg);
           }

           fi.increment((Int64)j, (ULng32)f2);

         }
      }

      UInt64 i = 0;
      UInt32 b = 0;

      // The trick to use 2*idx+k as the iterators is only possible with  
      // CountingBloomFilterWithKnownSkews. The assertion at the beginning of
      // this method assures this.

      for ( UInt32 k=0; k<=1; k++ ) {

        UInt64 f2 = cbf->highF2(2*idx+k, i, b);  
                                           // f2, i and b are fi, index (i in fi) and
                                           // the bucket# of the kth entry
                                           // in overflow area, respectively.
                                           // b === (2*idx+k)/2 === idx
  
  
        if ( i > 0 && f2 > 0 ) {
           sampledIntvlUECs[b]++;
           sampledIntvlRCs[b] += i;
  
           if (LM->LogNeeded() && getenv("hf") ) {
             sprintf(LM->msg, "   high freq: f%d=%d", (UInt32)i, (UInt32)f2);
             LM->Log(LM->msg);
           }
  
           fi.increment((Int64)i, (ULng32)f2);  // @WARN: Narrowing cast of 2nd argument
  

           if ( k == 0 )
              hist->setIntMFVRowCount(b, i);
           else
              hist->setIntMFV2RowCount(b, i);
        }

      }

      // Verify interval's row count accumulated by CBF.
      if (intvlRC && LM->LogNeeded())
        {
          if (intvlRC[idx] != sampledIntvlRCs[idx])
            {
              sprintf(LM->msg,
                      "*** Row count mismatch for interval %d: CBF=" PF64 ", count=" PF64 " ***",
                      idx, sampledIntvlRCs[idx], intvlRC[idx]);
              LM->Log(LM->msg);
            }
        } 


      double oldRC = (double)hist->getIntRowCount(idx);
      double newEstRC = (double)(sampledIntvlRCs[idx] * scaleFactor);

      if ( 0 == sampledIntvlRCs[idx]) {
         if(shapeTestError == UERR_NO_ERROR)
            shapeTestError = UERR_WARNING_IUS_EMPTY_INTERVAL;
         if (LM->LogNeeded())
            strcat(shapeFlags, "e");
         else
           break;  // exit loop and return result
      }

      double estIntvlUEC ;

      // If sampled RC and UEC are the same, we set the newEstRC to the scaled up RC.
      if ( sampledIntvlUECs[idx] >= sampledIntvlRCs[idx] ) 
         estIntvlUEC = newEstRC;
      else 
         estIntvlUEC = lwcUecEstimate(
                                (double)sampledIntvlUECs[idx],// sampleUEC
                                (double)sampledIntvlRCs[idx], // sampleRowCnt
                                newEstRC,   // est total RC in the interval
                                &fi,        // fi
                                DshMax,     // input
                                coeffOfVar, // output
                                Duj,        // output
                                Dsh         // output
                                );


      double oldUec = (double)hist->getIntUec(idx);

      // Use the oldUec if estimatedUec is nan.  This is to work around 
      // the nan value produced by lwcUecEstimate() above.
     
#if __GNUC_MINOR__ == 8
      if ( std::isnan(estIntvlUEC) )
#else
      if ( isnan(estIntvlUEC) )
#endif
        estIntvlUEC = oldUec;

      // cap the new UEC with the RC
      if ( estIntvlUEC > newEstRC )
         estIntvlUEC = newEstRC;


      totalRC += (UInt64)newEstRC;
      totalUEC += (UInt64)estIntvlUEC;


      if ( estIntvlUEC== 0 ) {
         if(shapeTestError == UERR_NO_ERROR)
            shapeTestError = UERR_WARNING_IUS_ZERO_UEC_INTERVAL;
         if (LM->LogNeeded())
            strcat(shapeFlags, "z");
         else
            break;  // exit loop and return result
      }

      if ( oldUec > 2 ) {

        // Run the interval based shape test right here.
  
        //
        // Test that the row-count changes 
        //
        if ( (newEstRC>oldRC) && 
             delta(oldRC, newEstRC)/oldRC > rcIntChangeThreshold ) {
           if(shapeTestError == UERR_NO_ERROR)
              shapeTestError = UERR_WARNING_IUS_TOO_MUCH_RC_CHANGE_INTERVAL;
           if (LM->LogNeeded())
              strcat(shapeFlags, "r");
           else
              break;  // exit loop and return result
        }
  
  
        //
        // Test UEC changes 
        //
  
        if ( (estIntvlUEC > oldUec) &&
             delta(oldUec, estIntvlUEC)/oldUec > uecIntChangeThreshold ) {
           if(shapeTestError == UERR_NO_ERROR)
              shapeTestError = UERR_WARNING_IUS_TOO_MUCH_UEC_CHANGE_INTERVAL;
           if (LM->LogNeeded())
              strcat(shapeFlags, "u");
           else
              break;  // exit loop and return result
        }
      }

      if (LM->LogNeeded()) {
         sprintf(LM->msg, 
           "%-5sAt intv[%d], RC(old, new)= (%f, %f); UEC(old, new)=(%f,%f), (coeffOfVar, Duj,Dsh)=(%f, %f,%f)",
           shapeFlags, idx,
           oldRC, newEstRC, 
           oldUec, estIntvlUEC, 
           coeffOfVar, Duj, Dsh );
         LM->Log(LM->msg);
      }

      // save the new RC and UEC for the interval
      hist->setIntRowCount(idx, (Int64)newEstRC);
      hist->setIntUec(idx, (Int64)estIntvlUEC);
  } // for each nonnull interval


  if (shapeTestError != UERR_NO_ERROR) {
     if (LM->LogNeeded()) {
        sprintf(LM->msg, "IUS could not be used for column %s due to failure of "
                          "one or more shape tests as indicated by symbols in "
                          "leftmost column in table above:\n"
                          "  e = empty interval\n"
                          "  z = zero UEC interval\n"
                          "  r = too much RC change\n"
                          "  u = too much UEC change",
                group->colSet[0].colname->data());
        LM->Log(LM->msg);
        // only issue the warning diagnostic if logging is on
        diagsArea << DgSqlCode(shapeTestError)
                  << DgString0(group->colSet[0].colname->data());
        }
     LM->StopTimer();
     return shapeTestError;
     }

  if (LM->LogNeeded()) {
     sprintf(LM->msg, "IUS upgraded histogram for column %s is",
                 group->colSet[0].colname->data());
     group->groupHist->logAll(LM->msg);
  }


  //
  // Handle NULLs 
  //
  if ( ! hist->hasNullInterval() && nullCount > 0 ) {
       hist->addNullInterval(nullCount, group->colCount);
       totalRC += nullCount;
       totalUEC++;
  } else if (hist->hasNullInterval()) {
	 // Add number of nulls inserted minus the number deleted.
	 // nullCount does not include original #nulls in sample.
     hist->addIntRowCount(hist->getNumIntervals(), nullCount);
     totalRC += hist->getIntRowCount(hist->getNumIntervals());
  }

  //
  // Run the total RC and UEC shape test.
  //

  //
  // Test the absolute total row-count percentage change.
  //
  double rcTotalChangeThreshold = 
    CmpCommon::getDefaultNumeric(USTAT_IUS_TOTAL_ROWCOUNT_CHANGE_THRESHOLD);


  if ( (totalRC > (UInt64)origTotalRC) &&
      delta((UInt64)origTotalRC, totalRC)/origTotalRC > rcTotalChangeThreshold )  {
     if (LM->LogNeeded()) 
       {
         // only do the warning diagnostic if logging is enabled
         diagsArea << DgSqlCode(UERR_WARNING_IUS_TOO_MUCH_RC_CHANGE_TOTAL)
                   << DgString0(group->colSet[0].colname->data());
       }
     LM->StopTimer();
     return UERR_WARNING_IUS_TOO_MUCH_RC_CHANGE_TOTAL;
  }

  //
  // Test the absolute total UEC percentage change.
  //
  double uecTotalChangeThreshold = 
    CmpCommon::getDefaultNumeric(USTAT_IUS_TOTAL_UEC_CHANGE_THRESHOLD);

  if ((totalUEC > (UInt64)origTotalUEC) && 
      delta((UInt64)origTotalUEC, totalUEC)/origTotalUEC > uecTotalChangeThreshold ) {
     if (LM->LogNeeded()) 
       {
         // only do the warning diagnostic if logging is enabled
         diagsArea << DgSqlCode(UERR_WARNING_IUS_TOO_MUCH_UEC_CHANGE_TOTAL)
                   << DgString0(group->colSet[0].colname->data());
       }
     LM->StopTimer();
     return UERR_WARNING_IUS_TOO_MUCH_UEC_CHANGE_TOTAL;
  }



  // 
  // Handle avg varchar
  // 
  if (group->computeAvgVarCharSize())
    group->avgVarCharSize = computeAvgCharLengthForIUS(group, delGroup, insGroup);
  else
    group->avgVarCharSize = -1;

 
  // 
  //  Handle std. deviation of frequencies per interval
  //  by computing the sum of frequencies squared per interval.
  //  The actual std deviation of frequencies will be computed
  //  using the sum in method HSGlobalsClass::WriteStatistics().
  //
  double* sumSq = new (STMTHEAP) double[numNonNullIntervals+1];
  cbf->computeSumOfFrequencySquared(sumSq, numNonNullIntervals+1);

  // Copy out the sums to hist. 
  for (Int32 i=1; i<=hist->getNumIntervals(); i++) {
     hist->setIntSquareSum(i, sumSq[i]);
  }

  NADELETEBASIC(sumSq, STMTHEAP);
  

  if (LM->LogNeeded()) {
     sprintf(LM->msg, "IUS: computed histogram for column %s is",
                  group->colSet[0].colname->data());
     hist->logAll(LM->msg);
     LM->StopTimer();
  }


  return retcode;
}


   
// For each group in PENDING state, we need to run RUS. Here we
// merge the data from set S(i-1), D and I into one data stream
// before applying the internal sort .
Lng32 HSGlobalsClass::mergeDatasetsForIUS()
{
   Int32 retcode = 0;

   HSColGroupStruct* group = singleGroup;
   HSColGroupStruct* delgroup = iusSampleDeletedInMem->getColumns();
   Int64 delrows = iusSampleDeletedInMem->getNumRows();
   HSColGroupStruct* insgroup = iusSampleInsertedInMem->getColumns();
   Int64 insrows = iusSampleInsertedInMem->getNumRows();
   while (group) {
     if (group->state==PENDING) {
        retcode = mergeDatasetsForIUS(group, sampleRowCount, delgroup, delrows, insgroup, insrows);
        HSHandleError(retcode);
     }
     group = group->next;
     insgroup = insgroup->next;
     delgroup = delgroup->next;
   }
   return retcode;
}

Lng32 HSGlobalsClass::mergeDatasetsForIUS(
                       HSColGroupStruct* smplGroup, Int64 smplrows,
                       HSColGroupStruct* delGroup, Int64 delrows,
                       HSColGroupStruct* insGroup, Int64 insrows)
{
  Lng32 retcode = -1;
  Lng32 datatype = smplGroup->ISdatatype;

  // Only need to handle types used for IS/IUS. Datetime/interval types and
  // non-integral fixed numerics are all converted to one of these types.
  // Two template parameters are used by the templated overload of
  // mergeDatasetsForIUS(), because in the cases of char and varchar, it uses
  // both IS{Var|Fixed}Char and IUS{Fixed|Var}Char. For all other types, the
  // same type is used for both template parameters.
  switch (datatype)
    {
      case REC_BIN8_SIGNED:
        return mergeDatasetsForIUS((Int8*)smplGroup->data, (Int8*)NULL,
                                   smplGroup, smplrows, delGroup, delrows, insGroup, insrows);
        break;
      case REC_BOOLEAN:
      case REC_BIN8_UNSIGNED:
        return mergeDatasetsForIUS((UInt8*)smplGroup->data, (UInt8*)NULL,
                                   smplGroup, smplrows, delGroup, delrows, insGroup, insrows);
        break;
      case REC_BIN16_SIGNED:
        return mergeDatasetsForIUS((Int16*)smplGroup->data, (Int16*)NULL,
                                   smplGroup, smplrows, delGroup, delrows, insGroup, insrows);
        break;
      case REC_BPINT_UNSIGNED:
      case REC_BIN16_UNSIGNED:
        return mergeDatasetsForIUS((UInt16*)smplGroup->data, (UInt16*)NULL,
                                   smplGroup, smplrows, delGroup, delrows, insGroup, insrows);
        break;
      case REC_BIN32_SIGNED:
        return mergeDatasetsForIUS((Int32*)smplGroup->data, (Int32*)NULL,
                                   smplGroup, smplrows, delGroup, delrows, insGroup, insrows);
        break;
      case REC_BIN32_UNSIGNED:
        return mergeDatasetsForIUS((UInt32*)smplGroup->data, (UInt32*)NULL,
                                   smplGroup, smplrows, delGroup, delrows, insGroup, insrows);
        break;
      case REC_BIN64_SIGNED:
        return mergeDatasetsForIUS((Int64*)smplGroup->data, (Int64*)NULL,
                                   smplGroup, smplrows, delGroup, delrows, insGroup, insrows);
        break;
      case REC_BIN64_UNSIGNED:
        return mergeDatasetsForIUS((UInt64*)smplGroup->data, (UInt64*)NULL,
                                   smplGroup, smplrows, delGroup, delrows, insGroup, insrows);
        break;
      case REC_FLOAT32:
        return mergeDatasetsForIUS((Float32*)smplGroup->data, (Float32*)NULL,
                                   smplGroup, smplrows, delGroup, delrows, insGroup, insrows);
        break;
      case REC_FLOAT64:
        return mergeDatasetsForIUS((Float64*)smplGroup->data, (Float64*)NULL,
                                   smplGroup, smplrows, delGroup, delrows, insGroup, insrows);
        break;
      case REC_BYTE_F_ASCII:
      case REC_BYTE_F_DOUBLE:
      case REC_BINARY_STRING:
        {
          // Create an object to be used with the value iterator; does not own its content.
          IUSFixedChar fixedChar(FALSE);
          IUSFixedChar::setLength(smplGroup->ISlength);
          IUSFixedChar::setCaseInsensitive(smplGroup->colSet[0].caseInsensitive == 1);
          IUSFixedChar::setColCollation(smplGroup->colSet[0].colCollation);
          IUSFixedChar::setCharSet(smplGroup->colSet[0].charset);
          return mergeDatasetsForIUS(&fixedChar, (ISFixedChar*)NULL,
                                     smplGroup, smplrows, delGroup, delrows, insGroup, insrows);
        }
        break;
      case REC_BYTE_V_ASCII:
      case REC_BYTE_V_DOUBLE:
      case REC_VARBINARY_STRING:
        {
          // Create an object to be used with the value iterator; does not own its content.
          IUSVarChar varChar(FALSE);
          IUSVarChar::setDeclaredLength(smplGroup->ISlength);
          IUSVarChar::setCaseInsensitive(smplGroup->colSet[0].caseInsensitive == 1);
          IUSVarChar::setColCollation(smplGroup->colSet[0].colCollation);
          IUSVarChar::setCharSet(smplGroup->colSet[0].charset);
          return mergeDatasetsForIUS(&varChar, (ISVarChar*)NULL,
                                     smplGroup, smplrows, delGroup, delrows, insGroup, insrows);
        }
        break;

      default:
        retcode = -1;
        HSHandleError(retcode);
    } // switch

  return retcode;
}

// For char/varchar, two types are used, hence the two type parameters to this
// template. The IUS subclasses of IS{Fixed|Var}Char are needed for the iterator
// and for the arrays of boundary and MFV values, while the lighter weight
// IS parent classes are used as the values in the data array.  For all other
// types, the same type is used for both template parameters.
template <class T_IUS, class T_IS>
Int32 HSGlobalsClass::mergeDatasetsForIUS(T_IUS* ptr, T_IS* dummyPtr,
                       HSColGroupStruct* smplGroup, Int64 smplrows,
                       HSColGroupStruct* delGroup, Int64 delrows,
                       HSColGroupStruct* insGroup, Int64 insrows)
{
  HSLogMan *LM = HSLogMan::Instance();
  HSHistogram* hist = smplGroup->groupHist;
  Lng32 numIntervals = hist->getNumIntervals();
  Lng32 numNonNullIntervals = hist->hasNullInterval()
                                    ? numIntervals - 1
                                    : numIntervals;


  // save the address of the source data
  void* source = smplGroup->data;

  // Kludge. allocate the memory at smplGroup->data and grab it as buffer.
  if ( smplGroup->allocateISMemory(smplrows + insrows, FALSE, TRUE) == FALSE )
     return -1;
  
  // the target buffer
  T_IS* buffer = (T_IS*)(smplGroup->data);

  // restore the source data
  smplGroup->data = source;

  T_IUS* boundaryValues = (T_IUS*)(smplGroup->boundaryValues);
  T_IUS* MFVValues = (T_IUS*)(smplGroup->MFVValues);


  // first work on smplGroup
  Lng32 ct = 0;

  Int32 intervalIdx;

  IUSValueIterator<T_IUS> valIter(ptr);

  // If there is only a null interval in the existing histogram, calculate the
  // null count for the updated histogram, and set up buffer and ct using I alone.
  //
  if (numNonNullIntervals == 0) {
     smplGroup->nullCount -= delGroup->nullCount;
     smplGroup->nullCount += insGroup->nullCount;
     buffer = (T_IS*)(insGroup->data);
     ct = insrows - insGroup->nullCount;

  } else if ( smplGroup->allKeysInsertedIntoCBF == FALSE ) {

     // Since we have not processed all keys from I, we have to clear the CBF
     // and reload it with (S(i-1) - D), and insert I into the buffer directly.
     smplGroup->cbf->clear();

     // Insert S(i-1)

     valIter.init(smplGroup);
     for (Lng32 i=0; i<smplrows; i++ ) {

         intervalIdx = findInterval(numNonNullIntervals, boundaryValues, valIter.val());

         CountingBloomFilter::INSERT_ENUM insert_status =
            smplGroup->cbf->insert((char*)valIter.dataRepPtr(), valIter.size(), intervalIdx,
                     (valIter.val() == MFVValues[intervalIdx]) ? cbf_key::MFV : cbf_key::NONE);


         valIter.next();
     }

     if (LM->LogNeeded()) {
       smplGroup->cbf->setLogFile((char*)(LM->logFileName()->data()));
       logCBF("MergeDatasets: after insert S(i-1)", smplGroup->cbf);
     }

     // Delete from D
     valIter.init(delGroup);
     for (Lng32 i=0; i<delrows; i++ ) {

         intervalIdx = findInterval(numNonNullIntervals, boundaryValues, valIter.val());

         smplGroup->cbf->remove((char*)valIter.dataRepPtr(), valIter.size(), intervalIdx,
                     (valIter.val() == MFVValues[intervalIdx]) ? cbf_key::MFV : cbf_key::NONE);

         valIter.next();
     }
        
     if (LM->LogNeeded()) {
       smplGroup->cbf->setLogFile((char*)(LM->logFileName()->data()));
       logCBF("MergeDatasets: after delete D", smplGroup->cbf);
     }

     // Now lookup the reconstructed CBF and insert (S(i-1) - D) into the buffer 
     valIter.init(smplGroup);
     for (Lng32 i=0; i<smplrows; i++ ) {

         intervalIdx = findInterval(numNonNullIntervals, boundaryValues, valIter.val());

         // if the reference counter does not reach zero (cbf::remove() return true),
         // we need to save a copy of the data to the buffer. Otherwise, the data is
         // already deleted from CBF and should not be saved in the buffer.
         if ( smplGroup->cbf->remove((char*)valIter.dataRepPtr(), valIter.size(), intervalIdx,
                (valIter.val() == MFVValues[intervalIdx]) ? cbf_key::MFV : cbf_key::NONE) )
         {
            buffer[ct++] = valIter.val();
         }

         valIter.next();
     }

     if (LM->LogNeeded()) {
       smplGroup->cbf->setLogFile((char*)(LM->logFileName()->data()));
       logCBF("MergeDatasets: after check cbf and insert S(i-1)-D", smplGroup->cbf);
     }

     // Finally insert all keys from I into the buffer
     valIter.init(insGroup);
     for (Lng32 i=0; i<insrows; i++ ) {
         buffer[ct++] = valIter.val();
         valIter.next();
     }

  } else {

    // All keys inserted case (i.e., IUS fails because of shape test failures)

     valIter.init(smplGroup);
   
     for (Lng32 i=0; i<smplrows; i++ ) {
   
         intervalIdx = findInterval(numNonNullIntervals, boundaryValues, valIter.val());
   
         // if the reference counter does not reach zero (cbf::remove() return true),
         // we need to save a copy of the data to the buffer. Otherwise, the data is 
         // already deleted from CBF and should not be saved in the buffer.
         if ( smplGroup->cbf->remove((char*)valIter.dataRepPtr(), valIter.size(), intervalIdx,
                (valIter.val() == MFVValues[intervalIdx]) ? cbf_key::MFV : cbf_key::NONE) )
         {
            buffer[ct++] = valIter.val();
         }
   
         valIter.next();
     }
   
     // Now work on insGroup
     valIter.init(insGroup);
   
     for (Lng32 i=0; i<insrows; i++ ) {
   
         intervalIdx = findInterval(numNonNullIntervals, boundaryValues, valIter.val());
   
         if ( smplGroup->cbf->remove((char*)valIter.dataRepPtr(), valIter.size(), intervalIdx,
                (valIter.val() == MFVValues[intervalIdx]) ? cbf_key::MFV : cbf_key::NONE) )
         {
            buffer[ct++] = valIter.val();
         }
   
         valIter.next();
     }
  }


  // We can delete the old memory now. Pass FALSE so strData is preserved. The
  // string content is still referenced by the new objects in data.
  smplGroup->freeISMemory(FALSE);

  // Set the new buffer in place
  smplGroup->data = buffer;
  smplGroup->nextData = buffer + ct;

  // As a safeguard, mark the group as not having its string content (only
  // applicable for char types) laid out consecutively in the strData
  // buffer. This invalidates it for future use with an IUSValueIterator.
  smplGroup->strDataConsecutive = FALSE;

  // Done with these; delete them within this template function while we have
  // the type as a parameter.
  delete [] (T_IUS*)smplGroup->boundaryValues;
  delete [] (T_IUS*)smplGroup->MFVValues;
  smplGroup->boundaryValues = NULL;
  smplGroup->MFVValues = NULL;

  return 0;
}

// Format an integral value (which is a scaled value with implied decimal point)
// as a fixed numeric value.
void formatFixedNumeric(Int64 value, Lng32 scale, char* buffer)
{
  char digits[] = "0123456789";
  char temp;
  char *p1 = buffer, *p2 = buffer;
  Int64 xval = (Int64)(value >=0 ? value : -value);  // no template for abs()
  Int32 numDigits = 0;

  // Write the digits out in reverse order, adding the decimal point at the
  // appropriate location.
  do
    {
      *p2++ = digits[xval % 10];
      if (++numDigits == scale)
        *p2++ = '.';
      xval /= 10;
    }
  while (xval > 0 || numDigits < scale);

  // Add sign if negative, then terminating null.
  if (value < 0)
    *p2++ = '-';
  *p2 = '\0';

  // Reverse the string: point to first and last chars, swap and move pointers
  // towards each other until they meet.
  p2--;
  while (p1 < p2)
    {
      temp = *p1;
      *p1++ = *p2;
      *p2-- = temp;
    }
}


// Called by setBufferValue() to format an interval boundary value for a
// 64-bit float column.
//
Int32 copyValue(double &value, char *valueBuff, const HSColumnStruct &colDesc, short *len)
{
  Int32 retcode = 0;  // status is good unless column does not have expected type
  char *ptr;

  if (colDesc.datatype == REC_IEEE_FLOAT64)
    {
      retcode = convFloat64ToAscii(valueBuff, SQL_DOUBLE_PRECISION_DISPLAY_SIZE, value,
                                   SQL_DOUBLE_PRECISION_FRAG_DIGITS, NULL, 0, false);
      ptr = valueBuff + SQL_DOUBLE_PRECISION_DISPLAY_SIZE - 1;
      while (*ptr == ' ')
        ptr--;
      *(ptr+1) = '\0';
    }
  else
    retcode = -1;
  return retcode;
}

// Called by setBufferValue() to format an interval boundary value for a
// 32-bit float column.
//
Int32 copyValue(float &value, char *valueBuff, const HSColumnStruct &colDesc, short *len)
{
  Int32 retcode = 0;  // status is good unless no case for type
  char *ptr;

  if (colDesc.datatype == REC_IEEE_FLOAT32)
    {
      retcode = convFloat64ToAscii(valueBuff, SQL_REAL_DISPLAY_SIZE, value,
                                   SQL_REAL_FRAG_DIGITS, NULL, 0, false);
      ptr = valueBuff + SQL_REAL_DISPLAY_SIZE - 1;
      while (*ptr == ' ')
        ptr--;
      *(ptr+1) = '\0';
    }
  else
    retcode = -1;

  return retcode;
}

// Called by setBufferValue() to format an interval boundary value for a
// char column.
//
Int32 copyValue(ISFixedChar &value, char *valueBuff, const HSColumnStruct &colDesc, short *len)
{
  char *ptr;
  NAWchar *wptr;
  Int32 retcode = 0;  // status is good unless no case for type
  const Lng32 maxCharBoundaryLen = 
    (Lng32) CmpCommon::getDefaultNumeric(USTAT_MAX_CHAR_BOUNDARY_LEN);

  switch (colDesc.datatype)
    {
        case REC_BYTE_F_ASCII:
        case REC_BINARY_STRING:
          *len = (short)MINOF(colDesc.length, maxCharBoundaryLen);
          memmove(valueBuff,
                 ((ISFixedChar*)((void*)&value))->getContent(), // make it work with template
                 *len);
          ptr = valueBuff + *len - 1;

          // Trim trailing spaces. This is done after possibly truncating the
          // string to maxCharBoundaryLen characters, which can result in
          // removing spaces that are embedded rather than trailing spaces in
          // the original (full) string. This is done to produce the same results
          // as the old (non-internal sort) code.
          while (*ptr == ' ' && ptr >= valueBuff)
            {
              ptr--;
              (*len)--;
            }
          break;

        case REC_BYTE_F_DOUBLE:
          *len = (short)MINOF(colDesc.length, maxCharBoundaryLen*2); // in bytes
          memmove(valueBuff,
                 ((ISFixedChar*)((void*)&value))->getContent(),
                 *len);
          wptr = (NAWchar*)valueBuff + (*len / 2) - 1;
          while (*wptr == L' ' && wptr >= (NAWchar*)valueBuff)  // trim trailing spaces
            {
              wptr--;
              *len -= 2;
            }
          break;

        default:
          retcode = -1;
          break;
    }

  return retcode;
}

// Called by setBufferValue() to format an interval boundary value for a
// varchar column.
//
Int32 copyValue(ISVarChar &value, char *valueBuff, const HSColumnStruct &colDesc, short *len)
{
  char *ptr;
  NAWchar *wptr;
  Int32 retcode = 0;  // status is good unless no case for type
  const Lng32 maxCharBoundaryLen = 
    (Lng32) CmpCommon::getDefaultNumeric(USTAT_MAX_CHAR_BOUNDARY_LEN);

  switch (colDesc.datatype)
    {
        case REC_BYTE_V_ASCII:
        case REC_VARBINARY_STRING:
          ptr = ((ISVarChar*)((void*)&value))->getContent(); // make it work with template
          *len = (short)MINOF(*(short*)ptr, maxCharBoundaryLen);
          memmove(valueBuff, ptr+sizeof(short), *len);
          ptr = valueBuff + *len - 1;
          while (*ptr == ' ' && ptr >= valueBuff)        // trim trailing spaces
            {
              ptr--;
              (*len)--;
            }
          break;

        case REC_BYTE_V_DOUBLE:
          ptr = ((ISVarChar*)((void*)&value))->getContent(); // make it work with template
          *len = (short)MINOF(*(short*)ptr, maxCharBoundaryLen*2); // in bytes
          memmove(valueBuff, ptr+sizeof(short), *len);
          wptr = (NAWchar*)valueBuff + (*len / 2) - 1;
          while (*wptr == L' ' && wptr >= (NAWchar*)valueBuff)        // trim trailing spaces
            {
              wptr--;
              *len -= 2;
            }
          break;

        default:
          retcode = -1;
          break;
    }

  return retcode;
}

// Called by setBufferValue() to format an interval boundary value for an
// integral type, or any type mapped to an integer for internal sort.
//
Int32 copyValue(Int64 value, char *valueBuff, const HSColumnStruct &colDesc, short *len)
{
    Int32 scaleFactor;  // 10**scale for fractional seconds
    Int32 fracSecPart;  // fractional part of second, scaled to integer
    Int64 intSecPart; // integral part of number of seconds
    char *ptr = NULL;
    Int32 retcode = 0;  // status is good unless no case for type

    if ((colDesc.datatype >= REC_MIN_BINARY_NUMERIC &&
        colDesc.datatype <= REC_MAX_BINARY_NUMERIC)||
        colDesc.datatype == REC_DECIMAL_LSE ||
        colDesc.datatype == REC_DECIMAL_UNSIGNED ||
        colDesc.datatype == REC_DECIMAL_LS)
      {
        // Faster than sprintf, works for scale>0, no platform-dependent
        // format specifier for Int64.
        formatFixedNumeric(value, colDesc.scale, valueBuff);
      }
    else 
      {
       // The pre-handling of negative interval values is the same for all interval
       // types, so we do it once here before entering the switch statement.
       // FormatRow requires negative interval values to be in a particular
       // or an invalid boundary value will be produced.
       if (colDesc.datatype >= REC_MIN_INTERVAL && colDesc.datatype <= REC_MAX_INTERVAL)
         {
           ptr = valueBuff;
           if (value < 0)
             {
               // FormatRow requires minus sign before right-justified interval
               // value for negative interval.
               *ptr++ = '-';

               // No need to worry about overflow of 2's-complement min negative
               // value; int type chosen depends on the decimal leading field
               // precision, so there is plenty of wiggle room.
               value = -value;
             }
         }
       switch (colDesc.datatype)
       {
        case REC_DATETIME:
          switch (colDesc.precision)
            {
              case REC_DTCODE_DATE:
                {
                  short year;
                  char month, day;
                  ExpDatetime::getYearMonthDay(value, year, month, day);
                  sprintf(valueBuff, "%04d-%02d-%02d", year, month, day);
                }
                break;

              case REC_DTCODE_TIME:
                if (colDesc.scale > 0)
                  {
                    Int32 seconds = (Int32)(value / (Int32)pow(10, colDesc.scale));
                    sprintf(valueBuff, "%02d:%02d:%02d.%0*d",
                            seconds / 3600, (seconds % 3600) / 60, seconds % 60,
                            colDesc.scale, (Int32)(value % (Int32)pow(10, colDesc.scale)));
                  }
                else
                  // value must be in int range; cast it so don't have to worry about
                  // platform-specific int64 format specifier.
                  sprintf(valueBuff, "%02d:%02d:%02d",
                          (Int32)value / 3600, ((Int32)value % 3600) / 60, (Int32)value % 60);
                break;

              case REC_DTCODE_TIMESTAMP:
                {
                 short dtvals[8];
                 INTERPRETTIMESTAMP(value, dtvals);
                 if (colDesc.scale > 0)
                   sprintf(valueBuff, "%04d-%02d-%02d %02d:%02d:%02d.%0*d",
                           dtvals[0],  // year
                           dtvals[1],  // month
                           dtvals[2],  // day
                           dtvals[3],  // hour
                           dtvals[4],  // minute
                           dtvals[5],  // seconds
                           colDesc.scale, // display width for fractional seconds
                           // Fractional second; compute microseconds, remove trailing
                           // zeroes beyond the scale.
                           (dtvals[6] * 1000 + dtvals[7]) / 
                           (Int32)pow(10, 6 - MINOF(6, colDesc.scale)));
                 else
                   sprintf(valueBuff, "%04d-%02d-%02d %02d:%02d:%02d",
                                      dtvals[0], dtvals[1], dtvals[2],
                                      dtvals[3], dtvals[4], dtvals[5]);
                }
                break;

              default:
                retcode = -1;
                break;
            }
          break;

// Unary minus used in several places in the following code for intervals,
// which are always encoded as signed integers. The template instantiations for
// unsigned types will complain about the attempted negation.

        // For single-field intervals, all we have to do is right-justify the
        // value in a field with width equal to the interval's precision.
        // FormatRow turns this into a valid literal of the specific interval type.
        case REC_INT_YEAR:
        case REC_INT_MONTH:
        case REC_INT_DAY:
        case REC_INT_HOUR:
        case REC_INT_MINUTE:
          // ptr has been set and adjustment made for negative values above.
          sprintf(ptr, PFV64, colDesc.precision, value);

        case REC_INT_SECOND:
          // ptr has been set and adjustment made for negative values above.
          if (colDesc.scale > 0)
            {
              scaleFactor = (Int32)pow(10, colDesc.scale);
              sprintf(ptr,
                      PFV64 "." PFLV64,
                      colDesc.precision,
                      value / scaleFactor,
                      colDesc.scale,
                      value % scaleFactor);
            }
          else
            sprintf(ptr, PFV64, colDesc.precision, value);
          break;

        case REC_INT_YEAR_MONTH:
          // ptr has been set and adjustment made for negative values above.
          sprintf(ptr, PFV64 "-%02d", colDesc.precision, value/12, (Int32)(value%12));
          break;

        case REC_INT_DAY_HOUR:
          // ptr has been set and adjustment made for negative values above.
          sprintf(ptr, PFV64 " %02d", colDesc.precision, value/24, (Int32)(value%24));
          break;

        case REC_INT_HOUR_MINUTE:
          // ptr has been set and adjustment made for negative values above.
          sprintf(ptr, PFV64 ":%02d", colDesc.precision, value/60, (Int32)(value%60));
          break;

        case REC_INT_DAY_MINUTE:
          // ptr has been set and adjustment made for negative values above.
          sprintf(ptr,
                  PFV64 " %02d:%02d",
                  colDesc.precision, value/(24*60), (Int32)(value%(24*60)/60), (Int32)(value%(24*60)%60));
          break;

        case REC_INT_MINUTE_SECOND:
          // ptr has been set and adjustment made for negative values above.
          if (colDesc.scale > 0)
            {
              scaleFactor = (Int32)pow(10, colDesc.scale);
              fracSecPart = (Int32)(value % scaleFactor);
              intSecPart = value / scaleFactor;
              sprintf(ptr, 
                      PFV64 ":%02d.%0*d",
                      colDesc.precision, intSecPart / 60,
                      (Int32)(intSecPart % 60), colDesc.scale, fracSecPart);
            }
          else
            sprintf(ptr, PFV64 ":%02d", colDesc.precision, value/60, (Int32)(value%60));
          break;

        case REC_INT_HOUR_SECOND:
          // ptr has been set and adjustment made for negative values above.
          if (colDesc.scale > 0)
            {
              scaleFactor = (Int32)pow(10, colDesc.scale);
              fracSecPart = (Int32)(value % scaleFactor);
              intSecPart = (Int64)value / scaleFactor;
              sprintf(ptr,
                      PFV64 ":%02d:%02d.%0*d", 
                      colDesc.precision,
                      intSecPart / 3600,               // hours
                      (Int32)(intSecPart % 3600 / 60), // minutes
                      (Int32)(intSecPart % 60),        // seconds
                      colDesc.scale,
                      fracSecPart);
            }
          else
            sprintf(ptr,
                    PFV64 ":%02d:%02d",
                    colDesc.precision, value / 3600,
                    (Int32)(value % 3600 / 60), (Int32)(value % 60));
          break;

        case REC_INT_DAY_SECOND:
          // ptr has been set and adjustment made for negative values above.
          if (colDesc.scale > 0)
            {
              scaleFactor = (Int32)pow(10, colDesc.scale);
              fracSecPart = (Int32)(value % scaleFactor);
              intSecPart = (Int64)value / scaleFactor;
              sprintf(ptr,
                      PFV64 " %02d:%02d:%02d.%0*d", 
                      colDesc.precision,
                      intSecPart / 86400,         // days (86400 seconds=1 day)
                      (Int32)(intSecPart % 86400 / 3600),  // hours
                      (Int32)(intSecPart % 3600 / 60),     // minutes
                      (Int32)(intSecPart % 60),            // seconds
                      colDesc.scale,
                      fracSecPart);
            }
          else
            sprintf(ptr,
                    PFV64 " %02d:%02d:%02d", 
                    colDesc.precision,
                    value / 86400,           // days
                    (Int32)(value % 86400 / 3600),    // hours
                    (Int32)(value % 3600 / 60),       // minutes
                    (Int32)(value % 60));             // seconds
          break;

        case REC_BOOLEAN:
          {
            if (value)
              strcpy(valueBuff,"TRUE");
            else
              strcpy(valueBuff,"FALSE");
          }
          break;


        default:
          retcode = -1;
          break;
       } // switch
      }  // else

  return retcode;
}


/***************************************************************************/
/* METHOD:  setBufferValue()                                             */
/* PURPOSE: Format the boundary value for the current interval as a        */
/*          Unicode string. The boundary value is the max value represented*/
/*          by the interval, except in the case of interval 0, in which    */
/*          case it is the minimum value of interval 1. This function is   */
/*          used only for internal sort.                                   */
/* PARAMS:  value(in)    -- Boundary value in string form.                 */
/*          group(in)    -- Group the histogram is for.                    */
/*          boundary(out) -- HSDataBuffer object representing the boundary.*/
/* RETCODE: 0 if successful, negative if error.                            */
/***************************************************************************/
template <class T>
Lng32 setBufferValue(T& value,
                      const HSColGroupStruct *group,
                      HSDataBuffer &boundary)
  {
    HSLogMan *LM = HSLogMan::Instance();
    const HSColumnStruct &colDesc = group->colSet[0];
    Lng32 retcode = 0;
    char formatRowBuff[HS_MAX_BOUNDARY_LEN+10];
    char valueBuff    [HS_MAX_BOUNDARY_LEN+10];
    char *ptr;
    NAWchar *wptr;
    short *len = (short*)formatRowBuff;
    char *data = (char*)(formatRowBuff + sizeof(short));  // Offset of data pointer.

    // Copy the value, with any required formatting, into the buffer.
    Int32 rc=0;
    if ((rc = copyValue(value, valueBuff, colDesc, len)) < 0)
      {
        LM->Log("INTERNAL ERROR (copyValue):");
        sprintf(LM->msg, "Undefined datatype %d", colDesc.datatype);
        LM->Log(LM->msg);
        char errCode[20];
        sprintf(errCode, "%d", rc);
        *CmpCommon::diags() << DgSqlCode(-UERR_GENERIC_ERROR)
                            << DgString0("copyValue()")
                            << DgString1(errCode)
                            << DgString2(LM->msg);
        throw CmpInternalException("failure in copyValue()",
                                   __FILE__, __LINE__);
      }

    // *len has already been set for char types due to possibility of embedded
    // nulls, but will have to be adjusted here to len in bytes instead of chars.
    if (DFS2REC::isAnyCharacter(colDesc.datatype))
      {
        // Unicode char strings are already in the proper format and can just
        // be copied to the output buffer. For ascii strings, convert to their
        // Unicode equivalent.
        if (DFS2REC::isDoubleCharacter(colDesc.datatype))
          memmove(data, valueBuff, *len);
        else
          {
            wptr = (NAWchar*)data;
            ptr = valueBuff;
            while (ptr < valueBuff + *len)
              {
                if (*ptr)
                  { *wptr++ = (NAWchar)(unsigned char)(*ptr); ptr++; }
                else
                  {
                    *wptr++ = NAWchar('\0');
                    ptr++;
                  }
              }
            *len *= 2;  // len must be in bytes, not chars
          }
      }
    else
      {
        *len = strlen(valueBuff);
        na_mbstowcs((NAWchar*)data, valueBuff, *len);  // Len of destination string.
        *len *= 2;        // len must be in bytes, not chars
      }

    retcode = FormatRow(&colDesc, formatRowBuff, boundary);
    HSHandleError(retcode);
    return retcode;
  }

/***************************************************************************/
/* METHOD:  setBufferValue()                                             */
/* PURPOSE: Template specialization that handles the non-internal sort     */
/*          case, where the boundary value is already in the proper form   */
/*          and just needs to be passed to FormatRow().                    */
/* PARAMS:  value(in)    -- Boundary value in string form.                 */
/*          group(in)    -- Group the histogram is for.                    */
/*          boundary(out) -- HSDataBuffer object representing the boundary.*/
/* RETCODE: 0 if successful, negative if error.                            */
/***************************************************************************/
Lng32 setBufferValue(myVarChar& value,
                      const HSColGroupStruct *group,
                      HSDataBuffer &boundary)
  {
    Lng32 retcode = 0;
    Lng32 colcount = group->colCount;
    if(group->skewedValuesCollected)
    {
      short lengthOfToken;
      Lng32 i = 0;
      HSDataBuffer tempBoundary(WIDE_(""));
      boundary = tempBoundary;
      HSDataBuffer comma(WIDE_(","));

      short remainingLength = value.len;
      char * tempStr  = (char *) &value;
      tempStr += sizeof(short);

      NABoolean isACharColumn= FALSE;
      NAWchar* ptrToContiguousSingleQuotes = NULL;
      char* copyOfValue = NULL;
      short noOfContiguousSingleQuotes = 0, tempLength = 0;      

      short sizeOfNAWchar =
          sizeof(NAWchar);

      // na_wcswcs below expects the string to be null terminated
      // so we are using a temporary string that is a copy of
      // value and is one NAWchar longer with the last char set to
      // null
      copyOfValue = new (STMTHEAP) char[remainingLength+sizeOfNAWchar];
      memcpy(copyOfValue, tempStr, remainingLength);
      NAWchar* wptr = ((NAWchar*)copyOfValue) + ((remainingLength+sizeOfNAWchar)/sizeOfNAWchar) -1;
      *wptr = WIDE_('\0');
      tempStr = copyOfValue;

      NAWchar tempString [HS_MAX_UCS_BOUNDARY_CHAR];
      NAWchar *tempStringPtr = tempString;

      NAWchar * begin = (NAWchar *) tempStr;
      if(*begin == L'\'')
        isACharColumn = TRUE;
      NAWchar* end = NULL;
      
      if(isACharColumn)
      {
        begin++;
        end = na_wcschr(begin, L'\'');
        while(*(++end) == L'\'')
        {
          end++;
          end = na_wcschr(end, L'\'');
        }
        end--;
      }
      else
        end = na_wcschr(begin, L',');

      do
      {
        if(isACharColumn)
        {        
          remainingLength -= sizeof(NAWchar);
          ptrToContiguousSingleQuotes = (NAWchar *)begin;
          while(ptrToContiguousSingleQuotes = na_wcswcs(ptrToContiguousSingleQuotes, WIDE_("''")))
          {
            noOfContiguousSingleQuotes++;
            tempLength = remainingLength - ((ptrToContiguousSingleQuotes - ((NAWchar *)begin)) + 1) * sizeof(NAWchar);
            memmove(ptrToContiguousSingleQuotes, (ptrToContiguousSingleQuotes + 1), tempLength);
            if(i == (colcount - 1))
              begin[(remainingLength-2)/2] = L'\0';
            ptrToContiguousSingleQuotes ++;
            if(end)
              end--;
          }
          remainingLength -= sizeof(NAWchar);
        }

	if(i < colcount - 1)
	  lengthOfToken = (end - begin) * sizeof(NAWchar);
	else
	  lengthOfToken = remainingLength - (noOfContiguousSingleQuotes*sizeof(NAWchar));

	memcpy(tempStringPtr, &lengthOfToken , sizeof(short));
	tempStringPtr ++;
	na_wcsncpy (tempStringPtr, begin, (lengthOfToken /sizeof(NAWchar)));
	tempStringPtr --;

	retcode = FormatRow(&(group->colSet[i]), (char *)tempStringPtr, tempBoundary);
	HSHandleError(retcode);
	if(retcode)
	  break;
	else
	{
	  retcode = boundary.append(tempBoundary);
	  if(retcode)
	    break;
	}

	remainingLength -= (lengthOfToken + (noOfContiguousSingleQuotes*sizeof(NAWchar)));

	if(end && (remainingLength > 0))
	{
	  retcode = boundary.append(comma);
	  if(retcode)
	    break;

	  remainingLength -= sizeof(NAWchar);
      if(isACharColumn)
      {
        noOfContiguousSingleQuotes = 0;
        isACharColumn = FALSE;
        end++;
      }
	  begin = end;
	  begin ++;
      if(*begin == L'\'')
        isACharColumn = TRUE;
      if(isACharColumn)
      {
        begin++;
        end = na_wcschr(begin, L'\'');
        while(end && *(++end) == L'\'')
        {
          end++;
          end = na_wcschr(end, L'\'');
        }
        if(end)
          end--;
      }
      else
        end = na_wcschr(begin, L',');
    }
	i++;
    }	while (i < colcount);

    if(copyOfValue)
      NADELETEBASIC(copyOfValue, STMTHEAP);
    }
    else
    {
      const HSColumnStruct &colDesc = group->colSet[0];
      retcode = FormatRow(&colDesc, (char*)&value, boundary);
      HSHandleError(retcode);
    }
    return retcode;
  }

/***************************************************************************/
/* METHOD:  setBufferValue()                                               */
/* PURPOSE: Template specialization that handles the MC internal sort case */
/*          to format the current MC interval boundary value as a Unicode  */
/*          string.                                                        */
/* PARAMS:  value(in)    -- Boundary value in string form.                 */
/*          group(in)    -- Group the histogram is for.                    */
/*          boundary(out) -- HSDataBuffer object representing the boundary.*/
/* RETCODE: 0 if successful, negative if error.                            */
/***************************************************************************/

Lng32 setBufferValue(MCWrapper& value,
                     const HSColGroupStruct *mgroup,
                     HSDataBuffer &boundary)
{
    Lng32 retcode = 0;
    Lng32 colcount = mgroup->colCount;
    HSDataBuffer comma(WIDE_(","));
    HSDataBuffer null(WIDE_("NULL"));

    NABoolean isNull = FALSE;

    HSLogMan *LM = HSLogMan::Instance();
    const HSColumnStruct &colDesc = mgroup->colSet[0];
    char formatRowBuff[HS_MAX_BOUNDARY_LEN+10];
    char valueBuff    [HS_MAX_BOUNDARY_LEN+10];
    char *ptr;
    NAWchar *wptr;
    short *len = (short*)formatRowBuff;
    char *data = (char*)(formatRowBuff + sizeof(short));  // Offset of data pointer.

    ISFixedChar isf;
    ISVarChar   isv;

    // Copy the value, with any required formatting, into the buffer.
    Int32 rc=0;
    for (Int32 i = 0; i < colcount; i++)
    {
       const HSColumnStruct &colDesc = mgroup->colSet[i];
       if (value.allCols_[i]->isNull(value.index_))
       {
          isNull = TRUE;
       }

       else
       {
          switch (value.allCols_[i]->ISdatatype)
          {
             case REC_BIN8_SIGNED:
                retcode = copyValue(*((MCNonCharIterator<Int8>*)(value.allCols_[i]))->getContent(value.index_), valueBuff, mgroup->colSet[i], len);
                break;
             case REC_BOOLEAN:
             case REC_BIN8_UNSIGNED:
                retcode = copyValue(*((MCNonCharIterator<UInt8>*)(value.allCols_[i]))->getContent(value.index_), valueBuff, mgroup->colSet[i], len);
                break;
             case REC_BIN16_SIGNED:
                retcode = copyValue(*((MCNonCharIterator<short>*)(value.allCols_[i]))->getContent(value.index_), valueBuff, mgroup->colSet[i], len);
                break;
             case REC_BIN16_UNSIGNED:
                retcode = copyValue(*((MCNonCharIterator<unsigned short>*)(value.allCols_[i]))->getContent(value.index_), valueBuff, mgroup->colSet[i], len);
                break;
             case REC_BIN32_SIGNED:
                retcode = copyValue(*((MCNonCharIterator<Int32>*)(value.allCols_[i]))->getContent(value.index_), valueBuff, mgroup->colSet[i], len);
                break;
             case REC_BIN32_UNSIGNED:
                retcode = copyValue(*((MCNonCharIterator<UInt32>*)(value.allCols_[i]))->getContent(value.index_), valueBuff, mgroup->colSet[i], len);
                break;
             case REC_BIN64_SIGNED:
                retcode = copyValue(*((MCNonCharIterator<Int64>*)(value.allCols_[i]))->getContent(value.index_), valueBuff, mgroup->colSet[i], len);
                break;
             case REC_BIN64_UNSIGNED:
                retcode = copyValue(*((MCNonCharIterator<UInt64>*)(value.allCols_[i]))->getContent(value.index_), valueBuff, mgroup->colSet[i], len);
                break;
             case REC_IEEE_FLOAT32:
                retcode = copyValue(*((MCNonCharIterator<float>*)(value.allCols_[i]))->getContent(value.index_), valueBuff, mgroup->colSet[i], len);
                break;
             case REC_IEEE_FLOAT64:
                retcode = copyValue(*((MCNonCharIterator<double>*)(value.allCols_[i]))->getContent(value.index_), valueBuff, mgroup->colSet[i], len);
               break;
             case REC_BYTE_F_ASCII:
             case REC_BYTE_F_DOUBLE:
             case REC_BINARY_STRING:
                ((MCFixedCharIterator*)(value.allCols_[i]))->copyToISFixChar(isf, value.index_);
                retcode = copyValue(isf, valueBuff, mgroup->colSet[i], len);
               break;
             case REC_BYTE_V_ASCII:
             case REC_BYTE_V_DOUBLE:
             case REC_VARBINARY_STRING:
                ((MCVarCharIterator*)(value.allCols_[i]))->copyToISVarChar(isv, value.index_);
                retcode = copyValue(isv, valueBuff, mgroup->colSet[i], len);
               break;
             default:
                retcode = -1;
                break;
          }
       }

       if (retcode < 0)
       {
           LM->Log("INTERNAL ERROR (copyValue):");
           sprintf(LM->msg, "Undefined datatype %d", mgroup->colSet[i].datatype);
           LM->Log(LM->msg);
           char errCode[20];
           sprintf(errCode, "%d", rc);
           *CmpCommon::diags() << DgSqlCode(-UERR_GENERIC_ERROR)
                               << DgString0("copyValue()")
                               << DgString1(errCode)
                               << DgString2(LM->msg);
           throw CmpInternalException("failure in copyValue()", __FILE__, __LINE__);
       }
       else
       {
           if (i!=0)
              boundary.append(comma);

           if (!isNull)
           {
              // *len has already been set for char types due to possibility of embedded
              // nulls, but will have to be adjusted here to len in bytes instead of chars.
              if (DFS2REC::isAnyCharacter(colDesc.datatype))
              {
                  // Unicode char strings are already in the proper format and can just
                  // be copied to the output buffer. For ascii strings, convert to their
                  // Unicode equivalent.
                  if (DFS2REC::isDoubleCharacter(colDesc.datatype))
                    memmove(data, valueBuff, *len);
                  else
                  {
                      wptr = (NAWchar*)data;
                      ptr = valueBuff;
                      while (ptr < valueBuff + *len)
                      {
                          if (*ptr)
                          {
                             *wptr++ = (NAWchar)(unsigned char)(*ptr); 
                              ptr++; 
                          }
                          else
                          {
                             *wptr++ = NAWchar('\0');
                             ptr++;
                          }
                      }
                      *len *= 2;  // len must be in bytes, not chars
                  }
              }
              else
              {
                 *len = strlen(valueBuff);
                 na_mbstowcs((NAWchar*)data, valueBuff, *len);  // Len of destination string.
                 *len *= 2;        // len must be in bytes, not chars
              }

              HSDataBuffer tempBoundary;
              retcode = FormatRow(&colDesc, formatRowBuff, tempBoundary);
              HSHandleError(retcode)
              boundary.append(tempBoundary);
           }
           else
           {
              boundary.append(null);
              isNull = FALSE;
           }
       }
    }

    return retcode;
}

// The following 3 functions (getDigitCount(), convInt64ToAscii(), and
// convFloat64ToAscii()) were borrowed from exp\exp_conv.cpp, and declared as
// static to give them internal linkage and avoid link errors.

static Lng32 getDigitCount(Int64 value)
  {
    static const Int64 decValue[] = {0,
                               9,
                               99,
                               999,
                               9999,
                               99999,
                               999999,
                               9999999,
                               99999999,
                               999999999,
                               9999999999LL,
                               99999999999LL,
                               999999999999LL,
                               9999999999999LL,
                               99999999999999LL,
                               999999999999999LL,
                               9999999999999999LL,
                               99999999999999999LL,
                               999999999999999999LL};

    for (Int32 i = 4; i <= 16; i += 4)
      if (value <= decValue[i]) {
	if (value <= decValue[i-3])
	  return(i-3);
	if (value <= decValue[i-2])
	  return(i-2);
	if (value <= decValue[i-1])
	  return(i-1);
	else return i;
      }
    if (value <= decValue[17])
      return 17;
    if (value <= decValue[18])
      return 18;
    return 19;
  }

//ex_expr::exp_return_type convInt64ToAscii(char *target,
static short convInt64ToAscii(char *target,
					  Lng32 targetLen,
					  Int64 source,
					  Lng32 scale,
					  char * varCharLen,
					  Lng32 varCharLenSize,
					  char filler,
					  NABoolean leadingSign,
                                          NABoolean leftPad,
					  CollHeap *heap,
					  ComDiagsArea** diagsArea) {

  Lng32 digitCnt = 0;
  NABoolean negative = (source < 0);
  NABoolean fixRightMost  = FALSE;  // True if need to fix the rightmost digit.

  Lng32 padLen = targetLen;
  Lng32 requiredDigits = 0;
  Lng32 leftMost;   // leftmost digit.
  Lng32 rightMost;  // rightmost digit.
  Lng32 sign = 0;

  //  Int64 newSource = (negative ? -source : source);
  Int64 newSource = 0; 
  if ((negative) && (source == 0x8000000000000000LL)) // = -2 ** 63
    {
      newSource = 0x7fffffffffffffffLL;
      //             123456789012345
      digitCnt = 19;
      fixRightMost = TRUE;
    }
  else
    {
      newSource = (negative ? -source : source);
      digitCnt = getDigitCount(newSource);
    }

  if (leadingSign || negative) {
    sign = 1;
    padLen--;
  }
  // No truncation allowed.
  requiredDigits = digitCnt;
  // Add extra zero's.
  if (scale > requiredDigits)
    requiredDigits += (scale - requiredDigits);
  padLen -= requiredDigits;
  if (scale)
    padLen--; // decimal point
  if (padLen < 0) {
    // target string is not long enough - overflow
//    ExRaiseSqlError(heap, diagsArea, EXE_STRING_OVERFLOW);
//    return ex_expr::EXPR_ERROR;   
      return 1;
  } 
  
  if (varCharLenSize) {
    // we do not pad. Instead, we adjust the targetLen
    leftPad = FALSE;
    targetLen -= padLen;
    padLen = 0;
  };

  if (leftPad) { 
    leftMost = padLen + sign;  
  }
  else {
    leftMost = sign;
  }

  Lng32 currPos;
  // Add filler.
  rightMost = currPos = targetLen - 1; 
  if (padLen) {
    Lng32 start; 
    if (leftPad) { // Pad to the left. 
      start = sign;
    }
    else { // Pad to the right
      start = targetLen - padLen;
      rightMost = currPos = start - 1;
    }
    str_pad(&target[start], padLen, filler);
  }

  // Convert the fraction part and add decimal point.
  if (scale) {
    Lng32 low = (currPos - scale);
    for (; currPos > low; currPos--) {
      target[currPos] = (char)(Int32)(newSource % 10) + '0';
      newSource /= 10;
    }
    target[currPos--] = '.';
  }

  // Convert the integer part.
  for (; currPos >= leftMost; currPos--) {
    target[currPos] = (char)(Int32)(newSource % 10) + '0';
    newSource /= 10;
  }

  // Add sign.
  if (leadingSign) {
    if (negative)
      target[0] = '-';
    else 
      target[0] = '+';
  }
  else if (negative) 
      target[currPos] = '-';

  // Fix the rightmost digit for -2 ** 63.
  if (fixRightMost && target[rightMost] == '7')
    target[rightMost] = '8';

  if (newSource != 0 || currPos < -1)
    { // Sanity check fails.
//      ExRaiseSqlError(heap, diagsArea, EXE_STRING_OVERFLOW);
//      return ex_expr::EXPR_ERROR;   
      return 1;
    }

  // Set varchar length field for varchar.
  if (varCharLenSize)
    if (varCharLenSize == sizeof(Lng32))
      str_cpy_all(varCharLen, (char *) &targetLen, sizeof(Lng32));
    else {
      short VCLen = (short) targetLen;
      str_cpy_all(varCharLen, (char *) &VCLen, sizeof(short));
    };

//  return ex_expr::EXPR_OK;
  return 0;
};

//////////////////////////////////////////////////////////////////
// function to convert an FLOAT64  to an ASCII string
// Trailing '\0' is not set!
// This routine assumes that targetLen is at least
// SQL_REAL_MIN_DISPLAY_SIZE:
// 1 byte sign
// 1 byte for digit in front of decimal point
// 1 byte decimal point
// 1 byte for at least one digit after decimal point
// 5 bytes for exponent (E+DDD)
///////////////////////////////////////////////////////////////////
static short convFloat64ToAscii(char *target,
		 Lng32 targetLen,
			double source,
			// maximum # of fraction digits
		 Lng32 digits,
			char * varCharLen,
		 Lng32 varCharLenSize,
			NABoolean leftPad) {

  short err = 0;

  Lng32 displaySize = digits + 8; // Mantissa = digits + 3, E = 1, Exponent = 4
  HS_ASSERT(displaySize <= SQL_DOUBLE_PRECISION_DISPLAY_SIZE);
  char tempTarget[SQL_DOUBLE_PRECISION_DISPLAY_SIZE + 1];
  //char format[8];

  // the fraction has always between 1 and "digits" digits
  if ((targetLen - 8) < digits)
    digits = targetLen - 8;

  Lng32 usedTargetLen = MINOF(displaySize, targetLen);

  double absSource = source;
  NABoolean neg = FALSE;
  if (source < 0)
    {
      absSource = -source;
      neg = TRUE;
    }

  Int64 expon = 0;
  Int64 intMantissa = 0;
  NABoolean expPos = TRUE;
  if (absSource > 0)
    {
      double logTen = MathLog10(absSource, err);
      if (err)
	return -1;

      if (logTen >= 0)
	{
	  expPos = TRUE;
	}
      else
	{
	  logTen = -logTen;
	  expPos = FALSE;
	};
      
      while (logTen > 0)
	{
	  expon++;
	  logTen -= 1;
	}
      
      if ((expPos) && (logTen != 0))
	expon--;

      NABoolean reduceExpon = FALSE;
      short reduceExponBy = 0;
      if (expon >= DBL_MAX_10_EXP)
	{
	  // if expon is greater than MAX exponent allowed, then reduce it 
	  // the diff between expon and DBL_MAX_10_EXP.
	  // This is needed so the next MathPow call doesn't return
	  // an error when it tries to do   10 ** DBL_MAX_10_EXP
	  // (which will make it greater than the max double value).
	  reduceExponBy = (short)(expon - DBL_MAX_10_EXP + 1);
	  expon -= reduceExponBy;
	  reduceExpon = TRUE;
	}
      double TenPowerExpon = 
	((expPos == FALSE) ? MathPow(10.0, (double)expon, err)
                           : MathPow(10.0, (double)-expon, err));
      if (err)
	return -1;

      double mantissa = absSource * TenPowerExpon;
      
      if (reduceExpon)
	{
	  // now fix mantissa by multiplying or dividing by 10.
	  if (expPos == FALSE)
	    mantissa = mantissa * MathPow(10.0, reduceExponBy, err);
	  else
	    mantissa = mantissa / MathPow(10.0, reduceExponBy, err);

	  if (err)
	    return -1;

	  // and increase expon to its original value
	  expon += reduceExponBy;
	}


      intMantissa = (Int64)(mantissa * MathPow(10.0,(double)digits,err));

      if (err)
	return -1;
    }

  short error;
  error =
    convInt64ToAscii(tempTarget, digits+3, (neg ? -intMantissa : intMantissa),
		     digits, NULL, 0, ' ',
		     neg, TRUE, NULL, NULL);
  if (error)
    return -1;

  if (intMantissa == 0)
    {
      // add a 0 before the decimal point of mantissa
      tempTarget[1] = '0';
    }

  tempTarget[digits+3] = 'E';
  error =
    convInt64ToAscii(&tempTarget[digits+4], 4, (expPos ? expon : -expon), 
		     0, NULL, 0, '0',
		     TRUE, TRUE, NULL, NULL);
  if (error)
    return -1;

  char *pch = tempTarget;
  while (*pch == ' ') {
    usedTargetLen--;
    pch++;
  }

  if (varCharLenSize) {
    // the target is a varChar. Just move the data left adjusted and
    // set the length field.
    str_cpy_all(target, pch, usedTargetLen);

    if (varCharLenSize == sizeof(Lng32))
      str_cpy_all(varCharLen, (char *) &usedTargetLen, sizeof(Lng32));
    else {
      short VCLen = (short) usedTargetLen;
      str_cpy_all(varCharLen, (char *) &VCLen, sizeof(short));
    };
  }
  else {
    // if target is larger than usedTargetLen, fill in blanks.
    Lng32 padLen = targetLen - usedTargetLen;
    if (leftPad) {
      str_pad(target, padLen, ' ');
      str_cpy_all(&target[padLen], pch, usedTargetLen);
    }
    else {
      str_cpy_all(target, pch, usedTargetLen);
      str_pad(&target[usedTargetLen], padLen, ' ');
    } 
  };
  return 0;
}


#ifdef _TEST_ALLOC_FAILURE
// Sets up the array that indicates which memory allocation attempts for internal
// sort are to be simulated as failures. If then nth allocation is to "fail",
// then n will be an element of the array. The values to put in the array come
// from the CQD COMP_STRING_5, which should have a sequence of numbers separated
// by the '/' character.
//
void HSColGroupStruct::initFilter()
{
  HSLogMan *LM = HSLogMan::Instance();
  char* cqdValue = (char*)ActiveSchemaDB()->getDefaults().getValue(COMP_STRING_5);
  char* filterString = new (STMTHEAP) char[strlen(cqdValue)+1];
  // Have to copy; strtok overwrites delims with nulls in stored cqd value.
  strcpy(filterString, cqdValue);
  char* val = strtok(filterString, "/");
  Int32 i = 1; // count will be stored in array elem 0
  while (val && i<MAX_FILTER_COUNT)
    {
      filterTargets[i++] = atoi(val);
      if (LM->LogNeeded())
        {
          sprintf(LM->msg, "Filter target %d = %d", i-1, filterTargets[i-1]);
          LM->Log(LM->msg);
        }
      val = strtok(NULL, "/");
    }
  filterTargets[0] = i-1;
  NADELETEBASIC(filterString, STMTHEAP);
}

// Filters memory allocation requests for internal sort, returning TRUE for
// the ones that match an element of filterTargets, which results in NULL
// being returned by the memory allocation routine. The parameter is the
// number of allocation requests for internal sort so far. filterTargets is
// a list of the requests, by ordinal position, that are to be simulated
// failures.
//
// Parameters:
//   count -- Number of memory allocation requests for internal sort overall
//            for the current update stats statement, including the current
//            allocation request.
// Return value:
//   TRUE if the allocation is to fail, FALSE otherwise.
//
NABoolean HSColGroupStruct::allocFilter(Lng32 count)
{
  for (Int32 i=1; i<=filterTargets[0]; i++)
    {
      if (count == filterTargets[i])
        return TRUE;
    }

  return FALSE;
}
#endif  /* _TEST_ALLOC_FAILURE */


/****************************************************************************/
/* METHOD:  histIsObsolete()                                                */
/* PURPOSE: Determine whether or not a histogram is to be considered        */
/*          obsolete. If either the total number of inserts/deletes/updates */
/*          since the last stats generation, or the difference in row       */
/*          counts, is greater than a certain percentage of the row count   */
/*          stored in the histogram (determined by the CQD                  */
/*          USTAT_OBSOLETE_PERCENT_ROWCOUNT CQD), we consider it obsolete.  */
/* RETCODE: TRUE if obsolete, FALSE otherwise.                              */
/* PARAMS:                                                                  */
/*   obsoletePercent(in) -- Percentage of the histogram row count that must */
/*                          have changed for the histogram to be considered */
/*                          obsolete.                                       */
/*   histRowCount(in)    -- Row count stored with the histogram.            */
/*   changedRowCount(in) -- Sum of inserts/deletes/updates since last time  */
/*                          Update Statistics was run on the table.         */
/****************************************************************************/
NABoolean histIsObsolete(float obsoletePercent,
                         Int64 histRowCount,
                         Int64 changedRowCount)
  {
    HSGlobalsClass *hs_globals = GetHSContext();
    Int64 obsoleteThreshold = (Int64)(obsoletePercent * histRowCount);
    Int64 rowCountDiff = hs_globals->actualRowCount - histRowCount;
    if (rowCountDiff < 0)
      rowCountDiff = -rowCountDiff;  // No abs fn for Int64
    return MAXOF(changedRowCount, rowCountDiff) > obsoleteThreshold;
  }

/****************************************************************/
/* METHOD:  AddNecessaryColumns()                               */
/* PURPOSE: Determine histograms in need of update by finding   */
/*          those that have been requested by the optimizer but */
/*          were not available, or had abbreviated stats added  */
/*          on the fly by the optimizer.                        */
/* RETCODE:  0 - successful                                     */
/*          -1 - failure                                        */
/* PARAMS:  none                                                */
/****************************************************************/
Lng32 AddNecessaryColumns()
  {
    HSLogMan *LM = HSLogMan::Instance();
    HSGlobalsClass *hs_globals = GetHSContext();

    if (LM->LogNeeded())
      LM->Log("Determining NECESSARY columns");

    Lng32 retcode;
    HSColGroupStruct *group = NULL, *sgroup = NULL, *sgroup2 = NULL, *prevGroup = NULL;
    HSColumnStruct col;
    NAString columnName;
    char tempStr[30];
    Int64 objID = hs_globals->objDef->getObjectUID(); // TABLE_UID
    Lng32 maxMCWidthForAutomation = CmpCommon::getDefaultLong(USTAT_AUTO_MC_MAX_WIDTH);
    // Columns read from Histograms table.
    ULng32 histid;
    Lng32 colPos;
    Lng32 colNum;
    Lng32 colCount;
    char reason;

    // Keep track of info on single-column groups for processing multi-column
    // groups; READ_TIME, etc. columns are not maintained for the MC groups.
    struct SingleColStatus
      {
        SingleColStatus() 
          : recentlyRead(FALSE), obsolete(FALSE)
          {}
        NABoolean recentlyRead;
        NABoolean obsolete;
      };

    SingleColStatus *singleCols 
          = new (STMTHEAP) SingleColStatus[hs_globals->objDef->getNumCols()+1];  // +1 bec. 0 not used

    // Max read age must be at least twice as long as the automation interval.
    Lng32 maxReadMinutes = CmpCommon::getDefaultLong(USTAT_MAX_READ_AGE_IN_MIN);
    if (maxReadMinutes < 2 * HSGlobalsClass::autoInterval)
      maxReadMinutes = 2 *  HSGlobalsClass::autoInterval;

    // Calculate oldest allowed READ_TIME in GMT.
    char maxGMTTimeStr[HS_TIMESTAMP_SIZE];
    Int64 curSecs    = hs_getEpochTime();                     // Get current time in secs.
    Int64 oldestSecs = curSecs - (Int64) (maxReadMinutes*60); // Subtract max read time.
    hs_formatTimestamp(oldestSecs, maxGMTTimeStr);     // Convert to GMT and timestamp string.

    if (LM->LogNeeded())
      {
        sprintf(LM->msg, "\tLooking for histograms read in the last %d minutes.", 
                         maxReadMinutes);
        LM->Log(LM->msg);
      }

    // Start a transaction if one not already in progress. The query used to
    // get the current histograms would implicitly start one otherwise, and
    // we would finish the update stats statement still in a transaction that
    // the user did not start. The HSTranController object will either commit
    // or abort the transaction in its dtor on function exit. It must be
    // declared BEFORE the HSErrorCatcher object, so the error catcher dtor
    // is invoked first (reverse order of construction). Otherwise, the execution
    // of the rollback in an error case will cause the CLI diagnostics area to be
    // cleared before the CLI errors can be merged with our own diagnostics area
    // by the HSErrorCatcher dtor.
    //
    HSTranController TC("GET GROUP LIST FOR NECESSARY", &retcode);
    HSErrorCatcher errorCatcher(retcode, -UERR_INTERNAL_ERROR, "AddNecessaryColumns", TRUE);
    
#ifdef NA_USTAT_USE_STATIC  // use static query defined in module file
    // Note that the output list of the query used in the SQ module file no
    // longer matches the items selected by the new dynamic version of the query.
    HSCliStatement necStmt(HSCliStatement::CURSOR105_MX_2300,
                           (char *)hs_globals->hstogram_table->data(),
                           (char *)&objID,
                           (char *)&maxGMTTimeStr);
#else // NA_USTAT_USE_STATIC not defined, use dynamic query
    char sbuf[25];
    NAString qry = "SELECT HISTOGRAM_ID, COL_POSITION, COLUMN_NUMBER, COLCOUNT, REASON "
                   "FROM ";
    qry.append(hs_globals->hstogram_table->data());
    qry.append(    " WHERE TABLE_UID=");
    snprintf(sbuf, sizeof(sbuf), PF64, objID);
    qry.append(sbuf);
    qry.append(      " AND (REASON='S'");
    qry.append(       " OR  REASON=' ')");
    qry.append(    " ORDER BY COLCOUNT, HISTOGRAM_ID, COL_POSITION");
    qry.append(    " FOR READ COMMITTED ACCESS");

    HSCursor necStmt(STMTHEAP, "HS_NECESSARY_COLS_STMT");
    retcode = necStmt.prepareQuery(qry.data(), 0, 5);
    HSLogError(retcode);
    if (retcode < 0)
      {
        if (LM->LogNeeded())
          LM->Log("Failed to prepare query to fetch missing stats data from histograms table");
        errorCatcher.setString1(" in call to prepareQuery()");
        return retcode;
      }
#endif
    retcode = necStmt.open();
    if (retcode < 0)
      {
        if (LM->LogNeeded())
          LM->Log("Failed to open cursor for query to fetch missing stats data from histograms table");
        errorCatcher.setString1(" in call to open()");
        return retcode;
      }

    colCount = 1;                       // just to satisfy loop initially
    hs_globals->allMissingStats = TRUE; // Initialize flag.

    // First read through the single-column groups. The query for the cursor
    // we're using sorts by COLCOUNT, so the singles will come first. We need
    // their info before we can process the multicolumn groups.
    while (retcode == 0 && colCount == 1)
      {
        retcode = necStmt.fetch(5,
                               (void *)&histid, (void *)&colPos,
                               (void *)&colNum, (void *)&colCount,
                               (void *)&reason
                              );

        if (retcode || colCount > 1)  // end of data, error, or end of single-col groups
          break;

        if (LM->LogNeeded())
          {
            sprintf(LM->msg, "\tFound column %s", hs_globals->objDef->getColName(colNum));
            LM->Log(LM->msg);
          }
        if (ColumnExists(colNum))     // this column already explicitly requested
          {
            if (reason == HS_REASON_EMPTY && prevGroup) {
              sprintf(tempStr, " %u,", histid);
              prevGroup->oldHistidList += tempStr;
              if (LM->LogNeeded())
                LM->Log("\t(duplicate empty histogram)");
            }
            else if (LM->LogNeeded())
              LM->Log("\t\t(already explicitly requested)");
            continue;
          }

        singleCols[colNum].recentlyRead = TRUE;  // query selects only these

        // Set up a group containing this column, add it to the HSGlobalsClass
        // single-col list, and allocate a new group for the next one.
        HS_ASSERT(colCount == 1); // some of the following code assumes this
        group = new(STMTHEAP) HSColGroupStruct;
        col = hs_globals->objDef->getColInfo(colNum); // colNum = position in table
        col.colnum = colNum;
        col.position = colPos;
        columnName = hs_globals->objDef->getColName(colNum);
        group->colSet.insert(col);
        group->oldHistid = histid;
        group->colCount = colCount;
        HS_ASSERT(reason == HS_REASON_EMPTY ||
                  reason == HS_REASON_SMALL_SAMPLE);
        group->newReason = HS_REASON_AUTO_INIT;
        *group->colNames += ToAnsiIdentifier(columnName);
        hs_globals->addGroup(group);
        prevGroup = group;

        // If reason for this single column histogram is not empty or small-stats,
        // all of the histograms being generated are NOT for missing statistics.
        // (If all single column histograms ARE for missing statistics, then
        // all histograms are.)  This flag is used to determine whether table
        // rowcounts should be reset.
        // (Note: the condition will never be true with the current code; it was
        //  left in place so it isn't forgotten if we add an obsolescence
        //  criterion for automation).
        if (reason != HS_REASON_EMPTY && reason != HS_REASON_SMALL_SAMPLE)
          hs_globals->allMissingStats = FALSE;
      }

    if (LM->LogNeeded() && (hs_globals->optFlags & SAMPLE_REQUESTED))
      LM->Log("Using sampling specified by user.");

    // Next read the multicolumn groups. Upon entry, the first column
    // of the first MC group has been read (unless retcode != 0).
    NABoolean skipToNextHist;
    NABoolean rowReadByPreviousLoop = TRUE;
    while (retcode == 0)
      {
        // Get the next row, except on the first iteration when we already have
        // the one read by the loop above. Query returns rows ordered by
        // colcount/histid/colpos, so MC columns are read in correct order.
        if (rowReadByPreviousLoop)
          {
            HS_ASSERT(colPos == 0);
            rowReadByPreviousLoop = FALSE;  // need to read a new one next time
          }
        else
          {
            retcode = necStmt.fetch(5,
                                  (void *)&histid, (void *)&colPos,
                                  (void *)&colNum, (void *)&colCount,
                                  (void *)&reason
                                  );
            if (retcode)  // end of data or error
              break;
          }

        // Since the results are ordered by column position within histogram
        // id, we know we're starting a new group when colPos is 0.
        // All rows representing the MC will have the same reason and colcount
        // values, so we can tell whether the MC should be included in the
        // necessary histograms by looking at the first row. If it is to be
        // included, create the group and set the values for it that are not
        // column-specific. Below, the component columns of the MC are added
        // to the group, one per iteration of this loop.
        if (colPos == 0)
          {
            // Skip if not an empty histogram or if # cols in the histogram is
            // larger than that allowed for automation. An MC histogram will
            // never have a reason of HS_REASON_SMALL_SAMPLE.
            skipToNextHist = (reason != HS_REASON_EMPTY || colCount > maxMCWidthForAutomation);
            if (!skipToNextHist)
              {
                group = new(STMTHEAP) HSColGroupStruct;
                group->oldHistid = histid;
                group->colCount = colCount;
                HS_ASSERT(reason == HS_REASON_EMPTY);
                group->newReason = HS_REASON_AUTO_INIT;
              }
          }

        // For any colPos, avoid rest of loop if this MC is not necessary.
        // This ignores all rows for the columns that make up the MC.
        // skipToNextHist will be assigned a new value on the next iteration
        // in which colPos==0, which marks the beginning of the set of rows
        // for the next MC.
        if (skipToNextHist)
          continue;

        // Add this column to the group.
        col = hs_globals->objDef->getColInfo(colNum); // colNum is position in table
        col.colnum = colNum;
        col.position = colPos;
        group->colSet.insert(col);
        columnName = hs_globals->objDef->getColName(colNum);
        *group->colNames += ToAnsiIdentifier(columnName);
        if (colPos < colCount - 1)
          *group->colNames += ", ";

        // Make sure that a single column group exists for this column.  If not,
        // create it.  This can occur if an MC histogram is empty, but the corresponding
        // single column histograms are not. Note that MC stats don't have a reason of
        // HS_REASON_SMALL_SAMPLE.
        if (hs_globals->findGroup(colNum) == NULL)
          {
            sgroup2 = new(STMTHEAP) HSColGroupStruct;
            if (sgroup)
              // Link single col groups so they can be added later. Note that
              // this is a partially-constructed link; we can only traverse
              // from the end of the list to the front.
              sgroup2->prev = sgroup;
            sgroup = sgroup2;
            col = hs_globals->objDef->getColInfo(colNum); // colNum = position in table
            col.colnum = colNum;
            col.position = 0;
            sgroup->colSet.insert(col);
            sgroup->oldHistid = 0;
            sgroup->colCount = 1;
            HS_ASSERT(reason == HS_REASON_EMPTY);
            group->newReason = HS_REASON_AUTO_INIT;
            *sgroup->colNames += ToAnsiIdentifier(columnName);
          }

        // If complete, check to see if it is a duplicate.  If not, add the group
        // to the multicolumn list of HSGlobalsClass and allocate a new group. 
        if (colPos == colCount - 1)
          {
            // Check to see if this is a duplicate group.
            prevGroup = hs_globals->findGroup(group);
            if (!prevGroup)
              {
                hs_globals->addGroup(group);
                while (sgroup) // Add the list of single col groups if necessary.
                  {
                    sgroup2 = sgroup->prev;
                    sgroup->prev = 0; // Zero out 'prev' before adding group.
                    hs_globals->addGroup(sgroup);
                    sgroup = sgroup2;
                  }
              }
            else
              {
                // Duplicate, save histid to remove it later.  Delete MC group.
                // No need to delete sgroup, none will be created for an MC duplicate.
                sprintf(tempStr, " %u,", histid);
                prevGroup->oldHistidList += tempStr;
                delete group;
              }
          }
      }

    // Preserve failure retcode from fetch for return value.
    if (retcode < 0)
      {
        errorCatcher.setString1(" in call to fetch()");
        errorCatcher.finalize();  // before call to close() erases CLI diags area
        necStmt.close();
        if (LM->LogNeeded())
          {
            sprintf(LM->msg, "AddNecessaryColumns: A fetch returned %d", retcode);
            LM->Log(LM->msg);
          }
      }
    else
      {
        retcode = necStmt.close();
        if (retcode < 0)
          {
            errorCatcher.setString1(" in call to close()");
            if (LM->LogNeeded())
              {
                sprintf(LM->msg, "AddNecessaryColumns: close() returned %d", retcode);
                LM->Log(LM->msg);
              }
          }
      }

    HSHandleError(retcode);

    // Use default sampling unless it was specified by the user.
    if (!(hs_globals->optFlags & SAMPLE_REQUESTED))
      {
        hs_globals->optFlags |= SAMPLE_BASIC_0;  // use default
        if (LM->LogNeeded())
          {
            // Preserve the text used in this log message; it is searched for
            // by certain ustat regression tests.
            LM->Log("Sampling for NECESSARY: default used");
          }
      }

    return retcode;
  }

/****************************************************************/
/* METHOD:  AddAllColumnsForIUS()                               */
/* PURPOSE: Read in all histograms for the table                */
/*                                                              */
/* RETCODE:  0 - successful                                     */
/*          -1 - failure                                        */
/* PARAMS:  none                                                */
/****************************************************************/
Lng32 AddAllColumnsForIUS()
  {
    HSLogMan *LM = HSLogMan::Instance();
    HSTranMan *TM = HSTranMan::Instance();
    HSGlobalsClass *hs_globals = GetHSContext();

    if (LM->LogNeeded())
      {
        LM->Log("Determining NECESSARY columns");
      }

    Lng32 retcode;
    HSColGroupStruct *group = NULL, *sgroup = NULL, *sgroup2 = NULL, *prevGroup = NULL;
    HSColumnStruct col;
    NAString columnName;
    char tempStr[30];
    Int64 objID = hs_globals->objDef->getObjectUID(); // TABLE_UID
    double samplePercent;               // Sample percent for a previously generated hist.
    // Columns read from Histograms table.
    ULng32 histid;
    Lng32 colPos;
    Lng32 colNum;
    Lng32 colCount;
    Int64 rowCount;
    short samplePercentX100;  // Stored value is sample % * 100.
    double cv;
    char reason;

    // Keep track of info on single-column groups for processing multi-column
    // groups; READ_TIME, etc. columns are not maintained for the MC groups.
    struct SingleColStatus
      {
        SingleColStatus() 
          : recentlyRead(FALSE), obsolete(FALSE)
          {}
        NABoolean recentlyRead;
        NABoolean obsolete;
      };

    SingleColStatus *singleCols 
          = new (STMTHEAP) SingleColStatus[hs_globals->objDef->getNumCols()+1];  // +1 bec. 0 not used

    char baseGMTTimeStr[HS_TIMESTAMP_SIZE];
    Int64 oldestSecs = hs_getBaseTime();                      // Get the base time 
    hs_formatTimestamp(oldestSecs, baseGMTTimeStr);            // Convert to GMT and timestamp string.

    // Start a transaction if one not already in progress. The query used to
    // get the current histograms would implicitly start one otherwise, and
    // we would finish the update stats statement still in a transaction that
    // the user did not start.
    //
    NABoolean startedTrans = ((TM->Begin("GET GROUP LIST FOR NECESSARY") == 0) ? TRUE : FALSE);

    HSCliStatement necStmt(HSCliStatement::CURSOR105_MX_2300,
                           (char *)hs_globals->hstogram_table->data(),
                           (char *)&objID,
                           (char *)&baseGMTTimeStr);

    retcode = necStmt.open();
    if (retcode < 0)
      {
        if (LM->LogNeeded())
          LM->Log("Failed to open HSCliStatement for CURSOR105_MX_2300");
        if (startedTrans)
          TM->Rollback();
        return retcode;
      }

    colCount = 1;                       // just to satisfy loop initially
    hs_globals->allMissingStats = TRUE; // Initialize flag.

    // First read through the single-column groups. The query for the cursor
    // we're using sorts by COLCOUNT, so the singles will come first. We need
    // their info before we can process the multicolumn groups.
    while (retcode == 0 && colCount == 1)
      {
        retcode = necStmt.fetch(8,
                               (void *)&histid, (void *)&colPos,
                               (void *)&colNum, (void *)&colCount,
                               (void *)&rowCount, (void *)&samplePercentX100,
                               (void *)&cv, (void *)&reason
                              );

        if (retcode || colCount > 1)  // end of data, error, or end of single-col groups
          break;
        samplePercent = ((float) samplePercentX100) / 100; // Stored value is % * 100.

        if (LM->LogNeeded())
          {
            sprintf(LM->msg, "\tFound column %s", hs_globals->objDef->getColName(colNum));
            LM->Log(LM->msg);
          }

        // If there is an existing histogram which is not obsolete, ignore the
        // current column.
        singleCols[colNum].recentlyRead = TRUE;  // query selects only these
        singleCols[colNum].obsolete = FALSE;

        if (reason != HS_REASON_EMPTY && reason != HS_REASON_SMALL_SAMPLE) // IUSR revisit
          {
            if (LM->LogNeeded())
              LM->Log("\t\t(ignored; existing histogram not empty, or small stats)");
            continue;  // don't keep this group
          }

        // Set up a group containing this column, add it to the HSGlobalsClass
        // single-col list, and allocate a new group for the next one.
        HS_ASSERT(colCount == 1); // some of the following code assumes this
        group = new(STMTHEAP) HSColGroupStruct;
        col = hs_globals->objDef->getColInfo(colNum); // colNum = position in table
        col.colnum = colNum;
        col.position = colPos;
        columnName = hs_globals->objDef->getColName(colNum);
        group->colSet.insert(col);
        group->oldHistid = histid;
        group->colCount = colCount;
        group->newReason = ((reason == HS_REASON_EMPTY || 
                             reason == HS_REASON_SMALL_SAMPLE) 
                                        ? HS_REASON_AUTO_INIT 
                                        : HS_REASON_AUTO_REGEN);
        *group->colNames += ToAnsiIdentifier(columnName);
        hs_globals->addGroup(group);
        prevGroup = group;

        // IUSR revisit
        // This flag (set to FALSE) is used to determine whether table rowcounts should be reset.
        hs_globals->allMissingStats = FALSE;
      }

    // Next read the multicolumn groups. Upon entry, the first column
    // of the first MC group has been read (unless retcode != 0).
    NABoolean skipToNextHist = FALSE;
    NABoolean mcEmpty=FALSE, mcObsolete=FALSE, mcSizeTooBig=FALSE;
    NABoolean rowReadByPreviousLoop = TRUE;
    while (retcode == 0)
      {
        // Get the next row, except on the first iteration when we already have
        // the one read by the loop above. Query returns rows ordered by
        // colcount/histid/colpos, so MC columns are read in correct order.
        if (rowReadByPreviousLoop)
          rowReadByPreviousLoop = FALSE;  // need to read a new one next time
        else
          {
            retcode = necStmt.fetch(8,
                                  (void *)&histid, (void *)&colPos,
                                  (void *)&colNum, (void *)&colCount,
                                  (void *)&rowCount, (void *)&samplePercentX100,
                                  (void *)&cv, (void *)&reason
                                  );
            if (retcode)  // end of data or error
              break;
          }


        // Add this column to the group.
        if (colPos == 0) 
          {
            group = new(STMTHEAP) HSColGroupStruct;
            group->oldHistid = histid;
            group->colCount = colCount;

            // IUSR revisit
            group->newReason = (reason == HS_REASON_EMPTY 
                                        ? HS_REASON_AUTO_INIT 
                                        : HS_REASON_AUTO_REGEN);
          }
        col = hs_globals->objDef->getColInfo(colNum); // colNum is position in table
        col.colnum = colNum;
        col.position = colPos;
        group->colSet.insert(col);
        columnName = hs_globals->objDef->getColName(colNum);
        *group->colNames += ToAnsiIdentifier(columnName);
        if (colPos < colCount - 1)
          *group->colNames += ", ";

        // Make sure that a single column group exists for this column.  If not,
        // create it.  This can occur if an MC histogram is empty, but the corresponding
        // single column histograms are not.  Note that MC stats don't have a reason of
        // HS_REASON_SMALL_SAMPLE.
        if (hs_globals->findGroup(colNum) == NULL)
          {
            sgroup2 = new(STMTHEAP) HSColGroupStruct;
            if (sgroup) sgroup2->prev = sgroup; // Link single col groups so they
                                                // can be added later.
            sgroup = sgroup2;
            col = hs_globals->objDef->getColInfo(colNum); // colNum = position in table
            col.colnum = colNum;
            col.position = 0;
            sgroup->colSet.insert(col);
            sgroup->oldHistid = 0;
            sgroup->colCount = 1;

            // IUSR revisit
            sgroup->newReason = (reason == HS_REASON_EMPTY 
                                        ? HS_REASON_AUTO_INIT 
                                        : HS_REASON_AUTO_REGEN);
            *sgroup->colNames += ToAnsiIdentifier(columnName);
          }

        // If complete, check to see if it is a duplicate.  If not, add the group
        // to the multicolumn list of HSGlobalsClass and allocate a new group. 
        if (colPos == colCount - 1)
          {
            // Check to see if this is a duplicate group. 
            if (!(prevGroup = hs_globals->findGroup(group)))
            {
              hs_globals->addGroup(group);
              while (sgroup) // Add the list of single col groups if necessary.
                {
                  sgroup2 = sgroup->prev;
                  sgroup->prev = 0; // Zero out 'prev' before adding group.
                  hs_globals->addGroup(sgroup);
                  sgroup = sgroup2;
                }
            }
            else {
              // Duplicate, save histid to remove it later.  Delete MC group.
              // No need to delete sgroup, none will be created for an MC duplicate.
              sprintf(tempStr, " %u,", histid);
              prevGroup->oldHistidList += tempStr;
              delete group;
            }
            mcEmpty = mcObsolete = mcSizeTooBig = FALSE;
          }
      }

    retcode = necStmt.close();

    if (startedTrans)
      TM->Commit();    // Just ends the transaction; no changes made

    // We will use as the sampling percentage the max we judge is needed for
    // any column, unless the smallest of those previously manually generated 
    // exceeds it. If there were no obsolete columns (only columns with no 
    // existing histogram), use the default sampling parameters.
    //

    return retcode;
  }


/**********************************************************************/
/* METHOD:  doubleToHSDataBuffer(const double dbl, HSDataBuffer& dbf) */
/* PURPOSE: Save a double precision number in HSDataBuffer            */
/* RETCODE:  0 - successful                                           */
/*          -1 - failure                                              */
/* INPUT:   double dbl: the number to save                            */
/*          HSDataBuffer dbf: where to save the number                */
/**********************************************************************/
    

Lng32 doubleToHSDataBuffer(const double dbl, HSDataBuffer& dbf)
  {
    char dvalue[SQL_DOUBLE_PRECISION_DISPLAY_SIZE+1]={0}; 
    char *ptr; 
    Lng32 retcode = 0;
    retcode = convFloat64ToAscii((char *)dvalue, 
      SQL_DOUBLE_PRECISION_DISPLAY_SIZE, dbl,
      SQL_DOUBLE_PRECISION_FRAG_DIGITS, NULL, 0, false);
    if (retcode != 0)
      return -1;
    ptr = dvalue + SQL_DOUBLE_PRECISION_DISPLAY_SIZE - 1;
    while (*ptr == ' ')
      ptr--;
    *(ptr+1) = '\0';

    static NAWchar val[SQL_DOUBLE_PRECISION_DISPLAY_SIZE+1];
    memset((char*)val, 0, sizeof(val));

    short s = strlen(dvalue);
    na_mbstowcs((NAWchar *)val, dvalue, s);
    dbf = (NAWchar *)val;
    return retcode;
  }

/**********************************************************************/
/* METHOD:  managePersistentSamples()                                 */
/* PURPOSE: Create or delete persistent sample tables from update     */
/*          statistics command line.                                  */
/* RETCODE:  0 - successful                                           */
/*          -1 - failure                                              */
/**********************************************************************/
Lng32 managePersistentSamples()
{
  HSLogMan *LM = HSLogMan::Instance();
  Lng32 retcode = 0;
  HSGlobalsClass *hs_globals = GetHSContext();
  if (!hs_globals) retcode = -1;
  else
  {
    // initialize stats schema if our object is a Hive or native HBase table
    retcode = hs_globals->InitializeStatsSchema();
    HSHandleError(retcode);

    NAString table;
    Int64 sampleRows, tableRows;
    NABoolean isEstimate = FALSE;

    Int32 errorCode = 0;
    Int32 breadCrumb = 0;
    tableRows = hs_globals->objDef->getRowCount(isEstimate, 
                                                errorCode /* out */,
                                                breadCrumb /* out */);
    if (errorCode)
      {
        *CmpCommon::diags() << DgSqlCode(-UERR_BAD_EST_ROWCOUNT) << DgInt0(errorCode) << DgInt1(breadCrumb);
        return -1;
      }

    // If all the user columns in the table are LOB columns, our sample table
    // would have zero columns. So return an error for that case.
    if (hs_globals->objDef->allUserColumnsAreLOBs())
      {
        HSFuncMergeDiags(- UERR_ALL_LOB_COLUMNS);
        return -1;
      }

    // tableRows could be zero for a Trafodion or HBase table if the table is new
    // and all the data is still in memstore. So, in the logic below we dance around
    // that, attempting to supply a not unreasonable guess for tableRows in that case.
    // If we don't do this, then we later get a sampling ratio of -nan which will
    // cause a syntax error when we formulate the sampling query.

    if (hs_globals->optFlags & SAMPLE_BASIC_1) 
      {
        sampleRows = hs_globals->sampleValue1;
        if (tableRows == 0)
          tableRows = sampleRows;  // just use the value the user gave 
      }
    else if (hs_globals->optFlags & SAMPLE_RAND_1) // sampleValue1 is % * HS_SAMP_PCNT_UPSCALE. */ 
      {
        if (tableRows == 0)
          tableRows = 10000;  // just use a made-up number
        sampleRows = (Int64)(((double)hs_globals->sampleValue1/(HS_SAMP_PCNT_UPSCALE*100)) * tableRows);
      }
    else // hs_globals->optFlags & SAMPLE_ALL
      {
        if (tableRows == 0)
          tableRows = 10000;  // just use a made-up number
        sampleRows = tableRows/100; // use default sample size and then match all sample w/ large diff.
      }

    //Return -1 with error msg if sample rows greater than base table rows.
    if (sampleRows > tableRows)
    {
      HSFuncMergeDiags(- UERR_INVALID_OPTION,
                       "CREATE SAMPLE ROWS",
                       "a value less than or equal to base table rows");
      return -1;
    }

    HSPersSamples *sampleList = HSPersSamples::Instance(hs_globals->objDef->getCatName(),
                                                        hs_globals->objDef->getSchemaName());
    if (!sampleList) retcode = -1;
    else
    {
      if (hs_globals->optFlags & CREATE_SAMPLE_OPT)  /* create sample requested*/
      {
        hs_globals->setHiveMaxStringLengthInBytes();
        if (sampleList->createAndInsert(hs_globals->objDef, table, 
                                        sampleRows, tableRows, 
                                        isEstimate,
                                        'M'))  // manually created persistent sample table
          retcode = -1;
        if (LM->LogNeeded())
          {
            char intStr[30];
            convertInt64ToAscii(sampleRows, intStr);
            sprintf(LM->msg, "Create persistent sample, %s, with %s rows.  Retcode=%d.",
                             table.data(), intStr, retcode); 
            LM->Log(LM->msg);
          }
        hs_globals->resetCQDs();
      }
      if (hs_globals->optFlags & REMOVE_SAMPLE_OPT)  /* remove sample requested*/
      {
        float allowedDiff = (float) CmpCommon::getDefaultNumeric(USTAT_SAMPLE_PERCENT_DIFF)/100;
        if (hs_globals->optFlags & SAMPLE_ALL) 
          allowedDiff = 1e10; // Set to large value to find ALL samples.
        sampleList->removeMatchingSamples(hs_globals->objDef, sampleRows, allowedDiff);
        if (LM->LogNeeded())
          {
            char intStr1[30], intStr2[30];
            convertInt64ToAscii(hs_globals->objDef->getObjectUID(), intStr1);
            convertInt64ToAscii(sampleRows, intStr2);
            sprintf(LM->msg, "Remove persistent samples for UID=%s, with %s rows and diff=%f",
                              intStr1, intStr2, allowedDiff); 
            LM->Log(LM->msg);
          }
      }
    }
  }
  return retcode;
}

NAString HSColGroupStruct::generateTextForColumnCast()
{
  NAString textForColumnCast = ", ";
  NAString columnName = "", dblQuote = "\"";

  // This applies to MC groups only.
  // Add NVL function to mark individual columns returning NULL values correctly,
  // otherwise the entire output as NULL
  NABoolean isMCGroup = FALSE;
  NAString nvlTextFirstPart = "NVL(", nvlTextLastPart = ", 'NULL')";
  NAString replaceFuncFirstPart = "REPLACE(", replaceFuncLastPart = ", '''', '''''')";

  NABoolean firstColumn = TRUE;
  const Lng32 maxCharBoundaryLen = (Lng32) CmpCommon::getDefaultNumeric(USTAT_MAX_CHAR_BOUNDARY_LEN);

  if (colCount > 1)
    isMCGroup = TRUE;

  for (Int32 i=0; i<colCount; i++)
  {
    HSColumnStruct &col = colSet[i];
    columnName = ToAnsiIdentifier(col.colname->data());

    // Surround column name with double quotes, if not already delimited.
    if (columnName.data()[0] != '"') 
      columnName=dblQuote+columnName+dblQuote;

    if(!firstColumn)
      textForColumnCast.append(" || ',' || ");

    NABoolean isACharColumn = DFS2REC::isAnyCharacter(col.datatype);

    if(isMCGroup)
    {
      if(isACharColumn)
        textForColumnCast.append(" '''' || ");
      textForColumnCast.append(nvlTextFirstPart);
    }

    //We must use TRANSLATE to convert non-unicode character strings
    //to unicode
    if (isACharColumn)
    {
      if(isMCGroup)
        textForColumnCast.append(replaceFuncFirstPart);

      NAString fromCS(CharInfo::getCharSetName(col.charset));
      HS_ASSERT(fromCS != SQLCHARSETSTRING_UNKNOWN);

      //10-040322-4394
      //We need to trim trailing blanks when the data is in its natural
      //character set form - NOT when it has been translated. Otherwise,
      //we may trim incorrect blanks.
      //
      //10-040224-3482
      //Since there is no direct translation between KANJI->UCS2 and
      //KCS5601->UCS2, we must first translate to ISO88591, then to UCS2
      switch (col.charset)
        {
          //KANJI/KCS::
          //  TRANSLATE(
          //    TRANSLATE(
          //      TRIM(TRAILING FROM
          //        SUBSTRING(<col>, 1, <#>)
          //      )
          //    USING <cs>TOISO88591)
          //  USING ISO88591TOUCS2)
          case CharInfo::KSC5601_MP:
          case CharInfo::KANJI_MP:
            {
              textForColumnCast.append("TRANSLATE(TRANSLATE(TRIM(TRAILING FROM SUBSTRING(");
              textForColumnCast.append(columnName.data());
              textForColumnCast.append(", 1, ");
              textForColumnCast.append(LongToNAString(maxCharBoundaryLen));
              textForColumnCast.append(")) USING ");
              textForColumnCast.append(fromCS.data());
              textForColumnCast.append("TOISO88591) USING ISO88591TOUCS2)");
              break;
            }
            
          //UNICODE:
          //    TRIM(TRAILING FROM
          //      SUBSTRING(<col>, 1, <#>)
          //    )
          case CharInfo::UNICODE:
            {
              textForColumnCast.append("TRIM(TRAILING FROM SUBSTRING(");
              textForColumnCast.append(columnName.data());
              textForColumnCast.append(", 1, ");
              textForColumnCast.append(LongToNAString(maxCharBoundaryLen));
              textForColumnCast.append(")) ");
              break;
            }

          //OTHER CHARACTER DATATYPES:
          //    TRANSLATE(
          //      TRIM(TRAILING FROM
          //        SUBSTRING(<col>, 1, #)
          //      )
          //    USING <cs>TOUCS2)
          default:
            {
              textForColumnCast.append("TRANSLATE(TRIM(TRAILING FROM SUBSTRING(");
              textForColumnCast.append(columnName.data());
              textForColumnCast.append(", 1, ");
              textForColumnCast.append(LongToNAString(maxCharBoundaryLen));
              textForColumnCast.append(")) USING ");
              textForColumnCast.append(fromCS.data());
              textForColumnCast.append("TOUCS2)");
              break;
            }
        }
      if(isMCGroup)
        textForColumnCast.append(replaceFuncLastPart);
    }
  else if (colSet[i].datatype == REC_BOOLEAN)
    {
      // CAST of boolean to VARCHAR UCS2 isn't supported in the 
      // engine yet (you get error 8414 at run-time if you try it),
      // so work around this by CASTing to ISO88591 then CASTing
      // to UCS2. Once the engine supports this cast we can
      // delete this code and just use the "else" case below.
      textForColumnCast.append("TRIM(TRAILING FROM CAST (CAST (");
      textForColumnCast.append(columnName.data());
      textForColumnCast.append(" AS CHAR(10)) AS VARCHAR(");
      textForColumnCast.append(LongToNAString(maxCharBoundaryLen));
      textForColumnCast.append(") CHARACTER SET UCS2))");
    }
  else
    {
      //CAST ALL OTHER DATATYPES TO UNICODE
      //      TRIM(TRAILING FROM
      //        CAST(<col> AS VARCHAR(#) CHARACTER SET UCS2)
      //      )
      textForColumnCast.append("TRIM(TRAILING FROM CAST(");
      textForColumnCast.append(columnName.data());
      textForColumnCast.append(" AS VARCHAR(");

      // for BIGNUM, increase the cast length
      // the largest possible length is 130, for example, in
      // 0.123456.... (1 leading zero + 1 decimal point + 128 precision)
      if (DFS2REC::isBigNum(colSet[i].datatype))
        textForColumnCast.append(LongToNAString(HS_MAX_UCS_BOUNDARY_CHAR));
      else
        textForColumnCast.append(LongToNAString(maxCharBoundaryLen));
      textForColumnCast.append(") CHARACTER SET UCS2))");
    }
    
    if(isMCGroup)
    {
      textForColumnCast.append(nvlTextLastPart);
      if(isACharColumn)
        textForColumnCast.append(" || ''''");
    }

    if(firstColumn)
      firstColumn = FALSE;
  }
  return textForColumnCast;
}

void HSInMemoryTable::generateSelectList(NAString& queryText)
{
  // Create query to get data for columns in PENDING state.
  HSColGroupStruct* group = columns_;

  NABoolean firstExpn = TRUE;
  do
    {
       if ( group->state == PENDING )
       {
          if (firstExpn)
            firstExpn = FALSE;
          else
            queryText.append(", ");
          queryText.append(group->ISSelectExpn);
       }
    }
  while (group = group->next);
}

void HSInMemoryTable::generateInsertSelectDQuery(
                    NAString& targetTable, NAString& smplTable, 
                    NAString& queryText)
{
  if (whereCondition_.length() == 0)
    return;

  // Produce the following string
  // insert into <smplTable>_D (
  // select * from <smplTable> where <whereCondition>
  //                         )
  //
  queryText.append("INSERT INTO ");

  queryText.append(targetTable);

  queryText.append(" (SELECT * ");

  queryText.append(" FROM ");

  queryText.append(smplTable.data());

  queryText.append(" WHERE ");
  queryText.append(whereCondition_);

  queryText.append(" )");
}


void HSInMemoryTable::generateSelectDQuery(NAString& smplTable, NAString& queryText)
{
  if (whereCondition_.length() == 0)
    return;

  // Produce the following string
  // select <selList> from <smplTable> where
  //
  queryText.append("SELECT ");

  NAString selectList;
  generateSelectList(selectList);
  queryText.append(selectList);

  queryText.append(" FROM ");

  queryText.append(smplTable.data());

  queryText.append(" WHERE ");
  queryText.append(whereCondition_);
  queryText.append(" FOR SKIP CONFLICT ACCESS");
}


void 
HSInMemoryTable::generateInsertSelectIQuery(NAString& targetTable, 
                                            NAString& sourceTable,
                                            NAString& queryText,
                                            NABoolean hasOversizedColumns,
                                            HSTableDef * objDef,
                                            Int64 futureSampleSize,
                                            Int64 currentSampleSize,
                                            Int64 sourceTableSize)
{
  if (whereCondition_.length() == 0)
    return;

  // Create query to get data for the desired columns.
  // 
  // upsert using load into <tmpTable> 
  //  (select <selList> from <sourceTable> where <whereCond> <sample>)
  //                            T 
  // 

  queryText.append("UPSERT USING LOAD INTO ");

  queryText.append(targetTable.data());

  queryText.append(" (SELECT ");

  // Generate the select list. Truncate any over-long char/varchar columns
  // by using SUBSTRING calls. Omit any LOB columns.
  objDef->addTruncatedSelectList(queryText);

  queryText.append(" FROM ");

  queryText.append(sourceTable.data());
  queryText.append(" WHERE ");
  queryText.append(whereCondition_);
  queryText.append(" ");

  NABoolean usePeriodic = 
    (CmpCommon::getDefault(USTAT_IUS_USE_PERIODIC_SAMPLING) == DF_ON);

  // First compute the sample rate as  
  //   currentSampleSize - deleteSetSize = remainingUndeleteRows
  //   futureSampleSize - remainingUndeleteRows = rowsToBeInserted
  //   new sample rate = rowsToBeInserted / sourceTableSize

  //Int64 remainingUndeleteRows = currentSampleSize - deleteSetSize;
  //Int64 rowsToBeInserted = futureSampleSize - remainingUndeleteRows;
  //Int64 newSampleRate = rowsToBeInserted / sourceTableSize;

  double newSampleRate = sampleRate_;
  
  if (newSampleRate > 0) {
     NAString sampleClause;
     if ( usePeriodic ) {
       //
       // periodic 1 rows in every x rows
       // Let m denote # of x-row sample set, in which 1 row will be picked.
       // m = newSampleRate * rows 
       // x * m = rows
       // x = rows / m = rows / (newSampleRate * rows) = 1/ newSampleRate
       //
       Int64 sv1 = 1;
       Int64 sv2 = (Int64)ceil((double)(1 / newSampleRate));

       if ( sv1 < sv2 ) 
         createSampleOption(SAMPLE_PERIODIC, newSampleRate * 100.0, 
                            sampleClause, sv1, sv2);
     } else {
       createSampleOption(SAMPLE_RAND_1, newSampleRate * 100.0, 
                          sampleClause, 0, 0);
     }
     queryText.append(sampleClause);
  }

  queryText.append(")");
}

void 
HSInMemoryTable::generateSelectIQuery(NAString& smplTable, 
                                      NAString& queryText)
{
  // Create query to get data for the desired columns.
  // 
  //  select <selList> from <sourceTable> 
  // 

  queryText.append("SELECT ");

  NAString selectList;
  generateSelectList(selectList);
  queryText.append(selectList);

  queryText.append(" FROM ");
  queryText.append(smplTable.data());

  queryText.append("_I FOR SKIP CONFLICT ACCESS");
}


// used by alg1
void 
HSInMemoryTable::generateInsertQuery(NAString& smplTable, NAString& sourceTable, 
                                     NAString& queryText, NABoolean addNoRollback)
{
  if (whereCondition_.length() == 0)
    return;

  // Create query to get data for the desired columns.
  // 
  // select * from (insert into <smplTbl> 
  //                 (select * from <targetTbl> where <whereCond> <sample>)
  //                           ) T 
  // 

  //queryText.append("SELECT * FROM (INSERT INTO ");

  if ( addNoRollback )
    queryText.append("INSERT WITH NO ROLLBACK INTO "); // for algorithm 1
  else
    queryText.append("INSERT INTO "); // for algorithm 1

  queryText.append(smplTable.data());

  queryText.append(" (SELECT * FROM ");

  queryText.append(sourceTable.data());
  queryText.append(" WHERE ");
  queryText.append(whereCondition_);
  queryText.append("  ");

  NABoolean usePeriodic = 
    (CmpCommon::getDefault(USTAT_IUS_USE_PERIODIC_SAMPLING) == DF_ON);

  if (sampleRate_ > 0)
    {
      NAString sampleClause;
      if ( usePeriodic ) 
        {
          //
          // periodic 1 rows in every x rows
          // Let m denote # of x-row sample set, in which 1 row will be picked.
          // m = sampleRate_ * rows 
          // x * m = rows
          // x = rows / m = rows / (sampleRate_ * rows) = 1/sampleRate_
          //
          Int64 sv1 = 1;
          Int64 sv2 = (Int64)ceil(1 / sampleRate_);

          if ( sv1 < sv2 ) 
            createSampleOption(SAMPLE_PERIODIC, sampleRate_ * 100.0, 
                                sampleClause, sv1, sv2);
        }
      else
        createSampleOption(SAMPLE_RAND_1, sampleRate_ * 100.0,
                           sampleClause, 0, 0);

      queryText.append(sampleClause);
    }

  queryText.append(")");
  //queryText.append(")) T");
}

Lng32 HSInMemoryTable::populate(NAString& queryText)
{
  HS_ASSERT(!isPopulated_);

  HSLogMan *LM = HSLogMan::Instance();
  Lng32 retcode = 0;
  Int64 rowsLeft;
  HSCursor popCursor;

  // the most likely error is on a prepare due to a bad WHERE clause
  // from the UPDATE STATS command itself; e.g. a syntax error or
  // perhaps a bad column reference due to a typo
  HSErrorCatcher errorCatcher(retcode, - UERR_IUS_BAD_WHERE_CLAUSE, 
                              "POPULATE_FROM_QUERY", TRUE);
  LM->Log("Preparing rowset...");
  // Allocate descriptors and statements for CLI and prepare rowset by
  // assigning location for results to be written.
  rowsLeft = rows_;

  HSFuncExecQuery("CONTROL QUERY DEFAULT ALLOW_DML_ON_NONAUDITED_TABLE 'ON'");



  // prepareRowset may do retries
  retcode = popCursor.prepareRowset(queryText.data(), FALSE, columns_,
       (Lng32)MINOF(MAX_ROWSET, rowsLeft));
  if (retcode < 0)
    {
      sprintf(LM->msg, "Error in prepareRowset for statement:\n%s", queryText.data());
      LM->Log(LM->msg);
      HSHandleError(retcode);
    }
  else
    retcode=0; // Set to 0 for warnings.
  LM->Log("...rowset prepared");
   
  LM->Log("fetching rowsets...");
  if (LM->LogNeeded())
    LM->StartTimer("Fetching rowsets");
  Int64 rowCount = 0;
  while (retcode >= 0          // allow warnings
         && retcode != HS_EOF  // exit if no more data
         && rowsLeft > 0)      // internal CLI error if 0 used for # rows to read
    {
      retcode = popCursor.fetchRowset();
      if (retcode == 0)  // 1 or more rows successfully read
        {
          rowCount += popCursor.rowsetSize();
          rowsLeft = rows_ - rowCount;
          retcode = HSGlobalsClass::processInternalSortNulls(popCursor.rowsetSize(), columns_);

          if ( retcode != 0 ) {

            HSFuncExecQuery("CONTROL QUERY DEFAULT ALLOW_DML_ON_NONAUDITED_TABLE reset");

            HSHandleError(retcode);
          }

          retcode = popCursor.setRowsetPointers(columns_,
                                                (Lng32)MINOF(MAX_ROWSET, rowsLeft));
        }
    }

  HSFuncExecQuery("CONTROL QUERY DEFAULT ALLOW_DML_ON_NONAUDITED_TABLE reset");

  if (retcode < 0) HSHandleError(retcode) else retcode=0; // Set to 0 for warnings.

  if (LM->LogNeeded())
    LM->StopTimer();

  LM->Log("...done fetching rowsets");
  isPopulated_ = TRUE;
  rows_ = rowCount;  // Actual number of rows read into memory
  return retcode;
}

extern HSColGroupStruct* 
AddSingleColumn(const Lng32, HSColGroupStruct*&, NABoolean prepend = TRUE);

void HSInMemoryTable::setUpColumns()
{
  HSGlobalsClass *hs_globals = GetHSContext();
  HSColGroupStruct* group = hs_globals->singleGroup;
  HSColGroupStruct* newGroup;
  while (group != NULL)
    {
      HSColumnStruct& col = group->colSet[0];
      newGroup = AddSingleColumn(col.colnum, columns_, FALSE);
      newGroup->state = UNPROCESSED;
      group = group->next;
    }

  // For each column get the C++ type used to store it internally, determine the
  // amount required memory required for all values of the column, then allocate
  // the memory.
  //@NOTE: Should check if internal sort types have already been mapped for the
  //       globals col list, and just copy type info if so.
  mapInternalSortTypes(columns_);
  HSGlobalsClass::getMemoryRequirements(columns_, rows_);
}

void HSInMemoryTable::logState(const char* title)
{
   // Create query to get data for columns in PENDING state.
  HSColGroupStruct* group = columns_;
  HSLogMan *LM = HSLogMan::Instance();

  sprintf(LM->msg, "%s, groups in PENDING state: ", title);
  LM->Log(LM->msg);
  do
    {
       if ( group->state == PENDING )
       {
         sprintf(LM->msg, "(%s, " PF64 ")", group->colSet[0].colname->data(), group->memNeeded);
         LM->Log(LM->msg);
       }
    }
  while (group = group->next);
  sprintf(LM->msg, " ");
  LM->Log(LM->msg);

}

//CollIndex HSGlobalsClass::selectFastStatsBatch(NAArray<HSColGroupStruct*>& colGroups)
CollIndex HSGlobalsClass::selectFastStatsBatch(HSColGroupStruct** colGroups)
{
  HSColGroupStruct* group = singleGroup;
  CollIndex groupCount = 0;
  while (group != NULL)
    {
      if (group->state == UNPROCESSED
          && !DFS2REC::isAnyCharacter(group->ISdatatype))  //@ZXbl -- temp restriction
        {
          //@ZXbl -- for now just return 1 column. Later, return as many as we
          //         have memory for.
          group->state = PENDING;
          colGroups[groupCount++] = group;
          break;    //@ZXbl -- for now, do 1 column at a time
        }
      group = group->next;
    }

  return groupCount;
}

//Lng32 HSGlobalsClass::processFastStatsBatch(CollIndex numCols, NAArray<HSColGroupStruct*> colGroups)
Lng32 HSGlobalsClass::processFastStatsBatch(CollIndex numCols, HSColGroupStruct** colGroups)
{
  Lng32 retcode = 0;
  HSCursor cursor;
  CollIndex i;
  HSColGroupStruct* group = NULL;
  HSLogMan *LM = HSLogMan::Instance();

  for (i=0; i<numCols; i++)
    {
      group = colGroups[i];

      //@ZXbl -- memory alloc may be moved later. Also needs to be able to recover
      //         from insufficient memory.
      if (!group->allocateISMemory(MAX_ROWSET,
                                   TRUE,        // alloc strdata if a char type
                                   FALSE))      // no recalc memneeded (IUS)
        {
          diagsArea << DgSqlCode(UERR_FASTSTATS_MEM_ALLOCATION_ERROR);
          retcode = -1;
          HSHandleError(retcode);
        }

      // setRowsetPointers() binds group->nextData to output. It is also used by
      // internal sort, which fetches all rowsets into memory before processing
      // any data.
      group->nextData = group->data;

      // This will be owned by the FastStatsHist object it is used to construct below.
      FastStatsCountingBloomFilter* cbf =
          new(STMTHEAP) FastStatsCountingBloomFilter(STMTHEAP, 5, sampleRowCount/2,
              .01, 255);

      switch (group->ISdatatype)
      {
        case REC_BIN8_SIGNED:
          group->fastStatsHist = new(STMTHEAP) FastStatsHist<Int8>(group, cbf);
          break;

        case REC_BOOLEAN:
        case REC_BIN8_UNSIGNED:
          group->fastStatsHist = new(STMTHEAP) FastStatsHist<UInt8>(group, cbf);
          break;

        case REC_BIN16_SIGNED:
          group->fastStatsHist = new(STMTHEAP) FastStatsHist<Int16>(group, cbf);
          break;

        case REC_BIN16_UNSIGNED:
          group->fastStatsHist = new(STMTHEAP) FastStatsHist<UInt16>(group, cbf);
          break;

        case REC_BIN32_SIGNED:
          group->fastStatsHist = new(STMTHEAP) FastStatsHist<Int32>(group, cbf);
          break;

        case REC_BIN32_UNSIGNED:
          group->fastStatsHist = new(STMTHEAP) FastStatsHist<UInt32>(group, cbf);
          break;

        case REC_BIN64_SIGNED:
          group->fastStatsHist = new(STMTHEAP) FastStatsHist<Int64>(group, cbf);
          break;

        case REC_BIN64_UNSIGNED:
          group->fastStatsHist = new(STMTHEAP) FastStatsHist<UInt64>(group, cbf);
          break;

        case REC_IEEE_FLOAT32:
          group->fastStatsHist = new(STMTHEAP) FastStatsHist<Float32>(group, cbf);
          break;

        case REC_IEEE_FLOAT64:
          group->fastStatsHist = new(STMTHEAP) FastStatsHist<Float64>(group, cbf);
          break;

        case REC_BYTE_F_ASCII:
        case REC_BYTE_F_DOUBLE:
        case REC_BINARY_STRING:
          //group->fastStatsHist = new(STMTHEAP) FastStatsHist<ISFixedChar*>(group, cbf);
          LM->Log("char types not yet supported for fast-stats");
          retcode=-1;
          HSHandleError(retcode);
          break;

        case REC_BYTE_V_ASCII:
        case REC_BYTE_V_DOUBLE:
        case REC_VARBINARY_STRING:
          //group->fastStatsHist = new(STMTHEAP) FastStatsHist<ISVarChar*>(group, cbf);
          LM->Log("char types not yet supported for fast-stats");
          retcode=-1;
          HSHandleError(retcode);
          break;

        default:
          sprintf(LM->msg, "processFastStatsBatch(): unknown type %d",
                           group->ISdatatype);
          LM->Log(LM->msg);
          retcode=-1;
          HSHandleError(retcode);
          break;
      }
    }

  retcode = prepareToReadColumnsIntoMem(&cursor, MAX_ROWSET);
  while (retcode >= 0          // allow warnings
         && retcode != HS_EOF) // exit if no more data
    {
      retcode = cursor.fetchRowset();
      if (retcode == 0)  // 1 or more rows successfully read
        {
          for (i=0; i<numCols; i++)
            {
              colGroups[i]->fastStatsHist->addRowset(cursor.rowsetSize());
            }
        }
    }

  cursor.close();

  // All the data is now represented in CBFs, so the buffers used to read the
  // data into can be freed.
  for (i=0; i<numCols; i++)
    {
      colGroups[i]->freeISMemory();
    }

  // Finish processing the histogram for each column and mark it as completed.
  for (i=0; i<numCols; i++)
    {
      group = colGroups[i];
      group->fastStatsHist->actuate(intCount);
      group->state = PROCESSED;
      delete group->fastStatsHist;
      group->fastStatsHist = NULL;
    }

  return retcode;
}

Lng32 HSGlobalsClass::CollectStatisticsWithFastStats()
{
  Lng32 retcode = 0;

  mapInternalSortTypes(singleGroup, TRUE);
  getMemoryRequirements(singleGroup, MAX_ROWSET);

  //NAArray<HSColGroupStruct*> colGroups(20); //singleGroupCount);
  HSColGroupStruct** colGroups;
  colGroups = new(STMTHEAP) HSColGroupStruct*[singleGroupCount];

  CollIndex numCols;
  do
  {
    numCols = selectFastStatsBatch(colGroups);
    if (numCols > 0)
      retcode = processFastStatsBatch(numCols, colGroups);
  } while (numCols > 0 && retcode >= 0);

  return retcode;
}

