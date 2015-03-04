/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2015 Hewlett-Packard Development Company, L.P.
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
**************************************************************************
*
* File:         NATable.C
* Description:  A Non-Alcoholic table
* Created:      4/27/94
* Language:     C++
*
*
**************************************************************************
*/

#define   SQLPARSERGLOBALS_FLAGS	// must precede all #include's

#undef  _DP2NT_
#define _DP2NT_
// #define NA_ARKFS
#define __ROSETTA
#undef _DP2NT_
// #undef NA_ARKFS
#undef __ROSETTA

#include "NATable.h"
#include "Sqlcomp.h"
#include "Const.h"
#include "desc.h"
#include "dfs2rec.h"
#include "hs_read.h"
#include "parser.h"
#include "BindWA.h"
#include "ComAnsiNamePart.h"
#include "ItemColRef.h"
#include "ItemFunc.h"
#include "ItemOther.h"
#include "PartFunc.h"
#include "EncodedValue.h"
#include "SchemaDB.h"
#include "NAClusterInfo.h"
#include "MVInfo.h"
#include "ComMPLoc.h"
#include "NATable.h"
#include "opt.h"
#include "CmpStatement.h"
#include "ControlDB.h"
#include "ComCextdecs.h"
#include "ComSysUtils.h"
#include "ComObjectName.h"
#include "SequenceGeneratorAttributes.h"
#include "security/uid.h"
#include "HDFSHook.h"
#include "ExpLOBexternal.h"
#include "ComCextdecs.h"
#include "ExpHbaseInterface.h"
#include "CmpSeabaseDDL.h"
#include "RelScan.h"
#include "exp_clause_derived.h"
#include "PrivMgrCommands.h"
#include "ComDistribution.h"
#include "ExExeUtilCli.h"
#include "CmpDescribe.h"
#include "Globals.h"
#include "ComUser.h"
#include "ComSmallDefs.h"
#include "CmpMain.h"

#define MAX_NODE_NAME 9

#include "SqlParserGlobals.h"

//#define __ROSETTA
//#include "rosetta_ddl_include.h"


#include "SqlParserGlobals.h"
extern desc_struct *generateSpecialDesc(const CorrName& corrName);

#include "CmpMemoryMonitor.h"

#include "OptimizerSimulator.h"

#include "SQLCLIdev.h"
#include "sql_id.h"
SQLMODULE_ID __SQL_mod_natable = {
  /* version */         SQLCLI_CURRENT_VERSION,
  /* module name */     "HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.READDEF_N29_000",
  /* time stamp */      866668761818000LL,
  /* char set */        "ISO88591",
  /* name length */     47 
};

// -----------------------------------------------------------------------
// skipLeadingBlanks()
// Examines the given string keyValueBuffer from 'startIndex' for
// 'length' bytes and skips any blanks that appear as a prefix of the
// first non-blank character.
// -----------------------------------------------------------------------

Int64 HistogramsCacheEntry::getLastUpdateStatsTime()
{
  return cmpCurrentContext->getLastUpdateStatsTime();
}

void HistogramsCacheEntry::setUpdateStatsTime(Int64 updateTime)
{
  cmpCurrentContext->setLastUpdateStatsTime(updateTime);
}

static Int64 getCurrentTime()
{
  // GETTIMEOFDAY returns -1, in case of an error
  Int64 currentTime;
  TimeVal currTime;
  if (GETTIMEOFDAY(&currTime, 0) != -1)
    currentTime = currTime.tv_sec;
  else
    currentTime = 0;
  return currentTime;
}

void HistogramsCacheEntry::updateRefreshTime()
{
  Int64 currentTime = getCurrentTime();
  this->setRefreshTime(currentTime);
}

static Lng32 skipLeadingBlanks(const char * keyValueBuffer,
			      const Lng32 startIndex,
			      const Lng32 length)
{
  Lng32 newIndex = startIndex;
  Lng32 stopIndex = newIndex + length;
  // Localize the search for blanks between the startIndex and stopIndex.
  while ((newIndex <= stopIndex) AND (keyValueBuffer[newIndex] == ' '))
    newIndex++;
  return newIndex;
} // static skipLeadingBlanks()

// -----------------------------------------------------------------------
// skipTrailingBlanks()
// Examines the given string keyValueBuffer from startIndex down through
// 0 and skips any blanks that appear as a suffix of the first non-blank
// character.
// -----------------------------------------------------------------------
static Lng32 skipTrailingBlanks(const char * keyValueBuffer,
			       const Lng32 startIndex)
{
  Lng32 newIndex = startIndex;
  while ((newIndex >= 0) AND (keyValueBuffer[newIndex] == ' '))
    newIndex--;
  return newIndex;
} // static skipTrailingBlanks

//----------------------------------------------------------------------
// qualNameHashFunc()
// calculates a hash value given a QualifiedName.Hash value is mod by
// the hashTable size in HashDictionary.
//----------------------------------------------------------------------
ULng32 qualNameHashFunc(const QualifiedName& qualName)
{
  ULng32 index = 0;
  const NAString& name = qualName.getObjectName();
  
  for(UInt32 i=0;i<name.length();i++)
    {
      index += (ULng32) (name[i]);
    }
  return index;
}

//-------------------------------------------------------------------------
//constructor() for HistogramCache
//-------------------------------------------------------------------------
HistogramCache::HistogramCache(NAMemory * heap,Lng32 initSize)
: heap_(heap),
  hits_(0),
  lookups_(0),
  memoryLimit_(33554432),
  lruQ_(heap), tfd_(NULL), mfd_(NULL), size_(0)
{
	//create the actual cache
  HashFunctionPtr hashFunc = (HashFunctionPtr)(&qualNameHashFunc);
  histogramsCache_ = new (heap_) 
    NAHashDictionary<QualifiedName,HistogramsCacheEntry>
    (hashFunc,initSize,TRUE,heap_);
}

//reset all entries to not accessedInCurrentStatement
void HistogramCache::resetAfterStatement()
{
  for (CollIndex x=lruQ_.entries(); x>0; x--)
  {
    if (lruQ_[x-1]->accessedInCurrentStatement())
      lruQ_[x-1]->resetAfterStatement();
  }
}

//-------------------------------------------------------------------------
//invalidate what is in the cache
//-------------------------------------------------------------------------
void HistogramCache::invalidateCache()
{
  while (lruQ_.entries())
  {
    HistogramsCacheEntry* entry = lruQ_[0];
    deCache(&entry);
  }
  histogramsCache_->clearAndDestroy();
  lruQ_.clear();
}

//--------------------------------------------------------------------------
// HistogramCache::getCachedHistogram()
// Looks for the histogram in the cache if it is there then makes a deep copy
// of it on the statementHeap() and returns it. If the histogram is not in
// the cache then it fetches the histogram and makes a deep copy of it on the
// context heap to store it in the hash table.
//--------------------------------------------------------------------------

#pragma nowarn(770)   // warning elimination
void HistogramCache::getHistograms(NATable& table)
{
  const QualifiedName& qualifiedName = table.getFullyQualifiedGuardianName();
  ExtendedQualName::SpecialTableType type = table.getTableType();
  const NAColumnArray& colArray = table.getNAColumnArray();
  StatsList& colStatsList = *(table.getColStats());
  const Int64& redefTime = table.getRedefTime();
  Int64& statsTime = const_cast<Int64&>(table.getStatsTime());

  //1//
  //This 'flag' is set to NULL if FetchHistogram has to be called to
  //get the statistics in case
  //1. If a table's histograms are not in the cache
  //2. If some kind of timestamp mismatch occurs and therefore the
  //   cached histogram has to be refreshed from disk.
  //Pointer to cache entry for histograms on this table
  HistogramsCacheEntry * cachedHistograms = NULL;

  // skip reading the histograms if they have not been changed in last
  // CACHE_HISTOGRAMS_REFRESH_INTERVAL hours
  NABoolean skipRead = FALSE;

  //Do we need to use the cache
  //Depends on :
  //1. If histogram caching is ON
  //2. If the table is a normal table
  if(CURRSTMT_OPTDEFAULTS->cacheHistograms() &&
    type == ExtendedQualName::NORMAL_TABLE)
  { //2//
    // Do we have cached histograms for this table
    // look up the cache and get a reference to statistics for this table
    cachedHistograms = lookUp(table);

    // first thing to check is, if the table to which the histograms are cached
    // has been updated

    if (cachedHistograms && (cachedHistograms->getRedefTime() != redefTime))
    {
      deCache(&cachedHistograms);
    }
    // If the histograms exist in the cache, then we want to avoid reading
    // timestamps, if the histograms have not been updated in last default
    // refresh time (CACHE_HISTOGRAMS_REFRESH_INTERVAL) or if the histograms in the cache
    // are less than CACHE_HISTOGRAMS_REFRESH_INTERVAL old.

    Int64 lastRefTimeDef, lastFakeRefTimeDef, currentTime;
    if (cachedHistograms)
    {
      lastRefTimeDef = uint32ToInt64(CURRSTMT_OPTDEFAULTS->defRefTime());
      lastFakeRefTimeDef = uint32ToInt64(CURRSTMT_OPTDEFAULTS->defFakeRefTime());

      currentTime = getCurrentTime();

      Int64 histLastRefreshedTime = cachedHistograms->getRefreshTime();

      if (currentTime && cachedHistograms->isAllStatsFake())
      {
        // Check if it has been more than 'lastFakeRefTimeDef' secs 
        // (equal to CQD HIST_NO_STATS_REFRESH_INTERVAL) since histograms have
        // been checked OR if update statistics automation is ON and it has
        // been more than 'lastFakeRefTimeDef'/360 (should = 10 by default).
        Int64 timeSinceLastHistRefresh = currentTime - histLastRefreshedTime;
        if(!CURRSTMT_OPTDEFAULTS->ustatAutomation() && timeSinceLastHistRefresh > lastFakeRefTimeDef ||
            CURRSTMT_OPTDEFAULTS->ustatAutomation() && timeSinceLastHistRefresh > lastFakeRefTimeDef/360)
        {
          //the histograms are in the cache but we need to re-read them because
          //their default values might have been re-estimated
          deCache(&cachedHistograms);
        }
      }

      // Histograms are not fake. Check to see if we need to do anymore timestamp checks

      if (currentTime && cachedHistograms && lastRefTimeDef > 0)
      {
        Int64 lastUpdateStatsTime = HistogramsCacheEntry::getLastUpdateStatsTime();

        if ((lastUpdateStatsTime != -1) && 
            ((currentTime - lastUpdateStatsTime) < lastRefTimeDef))
        {
          // Last known update stats time for this table occurred less than
          // CACHE_HISTOGRAMS_REFRESH_INTERVAL secs ago.
          if (lastUpdateStatsTime < histLastRefreshedTime)
          {
            // Last time the histograms cache was refreshed for this table is newer
            // than last known update stats time.  Skip read of hists.
            skipRead = TRUE;
          }
        }
        else
          // No update stats time recorded OR last known update stats time occurred
          // more than CACHE_HISTOGRAMS_REFRESH_INTERVAL secs ago.
          if ((currentTime - histLastRefreshedTime) < lastRefTimeDef)
            // Histograms were refreshed less than CACHE_REFRESH_HISTOGRAMS_INTERVAL
            // secs ago.  Skip read of hists.
            skipRead = TRUE;
      }
    }

    //assumption:
    //if tempHist is not NULL then it should have a pointer to full Histograms

    //check if histogram preFetching is on
    if(CURRSTMT_OPTDEFAULTS->preFetchHistograms() && cachedHistograms)
    { //3//
      //we do need to preFetch histograms
      if(!cachedHistograms->preFetched())
      { //4//
        //preFetching is on but these histograms
        //were not preFetched so delete them and
        //re-Read them
        deCache(&cachedHistograms);
      } //4//
    } //3//

    //Check if there is a timestamp mis-match
    if(cachedHistograms AND cachedHistograms->getRedefTime() != redefTime)
    { //5//
       //the histograms are in the cache but we need to re-read them because of
       //a time stamp mismatch
       deCache(&cachedHistograms);
    } //5//
    else if (!skipRead)
      { //6//
        //Do some more timestamp calculations and set re-Read flag if
        //there is a mis-match
        if(cachedHistograms)
        { //9 //
          // Check when the histogram table was last modified.  If this time doesn't equal
          // the modification time of the cached histograms, OR this time is more than
          // lastRefTimeDef secs old, call FetchStatsTime to read STATS_TIME field of
          // the actual histogram.  The last condition here is used to force a call of
          // FetchStatsTime() after awhile.  This is for update stats automation:
          // FetchStatsTime() will update the READ_TIME field of the histogram.
          Int64 modifTime;
          Int64 currentJulianTime = NA_JulianTimestamp();
          GetHSModifyTime(qualifiedName, type, modifTime, FALSE);
          Int64 readCntInterval = (Int64)CmpCommon::getDefaultLong(USTAT_AUTO_READTIME_UPDATE_INTERVAL);
          if (modifTime != 0)
            // If the HISTOGRAMS table was modified since the last time FetchStatsTime()
            // called and the time is not the same as the cached histograms OR
            // if it was modified more than READTIME_UPDATE_INTERVAL secs ago and
            // ustat automation is ON:
            if (cachedHistograms->getModifTime() != modifTime ||
                (currentJulianTime - modifTime > readCntInterval*1000000 &&
                 CmpCommon::getDefaultLong(USTAT_AUTOMATION_INTERVAL) > 0))
          { //10//
            FetchStatsTime(qualifiedName,type,colArray,statsTime,FALSE);
            cachedHistograms->updateRefreshTime();
            // If ustat automation is on, FetchStatsTime will modify the HISTOGRAMS table.
            // So, the new modification time of the HISTOGRAMS table must be saved to the
            // cached histograms when automation is on, so that only changes to HISTOGRAMS 
            // by update stats cause the above 'if' to be TRUE.
            if (CmpCommon::getDefaultLong(USTAT_AUTOMATION_INTERVAL) > 0)
            {
              GetHSModifyTime(qualifiedName, type, modifTime, FALSE);
              cachedHistograms->setModifTime(modifTime);
            }

            if (cachedHistograms->getStatsTime() != statsTime)
            { //11//
              deCache(&cachedHistograms);
            } //11//
          } //10//
        } //9//
      } //6//
    } //2//

    if( cachedHistograms )
    {
      hits_++;
    }
    else
    {
      lookups_++;
    }

    //retrieve the statistics for the table in colStatsList
    createColStatsList(table, cachedHistograms);

    //if not using histogram cache, then invalidate cache
    if(!CURRSTMT_OPTDEFAULTS->cacheHistograms())
      invalidateCache();
} //1//
#pragma warn(770)  // warning elimination
  
//----------------------------------------------------------------------------
// HistogramCache::createColStatsList()
// This method actually puts the statistics for columns that require statistics
// into colStatsList.
// 1. If reRead is false meaning that the table's statistics exist in the cache,
//    then this method gets statistics from the cache and copies them into
//    colStatsList. If statistics for some columns are not found in the cache, then
//    this method calls FetchHistograms to get statistics for these columns. It
//    then puts these missing statistics into the cache, then copies the statistics
//    from the cache into colStatsList
// 2. If reRead is true meaning that we need to get statistics from disk via
//    FetchHistograms. reRead can be true for any of the following cases:
//    a. Histogram Caching is on but we updated statistics since we last read them
//       so we have deleted the old statistics and we need to read the tables
//       statistics again from disk.
// 3. If histograms are being Fetched on demand meaning that histogram caching is off,
//    then this method will fetch statistics into colStatsList using FetchHistograms.
//
// Now that we also have the option of reducing the number of intervals in histograms
// this method also factors that in.
//
// Each entry of the colArray contains information about a column that tells
// us what kind of histograms is required by that colum. The decision on what
// kind of a histograms is required for a column is base on the following factors
//
// 1. A column that is not referenced and neither is a index/primary key does
//    not need histogram
//
// 2. Column that is a index/primary key or is referenced in the query but not part
//    of a predicate or groupby or orderby clause requires compressed histogram.
//    A full histogram can be altered to make it seem like a compressed histogram.
//
// 3. Columns that are part of a predicate or are in orderby or groupby clause requires
//    full histogram are referencedForHistogram. A full histogram can only satisfy
//    the requirement for a full histogram.
//
// Just to for the sake of reitirating the main point:
// Columns that are referencedForHistogram needs full histogram
// Columns that are just referenced or is a index/primary key only requires a
// compressed histogram
//----------------------------------------------------------------------------
void HistogramCache::createColStatsList
(NATable& table, HistogramsCacheEntry* cachedHistograms)
{
  StatsList& colStatsList = *(table.getColStats());

  NAColumnArray& colArray = const_cast<NAColumnArray&>
    (table.getNAColumnArray());
  const QualifiedName& qualifiedName = table.getFullyQualifiedGuardianName();
  ExtendedQualName::SpecialTableType type = table.getTableType();
  const Int64& redefTime = table.getRedefTime();
  Int64& statsTime = const_cast<Int64&>(table.getStatsTime());

  // The singleColsFound is used to prevent stats from being inserted
  // more than once in the output list.
  ColumnSet singleColsFound(STMTHEAP);

  //"lean" cachedHistograms/are in the context heap.
  //colStatsList is in the statement heap. 
  //The context heap persists for the life of this mxcmp.
  //The statement heap is deleted at end of a compilation.
  //getStatsListFromCache will expand "lean" cachedHistograms
  //into "fat" colStatsList.

  //this points to the stats list
  //that is used to fetch statistics
  //that are not in the cache
  StatsList * statsListForFetch=NULL;

  // Used to count the number of columns
  // whose histograms are in the cache.
  UInt32 coveredList = 0;

  //Do we need to use the cache
  //Depends on :
  //1. If histogram caching is ON
  //2. If the table is a normal table
  if(cachedHistograms && (CURRSTMT_OPTDEFAULTS->cacheHistograms() &&
		          type == ExtendedQualName::NORMAL_TABLE))
  {
    // getStatsListFromCache will unmark columns that have statistics 
    // in cachedHistograms. All columns whose statistics are not in
    // cachedHistogram are still marked as needing histograms. 
    // This is then passed into FetchHistograms, which will
    // return statistics for columns marked as needing histograms. 
    // colArray tells getStatsListFromCache what columns need 
    // histograms. getStatsListFromCache uses colArray to tell
    // us what columns were not found in cachedHistograms.

    // get statistics from cachedHistograms into list.
    // colArray has the columns whose histograms we need.
    coveredList = getStatsListFromCache
      (colStatsList, colArray, cachedHistograms, singleColsFound);
   }

  Int64 modifTime = 0;

  // set to TRUE if all columns in the table have default statistics
  NABoolean allFakeStats = TRUE;

  //if some of the needed statistics were not found in the cache
  //then call FetchHistograms to get those statistics
  if (colArray.entries() > coveredList)
  {
     //this is the stats list into which statistics will be fetched
     statsListForFetch = &colStatsList;

     if(CURRSTMT_OPTDEFAULTS->cacheHistograms() &&
        type == ExtendedQualName::NORMAL_TABLE)
     {
       //if histogram caching is on and not all histograms where found in the cache
       //then create a new stats list object to get histograms that were missing
       statsListForFetch = new(CmpCommon::statementHeap()) 
               StatsList(CmpCommon::statementHeap(),2*colArray.entries());
     }

     //set pre-fetching to false by default
     NABoolean preFetch = FALSE;

     //turn prefetching on if caching is on and
     //we want to prefetch histograms
     if(CURRSTMT_OPTDEFAULTS->cacheHistograms() &&
        CURRSTMT_OPTDEFAULTS->preFetchHistograms() &&
		   (type == ExtendedQualName::NORMAL_TABLE))
      preFetch = TRUE;

     // flag the unique columns so the uec can be set correctly
     // specially in the case of columns with fake stats
     for (CollIndex j = 0; j < colArray.entries(); j++)
     {
        NAList<NAString> keyColList(STMTHEAP, 1);
        NAColumn *col = colArray[j];
        if (!col->isUnique())
        {
           const NAString &colName = col->getColName();
           keyColList.insert(colName);
 
           // is there a unique index on this column?
           if (col->needHistogram () && 
               table.getCorrespondingIndex(keyColList,  // input columns
                                           TRUE,        // look for explicit index
                                           TRUE,        // look for unique index
                                           FALSE,       // look for primary key
                                           FALSE,       // look for any index or primary key
                                           NULL         // index name
                                           ))
              col->setIsUnique(); 
        }
     }

     FetchHistograms(qualifiedName,
                     type,
                    (colArray),
                    (*statsListForFetch),
                     FALSE,
                    CmpCommon::statementHeap(),
                    modifTime,
                    statsTime,
                    allFakeStats,//set to TRUE if all columns have default stats
                    preFetch,
                    (Int64) CURRSTMT_OPTDEFAULTS->histDefaultSampleSize()
                    );   
     
  }

  //check if we are using the cache
  if(CURRSTMT_OPTDEFAULTS->cacheHistograms() &&
     type == ExtendedQualName::NORMAL_TABLE)
  {
      //we are using the cache but did we already
      //have the statistics in cache
      if(cachedHistograms)
      {
        // yes some of the statistics where already in cache
        // Did we find statistics in the cache for all the columns
        // whose statistics we needed?
        if (colArray.entries() > coveredList)
        {
          // not all the required statistics were in the cache,
          // some statistics were missing from the cache entry.
          // therefore must have done a FetchHistograms to get
          // the missing histograms. Now update the cache entry
          // by adding the missing histograms that were just fetched
          ULng32 histCacheHeapSize = heap_->getAllocSize();
          cachedHistograms->addToCachedEntry(colArray,(*statsListForFetch));
          ULng32 entrySizeGrowth = (heap_->getAllocSize() - histCacheHeapSize);
          ULng32 entrySize = cachedHistograms->getSize() + entrySizeGrowth;
          cachedHistograms->setSize(entrySize);
          size_ += entrySizeGrowth;
  
          //get statistics from the cache that where missing from the 
          //cache earlier and have since been added to the cache
          coveredList = getStatsListFromCache
            (colStatsList, colArray, cachedHistograms, singleColsFound);
        }
      }
      else
      {
        CMPASSERT(statsListForFetch);

        // used the cache but had to re-read
        // all the table's histograms from disk
  
        // put the re-read histograms into cache
        putStatsListIntoCache((*statsListForFetch), colArray, qualifiedName,
                             modifTime, statsTime, redefTime, allFakeStats);
  
        // look up the cache and get a reference to statistics for this table
        cachedHistograms = lookUp(table);


        // get statistics from the cache
        coveredList = getStatsListFromCache
          (colStatsList, colArray, cachedHistograms, singleColsFound);
      }
  }


  if(CURRSTMT_OPTDEFAULTS->reduceBaseHistograms())
    colStatsList.reduceNumHistIntsAfterFetch(table);

    //clean up
	if(statsListForFetch != &colStatsList)
		delete statsListForFetch;

  // try to decache any old entries if we're over the memory limit
  if(CURRSTMT_OPTDEFAULTS->cacheHistograms())
  {
    enforceMemorySpaceConstraints();
  }
  traceTable(table);
}

//------------------------------------------------------------------------
//HistogramCache::getStatsListFromCache()
//gets the StatsList into list from cachedHistograms and
//returns the number of columns whose statistics were
//found in the cache. The columns whose statistics are required
//are passed in through colArray. 
//------------------------------------------------------------------------
#pragma nowarn(1506)   // warning elimination
Int32 HistogramCache::getStatsListFromCache
( StatsList&            list, //In \ Out
  NAColumnArray&        colArray, //In
  HistogramsCacheEntry* cachedHistograms, // In
  ColumnSet&            singleColsFound) //In \ Out
{
  // cachedHistograms points to the memory-efficient contextheap 
  // representation of table's histograms. 
  // list points to statementheap list container that caller is 
  // expecting us to fill-in with ColStats required by colArray.

  // counts columns whose histograms are in cache or not needed
  UInt32 columnsCovered = 0;

  // Collect the mc stats with this temporary list. If the 
  // mc stats objects are stored in the middle of the output 'list', 
  // IndexDescHistograms::appendHistogramForColumnPosition() will
  // abort, because "There must be a ColStatDesc for every key column!".
  StatsList mcStatsList(CmpCommon::statementHeap());

    //iterate over all the columns in the colArray
  for(UInt32 i=0;i<colArray.entries();i++)
	{
		//get a reference to the column
    NAColumn * column = colArray[i];

		//get the position of the column in the table
    CollIndex colPos = column->getPosition();

    // singleColsFound is used to prevent stats from
    // being inserted more than once in the output list.
    if (singleColsFound.contains(colPos))
    {
      columnsCovered++;
      continue;
    }

    NABoolean columnNeedsHist = column->needHistogram();
    NABoolean columnNeedsFullHist = column->needFullHistogram();

    // Did histograms for this column get added
    NABoolean colAdded = FALSE;

    if (NOT columnNeedsHist)
    {
      //if the column was marked as not needing any histogram
      //then increment columnsCovered & skip to next column, as neither 
      //single interval nor full histograms are required for this column.
      columnsCovered++;
    }
    else if (cachedHistograms->contains(colPos) AND columnNeedsHist)
		{
			//we have full histograms for this column
      columnsCovered++;
      colAdded = TRUE;

      //set flag in column not to fetch histogram
      //the histogram is already in cache
      column->setDontNeedHistogram();

      NABoolean copyIntervals=TRUE;
      ColStatsSharedPtr const singleColStats =
        cachedHistograms->getHistForCol(*column);
      if (NOT columnNeedsFullHist)
      {
        //full histograms are not required. get single interval histogram
        //from the full histogram and insert it into the user's statslist 
        copyIntervals=FALSE;
      }
      //since we've tested containment, we are guaranteed to get a
      //non-null histogram for column
      list.insertAt
        (list.entries(), 
         ColStats::deepCopySingleColHistFromCache
         (*singleColStats, *column, list.heap(), copyIntervals));
    }
    //Assumption: a multi-column histogram is retrieved when 
    //histograms for any of its columns are retrieved.
    if (columnNeedsHist)
    {
      // insert all multicolumns referencing column
      // use singleColsFound to avoid duplicates
      cachedHistograms->getMCStatsForColFromCacheIntoList
        (mcStatsList, *column, singleColsFound);
    }
	
    // if column was added, then add it to the duplist
    if (colAdded) singleColsFound += colPos;
  }

  // append the mc stats at the end of the output lit.
  for (Lng32 i=0; i<mcStatsList.entries(); i++ ) {
     list.insertAt(list.entries(), mcStatsList[i]);
  }

  return columnsCovered;
}
#pragma warn(1506)  // warning elimination

//this method is used to put into the cache stats lists, that
//needed to be re-read or were not there in the cache
void HistogramCache::putStatsListIntoCache(StatsList & colStatsList,
                                          const NAColumnArray& colArray,
                                          const QualifiedName & qualifiedName,
                                          Int64 modifTime,
                                          Int64 statsTime,
                                          const Int64 & redefTime,
					  NABoolean allFakeStats)
{
  ULng32 histCacheHeapSize = heap_->getAllocSize();
  // create memory efficient representation of colStatsList
  HistogramsCacheEntry * histogramsForCache = new (heap_) 
    HistogramsCacheEntry(colStatsList, qualifiedName, 
                         modifTime, statsTime, redefTime, heap_);
  ULng32 cacheEntrySize = heap_->getAllocSize() - histCacheHeapSize;

  if(CmpCommon::getDefault(CACHE_HISTOGRAMS_CHECK_FOR_LEAKS) == DF_ON)
  {
    delete histogramsForCache;
    ULng32 histCacheHeapSize2 = heap_->getAllocSize();
    CMPASSERT( histCacheHeapSize == histCacheHeapSize2);
    histogramsForCache = new (heap_) 
      HistogramsCacheEntry(colStatsList, qualifiedName, 
                           modifTime, statsTime, redefTime, heap_);
    cacheEntrySize = heap_->getAllocSize() - histCacheHeapSize2;
  }
  histogramsForCache->setSize(cacheEntrySize);

  if(FALSE)
  {
    delete histogramsForCache;
    histogramsForCache = new (heap_) 
    HistogramsCacheEntry(colStatsList, qualifiedName, 
                         modifTime, statsTime, redefTime, heap_);
  }
  // add it to the cache 
  QualifiedName* key = const_cast<QualifiedName*>
    (histogramsForCache->getName());
  QualifiedName *name = histogramsCache_->insert(key, histogramsForCache);
  if (name)
  {
    // append it to least recently used queue
    lruQ_.insertAt(lruQ_.entries(), histogramsForCache);
  }
  size_ += cacheEntrySize;
}

// if we're above memoryLimit_, try to decache
NABoolean HistogramCache::enforceMemorySpaceConstraints()
{
  if (size_ <= memoryLimit_)
    return TRUE;

  HistogramsCacheEntry* entry = NULL;
  while (lruQ_.entries())
  {
    entry = lruQ_[0];
    if (entry->accessedInCurrentStatement())
      return FALSE;
    deCache(&entry);
    if (size_ <= memoryLimit_)
      return TRUE;
  }
  return FALSE;
}

// lookup given table's histograms.
// if found, return its HistogramsCacheEntry*.
// otherwise, return NULL.
HistogramsCacheEntry* HistogramCache::lookUp(NATable& table)
{
  const QualifiedName& tblNam = table.getFullyQualifiedGuardianName();
  HistogramsCacheEntry* hcEntry = NULL;
  if (histogramsCache_) 
  {
    // lookup given table's lean histogram cache entry
    hcEntry = histogramsCache_->getFirstValue(&tblNam);
    if (hcEntry)
    {
      // move entry to tail of least recently used queue
      lruQ_.remove(hcEntry);
      lruQ_.insertAt(lruQ_.entries(), hcEntry);
    }
  }
  return hcEntry;
}

// decache entry
void HistogramCache::deCache(HistogramsCacheEntry** entry)
{
  if (entry && (*entry))
  {
    ULng32 entrySize = (*entry)->getSize();
    histogramsCache_->remove(const_cast<QualifiedName*>((*entry)->getName()));
    lruQ_.remove(*entry);
    ULng32 heapSizeBeforeDelete = heap_->getAllocSize();
    delete (*entry);
    ULng32 memReclaimed = heapSizeBeforeDelete - heap_->getAllocSize();
    if(CmpCommon::getDefault(CACHE_HISTOGRAMS_CHECK_FOR_LEAKS) == DF_ON)
      CMPASSERT( memReclaimed >= entrySize );
    *entry = NULL;
    size_ -= entrySize;
  }
}


void HistogramCache::resizeCache(size_t limit)
{
  memoryLimit_ = limit;
  enforceMemorySpaceConstraints();
}

ULng32 HistogramCache::entries() const
{
  return histogramsCache_ ? histogramsCache_->entries() : 0;
}

void HistogramCache::display() const
{
  HistogramCache::print();
}

void 
HistogramCache::print(FILE *ofd, const char* indent, const char* title) const
{
#ifndef NDEBUG
  BUMP_INDENT(indent);
  fprintf(ofd,"%s%s\n",NEW_INDENT,title);
  fprintf(ofd,"entries: %d \n", entries());
  fprintf(ofd,"size: %d bytes\n", size_);
  for (CollIndex x=lruQ_.entries(); x>0; x--)
  {
    lruQ_[x-1]->print(ofd, indent, "HistogramCacheEntry");
  }
#endif
}

void HistogramCache::traceTable(NATable& table) const
{
  if (tfd_)
  {
    NAString tableName(table.getTableName().getQualifiedNameAsString());
    fprintf(tfd_,"table:%s\n",tableName.data());
    table.getColStats()->trace(tfd_, &table);
    fflush(tfd_);
  }
}

void HistogramCache::traceTablesFinalize() const
{
  if (tfd_)
  {
    fprintf(tfd_,"cache_size:%d\n", size_);
    fprintf(tfd_,"cache_heap_size:" PFSZ "\n", heap_->getAllocSize());
    fflush(tfd_);
  }
}

void HistogramCache::closeTraceFile() 
{
  if (tfd_) fclose(tfd_);
  tfd_ = NULL;
}

void HistogramCache::openTraceFile(const char *filename) 
{
  tfd_ = fopen(filename, "w+");
}

void HistogramCache::closeMonitorFile() 
{
  if (mfd_) fclose(mfd_);
  mfd_ = NULL;
}

void HistogramCache::openMonitorFile(const char *filename) 
{
  mfd_ = fopen(filename, "w+");
}

void HistogramCache::monitor() const
{
  // if histogram caching is off, there's nothing to monitor
  if(!OptDefaults::cacheHistograms()) return;

  if (mfd_)
  {
    for (CollIndex x=lruQ_.entries(); x>0; x--)
    {
      lruQ_[x-1]->monitor(mfd_);
    }
    if (CmpCommon::getDefault(CACHE_HISTOGRAMS_MONITOR_MEM_DETAIL) == DF_ON)
    {
      fprintf(mfd_,"cache_size:%d\n", size_);
      fprintf(mfd_,"cache_heap_size:" PFSZ "\n", heap_->getAllocSize());
    }
    fflush(mfd_);
  }
}

// constructor for memory efficient representation of colStats.
// colStats has both single-column & multi-column histograms.
HistogramsCacheEntry::HistogramsCacheEntry
(const StatsList & colStats,
 const QualifiedName & qualifiedName,
				                     const Int64 & modifTime,
                                     const Int64 & statsTime,
                                     const Int64 & redefTime,
 NAMemory * heap) 
  : full_(NULL), multiColumn_(NULL), name_(NULL), heap_(heap)
  , refreshTime_(0), singleColumnPositions_(heap)
  , accessedInCurrentStatement_(TRUE)
  , size_(0)
{
	modifTime_ = modifTime;
    statsTime_ = statsTime;
  updateRefreshTime();
  redefTime_ = redefTime;
  preFetched_ = CURRSTMT_OPTDEFAULTS->preFetchHistograms();
  allFakeStats_ = colStats.allFakeStats();

  // make a deep copy of the key. 
  // qualifiedName is short-lived (from stmtheap).
  // name_ is longer-lived (from contextheap).
  name_ = new(heap_) QualifiedName(qualifiedName, heap_);

  // create pointers to full single-column histograms (include fake)
  UInt32 singleColumnCount = colStats.getSingleColumnCount(); 
  if (singleColumnCount > 0)
  {
    full_ = new(heap_) NAList<ColStatsSharedPtr>(heap_, singleColumnCount);

    // fill-in pointers to deep copy of single-column histograms
	for(UInt32 i=0; i<colStats.entries();i++)
	{
      const NAColumnArray& colArray = colStats[i]->getStatColumns();
      if (colArray.entries() == 1)
      {
        // keep pointer to deep copy of single-column histogram
        full_->insertAt(full_->entries(),
                        ColStats::deepCopyHistIntoCache(*(colStats[i]),heap_));

        // update singleColumnPositions
        singleColumnPositions_ += 
          (Lng32)colArray.getColumn(Lng32(0))->getPosition();
      }
    }
  }

  // create pointers to multi-column histograms
  multiColumn_ = new(heap_) MultiColumnHistogramList(heap_);

  // add deep copy of multi-column histograms (but, avoid duplicates)
  multiColumn_->addMultiColumnHistograms(colStats);
}

// insertDeepCopyIntoCache adds histograms of the sametype
// (single-column and/or multi-column) to this cache entry
void 
HistogramsCacheEntry::addToCachedEntry
(NAColumnArray & columns, StatsList & list)
{
  // update allFakeStats_
  if (allFakeStats_)
    allFakeStats_ = list.allFakeStats();

		//iterate over all the colstats in the stats list passed in
  ColumnSet singleColHistAdded(heap_);
		
  for(UInt32 j=0;j<list.entries();j++)
  {
    //get the columns for the current colstats
    NAColumnArray colList = list[j]->getStatColumns();
    
		//get the first column for the columns represented by
		//the current colstats
		NAColumn * column = colList.getColumn(Lng32(0));
    
    //column position of first column
    Lng32 currentColPosition = column->getPosition();
    
    //check if current column requires full histograms
    NABoolean requiresHistogram = column->needHistogram();
    
    //check if current colstats is a single-column histogram
    NABoolean singleColHist = (colList.entries()==1? TRUE: FALSE);
    NABoolean mcForHbasePart = list[j]->isMCforHbasePartitioning ();

    //only fullHistograms are inserted in full_.
    //We also add fake histograms to the cache.
    //This will help us not to call FetchHistograms
    //for a column that has fake statistics. 
    //Previously we did not cache statistics for
    //columns that did not have statistics in the histograms tables 
    //(FetchHistogram faked statistics for such column). 
    //Since statistics for such columns were not found in the
    //cache we had to repeatedly call FetchHistogram 
    //to get statistics for these columns
    //instead of just getting the fake statistics from the cache. 
    //FetchHistograms always return fake statistics for such columns 
    //so why not just cache them and not call FetchHistograms. 
    //When statistics are added for these columns then the timestamp
    //matching code will realize that and 
    //re-read the statistics for the table again.
    if((requiresHistogram || NOT singleColHist)|| list[j]->isFakeHistogram())
    {
        //if single column Histograms
        //if((singleColHist || mcForHbasePart) && (!singleColumnPositions_.contains(currentColPosition)))
        if((singleColHist) && (!singleColumnPositions_.contains(currentColPosition)))
			{
			  //Current colstats represent a single column histogram
			  //Insert the colstats from the stats list passed in, at the end of
			  //this objects stats list (represented by colStats_).
        full_->insertAt(full_->entries(),
                        ColStats::deepCopyHistIntoCache(*(list[j]),heap_));
        
        singleColHistAdded += currentColPosition;
      }
      else if (NOT singleColHist)
      {
        //Assumption: a multi-column histogram is retrieved when 
        //histograms for any of its columns are retrieved.
        //e.g. Table T1(a int, b int, c int)
        //histograms: {a},{b},{c},{a,b},{a,c},{b,c},{a,b,c}
        //If histograms for column a are fetched we will get 
        //histograms: {a}, {a,b}, {a,c}, {a,b,c}
        //If histograms for column b are fetched we will get
        //histograms: {b}, {a,b}, {b,c}, {a,b,c}
        //Therefore to avoid duplicated multicolumn stats being inserted
        //we pass down the list of single columns for which we have stats

			  //Current colstats represent a multicolumn histogram
        addMultiColumnHistogram(*(list[j]), &singleColumnPositions_);
      }
    }
  }
  singleColumnPositions_ += singleColHistAdded;
}

// add multi-column histogram to this cache entry
void 
HistogramsCacheEntry::addMultiColumnHistogram
(const ColStats& mcStat, ColumnSet* singleColPositions)
{
  if (!multiColumn_)
    multiColumn_ = new(heap_) MultiColumnHistogramList(heap_);

  multiColumn_->addMultiColumnHistogram(mcStat, singleColPositions);
}

const QualifiedName* 
HistogramsCacheEntry::getName() const
{
  return name_;
}

const ColStatsSharedPtr 
HistogramsCacheEntry::getStatsAt(CollIndex x) const
{
  if (!full_ OR x > full_->entries())
    return NULL;
  else
    return full_->at(x);
}

const MultiColumnHistogram*
HistogramsCacheEntry::getMultiColumnAt(CollIndex x) const
{
  if (!multiColumn_ OR x > multiColumn_->entries())
    return NULL;
  else
    return multiColumn_->at(x);
}

// return pointer to full single-column histogram identified by col
ColStatsSharedPtr const
HistogramsCacheEntry::getHistForCol (NAColumn& col) const
{
  if (!full_) return NULL;

  // search for colPos in full_
  for(UInt32 i=0; i < full_->entries(); i++)
  {
    // have we found colPos?
    if (((*full_)[i]->getStatColumnPositions().entries() == 1) AND
        (*full_)[i]->getStatColumnPositions().contains(col.getPosition()))
    {
      return (*full_)[i];
    }
  }
  return NULL;
}
  
// insert all multicolumns referencing col into list
// use singleColsFound to avoid duplicates
void
HistogramsCacheEntry::getMCStatsForColFromCacheIntoList
(StatsList& list, // out: "fat" rep of multi-column stats for col
 NAColumn&  col,  // in:  column whose multi-column stats we want
 ColumnSet& singleColsFound) // in: columns whose single-column 
  //stats have already been processed by caller.
  //Assumption: a multi-column histogram is retrieved when 
  //histograms for any of its columns are retrieved.
{
  CollIndex multiColCount = multiColumnCount();
  if (multiColCount <= 0) return; // entry has no multicolumn stats

  // search entry's multicolumn stats for col
  NAMemory* heap = list.heap();
  for(UInt32 i=0; i<multiColCount; i++)
  {
    const MultiColumnHistogram* mcHist = getMultiColumnAt(i);
    if (mcHist)
    {
      ColumnSet mcCols(mcHist->cols(), STMTHEAP);
      if (!mcCols.contains(col.getPosition())) 
        continue; // no col

      if ((mcCols.intersectSet(singleColsFound)).entries()) 
        continue; // avoid dup
 
      // create "fat" representation of multi-column histogram

      ColStatsSharedPtr mcStat;
      if (col.getNATable()->isHbaseTable() && col.isPrimaryKey()) {

        // For mcStats covering a key column of a HBASE table, 
        // create a colStat object with multi-intervals, which will
        // be useful in allowing better stats-based split.
        mcStat = new (STMTHEAP) ColStats(*(mcHist->getColStatsPtr()), 
                                         STMTHEAP, TRUE);

      } else {

        ComUID id(mcHist->id());
        CostScalar uec(mcHist->uec());
        CostScalar rows(mcHist->rows());

        mcStat = new (STMTHEAP) ColStats 
          (id, uec, rows, rows, FALSE, FALSE, NULL, FALSE,
           1.0, 1.0, 0, STMTHEAP, FALSE);
      
        // populate its NAColumnArray with mcCols
        (*mcStat).populateColumnArray(mcHist->cols(), col.getNATable());

        // set up its histogram interval
        HistogramSharedPtr histogram = new(STMTHEAP) Histogram(heap);
        HistInt loInt;
        NABoolean boundaryInclusive = TRUE;
        HistInt hiInt(1, NULL, (*mcStat).statColumns(), 
                      rows, uec, boundaryInclusive, 0);
        histogram->insert(loInt);
        histogram->insert(hiInt);
        mcStat->setHistogram(histogram);

        MCSkewedValueList * mcSkewedValueList = new (STMTHEAP) MCSkewedValueList (*(mcHist->getMCSkewedValueList()), STMTHEAP);
        mcStat->setMCSkewedValueList(*mcSkewedValueList);
      }

      // append to list the mcStat
      list.insertAt(list.entries(), mcStat);
    }
  }
}

//destructor
HistogramsCacheEntry::~HistogramsCacheEntry()
{
	if(full_)
  {
    ColStatsSharedPtr colStat = NULL;
    while(full_->getFirst(colStat))
    {
      colStat->deepDeleteFromHistogramCache();

      //colStats is a shared pointer
      //and will not be deleted till
      //ref count goes to zero
      //Therefore to avoid leaks and
      //ensure colStats is deleted we
      //do the following
      ColStats * colStatPtr = colStat.get();
      colStat.reset();
      delete colStatPtr;
    }
    delete full_;
  }
  if(multiColumn_)
    delete multiColumn_;
  if(name_)
    delete name_;
  singleColumnPositions_.clear();
}

void HistogramsCacheEntry::display() const
{
  HistogramsCacheEntry::print();
}

void HistogramsCacheEntry::monitor(FILE* mfd) const
{
  NAString tableName(name_->getQualifiedNameAsString());
  fprintf(mfd,"table:%s\n",tableName.data());
  if (CmpCommon::getDefault(CACHE_HISTOGRAMS_MONITOR_HIST_DETAIL) == DF_ON)
  {
    if (full_)
    {
      for (CollIndex x=0; x<full_->entries(); x++)
      {
        full_->at(x)->trace(mfd, NULL);
      }
    }
    if (multiColumn_)
    {
      multiColumn_->print(mfd, NULL);
    }
  }
  if (CmpCommon::getDefault(CACHE_HISTOGRAMS_MONITOR_MEM_DETAIL) == DF_ON)
    fprintf(mfd,"table_size:%d\n",size_);
  fflush(mfd);
}

void HistogramsCacheEntry::print
(FILE *ofd, const char* indent, const char* title) const
{
#ifndef NDEBUG
  BUMP_INDENT(indent);
  fprintf(ofd,"%s%s\n",NEW_INDENT,title);
  name_->print(ofd);
  fprintf(ofd,"accessedInCurrentStatement_:%d ", accessedInCurrentStatement_);
  fprintf(ofd,"allFakeStats_:%d ", allFakeStats_);
  fprintf(ofd,"preFetched_:%d \n", preFetched_);
  char time[30];
  convertInt64ToAscii(modifTime_, time);
  fprintf(ofd,"modifTime_:%s ", time);
  convertInt64ToAscii(redefTime_, time);
  fprintf(ofd,"redefTime_:%s ", time);
  convertInt64ToAscii(refreshTime_, time);
  fprintf(ofd,"refreshTime_:%s ", time);
  convertInt64ToAscii(statsTime_, time);
  fprintf(ofd,"statsTime_:%s ", time);
  convertInt64ToAscii(getLastUpdateStatsTime(), time);
  fprintf(ofd,"lastUpdateStatsTime:%s \n", time);
  fprintf(ofd,"single-column histograms:%d ", singleColumnCount());
  singleColumnPositions_.printColsFromTable(ofd,NULL);
  if (full_)
  {
    for (CollIndex x=0; x<full_->entries(); x++)
    {
      full_->at(x)->print(ofd);
    }
  }
  fprintf(ofd,"multi-column histograms:%d ", multiColumnCount());
  if (multiColumn_)
  {
    multiColumn_->print(ofd);
  }
#endif
}

// -----------------------------------------------------------------------
// getRangePartitionBoundaryValues()
// This method receives a string within which the partitioning key values
// appear in a comma-separated sequence. It returns an ItemExprList that
// contains ConstValue expressions for representing each partitioning
// key value as shown below:
//
//                                   ------      ------      ------
// "<value1>, <value2>, <value3>" => |    | ---> |    | ---> |    |
//                                   ------      ------      ------
//                                     |           |           |
//                                     v           v           v
//                                 ConstValue  ConstValue  ConstValue
//                                 (<value1>)  (<value2>)  (<value3>)
//
// -----------------------------------------------------------------------
ItemExpr * getRangePartitionBoundaryValues
                        (const char * keyValueBuffer,
			 const Lng32   keyValueBufferSize,
			 NAMemory* heap,
                          CharInfo::CharSet strCharSet = CharInfo::UTF8
                         )
{
  char * keyValue;             // the string for the key value
  ItemExpr * partKeyValue;     // -> dynamically allocated expression
  Lng32 length;                 // index to the next key value and its length
  Lng32 startIndex = 0;
  Lng32 stopIndex = keyValueBufferSize-1;

  startIndex = skipLeadingBlanks(keyValueBuffer, startIndex, stopIndex);
  // Skip leading '('
  NABoolean leadingParen = FALSE;
  if (keyValueBuffer[startIndex] == '(')
    {
      leadingParen = TRUE;
      startIndex++;
    }

  stopIndex = skipTrailingBlanks(&keyValueBuffer[startIndex], stopIndex);
  // Skip trailing ')' only if there was a leading paren. This
  // is the case where the value comes in as (<value>)
  if ((keyValueBuffer[stopIndex] == ')') &&
      (leadingParen == TRUE))
    stopIndex--;

  length = stopIndex - startIndex + 1;

  NAString keyValueString( &keyValueBuffer[startIndex], (size_t) length );

  // ---------------------------------------------------------------------
  // Copy the string from the keyValueBuffer into a string that
  // is terminated by a semicolon and a null.
  // ---------------------------------------------------------------------
  keyValue = new (heap) char[length + 1 /* for semicolon */ + 1 /* for eol */ ];
  // strncpy( keyValue, keyValueString.data(), (size_t) length );
  //soln:10-031112-1256
  // strncpy replaced with memcpy to handle columns of the partition?s first key value is
  // NULL character within double-quote  eg:( ?\0? ).  i.e (  "( "6666673"  , "\0" ,  8060928 )").

  memcpy(keyValue, (char *)( keyValueString.data() ), (size_t) length );
  keyValue[length] = ';';
  keyValue[length+1] = '\0';

  // ---------------------------------------------------------------------
  // Create a new ItemExprList using the parse tree generated from the
  // string of comma-separated literals.
  // ---------------------------------------------------------------------
  Parser parser(CmpCommon::context());
  //partKeyValue = parser.getItemExprTree(keyValue);
  partKeyValue = parser.getItemExprTree(keyValue,length+1,strCharSet);
  // Check to see if the key values parsed successfully.  An error
  // could occur if the table is an MP Table and the first key values
  // contain MP syntax that is not supported by MX.  For instance
  // Datetime literals which do not have the max number of digits in
  // each field. (e.g. DATETIME '1999-2-4' YEAR TO DAY)
  //
  if(partKeyValue == NULL) {
    return NULL;
  }

  return partKeyValue->copyTree(heap);

} // static getRangePartitionBoundaryValues()

// In some cases we don't have a text representation of the start keys,
// only the encoded keys (e.g. from HBase regions start keys). In this
// case, un-encode these binary values and form ConstValues from them.
static ItemExpr * getRangePartitionBoundaryValuesFromEncodedKeys(
             const NAColumnArray & partColArray,
             const char * encodedKey,
             const Lng32  encodedKeyLen,
             NAMemory*    heap)
{
  Lng32 keyColOffset = 0;
  ItemExpr *result = NULL;
  char *actEncodedKey = (char *) encodedKey; // original key or a copy
  const char* encodedKeyP = NULL;
  char* varCharstr = NULL;
  Lng32 totalKeyLength = 0;
  Lng32 numProvidedCols = 0;
  Lng32 lenOfFullyProvidedCols = 0;

  // in newer HBase versions, the region start key may be shorter than an actual key
  for (CollIndex i = 0; i < partColArray.entries(); i++)
    {
      const NAType *pkType = partColArray[i]->getType();
      Lng32 colEncodedLength = pkType->getSQLnullHdrSize() + pkType->getNominalSize();

      totalKeyLength += colEncodedLength;
      if (totalKeyLength <= encodedKeyLen)
        {
          // this column is fully provided in the region start key
          numProvidedCols++;
          lenOfFullyProvidedCols = totalKeyLength;
        }
    }

  if (encodedKeyLen < totalKeyLength)
    {
      // the provided key does not cover all the key columns

      // need to extend the partial buffer, allocate a copy
      actEncodedKey = new(heap) char[totalKeyLength];
      memcpy(actEncodedKey, encodedKey, encodedKeyLen);
      memset(&actEncodedKey[encodedKeyLen], 0, totalKeyLength-encodedKeyLen);
      Lng32 currOffset = lenOfFullyProvidedCols;

      // go through the partially or completely missing columns and make something up
      // so that we can treat the buffer as fully encoded in the final loop below
      for (CollIndex j = numProvidedCols; j < partColArray.entries(); j++)
        {
          const NAType *pkType = partColArray[j]->getType();
          Lng32 nullHdrSize = pkType->getSQLnullHdrSize();
          Lng32 colEncodedLength = nullHdrSize + pkType->getNominalSize();
          NABoolean isDescending = (partColArray[j]->getClusteringKeyOrdering() == DESCENDING);

          NABoolean columnIsPartiallyProvided = (currOffset < encodedKeyLen);

          if (columnIsPartiallyProvided)
            {
              // This column is partially provided, try to make sure that it has a valid
              // value. Note that the buffer has a prefix of some bytes with actual key
              // values, followed by bytes that are zeroed out. 


              // First, for descending columns, use 0xFF instead of 0 for fillers
              if (isDescending)
                memset(&actEncodedKey[encodedKeyLen],
                       0xFF,
                       currOffset + colEncodedLength - encodedKeyLen);

              // Next, decide by data type whether it's ok for the type to have
              // a suffix of the buffer zeroed out (even descending columns will
              // in the end see zeroes). If the type can't take it, we'll just
              // discard all the partial information.

              switch (pkType->getTypeQualifier())
                {
                case NA_NUMERIC_TYPE:
                  {
                    NumericType *nt = (NumericType *) pkType;

                    if (!nt->isExact() || nt->isDecimal() || nt->isBigNum())
                      columnIsPartiallyProvided = FALSE;
                  }
                  break;

                case NA_DATETIME_TYPE:
                case NA_INTERVAL_TYPE:
                  // those types should tolerate zeroing out trailing bytes
                  break;

                case NA_CHARACTER_TYPE:
                  // generally, character types should also tolerate zeroing out
                  // trailing bytes, but we might need to clean up characters
                  // that got split in the middle
                  {
                    CharInfo::CharSet cs = pkType->getCharSet();

                    switch (cs)
                      {
                      case CharInfo::UCS2:
                      case CharInfo::UTF8:
                        // For now just accept partial characters, it's probably ok
                        // since they are just used as a key. May look funny in EXPLAIN.
                        break;

                      default:
                        break;
                      }
                  }
                  break;

                default:
                  columnIsPartiallyProvided = FALSE;
                  break;
                }

              if (columnIsPartiallyProvided)
                // from now on, treat it as if it were fully provided
                numProvidedCols++;
            }

          if (!columnIsPartiallyProvided)
            {
              // This column is not at all provided in the region start key
              // or we decided to erase the partial value.
              // Generate the min/max value for ASC/DESC key columns.
              // NOTE: This is generating un-encoded values, unlike
              //       the values we get from HBase. The next loop below
              //       will skip decoding for any values generated here.
              Lng32 remainingBufLen = colEncodedLength - nullHdrSize;

              if (nullHdrSize)
                {
                  // generate a NULL indicator
                  // NULL (-1) for descending columns, this is the max value
                  // non-NULL (0) for ascending columns, min value is non-null
                  short indicatorVal = (isDescending ? -1 : 0);

                  CMPASSERT(nullHdrSize == sizeof(short));
                  memcpy(&actEncodedKey[currOffset], &indicatorVal, sizeof(indicatorVal));
                }

              if (isDescending)
                {
                  pkType->maxRepresentableValue(&actEncodedKey[currOffset + nullHdrSize],
                                                &remainingBufLen,
                                                NULL,
                                                heap);
                }
              else
                {
                  pkType->minRepresentableValue(&actEncodedKey[currOffset + nullHdrSize],
                                                &remainingBufLen,
                                                NULL,
                                                heap);
                }
            }

          currOffset += colEncodedLength;

        } // loop through columns not entirely provided

    } // provided encoded key length < total key length

  for (CollIndex c = 0; c < partColArray.entries(); c++)
    {
      const NAType *pkType = partColArray[c]->getType();
      Lng32 decodedValueLen = 
        pkType->getNominalSize() + pkType->getSQLnullHdrSize();
      ItemExpr *keyColVal = NULL;

      // does this column need encoding (only if it actually came
      // from an HBase split key, if we made up the value it's
      // already in the decoded format)
      if (pkType->isEncodingNeeded() && c < numProvidedCols)
        {
          encodedKeyP = &actEncodedKey[keyColOffset];

          // for varchar the decoding logic expects the length to be in the first
          // pkType->getVarLenHdrSize() chars, so add it.
          // Please see bug LP 1444134 on how to improve this in the long term.

          // Note that this is less than ideal:
          // - A VARCHAR is really encoded as a fixed char in the key, as
          //   the full length without a length field
          // - Given that an encoded key is not aligned, we should really
          //   consider it a byte string, e.g. a character type with charset
          //   ISO88591, which tolerates any bit patterns. Considering the
          //   enoded key as the same data type as the column causes all kinds
          //   of problems.
          // - The key decode function in the expressions code expects the varchar
          //   length field, even though it is not present in an actual key. So,
          //   we add it here in a separate buffer.
          // - When we generate a ConstValue to represent the decoded key, we also
          //   need to include the length field, with length = max. length

          if (pkType->getTypeName() == "VARCHAR")
          {
            Int32 varLenSize = pkType->getVarLenHdrSize() ;
            Int32 nullHdrSize = pkType->getSQLnullHdrSize();
            // Format of encodedKeyP :| null hdr | varchar data|
            // Format of VarcharStr : | null hdr | var len hdr | varchar data|
            varCharstr = new (heap) char[decodedValueLen + varLenSize];
            
            if (nullHdrSize > 0)
              str_cpy_all(varCharstr, encodedKeyP, nullHdrSize);

            // careful, this works on little-endian systems only!!
            str_cpy_all(varCharstr+nullHdrSize, (char*) &decodedValueLen, 
                        varLenSize);
            str_cpy_all(varCharstr+nullHdrSize+varLenSize, 
                        encodedKeyP+nullHdrSize, 
                        decodedValueLen-nullHdrSize);
            decodedValueLen += pkType->getVarLenHdrSize();
            encodedKeyP = varCharstr;
          }

          // un-encode the key value by using an expression
          ConstValue *keyColEncVal =
            new (heap) ConstValue(pkType,
                                  (void *) encodedKeyP,
                                  decodedValueLen,
                                  NULL,
                                  heap);
          CMPASSERT(keyColEncVal);

          if (keyColEncVal->isNull())
          {
            // do not call the expression evaluator if the value 
            // to be decoded is NULL.
            keyColVal = keyColEncVal ;
          }
          else 
          {
            keyColVal =
              new(heap) CompDecode(keyColEncVal,
                                   pkType,
                                   !partColArray.isAscending(c),
                                   decodedValueLen,
                                   CollationInfo::Sort,
                                   TRUE,
                                   heap);

            keyColVal->synthTypeAndValueId();

            ValueIdList exprs;
            exprs.insert(keyColVal->getValueId());

            char staticDecodeBuf[200];
            Lng32 staticDecodeBufLen = 200;

            char* decodeBuf = staticDecodeBuf;
            Lng32 decodeBufLen = staticDecodeBufLen;

            // For character types, multiplying by 8 to deal with conversions between
            // any two known character sets supported.  
            Lng32 factor = (DFS2REC::isAnyCharacter(pkType->getFSDatatype())) ? 8 : 1;

            if ( staticDecodeBufLen < decodedValueLen * factor) {
              decodeBufLen = decodedValueLen * factor;
              decodeBuf = new (STMTHEAP) char[decodeBufLen];
            }

            Lng32 resultLength = 0;
            Lng32 resultOffset = 0;

            // Produce the decoded key. Refer to 
            // ex_function_encode::decodeKeyValue() for the 
            // implementation of the decoding logic.
            ex_expr::exp_return_type rc = exprs.evalAtCompileTime
              (0, ExpTupleDesc::SQLARK_EXPLODED_FORMAT, decodeBuf, decodeBufLen,
               &resultLength, &resultOffset, CmpCommon::diags()
               );


            if ( rc == ex_expr::EXPR_OK ) {
              CMPASSERT(resultOffset == pkType->getPrefixSizeWithAlignment());
              // expect the decodeBuf to have this layout
              // | null ind. | varchar length ind. | alignment | result |
              // |<---getPrefixSizeWithAlignment-------------->|
              // |<----getPrefixSize-------------->|

              // The method getPrefixSizeWithAlignment(), the diagram above,
              // and this code block assumes that varchar length ind. is
              // 2 bytes if present. If it is 4 bytes we should fail the 
              // previous assert

              // Next we get rid of alignment bytes by prepending the prefix
              // (null ind. + varlen ind.) to the result. ConstValue constr.
              // will process prefix + result. The assert above ensures that 
              // there are no alignment fillers at the beginning of the 
              // buffer. Given the previous assumption about size
              // of varchar length indicator, alignment bytes will be used by
              // expression evaluator only if column is of nullable type.
              // For a description of how alignment is computed, please see
              // ExpTupleDesc::sqlarkExplodedOffsets() in exp/exp_tuple_desc.cpp

              if (pkType->getSQLnullHdrSize() > 0)
                memmove(&decodeBuf[resultOffset - pkType->getPrefixSize()], 
                                  decodeBuf, pkType->getPrefixSize());
              keyColVal =
                new (heap) 
                ConstValue(pkType,
                           (void *) &(decodeBuf[resultOffset - 
                                                pkType->getPrefixSize()]),
                           resultLength+pkType->getPrefixSize(),
                           NULL,
                           heap);
            }

            if ( rc != ex_expr::EXPR_OK ) 
              return NULL;
          }

        } // encoded 
      else
        {
          // simply use the provided value as the binary value of a constant
          keyColVal =
            new (heap) ConstValue(pkType,
                                  (void *) &actEncodedKey[keyColOffset],
                                  decodedValueLen,
                                  NULL,
                                  heap);
        }

      // this and the above assumes that encoded and unencoded values
      // have the same length
      keyColOffset += decodedValueLen;
      if (pkType->getTypeName() == "VARCHAR")
      {
         keyColOffset -= pkType->getVarLenHdrSize();
         NADELETEBASIC (varCharstr, heap);
         varCharstr = NULL;
      }

      if (result)
        result = new(heap) ItemList(result, keyColVal);
      else
        result = keyColVal;
    }

  // make sure we consumed the entire key but no more than that
  CMPASSERT(keyColOffset == totalKeyLength);

  if (actEncodedKey != encodedKey)
    NADELETEBASIC(actEncodedKey, heap);

  return result;
} // static getRangePartitionBoundaryValuesFromEncodedKeys()


// -----------------------------------------------------------------------
// createRangePartitionBoundaries()
// This method is used for creating a tuple, which defines the maximum
// permissible values that the partitioning key columns can contain
// within a certain partition, for range-partitioned data.
// -----------------------------------------------------------------------
NABoolean checkColumnTypeForSupportability(const NAColumnArray & partColArray, const char* key)
{
  NABoolean floatWarningIssued = FALSE;
  for (CollIndex c = 0; c < partColArray.entries(); c++) {

    const NAType *pkType = partColArray[c]->getType();

    // For the EAP release, the unsupported types are the non-standard
    // SQL/MP Datetime types.  For the FCS release the unsupported
    // types are the FRACTION only SQL/MP Datetime types.
    //
    // They are (for now) represented as CHAR types that have a
    // non-zero MP Datetime size.
    //
    NABoolean unsupportedPartnKey = FALSE;
    NABoolean unsupportedFloatDatatype = FALSE;
    if (NOT pkType->isSupportedType())
      unsupportedPartnKey = TRUE;
    else if (DFS2REC::isFloat(pkType->getFSDatatype())) {

      const NATable * naTable = partColArray[c]->getNATable();
      
      if ((CmpCommon::getDefault(MARIAQUEST_PROCESS) == DF_OFF) &&
	  (NOT naTable->isSeabaseTable()) &&
	  (NOT naTable->isHiveTable())) {
	unsupportedPartnKey = TRUE;
	unsupportedFloatDatatype = TRUE;
      }
    }

    if (unsupportedPartnKey) {
      // Get the name of the table which has the unsupported
      // partitioning key column.
      //
      const NAString &tableName =
          partColArray[c]->getNATable()->
          getTableName().getQualifiedNameAsAnsiString();

      if (unsupportedFloatDatatype)
	*CmpCommon::diags()
	  << DgSqlCode(-1120);
      else
	// ERROR 1123 Unable to process the partition key values...
	*CmpCommon::diags()
	  << DgSqlCode(-1123)
	  << DgString0(key)
	  << DgTableName(tableName);

      return FALSE;
    }
  }
      
  return TRUE;
}

// -----------------------------------------------------------------------
// createRangePartitionBoundaries()
// This method is used for creating a tuple, which defines the maximum
// permissible values that the partitioning key columns can contain
// within a certain partition, for range-partitioned data.
// -----------------------------------------------------------------------
#pragma nowarn(1506)   // warning elimination
static RangePartitionBoundaries * createRangePartitionBoundaries
                                     (desc_struct * part_desc_list,
				      Lng32 numberOfPartitions,
			              const NAColumnArray & partColArray,
				      NAMemory* heap)
{

  // ---------------------------------------------------------------------
  // ASSUMPTION: The partitions descriptor list is a singly-linked list
  // ==========  in which the first element is the descriptor for the
  //             first partition and the last element is the descriptor
  //             for the last partition, in partitioning key sequence.
  // ---------------------------------------------------------------------
    desc_struct * partns_desc = part_desc_list;
  CMPASSERT(partns_desc->body.partns_desc.primarypartition);


  // Check all the partitioning keys.  If any of them are not
  // supported, issue an error and return.
  //
  // Skip past the primary partition, so that a meaningful first
  // key value can be used for the error message.

  char* key = (partns_desc->header.next) ->body.partns_desc.firstkey;

  if ( !checkColumnTypeForSupportability(partColArray, key) )
    return NULL;
  

  // ---------------------------------------------------------------------
  // Allocate a new RangePartitionBoundaries.
  // ---------------------------------------------------------------------
  RangePartitionBoundaries * partBounds = new (heap)
    RangePartitionBoundaries
      (numberOfPartitions,
       partColArray.entries(),heap);

  // ---------------------------------------------------------------------
  // compute the length of the encoded partitioning key
  // ---------------------------------------------------------------------


  // ---------------------------------------------------------------------
  // Iterate over all the partitions and define the boundary (maximum
  // permissible key values) for each one of them.
  // The first key for the first partition cannot be specified in
  // the CREATE TABLE command. It is therefore stored as an empty
  // string in the SMD.
  // NOTE: The RangePartitionBoundaries is 0 based.
  // ---------------------------------------------------------------------
  partns_desc = partns_desc->header.next; // skip the primary partition
  Lng32 counter = 1;
  char* encodedKey;

  while (partns_desc AND (counter < numberOfPartitions))
    {
      encodedKey = partns_desc->body.partns_desc.encodedkey;
      size_t encodedKeyLen = partns_desc->body.partns_desc.encodedkeylen;

      if(heap != CmpCommon::statementHeap())
      {
        //we don't know here if encodedkey is a regular char or a wchar
        //if it's a wchar then it should end with "\0\0", so add an extra
        //'\0' to the end, it wont hurt anyways. Copying encodedKeyLen+1 chars
        //will include one '\0' character and we add an extra '\0' to the end
        //to make it "\0\0".
        encodedKey = new(heap) char [encodedKeyLen+2];
        encodedKey[encodedKeyLen] = encodedKey[encodedKeyLen+1] = '\0';
        str_cpy_all(encodedKey, partns_desc->body.partns_desc.encodedkey,
                    encodedKeyLen);
      }

      ItemExpr *rangePartBoundValues = NULL;

      if (partns_desc->body.partns_desc.firstkey)
        // Extract and parse the partition boundary values, producing an
        // ItemExprList of the boundary values.
        //
        rangePartBoundValues = getRangePartitionBoundaryValues(
             partns_desc->body.partns_desc.firstkey,
             partns_desc->body.partns_desc.firstkeylen,
             heap);
      else
        rangePartBoundValues = getRangePartitionBoundaryValuesFromEncodedKeys(
             partColArray,
             encodedKey,
             encodedKeyLen,
             heap);

      // Check to see if the key values parsed successfully.  An error
      // could occur if the table is an MP Table and the first key
      // values contain MP syntax that is not supported by MX. For
      // instance Datetime literals which do not have the max number
      // of digits in each field. (e.g. DATETIME '1999-2-4' YEAR TO
      // DAY)
      //
      if (rangePartBoundValues == NULL) {

        // Get the name of the table which has the 'bad' first key
        // value.  Use the first entry in the array of partition
        // columns (partColArray) to get to the NATable object.
        //
        const NAString &tableName =
          partColArray[0]->getNATable()->
          getTableName().getQualifiedNameAsAnsiString();

        // The Parser will have already issued an error.
        // ERROR 1123 Unable to process the partition key values...
        *CmpCommon::diags()
          << DgSqlCode(-1123)
          << DgString0(partns_desc->body.partns_desc.firstkey)
          << DgTableName(tableName);
        delete partBounds;
        //coverity[leaked_storage]
        return NULL;
      }

      partBounds->defineUnboundBoundary(
           counter++,
           rangePartBoundValues,
           encodedKey);

      partns_desc = partns_desc->header.next;
    } // end while (partns_desc)

  // ---------------------------------------------------------------------
  // Before doing consistency check setup for the statement
  // ---------------------------------------------------------------------
  partBounds->setupForStatement(FALSE);

  // ---------------------------------------------------------------------
  // Perform a consistency check to ensure that a boundary was defined
  // for each partition.
  // ---------------------------------------------------------------------
  partBounds->checkConsistency(numberOfPartitions);

  return partBounds;

} // static createRangePartitionBoundaries()
#pragma warn(1506)  // warning elimination

// -----------------------------------------------------------------------
// createRangePartitioningFunction()
// This method is used for creating a rangePartitioningFunction.
// -----------------------------------------------------------------------
static PartitioningFunction * createRangePartitioningFunction
                                (desc_struct * part_desc_list,
			         const NAColumnArray & partKeyColArray,
                                 NodeMap* nodeMap,
				 NAMemory* heap)
{
  // ---------------------------------------------------------------------
  // Compute the number of partitions.
  // ---------------------------------------------------------------------
  desc_struct * partns_desc = part_desc_list;
  Lng32 numberOfPartitions = 0;
  while (partns_desc)
    {
      numberOfPartitions++;
      partns_desc = partns_desc->header.next;
    }

  // ---------------------------------------------------------------------
  // Each table has at least 1 partition
  // ---------------------------------------------------------------------
  numberOfPartitions = MAXOF(1,numberOfPartitions);

  if (numberOfPartitions == 1)
    return new (heap) SinglePartitionPartitioningFunction(nodeMap, heap);

  // ---------------------------------------------------------------------
  // Create the partitioning key ranges
  // ---------------------------------------------------------------------
  RangePartitionBoundaries *boundaries =
    createRangePartitionBoundaries(part_desc_list,
				   numberOfPartitions,
				   partKeyColArray,
				   heap);

  // Check to see if the boundaries were created successfully.  An
  // error could occur if one of the partitioning keys is an
  // unsupported type or if the table is an MP Table and the first key
  // values contain MP syntax that is not supported by MX.  For the
  // EAP release, the unsupported types are the non-standard SQL/MP
  // Datetime types.  For the FCS release the unsupported types are
  // the FRACTION only SQL/MP Datetime types. An example of a syntax
  // error is a Datetime literal which does not have the max number of
  // digits in each field. (e.g. DATETIME '1999-2-4' YEAR TO DAY)
  //
  if (boundaries == NULL) {
    // The Parser may have already issued an error.
    // ERROR 1123 Unable to process the partition key values...
    // will have been issued by createRangePartitionBoundaries.
    //
    return NULL;
  }

  return new (heap) RangePartitioningFunction(boundaries,  // memory leak??
                                              nodeMap, heap);

} // static createRangePartitioningFunction()

// -----------------------------------------------------------------------
// createRoundRobinPartitioningFunction()
// This method is used for creating a RoundRobinPartitioningFunction.
// -----------------------------------------------------------------------
// LCOV_EXCL_START :cnu
static PartitioningFunction * createRoundRobinPartitioningFunction
                                (desc_struct * part_desc_list,
                                 NodeMap* nodeMap,
				 NAMemory* heap)
{
  // ---------------------------------------------------------------------
  // Compute the number of partitions.
  // ---------------------------------------------------------------------
  desc_struct * partns_desc = part_desc_list;
  Lng32 numberOfPartitions = 0;
  while (partns_desc)
    {
      numberOfPartitions++;
      partns_desc = partns_desc->header.next;
    }

  // ---------------------------------------------------------------------
  // Each table has at least 1 partition
  // ---------------------------------------------------------------------
  numberOfPartitions = MAXOF(1,numberOfPartitions);

  // For round robin partitioning, must create the partitioning function
  // even for one partition, since the SYSKEY must be generated for
  // round robin and this is trigger off the partitioning function.
  //
//  if (numberOfPartitions == 1)
//    return new (heap) SinglePartitionPartitioningFunction(nodeMap);

  return new (heap) RoundRobinPartitioningFunction(numberOfPartitions, nodeMap, heap);

} // static createRoundRobinPartitioningFunction()
// LCOV_EXCL_STOP

// -----------------------------------------------------------------------
// createHashDistPartitioningFunction()
// This method is used for creating a HashDistPartitioningFunction.
// -----------------------------------------------------------------------
static PartitioningFunction * createHashDistPartitioningFunction
                                (desc_struct * part_desc_list,
			         const NAColumnArray & partKeyColArray,
                                 NodeMap* nodeMap,
				 NAMemory* heap)
{
  // ---------------------------------------------------------------------
  // Compute the number of partitions.
  // ---------------------------------------------------------------------
  desc_struct * partns_desc = part_desc_list;
  Lng32 numberOfPartitions = 0;
  while (partns_desc)
    {
      numberOfPartitions++;
      partns_desc = partns_desc->header.next;
    }

  // ---------------------------------------------------------------------
  // Each table has at least 1 partition
  // ---------------------------------------------------------------------
  numberOfPartitions = MAXOF(1,numberOfPartitions);

  if (numberOfPartitions == 1)
    return new (heap) SinglePartitionPartitioningFunction(nodeMap, heap);

  return new (heap) HashDistPartitioningFunction(numberOfPartitions, nodeMap, heap);

} // static createHashDistPartitioningFunction()

// -----------------------------------------------------------------------
// createHash2PartitioningFunction()
// This method is used for creating a Hash2PartitioningFunction.
// -----------------------------------------------------------------------
static PartitioningFunction * createHash2PartitioningFunction
                                (desc_struct * part_desc_list,
                                 const NAColumnArray & partKeyColArray,
                                 NodeMap* nodeMap,
                                 NAMemory* heap)
{
  // ---------------------------------------------------------------------
  // Compute the number of partitions.
  // ---------------------------------------------------------------------
  desc_struct * partns_desc = part_desc_list;
  Lng32 numberOfPartitions = 0;
  while (partns_desc)
    {
      numberOfPartitions++;
      partns_desc = partns_desc->header.next;
    }

  // ---------------------------------------------------------------------
  // Each table has at least 1 partition
  // ---------------------------------------------------------------------
  numberOfPartitions = MAXOF(1,numberOfPartitions);

  if (numberOfPartitions == 1)
    return new (heap) SinglePartitionPartitioningFunction(nodeMap, heap);

  return new (heap) Hash2PartitioningFunction(numberOfPartitions, nodeMap, heap);

} // static createHash2PartitioningFunction()


static PartitioningFunction * createHash2PartitioningFunction
                                (Int32 numberOfPartitions,
                                 const NAColumnArray & partKeyColArray,
                                 NodeMap* nodeMap,
                                 NAMemory* heap)
{
  // ---------------------------------------------------------------------
  // Each table has at least 1 partition
  // ---------------------------------------------------------------------
  if (numberOfPartitions == 1)
    return new (heap) SinglePartitionPartitioningFunction(nodeMap, heap);

  return new (heap) Hash2PartitioningFunction(numberOfPartitions, nodeMap, heap);

} // static createHash2PartitioningFunction()


static 
NodeMap* createNodeMapForHbase(desc_struct* desc, NAMemory* heap)
{
   Int32 partns = 0;
   desc_struct* hrk = desc;
 
   while ( hrk ) {
      partns++;
      hrk=hrk->header.next;
   }

   NodeMap* nodeMap = new (heap) 
       NodeMap(heap, partns, NodeMapEntry::ACTIVE, NodeMap::HBASE);

   return nodeMap;
}

static 
PartitioningFunction*
createHash2PartitioningFunctionForHBase(desc_struct* desc, NAMemory* heap)
{

   desc_struct* hrk = desc;
 
   NodeMap* nodeMap = createNodeMapForHbase(desc, heap);

   Int32 partns = nodeMap->getNumEntries();

   PartitioningFunction* partFunc;
   if ( partns > 1 )
     partFunc = new (heap) Hash2PartitioningFunction(partns, nodeMap, heap);
   else
     partFunc = new (heap) SinglePartitionPartitioningFunction(nodeMap, heap);

   return partFunc;
}

// -----------------------------------------------------------------------
// createRangePartitionBoundaries()
// This method is used for creating a tuple, which defines the maximum
// permissible values that the partitioning key columns can contain
// within a certain partition, for range-partitioned data.
//
// The boundary values of the range partitions are completely defined by 
// a histogram's boundary values.
//
// -----------------------------------------------------------------------
#pragma nowarn(1506)   // warning elimination
RangePartitionBoundaries * createRangePartitionBoundariesFromStats
                                      (const IndexDesc* idesc, 
                                       HistogramSharedPtr& hist,
                                       Lng32 numberOfPartitions,
                                       const NAColumnArray & partColArray,
                                       const ValueIdList& partitioningKeyColumnsOrder,
                                       const Int32 statsColsCount,
                                       NAMemory* heap)
{
  if ( (!checkColumnTypeForSupportability(partColArray, "")) ||
       (numberOfPartitions != hist->numIntervals())          ||
       (partColArray.entries() < statsColsCount)
     )
     return NULL;

  // ---------------------------------------------------------------------
  // Allocate a new RangePartitionBoundaries.
  // ---------------------------------------------------------------------
  RangePartitionBoundaries * partBounds = new (heap)
    RangePartitionBoundaries
      (numberOfPartitions,
       partColArray.entries(),heap);

  // ---------------------------------------------------------------------
  // compute the length of the encoded partitioning key
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // Iterate over all the partitions and define the boundary (maximum
  // permissible key values) for each one of them.
  // The first key for the first partition cannot be specified in
  // the CREATE TABLE command. It is therefore stored as an empty
  // string in the SMD.
  // NOTE: The RangePartitionBoundaries is 0 based.
  // ---------------------------------------------------------------------
  Lng32 counter = 1;
  ULng32 totalEncodedKeyLength = 0;

  Interval iter = hist->getFirstInterval();

  while ( iter.isValid() ) {

     totalEncodedKeyLength = 0;
  
     NAString* evInStr = NULL;

     NAColumn* ncol = partColArray[0];
     const NAType* nt = ncol->getType();

     double ev = ( !iter.isLast() ) ?  
                  iter.hiBound().getDblValue() : nt->getMaxValue();

     if ((partColArray.entries() == 1) && (statsColsCount == 1))
     {
        // Convert the double into a string value of the type of 
        // the leading key column
        evInStr = nt->convertToString(ev, heap);
     }
     else if ((partColArray.entries() > 1) && (statsColsCount == 1))
     {
        MCboundaryValueList mcEv; 
        mcEv.insert(EncodedValue(ev));
        evInStr = mcEv.convertToString(partColArray, iter.isLast());
     }
     else // partColArray.entries() > 1 && statsColsCount > 1
     {
        MCboundaryValueList mcEv = iter.hiMCBound();
        evInStr = mcEv.convertToString(partColArray, iter.isLast());
     }

     if ( !evInStr )
        return NULL;

     // Construct a boundary as ItemExprList of ConstValues 
     ItemExpr* rangePartBoundValues = getRangePartitionBoundaryValues(
          evInStr->data(), evInStr->length(), heap, CharInfo::ISO88591);

     NAString totalEncodedKeyBuf;
     ItemExpr* val = NULL;
     ItemExpr* encodeExpr = NULL ;
   
     ItemExprList* list = NULL;
     list = new (heap) ItemExprList(rangePartBoundValues, heap,ITM_ITEM_LIST,FALSE);

     for (CollIndex c = 0; c < partColArray.entries(); c++)
     {
         NAColumn* ncol = partColArray[c];
         const NAType* nt = ncol->getType();
         
         if (rangePartBoundValues->getOperatorType() == ITM_ITEM_LIST ) 
            val = (ItemExpr*) (*list) [c];
          else
            val = (ItemExpr*) (*list) [0];

         if (nt->isEncodingNeeded())
            encodeExpr = new(heap) CompEncode(val, !(partColArray.isAscending(c)));
         else
            encodeExpr = val;

         encodeExpr->synthTypeAndValueId();
         const NAType& eeNT = encodeExpr->getValueId().getType();
         ULng32 encodedKeyLength = eeNT.getEncodedKeyLength();

         char* encodedKeyBuffer = new (heap) char[encodedKeyLength];

         Lng32 offset;
         Lng32 length;
         ValueIdList vidList;
    
         short ok = vidList.evaluateTree(encodeExpr,
                                    encodedKeyBuffer,
                                    encodedKeyLength,
                                    &length,
                                    &offset,
                                    (CmpCommon::diags()));

         totalEncodedKeyLength += encodedKeyLength;
         totalEncodedKeyBuf += encodedKeyBuffer;

         if ( ok != 0 ) 
            return NULL;
     }

     char* char_totalEncodedKeyBuf =new char[totalEncodedKeyLength];
     memcpy (char_totalEncodedKeyBuf, totalEncodedKeyBuf.data(), totalEncodedKeyLength);

     if (totalEncodedKeyLength != 0)
     {
        partBounds->defineUnboundBoundary(
              counter++,
              rangePartBoundValues,
              char_totalEncodedKeyBuf);

     }

     iter.next();
  }

  // ---------------------------------------------------------------------
  // Before doing consistency check setup for the statement
  // ---------------------------------------------------------------------
  partBounds->setupForStatement(FALSE);

  // ---------------------------------------------------------------------
  // Perform a consistency check to ensure that a boundary was defined
  // for each partition.
  // ---------------------------------------------------------------------
  partBounds->checkConsistency(numberOfPartitions);

  // -----------------------------------------------------------------
  // Add the first and the last boundary (0 and numberOfPartitions)
  // at the ends that do not separate two partitions
  // -----------------------------------------------------------------
   partBounds->completePartitionBoundaries(
          partitioningKeyColumnsOrder,
          totalEncodedKeyLength);

  return partBounds;

} // createRangePartitionBoundariesFromStats()
#pragma warn(1506)  // warning elimination

static 
PartitioningFunction*
createRangePartitioningFunctionForSingleRegionHBase(
			                const NAColumnArray & partKeyColArray,
                                        NAMemory* heap
                                                   )
{
   NodeMap* nodeMap = NULL;

   Lng32 regionsToFake = 
      (ActiveSchemaDB()->getDefaults()).getAsLong(HBASE_USE_FAKED_REGIONS);

   if ( regionsToFake == 0 ) {

     nodeMap = new (heap) 
            NodeMap(heap, 1, NodeMapEntry::ACTIVE, NodeMap::HBASE);

     return new (heap) SinglePartitionPartitioningFunction(nodeMap, heap);
   }

   nodeMap = new (heap) 
            NodeMap(heap, regionsToFake, NodeMapEntry::ACTIVE, NodeMap::HBASE);

   //
   // Setup an array of doubles to record the next begin key value for
   // each key column. Needed when the table has a single region.
   // The number ranges is controlled by CQD HBASE_USE_FAKED_REGIONS.
   //
   // Later on, we can make smart split utilizing the stats. 
   // 
   Int32 keys = partKeyColArray.entries();

   double* firstkeys = new (heap) double[keys];
   double* steps = new (heap) double[keys];

   for ( Int32 i=0; i<keys; i++ ) {

       double min = partKeyColArray[i]->getType()->getMinValue();
       double max = partKeyColArray[i]->getType()->getMaxValue();

       firstkeys[i] = partKeyColArray[i]->getType()->getMinValue();
       steps[i] = (max - min) / regionsToFake;
   }


   struct desc_struct* head = NULL;
   struct desc_struct* tail = NULL;

   Int32 i=0;
   for ( i=0; i<regionsToFake; i++ ) {

     if ( tail == NULL ) {
        head = tail = new (heap) struct desc_struct;

        // to satisfy createRangePartitionBoundaries() in NATable.cpp
        tail->body.partns_desc.primarypartition = 1;

     } else {
        tail->header.next = new (heap) struct desc_struct;
        tail = tail->header.next;
     }

     NAString firstkey('(');
     for ( Int32 i=0; i<keys; i++ ) {
         double v = firstkeys[i];
         NAString* v_str = partKeyColArray[i]->getType()->convertToString(v,heap);

        // If for some reason we can not make the conversion, we 
         // return a single-part func.
         if ( !v_str ) {
            nodeMap = new (heap)
                   NodeMap(heap, 1, NodeMapEntry::ACTIVE, NodeMap::HBASE);
            return new (heap) SinglePartitionPartitioningFunction(nodeMap, heap);
         }

         firstkey.append(*v_str);

         if ( i < keys-1 )
           firstkey.append(',');

         // Prepare for the next range
         firstkeys[i] += steps[i];
     }
     firstkey.append(')');


     Int32 len = firstkey.length();

     tail->body.partns_desc.firstkeylen = len;
     tail->body.partns_desc.firstkey = new (heap) char[len];
     memcpy(tail->body.partns_desc.firstkey, firstkey.data(), len);

     // For now, assume firstkey == encodedkey
     tail->body.partns_desc.encodedkeylen = len;
     tail->body.partns_desc.encodedkey = new (heap) char[len];
     memcpy(tail->body.partns_desc.encodedkey, firstkey.data(), len);

   }

   // 
   return createRangePartitioningFunction
                                (head,
                                 partKeyColArray,
                                 nodeMap,
                                 heap);
}

static 
PartitioningFunction*
createRangePartitioningFunctionForMultiRegionHBase(Int32 partns,
                                        desc_struct* desc, 
			                const NAColumnArray & partKeyColArray,
                                        NAMemory* heap)
{
   desc_struct* hrk = desc;
   desc_struct* prevEndKey = NULL;
 
   NodeMap* nodeMap = createNodeMapForHbase(desc, heap);

   struct desc_struct* head = NULL;
   struct desc_struct* tail = NULL;

   Int32 i=0;
   while ( hrk ) {

     struct desc_struct *newNode = new (heap) struct desc_struct;
     memset(&newNode->header, 0, sizeof(newNode->header));
     memset(&newNode->body.partns_desc, 0, sizeof(tail->body.partns_desc));
     newNode->header.nodetype = DESC_PARTNS_TYPE;

     if ( tail == NULL ) {
        head = tail = newNode;

        // to satisfy createRangePartitionBoundaries() in NATable.cpp
        tail->body.partns_desc.primarypartition = 1;

     } else {
        tail->header.next = newNode;
        tail = tail->header.next;
     }

     if (!prevEndKey) {
       // the start key of the first partitions has all zeroes in it
       Int32 len = hrk->body.hbase_region_desc.endKeyLen;

       tail->body.partns_desc.encodedkeylen = len;
       tail->body.partns_desc.encodedkey = new (heap) char[len];
       memset(tail->body.partns_desc.encodedkey, 0, len);
     }
     else {
       // the beginning key of this partition is the end key of
       // the previous one
       // (HBase returns end keys, we need begin keys here)
       Int32 len = prevEndKey->body.hbase_region_desc.endKeyLen;

       // For HBase regions, we don't have the text representation
       // (value, value, ... value) of the boundary, just the encoded
       // key.
       tail->body.partns_desc.encodedkeylen = len;
       tail->body.partns_desc.encodedkey = new (heap) char[len];
       memcpy(tail->body.partns_desc.encodedkey, 
              prevEndKey->body.hbase_region_desc.endKey, len);
     }

     prevEndKey = hrk;
     hrk     = hrk->header.next;
   }

   return createRangePartitioningFunction
                                (head,
                                 partKeyColArray,
                                 nodeMap,
                                 heap);
}


Int32 findDescEntries(desc_struct* desc)
{
   Int32 partns = 0;
   desc_struct* hrk = desc;
   while ( hrk ) {
     partns++;
     hrk = hrk->header.next;
   }
   return partns;
}

//
// A single entry point to figure out range partition function for
// Hbase. 
//
static 
PartitioningFunction*
createRangePartitioningFunctionForHBase(desc_struct* desc, 
			                const NAColumnArray & partKeyColArray,
                                        NAMemory* heap)
{

  Int32 partns = 0;
  if (CmpCommon::getDefault(HBASE_RANGE_PARTITIONING) != DF_OFF)
      // First figure out # partns
      partns = findDescEntries(desc);
  else
    partns = 1;

   return (partns > 1) ?
      createRangePartitioningFunctionForMultiRegionHBase(partns,
                                    desc, partKeyColArray, heap)
       :
      createRangePartitioningFunctionForSingleRegionHBase(
                                    partKeyColArray, heap);
}

static PartitioningFunction * createHivePartitioningFunction
                                (Int32 numberOfPartitions,
                                 const NAColumnArray & partKeyColArray,
                                 NodeMap* nodeMap,
                                 NAMemory* heap)
{
  // ---------------------------------------------------------------------
  // Each table has at least 1 partition
  // ---------------------------------------------------------------------
  if (numberOfPartitions == 1)
    return new (heap) SinglePartitionPartitioningFunction(nodeMap, heap);

  return new (heap) HivePartitioningFunction(numberOfPartitions, nodeMap, heap);

} // static createHivePartitioningFunction()

// -----------------------------------------------------------------------
// createNodeMap()
// This method is used for creating a node map for all DP2 partitions of
// associated with this table or index.
// -----------------------------------------------------------------------
#pragma nowarn(1506)   // warning elimination
static void createNodeMap (desc_struct* part_desc_list,
		           NodeMap*     nodeMap,
                           NAMemory*    heap,
                           char * tableName,
                           Int32 tableIdent)
{
  // ---------------------------------------------------------------------
  // Loop over all partitions creating a DP2 node map entry for each
  // partition.
  // ---------------------------------------------------------------------
  desc_struct* partns_desc      = part_desc_list;
  CollIndex    currentPartition = 0;
  if(NOT partns_desc)
  {
    NodeMapEntry entry =
          NodeMapEntry(tableName,NULL,heap,tableIdent);
    nodeMap->setNodeMapEntry(currentPartition,entry,heap);
  }
  else{
    while (partns_desc)
    {
      NodeMapEntry entry(partns_desc->body.partns_desc.partitionname,
	partns_desc->body.partns_desc.givenname,
        heap,tableIdent);
      nodeMap->setNodeMapEntry(currentPartition,entry,heap);
      partns_desc = partns_desc->header.next;
      currentPartition++;
    }
  }
  // -------------------------------------------------------------------
  //  If no partitions supplied, create a single partition node map with
  // a dummy entry.
  // -------------------------------------------------------------------
  if (nodeMap->getNumEntries() == 0)
    {

      NodeMapEntry entry(NodeMapEntry::ACTIVE);
      nodeMap->setNodeMapEntry(0,entry,heap);

    }

  // -------------------------------------------------------------------
  // Set the tableIndent into the nodemap itself.
  // -------------------------------------------------------------------
  nodeMap->setTableIdent(tableIdent);

  // -----------------------------------------------------------------------
  //  See if we need to build a bogus node map with fake volume assignments.
  // This will allow us to fake costing code into believing that all
  // partitions are distributed evenly among SMP nodes in the cluster.
  // -----------------------------------------------------------------------
  if (CmpCommon::getDefault(FAKE_VOLUME_ASSIGNMENTS) == DF_ON)
    {

      // --------------------------------------------------------------------
      //  Extract number of SMP nodes in the cluster from the defaults table.
      // --------------------------------------------------------------------
      NADefaults &defs     = ActiveSchemaDB()->getDefaults();
      CollIndex  numOfSMPs =  gpClusterInfo->numOfSMPs();

      if(CURRSTMT_OPTDEFAULTS->isFakeHardware())
      {
        numOfSMPs = defs.getAsLong(DEF_NUM_NODES_IN_ACTIVE_CLUSTERS);
      }


      // ------------------------------------------------------------------
      //  Determine how many node map entries will be assigned a particular
      // node, and also calculate if there are any remaining entries.
      // ------------------------------------------------------------------
      CollIndex entriesPerNode   = nodeMap->getNumEntries() / numOfSMPs;
      CollIndex entriesRemaining = nodeMap->getNumEntries() % numOfSMPs;

      // ----------------------------------------------------------------
      //  Assign each node to consecutive entries such that each node has
      // approximately the same number of entries.
      //
      //  Any extra entries get assigned evenly to the last remaining
      // nodes.  For example, if the cluster has 5 nodes and the node map
      // has 23 entries, we would assign nodes to entries as follows:
      //
      //  Entries  0 -  3 to node 0. (4 entries)
      //  Entries  4 -  7 to node 1. (4 entries)
      //  Entries  8 - 12 to node 2. (5 entries)
      //  Entries 13 - 17 to node 3. (5 entries)
      //  Entries 18 - 22 to node 4. (5 entries)
      // ----------------------------------------------------------------
      CollIndex mapIdx = 0;
      for (CollIndex nodeIdx = 0; nodeIdx < numOfSMPs; nodeIdx++)
        {
          if (nodeIdx == numOfSMPs - entriesRemaining)
            {
              entriesPerNode += 1;
            }

          for (CollIndex entryIdx = 0; entryIdx < entriesPerNode; entryIdx++)
            {
              nodeMap->setNodeNumber(mapIdx,nodeIdx);
              mapIdx += 1;
            }
        }

    }

} // static createNodeMap()
#pragma warn(1506)  // warning elimination

#pragma nowarn(1506)   // warning elimination
static void createNodeMap (hive_tbl_desc* hvt_desc,
		           NodeMap*     nodeMap,
                           NAMemory*    heap,
                           char * tableName,
                           Int32 tableIdent)
{

  // ---------------------------------------------------------------------
  // Loop over all hive storage (partition file ) creating a node map 
  // entry for each partition.
  // ---------------------------------------------------------------------

  CMPASSERT(nodeMap->type() == NodeMap::HIVE);
  hive_sd_desc* sd_desc = hvt_desc->getSDs();

  CollIndex    currentPartition = 0;

  //  char buf[500];
  Int32 i= 0;
  while (sd_desc)
    {
      HiveNodeMapEntry entry(NodeMapEntry::ACTIVE, heap);
      nodeMap->setNodeMapEntry(currentPartition++,entry,heap);
      sd_desc = sd_desc->next_;
    }

  // -------------------------------------------------------------------
  //  If no partitions supplied, create a single partition node map with
  // a dummy entry.
  // -------------------------------------------------------------------
  if (nodeMap->getNumEntries() == 0)
    {

      HiveNodeMapEntry entry(NodeMapEntry::ACTIVE, heap);
      nodeMap->setNodeMapEntry(0,entry,heap);

    }

  // -------------------------------------------------------------------
  // Set the tableIndent into the nodemap itself.
  // -------------------------------------------------------------------
  nodeMap->setTableIdent(tableIdent);

  // No fake volumn assignment because Hive' partitions are not hash
  // based, there is no balance of data among all partitions.
} // static createNodeMap()
#pragma warn(1506)  // warning elimination

//-------------------------------------------------------------------------
// This function checks if a table/index or any of its partitions are
// remote. This is required to determine the size of the EidRootBuffer
// to be sent to DP2 - Expand places limits on the size of messages
// - approx 31000 for messages to remote nodes, and 56000 for messages
// on the local node.
//-------------------------------------------------------------------------
#pragma nowarn(262)   // warning elimination
static NABoolean checkRemote(desc_struct* part_desc_list,
                             char * tableName)
{
  if (!OSIM_isNSKbehavior())
    return TRUE;

  return FALSE;
}
#pragma warn(262)  // warning elimination


// warning elimination (removed "inline")
static NAString makeTableName(const NATable *table,
			      const columns_desc_struct *column_desc)
{
  return NAString(
	      table ?
		table->getTableName().getQualifiedNameAsAnsiString().data() :
	      column_desc->tablename ?
		column_desc->tablename : "");
}
// warning elimination (removed "inline")
static NAString makeColumnName(const NATable *table,
			       const columns_desc_struct *column_desc)
{
  NAString nam(makeTableName(table, column_desc));
  if (!nam.isNull()) nam += ".";
  nam += column_desc->colname;
  return nam;
}

// -----------------------------------------------------------------------
// Method for creating NAType from desc_struct.
// -----------------------------------------------------------------------
NABoolean createNAType(columns_desc_struct *column_desc	/*IN*/,
		       const NATable *table  		/*IN*/,
		       NAType *&type       		/*OUT*/,
		       NAMemory *heap			/*IN*/,
		       Lng32 * errorCode
		       )
{
  //
  // Compute the NAType for this column
  //
  #define REC_INTERVAL REC_MIN_INTERVAL

  DataType datatype = column_desc->datatype;
  if (REC_MIN_INTERVAL <= datatype && datatype <= REC_MAX_INTERVAL)
    datatype = REC_INTERVAL;

  Lng32 charCount = column_desc->length;

  if ( DFS2REC::isAnyCharacter(column_desc->datatype) )
  {
     if ( CharInfo::isCharSetSupported(column_desc->character_set) == FALSE ) {
       if (!errorCode)
       {
         *CmpCommon::diags() << DgSqlCode(-4082)
	       << DgTableName(makeTableName(table, column_desc));
       }
       else
       {
         *errorCode = 4082;
       }
       return TRUE; // error
     }

     if ( CharInfo::is_NCHAR_MP(column_desc->character_set) )
        charCount /= SQL_DBCHAR_SIZE;
  }

  switch (datatype)
    {

    case REC_BPINT_UNSIGNED :
      type = new (heap)
      SQLBPInt(column_desc->precision, column_desc->null_flag, FALSE, heap);
      break;

    case REC_BIN16_SIGNED:
      if (column_desc->precision > 0)
	type = new (heap)
	SQLNumeric(column_desc->length,
		   column_desc->precision,
		   column_desc->scale,
		   TRUE,
		   column_desc->null_flag,
                   heap
		   );
      else
	type = new (heap)
	SQLSmall(TRUE,
		 column_desc->null_flag,
                 heap
		 );
      break;
    case REC_BIN16_UNSIGNED:
      if (column_desc->precision > 0)
	type = new (heap)
	SQLNumeric(column_desc->length,
		   column_desc->precision,
		   column_desc->scale,
		   FALSE,
		   column_desc->null_flag,
                   heap
		   );
      else
	type = new (heap)
	SQLSmall(FALSE,
		 column_desc->null_flag,
                 heap
		 );
      break;
    case REC_BIN32_SIGNED:
      if (column_desc->precision > 0)
	type = new (heap)
	SQLNumeric(column_desc->length,
		   column_desc->precision,
		   column_desc->scale,
		   TRUE,
		   column_desc->null_flag,
                   heap
		   );
      else
	type = new (heap)
	SQLInt(TRUE,
	       column_desc->null_flag,
               heap
	       );
      break;
    case REC_BIN32_UNSIGNED:
      if (column_desc->precision > 0)
	type = new (heap)
	SQLNumeric(column_desc->length,
		   column_desc->precision,
		   column_desc->scale,
		   FALSE,
		   column_desc->null_flag,
                   heap
		   );
      else
	type = new (heap)
	SQLInt(FALSE,
	       column_desc->null_flag,
               heap
	       );
      break;
    case REC_BIN64_SIGNED:
      if (column_desc->precision > 0)
	type = new (heap)
	SQLNumeric(column_desc->length,
		   column_desc->precision,
		   column_desc->scale,
		   TRUE,
		   column_desc->null_flag,
                   heap
		   );
      else
	type = new (heap)
	SQLLargeInt(TRUE,
		    column_desc->null_flag,
                    heap
		    );
      break;
    case REC_DECIMAL_UNSIGNED:
      type = new (heap)
	SQLDecimal(column_desc->length,
		   column_desc->scale,
		   FALSE,
		   column_desc->null_flag,
                   heap
		   );
      break;
    case REC_DECIMAL_LSE:
      type = new (heap)
	SQLDecimal(column_desc->length,
		   column_desc->scale,
		   TRUE,
		   column_desc->null_flag,
                   heap
		   );
      break;
    case REC_NUM_BIG_UNSIGNED:
      type = new (heap)
	SQLBigNum(column_desc->precision,
		  column_desc->scale,
		  TRUE, // is a real bignum
		  FALSE,
		  column_desc->null_flag,
		  heap
		  );
      break;
    case REC_NUM_BIG_SIGNED:
      type = new (heap)
	SQLBigNum(column_desc->precision,
		  column_desc->scale,
		  TRUE, // is a real bignum
		  TRUE,
		  column_desc->null_flag,
		  heap
		  );
      break;

    case REC_TDM_FLOAT32:
      type = new (heap)
	SQLRealTdm(column_desc->null_flag, heap, column_desc->precision);
      break;

    case REC_TDM_FLOAT64:
      type = new (heap)
	SQLDoublePrecisionTdm(column_desc->null_flag, heap, column_desc->precision);
      break;

    case REC_FLOAT32:
      type = new (heap)
	SQLReal(column_desc->null_flag, heap, column_desc->precision);
      break;

    case REC_FLOAT64:
      type = new (heap)
	SQLDoublePrecision(column_desc->null_flag, heap, column_desc->precision);
      break;

    case REC_BYTE_F_DOUBLE:
      charCount /= SQL_DBCHAR_SIZE;	    // divide the storage length by 2
      type = new (heap)
	SQLChar(charCount,
		column_desc->null_flag,
		column_desc->upshift,
		column_desc->caseinsensitive,
		FALSE,
		column_desc->character_set,
		column_desc->collation_sequence,
		CharInfo::IMPLICIT
		);
      break;

    case REC_BYTE_F_ASCII:
      if (column_desc->character_set == CharInfo::UTF8 ||
          (column_desc->character_set == CharInfo::SJIS &&
           column_desc->encoding_charset == CharInfo::SJIS))
      {
        Lng32 maxBytesPerChar = CharInfo::maxBytesPerChar(column_desc->character_set);
        Lng32 sizeInChars = charCount ;  // Applies when CharLenUnit == BYTES
        if ( column_desc->precision > 0 )
           sizeInChars = column_desc->precision;
        type = new (heap)
	SQLChar(CharLenInfo(sizeInChars, charCount/*in_bytes*/),
		column_desc->null_flag,
		column_desc->upshift,
		column_desc->caseinsensitive,
		FALSE, // varLenFlag
		column_desc->character_set,
		column_desc->collation_sequence,
		CharInfo::IMPLICIT, // Coercibility
		column_desc->encoding_charset
		);
      }
      else // keep the old behavior
      type = new (heap)
	SQLChar(charCount,
		column_desc->null_flag,
		column_desc->upshift,
		column_desc->caseinsensitive,
		FALSE,
		column_desc->character_set,
		column_desc->collation_sequence,
		CharInfo::IMPLICIT
		);
      break;

    case REC_BYTE_V_DOUBLE:
      charCount /= SQL_DBCHAR_SIZE;	    // divide the storage length by 2
      // fall thru
    case REC_BYTE_V_ASCII:
      if (column_desc->character_set == CharInfo::SJIS ||
          column_desc->character_set == CharInfo::UTF8)
      {
        Lng32 maxBytesPerChar = CharInfo::maxBytesPerChar(column_desc->character_set);
        Lng32 sizeInChars = charCount ;  // Applies when CharLenUnit == BYTES
        if ( column_desc->precision > 0 )
           sizeInChars = column_desc->precision;
        type = new (heap)
	SQLVarChar(CharLenInfo(sizeInChars, charCount/*in_bytes*/),
		   column_desc->null_flag,
		   column_desc->upshift,
		   column_desc->caseinsensitive,
		   column_desc->character_set,
		   column_desc->collation_sequence,
		   CharInfo::IMPLICIT, // Coercibility
		   column_desc->encoding_charset
		   );
      }
      else // keep the old behavior
      type = new (heap)
	SQLVarChar(charCount,
		   column_desc->null_flag,
		   column_desc->upshift,
		   column_desc->caseinsensitive,
		   column_desc->character_set,
		   column_desc->collation_sequence,
		   CharInfo::IMPLICIT
		   );
      break;

    case REC_BYTE_V_ASCII_LONG:
      type = new (heap)
	SQLLongVarChar(charCount,
		       FALSE,
		       column_desc->null_flag,
		       column_desc->upshift,
		       column_desc->caseinsensitive,
		       column_desc->character_set,
		       column_desc->collation_sequence,
		       CharInfo::IMPLICIT
		      );
      break;
    case REC_DATETIME:
      type = DatetimeType::constructSubtype(
					    column_desc->null_flag,
					    column_desc->datetimestart,
					    column_desc->datetimeend,
					    column_desc->datetimefractprec,
					    heap
					    );
      CMPASSERT(type);
      if (!type->isSupportedType())
	{
         column_desc->defaultClass = COM_NO_DEFAULT;           // can't set a default for these, either.
	  // 4030 Column is an unsupported combination of datetime fields
     if (!errorCode)
     {
         *CmpCommon::diags() << DgSqlCode(4030)
	    << DgColumnName(makeColumnName(table, column_desc))
	    << DgInt0(column_desc->datetimestart)
	    << DgInt1(column_desc->datetimeend)
	    << DgInt2(column_desc->datetimefractprec);
     }
     else
     {
       *errorCode = 4030;
     }
	}
      break;
    case REC_INTERVAL:
      type = new (heap)
         SQLInterval(column_desc->null_flag,
		    column_desc->datetimestart,
		    column_desc->intervalleadingprec,
		    column_desc->datetimeend,
		    column_desc->datetimefractprec,
                    heap
		    );
      CMPASSERT(type);
      if (! ((SQLInterval *)type)->checkValid(CmpCommon::diags()))
         return TRUE;                                            // error
      if (!type->isSupportedType())
      {
        column_desc->defaultClass = COM_NO_DEFAULT;           // can't set a default for these, either.
        if (!errorCode)
          *CmpCommon::diags() << DgSqlCode(3044) << DgString0(column_desc->colname);
        else
          *errorCode = 3044;

      }
      break;

    case REC_BLOB :
      type = new (heap)
	SQLBlob(column_desc->precision,Lob_Invalid_Storage,
		column_desc->null_flag);
      break;

    case REC_CLOB :
      type = new (heap)
	SQLClob(column_desc->precision,Lob_Invalid_Storage,
		column_desc->null_flag);
      break;

    default:
      {
	// 4031 Column %s is an unknown data type, %d.
        if (!errorCode)
        {
	*CmpCommon::diags() << DgSqlCode(-4031)
	  << DgColumnName(makeColumnName(table, column_desc))
	  << DgInt0(column_desc->datatype);
        }
        else
        {
          *errorCode = 4031;
        }
	return TRUE;						// error
      }
    } // end switch (column_desc->datatype)

  CMPASSERT(type);

  if (type->getTypeQualifier() == NA_CHARACTER_TYPE) {
    CharInfo::Collation co = ((CharType *)type)->getCollation();

    // a "mini-cache" to avoid proc call, for perf
    static THREAD_P CharInfo::Collation cachedCO = CharInfo::UNKNOWN_COLLATION;
    static THREAD_P Int32         cachedFlags = CollationInfo::ALL_NEGATIVE_SYNTAX_FLAGS;

    if (cachedCO != co) {
      cachedCO = co;
      cachedFlags = CharInfo::getCollationFlags(co);
    }

    if (cachedFlags & CollationInfo::ALL_NEGATIVE_SYNTAX_FLAGS) {
      //
      //## The NCHAR/COLLATE NSK-Rel1 project is forced to disallow all user-
      //	defined collations here.  What we can't handle is:
      //	- full support!  knowledge of how to really collate!
      //	- safe predicate-ability of collated columns, namely
      //	  . ORDER/SEQUENCE/SORT BY
      //	    MIN/MAX
      //	    < <= > >=
      //		These *would* have been disallowed by the
      //		CollationInfo::ORDERED_CMP_ILLEGAL flag.
      //	  . DISTINCT
      //	    GROUP BY
      //	    = <>
      //		These *would* have been disallowed by the
      //		CollationInfo::EQ_NE_CMP_ILLEGAL flag.
      //	  . SELECTing a collated column which is a table or index key
      //		We *would* have done full table scan only, based on flag
      //	  . INS/UPD/DEL involving a collated column which is a key
      //		We *would* have had to disallow this, based on flag;
      //		otherwise we would position in wrong and corrupt either
      //		our partitioning or b-trees or both.
      //	See the "MPcollate.doc" document, and
      //	see sqlcomp/DefaultValidator.cpp ValidateCollationList comments.
      //
	{
	  // 4069 Column TBL.COL uses unsupported collation COLLAT.
	  if (!errorCode)
	  {
	  *CmpCommon::diags() << DgSqlCode(-4069)
	    << DgColumnName(makeColumnName(table, column_desc));
	  }
	  else
	  {
	    *errorCode= 4069;
	  }
	  return TRUE;						// error
	}
    }
  }

  return FALSE;							// no error

} // createNAType()

// -----------------------------------------------------------------------
// Method for inserting new NAColumn entries in NATable::colArray_,
// one for each column_desc in the list supplied as input.
// -----------------------------------------------------------------------
#pragma nowarn(1506)   // warning elimination
NABoolean createNAColumns(desc_struct *column_desc_list	/*IN*/,
			  NATable *table		/*IN*/,
			  NAColumnArray &colArray	/*OUT*/,
			  NAMemory *heap		/*IN*/)
{
  NAType *type;
  ColumnClass colClass;
  while (column_desc_list)
    {
      columns_desc_struct * column_desc = &column_desc_list->body.columns_desc;
      NABoolean isMvSystemAdded = FALSE;
      NABoolean hasSystemColumnAsUserColumn = FALSE;

      if (NAColumn::createNAType(column_desc, table, type, heap))
	return TRUE;

      // Get the column class.  The column will either be a system column or a
      // user column.
      //
      switch (column_desc->colclass)
	{
	case 'S':
	  {
	    if ( (CmpCommon::getDefault(OVERRIDE_SYSKEY)==DF_ON) &&
		 (table && table->getSpecialType() != ExtendedQualName::VIRTUAL_TABLE) )
	      {
	      colClass = USER_COLUMN;
	      hasSystemColumnAsUserColumn = TRUE;
	      }
	    else
	      colClass = SYSTEM_COLUMN;
	  }
	  break;
	case 'U':
	  colClass = USER_COLUMN;
	  break;
        case 'A':
	  colClass = USER_COLUMN;
	  break;
        case 'M':  // MVs --
	  colClass = USER_COLUMN;
	  isMvSystemAdded = TRUE;
	  break;
	default:
	  {
            // 4032 column is an unknown class (not sys nor user)
            *CmpCommon::diags() << DgSqlCode(-4032)
	      << DgColumnName(makeColumnName(table, column_desc))
	      << DgInt0(column_desc->colclass);
	    return TRUE;					// error
	  }
	} // end switch (column_desc->colclass)

      // Create an NAColumn and insert it into the NAColumn array.
      //
      NAColumn *newColumn = NULL;
      if (column_desc->colname[0] != '\0')
        {
	  // Standard named column from ReadTableDef...
          CMPASSERT(column_desc->colnumber >= 0);

         char* defaultValue = column_desc->defaultvalue;
         char* heading = column_desc->heading;
         char* computed_column_text = column_desc->computed_column_text;
         NABoolean isSaltColumn = FALSE;
         NABoolean isDivisioningColumn = FALSE;

         if (column_desc->defaultClass == COM_ALWAYS_COMPUTE_COMPUTED_COLUMN_DEFAULT)
           {
             if (column_desc->colFlags & SEABASE_COLUMN_IS_SALT)
               isSaltColumn = TRUE;
             if (column_desc->colFlags & SEABASE_COLUMN_IS_DIVISION)
               isDivisioningColumn = TRUE;
             if (!computed_column_text)
               {
                 computed_column_text = defaultValue;
                 defaultValue = NULL;
               }
           }

         if(ActiveSchemaDB()->getNATableDB()->cachingMetaData()){
           //make copies of stuff onto the heap passed in
           if(defaultValue){
             defaultValue = (char*) new (heap) char[strlen(defaultValue)+1];
             strcpy(defaultValue, column_desc->defaultvalue);
           }

           if(heading){
             Int32 headingLength = str_len(heading)+1;
             heading = new (heap) char [headingLength];
             memcpy(heading,column_desc->heading,headingLength);
           }

           if(computed_column_text){
             char * computed_column_text_temp = computed_column_text;
             Int32 cctLength = str_len(computed_column_text)+1;
             computed_column_text = new (heap) char [cctLength];
             memcpy(computed_column_text,computed_column_text_temp,cctLength);
           }
         }

         newColumn = new (heap)
		      NAColumn(column_desc->colname,
			       column_desc->colnumber,
			       type,
                               heap,
			       table,
			       colClass,
			       column_desc->defaultClass,
			       defaultValue,
                               heading,
			       column_desc->upshift,
			       (column_desc->colclass == 'A'),
                               COM_UNKNOWN_DIRECTION,
                               FALSE,
                               NULL,
                               column_desc->stored_on_disk,
                               computed_column_text,
                               isSaltColumn,
                               isDivisioningColumn);
	}
      else
        {
          CMPASSERT(0);
	}
      
      if (isMvSystemAdded)
	newColumn->setMvSystemAddedColumn();

      if (table &&
	  ((table->isSeabaseTable()) ||
	   (table->isHbaseCellTable()) ||
	   (table->isHbaseRowTable())))
	{
	  if (column_desc->hbaseColFam)
	    newColumn->setHbaseColFam(column_desc->hbaseColFam);
	  if (column_desc->hbaseColQual)
	    newColumn->setHbaseColQual(column_desc->hbaseColQual);

	  newColumn->setHbaseColFlags(column_desc->hbaseColFlags);
	}
      
      if (table != NULL)
	{
	  if (newColumn->isAddedColumn())
	    table->setHasAddedColumn(TRUE);

	  if (newColumn->getType()->isVaryingLen())
	    table->setHasVarcharColumn(TRUE);

	  if (hasSystemColumnAsUserColumn)
	    table->setSystemColumnUsedAsUserColumn(TRUE) ;

	  if (newColumn->getType()->isLob())
	    table->setHasLobColumn(TRUE);

	  if (CmpSeabaseDDL::isEncodingNeededForSerialization(newColumn))
	    table->setHasSerializedColumn(TRUE);
	}

      colArray.insert(newColumn);

      column_desc_list = column_desc_list->header.next;
    } // end while

  return FALSE;							// no error

} // createNAColumns()
#pragma warn(1506)  // warning elimination
      
NAType* getSQColTypeForHive(const char* hiveType, NAMemory* heap)
{
  if ( !strcmp(hiveType, "tinyint") || 
       !strcmp(hiveType, "smallint")) 
    return new (heap) SQLSmall(TRUE /* neg */, TRUE /* allow NULL*/, heap);
 
  if ( !strcmp(hiveType, "int")) 
    return new (heap) SQLInt(TRUE /* neg */, TRUE /* allow NULL*/, heap);

  if ( !strcmp(hiveType, "bigint"))
    return new (heap) SQLLargeInt(TRUE /* neg */, TRUE /* allow NULL*/, heap);

  if ( !strcmp(hiveType, "string"))
    {
      Int32 len = CmpCommon::getDefaultLong(HIVE_MAX_STRING_LENGTH);
      NAString hiveCharset =
        ActiveSchemaDB()->getDefaults().getValue(HIVE_DEFAULT_CHARSET);
      return new (heap) SQLVarChar(CharLenInfo((hiveCharset == CharInfo::UTF8 ? 0 : len),len),
                                   TRUE, // allow NULL
                                   FALSE, // not upshifted
                                   FALSE, // not case-insensitive
                                   CharInfo::getCharSetEnum(hiveCharset));
    }
  
  if ( !strcmp(hiveType, "float"))
    return new (heap) SQLReal(TRUE /* allow NULL*/, heap);

  if ( !strcmp(hiveType, "double"))
    return new (heap) SQLDoublePrecision(TRUE /* allow NULL*/, heap);

  if ( !strcmp(hiveType, "timestamp"))
    return new (heap) SQLTimestamp(TRUE /* allow NULL */ , 6, heap);

  return NULL;
}

NABoolean createNAColumns(struct hive_column_desc* hcolumn /*IN*/,
			  NATable *table		/*IN*/,
			  NAColumnArray &colArray	/*OUT*/,
			  NAMemory *heap		/*IN*/)
{
  // Assume that hive_struct->conn has the right connection,
  // and tblID and sdID has be properly set.
  // In the following loop, we need to extract the column information.


   while (hcolumn) {

      NAType* natype = getSQColTypeForHive(hcolumn->type_, heap);

      if ( !natype ) {
	*CmpCommon::diags()
	  << DgSqlCode(-1204)
	  << DgString0(hcolumn->type_);
         return TRUE;
      }

      NAString colName(hcolumn->name_);
      colName.toUpper();

      NAColumn* newColumn = new (heap)
                    NAColumn(colName.data(),
                             hcolumn->intIndex_,
                             natype,
                             heap,
                             table,
                             USER_COLUMN, // colClass,
                             COM_NULL_DEFAULT  ,//defaultClass,
                             (char*)"", // defaultValue,
                             (char*)"", // heading,
                             FALSE, // column_desc->upshift,
                             FALSE, // added column
                             COM_UNKNOWN_DIRECTION,
                             FALSE,  // isOptional
                             NULL,  // routineParamType
                             TRUE, // column_desc->stored_on_disk,
                             (char*)"" //computed_column_text
                            );

      if (table != NULL)
      {
        if (newColumn->isAddedColumn())
          table->setHasAddedColumn(TRUE);

        if (newColumn->getType()->isVaryingLen())
          table->setHasVarcharColumn(TRUE);
      }

      colArray.insert(newColumn);

      hcolumn= hcolumn->next_;

    }

  return FALSE;							// no error

} // createNAColumns()
#pragma warn(1506)  // warning elimination



NABoolean createNAKeyColumns(desc_struct *keys_desc_list	/*IN*/,
			     NAColumnArray &colArray	/*IN*/,
			     NAColumnArray &keyColArray /*OUT*/,
			     CollHeap *heap		/*IN*/)
{
  const desc_struct *keys_desc = keys_desc_list;

  while (keys_desc)
    {
      Int32 tablecolnumber = keys_desc->body.keys_desc.tablecolnumber;

      NAColumn *indexColumn = colArray.getColumn(tablecolnumber);

      SortOrdering order = NOT_ORDERED;

      keyColArray.insert(indexColumn);
      order = keys_desc->body.keys_desc.ordering ? DESCENDING : ASCENDING;
      keyColArray.setAscending(keyColArray.entries()-1, order == ASCENDING);

      // Remember that this columns is part of the clustering
      // key and remember its key ordering (asc or desc)
      indexColumn->setClusteringKey(order);

      keys_desc = keys_desc->header.next;
    } // end while (keys_desc)

  return FALSE;
}

// ************************************************************************
// The next two methods are used for code related to indexes hiding.
// In particular, this is related to hiding remote indexes having the same
// name as the local name. Here we mark the remote indexes that have the
// same local name and in addition share the following:
//  (1) both share the same index columns
//  (2) both have the same partioning keys
//
// The method naStringHashFunc is used by the NAHashDictionary<NAString, Index>
// that maps indexname to the corresponding list of indexes having that name
//
//*************************************************************************
ULng32 naStringHashFunc(const NAString& indexName)
{
  ULng32 hash= (ULng32) NAString::hash(indexName);
  return hash;
}

//*************************************************************************
// The method processDuplicateNames() is called by createNAFileSet() for
// tables having duplicate remote indexes.
//*************************************************************************
// LCOV_EXCL_START :nsk
void processDuplicateNames(NAHashDictionaryIterator<NAString, Int32> &Iter,
                           NAFileSetList & indexes,
                           char *localNodeName)

{
  if (!OSIM_isNSKbehavior())
    return;
} // processDuplicateNames()
// LCOV_EXCL_STOP

// -----------------------------------------------------------------------
// Method for:
// -  inserting new NAFileSet entries in NATable::indexes_
//    one for each index in the list supplied as input. It also
//    returns a pointer to the NAFileSet for the clustering index
//    as well as the primary index on this NATable.
// -  inserting new NAFileSet entries in NATable::vertParts_
//    one for each vertical partition in the list supplied as input.
// -----------------------------------------------------------------------
#pragma nowarn(1506)   // warning elimination
static
NABoolean createNAFileSets(desc_struct * table_desc       /*IN*/,
                           const NATable * table          /*IN*/,
                           const NAColumnArray & colArray /*IN*/,
                           NAFileSetList & indexes        /*OUT*/,
                           NAFileSetList & vertParts      /*OUT*/,
                           NAFileSet * & clusteringIndex  /*OUT*/,
			   LIST(CollIndex) & tableIdList  /*OUT*/,
                           NAMemory* heap,
                           BindWA * bindWA,
                           NAColumnArray &newColumns, /*OUT */
			   Int32 *maxIndexLevelsPtr = NULL)
{
  // ---------------------------------------------------------------------
  // Add index/vertical partition (VP) information; loop over all indexes/
  // VPs, but start with the clustering key, then process all others.
  // The clustering key has a keytag 0.
  // ---------------------------------------------------------------------

  // this dictionary is used for hiding remote indexes; the remote indexes
  // are hidden when the CQD INDEX_ELIMINATION_LEVEL is set to aggressive
  NAHashDictionary<NAString, Int32> *indexFilesetMap =
    new (heap) NAHashDictionary<NAString, Int32>
        (naStringHashFunc, 101, FALSE, CmpCommon::statementHeap());

  NAList<NAString *> stringList (CmpCommon::statementHeap());

  desc_struct *indexes_desc = table_desc->body.table_desc.indexes_desc;

  while (indexes_desc AND indexes_desc->body.indexes_desc.keytag)
    indexes_desc = indexes_desc->header.next;

  // must have a clustering key if not view
  CMPASSERT((indexes_desc AND !indexes_desc->body.indexes_desc.keytag) OR
	    (table_desc->body.table_desc.views_desc));

  NABoolean isTheClusteringKey = TRUE;
  NABoolean isVerticalPartition;
  NABoolean hasRemotePartition = FALSE;
  CollIndex numClusteringKeyColumns = 0;

  // get hbase table index level and blocksize. costing code uses index_level
  // and block size to estimate cost. Here we make a JNI call to read index level
  // and block size. If there is a need to avoid reading from Hbase layer,
  // HBASE_INDEX_LEVEL cqd can be used to disable JNI call. User can
  // set this CQD to reflect desired index_level for his query.
  // Default value of HBASE_BLOCK_SIZE is 64KB, when not reading from Hbase layer. 
  Int32 hbtIndexLevels = 0;
  Int32 hbtBlockSize = 0;
  NABoolean res = false;
  if (table->isHbaseTable())
  {
    // get default values of index_level and block size
    hbtIndexLevels = (ActiveSchemaDB()->getDefaults()).getAsLong(HBASE_INDEX_LEVEL);
    hbtBlockSize = (ActiveSchemaDB()->getDefaults()).getAsLong(HBASE_BLOCK_SIZE);
    // call getHbaseTableInfo if index level is set to 0
    if (hbtIndexLevels == 0)
      res = table->getHbaseTableInfo(hbtIndexLevels, hbtBlockSize);
  }
    
  // Set up global cluster information.  This global information always
  // gets put on the context heap.
  //
  // Note that this function call will probably not do anything, since
  // this cluster information is set up when arkcmp is created; however,
  // it's certainly better to have this call here, rather than in a
  // doubly-nested loop below where it used to be ...

  // $$$ probably not necessary to call this even once ...
  setUpClusterInfo(CmpCommon::contextHeap());

  NABoolean doHash2 = 
      (CmpCommon::getDefault(HBASE_HASH2_PARTITIONING) != DF_OFF && 
       !(bindWA && bindWA->isTrafLoadPrep())); 

  // ---------------------------------------------------------------------
  // loop over all indexes/VPs defined on the base table
  // ---------------------------------------------------------------------
  while (indexes_desc)
    {
      Lng32 numberOfFiles = 1;	  	// always at least 1
      NAColumn * indexColumn;		// an index/VP key column
      NAColumn * newIndexColumn;
      NAFileSet * newIndex;		// a new file set
      //hardcoding statement heap here, previosly the following calls
      //used the heap that was passed in (which was always statement heap)
      //Now with the introduction of NATable caching we pass in the NATable
      //heap and these guys should not be created on the NATable heap, they
      //should be created on the statement heap. Only the array objects
      //will be on the statement heap whatever is in the arrays i.e. NAColumns
      //will still be where ever they were before.
      NAColumnArray allColumns(CmpCommon::statementHeap());// all columns that belong to an index
      NAColumnArray indexKeyColumns(CmpCommon::statementHeap());// the index key columns
      NAColumnArray saveNAColumns(CmpCommon::statementHeap());// save NAColums of secondary index columns
      NAColumnArray partitioningKeyColumns(CmpCommon::statementHeap());// the partitioning key columns
      PartitioningFunction * partFunc = NULL;
      // is this an index or is it really a VP?
      isVerticalPartition = indexes_desc->body.indexes_desc.isVerticalPartition;
      NABoolean isPacked = indexes_desc->body.indexes_desc.isPacked;

      NABoolean isNotAvailable =
	indexes_desc->body.indexes_desc.notAvailable;

      ItemExprList hbaseSaltColumnList(CmpCommon::statementHeap());
      Int64 numOfSaltedPartitions = 0;

      // ---------------------------------------------------------------------
      // loop over the clustering key columns of the index
      // ---------------------------------------------------------------------
      const desc_struct *keys_desc = indexes_desc->body.indexes_desc.keys_desc;
      while (keys_desc)
	{
          // Add an index/VP key column.
          //
          // If this is an alternate index or a VP, the keys table actually
          // describes all columns of the index or VP. For nonunique
	  // indexes, all index columns form the key, while for unique
	  // alternate indexes the last "numClusteringKeyColumns"
	  // columns are non-key columns, they are just the clustering
	  // key columns used to find the base table record. This is
	  // true for both SQL/MP and SQL/MX tables at this time.
	  // To make these assumptions is not optimal, but the
	  // desc_structs that are used as input are a historical
	  // leftover from SQL/MP and therefore aren't set up very
	  // well to describe index columns and index keys.  Some day
	  // we might consider a direct conversion from the MX catalog
	  // manager (SOL) objects into NATables and NAFilesets.
	  //
	  // NOTE:
	  // The last "numClusteringKeyColumns" key columns
	  // of a unique alternate index (which ARE described in the
	  // keys_desc) get deleted later.
          
	  Int32 tablecolnumber = keys_desc->body.keys_desc.tablecolnumber;
          indexColumn = colArray.getColumn(tablecolnumber);
          
          if ((table->isHbaseTable()) &&
              (indexes_desc->body.indexes_desc.keytag != 0))
            {
              newIndexColumn = new(heap) NAColumn(*indexColumn);
              newIndexColumn->setIndexColName(keys_desc->body.keys_desc.keyname);
              newIndexColumn->setHbaseColFam(keys_desc->body.keys_desc.hbaseColFam);
              newIndexColumn->setHbaseColQual(keys_desc->body.keys_desc.hbaseColQual);
              
              saveNAColumns.insert(indexColumn);
              newColumns.insert(newIndexColumn);
              indexColumn = newIndexColumn;
              
            }
          
          SortOrdering order = NOT_ORDERED;

          // as mentioned above, for all alternate indexes we
          // assume at first that all columns are key columns
          // and we make adjustments later
          indexKeyColumns.insert(indexColumn);
          order = keys_desc->body.keys_desc.ordering ?
            DESCENDING : ASCENDING;
          indexKeyColumns.setAscending(indexKeyColumns.entries() - 1,
                                       order == ASCENDING);
          
          if ( table->isHbaseTable() && 
               indexColumn->isComputedColumnAlways() ) 
            {
              
              // examples of the saltClause string:
              // 1. HASH2PARTFUNC(CAST(L_ORDERKEY AS INT NOT NULL) FOR 4)
              // 2. HASH2PARTFUNC(CAST(A AS INT NOT NULL),CAST(B AS INT NOT NULL) FOR 4) 
              const char* saltClause = indexColumn->getComputedColumnExprString();
              
              Parser parser(CmpCommon::context());
              ItemExpr* saltExpr = parser.getItemExprTree(saltClause, 
                                                          strlen(saltClause), 
                                                          CharInfo::ISO88591);
              
              if ( saltExpr &&
                   saltExpr->getOperatorType() == ITM_HASH2_DISTRIB) {
                
                // get the # of salted partitions from saltClause
                ItemExprList csList(CmpCommon::statementHeap());
                saltExpr->findAll(ITM_CONSTANT, csList, FALSE, FALSE);
                
                // get #salted partitions from last ConstValue in the list
                if ( csList.entries() > 0 ) {
                  ConstValue* ct = (ConstValue*)csList[csList.entries()-1];
                  
                  if ( ct->canGetExactNumericValue() )  {
                    numOfSaltedPartitions = ct->getExactNumericValue();
                  }
                }
              }
              
              // collect all ColReference objects into hbaseSaltColumnList.
              CMPASSERT(saltExpr != NULL);
              saltExpr->findAll(ITM_REFERENCE, hbaseSaltColumnList, FALSE, FALSE);
              
            }
          
	  if (isTheClusteringKey)
            {
              // Since many columns of the base table may not be in the
              // clustering key, we'll delay setting up the list of all
              // columns in the index until later, so we can then just
              // add them all at once.
              
              // Remember that this columns is part of the clustering
              // key and remember its key ordering (asc or desc)
	      indexColumn->setClusteringKey(order);
	      numClusteringKeyColumns++;
            }
	  else
	  {
            // Since all columns in the index are guaranteed to be in
            // the key, we can set up the list of all columns in the index
            // now just by adding every key column.
	    allColumns.insert(indexColumn);
	  }

	  keys_desc = keys_desc->header.next;
	} // end while (keys_desc)

      // ---------------------------------------------------------------------
      // Loop over the non key columns of the index/vertical partition.
      // These columns get added to the list of all the columns for the index/
      // VP.  Their length also contributes to the total record length.
      // ---------------------------------------------------------------------
      const desc_struct *non_keys_desc =
                         indexes_desc->body.indexes_desc.non_keys_desc;
      while (non_keys_desc)
	{
	  Int32 tablecolnumber = non_keys_desc->body.keys_desc.tablecolnumber;
          indexColumn = colArray.getColumn(tablecolnumber);

	  if ((table->isHbaseTable()) &&
	      (indexes_desc->body.indexes_desc.keytag != 0))
	    {
	      newIndexColumn = new(heap) NAColumn(*indexColumn);
	      if (non_keys_desc->body.keys_desc.keyname)
		newIndexColumn->setIndexColName(non_keys_desc->body.keys_desc.keyname);
	      newIndexColumn->setHbaseColFam(non_keys_desc->body.keys_desc.hbaseColFam);
	      newIndexColumn->setHbaseColQual(non_keys_desc->body.keys_desc.hbaseColQual);
	      
	      indexColumn = newIndexColumn;
              newColumns.insert(newIndexColumn);
	    }

	  allColumns.insert(indexColumn);
	  non_keys_desc = non_keys_desc->header.next;
	} // end while (non_keys_desc)

      desc_struct *files_desc;
      NABoolean isSystemTable;
      if (isTheClusteringKey)
	{
          // We haven't set up the list of all columns in the clustering
          // index yet, so do that now. Do this by adding all
	  // the base table columns to the columns of the clustering index.
          // Don't add a column, of course, if somehow it has already
          // been added.
	  for (CollIndex bcolNo = 0; bcolNo < colArray.entries(); bcolNo++)
	    {
	      NAColumn *baseCol = colArray[bcolNo];
	      if (NOT allColumns.contains(baseCol))
		{
		  // add this base column
		  allColumns.insert(baseCol);
		}
	    } // end for

	  files_desc = table_desc->body.table_desc.files_desc;
	  isSystemTable = table_desc->body.table_desc.issystemtablecode;

          // Record length of clustering key is the same as that of the base table record
          indexes_desc->body.indexes_desc.record_length = table_desc->body.table_desc.record_length;
	} // endif (isTheClusteringKey)
      else
	{
	  if (indexes_desc->body.indexes_desc.unique)
	    {
	      // As mentioned above, if this is a unique index,
	      // the last numClusteringKeyColumns are actually not
	      // part of the KEY of the index, they are just part of
	      // the index record. Since there are keys_desc entries
	      // for these columns, remove the correspoinding entries
	      // from indexKeyColumns
	      // $$$$ Commenting this out, since Catman and DP2 handle index
	      // keys differently: they always assume that all index columns
	      // are part of the key. Somehow DP2 is told which prefix length
	      // of the key is actually the unique part.
	      // $$$$ This could be enabled when key lengths and record lengths
	      // are different.
	      // for (CollIndex i = 0; i < numClusteringKeyColumns; i++)
	      //   indexKeyColumns.removeAt(indexKeyColumns.entries() - 1);
	    }

	  files_desc = indexes_desc->body.indexes_desc.files_desc;
	  isSystemTable = indexes_desc->body.indexes_desc.issystemtablecode;

	} // endif (NOT isTheClusteringKey)

      // -------------------------------------------------------------------
      // Build the partition attributes for this table.
      //
      // Usually the partitioning key columns are the same as the
      // clustering key columns.  If no partitioning key columns have
      // been specified then the partitioning key columns will be assumed
      // to be the same as the clustering key columns.  Otherwise, they
      // could be the same but may not necessarily be the same.
      //
      // We will ASSUME here that NonStop SQL/MP or the simulator will not
      // put anything into partitioning keys desc and only SQL/MX will.  So
      // we don't have to deal with keytag columns here.
      // -------------------------------------------------------------------
      const desc_struct *partitioning_keys_desc =
                         indexes_desc->body.indexes_desc.partitioning_keys_desc;

      // the key columns that build the salt column for HBase table
      NAColumnArray hbaseSaltOnColumns(CmpCommon::statementHeap());

      if (partitioning_keys_desc)
        {
	  keys_desc = partitioning_keys_desc;
          while (keys_desc)
	    {
              Int32 tablecolnumber = keys_desc
                                   ->body.keys_desc.tablecolnumber;
              indexColumn = colArray.getColumn(tablecolnumber);
              partitioningKeyColumns.insert(indexColumn);
              SortOrdering order = keys_desc
				     ->body.keys_desc.ordering ?
				       DESCENDING : ASCENDING;
	      partitioningKeyColumns.setAscending
                                       (partitioningKeyColumns.entries() - 1,
				        order == ASCENDING);
              keys_desc = keys_desc->header.next;
            } // end while (keys_desc)
        }
      else {
        partitioningKeyColumns = indexKeyColumns;

         // compute the partition key columns for HASH2 partitioning scheme
         // for a salted HBase table. Later on, we will replace 
         // partitioningKeyColumns with the column list computed here if
         // the desired partitioning schema is HASH2.
         for (CollIndex i=0; i<hbaseSaltColumnList.entries(); i++ )
         {
            ColReference* cRef = (ColReference*)hbaseSaltColumnList[i];
            const NAString& colName = (cRef->getColRefNameObj()).getColName();
            NAColumn *col = allColumns.getColumn(colName.data()) ;
            hbaseSaltOnColumns.insert(col);
         }
      }

      // Create DP2 node map for partitioning function.
      NodeMap* nodeMap = NULL; 

      //increment for each table/index to create unique identifier
      cmpCurrentContext->incrementTableIdent();
      

      // NB: Just in case, we made a call to setupClusterInfo at the
      // beginning of this function.
      desc_struct * partns_desc;
      Int32 indexLevels = 1;
      Int32 blockSize = indexes_desc->body.indexes_desc.blocksize;
      if (files_desc)
      {
	if( (table->getSpecialType() != ExtendedQualName::VIRTUAL_TABLE AND
/*
	     table->getSpecialType() != ExtendedQualName::ISP_TABLE AND
*/
	     (NOT table->isHbaseTable()))
	    OR files_desc->body.files_desc.partns_desc )
	  {
            nodeMap = new (heap) NodeMap(heap);
	    createNodeMap(files_desc->body.files_desc.partns_desc,
			  nodeMap,
			  heap,
			  table_desc->body.table_desc.tablename,
			  cmpCurrentContext->getTableIdent());
	    tableIdList.insert(CollIndex(cmpCurrentContext->getTableIdent()));
	  }
	// Check whether the index has any remote partitions.
	if (checkRemote(files_desc->body.files_desc.partns_desc,
			indexes_desc->body.indexes_desc.indexname))
	  hasRemotePartition = TRUE;
	else
	  hasRemotePartition = FALSE;
	
        // Sol: 10-030703-7600. Earlier we assumed that the table is
	// partitioned same as the indexes, hence we used table partitioning
	// to create partitionining function. But this is not true. Hence
	// we now use the indexes partitioning function
	switch (indexes_desc->body.indexes_desc.partitioningScheme)
        {
	case COM_ROUND_ROBIN_PARTITIONING :
	  // Round Robin partitioned table
	  partFunc = createRoundRobinPartitioningFunction(
	       files_desc->body.files_desc.partns_desc,
	       nodeMap,
	       heap);
	  break;

	case COM_HASH_V1_PARTITIONING :
	  // Hash partitioned table
	  partFunc = createHashDistPartitioningFunction(
	       files_desc->body.files_desc.partns_desc,
	       partitioningKeyColumns,
	       nodeMap,
	       heap);
	  break;

	case COM_HASH_V2_PARTITIONING :
	  // Hash partitioned table
	  partFunc = createHash2PartitioningFunction(
	       files_desc->body.files_desc.partns_desc,
	       partitioningKeyColumns,
	       nodeMap,
	       heap);

          partitioningKeyColumns = hbaseSaltOnColumns;

	  break;

	case COM_UNSPECIFIED_PARTITIONING :
	case COM_NO_PARTITIONING :
	case COM_RANGE_PARTITIONING :
	case COM_SYSTEM_PARTITIONING :
	  {
	    // If this is an MP Table, parse the first key
	    // values as MP Stored Text.
	    //

              desc_struct* hbd = 
                   ((table_desc_struct*)table_desc)->hbase_regionkey_desc;

              // splits will be 1 for single partitioned table.
              Int32 splits = findDescEntries(hbd);

              // Do Hash2 only if the table is salted orignally 
              // and the current number of partitions is greater than 1.
              if ( doHash2 )
                 doHash2 = (numOfSaltedPartitions > 0 && splits > 1);

              if ( doHash2 && hbd && hbd->header.nodetype == DESC_HBASE_HASH2_REGION_TYPE ) {
	           partFunc = createHash2PartitioningFunctionForHBase(
	    	      ((table_desc_struct*)table_desc)->hbase_regionkey_desc,
		      heap);

                   partitioningKeyColumns = hbaseSaltOnColumns;

              } else 
                if ((!doHash2 && (hbd && hbd->header.nodetype == DESC_HBASE_HASH2_REGION_TYPE))
                      ||
                (hbd && hbd->header.nodetype == DESC_HBASE_RANGE_REGION_TYPE))
	           partFunc = createRangePartitioningFunctionForHBase(
	    	      ((table_desc_struct*)table_desc)->hbase_regionkey_desc,
	    	      partitioningKeyColumns,
		      heap);
              else {

	        // Range partitioned or single partition table
	        partFunc = createRangePartitioningFunction(
	    	   files_desc->body.files_desc.partns_desc,
	    	   partitioningKeyColumns,
		   nodeMap,
		   heap);
              }


	    break;
	  }
	case COM_UNKNOWN_PARTITIONING:
	  {
            *CmpCommon::diags() << DgSqlCode(-4222)
                                << DgString0("Unsupported partitioning");
	    return TRUE;
	  }
	default:
	  CMPASSERT_STRING(FALSE, "Unhandled switch statement");
        }

        // Check to see if the partitioning function was created
        // successfully.  An error could occur if one of the
        // partitioning keys is an unsupported type or if the table is
        // an MP Table and the first key values contain MP syntax that
        // is not supported by MX.  The unsupported types are the
        // FRACTION only SQL/MP Datetime types.  An example of a
        // syntax error is a Datetime literal which does not have the
        // max number of digits in each field. (e.g. DATETIME
        // '1999-2-4' YEAR TO DAY)
        //
        if (partFunc == NULL) {
          return TRUE;
        }

        // currently we save the indexLevels in the fileset. Since there
        // is a indexLevel for each file that belongs to the fileset,
        // we get the biggest of this indexLevels and save in the fileset.
        partns_desc = files_desc->body.files_desc.partns_desc;
	if(partns_desc)
	  {
	    while (partns_desc)
	      {
		if ( indexLevels < partns_desc->body.partns_desc.indexlevel)
		  indexLevels = partns_desc->body.partns_desc.indexlevel;
		partns_desc = partns_desc->header.next;
	      }

	  }
	else
	  {
	    if ( OSIM_isNSKbehavior() )
	      if(maxIndexLevelsPtr)
		indexLevels= *maxIndexLevelsPtr;
	    
	  }
      }

      // add a new access path
      //
      // $$$ The estimated number of records should be available from
      // $$$ a FILES descriptor. If it is not available, it may have
      // $$$ to be computed by examining the EOFs of each individual
      // $$$ file that belongs to the file set.

      // Create fully qualified ANSI name from indexname, the PHYSICAL name.
      // If this descriptor was created for a sql/mp table, then the
      // indexname is a fully qualified NSK name (\sys.$vol.subvol.name).
      QualifiedName qualIndexName(indexes_desc->body.indexes_desc.indexname,
      				  1, heap, bindWA);

      // This ext_indexname is expected to be set up correctly as an
      // EXTERNAL-format name (i.e., dquoted if any delimited identifiers)
      // by sqlcat/read*.cpp.  The ...AsAnsiString() is just-in-case (MP?).
      NAString extIndexName(
	   indexes_desc->body.indexes_desc.ext_indexname ?
	   (NAString)indexes_desc->body.indexes_desc.ext_indexname :
	   qualIndexName.getQualifiedNameAsAnsiString(),
	   CmpCommon::statementHeap());

      QualifiedName qualExtIndexName;

      //if (indexes_desc->body.indexes_desc.isVolatile)
      if (table->getSpecialType() != ExtendedQualName::VIRTUAL_TABLE)
	qualExtIndexName = QualifiedName(extIndexName, 1, heap, bindWA);
      else
	qualExtIndexName = qualIndexName;

      // for volatile tables, set the object part as the external name.
      // cat/sch parts are internal and should not be shown.
      if (indexes_desc->body.indexes_desc.isVolatile)
	{
	  ComObjectName con(extIndexName);
	  extIndexName = con.getObjectNamePartAsAnsiString();
	}

      if (partFunc)
	numberOfFiles = partFunc->getCountOfPartitions();

      CMPASSERT(indexes_desc->body.indexes_desc.blocksize > 0);

      NAList<HbaseCreateOption*>* hbaseCreateOptions = NULL;
      if ((indexes_desc->body.indexes_desc.hbaseCreateOptions) &&
          (CmpSeabaseDDL::genHbaseCreateOptions
           (indexes_desc->body.indexes_desc.hbaseCreateOptions,
            hbaseCreateOptions,
            heap,
            NULL,
            NULL)))
        return TRUE;

      if (table->isHbaseTable())
      {
        indexLevels = hbtIndexLevels;
        blockSize = hbtBlockSize;
      }
      newIndex = new (heap)
	NAFileSet(
		  qualIndexName, // QN containing "\NSK.$VOL", FUNNYSV, FUNNYNM
		  //(indexes_desc->body.indexes_desc.isVolatile ?
		  qualExtIndexName, // :
		  //qualIndexName),
		  extIndexName,	 // string containing Ansi name CAT.SCH."indx"
		  files_desc ? files_desc->body.files_desc.fileorganization
		             : KEY_SEQUENCED_FILE,
		  isSystemTable,
		  numberOfFiles,
		  MAXOF(table_desc->body.table_desc.rowcount,0),
		  indexes_desc->body.indexes_desc.record_length,
		  files_desc ? files_desc->body.files_desc.lockLength : 0,
                  blockSize,
		  indexLevels,
		  allColumns,
		  indexKeyColumns,
		  partitioningKeyColumns,
		  partFunc,
		  indexes_desc->body.indexes_desc.keytag,
		  uint32ArrayToInt64(
		       indexes_desc->body.indexes_desc.redeftime),
		  files_desc ? files_desc->body.files_desc.audit : 0,
		  files_desc ? files_desc->body.files_desc.auditcompress : 0,
		  files_desc ? files_desc->body.files_desc.compressed : 0,
		  files_desc ? (ComCompressionType)files_desc->body.files_desc.dcompressed : COM_NO_COMPRESSION,
		  files_desc ? files_desc->body.files_desc.icompressed : 0,
		  files_desc ? files_desc->body.files_desc.buffered: 0,
		  files_desc ? files_desc->body.files_desc.clearOnPurge: 0,
		  isPacked,
                  hasRemotePartition,
		  ((indexes_desc->body.indexes_desc.keytag != 0) &&
		   (indexes_desc->body.indexes_desc.unique != 0)),
                  files_desc ? files_desc->body.files_desc.decoupledPartitionKeyList: 0,
                  files_desc ? files_desc->body.files_desc.fileCode : 0,
		  (indexes_desc->body.indexes_desc.isVolatile != 0),
		  (indexes_desc->body.indexes_desc.isInMemoryObjectDefn != 0),
                  indexes_desc->body.indexes_desc.keys_desc,
                  NULL, // no Hive stats
                  indexes_desc->body.indexes_desc.numSaltPartns,
                  hbaseCreateOptions,
                  heap);

      if (isNotAvailable)
	newIndex->setNotAvailable(TRUE);

      // Mark each NAColumn in the list
      indexKeyColumns.setIndexKey();
      if ((table->isHbaseTable()) && (indexes_desc->body.indexes_desc.keytag != 0))
        saveNAColumns.setIndexKey();

      if (indexes_desc->body.indexes_desc.isCreatedExplicitly)
	newIndex->setIsCreatedExplicitly(TRUE);

      //if index is unique and is on one column, then mark column as unique
      if ((indexes_desc->body.indexes_desc.unique) &&
	  (indexKeyColumns.entries() == 1))
        indexKeyColumns[0]->setIsUnique();
      
      partitioningKeyColumns.setPartitioningKey();

      // If it is a VP add it to the list of VPs.
      // Otherwise, add it to the list of indices.
      if (isVerticalPartition)
        vertParts.insert(newIndex); // >>>> RETURN VALUE
      else
      {
        indexes.insert(newIndex);
      }

      //
      // advance to the next index
      //
      if (isTheClusteringKey)
	{
	  clusteringIndex = newIndex; // >>>> RETURN VALUE
	  // switch to the alternate indexes by starting over again
	  isTheClusteringKey = FALSE;
	  indexes_desc = table_desc->body.table_desc.indexes_desc;
	}
      else
	{
	  // simply advance to the next in the list
	  indexes_desc = indexes_desc->header.next;
	}

      // skip the clustering index, if we encounter it again
      if (indexes_desc AND !indexes_desc->body.indexes_desc.keytag)
	indexes_desc = indexes_desc->header.next;
    } // end while (indexes_desc)

    // logic related to indexes hiding
  return FALSE;
} // static createNAFileSets()
#pragma warn(1506)  // warning elimination


// for Hive tables
NABoolean createNAFileSets(hive_tbl_desc* hvt_desc        /*IN*/,
                           const NATable * table          /*IN*/,
                           const NAColumnArray & colArray /*IN*/,
                           NAFileSetList & indexes        /*OUT*/,
                           NAFileSetList & vertParts      /*OUT*/,
                           NAFileSet * & clusteringIndex  /*OUT*/,
			   LIST(CollIndex) & tableIdList  /*OUT*/,
                           NAMemory* heap,
                           BindWA * bindWA,
			   Int32 *maxIndexLevelsPtr = NULL)
{
  NABoolean isTheClusteringKey = TRUE;
  NABoolean isVerticalPartition;
  NABoolean hasRemotePartition = FALSE;
  CollIndex numClusteringKeyColumns = 0;

  // Set up global cluster information.  This global information always
  // gets put on the context heap.
  //
  // Note that this function call will probably not do anything, since
  // this cluster information is set up when arkcmp is created; however,
  // it's certainly better to have this call here, rather than in a
  // doubly-nested loop below where it used to be ...

  // $$$ probably not necessary to call this even once ...
  setUpClusterInfo(CmpCommon::contextHeap());

  // only one set of key columns to handle for hive

      Lng32 numberOfFiles = 1;	  	// always at least 1
      //      NAColumn * indexColumn;		// an index/VP key column
      NAFileSet * newIndex;		// a new file set

      // all columns that belong to an index
      NAColumnArray allColumns(CmpCommon::statementHeap());

      // the index key columns - the SORT columns
      NAColumnArray indexKeyColumns(CmpCommon::statementHeap());

      // the partitioning key columns - the BUCKETING columns
      NAColumnArray partitioningKeyColumns(CmpCommon::statementHeap());

      PartitioningFunction * partFunc = NULL;
      // is this an index or is it really a VP?

      isVerticalPartition = FALSE;
      NABoolean isPacked = FALSE;

      NABoolean isNotAvailable = FALSE;

      // ---------------------------------------------------------------------
      // loop over the clustering key columns of the index
      // ---------------------------------------------------------------------
      const hive_bkey_desc *hbk_desc = hvt_desc->getBucketingKeys();

      Int32 numBucketingColumns = 0;

      while (hbk_desc)
	{
          NAString colName(hbk_desc->name_);
          colName.toUpper();

          NAColumn* bucketingColumn = colArray.getColumn(colName);

          if ( bucketingColumn ) {
	     partitioningKeyColumns.insert(bucketingColumn);
             numBucketingColumns++;
          }

	  hbk_desc = hbk_desc->next_;
	} // end while (hvk_desc)

      const hive_skey_desc *hsk_desc = hvt_desc->getSortKeys();
      if ( hsk_desc == NULL ) {
         // assume all columns are index key columns
          for (CollIndex i=0; i<colArray.entries(); i++ )
	     indexKeyColumns.insert(colArray[i]);
      } else {
        while (hsk_desc)
        {
          NAString colName(hsk_desc->name_);
          colName.toUpper();

          NAColumn* sortKeyColumn = colArray.getColumn(colName);

          if ( sortKeyColumn ) {

               indexKeyColumns.insert(sortKeyColumn);
               indexKeyColumns.setAscending(indexKeyColumns.entries() - 1,
                                            hsk_desc->orderInt_);
          }

          hsk_desc = hsk_desc->next_;
        } // end while (hsk_desc)
      }


      // ---------------------------------------------------------------------
      // Loop over the non key columns. 
      // ---------------------------------------------------------------------
      for (CollIndex i=0; i<colArray.entries(); i++)
	{
	  allColumns.insert(colArray[i]);
	}

      //increment for each table/index to create unique identifier
      cmpCurrentContext->incrementTableIdent();

      // collect file stats from HDFS for the table
      const hive_sd_desc *sd_desc = hvt_desc->getSDs();
      HHDFSTableStats * hiveHDFSTableStats = new(heap) HHDFSTableStats(heap);
      hiveHDFSTableStats->
        setPortOverride(CmpCommon::getDefaultLong(HIVE_LIB_HDFS_PORT_OVERRIDE));

      // create file-level statistics and estimate total row count and record length
      hiveHDFSTableStats->populate(hvt_desc);
      if (hiveHDFSTableStats->hasError())
        {
          *CmpCommon::diags() << DgSqlCode(-1200)
                              << DgString0(hiveHDFSTableStats->getDiags().getErrMsg())
                              << DgTableName(table->getTableName().getQualifiedNameAsAnsiString());
          return TRUE;
        }

      if ((hiveHDFSTableStats->isOrcFile()) &&
          (CmpCommon::getDefault(TRAF_ENABLE_ORC_FORMAT) == DF_OFF))
        {
          *CmpCommon::diags() << DgSqlCode(-3069)
                              << DgTableName(table->getTableName().getQualifiedNameAsAnsiString());
          return TRUE;
        }

#ifndef NDEBUG
      NAString logFile = 
        ActiveSchemaDB()->getDefaults().getValue(HIVE_HDFS_STATS_LOG_FILE);
      if (logFile.length())
        {
          FILE *ofd = fopen(logFile, "a");
          if (ofd)
            {
              hiveHDFSTableStats->print(ofd);
              fclose(ofd);
            }
        }
      // for release code, would need to sandbox the ability to write
      // files, e.g. to a fixed log directory
#endif


      // Create a node map for partitioning function.
      NodeMap* nodeMap = new (heap) NodeMap(heap, NodeMap::HIVE);

      createNodeMap(hvt_desc,
                  nodeMap,
                  heap,
                  (char*)(table->getTableName().getObjectName().data()),
                  cmpCurrentContext->getTableIdent());
      tableIdList.insert(CollIndex(cmpCurrentContext->getTableIdent()));

      // For the time being, set it up as Hash2 partitioned table
               
      Int32 numBuckets = hvt_desc->getSDs()->buckets_;

      if (numBuckets>1 && partitioningKeyColumns.entries()>0) {
         if ( CmpCommon::getDefault(HIVE_USE_HASH2_AS_PARTFUNCION) == DF_ON )
            partFunc = createHash2PartitioningFunction
                          (numBuckets, partitioningKeyColumns, nodeMap, heap);
         else
            partFunc = createHivePartitioningFunction
                          (numBuckets, partitioningKeyColumns, nodeMap, heap);
      } else
         partFunc = new (heap)
                       SinglePartitionPartitioningFunction(nodeMap, heap);

      // NB: Just in case, we made a call to setupClusterInfo at the
      // beginning of this function.
      //      desc_struct * partns_desc;
      Int32 indexLevels = 1;
      

      // add a new access path
      //
      // $$$ The estimated number of records should be available from
      // $$$ a FILES descriptor. If it is not available, it may have
      // $$$ to be computed by examining the EOFs of each individual
      // $$$ file that belongs to the file set.

      // Create fully qualified ANSI name from indexname, the PHYSICAL name.
      // If this descriptor was created for a sql/mp table, then the
      // indexname is a fully qualified NSK name (\sys.$vol.subvol.name).
      QualifiedName qualIndexName(
                  (char*)(table->getTableName().getObjectName().data()),
                  "HIVE", "", heap);

      // This ext_indexname is expected to be set up correctly as an
      // EXTERNAL-format name (i.e., dquoted if any delimited identifiers)
      // by sqlcat/read*.cpp.  The ...AsAnsiString() is just-in-case (MP?).
      NAString extIndexName(
	   qualIndexName.getQualifiedNameAsAnsiString(),
	   CmpCommon::statementHeap());

      QualifiedName qualExtIndexName = QualifiedName(extIndexName, 1, heap, bindWA);


      if (partFunc)
	numberOfFiles = partFunc->getCountOfPartitions();

      Int64 estimatedRC = 0;
      Int64 estimatedRecordLength = 0;

      if ( !sd_desc->isTrulyText() ) {
         //
         // Poor man's estimation by assuming the record length in hive is the 
         // same as SQ's. We can do better once we know how the binary data is
         // stored in hdfs.
         //
         estimatedRecordLength = colArray.getTotalStorageSize();
         estimatedRC = hiveHDFSTableStats->getTotalSize() / estimatedRecordLength;
      } else {
         // use the information estimated during populate() call
         estimatedRC = hiveHDFSTableStats->getEstimatedRowCount();
         estimatedRecordLength = 
           Lng32(MINOF(hiveHDFSTableStats->getEstimatedRecordLength(),
                       hiveHDFSTableStats->getEstimatedBlockSize()-100));
      }

      ((NATable*)table)-> setOriginalRowCount((double)estimatedRC);

      newIndex = new (heap)
	NAFileSet(
		  qualIndexName, // QN containing "\NSK.$VOL", FUNNYSV, FUNNYNM
		  //(indexes_desc->body.indexes_desc.isVolatile ?
		  qualExtIndexName, // :
		  //qualIndexName),
		  extIndexName,	 // string containing Ansi name CAT.SCH."indx"

                  // The real orginization is a hybrid of KEY_SEQ and HASH.
                  // Well, we just take the KEY_SEQ for now.
                  KEY_SEQUENCED_FILE,

		  FALSE, // isSystemTable
		  numberOfFiles,

                  // HIVE-TBD
		  Cardinality(estimatedRC),
		  Lng32(estimatedRecordLength), 

		  0, // lock length

		  //hvt_desc->getBlockSize(), 
		  (Lng32)hiveHDFSTableStats->getEstimatedBlockSize(), 

		  indexLevels, // HIVE-TBD
		  allColumns,
		  indexKeyColumns,
		  partitioningKeyColumns,
		  partFunc,
		  0, // indexes_desc->body.indexes_desc.keytag,

		  hvt_desc->redeftime(), 
                 
		  1, // files_desc->body.files_desc.audit 
		  0, // files_desc->body.files_desc.auditcompress 
		  0, // files_desc->body.files_desc.compressed 
		  COM_NO_COMPRESSION,
		  0, // files_desc->body.files_desc.icompressed 
		  0, // files_desc->body.files_desc.buffered:
		  0, // files_desc->body.files_desc.clearOnPurge: 0,
		  isPacked,
                  hasRemotePartition,
		  0, // not a unique secondary index
                  0, // isDecoupledRangePartitioned
                  0, // file code
		  0, // not a volatile
		  0, // inMemObjectDefn
                  NULL, // indexes_desc->body.indexes_desc.keys_desc,
                  hiveHDFSTableStats,
                  0, // saltPartns
                  NULL, //hbaseCreateOptions
                  heap);

      if (isNotAvailable)
	newIndex->setNotAvailable(TRUE);

      // Mark each NAColumn in the list
      indexKeyColumns.setIndexKey();

      partitioningKeyColumns.setPartitioningKey();

      // If it is a VP add it to the list of VPs.
      // Otherwise, add it to the list of indices.
      indexes.insert(newIndex);

       clusteringIndex = newIndex;


  return FALSE;
} // static createNAFileSets()

#pragma warn(1506)  // warning elimination

// -----------------------------------------------------------------------
// Mark columns named in PRIMARY KEY constraint (these will be different
// from clustering key columns when the PK is droppable), for Binder error 4033.
// -----------------------------------------------------------------------
static void markPKCols(const constrnts_desc_struct * constrnt /*IN*/,
		       const NAColumnArray& columnArray       /*IN*/)
{
  desc_struct *keycols_desc = constrnt->constr_key_cols_desc;
  while (keycols_desc)
    {
      constrnt_key_cols_desc_struct *key =
      				    &keycols_desc->body.constrnt_key_cols_desc;
      // Lookup by name (not position: key->position is pos *within the PK*)
      NAColumn *nacol = columnArray.getColumn(key->colname);
      if(nacol != NULL)
	nacol->setPrimaryKey();
      keycols_desc = keycols_desc->header.next;
    }
} // static markPKCols

// -----------------------------------------------------------------------
// Insert MP CHECK CONSTRAINT text into NATable::checkConstraints_.
// -----------------------------------------------------------------------
static NABoolean
createConstraintInfo(const desc_struct * table_desc        /*IN*/,
                     const QualifiedName& tableQualName    /*IN*/,
                     const NAColumnArray& columnArray      /*IN*/,
                     CheckConstraintList& checkConstraints /*OUT*/,
                     AbstractRIConstraintList& uniqueConstraints,
                     AbstractRIConstraintList& refConstraints,
                     NAMemory* heap,
                     BindWA *bindWA)
{
  desc_struct *constrnts_desc = table_desc->body.table_desc.constrnts_desc;

  while (constrnts_desc)
    {
      constrnts_desc_struct *constrntHdr = &constrnts_desc->body.constrnts_desc;

      Int32 minNameParts=3;


      QualifiedName constrntName(constrntHdr->constrntname, minNameParts, (NAMemory*)0, bindWA);

      if (constrntName.numberExpanded() == 0) {
        // There was an error parsing the name of the constraint (see
        // QualifiedName ctor).  Return TRUE indicating an error.
        //
        return TRUE;
      }

      switch (constrntHdr->type)
	{

	case PRIMARY_KEY_CONSTRAINT:
	  markPKCols(constrntHdr, columnArray);

	case UNIQUE_CONSTRAINT:	  {

	    UniqueConstraint *uniqueConstraint = new (heap)
	      UniqueConstraint(constrntName, tableQualName, heap,
                               (constrntHdr->type == PRIMARY_KEY_CONSTRAINT));
            uniqueConstraint->setKeyColumns(constrntHdr, heap);

	    uniqueConstraint->setRefConstraintsReferencingMe(constrntHdr, heap, bindWA);
	    uniqueConstraints.insert(uniqueConstraint);
	}
	break;
	case REF_CONSTRAINT:
	  {
	    char *refConstrntName = constrntHdr->referenced_constrnts_desc->
	      body.ref_constrnts_desc.constrntname;
	    char *refTableName = constrntHdr->referenced_constrnts_desc->
	      body.ref_constrnts_desc.tablename;

	    QualifiedName refConstrnt(refConstrntName, 3, (NAMemory*)0, bindWA);
	    QualifiedName refTable(refTableName, 3, (NAMemory*)0, bindWA);

	    RefConstraint *refConstraint = new (heap)
	      RefConstraint(constrntName, tableQualName,
			    refConstrnt, refTable, heap);

	    refConstraint->setKeyColumns(constrntHdr, heap);
	    refConstraint->setIsEnforced((constrntHdr->isEnforced == 1));

	    refConstraints.insert(refConstraint);
	  }
	break;
	case CHECK_CONSTRAINT:
	case MP_CHECK_CONSTRAINT:
	  {
	    char *constrntText = constrntHdr->check_constrnts_desc->
		 	         body.check_constrnts_desc.constrnt_text;
	    checkConstraints.insert(new (heap)
	      CheckConstraint(constrntName, constrntText, heap));
	  }
          break;
        default:
          CMPASSERT(FALSE);
	}

      constrnts_desc = constrnts_desc->header.next;
    }

  // return FALSE, indicating no error.
  //
  return FALSE;

} // static createConstraintInfo()

ULng32 hashColPosList(const CollIndexSet &colSet)
{
  return colSet.hash();
}


// -----------------------------------------------------------------------
// NATable::NATable() constructor
// -----------------------------------------------------------------------

const Lng32 initHeapSize = 32 * 1024;		// ## 32K: tune this someday!

#pragma nowarn(770)  // warning elimination
NATable::NATable(BindWA *bindWA,
                 const CorrName& corrName,
		 NAMemory *heap,
		 desc_struct* inTableDesc)
  //
  // The NATable heap ( i.e. heap_ ) used to come from ContextHeap
  // (i.e. heap) but it creates high memory usage/leakage in Context
  // Heap. Although the NATables are deleted at the end of each statement,
  // the heap_ is returned to heap (i.e. context heap) which caused
  // context heap containing a lot of not used chunk of memory. So it is
  // changed to be from whatever heap is passed in at the call in
  // NATableDB.getNATable.
  //
  // Now NATable objects can be cached.If an object is to be cached (persisted
  // across statements) a NATable heap is allocated for the object
  // and is passed in (this is done in NATableDB::get(CorrName& corrName...).
  // Otherwise a reference to the Statement heap is passed in. When a cached
  // object is to be deleted the object's heap is deleted which wipes out the
  // NATable object all its related stuff. NATable objects that are not cached
  // are wiped out at the end of the statement when the statement heap is deleted.
  //
  : heap_(heap),
    referenceCount_(0),
    refsIncompatibleDP2Halloween_(FALSE),
    isHalloweenTable_(FALSE),
    qualifiedName_(corrName.getExtendedQualNameObj(),heap),
    synonymReferenceName_(heap),
    fileSetName_(corrName.getQualifiedNameObj(),heap),   // for now, set equal
    clusteringIndex_(NULL),
    colcount_(0),
    colArray_(heap),
    recordLength_(0),
    indexes_(heap),
    vertParts_(heap),
    colStats_(NULL),
    statsFetched_(FALSE),
    viewFileName_(NULL),
    viewText_(NULL),
    viewTextInNAWchars_(heap),
    viewTextCharSet_(CharInfo::UnknownCharSet),
    viewCheck_(NULL),
    flags_(IS_INSERTABLE | IS_UPDATABLE),
    insertMode_(COM_REGULAR_TABLE_INSERT_MODE),
    isSynonymTranslationDone_(FALSE),
    checkConstraints_(heap),
    createTime_(0),
    redefTime_(0),
    cacheTime_(0),
    statsTime_(0),
    catalogUID_(0),
    schemaUID_(0),
    objectUID_(0),
    objectType_(COM_UNKNOWN_OBJECT),
    partitioningScheme_(COM_UNKNOWN_PARTITIONING),
    uniqueConstraints_(heap),
    refConstraints_(heap),
    isAnMV_(FALSE),
    isAnMVMetaData_(FALSE),
    mvsUsingMe_(heap),
    mvInfo_(NULL),
    accessedInCurrentStatement_(TRUE),
    setupForStatement_(FALSE),
    resetAfterStatement_(FALSE),
    hitCount_(0),
    replacementCounter_(2),
    tableConstructionHadWarnings_(FALSE),
    isAnMPTableWithAnsiName_(FALSE),
    isUMDTable_(FALSE),
    isSMDTable_(FALSE),
    isMVUMDTable_(FALSE),

    // For virtual tables, we set the object schema version
    // to be the current schema version
    osv_(COM_VERS_CURR_SCHEMA),
    ofv_(COM_VERS_CURR_SCHEMA),
    partnsDesc_(NULL),
    colsWithMissingStats_(NULL),
    originalCardinality_(-1.0),
    tableIdList_(heap),
    rcb_(NULL),
    rcbLen_(0),
    keyLength_(0),
    parentTableName_(NULL),
    sgAttributes_(NULL),
    isHive_(FALSE),
    isHbase_(FALSE),
    isHbaseCell_(FALSE),
    isHbaseRow_(FALSE),
    isSeabase_(FALSE),
    isSeabaseMD_(FALSE),
    isSeabasePrivSchemaTable_(FALSE),
    isUserUpdatableSeabaseMD_(FALSE),
    resetHDFSStatsAfterStmt_(FALSE),
    hiveDefaultStringLen_(0),
    hiveTableId_(-1),
    tableDesc_(inTableDesc),
    privInfo_(NULL),
    secKeySet_(heap),
    newColumns_(heap),
    snapshotName_(NULL)
{
  NAString tblName = qualifiedName_.getQualifiedNameObj().getQualifiedNameAsString();
  NAString mmPhase;

  Lng32 preCreateNATableWarnings = CmpCommon::diags()->getNumber(DgSqlCode::WARNING_);

  //set heap type
  if(heap_ == CmpCommon::statementHeap()){
    heapType_ = STATEMENT;
    mmPhase = "NATable Init (Stmt) - " + tblName;
  }else if (heap_ == CmpCommon::contextHeap()){
    heapType_ = CONTEXT;
    mmPhase = "NATable Init (Cnxt) - " + tblName;
  }else {
    heapType_ = OTHER;
    mmPhase = "NATable Init (Other) - " + tblName;
  }
  

  MonitorMemoryUsage_Enter((char*)mmPhase.data(), heap_, TRUE);

  // Do a readTableDef, if table descriptor has not been passed in
  //
  desc_struct * table_desc;
  Int32 *maxIndexLevelsPtr = new (STMTHEAP) Int32;
  if (!inTableDesc)
    {
      // lookup from metadata other than HBase is not currently supported
      CMPASSERT(inTableDesc);
    }
  else
    {
      // use the input descriptor to create NATable.
      // Used if 'virtual' tables, like EXPLAIN,
      // DESCRIBE, RESOURCE_FORK, etc are to be created.
      table_desc = inTableDesc;

      // Need to initialize the maxIndexLevelsPtr field
      *maxIndexLevelsPtr = 1;
    }

  if ((corrName.isHbase()) || (corrName.isSeabase()))
    {
      setIsHbaseTable(TRUE); 
      setIsSeabaseTable(corrName.isSeabase());
      setIsHbaseCellTable(corrName.isHbaseCell());
      setIsHbaseRowTable(corrName.isHbaseRow());
      setIsSeabaseMDTable(corrName.isSeabaseMD());
    }

  // Check if the synonym name translation to reference object has been done.
  if (table_desc->body.table_desc.isSynonymNameTranslationDone)
  {
    isSynonymTranslationDone_ = TRUE;
    NAString synonymReferenceName(table_desc->body.table_desc.tablename);
    synonymReferenceName_ = synonymReferenceName;
    ComUID uid(table_desc->body.table_desc.objectUID[0]*0x100000000LL +
               table_desc->body.table_desc.objectUID[1]);
    synonymReferenceObjectUid_ = uid;
  }
  // Check if it is a UMD table, or SMD table or MV related UMD object
  // and set cll correcsponding flags to indicate this.
  if (table_desc->body.table_desc.isUMDTable)
  {
     isUMDTable_ = TRUE;
  }

  if (table_desc->body.table_desc.issystemtablecode)
  {
     isSMDTable_ = TRUE;
  }

  if (table_desc->body.table_desc.isMVMetaDataObject)
  {
     isMVUMDTable_ = TRUE;
  }

  isTrigTempTable_ = (qualifiedName_.getSpecialType() == ExtendedQualName::TRIGTEMP_TABLE);

  switch(table_desc->body.table_desc.rowFormat)
    {
    case COM_PACKED_FORMAT_TYPE:
      setSQLMXTable(TRUE);
      break;
    case COM_ALIGNED_FORMAT_TYPE:
      setSQLMXAlignedTable(TRUE);
      break;
    }

  if (table_desc->body.table_desc.isVolatile)
  {
    setVolatileTable( TRUE );
  }

  if (table_desc->body.table_desc.isInMemoryObjectDefn)
  {
    setInMemoryObjectDefn( TRUE );
  }

  if (table_desc->body.table_desc.isDroppable)
  {
    setDroppableTable( TRUE );
  }

  insertMode_ = table_desc->body.table_desc.insertMode;

  setRecordLength(table_desc->body.table_desc.record_length);
  //
  // Add timestamp information.
  //
  createTime_ = uint32ArrayToInt64(table_desc->body.table_desc.createtime);
  redefTime_  = uint32ArrayToInt64(table_desc->body.table_desc.redeftime);
  cacheTime_  = uint32ArrayToInt64(table_desc->body.table_desc.cachetime);

  catalogUID_ = uint32ArrayToInt64(table_desc->body.table_desc.catUID);
  schemaUID_ = uint32ArrayToInt64(table_desc->body.table_desc.schemaUID);
  objectUID_ = uint32ArrayToInt64(table_desc->body.table_desc.objectUID);

  if (table_desc->body.table_desc.owner)
    {
      Int32 userInfo (table_desc->body.table_desc.owner);
      owner_ = userInfo;
    }
  if (table_desc->body.table_desc.schemaOwner)
    {
      Int32 schemaUser(table_desc->body.table_desc.schemaOwner);
      schemaOwner_ = schemaUser;
    }

  objectType_ = table_desc->body.table_desc.objectType;
  partitioningScheme_ = table_desc->body.table_desc.partitioningScheme;

  if (!(corrName.isSeabaseMD() || corrName.isSpecialTable()))
    setupPrivInfo();

  rcb_ = table_desc->body.table_desc.rcb;
  rcbLen_ = table_desc->body.table_desc.rcbLen;
  keyLength_ = table_desc->body.table_desc.keyLen;

  if (table_desc->body.table_desc.parentTableName)
    {
      parentTableName_ =
	new(heap_) char[strlen(table_desc->body.table_desc.parentTableName) + 1];
      strcpy(parentTableName_, table_desc->body.table_desc.parentTableName);
    }

  if (table_desc->body.table_desc.snapshotName)
    {
    snapshotName_ =
        new(heap_) char[strlen(table_desc->body.table_desc.snapshotName) + 1];
      strcpy(snapshotName_, table_desc->body.table_desc.snapshotName);
    }

  desc_struct * files_desc = table_desc->body.table_desc.files_desc;

  // Some objects don't have a file_desc set up (e.g. views)
  // Therefore, only setup the partnsDesc_ if this is a partitionable object
  if (files_desc)
  {
     if (files_desc->body.files_desc.partns_desc)
       partnsDesc_ = files_desc->body.files_desc.partns_desc;
  }
  else
     partnsDesc_ = NULL;

  //
  // Insert a NAColumn in the colArray_ for this NATable for each
  // columns_desc from the ARK SMD. Returns TRUE if error creating NAColumns.
  //
  if (createNAColumns(table_desc->body.table_desc.columns_desc,
		      this,
		      colArray_ /*OUT*/,
		      heap_))
    //coverity[leaked_storage]
    return; // colcount_ == 0 indicates an error

  //
  // Add view information, if this is a view
  //
  desc_struct *view_desc = table_desc->body.table_desc.views_desc;
  if (view_desc)
    {
      viewText_ = new (heap_) char[strlen(view_desc->body.view_desc.viewtext) + 2];
      strcpy(viewText_, view_desc->body.view_desc.viewtext);
      strcat(viewText_, ";");

      viewTextCharSet_ = (CharInfo::CharSet)view_desc->body.view_desc.viewtextcharset;

      viewCheck_    = NULL; //initialize
      if(view_desc->body.view_desc.viewchecktext){
        UInt32 viewCheckLength = str_len(view_desc->body.view_desc.viewchecktext)+1;
        viewCheck_ = new (heap_) char[ viewCheckLength];
        memcpy(viewCheck_, view_desc->body.view_desc.viewchecktext,
               viewCheckLength);
      }
      setUpdatable(view_desc->body.view_desc.updatable);
      setInsertable(view_desc->body.view_desc.insertable);

      //
      // The updatable flag is false for an MP view only if it is NOT a
      // protection view. Therefore updatable == FALSE iff it is a
      // shorthand view. See ReadTableDef.cpp, l. 3379.
      //

      viewFileName_ = NULL;
      CMPASSERT(view_desc->body.view_desc.viewfilename);
      UInt32 viewFileNameLength = str_len(view_desc->body.view_desc.viewfilename) + 1;
      viewFileName_ = new (heap_) char[viewFileNameLength];
      memcpy(viewFileName_, view_desc->body.view_desc.viewfilename,
	     viewFileNameLength);
    }
  else
    {
      //keep track of memory used by NAFileSets
      Lng32 preCreateNAFileSetsMemSize = heap_->getAllocSize();

      //
      // Process indexes and vertical partitions for this table.
      //
      if (createNAFileSets(table_desc       /*IN*/,
                           this             /*IN*/,
                           colArray_        /*IN*/,
                           indexes_         /*OUT*/,
                           vertParts_       /*OUT*/,
                           clusteringIndex_ /*OUT*/,
			   tableIdList_     /*OUT*/,
                           heap_,
                           bindWA,
                           newColumns_,     /*OUT*/
			   maxIndexLevelsPtr)) {
        return; // colcount_ == 0 indicates an error
      }

      // Add constraint info.
      //
      // This call to createConstraintInfo, calls the parser on
      // the constraint name
      //
      
      NABoolean  errorOccurred =
        createConstraintInfo(table_desc        /*IN*/,
                             getTableName()    /*IN*/,
                             getNAColumnArray()/*IN (some columns updated)*/,
                             checkConstraints_ /*OUT*/,
                             uniqueConstraints_/*OUT*/,
                             refConstraints_   /*OUT*/,
                             heap_,
                             bindWA);
      
      if (errorOccurred) {
        // return before setting colcount_, indicating that there
        // was an error in constructing this NATable.
        //
        return;
      }
      
      //
      // FetchHistograms call used to be here -- moved to getStatistics().
      //
    }
  
    // change partFunc for base table if PARTITION clause has been used
    // to limit the number of partitions that will be accessed.
    if ((qualifiedName_.isPartitionNameSpecified()) ||
	(qualifiedName_.isPartitionRangeSpecified())) {
      if (filterUnusedPartitions(corrName.getPartnClause())) {
	return ;
      }
    }

  //
  // Set colcount_ after all possible errors (Binder uses nonzero colcount
  // as an indicator of valid table definition).
  //
  CMPASSERT(table_desc->body.table_desc.colcount >= 0);   // CollIndex cast ok?
  colcount_ = (CollIndex)table_desc->body.table_desc.colcount;

  // If there is a host variable associated with this table, store it
  // for use by the generator to generate late-name resolution information.
  //
  HostVar *hv = corrName.getPrototype();
  prototype_ = hv ? new (heap_) HostVar(*hv) : NULL;

  // MV
  // Initialize the MV support data members
  isAnMV_           = table_desc->body.table_desc.isMVtable;
  isAnMVMetaData_   = table_desc->body.table_desc.isMVMetaDataObject;
  mvAttributeBitmap_.initBitmap(table_desc->body.table_desc.mvAttributesBitmap);

  desc_struct *mvs_desc = table_desc->body.table_desc.using_mvs_desc;
// Memory Leak
  while (mvs_desc)
  {
    using_mv_desc_struct *mv = &mvs_desc->body.using_mv_desc;

    UsingMvInfo *usingMv = new(heap_)
        UsingMvInfo(mv->mvName, mv->refreshType, mv->rewriteEnabled,
                    mv->isInitialized, heap_);
    mvsUsingMe_.insert(usingMv);

    mvs_desc = mvs_desc->header.next;
  }

  // ++MV

  // fix the special-type for MV objects. There are case where the type is
  // set to NORMAL_TABLE although this is an MV.
  //
  // Example:
  // --------
  // in the statement "select * from MV1" mv1 will have a NORMAL_TABLE
  // special-type, while in "select * from table(mv_table MV1)" it will
  // have the MV_TABLE special-type.

  if (isAnMV_)
  {
    switch(qualifiedName_.getSpecialType())
    {
    case ExtendedQualName::GHOST_TABLE:
      qualifiedName_.setSpecialType(ExtendedQualName::GHOST_MV_TABLE);
      break;
    case ExtendedQualName::GHOST_MV_TABLE:
      // Do not change it
      break;
    default:
      qualifiedName_.setSpecialType(ExtendedQualName::MV_TABLE);
      break;
    }
  }

  // --MV

  // Initialize the sequence generator fields
  desc_struct *sequence_desc = table_desc->body.table_desc.sequence_generator_desc;
  if (sequence_desc != NULL) {
    sequence_generator_desc_struct *sg_desc = &sequence_desc->body.sequence_generator_desc;
    
    if (sg_desc != NULL)
      {
	sgAttributes_ = 
	  new(heap_) SequenceGeneratorAttributes(
						 sg_desc->startValue,
						 sg_desc->increment,
						 sg_desc->maxValue,
						 sg_desc->minValue,
						 sg_desc->sgType,
						 sg_desc->sqlDataType,
						 sg_desc->fsDataType,
						 sg_desc->cycleOption,
                                                 FALSE,
						 sg_desc->objectUID,
						 sg_desc->cache,
						 sg_desc->nextValue,
                                                 0,
                                                 sg_desc->redefTime);
      }
  }
#ifndef NDEBUG
  if (getenv("NATABLE_DEBUG"))
    {
      cout << "NATable " << (void*)this << " "
	   << qualifiedName_.getQualifiedNameObj().getQualifiedNameAsAnsiString() << " "
	   << (Int32)qualifiedName_.getSpecialType() << endl;
      colArray_.print();
      }
  #endif
      //this guy is cacheable
  if((qualifiedName_.isCacheable())&&
     (NOT (isHbaseTable())) && 
      //this object is not on the statement heap (i.e. it is being cached)
     ((heap_ != CmpCommon::statementHeap())||
      (OSIM_runningInCaptureMode())))
  {
    char * nodeName = NULL;
    char * catStr = NULL;
    char * schemaStr = NULL;
    char * fileStr = NULL;
    short nodeNameLen = 0;
    Int32 catStrLen = 0;
    Int32 schemaStrLen = 0;
    Int32 fileStrLen = 0;
#ifdef NA_64BIT
    // dg64 - match signature
    int_32 primaryNodeNum=0;
#else
    Int32 primaryNodeNum=0;
#endif
    short error = 0;

    //clusteringIndex has physical filename that can be used to check
    //if a catalog operation has been performed on a table.
    //Views don't have clusteringIndex, so we get physical filename
    //from the viewFileName_ datamember.
    if(viewText_)
    {
      //view filename starts with node name
      //filename is in format \<node_name>.$<volume>.<subvolume>.<file>
      //catStr => <volume>
      //schemaStr => <subvolume>
      //fileStr => <file>
      nodeName = viewFileName_;
      catStr = nodeName;

      //skip over node name
      //measure node name length
      //get to begining of volume name
      //Measure length of node name
      //skip over node name i.e. \MAYA, \AZTEC, etc
      //and get to volume name
      while((nodeName[nodeNameLen]!='.')&&
            (nodeNameLen < 8)){
	catStr++;
        nodeNameLen++;
      };

      //skip over '.' and the '$' in volume name
      catStr=&nodeName[nodeNameLen+2];
      schemaStr=catStr;

      //skip over the volume/catalog name
      //while measuring catalog name length
      while((catStr[catStrLen]!='.')&&
            (catStrLen < 8))
      {
	schemaStr++;
	catStrLen++;
      }

      //skip over the '.'
      schemaStr++;
      fileStr=schemaStr;

      //skip over the subvolume/schema name
      //while measuring schema name length
      while((schemaStr[schemaStrLen]!='.')&&
            (schemaStrLen < 8))
      {
	fileStr++;
        schemaStrLen++;
      }

      //skip over the '.'
      fileStr++;
      fileStrLen = str_len(fileStr);

      //figure out the node number for the node
      //which has the primary partition.
      primaryNodeNum=0;

      if(!OSIM_runningSimulation())
        error = OSIM_NODENAME_TO_NODENUMBER (nodeName, nodeNameLen, &primaryNodeNum);
    }
    else{
      //get qualified name of the clustering index which should
      //be the actual physical file name of the table
      const QualifiedName fileNameObj = getClusteringIndex()->
        getRandomPartition();
      const NAString fileName = fileNameObj.getObjectName();

      //get schemaName object
      const SchemaName schemaNameObj = fileNameObj.getSchemaName();
      const NAString schemaName = schemaNameObj.getSchemaName();

      //get catalogName object
      //this contains a string in the form \<node_name>.$volume
      const CatalogName catalogNameObj = fileNameObj.getCatalogName();
      const NAString catalogName = catalogNameObj.getCatalogName();
      nodeName = (char*) catalogName.data();
      catStr = nodeName;

      //Measure length of node name
      //skip over node name i.e. \MAYA, \AZTEC, etc
      //and get to volume name
      while((nodeName[nodeNameLen]!='.')&&
            (nodeNameLen < 8)){
        catStr++;
        nodeNameLen++;
      };

      //get volume/catalog name
      //skip ".$"
      catStr=&nodeName[nodeNameLen+2];
#pragma nowarn(1506)   // warning elimination
      catStrLen = catalogName.length() - (nodeNameLen+2);
#pragma warn(1506)  // warning elimination

      //get subvolume/schema name
      schemaStr = (char *) schemaName.data();
#pragma nowarn(1506)   // warning elimination
      schemaStrLen = schemaName.length();
#pragma warn(1506)  // warning elimination

      //get file name
      fileStr = (char *) fileName.data();
#pragma nowarn(1506)   // warning elimination
      fileStrLen = fileName.length();
#pragma warn(1506)  // warning elimination

      //figure out the node number for the node
      //which has the primary partition.
      primaryNodeNum=0;

      error = OSIM_NODENAME_TO_NODENUMBER (nodeName, nodeNameLen, &primaryNodeNum);
    }
  }

  Lng32 postCreateNATableWarnings = CmpCommon::diags()->getNumber(DgSqlCode::WARNING_);

  if(postCreateNATableWarnings != preCreateNATableWarnings)
    tableConstructionHadWarnings_=TRUE;

  if (hasLobColumn())
    {
//    Memory Leak
      // read lob related information from lob metadata
      short *lobNumList = new (heap_) short[getColumnCount()];
      short *lobTypList = new (heap_) short[getColumnCount()];
      char  **lobLocList = new (heap_) char*[getColumnCount()];
      
      const NAColumnArray &colArray = getNAColumnArray();
      NAColumn *nac = NULL;
      
      Lng32 j = 0;
      for (CollIndex i = 0; i < getColumnCount(); i++)
	{
	  nac = colArray.getColumn(i);
	  
	  if (nac->getType()->getTypeQualifier() == NA_LOB_TYPE)
	    {
	      lobLocList[j] = new (heap_) char[1024];
	      j++;
	    }
	}      
      
      NAString schNam;
      schNam = "\"";
      schNam += getTableName().getCatalogName();
      schNam += "\".\"";
      schNam += getTableName().getSchemaName();
      schNam += "\"";
      
      Lng32 numLobs = 0;
      Lng32 cliRC = SQL_EXEC_LOBddlInterface
	(
	 (char*)schNam.data(),
	 schNam.length(),
	 objectUid().castToInt64(),
	 numLobs,
	 LOB_CLI_SELECT_CURSOR,
	 lobNumList,
	 lobTypList,
	 lobLocList);
      
      if (cliRC == 0)
	{
	  for (Lng32 i = 0; i < numLobs; i++)
	    {
	      nac = colArray.getColumn(lobNumList[i]);
	      
	      nac->lobNum() = lobNumList[i];
	      nac->lobStorageType() = (LobsStorage)lobTypList[i];
	      nac->lobStorageLocation() = lobLocList[i];
	    }
	} // if
    } // if
  
// LCOV_EXCL_STOP
  initialSize_ = heap_->getAllocSize();
  MonitorMemoryUsage_Exit((char*)mmPhase.data(), heap_, NULL, TRUE);
} // NATable()
#pragma warn(770)  // warning elimination



// Constructor for a Hive table
NATable::NATable(BindWA *bindWA,
                 const CorrName& corrName,
		 NAMemory *heap,
		 struct hive_tbl_desc* htbl)
  //
  // The NATable heap ( i.e. heap_ ) used to come from ContextHeap
  // (i.e. heap) but it creates high memory usage/leakage in Context
  // Heap. Although the NATables are deleted at the end of each statement,
  // the heap_ is returned to heap (i.e. context heap) which caused
  // context heap containing a lot of not used chunk of memory. So it is
  // changed to be from whatever heap is passed in at the call in
  // NATableDB.getNATable.
  //
  // Now NATable objects can be cached.If an object is to be cached (persisted
  // across statements) a NATable heap is allocated for the object
  // and is passed in (this is done in NATableDB::get(CorrName& corrName...).
  // Otherwise a reference to the Statement heap is passed in. When a cached
  // object is to be deleted the object's heap is deleted which wipes out the
  // NATable object all its related stuff. NATable objects that are not cached
  // are wiped out at the end of the statement when the statement heap is deleted.
  //
  : heap_(heap),
    referenceCount_(0),
    refsIncompatibleDP2Halloween_(FALSE),
    isHalloweenTable_(FALSE),
    qualifiedName_(corrName.getExtendedQualNameObj(),heap),
    synonymReferenceName_(heap),
    fileSetName_(corrName.getQualifiedNameObj(),heap),   // for now, set equal
    clusteringIndex_(NULL),
    colcount_(0),
    colArray_(heap),
    recordLength_(0),
    indexes_(heap),
    vertParts_(heap),
    colStats_(NULL),
    statsFetched_(FALSE),
    viewFileName_(NULL),
    viewText_(NULL),
    viewTextInNAWchars_(heap),
    viewTextCharSet_(CharInfo::UnknownCharSet),
    viewCheck_(NULL),
    flags_(IS_INSERTABLE | IS_UPDATABLE),
    insertMode_(COM_REGULAR_TABLE_INSERT_MODE),
    isSynonymTranslationDone_(FALSE),
    checkConstraints_(heap),
    createTime_(htbl->creationTS_),
    redefTime_(htbl->redeftime()),
    cacheTime_(0),
    statsTime_(0),
    catalogUID_(0),
    schemaUID_(0),
    objectUID_(0),
    objectType_(COM_UNKNOWN_OBJECT),
    partitioningScheme_(COM_UNKNOWN_PARTITIONING),
    uniqueConstraints_(heap),
    refConstraints_(heap),
    isAnMV_(FALSE),
    isAnMVMetaData_(FALSE),
    mvsUsingMe_(heap),
    mvInfo_(NULL),
    accessedInCurrentStatement_(TRUE),
    setupForStatement_(FALSE),
    resetAfterStatement_(FALSE),
    hitCount_(0),
    replacementCounter_(2),
    tableConstructionHadWarnings_(FALSE),
    isAnMPTableWithAnsiName_(FALSE),
    isUMDTable_(FALSE),
    isSMDTable_(FALSE),
    isMVUMDTable_(FALSE),

    // For virtual tables, we set the object schema version
    // to be the current schema version
    osv_(COM_VERS_CURR_SCHEMA),
    ofv_(COM_VERS_CURR_SCHEMA),
    partnsDesc_(NULL),
    colsWithMissingStats_(NULL),
    originalCardinality_(-1.0),
    tableIdList_(heap),
    rcb_(NULL),
    rcbLen_(0),
    keyLength_(0),
    parentTableName_(NULL),
    sgAttributes_(NULL),
    isHive_(TRUE),
    isHbase_(FALSE),
    isHbaseCell_(FALSE),
    isHbaseRow_(FALSE),
    isSeabase_(FALSE),
    isSeabaseMD_(FALSE),
    isSeabasePrivSchemaTable_(FALSE),
    isUserUpdatableSeabaseMD_(FALSE),
    resetHDFSStatsAfterStmt_(FALSE),
    hiveDefaultStringLen_(0),
    hiveTableId_(htbl->tblID_),
    tableDesc_(NULL),
    secKeySet_(heap),
    privInfo_(NULL),
    newColumns_(heap),
    snapshotName_(NULL)
{

  NAString tblName = qualifiedName_.getQualifiedNameObj().getQualifiedNameAsString();
  NAString mmPhase;

  Lng32 preCreateNATableWarnings = CmpCommon::diags()->getNumber(DgSqlCode::WARNING_);

  //set heap type
  if(heap_ == CmpCommon::statementHeap()){
    heapType_ = STATEMENT;
    mmPhase = "NATable Init (Stmt) - " + tblName;
  }else if (heap_ == CmpCommon::contextHeap()){
    heapType_ = CONTEXT;
    mmPhase = "NATable Init (Cnxt) - " + tblName;
  }else {
    heapType_ = OTHER;
    mmPhase = "NATable Init (Other) - " + tblName;
  }

  MonitorMemoryUsage_Enter((char*)mmPhase.data(), heap_, TRUE);


  isTrigTempTable_ = FALSE;


  insertMode_ = 
     COM_MULTISET_TABLE_INSERT_MODE; // allow dup, to check
     //ComInsertMode::COM_MULTISET_TABLE_INSERT_MODE; // allow dup, to check

  //
  // Add timestamp information.
  //

  // To get from Hive
/*
  createTime_ = longArrayToInt64(table_desc->body.table_desc.createtime);
  redefTime_  = longArrayToInt64(table_desc->body.table_desc.redeftime);
  cacheTime_  = longArrayToInt64(table_desc->body.table_desc.cachetime);
*/

  // To get from the qualified name for catalog and schema
/*
  catalogUID_ = longArrayToInt64(table_desc->body.table_desc.catUID);
  schemaUID_ = longArrayToInt64(table_desc->body.table_desc.schemaUID);
  objectUID_ = longArrayToInt64(table_desc->body.table_desc.objectUID);
*/

/*

  if (table_desc->body.table_desc.owner)
    {
#ifdef NA_WINNT
      NAUserInfo userInfo;
      userInfo.setUserId(table_desc->body.table_desc.owner, TRUE);
#else
      NAUserInfo userInfo (table_desc->body.table_desc.owner);
#endif
      owner_ = userInfo;
    }
  if (table_desc->body.table_desc.schemaOwner)
    {
#ifdef NA_WINNT
      NAUserInfo schemaUser;
      schemaUser.setUserId(table_desc->body.table_desc.schemaOwner, TRUE);
#else
      NAUserInfo schemaUser(table_desc->body.table_desc.schemaOwner);
#endif
      schemaOwner_ = schemaUser;
    }
*/


// What object type code to use? 
  objectType_ = COM_BASE_TABLE_OBJECT;

// to check
  partitioningScheme_ = COM_UNKNOWN_PARTITIONING;

// to check
  rcb_ = 0;
  rcbLen_ = 0;
  keyLength_ = 0;

  partnsDesc_ = NULL;

  //
  // Insert a NAColumn in the colArray_ for this NATable for each
  // columns_desc from the ARK SMD. Returns TRUE if error creating NAColumns.
  //

  if (createNAColumns(htbl->getColumns(),
		      this,
		      colArray_ /*OUT*/,
		      heap_))
    //coverity[leaked_storage]
    return;


  //
  // Set colcount_ after all possible errors (Binder uses nonzero colcount
  // as an indicator of valid table definition).
  //

  // To set it via the new createNAColumns()
  colcount_ = colArray_.entries();

  // compute record length from colArray

  Int32 recLen = 0;
  for ( CollIndex i=0; i<colcount_; i++ ) {
    recLen += colArray_[i]->getType()->getNominalSize();
  } 

  setRecordLength(recLen);

  if (createNAFileSets(htbl             /*IN*/,
                       this             /*IN*/,
                       colArray_        /*IN*/,
                           indexes_         /*OUT*/,
                           vertParts_       /*OUT*/,
                           clusteringIndex_ /*OUT*/,
                           tableIdList_     /*OUT*/,
                           heap_,
                           bindWA
                           )) {
    colcount_ = 0; // indicates failure
    return;
  }

  // HIVE-TBD ignore constraint info creation for now


  // If there is a host variable associated with this table, store it
  // for use by the generator to generate late-name resolution information.
  //
  HostVar *hv = corrName.getPrototype();
  prototype_ = hv ? new (heap_) HostVar(*hv) : NULL;

  // MV
  // Initialize the MV support data members
  isAnMV_           = FALSE;
  isAnMVMetaData_   = FALSE;

  Lng32 postCreateNATableWarnings = CmpCommon::diags()->getNumber(DgSqlCode::WARNING_);

  if(postCreateNATableWarnings != preCreateNATableWarnings)
    tableConstructionHadWarnings_=TRUE;

  hiveDefaultStringLen_ = CmpCommon::getDefaultLong(HIVE_MAX_STRING_LENGTH);

  if (!(corrName.isSeabaseMD() || corrName.isSpecialTable()))
    setupPrivInfo();

// LCOV_EXCL_STOP
  initialSize_ = heap_->getAllocSize();
  MonitorMemoryUsage_Exit((char*)mmPhase.data(), heap_, NULL, TRUE);
} // NATable()
#pragma warn(770)  // warning elimination




NABoolean NATable::doesMissingStatsWarningExist(CollIndexSet & colsSet) const
{
  return colsWithMissingStats_->contains(&colsSet);
}

NABoolean NATable::insertMissingStatsWarning(CollIndexSet colsSet) const
{
  CollIndexSet * setOfColsWithMissingStats = new (STMTHEAP) CollIndexSet (colsSet);

  Int32 someVar = 1;
  CollIndexSet * result = colsWithMissingStats_->insert(setOfColsWithMissingStats, &someVar);

  if (result == NULL)
    return FALSE;
  else
    return TRUE;
}

// This gets called in the Optimizer phase -- the Binder phase will already have
// marked columns that were referenced in the query, so that the ustat function
// below can decide which histograms and histints to leave in the stats list
// and which to remove.
//
StatsList &
NATable::getStatistics()
{
   // HIVE-TBD
   if ( isHiveTable() ) {

       NAMemory* heap = CmpCommon::statementHeap();

       if ( colStats_ == NULL ) {
	  colStats_ = new (heap) StatsList(CmpCommon::statementHeap());
       }

       return *colStats_;

//       // side-affect NAColumn in colArray on needHistogram() and 
//       // needFullHistogram()
//       markColumnsForHistograms();
//
//       NAColumnArray& colArray = const_cast<NAColumnArray&>(getNAColumnArray());
//
//       for(UInt32 i=0;i<colArray.entries();i++)
//       {
//          //get a reference to the column
//          NAColumn * column = colArray[i];
//
//          if ( column->needHistogram() ) {
//
//            ComUID id(ColStats::nextFakeHistogramID());
//
//            ColStatsSharedPtr colStatsPtr = new (heap)
//                ColStats (id,
//                          CostScalar(1000), /*CS uec*/
//                          CostScalar(2200), /* rowcount in CS */
//                          CostScalar(-1),  // baseRC
//                          FALSE, // NABoolean unique 
//                          0,     // Int32 avgVarcharSize = 0
//                          heap   // NAMemory* heap=0
//                         );
//
//             colStats_->insertAt(colStats_->entries(), colStatsPtr);
//          }
//       }
//       return *colStats_;
   }

    if (!statsFetched_)
    {
      // mark the kind of histograms needed for this table's columns
      markColumnsForHistograms();

      NAString tblName = qualifiedName_.getQualifiedNameObj().getQualifiedNameAsString();
      NAString mmPhase = "NATable getStats - " + tblName;
      MonitorMemoryUsage_Enter((char*)mmPhase.data(), NULL, TRUE);

      //trying to get statistics for a new statement allocate colStats_
      colStats_ = new (CmpCommon::statementHeap()) StatsList(CmpCommon::statementHeap());

      // Do not create statistics on the fly for the following tables
      if (isAnMV() || isUMDTable() ||
          isSMDTable() || isMVUMDTable() ||
          isTrigTempTable() )
        CURRSTMT_OPTDEFAULTS->setHistDefaultSampleSize(0);

      CURRCONTEXT_HISTCACHE->getHistograms(*this);

      if ((*colStats_).entries() > 0)
        originalCardinality_ = (*colStats_)[0]->getRowcount();
      else
        originalCardinality_ = ActiveSchemaDB()->getDefaults().getAsDouble(HIST_NO_STATS_ROWCOUNT);

      // -----------------------------------------------------------------------
      // So now we have read in the contents of the HISTOGRM & HISTINTS
      // tables from the system catalog.  Before we can use them, we need
      // to massage them into a format we can use.  In particular, we need
      // to make sure that what we read in (which the user may have mucked
      // about with) matches the histogram classes' internal semantic
      // requirements.  Also, we need to generate the MultiColumnUecList.
      //  ----------------------------------------------------------------------

      // what did the user set as the max number of intervals?
      NADefaults &defs = ActiveSchemaDB()->getDefaults();
      CollIndex maxIntervalCount = defs.getAsLong(HIST_MAX_NUMBER_OF_INTERVALS);

      //-----------------------------------------------------------------------------------
      // Need to flag the MC colStatsDesc so it is only used for the range partitioning task
      // and not any cardinality calculations tasks. Flagging it also makes the logic
      // to check fo the presence for this MC easier (at the time we need to create
      // the range partitioning function)
      //-----------------------------------------------------------------------------------

      if (CmpCommon::getDefault(HBASE_RANGE_PARTITIONING_MC_SPLIT) == DF_ON && 
          !(*colStats_).allFakeStats())
      {
         CollIndex currentMaxsize = 1;
         Int32 posMCtoUse = -1;

         NAColumnArray partCols;

         if (getClusteringIndex()->getPartitioningKeyColumns().entries() > 0)
             partCols = getClusteringIndex()->getPartitioningKeyColumns();
         else
             partCols = getClusteringIndex()->getIndexKeyColumns();

         CollIndex partColNum = partCols.entries();

         // look for MC histograms that have multiple intervals and whose columns are a prefix for the
         // paritition column list. If multiple pick the one with the most matching columns
         for (Int32 i=0; i < (*colStats_).entries(); i++)
         {
            NAColumnArray statsCols = (*colStats_)[i]->getStatColumns();
            CollIndex colNum = statsCols.entries();
   
            CollIndex j = 0;
                
            NABoolean potentialMatch = TRUE;
            if ((colNum > currentMaxsize) && 
                (!(*colStats_)[i]->isSingleIntHist()) && // no SIH -- number of histograms is large enough to do splitting
                (colNum <= partColNum))
            {
                while ((j < colNum) && potentialMatch)
                {
                   j++;
                   NAColumn * col = partCols[j-1];
                   if (statsCols[j-1]->getPosition() != partCols[j-1]->getPosition())
                   {
                      potentialMatch = FALSE;
                      break;
                   }   
                }
            }
            else
            {
               potentialMatch = FALSE;
            }
         
            if (potentialMatch)
            {
               currentMaxsize = j;
               posMCtoUse = i;
            }
   
            // we got what we need, just return
            if (potentialMatch && (currentMaxsize == partColNum))
            {
                break;
            }
         }

         if (posMCtoUse >= 0)
         {
            (*colStats_)[posMCtoUse]->setMCforHbasePartitioning (TRUE);
         }
      }

      // *************************************************************************
      // FIRST: Generate the stats necessary to later create the
      // MultiColumnUecList; then filter out the multi-column histograms
      // because later code doesn't know how to handle them
      // In the same loop, also mark another flag for originally fake histogram
      // This is to differentiate the cases when the histogram is fake because
      // it has no statistics and the case where the histogram has been termed
      // fake by the optimizer because its statistics is no longer reliable.
      // *************************************************************************
      CollIndex i ;
      for ( i = 0 ; i < (*colStats_).entries() ; /* no automatic increment */ )
        {
          // the StatsList has two lists which it uses to store the information we
          // need to fill the MultiColumnUecList with <table-col-list,uec value> pairs:
          //
          // LIST(NAColumnArray) groupUecColumns_
          // LIST(CostScalar)    groupUecValues_
          //
          // ==> insert the NAColumnArray & uec total values for each
          // entry in colStats_

          // don't bother storing multicolumnuec info for fake histograms
	  // but do set the originallly fake histogram flag to TRUE
          if ( (*colStats_)[i]->isFakeHistogram() )
	    (*colStats_)[i]->setOrigFakeHist(TRUE);
	  else
            {
              NAColumnArray cols = (*colStats_)[i]->getStatColumns() ;
              (*colStats_).groupUecColumns_.insert(cols) ;

              CostScalar uecs = (*colStats_)[i]->getTotalUec() ;
              (*colStats_).groupUecValues_.insert(uecs) ;

              if (CmpCommon::getDefault(USTAT_COLLECT_MC_SKEW_VALUES) == DF_ON)
              {
                 MCSkewedValueList mcSkewedValueList = (*colStats_)[i]->getMCSkewedValueList() ;
                 (*colStats_).groupMCSkewedValueLists_.insert(mcSkewedValueList) ;
              }
	    }

          // MCH:
          // once we've stored the column/uec information, filter out the
          // multi-column histograms, since our synthesis code doesn't
          // handle them
          if (( (*colStats_)[i]->getStatColumns().entries() != 1) &&
              (!(*colStats_)[i]->isMCforHbasePartitioning()))
          {
            (*colStats_).removeAt(i) ;
          }
          else
          {
            i++ ; // in-place removal from a list is a bother!
          }
        }

      // *************************************************************************
      // SECOND: do some fixup work to make sure the histograms maintain
      // the semantics we later expect (& enforce)
      // *************************************************************************

      // -------------------------------------------------------------------------
      // HISTINT fixup-code : char-string histograms
      // -------------------------------------------------------------------------
      // problem arises with HISTINTs that are for char* columns
      // here's what we can get:
      //
      // Rows    Uec    Value
      // ----    ---    -----
      //    0      0    "value"
      //   10      5    "value"
      //
      // this is not good!  The problem is our (lousy) encoding of
      // char strings into EncodedValue's
      //
      // After much deliberation, here's our current fix:
      //
      // Rows    Uec    Value
      // ----    ---    -----
      //    0      0    "valu" <-- reduce the min value of 1st interval
      //   10      5    "value"    by a little bit
      //
      // When we find two intervals like this where they aren't the
      // first intervals in the histogram, we simply merge them into
      // one interval (adding row/uec information) and continue; note
      // that in this case, we haven't actually lost any information;
      // we've merely made sense out of (the garbage) what we've got
      //
      // -------------------------------------------------------------------------
      // additional HISTINT fixup-code
      // -------------------------------------------------------------------------
      // 1. If there are zero or one HISTINTs, then set the HISTINTs to match
      // the max/min information contained in the COLSTATS object.
      //
      // 2. If there are any HISTINTs whose boundary values are out-of-order,
      // we abort with an an ERROR message.
      //
      // 3. If there is a NULL HISTINT at the end of the Histogram, then we
      // need to make sure there are *TWO* NULL HISTINTS, to preserve correct
      // histogram semantics for single-valued intervals.
      // -------------------------------------------------------------------------

      CollIndex j ;
      for ( i = 0 ; i < (*colStats_).entries() ; i++ )
        {
          // we only worry about histograms on char string columns
          // correction: it turns out that these semantically-deranged
          // ----------  histograms were being formed for other, non-char string
          //             columns, so we commented out the code below
          // if ( colStats_[i]->getStatColumns()[0]->getType()->getTypeQualifier() !=
          //     NA_CHARACTER_TYPE)
          //   continue ; // not a string, skip to next

          ColStatsSharedPtr stats = (*colStats_)[i] ;

          HistogramSharedPtr hist = stats->getHistogramToModify() ;
          // histograms for key columns of a table that are not
          // referenced in the query are read in with zero intervals
          // (to conserve memory); however, internal
          // histogram-semantic checking code assumes that any
          // histogram which has zero intervals is FAKE; however
          // however, MDAM will not be chosen in the case where one of
          // the histograms for a key column is FAKE.  Thus -- we will
          // avoid this entire issue by creating a single interval for
          // any Histograms that we read in that are empty.
          if ( hist->entries() < 2 )
            {
              if(stats->getMinValue() > stats->getMaxValue())
	      {
	      *CmpCommon::diags() << DgSqlCode(CATALOG_HISTOGRM_HISTINTS_TABLES_CONTAIN_BAD_VALUE)
		     << DgString0("")
		     << DgString1(stats->getStatColumns()[0]->getFullColRefNameAsAnsiString().data() );

		stats->createFakeHist();
		continue;
	      }

              stats->setToSingleInterval ( stats->getMinValue(),
                                           stats->getMaxValue(),
                                           stats->getRowcount(),
                                           stats->getTotalUec() ) ;
              // now we have to undo some of the automatic flag-setting
              // of ColStats::setToSingleInterval()
              stats->setMinSetByPred (FALSE) ;
              stats->setMaxSetByPred (FALSE) ;
              stats->setShapeChanged (FALSE) ;
              continue ; // skip to next ColStats
            }

          // NB: we'll handle the first Interval last
          for ( j = 1 ; j < hist->entries()-1 ; /* no automatic increment */ )
            {

              if ( (*hist)[j].getUec() == 0 || (*hist)[j].getCardinality() == 0 )
                {
                  hist->removeAt(j) ;
                  continue ; // don't increment, loop again
                }

              // intervals must be in order!
              if ( (*hist)[j].getBoundary() > (*hist)[j+1].getBoundary() )
                {
		  *CmpCommon::diags() <<
                    DgSqlCode(CATALOG_HISTINTS_TABLES_CONTAIN_BAD_VALUES)
		    << DgInt0(j)
		    << DgInt1(j+1)
		    << DgString1(stats->getStatColumns()[0]->getFullColRefNameAsAnsiString().data() );

		    stats->createFakeHist();
		    break ; // skip to next ColStats
                }

              if ( (*hist)[j].getBoundary() == (*hist)[j+1].getBoundary() )
                {
                  // merge Intervals, if the two consecutive intervals have same 
                  // boundaries and these are not single valued (UEC > 1)
                  // If there are more two single valued intervals, then merge
                  // all except the last one.
                  NABoolean mergeIntervals = FALSE;

                  if (CmpCommon::getDefault(COMP_BOOL_79) == DF_ON)
                  {
                    mergeIntervals = TRUE;

                    if( (j < (hist->entries() - 2)) && ((*hist)[j+1].getUec() == 1) &&
                        ((*hist)[j+1].getBoundary() != (*hist)[j+2].getBoundary())
                        ||
                        (j == (hist->entries() - 2)) && ((*hist)[j+1].getUec() == 1) )
                      mergeIntervals = FALSE;
                  }
                  else
                  {
                    if ( (*hist)[j+1].getUec() > 1)
                      mergeIntervals = TRUE;
                  }

                  if ( mergeIntervals ) 
                  {
                    // if the intervals with same boundary are not SVI, just merge them 
                    // together.
                    // Also do the merge, if there are more than one SVIs with same 
                    // encoded interval boundary. Example, we want to avoid intervals
                    // such as
                    //   boundary   inclusive_flag  UEC
                    //   12345.00    <               1
                    //   12345.00    <               1
                    //   12345.00    <=              1
                    // These would be changed to 
                    //   12345.00    <               2
                    //   12345.00    <=              1
                    CostScalar combinedRows = (*hist)[ j ].getCardinality() +
                                              (*hist)[j+1].getCardinality() ;
                    CostScalar combinedUec  = (*hist)[ j ].getUec() +
                                              (*hist)[j+1].getUec() ;
                    (*hist)[j].setCardAndUec (combinedRows, combinedUec) ;
                    stats->setIsColWithBndryConflict(TRUE);
                     hist->removeAt(j+1) ;
                    }
                  else
                    {
                      // for some reason, some SVI's aren't being
                      // generated correctly!
                      (*hist)[j].setBoundIncl(FALSE) ;
                      (*hist)[j+1].setBoundIncl(TRUE) ;
                      j++;
                    }
                }
              else
                j++ ; // in-place removal from a list is a bother!
            } // loop over intervals

          // ----------------------------------------------------------------------
          // now we handle the first interval
          //
          // first, it must be in order w.r.t. the second interval!
          if ( (*hist)[0].getBoundary() > (*hist)[1].getBoundary() )
            {
              *CmpCommon::diags() <<
                DgSqlCode(CATALOG_HISTINTS_TABLES_CONTAIN_BAD_VALUES)
		<< DgInt0(0)
		<< DgInt1(1)
 		<< DgString1(stats->getStatColumns()[0]->getFullColRefNameAsAnsiString().data() );

		  stats->createFakeHist();
		  continue ; // skip to next ColStats
            }

          // second, handle the case where first and second interval are the same
          if ( hist->entries() > 1 && // avoid the exception! might just be a single NULL
               //                     // interval after the loop above
               (*hist)[0].getBoundary() == (*hist)[1].getBoundary() &&
               (*hist)[1].getUec() > 1 )
            {
               const double KLUDGE_VALUE = 0.0001 ;
               const double oldVal = (*hist)[0].getBoundary().getDblValue() ;
               const EncodedValue newVal =
                 EncodedValue(oldVal - (_ABSOLUTE_VALUE_(oldVal) * KLUDGE_VALUE)) ; // kludge alert!
										//Absolute of oldval due to CR 10-010426-2457
               (*hist)[0].setBoundary( newVal ) ;
               (*hist)[0].setBoundIncl( FALSE ) ; // no longer a real boundary!
               (*colStats_)[i]->setMinValue( newVal ) ; // set aggr info also
             }
          // done with first interval
          // ----------------------------------------------------------------------

          //
          // NULL values must only be stored in single-valued intervals
          // in the histograms ; so, just in case we're only getting
          // *one* HistInt for the NULL interval, insert a 2nd one
          //
          // 0   1   2
          // |   |   |
          // |   |   |    entries() == 3
          //         NULL
          //
          // 0   1   2   3
          // |   |   |   |
          // |   |   |   |    entries() == 4
          //        new  NULL
          //        NULL
          //
          if ( hist->lastHistInt().isNull() )
            {
              CollIndex count = hist->entries() ;
              if ( !(*hist)[count-2].isNull() )
              {
                // insert a 2nd NULL HISTINT, with boundaryIncl value FALSE
                HistInt secondLast (hist->lastHistInt().getBoundary(), FALSE) ;
                hist->insertAt(count-1,secondLast) ;
                // new HISTINT by default has row/uec of 0, which is what we want
              }
            }

          //
          // Now, reduce the total number of intervals to be the number
          // that the user wants.  This is used to test the tradeoffs
          // between compile time & rowcount estimation.
          //
          (*colStats_)[i]->setMaxIntervalCount (maxIntervalCount) ;
          (*colStats_)[i]->reduceToMaxIntervalCount () ;

	  if ((*colStats_)[i]->getRowcount() == (*colStats_)[i]->getTotalUec() )
		(*colStats_)[i]->setAlmostUnique(TRUE);

        } // outer for loop -- done with this COLSTATS, continue with next one
      // ***********************************************************************

      statsFetched_ = TRUE;
      MonitorMemoryUsage_Exit((char*)mmPhase.data(), NULL, NULL, TRUE);
    } // !statsFetched_

    return (*colStats_);
}

StatsList &
NATable::generateFakeStats()
{
  if (colStats_ == NULL)
  {
    //trying to get statistics for a new statement allocate colStats_
    colStats_ = new (CmpCommon::statementHeap()) StatsList(CmpCommon::statementHeap());
  }

  if (colStats_->entries() > 0)
    return (*colStats_);

  NAColumnArray colList = getNAColumnArray() ;
  double defaultFakeRowCount = (ActiveSchemaDB()->getDefaults()).getAsDouble(HIST_NO_STATS_ROWCOUNT);
  double defaultFakeUec = (ActiveSchemaDB()->getDefaults()).getAsDouble(HIST_NO_STATS_UEC);

  if ( isHiveTable() ) {
      defaultFakeRowCount = getOriginalRowCount().value();
  }

  /*  if ( isHbaseTable() ) {
      defaultFakeRowCount = getOriginalRowCount().value();
  }
  */

  for (CollIndex i = 0; i < colList.entries(); i++ )
  {
    NAColumn * col = colList[i];

    if (col->isUnique() )
      defaultFakeUec = defaultFakeRowCount;
    else
      defaultFakeUec = MINOF(defaultFakeUec, defaultFakeRowCount);

    EncodedValue dummyVal(0.0);

    EncodedValue lowBound = dummyVal.minMaxValue(col->getType(), TRUE);
    EncodedValue highBound = dummyVal.minMaxValue(col->getType(), FALSE);

    HistogramSharedPtr emptyHist(new (HISTHEAP) Histogram(HISTHEAP));

    HistInt newFirstHistInt(lowBound, FALSE);

    HistInt newSecondHistInt(highBound, TRUE);

    newSecondHistInt.setCardAndUec(defaultFakeRowCount,
                                   defaultFakeUec);

    emptyHist->insert(newFirstHistInt);
    emptyHist->insert(newSecondHistInt);

    ComUID histid(NA_JulianTimestamp());
    ColStatsSharedPtr fakeColStats(
                    new (HISTHEAP) ColStats(histid,
                    defaultFakeUec,
                    defaultFakeRowCount,
                    defaultFakeRowCount,
                    col->isUnique(),
                    FALSE,
                    emptyHist,
                    FALSE,
                    1.0,
                    1.0,
                    -1, // avg varchar size
                    HISTHEAP));

    fakeColStats->setFakeHistogram(TRUE);
    fakeColStats->setOrigFakeHist(TRUE);
    fakeColStats->setMinValue(lowBound);
    fakeColStats->setMaxValue(highBound);
    fakeColStats->statColumns().insert(col);

    colStats_->insert(fakeColStats);
  }
  setStatsFetched(TRUE);
  setOriginalRowCount(defaultFakeRowCount);

  return (*colStats_);
}

NABoolean NATable::rowsArePacked() const
{
  // If one fileset is packed, they all are
  return (getVerticalPartitionList().entries() &&
          getVerticalPartitionList()[0]->isPacked());
}

// MV
// Read materialized view information from the catalog manager.
MVInfoForDML *NATable::getMVInfo(BindWA *bindWA)
{
  return mvInfo_;
}

// MV
// An MV is usable unly when it is initialized and not unavailable.
// If not initialized, keep a list and report error at runtime.
NABoolean NATable::verifyMvIsInitializedAndAvailable(BindWA *bindWA) const
{
  CMPASSERT(isAnMV());
  const ComMvAttributeBitmap& bitmap = getMvAttributeBitmap();

  // First check if the table is Unavailable.
  NAString value;
  if (bitmap.getIsMvUnAvailable())
  {

    // 12312 Materialized View $0~TableName is unavailable.
    *CmpCommon::diags() << DgSqlCode(-12312)
	  << DgTableName(getTableName().getQualifiedNameAsString());
    bindWA->setErrStatus();

    return TRUE;
  }

  // if the mv is uninitialized,
  // add it to the uninitializedMvList in the BindWA
  if (bitmap.getIsMvUnInitialized())
  {
    
    // get physical and ansi names
    NAString fileName(
	 getClusteringIndex()->getFileSetName().getQualifiedNameAsString(),
	 bindWA->wHeap() );
    
    NAString ansiName( getTableName().getQualifiedNameAsAnsiString(),
		       bindWA->wHeap() );
    
    // get physical and ansi name
    bindWA->addUninitializedMv(
	 convertNAString( fileName, bindWA->wHeap() ),
	 convertNAString( ansiName, bindWA->wHeap() ) );
  }


  return FALSE;
}

// Return value: TRUE, found an index or constr. FALSE, not found.
// explicitIndex: get explicitly created index
// uniqueIndex: TRUE, get unique index. FALSE, any index.
// 
// primaryKeyOnly: TRUE, get primary key 
// indexName: return index name, if passed in
NABoolean NATable::getCorrespondingIndex(NAList<NAString> &inputCols,
					 NABoolean lookForExplicitIndex,
					 NABoolean lookForUniqueIndex,
					 NABoolean lookForPrimaryKey,
					 NABoolean lookForAnyIndexOrPkey,
					 NAString *indexName)
{
  NABoolean indexFound = FALSE;

  if (inputCols.entries() == 0)
    {
      lookForPrimaryKey = TRUE;
      lookForUniqueIndex = FALSE;
      lookForAnyIndexOrPkey = FALSE;
    }

  Lng32 numBTpkeys = getClusteringIndex()->getIndexKeyColumns().entries();

  const NAFileSetList &indexList = getIndexList();
  for (Int32 i = 0; (NOT indexFound && (i < indexList.entries())); i++)
    {
      NABoolean isPrimaryKey = FALSE;
      NABoolean isUniqueIndex = FALSE;

      const NAFileSet * naf = indexList[i];
      if (naf->getKeytag() == 0)
	isPrimaryKey = TRUE;
      else if (naf->uniqueIndex())
	isUniqueIndex = TRUE;

      if ((NOT lookForPrimaryKey) && (isPrimaryKey))
	continue;

      NABoolean found = FALSE;
      if (lookForAnyIndexOrPkey)
	found = TRUE;
      else if (lookForPrimaryKey && isPrimaryKey)
	found = TRUE;
      else if (lookForUniqueIndex && isUniqueIndex)
	found = TRUE;

      if (found)
	{
	  if (lookForExplicitIndex)  // need an explicit index to match.
	    {
	      if ((naf->isCreatedExplicitly()) ||
		  (isPrimaryKey))
		found = TRUE;
	      else
		found = FALSE;
	    }
	}

      if (NOT found)
	continue;

      const NAColumnArray &nacArr = naf->getIndexKeyColumns();

      Lng32 numKeyCols = 
	(isPrimaryKey ? nacArr.entries() : 
         naf->getCountOfUserSpecifiedIndexCols());

      if (naf->numSaltPartns() > 0)
        numKeyCols-- ; // for salted index, the SALT column is counted
      // as a user specified column, but it is not present in inputCols
      // We want to disregard the salt column when looking for a match.

      if ((inputCols.entries() > 0) && (inputCols.entries() != numKeyCols))
	continue;

      NASet<NAString> keyColNAS(HEAP, inputCols.entries());
      NASet<NAString> indexNAS(HEAP, inputCols.entries());
      for (Int32 j = 0; j < numKeyCols; j++)
	{
	  const NAString &colName = inputCols[j];
	  keyColNAS.insert(colName);
          if (naf->numSaltPartns() > 0)
            indexNAS.insert(nacArr[j+1]->getColName()); //SALT column is first
          else                                          // skip it.
            indexNAS.insert(nacArr[j]->getColName());
	}

      if (inputCols.entries() == 0)
	indexFound = TRUE; // primary key
      else if (keyColNAS == indexNAS) // found a matching index
	indexFound = TRUE;
      
      if (indexFound)
	{
	  if (indexName)
	    {
	      *indexName = naf->getExtFileSetName();
	    }
	}
    }

  return indexFound;
}

NABoolean NATable::getCorrespondingConstraint(NAList<NAString> &inputCols,
					      NABoolean uniqueConstr,
					      NAString *constrName,
					      NABoolean * isPkey)
{
  NABoolean constrFound = FALSE;

  NABoolean lookForPrimaryKey = FALSE;
  if (inputCols.entries() == 0)
    lookForPrimaryKey = TRUE;

  const AbstractRIConstraintList &constrList = 
    (uniqueConstr ? getUniqueConstraints() : getRefConstraints());

  if (isPkey)
    *isPkey = FALSE;

  for (Int32 i = 0; (NOT constrFound && (i < constrList.entries())); i++)
    {
      AbstractRIConstraint *ariConstr = constrList[i];
 
      if (uniqueConstr && (ariConstr->getOperatorType() != ITM_UNIQUE_CONSTRAINT))
	continue;

      if (lookForPrimaryKey && (NOT ((UniqueConstraint*)ariConstr)->isPrimaryKeyConstraint()))
	continue;

      if ((NOT uniqueConstr) && (ariConstr->getOperatorType() != ITM_REF_CONSTRAINT))
	continue;

      if ((inputCols.entries() > 0) && (inputCols.entries() != ariConstr->keyColumns().entries()))
	continue;

      if (NOT lookForPrimaryKey)
	{
	  NASet<NAString> keyColNAS(HEAP, inputCols.entries());
	  NASet<NAString> constrNAS(HEAP, inputCols.entries());
	  for (Int32 j = 0; j < ariConstr->keyColumns().entries(); j++)
	    {
	      const NAString &colName = inputCols[j];
	      keyColNAS.insert(colName);
	      constrNAS.insert((ariConstr->keyColumns()[j])->getColName());
	    }
	  
	  if (keyColNAS == constrNAS)
	    constrFound = TRUE;
	}
      else
	constrFound = TRUE;
      
      if (constrFound)
	{
	  if (constrName)
	    {
	     *constrName = ariConstr->getConstraintName().getQualifiedNameAsAnsiString();
	   }

	 if (isPkey)
	   {
	     if ((uniqueConstr) && (((UniqueConstraint*)ariConstr)->isPrimaryKeyConstraint()))
	       *isPkey = TRUE;
	   }
       }
    }

  return constrFound;
}

void NATable::setupPrivInfo()
{
  Int32 thisUserID = ComUser::getCurrentUser();
  NAString privMDLoc = CmpSeabaseDDL::getSystemCatalogStatic();
  privMDLoc += ".\"";
  privMDLoc += SEABASE_PRIVMGR_SCHEMA;
  privMDLoc += "\"";

  PrivMgrCommands privInterface(privMDLoc.data(), CmpCommon::diags(),PrivMgr::PRIV_INITIALIZED);

  if (privInterface.isPrivMgrTable(
    qualifiedName_.getQualifiedNameObj().getQualifiedNameAsString().data()))
    {
      isSeabasePrivSchemaTable_ = TRUE;
      return;
    }

  privInfo_ = new(heap_) PrivMgrUserPrivs;
  if (!isSeabaseTable() || 
      !CmpCommon::context()->isAuthorizationEnabled() ||
      isVolatileTable() ||
      ComUser::isRootUserID()||
      ComUser::getCurrentUser() == owner_)
    {
      privInfo_->setOwnerDefaultPrivs();
      return;
    }

  std::vector <ComSecurityKey *> secKeyVec;

  bool testError = false;
#ifndef NDEBUG
  char *tpie = getenv("TEST_PRIV_INTERFACE_ERROR");
  if (tpie && *tpie == '1')
    testError = true;
#endif

  // use embedded compiler.
  CmpSeabaseDDL cmpSBD(STMTHEAP);
  if (cmpSBD.switchCompiler(CmpContextInfo::CMPCONTEXT_TYPE_META))
    {
      if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
        *CmpCommon::diags() << DgSqlCode( -4400 );

      return;
    }
  if (testError || (STATUS_GOOD !=
       privInterface.getPrivileges(objectUid().get_value(), thisUserID,
                                    *privInfo_, &secKeyVec)))
  {
    if (testError)
#ifndef NDEBUG
      *CmpCommon::diags() << DgSqlCode(-8142) <<
         DgString0("TEST_PRIV_INTERFACE_ERROR")  << DgString1(tpie) ;
#else
      abort();
#endif
    NADELETE(privInfo_, PrivMgrUserPrivs, heap_);
    privInfo_ = NULL;

  cmpSBD.switchBackCompiler();
  return;
  }

  CMPASSERT (privInfo_);

  cmpSBD.switchBackCompiler();

  for (std::vector<ComSecurityKey*>::iterator iter = secKeyVec.begin();
       iter != secKeyVec.end();
       iter++)
  {
    // Insertion of the dereferenced pointer results in NASet making
    // a copy of the object, and then we delete the original.
    secKeySet_.insert(**iter);
    delete *iter;
  }

}

Int64 lookupObjectUid( const QualifiedName& qualName
                     , ComObjectType objectType
                    )
{
  ExeCliInterface cliInterface(STMTHEAP);
  Int64 objectUID = 0;

  Lng32 diagsMark = CmpCommon::diags()->mark();

  CmpSeabaseDDL cmpSBD(STMTHEAP);
  objectUID = cmpSBD.getObjectUID(&cliInterface, 
                                  qualName.getCatalogName().data(),
                                  qualName.getSchemaName().data(),
                                  qualName.getObjectName().data(),
                                  comObjectTypeLit(objectType));

  if (objectUID <= 0)
    {
      // remove errors
      CmpCommon::diags()->rewind(diagsMark);
    }

  return objectUID;
}

// Query the metadata to find the object uid of the table. This is used when
// the uid for a metadata table is requested, since 0 is usually stored for
// these tables.
Int64 NATable::lookupObjectUid()
{
    QualifiedName qualName = getExtendedQualName().getQualifiedNameObj();
    objectUID_ = ::lookupObjectUid(qualName, objectType_);
    return objectUID_.get_value();
}

bool NATable::isEnabledForDDLQI() const
{
  if (isSeabaseMD_ || isSMDTable_ || (getSpecialType() == ExtendedQualName::VIRTUAL_TABLE))
    return false;
  else 
  {
    if (objectUID_.get_value() == 0)
    {
      // Looking up object UIDs at code-gen time was shown to cause
      // more than 10% performance regression in YCSB benchmark. In
      // that investigation, we learned that metadata and histogram 
      // NATables would have no object UID at code-gen and would 
      // require the lookup.  We're pretty sure these are the only 
      // types of tables but will abend here otherwise. If this 
      // causes problems, the envvar below can be used as a 
      // temporary workaround. 
      char *noAbendOnLp1398600 = getenv("NO_ABEND_ON_LP_1398600");
      if (!noAbendOnLp1398600 || *noAbendOnLp1398600 == '0')
        abort();
    }
    return true;
  }
}

NATable::~NATable()
{
  // remove the map entries of associated table identifers in
  // NAClusterInfo::tableToClusterMap_.
  CMPASSERT(gpClusterInfo);
  NAColumn *col;
  NABoolean delHeading = ActiveSchemaDB()->getNATableDB()->cachingMetaData();
  const LIST(CollIndex) & tableIdList = getTableIdList();
  for(CollIndex i = 0; i < tableIdList.entries(); i++)
  {
    gpClusterInfo->removeFromTableToClusterMap(tableIdList[i]);
  }
  if (privInfo_)
  {
    NADELETE(privInfo_, PrivMgrUserPrivs, heap_);
    privInfo_ = NULL;
  }
  if (! isHive_) {
     for (int i = 0 ; i < colcount_ ; i++) {
         col = (NAColumn *)colArray_[i];
         if (delHeading) {
            if (col->getDefaultValue())
                NADELETEBASIC(col->getDefaultValue(), heap_);
            if (col->getHeading())
                NADELETEBASIC(col->getHeading(), heap_);
            if (col->getComputedColumnExprString())
                NADELETEBASIC(col->getComputedColumnExprString(),heap_);
         }
         NADELETE(col->getType(), NAType, heap_);
         NADELETE(col, NAColumn, heap_);
     }
     colArray_.clear();
  }
  if (parentTableName_ != NULL)
  {
     NADELETEBASIC(parentTableName_, heap_);
     parentTableName_ = NULL;
  } 
  if (snapshotName_ != NULL)
  {
     NADELETEBASIC(snapshotName_, heap_);
     snapshotName_ = NULL;
  }
  if (viewText_ != NULL)
  {
     NADELETEBASIC(viewText_, heap_);
     viewText_ = NULL;
  } 
  if (viewCheck_ != NULL)
  {
     NADELETEBASIC(viewCheck_, heap_);
     viewCheck_ = NULL;
  } 
  if (viewFileName_ != NULL)
  {
     NADELETEBASIC(viewFileName_, heap_);
     viewFileName_ = NULL;
  } 
  if (prototype_ != NULL)
  {
     NADELETE(prototype_, HostVar, heap_);
     prototype_ = NULL;
  }
  if (sgAttributes_ != NULL)
  {
     NADELETE(sgAttributes_, SequenceGeneratorAttributes, heap_);
     sgAttributes_ = NULL;
  }
  // clusteringIndex_ is part of indexes - No need to delete clusteringIndex_
  CollIndex entryCount = indexes_.entries();
  for (CollIndex i = 0 ; i < entryCount; i++) {
      NADELETE(indexes_[i], NAFileSet, heap_);
  }
  indexes_.clear();
  entryCount  = vertParts_.entries();
  for (CollIndex i = 0 ; i < entryCount; i++) {
      NADELETE(vertParts_[i], NAFileSet, heap_);
  }
  vertParts_.clear();
  entryCount  = newColumns_.entries();
  for (int i = 0 ; i < entryCount ; i++) {
      col = (NAColumn *)newColumns_[i];
      NADELETE(col, NAColumn, heap_);
  }
  newColumns_.clear();
  entryCount  = checkConstraints_.entries();
  for (CollIndex i = 0 ; i < entryCount; i++) {
      NADELETE(checkConstraints_[i], CheckConstraint, heap_);
  }
  checkConstraints_.clear();
  entryCount  = uniqueConstraints_.entries();
  for (CollIndex i = 0 ; i < entryCount; i++) {
      NADELETE((UniqueConstraint *)uniqueConstraints_[i], UniqueConstraint, heap_);
  }
  uniqueConstraints_.clear();
  entryCount  = refConstraints_.entries();
  for (CollIndex i = 0 ; i < entryCount; i++) {
      NADELETE((RefConstraint *)refConstraints_[i], RefConstraint, heap_);
  }
  refConstraints_.clear();
  entryCount  = mvsUsingMe_.entries();
  for (CollIndex i = 0 ; i < entryCount; i++) {
      NADELETE(mvsUsingMe_[i], UsingMvInfo, heap_);
  }
  mvsUsingMe_.clear();
  // mvInfo_ is not used at all
  // tableIDList_ is list of ints - No need to delete the entries
  // colStats_ and colsWithMissingStats_ comes from STMTHEAP
  // secKeySet_ is the set that holds ComSecurityKeySet object itself
}

void NATable::resetAfterStatement() // ## to be implemented?
{
  if(resetAfterStatement_)
    return;
  //It is not clear to me whether virtual tables and resource forks
  //(any "special" table type) can/should be reused.  Maybe certain
  //types can; I just have no idea right now.  But as we're not reading
  //metadata tables for them anyway, there seems little savings in
  //caching them; perhaps we should just continue to build them on the fly.
  //
  //All the real metadata in NATable members can stay as it is.
  //But there are a few pieces of for-this-query-only data:

  referenceCount_ = 0;
  refsIncompatibleDP2Halloween_ = FALSE;
  isHalloweenTable_ = FALSE;
  //And we now optimize/filter/reduce histogram statistics for each
  //individual query, so stats and adminicular structures must be reset:

  statsFetched_ = FALSE;

  //set this to NULL, the object pointed to by mvInfo_ is on the
  //statement heap, for the next statement this will be set again
  //this is set in 'MVInfoForDML *NATable::getMVInfo' which is called
  //in the binder after the construction of the NATable. Therefore
  //This will be set for every statement
  mvInfo_ = NULL;

  //delete/clearAndDestroy colStats_
  //set colStats_ pointer to NULL the object itself is deleted when
  //the statement heap is disposed at the end of a statement
  colStats_ = NULL;

  //mark table as unaccessed for following statements
  accessedInCurrentStatement_ = FALSE;

  //for (i in colArray_) colArray_[i]->setReferenced(FALSE);
  for (UInt32 i = 0; i < colArray_.entries(); i++)
  {
    //reset each NAColumn
    if(colArray_[i])
      colArray_[i]->resetAfterStatement();
  }

  //reset the clustering index
  if(clusteringIndex_)
    clusteringIndex_->resetAfterStatement();

  //reset the fileset for indices
  for (UInt32 j=0; j < indexes_.entries(); j++)
  {
    //reset the fileset for each index
    if(indexes_[j])
      indexes_[j]->resetAfterStatement();
  }

  //reset the fileset for each vertical partition
  for (UInt32 k=0; k < vertParts_.entries(); k++)
  {
    //reset the fileset for each index
    if(vertParts_[k])
      vertParts_[k]->resetAfterStatement();
  }

  // reset the pointers (keyColumns_ in refConstraintsReferencingMe)
  // that are referencing the NATable of the 'other table'.
  uniqueConstraints_.resetAfterStatement();

  // reset the pointers (keyColumns_ in uniqueConstraintsReferencedByMe_)
  // that are referencing the NATable of the 'other table'.
  refConstraints_.resetAfterStatement();

  colsWithMissingStats_ = NULL;

  resetAfterStatement_ = TRUE;
  setupForStatement_ = FALSE;

  sizeAfterLastStatement_ = heap_->getAllocSize();
  return;
}

void NATable::setupForStatement()
{

  if(setupForStatement_)
    return;

  if( NOT qualifiedName_.isSpecialTable() )
    gpClusterInfo->setMaxOSV(qualifiedName_.getQualifiedNameObj(), osv_);

  //reset the clustering index
  if(clusteringIndex_)
    clusteringIndex_->setupForStatement();

  //reset the fileset for indices
  for (UInt32 i=0; i < indexes_.entries(); i++)
  {
    //reset the fileset for each index
    if(indexes_[i])
      indexes_[i]->setupForStatement();
  }

  //reset the fileset for each vertical partition
  for (UInt32 j=0; j < vertParts_.entries(); j++)
  {
    //reset the fileset for each index
    if(vertParts_[j])
      vertParts_[j]->setupForStatement();
  }

  // We are doing this here, as we want this to be maintained on a per statement basis
  colsWithMissingStats_ = new (STMTHEAP) NAHashDictionary<CollIndexSet, Int32>
    (&(hashColPosList),107,TRUE,STMTHEAP);

  setupForStatement_ = TRUE;
  resetAfterStatement_ = FALSE;

  return;
}

static void formatPartitionNameString(const NAString &tbl,
				 const NAString &pName,
				 NAString &fmtOut)
{
  fmtOut = NAString("(TABLE ") + tbl +
           ", PARTITION " + pName + ")";
}
static void formatPartitionNumberString(const NAString &tbl,
				 Lng32 pNumber,
				 NAString &fmtOut)
{
  char buf[10];
  sprintf(buf, "%d", pNumber);

  fmtOut = NAString("(TABLE ") + tbl +
           ", PARTITION NUMBER " + buf + ")";
}

NABoolean NATable::filterUnusedPartitions(const PartitionClause& pClause)
{
  if (pClause.isEmpty())
    return TRUE;

  if (getViewText())
    {
      *CmpCommon::diags()
	<< DgSqlCode(-1276)
	<< DgString0(pClause.getPartitionName())
	<< DgTableName(getTableName().getQualifiedNameAsString());
      return TRUE;
    }
  
  if ((pClause.partnNumSpecified() && pClause.getPartitionNumber() < 0) ||
      (pClause.partnNameSpecified() && IsNAStringSpaceOrEmpty(pClause.getPartitionName())))
    // Partion Number specified is less than zero or name specified was all blanks.
    return TRUE ;

  CMPASSERT(indexes_.entries() > 0);
  NAFileSet* baseTable = indexes_[0];
  PartitioningFunction* oldPartFunc = baseTable->getPartitioningFunction();
  CMPASSERT(oldPartFunc);
  const NodeMap* oldNodeMap = oldPartFunc->getNodeMap();
  CMPASSERT(oldNodeMap);
  const NodeMapEntry* oldNodeMapEntry = NULL;
  PartitioningFunction* newPartFunc = NULL; 
  if (pClause.partnRangeSpecified())
    {
      /*      if (NOT oldPartFunc->isAHash2PartitioningFunction())
	{
	  // ERROR 1097 Unable to find specified partition...
	  *CmpCommon::diags()
	    << DgSqlCode(-1097)
	    << DgString0("")
	    << DgTableName(getTableName().getQualifiedNameAsAnsiString());
	  return TRUE;
	}
	*/

      NAString errorString;
      // partition range specified
      if ((pClause.getBeginPartitionNumber() == -1) ||
	  ((pClause.getBeginPartitionNumber() > 0) &&
	   (oldPartFunc->getCountOfPartitions() >= pClause.getBeginPartitionNumber())))
	{
	  oldPartFunc->setRestrictedBeginPartNumber(
	       pClause.getBeginPartitionNumber());
	}
      else
	{
	  formatPartitionNumberString(
	       getTableName().getQualifiedNameAsAnsiString(),
	       pClause.getBeginPartitionNumber(), errorString);
	}

      if ((pClause.getEndPartitionNumber() == -1) ||
	  ((pClause.getEndPartitionNumber() > 0) &&
	   (oldPartFunc->getCountOfPartitions() >= pClause.getEndPartitionNumber())))
	{
	  oldPartFunc->setRestrictedEndPartNumber(
	       pClause.getEndPartitionNumber());
	}
      else
	{
	  formatPartitionNumberString(
	       getTableName().getQualifiedNameAsAnsiString(),
	       pClause.getEndPartitionNumber(), errorString);
	}

      if (NOT errorString.isNull())
	{
	  // ERROR 1097 Unable to find specified partition...
	  *CmpCommon::diags()
	    << DgSqlCode(-1097)
	    << DgString0(errorString)
	    << DgTableName(getTableName().getQualifiedNameAsAnsiString());
	  return TRUE;
	} // Unable to find specified partition.
    } // partition range specified
  else 
    {
      // single partition specified
      if (pClause.getPartitionNumber() >= 0) // PARTITION NUMBER was specified
	{ 
	  if ((pClause.getPartitionNumber() > 0) &&
	      (oldPartFunc->getCountOfPartitions() >= pClause.getPartitionNumber()))
	    oldNodeMapEntry = oldNodeMap->getNodeMapEntry(pClause.getPartitionNumber()-1);
	  else
	    {
	      NAString errorString;
	      formatPartitionNumberString(getTableName().getQualifiedNameAsAnsiString(),
					  pClause.getPartitionNumber(), errorString);
	      
	      // ERROR 1097 Unable to find specified partition...
	      *CmpCommon::diags()
		<< DgSqlCode(-1097)
		<< DgString0(errorString)
		<< DgTableName(getTableName().getQualifiedNameAsAnsiString());
	      return TRUE;
	    } // Unable to find specified partition.
	}
      else  // PARTITION NAME was specified
	{
	  for (CollIndex i =0; i < oldNodeMap->getNumEntries(); i++)
	    {
	      oldNodeMapEntry = oldNodeMap->getNodeMapEntry(i);
	      if (oldNodeMapEntry->getGivenName() == pClause.getPartitionName())
		break;
	      if ( i == (oldNodeMap->getNumEntries() -1)) // match not found
		{
		  NAString errorString;
		  formatPartitionNameString(getTableName().getQualifiedNameAsAnsiString(),
					    pClause.getPartitionName(), errorString);
		  
		  // ERROR 1097 Unable to find specified partition...
		  *CmpCommon::diags()
		    << DgSqlCode(-1097)
		    << DgString0(errorString)
		    << DgTableName(getTableName().getQualifiedNameAsAnsiString());
		  return TRUE;
		}
	    }
	}
      
      if (!isHbaseTable())
        {
          // Create DP2 node map for partitioning function with only the partition requested
          NodeMap* newNodeMap = new (heap_) NodeMap(heap_);
          NodeMapEntry newEntry((char *)oldNodeMapEntry->getPartitionName(),
                                (char *)oldNodeMapEntry->getGivenName(),
                                heap_,oldNodeMap->getTableIdent());
          newNodeMap->setNodeMapEntry(0,newEntry,heap_);
          newNodeMap->setTableIdent(oldNodeMap->getTableIdent());
          
          /*  if (oldPartFunc->getPartitioningFunctionType() == 
              PartitioningFunction::ROUND_ROBIN_PARTITIONING_FUNCTION)
              {
              // For round robin partitioning, must create the partitioning function
              // even for one partition, since the SYSKEY must be generated for
              // round robin and this is trigger off the partitioning function.
              newPartFunc = new (heap) RoundRobinPartitioningFunction(1, newNodeMap, heap_);
              }
              else */
          newPartFunc = new (heap_) SinglePartitionPartitioningFunction(newNodeMap, heap_);
          
          baseTable->setPartitioningFunction(newPartFunc);
          baseTable->setCountOfFiles(1);
          baseTable->setHasRemotePartitions(checkRemote(NULL,
                                                        (char *)oldNodeMapEntry->getPartitionName()));
          // for now we are not changing indexlevels_ It could potentially be larger than the 
          // number of index levels for the requested partition.
          QualifiedName physicalName(oldNodeMapEntry->getPartitionName(),
                                     1, heap_, NULL);
          baseTable->setFileSetName(physicalName);
        }
      else
        {
          // For HBase tables, we attach a predicate to select a single partition in Scan::bindNode
	  oldPartFunc->setRestrictedBeginPartNumber(pClause.getPartitionNumber());
	  oldPartFunc->setRestrictedEndPartNumber(pClause.getPartitionNumber());
        }

    } // single partition specified

  return FALSE;
}

const LIST(CollIndex) &
NATable::getTableIdList() const
{
  return tableIdList_;
}

void NATable::resetReferenceCount()
{ 
  referenceCount_ = 0;
  refsIncompatibleDP2Halloween_ = FALSE; 
  isHalloweenTable_ = FALSE; 
}

void NATable::decrReferenceCount()
{ 
  --referenceCount_; 
  if (referenceCount_ == 0)
    {
    refsIncompatibleDP2Halloween_ = FALSE; 
    isHalloweenTable_ = FALSE;
    }
}

CollIndex NATable::getUserColumnCount() const
{
  CollIndex result = 0;

  for (CollIndex i=0; i<colArray_.entries(); i++)
    if (colArray_[i]->isUserColumn())
      result++;

  return result;
}

// NATableDB function definitions
NATable * NATableDB::get(const ExtendedQualName* key, BindWA* bindWA, NABoolean findInCacheOnly)
{
  //get the cached NATable entry
  NATable * cachedNATable =
      NAKeyLookup<ExtendedQualName,NATable>::get(key);

  //entry not found in cache
  if(!cachedNATable)
    return NULL;

  //This flag determines if a cached object should be deleted and
  //reconstructed
  NABoolean removeEntry = FALSE;

  if ( cachedNATable->isHbaseTable() ) {

      const NAFileSet* naSet = cachedNATable -> getClusteringIndex();

      if ( naSet ) {
         PartitioningFunction* pf = naSet->getPartitioningFunction();

         if ( pf ) {
            NABoolean rangeSplitSaltedTable = 
              CmpCommon::getDefault(HBASE_HASH2_PARTITIONING) == DF_OFF ||
              (bindWA && bindWA->isTrafLoadPrep());
             
            // if force to range partition a salted table, and the salted table is 
            // not a range, do not return the cached object.
            if ( rangeSplitSaltedTable &&
                 cachedNATable->hasSaltedColumn() &&
                 pf->castToHash2PartitioningFunction() ) {
               removeEntry = TRUE;
            } else 
            // if force to hash2 partition a salted table, and the cached table is 
            // not a hash2, do not return the cached object.
            if ( 
                 CmpCommon::getDefault(HBASE_HASH2_PARTITIONING) != DF_OFF &&
                 cachedNATable->hasSaltedColumn() &&
                 pf->castToHash2PartitioningFunction() == NULL
               )
               removeEntry = TRUE;
         } 
     }
  }

  //Found in cache.  If that's all the caller wanted, return now.
  if ( !removeEntry && findInCacheOnly )
     return cachedNATable;

  //if this is the first time this cache entry has been accessed
  //during the current statement
  if( !removeEntry && !cachedNATable->accessedInCurrentStatement())
  {
    //Note: cachedNATable->labelDisplayKey_ won't not be NULL
    //for NATable Objects that are in the cache. If the object
    //is not a cached object from a previous statement then we
    //will not come into this code.

    //Read label to get time of last catalog operation
    short error = 0;
    //Get redef time of table
    const Int64 tableRedefTime = cachedNATable->getRedefTime();
    //Get last catalog operation time
    Int64 labelCatalogOpTime = tableRedefTime;
    Int64 rforkCatalogOpTime = tableRedefTime;
    Int64 currentSchemaRedefTS = 0;
    Int64 cachedSchemaRedefTS = 0;

    if (!OSIM_runningSimulation())
      {
        if ((!cachedNATable->isHiveTable()) &&
	    (!cachedNATable->isHbaseTable()))
          {
          } // non-hive table
        else if (!cachedNATable->isHbaseTable())
          {
            // oldest cache entries we will still accept
            // Values for CQD HIVE_METADATA_REFRESH_INTERVAL:
            // -1: Never invalidate any metadata
            //  0: Always check for the latest metadata in the compiler,
            //     no check in the executor
            // >0: Check in the compiler, metadata is valid n seconds
            //     (n = value of CQD). Recompile plan after n seconds.
            //     NOTE: n has to be long enough to compile the statement,
            //     values < 20 or so are impractical.
            Int64 refreshInterval = 
              (Int64) CmpCommon::getDefaultLong(HIVE_METADATA_REFRESH_INTERVAL);
            Int32 defaultStringLen = 
              CmpCommon::getDefaultLong(HIVE_MAX_STRING_LENGTH);
            Int64 expirationTimestamp = refreshInterval;
            NAString defSchema =
              ActiveSchemaDB()->getDefaults().getValue(HIVE_DEFAULT_SCHEMA);
            defSchema.toUpper();

            if (refreshInterval > 0)
              expirationTimestamp = NA_JulianTimestamp() - 1000000 * refreshInterval;

            // if default string length changed, don't reuse this entry
            if (defaultStringLen != cachedNATable->getHiveDefaultStringLen())
              removeEntry = TRUE;

            QualifiedName objName = cachedNATable->getTableName();
            NAString        sName = objName.getSchemaName();
            const NAString  tName = objName.getObjectName();

            // map the Trafodion default Hive schema (usually "HIVE")
            // to the name used in Hive (usually "default")
            if (objName.getUnqualifiedSchemaNameAsAnsiString() == defSchema)
              sName = hiveMetaDB_->getDefaultSchemaName();

            // validate Hive table timestamps
            if (!hiveMetaDB_->validate(cachedNATable->getHiveTableId(),
                                       cachedNATable->getRedefTime(),
                                       sName.data(), tName.data()))
              removeEntry = TRUE;

            // validate HDFS stats and update them in-place, if needed
            if (!removeEntry)
              removeEntry = 
                ! (cachedNATable->getClusteringIndex()->
                   getHHDFSTableStats()->validateAndRefresh(expirationTimestamp));
          }
      } // ! osim simulation

    //if time of last catalog operation and table redef times
    //don't match, then delete this cache entry since it is
    //stale.
    //if error is non-zero then we were not able to read file
    //label and therefore delete this cached entry because
    //we cannot ensure it is fresh.
    if((CmpCommon::statement()->recompiling())||
       (labelCatalogOpTime != tableRedefTime )||
       (error)||
       (currentSchemaRedefTS != cachedSchemaRedefTS) ||
       (!usingCache()) ||
       (refreshCacheInThisStatement_) ||
       (removeEntry == TRUE)) // to avoid unnecessary read of metadata
    {
      //mark this entry to be removed
      removeEntry = TRUE;
    }
  } // !cachedNATable->accessedInCurrentStatement()

  if(removeEntry)
  {
    //remove from list of cached NATables
    cachedTableList_.remove(cachedNATable);

    //remove pointer to NATable from cache
    remove(key);

    //if metadata caching is ON, then adjust cache size
    //since we are deleting a caching entry
    if(cacheMetaData_)
      currentCacheSize_ = heap_->getAllocSize();

    cachedNATable->removeTableToClusterMapInfo();

    //insert into list of tables that will be deleted
    //at the end of the statement after the query has
    //been compiled and the plan has been sent to the
    //executor. The delete is done in method
    //NATableDB::resetAfterStatement(). This basically
    //gives a little performance saving because the delete
    //won't be part of the compile time as perceived by the
    //client of the compiler
    tablesToDeleteAfterStatement_.insert(cachedNATable);

    return NULL;
  }
  else {
    // Special tables are not added to the statement table list.
    if( (NOT cachedNATable->getExtendedQualName().isSpecialTable()) ||
        (cachedNATable->getExtendedQualName().getSpecialType() == 
           ExtendedQualName::MV_TABLE) || 
	(cachedNATable->getExtendedQualName().getSpecialType() == 
           ExtendedQualName::GHOST_MV_TABLE) ||
        (cachedNATable->getExtendedQualName().getSpecialType() ==
           ExtendedQualName::GHOST_INDEX_TABLE) ||
        (cachedNATable->getExtendedQualName().getSpecialType() ==
           ExtendedQualName::INDEX_TABLE)
      )
      statementTableList_.insert(cachedNATable);
  }

  //increment the replacement, if not already max
  if(cachedNATable)
    {
      cachedNATable->replacementCounter_+=2;

      //don't let replacementcounter go over NATABLE_MAX_REFCOUNT
      if(cachedNATable->replacementCounter_ > NATABLE_MAX_REFCOUNT)
        cachedNATable->replacementCounter_ = NATABLE_MAX_REFCOUNT;

      //Keep track of tables accessed during current statement
      if((!cachedNATable->accessedInCurrentStatement()))
        {
          cachedNATable->setAccessedInCurrentStatement();
          statementCachedTableList_.insert(cachedNATable);
        }
    }

  //return NATable from cache
  return cachedNATable;
}

void NATable::removeTableToClusterMapInfo()
{
  CMPASSERT(gpClusterInfo);
  const LIST(CollIndex) & tableIdList = getTableIdList();
  for(CollIndex i = 0; i < tableIdList.entries(); i++)
  {
    gpClusterInfo->removeFromTableToClusterMap(tableIdList[i]);
  }

} // NATable::removeTableToClusterMapInfo()

// by default column histograms are marked to not be fetched, 
// i.e. needHistogram_ is initialized to DONT_NEED_HIST.
// this method will mark columns for appropriate histograms depending on
// where they have been referenced in the query
void NATable::markColumnsForHistograms()
{
  // Check if Show Query Stats command is being run
  NABoolean runningShowQueryStatsCmd = CmpCommon::context()->showQueryStats();

  // we want to get 1 key column that is not SYSKEY
  NABoolean addSingleIntHist = FALSE;
  if(colArray_.getColumn("SYSKEY"))
    addSingleIntHist = TRUE;

  // iterate over all the columns in the table
  for(UInt32 i=0;i<colArray_.entries();i++)
  {
    // get a reference to the column
    NAColumn * column = colArray_[i];

    // is column part of a key
    NABoolean isAKeyColumn = (column->isIndexKey() OR column->isPrimaryKey()
                              OR column->isPartitioningKey());

    //check if this column requires histograms
    if(column->isReferencedForHistogram() ||
       (isAKeyColumn && isHbaseTable()))
      column->setNeedFullHistogram();
    else
    // if column is:
    // * a key
    // OR
    // * isReferenced but not for histogram and addSingleIntHist is true
    if (isAKeyColumn ||
       ((runningShowQueryStatsCmd || addSingleIntHist) && 
         column->isReferenced() && !column->isReferencedForHistogram()))
    {
      // if column is not a syskey
      if (addSingleIntHist && (column->getColName() != "SYSKEY"))
        addSingleIntHist = FALSE;
      
      column->setNeedCompressedHistogram();      
    }
    else
    if (column->getType()->getVarLenHdrSize() &&
        (CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT) != DF_OFF ||
         CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT_BMO) != DF_OFF ))
    {
      column->setNeedCompressedHistogram();
    }
  }
}

const QualifiedName& NATable::getFullyQualifiedGuardianName()
{
  //qualified name and fileSetName are different 
  //so we use fileSetName because it will contain
  //fully qualified guardian name
  QualifiedName * fileName;

  if(qualifiedName_.getQualifiedNameObj().getQualifiedNameAsString()
     != fileSetName_.getQualifiedNameAsString())
  {
    fileName = new(CmpCommon::statementHeap()) QualifiedName
      (fileSetName_,CmpCommon::statementHeap());
  }
  else
  {
    fileName = new(CmpCommon::statementHeap()) QualifiedName
      (qualifiedName_.getQualifiedNameObj(),CmpCommon::statementHeap());
  }
  return *fileName;
}

ExtendedQualName::SpecialTableType NATable::getTableType()
{
  return qualifiedName_.getSpecialType();
}

NABoolean NATable::hasSaltedColumn()
{
  for (CollIndex i=0; i<colArray_.entries(); i++ )
  {
    if ( colArray_[i]->isSaltColumn() ) 
      return TRUE;
  }
  return FALSE;
}

// Get the part of the row size that is computable with info we have available
// without accessing HBase. The result is passed to estimateHBaseRowCount(), which
// completes the row size calculation with HBase info.
//
// A row stored in HBase consists of the following fields for each column:
//          -----------------------------------------------------------------------
//          | Key  |Value | Row  | Row  |Column|Column|Column| Time | Key  |Value |
//  Field   |Length|Length| Key  | Key  |Family|Family|Qualif| stamp| Type |      |
//          |      |      |Length|      |Length|      |      |      |      |      |
//          -----------------------------------------------------------------------
//  # Bytes    4      4       2             1                    8      1
//
// The field lengths calculated here are for Row Key, Column Qualif, and Value.
// The size of the Value fields are not known to HBase, which treats cols as
// untyped, so we add up their lengths here, as well as the row key lengths,
// which are readily accessible via Traf metadata. The qualifiers, which represent
// the names of individual columns, are not the Trafodion column names, but
// minimal binary values that are mapped to the actual column names.
// The fixed size fields could also be added in here, but we defer that to the Java
// side so constants of the org.apache.hadoop.hbase.KeyValue class can be used.
// The single column family used by Trafodion is also a known entity, but we
// again do it in Java using the HBase client interface as insulation against
// possible future changes.
Int32 NATable::computeHBaseRowSizeFromMetaData() const
{
  Int32 partialRowSize = 0;
  Int32 rowKeySize = 0;
  const NAColumnArray& keyCols = clusteringIndex_->getIndexKeyColumns();
  CollIndex numKeyCols = keyCols.entries();

  // For each column of the table, add the length of its value and the length of
  // its name (HBase column qualifier). If a given column is part of the primary
  // key, add the length of its value again, because it is part of the HBase row
  // key.
  for (Int32 colInx=0; colInx<colcount_; colInx++)
    {
      // Get length of the column qualifier and its data.
      NAColumn* col = colArray_[colInx];;
      Lng32 colLen = col->getType()->getNominalSize(); // data length
      Lng32 colPos = col->getPosition();  // position in table

      partialRowSize += colLen;

      // The qualifier is not the actual column name, but a binary value
      // representing the ordinal position of the col in the table.
      // Single byte is used if possible.
      partialRowSize++;
      if (colPos > 255)
        partialRowSize++;

      // Add col length again if a primary key column, because it will be part
      // of the row key.
      NABoolean found = FALSE;
      for (CollIndex keyColInx=0; keyColInx<numKeyCols && !found; keyColInx++)
        {
          if (colPos == keyCols[keyColInx]->getPosition())
            {
              rowKeySize += colLen;
              found = TRUE;
            }
        }
    }

  partialRowSize += rowKeySize;
  return partialRowSize;
}

// For an HBase table, we can estimate the number of rows by dividing the number
// of KeyValues in all HFiles of the table by the number of columns (with a few
// other considerations).
Int64 NATable::estimateHBaseRowCount() const
{
  if (!isHbaseTable() || isSeabaseMDTable() ||
      getExtendedQualName().getQualifiedNameObj().getObjectName() == HBASE_HISTINT_NAME ||
      getExtendedQualName().getQualifiedNameObj().getObjectName() == HBASE_HIST_NAME)
    return ActiveSchemaDB()->getDefaults().getAsDouble(HIST_NO_STATS_ROWCOUNT);

  NADefaults* defs = &ActiveSchemaDB()->getDefaults();
  const char* server = defs->getValue(HBASE_SERVER);
  const char* zkPort = defs->getValue(HBASE_ZOOKEEPER_PORT);
  ExpHbaseInterface* ehi = ExpHbaseInterface::newInstance
                           (STMTHEAP, server, zkPort);

  Int64 estRowCount;
  Lng32 retcode = ehi->init(NULL);
  if (retcode < 0)
    {
      *CmpCommon::diags()
                << DgSqlCode(-8448)
                << DgString0((char*)"ExpHbaseInterface::init()")
                << DgString1(getHbaseErrStr(-retcode))
                << DgInt0(-retcode)
                << DgString2((char*)GetCliGlobals()->getJniErrorStr().data());
      delete ehi;
      estRowCount = 0;
    }
  else
    {
      HbaseStr fqTblName;
      NAString tblName = getTableName().getQualifiedNameAsString();
      fqTblName.len = tblName.length();
      fqTblName.val = new(STMTHEAP) char[fqTblName.len+1];
      strncpy(fqTblName.val, tblName.data(), fqTblName.len);
      fqTblName.val[fqTblName.len] = '\0';

      Int32 partialRowSize = computeHBaseRowSizeFromMetaData();
      retcode = ehi->estimateRowCount(fqTblName,
                                      partialRowSize,
                                      colcount_,
                                      estRowCount);
      NADELETEBASIC(fqTblName.val, STMTHEAP);

      // Return 0 as the row count if an error occurred while estimating it.
      // The estimate could also be 0 if there is less than 1MB of storage
      // dedicated to the table -- no HFiles, and < 1MB in MemStore, for which
      // size is reported only in megabytes.
      if (retcode < 0)
        estRowCount = 0;
      delete ehi;
    }

  return estRowCount;
}

// Method to get hbase table index levels and block size
NABoolean  NATable::getHbaseTableInfo(Int32& hbtIndexLevels, Int32& hbtBlockSize) const
{
  if (!isHbaseTable() || isSeabaseMDTable() ||
      getExtendedQualName().getQualifiedNameObj().getObjectName() == HBASE_HISTINT_NAME ||
      getExtendedQualName().getQualifiedNameObj().getObjectName() == HBASE_HIST_NAME ||
      getSpecialType() == ExtendedQualName::VIRTUAL_TABLE)
    return FALSE;

  NADefaults* defs = &ActiveSchemaDB()->getDefaults();
  const char* server = defs->getValue(HBASE_SERVER);
  const char* zkPort = defs->getValue(HBASE_ZOOKEEPER_PORT);
  ExpHbaseInterface* ehi = ExpHbaseInterface::newInstance
                           (STMTHEAP, server, zkPort);

  Lng32 retcode = ehi->init(NULL);
  if (retcode < 0)
  {
    *CmpCommon::diags()
              << DgSqlCode(-8448)
              << DgString0((char*)"ExpHbaseInterface::init()")
              << DgString1(getHbaseErrStr(-retcode))
              << DgInt0(-retcode)
              << DgString2((char*)GetCliGlobals()->getJniErrorStr().data());
    delete ehi;
    return FALSE;
  }
  else
  {
    HbaseStr fqTblName;
    NAString tblName = getTableName().getQualifiedNameAsString();
    fqTblName.len = tblName.length();
    fqTblName.val = new(STMTHEAP) char[fqTblName.len+1];
    strncpy(fqTblName.val, tblName.data(), fqTblName.len);
    fqTblName.val[fqTblName.len] = '\0';

    retcode = ehi->getHbaseTableInfo(fqTblName,
                                     hbtIndexLevels,
                                     hbtBlockSize);

    NADELETEBASIC(fqTblName.val, STMTHEAP);
    delete ehi;
    if (retcode < 0)
      return FALSE;
  }
  return TRUE;

}

// get details of this NATable cache entry
void NATableDB::getEntryDetails(
     Int32 ii,                      // (IN) : NATable cache iterator entry
     NATableEntryDetails &details)  // (OUT): cache entry's details
{
  Int32      NumEnt = cachedTableList_.entries();
  if  ( ( NumEnt == 0 ) || ( NumEnt <= ii ) )
  {
    memset(&details, 0, sizeof(details));
  }
  else {
    NATable * object = cachedTableList_[ii];
    QualifiedName QNO = object->qualifiedName_.getQualifiedNameObj();

    Int32 partLen = QNO.getCatalogName().length();
    strncpy(details.catalog, (char *)(QNO.getCatalogName().data()), partLen );
    details.catalog[partLen] = '\0';

    partLen = QNO.getSchemaName().length();
    strncpy(details.schema, (char *)(QNO.getSchemaName().data()), partLen );
    details.schema[partLen] = '\0';

    partLen = QNO.getObjectName().length();
    strncpy(details.object, (char *)(QNO.getObjectName().data()), partLen );
    details.object[partLen] = '\0';
  }
}


NABoolean NATableDB::isHiveTable(CorrName& corrName)
{
  return corrName.isHive();
}
    
NABoolean NATableDB::isSQUtiDisplayExplain(CorrName& corrName)
{
  const char* tblName = corrName.getQualifiedNameObj().getObjectName();
  if ( !strcmp(tblName, "EXE_UTIL_DISPLAY_EXPLAIN__"))
    return TRUE;

  if ( !strcmp(tblName, "EXPLAIN__"))
    return TRUE;

  if ( !strcmp(tblName, "HIVEMD__"))
    return TRUE;

  if ( !strcmp(tblName, "DESCRIBE__"))
    return TRUE;

  if ( !strcmp(tblName, "EXE_UTIL_EXPR__"))
    return TRUE;

  if ( !strcmp(tblName, "STATISTICS__"))
    return TRUE;

  return FALSE;
}


NABoolean NATableDB::isSQInternalStoredProcedure(CorrName& corrName)
{
  const char* tblName = corrName.getQualifiedNameObj().getObjectName();

  if ( !strncmp(tblName, "SPTableOutQUERYCACHEENTRIES",
                         strlen("SPTableOutQUERYCACHEENTRIES")))
    return TRUE;

  if ( !strncmp(tblName, "SPTableOutQUERYCACHEDELETE",
                         strlen("SPTableOutQUERYCACHEDELETE")))
    return TRUE;

  if ( !strncmp(tblName, "SPTableOutQUERYCACHE",
                         strlen("SPTableOutQUERYCACHE")))
    return TRUE;

  if ( !strncmp(tblName, "SPTableOutHYBRIDQUERYCACHEENTRIES",
                         strlen("SPTableOutHYBRIDQUERYCACHEENTRIES")))
    return TRUE;

  if ( !strncmp(tblName, "SPTableOutHYBRIDQUERYCACHE",
                         strlen("SPTableOutHYBRIDQUERYCACHE")))
    return TRUE;

  return FALSE;
}


NABoolean NATableDB::isSQUmdTable(CorrName& corrName)
{
  return FALSE;
}
  
NATable * NATableDB::get(CorrName& corrName, BindWA * bindWA,
                         desc_struct *inTableDescStruct){

  //check cache to see if a cached NATable object exists
  NATable *table = get(&corrName.getExtendedQualNameObj(), bindWA);  

  if (table && corrName.genRcb())
    {
      remove(table->getKey());
      table = NULL;
    }

  if (table && (corrName.isHbase() || corrName.isSeabase()) && inTableDescStruct)
    {
      remove(table->getKey());
      table = NULL;
    }

  if (0) //(table && (corrName.isHbase() || corrName.isSeabase()))
    {
      const NAString * val =
	ActiveControlDB()->getControlSessionValue("SHOWPLAN");
      if ( ( (val) && (*val == "ON") ) &&
	   (CmpCommon::getDefault(TRAF_RELOAD_NATABLE_CACHE) == DF_ON))
	{
	  remove(table->getKey());
	  table = NULL;
	}
    }

  if (table && (corrName.isHbaseCell() || corrName.isHbaseRow()))
    {
      if (NOT HbaseAccess::validateVirtualTableDesc(table))
        {
          remove(table->getKey());
          table = NULL;
        }
    }

  // for caching statistics
  if ((cacheMetaData_ && useCache_) && corrName.isCacheable())
  {
      //One lookup counted
      ++totalLookupsCount_;
      if (table) ++totalCacheHits_;  //Cache hit counted
  }

  NABoolean isMV = (table && table->isAnMV());

  if (NOT table ||
      (NOT isMV && table->getSpecialType() != corrName.getSpecialType())) {

    // in open source, only the SEABASE catalog is allowed.
    // Return an error if some other catalog is being used.
    if ((NOT corrName.isHbase()) &&
	(NOT corrName.isSeabase()) &&
	(NOT corrName.isHive()) &&
	(corrName.getSpecialType() != ExtendedQualName::VIRTUAL_TABLE))
      {
	*CmpCommon::diags()
	  << DgSqlCode(-1002)
	  << DgCatalogName(corrName.getQualifiedNameObj().getCatalogName())
	  << DgString0("");
	
	bindWA->setErrStatus();
	return NULL;
      }
    
    // If this is a 'special' table, generate a table descriptor for it.
    //
    if (NOT inTableDescStruct && corrName.isSpecialTable())
      inTableDescStruct = generateSpecialDesc(corrName);

    //Heap used by the NATable object
    NAMemory * naTableHeap = CmpCommon::statementHeap();

    //if NATable caching is on check if this table is not already
    //in the NATable cache. If it is in the cache create this NATable
    //on the statment heap, since the cache can only store one value per
    //key, therefore all duplicates (two or more different NATable objects
    //that have the same key) are deleted at the end of the statement.
    //ALSO
    //We don't cache any special tables across statements. Please check
    //the class ExtendedQualName for method isSpecialTable to see what
    //are special tables
    if (((NOT table) && cacheMetaData_ && useCache_) &&
        corrName.isCacheable()){
      naTableHeap = getHeap();
    }

    //if table is in cache tableInCache will be non-NULL
    //otherwise it is NULL.
    NATable * tableInCache = table;

    if ((corrName.isHbase() || corrName.isSeabase()) &&
	(!isSQUmdTable(corrName)) &&
	(!isSQUtiDisplayExplain(corrName)) &&
	(!isSQInternalStoredProcedure(corrName))
	) {
      CmpSeabaseDDL cmpSBD((NAHeap *)CmpCommon::statementHeap());


      desc_struct *tableDesc = NULL;

      NABoolean isSeabase = FALSE;
      NABoolean isSeabaseMD = FALSE;
      NABoolean isUserUpdatableSeabaseMD = FALSE;
      NABoolean isHbaseCell = corrName.isHbaseCell();
      NABoolean isHbaseRow = corrName.isHbaseRow();
      if (isHbaseCell || isHbaseRow)// explicit cell or row format specification
	{
	  if (cmpSBD.existsInHbase(corrName.getQualifiedNameObj().getObjectName()) != 1)
	    {
	      *CmpCommon::diags()
		<< DgSqlCode(-1389)
		<< DgString0(corrName.getQualifiedNameObj().getObjectName());
	      
	      bindWA->setErrStatus();
	      return NULL;
	    }

	  tableDesc = 
	    HbaseAccess::createVirtualTableDesc
	    (corrName.getExposedNameAsAnsiString(FALSE, TRUE).data(),
	     isHbaseRow, isHbaseCell);
	  isSeabase = FALSE;
	}
      else if (corrName.isSeabaseMD())
	{
	  if (corrName.isSpecialTable() && corrName.getSpecialType() == ExtendedQualName::INDEX_TABLE)
	    {
	      tableDesc = 
		cmpSBD.getSeabaseTableDesc(
					   corrName.getQualifiedNameObj().getCatalogName(),
					   corrName.getQualifiedNameObj().getSchemaName(),
					   corrName.getQualifiedNameObj().getObjectName(),
					   COM_INDEX_OBJECT);
	    }
	  else
	    {
	      tableDesc = 
		cmpSBD.getSeabaseTableDesc(
					   corrName.getQualifiedNameObj().getCatalogName(),
					   corrName.getQualifiedNameObj().getSchemaName(),
					   corrName.getQualifiedNameObj().getObjectName(),
					   COM_BASE_TABLE_OBJECT);
	      if (tableDesc)
		{
		  if (cmpSBD.isUserUpdatableSeabaseMD(
						      corrName.getQualifiedNameObj().getCatalogName(),
						      corrName.getQualifiedNameObj().getSchemaName(),
						      corrName.getQualifiedNameObj().getObjectName()))
		    isUserUpdatableSeabaseMD = TRUE;
		}
	    }

	  isSeabase = TRUE;
	  isSeabaseMD = TRUE;
	}
      else if (! inTableDescStruct)
        {
          ComObjectType objectType = COM_BASE_TABLE_OBJECT;
          isSeabase = TRUE;
          if (corrName.isSpecialTable())
          {
            switch (corrName.getSpecialType())
            {
              case ExtendedQualName::INDEX_TABLE:
              {
                objectType = COM_INDEX_OBJECT;
                break;
              }
              case ExtendedQualName::SG_TABLE:
              {
                objectType = COM_SEQUENCE_GENERATOR_OBJECT;
                isSeabase = FALSE;
                break;
              }
              case ExtendedQualName::LIBRARY_TABLE:
              {
                objectType = COM_LIBRARY_OBJECT;
                isSeabase = FALSE;
                break;
              }
              default: //TODO: No SpecialTableType for UDFs/Routines/COM_USER_DEFINED_ROUTINE_OBJECT
              {
                objectType = COM_BASE_TABLE_OBJECT;
              }
            }
          }
          tableDesc = cmpSBD.getSeabaseTableDesc(
                                corrName.getQualifiedNameObj().getCatalogName(),
                                corrName.getQualifiedNameObj().getSchemaName(),
                                corrName.getQualifiedNameObj().getObjectName(),
                                objectType);
        }

      if (inTableDescStruct)
         tableDesc = inTableDescStruct;

      if (tableDesc)
	table = new (naTableHeap)
	  NATable(bindWA, corrName, naTableHeap, tableDesc);
      if (!tableDesc || !table || bindWA->errStatus())
	{
	  if (isSeabase)
	    *CmpCommon::diags()
	      << DgSqlCode(-4082)
	      << DgTableName(corrName.getExposedNameAsAnsiString());
	  else
	    *CmpCommon::diags()
	      << DgSqlCode(-1389)
	      << DgString0(corrName.getExposedNameAsAnsiString());
	  
	  bindWA->setErrStatus();
	  return NULL;
	}

      table->setIsHbaseCellTable(isHbaseCell);
      table->setIsHbaseRowTable(isHbaseRow);
      table->setIsSeabaseTable(isSeabase);
      table->setIsSeabaseMDTable(isSeabaseMD);
      table->setIsUserUpdatableSeabaseMDTable(isUserUpdatableSeabaseMD);
    }
    else if (isHiveTable(corrName) &&
	(!isSQUmdTable(corrName)) &&
	(!isSQUtiDisplayExplain(corrName)) &&
	(!corrName.isSpecialTable()) &&
	(!isSQInternalStoredProcedure(corrName))
	) {
      if ( hiveMetaDB_ == NULL ) {
	if (CmpCommon::getDefault(HIVE_USE_FAKE_TABLE_DESC) != DF_ON)
	  {
	    hiveMetaDB_ = new (CmpCommon::contextHeap()) HiveMetaData();
	    
	    if ( !hiveMetaDB_->init() ) {
	      *CmpCommon::diags() << DgSqlCode(-1190)
                                  << DgString0(hiveMetaDB_->getErrMethodName())
                                  << DgString1(hiveMetaDB_->getErrCodeStr())
                                  << DgString2(hiveMetaDB_->getErrDetail())
                                  << DgInt0(hiveMetaDB_->getErrCode());
	      bindWA->setErrStatus();
	      
	      NADELETEBASIC(hiveMetaDB_, CmpCommon::contextHeap());
	      hiveMetaDB_ = NULL;
	      
	      return NULL;
	    }
	  }
	else
	  hiveMetaDB_ = new (CmpCommon::contextHeap()) 
            HiveMetaData(); // fake metadata
      }
      
      // this default schema name is what the Hive default schema is called in SeaHive
       NAString defSchema = ActiveSchemaDB()->getDefaults().getValue(HIVE_DEFAULT_SCHEMA);
       defSchema.toUpper();
       struct hive_tbl_desc* htbl;
       NAString tableNameInt = corrName.getQualifiedNameObj().getObjectName();
       NAString schemaNameInt = corrName.getQualifiedNameObj().getSchemaName();
       if (corrName.getQualifiedNameObj().getUnqualifiedSchemaNameAsAnsiString() == defSchema)
         schemaNameInt = hiveMetaDB_->getDefaultSchemaName();
       // Hive stores names in lower case
       // Right now, just downshift, could check for mixed case delimited
       // identifiers at a later point, or wait until Hive supports delimited identifiers
       schemaNameInt.toLower();
       tableNameInt.toLower();

       if (CmpCommon::getDefault(HIVE_USE_FAKE_TABLE_DESC) == DF_ON)
         htbl = hiveMetaDB_->getFakedTableDesc(tableNameInt);
       else
         htbl = hiveMetaDB_->getTableDesc(schemaNameInt, tableNameInt);

       if ( htbl )
	 {
	   table = new (naTableHeap) NATable(bindWA, corrName, naTableHeap, htbl);
	 }
       else 
         {
           if ((hiveMetaDB_->getErrCode() == 0)||
               (hiveMetaDB_->getErrCode() == 100))
             {
               *CmpCommon::diags()
                 << DgSqlCode(-1388)
                 << DgTableName(corrName.getExposedNameAsAnsiString());
             }
           else
             {
               *CmpCommon::diags()
                 << DgSqlCode(-1192)
                 << DgString0(hiveMetaDB_->getErrMethodName())
                 << DgString1(hiveMetaDB_->getErrCodeStr())
                 << DgString2(hiveMetaDB_->getErrDetail())
                 << DgInt0(hiveMetaDB_->getErrCode());

               hiveMetaDB_->resetErrorInfo();
             } 

          bindWA->setErrStatus();
          return NULL;
       }

    } else
       table = new (naTableHeap)
         NATable(bindWA, corrName, naTableHeap, inTableDescStruct);
    
    CMPASSERT(table);
    
    //if there was a problem in creating the NATable object
    if (NOT ((table->getExtendedQualName().isSpecialTable()) &&
	     (table->getExtendedQualName().getSpecialType() == 
	      ExtendedQualName::SG_TABLE)) &&
	(table->getColumnCount() == 0)) {
      
      bindWA->setErrStatus();

      //Delete the NATable object by deleting the naTableHeap
      //if all of the following are true
      //metadata caching is 'ON' (i.e. cacheMetaData_ == TRUE)
      //we are using the metadata cache in this statement (i.e. useCache_==TRUE)
      //this type of object is cacheable (i.e. corrName.isCacheable==TRUE)
      if((cacheMetaData_) &&
         (useCache_) &&
         (corrName.isCacheable())) {
        table->removeTableToClusterMapInfo();
        delete table;
      } else {
        delete table;
      }
      return NULL;
    }

    // Special tables are not added to the statement table list.
    // Index tables are added to the statement table list
    if(  (NOT table->getExtendedQualName().isSpecialTable()) ||
         (table->getExtendedQualName().getSpecialType() == 
             ExtendedQualName::INDEX_TABLE) ||
         (table->getExtendedQualName().getSpecialType() == 
             ExtendedQualName::MV_TABLE) ||
	 (table->getExtendedQualName().getSpecialType() == 
             ExtendedQualName::GHOST_MV_TABLE) ||
         (table->getExtendedQualName().getSpecialType() ==
             ExtendedQualName::GHOST_INDEX_TABLE)
      )
      statementTableList_.insert(table);

    //if there was no entry in cache associated with this key then
    //insert it into cache.
    //if there is already a value associated with this in the cache
    //then don't insert into cache.
    //This might happen e.g. if we call this method twice for the same table
    //in the same statement.
    if(!tableInCache){

       //insert into cache
	insert(table);

      //if we are using the cache
      //if this NATable object is cacheable
      if((useCache_) &&
         (corrName.isCacheable()))
      {
        //insert into list of all cached tables;
        cachedTableList_.insert(table);

        //insert into list of cached tables accessed
        //during this statement
        statementCachedTableList_.insert(table);

        //if metadata caching is ON then adjust the size of the cache
        //since we are adding an entry to the cache
        if(cacheMetaData_)
          currentCacheSize_ = heap_->getAllocSize();

	//update the high watermark for caching statistics
	if (currentCacheSize_ > highWatermarkCache_)        
	  highWatermarkCache_ = currentCacheSize_;                  
        //
        // the CompilerTrackingInfo highWaterMark gets reset on each
        // tracking interval so it is tracked independently
        if (currentCacheSize_ > intervalWaterMark_)          
          intervalWaterMark_ = currentCacheSize_;  

        //if we are caching metadata and previously the cache was
        //empty set this flag to TRUE to indicate that there is
        //something in the cache
        if(!metaDataCached_ && cacheMetaData_)
          metaDataCached_ = TRUE;

        //enforce the cache memory constraints
        if(!enforceMemorySpaceConstraints())
        {
          //was not able to get cache size below
          //max allowed cache size
          #ifndef NDEBUG
          CMPASSERT(FALSE);
          #endif
        }
      }
      else{
        //this has to be on the context heap since we need
        //it after the statement heap has been remove
        ExtendedQualName * nonCacheableTableName = new(CmpCommon::contextHeap())
                               ExtendedQualName(corrName.getExtendedQualNameObj(),
                                                CmpCommon::contextHeap());
        //insert into list of names of special tables
        nonCacheableTableList_.insert(nonCacheableTableName);

        // insert into list of non cacheable table idents.  This
        // allows the idents to be removed after the statement so
        // the context heap doesn't keep growing.
        const LIST(CollIndex) & tableIdList = table->getTableIdList();
        for(CollIndex i = 0; i < tableIdList.entries(); i++)
        {
          nonCacheableTableIdents_.insert(tableIdList[i]);
        }
      }
    }
  }

  //setup this NATable object for use in current statement
  //if this object has already been setup earlier in the
  //statement then this method will just return without doing
  //anything

  if(table) {
    table->setupForStatement();
  }

  return table;
}

void NATableDB::removeNATable(CorrName &corrName, QiScope qiScope,
                              ComObjectType ot)
{
  const ExtendedQualName* toRemove = &(corrName.getExtendedQualNameObj());
  NAHashDictionaryIterator<ExtendedQualName,NATable> iter(*this); 
  ExtendedQualName *key = NULL;
  NATable *cachedNATable = NULL;
  NASet<Int64> objectUIDs(CmpCommon::statementHeap(), 1);

  // iterate over all entries and remove the ones that match the name
  // ignoring any partition clauses and other additional info
  iter.getNext(key,cachedNATable);

  while(key)
    {
      if (key->getQualifiedNameObj() == toRemove->getQualifiedNameObj())
        {
          cachedNATable->removeTableToClusterMapInfo();

          //remove from list of cached NATables
          if (cachedTableList_.remove(cachedNATable) > 0)
            {
              // LCOV_EXCL_START - caching is off by default for now
              //if metadata caching is ON, then adjust cache size
              //since we are deleting a caching entry
              if(cacheMetaData_)
                currentCacheSize_ = heap_->getAllocSize();
              if (cachedNATable->heap_ &&
                  cachedNATable->heap_ != CmpCommon::statementHeap())
                tablesToDeleteAfterStatement_.insert(cachedNATable);
              // LCOV_EXCL_STOP
            }
          else
            {
              // this must have been a non-cacheable table
              const LIST(CollIndex) & tableIdList = cachedNATable->getTableIdList();
              for(CollIndex i = 0; i < tableIdList.entries(); i++)
                {
                  nonCacheableTableIdents_.remove(tableIdList[i]);
                }

              for (CollIndex i=0; i<nonCacheableTableList_.entries(); i++)
                {
                  if (*(nonCacheableTableList_[i]) == *key)
                    {
                      nonCacheableTableList_.removeAt(i);
                      i--;
                    }
                }
            }
  
          //remove pointer to NATable from cache
          remove(key);

          objectUIDs.insert(cachedNATable->objectUid().castToInt64());
          statementCachedTableList_.remove(cachedNATable);
          statementTableList_.remove(cachedNATable);
        }
      
      iter.getNext(key,cachedNATable);
    }

    // clear out the other users' caches too.
    if (qiScope == REMOVE_FROM_ALL_USERS)
    {
      // There are some scenarios where the affected object
      // does not have an NATable cache entry. Need to get one and 
      // add its objectUID to the set.
      if (0 == objectUIDs.entries())
      {
        Int64 ouid = lookupObjectUid(
                       toRemove->getQualifiedNameObj(), 
                       ot); 
        if (ouid > 0)
          objectUIDs.insert(ouid);
      }
      Int32 numKeys = objectUIDs.entries();
      if (numKeys > 0)
      {                 
        SQL_QIKEY qiKeys[numKeys];
        for (CollIndex i = 0; i < numKeys; i++)
        {
          qiKeys[i].ddlObjectUID = objectUIDs[i];
          qiKeys[i].operation[0] = 'O';
          qiKeys[i].operation[1] = 'R';
        }
        long retcode = SQL_EXEC_SetSecInvalidKeys(numKeys, qiKeys);
      }
    }
}

//This method is called at the end of each statement to reset statement
//specific stuff in the NATable objects in the cache.
void NATableDB::resetAfterStatement(){

  //Variable used for iteration in loops below
  CollIndex i = 0;

  //Variable used to point to a table's heap. Only delete the heap if it is
  // neither the context nor the statement heap (i.e., allocated from the
  // C++ system heap). The CmpContext heap is deleted in 
  // in ContextCli::deleteMe().
  // The statement heap is deleted in the destructor of class CmpStatement. 

  NAMemory * tableHeap = NULL;

  //if metadata caching (i.e. NATable caching) is not on then just
  //flush the cache. Since it might be that there are still some
  //tables in the cache.
  if (!cacheMetaData_){
    flushCache();
  }
  else{

    //if caching is ON then reset all cached NATables used during statement
    //if this was a DDL statment delete all NATables that participated in the
    //statement
    for (i=0; i < statementCachedTableList_.entries(); i++)
    {
      if(statementCachedTableList_[i])
      {
        //if the statment was a DDL statement, if so then delete
        //all the tables used in the statement, since the DDL affected
        //the tables and they should be reconstructed for whatever
        //statement follows.
        if((!useCache_)||
           (statementCachedTableList_[i]->isAnMV())||
           (statementCachedTableList_[i]->isAnMVMetaData())||
           (statementCachedTableList_[i]->isAnMPTableWithAnsiName())||
           (statementCachedTableList_[i]->constructionHadWarnings()) ||
           (statementCachedTableList_[i]->getClearHDFSStatsAfterStmt())){
          //remove from list of cached Tables
          cachedTableList_.remove(statementCachedTableList_[i]);
          //remove from the cache itself
          remove(statementCachedTableList_[i]->getKey());
          statementCachedTableList_[i]->removeTableToClusterMapInfo();

          if ( statementCachedTableList_[i]->getHeapType() == NATable::OTHER ) {
            delete statementCachedTableList_[i];
            currentCacheSize_ = heap_->getAllocSize();
          }
        }
        else{
          statementCachedTableList_[i]->resetAfterStatement();
        }
      }
    }

    // Remove the nonCacheable tables from the table to cluster map.
    for (i=0; i < nonCacheableTableIdents_.entries(); i++) {
      gpClusterInfo->removeFromTableToClusterMap(nonCacheableTableIdents_[i]);
    }
    nonCacheableTableIdents_.clear();

    //remove references to nonCacheable tables from cache
    //and delete the name
    for(i=0; i < nonCacheableTableList_.entries(); i++){
      remove(nonCacheableTableList_[i]);
      delete nonCacheableTableList_[i]; // delete the name only
    }

    //clear the list of special tables
    nonCacheableTableList_.clear();

  }

  //delete tables that were not deleted earlier to
  //save compile-time performance. Since the heaps
  //deleted below are large 16KB+, it takes time
  //to delete them. The time to delete these heaps
  //at this point is not 'visible' in the compile-
  //time since the statement has been compiled and
  //sent to the executor.
  for(i=0; i < tablesToDeleteAfterStatement_.entries(); i++)
  {
    if ( tablesToDeleteAfterStatement_[i]->getHeapType() == NATable::OTHER ) {
      delete tablesToDeleteAfterStatement_[i];
    }
    currentCacheSize_ = heap_->getAllocSize();
  }

  //clear the list of tables to delete after statement
  tablesToDeleteAfterStatement_.clear();

  //clear the list of tables used in the current statement
  statementTableList_.clear();
  //clear the list of cached tables used in the current statement
  statementCachedTableList_.clear();
  //reset various statement level flags
  refreshCacheInThisStatement_=FALSE;
  useCache_=FALSE;

}

//flush the cache if there is anything cached in it
//otherwise just destroy all the keys in the cache.
//If there is nothing cached, which could mean either
//of the following:
//1. NATable caching is off.
//2. All entries currently in cache where created on
//   the statment heap, i.e. not persistent across
//   statements.
//In such a case we don't need to delete any NATable
//objects (since they will be removed when the statement
//heap is deleted. We only need to delete the keys.
void NATableDB::flushCache()
{

  //if something is cached
  if(metaDataCached_){
    //set the flag to indicate cache is clear
    metaDataCached_ = FALSE;

    //Destroy the keys in the cache, this also
    //clears out the cache entries without deleting
    //the cached NATable
    clearAndDestroyKeysOnly();

    //delete the tables that were cached by deleting each table's
    //heap. Each cached table and all of its stuff are allocated
    //on a seperate heap (i.e. a heap per table). That seems to
    //be the safest thing to do to avoid memory leaks.
    for(CollIndex i=0; i < cachedTableList_.entries(); i++)
    {
      if(cachedTableList_[i])
      {
        cachedTableList_[i]->removeTableToClusterMapInfo();
        delete cachedTableList_[i];
      }
    }

  }
  else{
    //no metadata cached (i.e. metadata caching is off and there
    //is no remaining metadata in the cache from when the caching
    //was on). Just clear out the cache entries, of course we need
    //to delete keys because the cache allocates keys on the context
    //heap.
    clearAndDestroyKeysOnly ();
  }

  //clear out the lists of tables in the cache
  //1. list of tables in the cache used in this statement
  //2. list of all tables in the cache
  statementCachedTableList_.clear();
  cachedTableList_.clear();

  //set cache size to 0 to indicate nothing in cache
  currentCacheSize_ = 0;
  highWatermarkCache_ = 0;   // High watermark of currentCacheSize_  
  totalLookupsCount_ = 0;    // reset NATable entries lookup counter
  totalCacheHits_ = 0;       // reset cache hit counter

  // per interval counters
  intervalWaterMark_ = 0;
}

//check if cache size is with maximum allowed cache size.
//if cache size is above the maximum allowed cache size,
//then remove entries in the cache based on the cache
//replacement policy to get the cache size under the maximum
//allowed cache size.
NABoolean NATableDB::enforceMemorySpaceConstraints()
{
  //check if cache size is within memory constraints
  if (maxCacheSize_ == 0 || heap_->getAllocSize()  <= maxCacheSize_)
    return TRUE;

  //need to get cache size under memory allowance

  //if our cursor is pointing past the end of the
  //list of cached entries, reset it to point to
  //start of the list of cached entries.
  if(replacementCursor_ >= (Int32) cachedTableList_.entries())
      replacementCursor_ = 0;

  //keep track of entry in the list of cached entries
  //where we are starting from, since we start from
  //where we left off the last time this method got
  //called.
  Int32 startingCursorPosition = replacementCursor_;
  Int32 numLoops = 0; //number of loops around the list of cached objects

  //this loop iterates over list of cached NATable objects.
  //in each iteration it decrements the replacementCounter
  //of a table.
  //if a table with a replacementCounter value of zero is
  //encountered, it is removed if it is not being used
  //in the current statement.

  //check if cache is now within memory constraints
  while (heap_->getAllocSize()  > maxCacheSize_){

    //get reference to table
    NATable * table = cachedTableList_[replacementCursor_];
    if(table)
      //check if table has a zero replacementCount
      if(!table->replacementCounter_)
      {
        //if table is not being accessed in current statement then remove it
        if(!table->accessedInCurrentStatement_)
        {
           RemoveFromNATableCache( table , replacementCursor_ );
        }
      }
      else{
        table->replacementCounter_--;
      }

    replacementCursor_++;

    //if replacement cursor ran of the end of the list of cached tables
    //reset it to the beginig of the list
    if(replacementCursor_ >= (Int32) cachedTableList_.entries())
      replacementCursor_ = 0;

    //check if we completed one loop around all the cached entries
    //if so, increment the loop count
    if(replacementCursor_ == startingCursorPosition){
        numLoops++;
    }

    //did NATABLE_MAX_REFCOUNT loops around list of cached objects
    //still could not free up enough space
    //We check for NATABLE_MAX_REFCOUNT loops since the replacementCounter_
    //is capped at NATABLE_MAX_REFCOUNT loops.
    if(numLoops==NATABLE_MAX_REFCOUNT)
      return FALSE;
  }

  //return true indicating cache size is below maximum memory allowance.
  return TRUE;
}

//Remove all the NATable objects from the cache that were used during
//the current statement.
//This is used when a binder error occurs. In rare cases the binder
//error might be due to a stale metadata cache entry.
// LCOV_EXCL_START :cnu
void NATableDB::flushCacheEntriesUsedInCurrentStatement(){

  //do this only if metadata caching is 'ON'
  if(cacheMetaData_)
  {
    for (CollIndex i=0; i < statementCachedTableList_.entries(); i++)
    {
      if(statementCachedTableList_[i])
      {
        //remove from list of cached Tables
        cachedTableList_.remove(statementCachedTableList_[i]);
        //remove from the cache itself
        remove(statementCachedTableList_[i]->getKey());
        //keep track of change in cache size
        delete statementCachedTableList_[i];
        currentCacheSize_ = heap_->getAllocSize();
      }
    }

    //clear the list of tables used in the current statement
    statementCachedTableList_.clear();
  }
}
// LCOV_EXCL_STOP

//Turn metadata caching ON
void NATableDB::setCachingON()
{
  resizeCache(getDefaultAsLong(METADATA_CACHE_SIZE)*1024*1024);
  cacheMetaData_ = TRUE;
}

// Obtain a list of table identifiers for the current statement.
// Allocate the list on the heap passed.
const LIST(CollIndex) &
NATableDB::getStmtTableIdList(NAMemory *heap) const
{
  LIST(CollIndex) *list = new (heap) LIST(CollIndex)(heap);
  for(CollIndex i = 0; i < statementTableList_.entries(); i++)
  {
    NATable *table = statementTableList_[i];
    list->insert(table->getTableIdList());
  }
  return *list;
}

// function to return number of entries in cachedTableList_ LIST.
Int32 NATableDB::end()
{
   return cachedTableList_.entries() ;
}

void
NATableDB::free_entries_with_QI_key(Int32 numKeys, SQL_QIKEY* qiKeyArray)
{
  UInt32 currIndx = 0;

  // For each table in cache, see if it should be removed
  while ( currIndx < cachedTableList_.entries() )
  {
    NATable * currTable = cachedTableList_[currIndx];

    // Only need to remove seabase tables
    if (!currTable->isSeabaseTable())
    {
      currIndx++;
      continue;
    }

    if (qiCheckForInvalidObject(numKeys, qiKeyArray,
                                currTable->objectUid().get_value(),
                                currTable->getSecKeySet()))
    {
      if ( currTable->accessedInCurrentStatement_ )
        statementCachedTableList_.remove( currTable );
      while ( statementTableList_.remove( currTable ) ) // Remove as many times as on list!
      { ; }

      RemoveFromNATableCache( currTable , currIndx );
    }
    else currIndx++; //Increment if NOT found ... else currIndx already pointing at next entry!
  }
}

//
// Remove a specifed NATable entry from the NATable Cache
//
void
NATableDB::RemoveFromNATableCache( NATable * NATablep , UInt32 currIndx )
{
   NAMemory * tableHeap = NATablep->heap_;
   NABoolean InStatementHeap = (tableHeap == (NAMemory *)CmpCommon::statementHeap());

   remove(NATablep->getKey());
   NATablep->removeTableToClusterMapInfo();
   cachedTableList_.removeAt( currIndx );
   if ( ! InStatementHeap )
      delete NATablep;
   if ( ! InStatementHeap )
      currentCacheSize_ = heap_->getAllocSize();
}

//
// Remove ALL entries from the NATable Cache that have been
// marked for removal before the next compilation.
//
void
NATableDB::remove_entries_marked_for_removal()
{
   NATableDB * TableDB = ActiveSchemaDB()->getNATableDB() ;

   UInt32 currIndx = 0;
   while ( currIndx < TableDB->cachedTableList_.entries() )
   {
      NATable * NATablep = TableDB->cachedTableList_[ currIndx ] ;
      NABoolean accInCurrStmt = NATablep->accessedInCurrentStatement() ;
      if ( NATablep->isToBeRemovedFromCacheBNC() ) //to be removed by CmpMain Before Next Comp. retry?
      {
         TableDB->RemoveFromNATableCache( NATablep, currIndx );
         if ( accInCurrStmt )
         {
            TableDB->statementCachedTableList_.remove( NATablep );
         }
         while ( TableDB->statementTableList_.remove( NATablep ) ) // Remove as many times as on list!
         { ; }
      }
      else currIndx++ ; //Note: No increment if the entry was removed !
   }
}

//
// UNMARK all entries from the NATable Cache that have been
// marked for removal before the next compilation.  We have 
// decided to leave them in the NATable cache afterall.
//
void
NATableDB::unmark_entries_marked_for_removal()
{
   NATableDB * TableDB = ActiveSchemaDB()->getNATableDB() ;

   UInt32 currIndx = 0;
   while ( currIndx < TableDB->cachedTableList_.entries() )
   {
      NATable * NATablep = TableDB->cachedTableList_[ currIndx ] ;
      if ( NATablep->isToBeRemovedFromCacheBNC() ) //to be removed by CmpMain Before Next Comp. retry?
      {
         NATablep->setRemoveFromCacheBNC(FALSE);
      }
      else currIndx++ ; //Note: No increment if the entry was removed !
   }
}

void NATableDB::getCacheStats(NATableCacheStats & stats)
{
  stats.numLookups = totalLookupsCount_;
  stats.numCacheHits = totalCacheHits_;
  stats.currentCacheSize = currentCacheSize_;
  stats.highWaterMark = highWatermarkCache_;
  stats.maxCacheSize = maxCacheSize_;
  stats.numEntries =  cachedTableList_.entries();    
}

