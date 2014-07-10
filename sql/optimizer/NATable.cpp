/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2014 Hewlett-Packard Development Company, L.P.
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
  NABoolean isSQLMPTable = table.isSQLMPTable();
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
          GetHSModifyTime(qualifiedName, type, modifTime, isSQLMPTable);
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
            FetchStatsTime(qualifiedName,type,colArray,statsTime,isSQLMPTable);
            cachedHistograms->updateRefreshTime();
            // If ustat automation is on, FetchStatsTime will modify the HISTOGRAMS table.
            // So, the new modification time of the HISTOGRAMS table must be saved to the
            // cached histograms when automation is on, so that only changes to HISTOGRAMS 
            // by update stats cause the above 'if' to be TRUE.
            if (CmpCommon::getDefaultLong(USTAT_AUTOMATION_INTERVAL) > 0)
            {
              GetHSModifyTime(qualifiedName, type, modifTime, isSQLMPTable);
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
  NABoolean isSQLMPTable = table.isSQLMPTable();
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
                    isSQLMPTable,
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
        (list, *column, singleColsFound);
    }
	
    // if column was added, then add it to the duplist
    if (colAdded) singleColsFound += colPos;
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
      ComUID id(mcHist->id());
      CostScalar uec(mcHist->uec());
      CostScalar rows(mcHist->rows());
      ColStatsSharedPtr mcStat = new (STMTHEAP) ColStats 
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

  // Replace "FRACTION" with "SECOND  " for SQL/MP objects
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
  const char* encodedKeyP = NULL;
  char* varCharstr = NULL;


  for (CollIndex c = 0; c < partColArray.entries(); c++)
    {
      const NAType *pkType = partColArray[c]->getType();
      Lng32 decodedValueLen = pkType->getNominalSize();
      ItemExpr *keyColVal = NULL;

      if (pkType->isEncodingNeeded())
        {
          encodedKeyP = &encodedKey[keyColOffset];

          // for varchar the decoding logic expects the length to be in the first
          // pkType->getVarLenHdrSize() chars, so add it 
          if (pkType->getTypeName() == "VARCHAR")
          {
              varCharstr = new (heap) char[decodedValueLen + pkType->getVarLenHdrSize()];
              str_cpy_all(varCharstr, (char*) &decodedValueLen, pkType->getVarLenHdrSize());
              str_cpy_all(varCharstr+pkType->getVarLenHdrSize(), encodedKeyP, decodedValueLen);
              decodedValueLen += pkType->getVarLenHdrSize();
              encodedKeyP = varCharstr;
          }

          // un-encode the key value by using an expression
          ConstValue *keyColEncVal =
            new (heap) ConstValue(pkType,
                                  (void *) encodedKeyP,
                                  decodedValueLen,
                                  new(heap) NAString("'<region boundary>'"),
                                  heap);
          CMPASSERT(keyColEncVal);

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

            char staticDecodeInAsciiBuf[200];
            Lng32 staticDecodeInAsciiBufLen = 200;

            Lng32 decodeInAsciiBufLen = staticDecodeInAsciiBufLen;
            char* decodeInAsciiBuf = staticDecodeInAsciiBuf;

            // Allocate a new buffer if the static one is not big enough.
            if ( DFS2REC::isAnyVarChar(pkType->getFSDatatype()) ) {

               // get the exact length for VARCHAR type
               Lng32 vc_len;

               if ( pkType->getVarLenHdrSize() == 2 ) {
                  UInt16 vcLen16;
                  str_cpy_all((char *)&vcLen16, &decodeBuf[resultOffset], 
                               pkType->getVarLenHdrSize());
                  vc_len = vcLen16;
               } else {
                  str_cpy_all((char *)&vc_len, &decodeBuf[resultOffset], 
                               pkType->getVarLenHdrSize());
               }

               // the actual result length is the VC length + the header length
               resultLength = vc_len + pkType->getVarLenHdrSize();

               // Multiplying by 8 to deal with conversion bewteen any two 
               // known character sets supported. 
               if ( decodeInAsciiBufLen < vc_len*8 ) {
                  decodeInAsciiBufLen = vc_len*8 + pkType->getVarLenHdrSize();  
                  decodeInAsciiBuf = new (STMTHEAP) char[decodeInAsciiBufLen];
               }
            } else 
             if ( staticDecodeInAsciiBufLen < resultLength * factor ) {
               decodeInAsciiBufLen = resultLength * factor;
               decodeInAsciiBuf = new (STMTHEAP) char[decodeInAsciiBufLen];
             }


            Lng32 len;

            rc = convDoIt(&decodeBuf[resultOffset], resultLength, 
                 pkType->getFSDatatype(), 
                 pkType->getPrecision(), 
                 pkType->getScaleOrCharset(), 
                 decodeInAsciiBuf, decodeInAsciiBufLen, REC_BYTE_V_ASCII, 0, 0, 
                 (char*)&len, sizeof(len), heap, NULL,
                 conv_case_index::CONV_UNKNOWN, 
                 0, // data conversionErrorFlag 
                 0 // flags
                );

           if ( rc == ex_expr::EXPR_OK ) {

             keyColVal =
                new (heap) ConstValue(pkType,
                                    (void *) &(decodeBuf[resultOffset]),
                                    resultLength,
                                    new(heap) NAString(decodeInAsciiBuf, len),
                                    heap);

           }
          }

          if ( rc != ex_expr::EXPR_OK ) 
            return NULL;

        } // encoded 
      else
        {
          // simply use the provided value as the binary value of a constant
          keyColVal =
            new (heap) ConstValue(pkType,
                                  (void *) &encodedKey[keyColOffset],
                                  decodedValueLen,
                                  new(heap) NAString("'<region boundary>'"),
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
  CMPASSERT(keyColOffset == encodedKeyLen);

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
                                      Int32 SQLMPKeytag,
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

      // If we are setting up partition boundaries for a SQL/MP index,
      // then the first two bytes of the encoded key will be the keytag.
      // We need to skip over the keytag.
      if (SQLMPKeytag != 0)
        encodedKey = &encodedKey[2];

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
                                 unsigned short SQLMPKeytag,
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
                                   SQLMPKeytag,
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
          totalEncodedKeyLength,
          FALSE /* not an SQLMP table  */);

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
                                 0, /// non MP table
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
                                 0,
                                 nodeMap,
                                 heap);
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
    {
      // First figure out # partns
      partns = 0;
      desc_struct* hrk = desc;
      while ( hrk ) {
	partns++;
	hrk = hrk->header.next;
      }
    }
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
  desc_struct* partns_desc = part_desc_list;
  char mySystem[9];
  char *currPart;
  short lengthCurrPart = 0;
  mySystem[8] = '\0';

  OSIM_GETSYSTEMNAME(OSIM_MYSYSTEMNUMBER(), (short *)mySystem);

  short lengthMySys = 0;
  while (lengthMySys < 8)
  {
    if (mySystem[lengthMySys] == ' ')
       break;
    lengthMySys++;
  }

  if(NOT partns_desc)
  {
    currPart = tableName;
    while (lengthCurrPart < 8)
    {
       if (currPart[lengthCurrPart] == '.')
          break;
       lengthCurrPart++;
    }
    if ((lengthCurrPart != lengthMySys) ||
        (str_cmp(currPart, mySystem, lengthMySys) != 0))
       return TRUE;
  }
  else
  {
    while (partns_desc)
    {
      // ------------------------------------------------------------------
      // Loop over all partitions
      // ------------------------------------------------------------------
      currPart = partns_desc->body.partns_desc.partitionname;
      lengthCurrPart = 0;
      while (lengthCurrPart < 8)
      {
         if (currPart[lengthCurrPart] == '.')
            break;
         lengthCurrPart++;
      }
      if ((lengthCurrPart != lengthMySys) ||
          (str_cmp(currPart, mySystem, lengthMySys) != 0))
         return TRUE;
      partns_desc = partns_desc->header.next;
    }
  }
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
         NABoolean isDivisioningColumn = FALSE;

         if (column_desc->defaultClass == COM_ALWAYS_COMPUTE_COMPUTED_COLUMN_DEFAULT)
           if (computed_column_text ||
               defaultValue == NULL ||
               str_len(defaultValue) == 0)
             // divisioning column, expression stored in TEXT metadata table
             isDivisioningColumn = TRUE;
           else
             {
               // HBase salt column, expression is stored in default value
               // computed column text is in UTF-8, default is in UCS-2, convert
               NAWString ccValUCS2((NAWchar*) defaultValue);
               Int32 ccValLengthWChars = ccValUCS2.length();
               NAString *ccValUTF8 = unicodeToChar(ccValUCS2,
                                                   ccValLengthWChars,
                                                   CharInfo::UTF8,
                                                   STMTHEAP);
               computed_column_text = (char *) ccValUTF8->data();
               defaultValue = NULL;
             }

         if(ActiveSchemaDB()->getNATableDB()->cachingMetaData()){
           //make copies of stuff onto the heap passed in
           if(defaultValue){
             Int32 defaultValueLength = NAWstrlen((NAWchar*)defaultValue)+1;
             defaultValue = (char*) new (heap) NAWchar [defaultValueLength];
             NAWstrcpy((NAWchar*)defaultValue,(NAWchar*)column_desc->defaultvalue);
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
			       0 /* not a SQLMP KEYTAG column */,
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
                               isDivisioningColumn);
	}
      else
        {
	  // Kludged KEYTAG column from srtdsrv.tal/RTD_Convert_Index_To_Table()
	  // fetching INDEX metadata in BASETABLE format...
	  // An empty COLNAME signals a KEYTAG column of what's actually an
	  // MP INDEX -- the INDEX's KEYTAG VALUE is passed to us in the COLNUM,
	  // which we save, then set to what we know is the real ordinal of
	  // the KEYTAG column, i.e. zero.
	  //
	  // Some sanity checks to be on the safe side...
	  //
          if(table !=NULL)
	    {
	      
	      CMPASSERT(table->isSQLMPTable());
	      CMPASSERT(table->getSpecialType() == ExtendedQualName::INDEX_TABLE);
	      CMPASSERT(colClass == SYSTEM_COLUMN);
	      CMPASSERT(column_desc->offset == 0);
	      unsigned short SQLMPKeytag = column_desc->colnumber;
	      CMPASSERT(SQLMPKeytag);
	      NAWchar* keytagStr = new (heap) NAWchar[20];
	      NAWsprintf(keytagStr, WIDE_("%u"), (UInt32)SQLMPKeytag);
	      
	      newColumn = new (heap)
		NAColumn(FUNNY_INTERNAL_IDENT("KEYTAG"),
			 0 /* colnumber */,
			 type,
			 heap,
			 SQLMPKeytag,
			 table,
			 colClass,
			 COM_USER_DEFINED_DEFAULT,
			 (char*)keytagStr
			 );
	    }
	} // else
      
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
         return TRUE;
      }

      NAString colName(hcolumn->name_);
      colName.toUpper();

      NAColumn* newColumn = new (heap)
                    NAColumn(colName.data(),
                             hcolumn->intIndex_,
                             natype,
                             heap,
                             (unsigned short)0 /* not a SQLMP KEYTAG column */,
                             table,
                             USER_COLUMN, // colClass,
                             COM_NULL_DEFAULT  ,//defaultClass,
                             (char*)L"", // defaultValue,
                             (char*)L"", // heading,
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
  // get index name
  NAString *str=NULL;
  Int32 *index=NULL;
  NAFileSet *nfs=NULL;
  NAFileSet *nfsRemote= NULL;
  NAFileSet *nfsLocal=NULL;

  NAFileSetList localIndexes(CmpCommon::statementHeap()),
                remoteIndexes(CmpCommon::statementHeap());

  // gather remote and local indexes
  UInt32 i,j;
  for (i=0; i<Iter.entries();i++)
    {
    Iter.getNext(str,index);
    nfs=(NAFileSet *)index;

    NAString  extFileName(nfs->getExtFileSetName().data(),
                          CmpCommon::statementHeap());
    char *str=(char *)extFileName.data();
    char *pos=strchr(str, '.');
    *pos='\0';

    if (strcmp(str, localNodeName) == 0)
      {
       // all local indexes go into the indexes list to be considered
       // by the optimizer
       indexes.insert(nfs);

       localIndexes.insert(nfs);
      }
    else
       remoteIndexes.insert(nfs);
    }

  for (i=0; i<remoteIndexes.entries();i++)
    {
    nfsRemote=remoteIndexes[i];

    for (j=0; j<localIndexes.entries();j++)
      {
      nfsLocal=localIndexes[j];
      NABoolean partitionedSameWay = TRUE;

      // check if both are partitioned....
      if (nfsLocal->isPartitioned() != nfsRemote->isPartitioned() ||
          (! (nfsLocal->getPartitioningKeyColumns() ==
             nfsRemote->getPartitioningKeyColumns())))
         partitionedSameWay=FALSE;

      if (nfsLocal->getAllColumns() == nfsRemote->getAllColumns()&&
          partitionedSameWay)
        {

        // nfsRemote goes away?
        nfsRemote->setRemoteIndexGone();
        break;
        }
      }
      indexes.insert(nfsRemote);
    }

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
  NAType *SQLMPKeytagType = NULL;

  // Set up global cluster information.  This global information always
  // gets put on the context heap.
  //
  // Note that this function call will probably not do anything, since
  // this cluster information is set up when arkcmp is created; however,
  // it's certainly better to have this call here, rather than in a
  // doubly-nested loop below where it used to be ...

  // $$$ probably not necessary to call this even once ...
  setUpClusterInfo(CmpCommon::contextHeap());

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

      // ---------------------------------------------------------------------
      // loop over the clustering key columns of the index
      // ---------------------------------------------------------------------
      unsigned short SQLMPKeytag = 0;
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
          if (tablecolnumber < 0) {	// this is < 0 only for MP indexes!
	    CMPASSERT(table->isSQLMPTable());
	    // Assert this keytag is the first column in the index
	    CMPASSERT(keys_desc->body.keys_desc.keyseqnumber == 0);
	    // Assert this is an alternate index (the primary index = the table)
      	    CMPASSERT(indexes_desc->body.indexes_desc.keytag);
            // Assert this is not a VP
            CMPASSERT(!isVerticalPartition);

	    if (!SQLMPKeytagType)
	      SQLMPKeytagType = new (heap)
		SQLSmall(FALSE/*allowNegVal*/, FALSE/*allowSQLnull*/);

	    CMPASSERT(!SQLMPKeytag);
	    SQLMPKeytag = indexes_desc->body.indexes_desc.keytag;
	    NAWchar* keytagStr = new (heap) NAWchar[20];
	    NAWsprintf(keytagStr, WIDE_("%u"), (UInt32)SQLMPKeytag);

	    // ? Instead of passing table, pass NULL since this is not a column
	    //   on the NATable but rather on an index(NAFileSet) on the table ?
	    indexColumn = new (heap)
	      NAColumn(FUNNY_INTERNAL_IDENT("KEYTAG"),
		       keys_desc->body.keys_desc.tablecolnumber,
		       SQLMPKeytagType,
                       heap,
		       SQLMPKeytag,
		       table,
		       SYSTEM_COLUMN,
		       COM_USER_DEFINED_DEFAULT,
		       (char*)keytagStr
		       );
	  }
	  else
          {
	    indexColumn = colArray.getColumn(tablecolnumber);

	    if ((table->isHbaseTable()) &&
		(indexes_desc->body.indexes_desc.keytag != 0))
	      {
		newIndexColumn = new(heap) NAColumn(*indexColumn);
		newIndexColumn->setIndexColName(keys_desc->body.keys_desc.keyname);
		newIndexColumn->setHbaseColFam(keys_desc->body.keys_desc.hbaseColFam);
		newIndexColumn->setHbaseColQual(keys_desc->body.keys_desc.hbaseColQual);

                saveNAColumns.insert(indexColumn);
		indexColumn = newIndexColumn;
	      }

            // Check if this is a SQLMP keytag column for index maintenance --
	    // i.e., the index treated as if it were a basetable.
            if (indexColumn->getSQLMPKeytag())
            {
	      CMPASSERT(!SQLMPKeytag);
              SQLMPKeytag = indexColumn->getSQLMPKeytag();
            }
          }

          SortOrdering order = NOT_ORDERED;

          // Only add a key column to the index key if it is not the keytag.
          if (NOT indexColumn->getSQLMPKeytag())
          {
	    // as mentioned above, for all alternate indexes we
	    // assume at first that all columns are key columns
	    // and we make adjustments later
	    indexKeyColumns.insert(indexColumn);
	    order = keys_desc->body.keys_desc.ordering ?
                                 DESCENDING : ASCENDING;
	    indexKeyColumns.setAscending(indexKeyColumns.entries() - 1,
	  			   order == ASCENDING);
          }

	  if (isTheClusteringKey)
          {
            // Since many columns of the base table may not be in the
            // clustering key, we'll delay setting up the list of all
            // columns in the index until later, so we can then just
            // add them all at once.

            // Remember that this columns is part of the clustering
            // key and remember its key ordering (asc or desc), but
            // NOT if it is the keytag column.
            if (NOT indexColumn->getSQLMPKeytag())
            {
	      indexColumn->setClusteringKey(order);
	      numClusteringKeyColumns++;
            }
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
      else
        partitioningKeyColumns = indexKeyColumns;

      // Create DP2 node map for partitioning function.
      NodeMap* nodeMap = new (heap) NodeMap(heap);

      //increment for each table/index to create unique identifier
      cmpCurrentContext->incrementTableIdent();
      

      // NB: Just in case, we made a call to setupClusterInfo at the
      // beginning of this function.
      desc_struct * partns_desc;
      Int32 indexLevels = 1;
      if (files_desc)
      {
	if( (table->getSpecialType() != ExtendedQualName::VIRTUAL_TABLE AND
/*
	     table->getSpecialType() != ExtendedQualName::ISP_TABLE AND
*/
	     (NOT table->isHbaseTable()))
	    OR files_desc->body.files_desc.partns_desc )
	  {
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

              if ( hbd && hbd->header.nodetype == DESC_HBASE_HASH2_REGION_TYPE )
	           partFunc = createHash2PartitioningFunctionForHBase(
	    	      ((table_desc_struct*)table_desc)->hbase_regionkey_desc,
		      heap);
              else
                if (hbd && hbd->header.nodetype == DESC_HBASE_RANGE_REGION_TYPE)
	           partFunc = createRangePartitioningFunctionForHBase(
	    	      ((table_desc_struct*)table_desc)->hbase_regionkey_desc,
	    	      partitioningKeyColumns,
		      heap);
              else {

	        // Range partitioned or single partition table
	        partFunc = createRangePartitioningFunction(
	    	   files_desc->body.files_desc.partns_desc,
	    	   partitioningKeyColumns,
	    	   SQLMPKeytag,
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
		  indexes_desc->body.indexes_desc.blocksize,
		  indexLevels,
		  allColumns,
		  indexKeyColumns,
		  partitioningKeyColumns,
		  partFunc,
		  indexes_desc->body.indexes_desc.keytag,
		  SQLMPKeytag,
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
        if ( NOT table->isSQLMPTable())
          indexes.insert(newIndex);
        else
          {
           char *pos = (char *) strrchr(extIndexName.data(), '.');

           if (pos == NULL)
             pos = (char *)extIndexName.data();
           else
             pos++;

           NAString *indexName = new (CmpCommon::statementHeap())
                      NAString((const char*)pos, CmpCommon::statementHeap());
           stringList.insert(indexName);

           indexFilesetMap->insert(indexName, (Int32 *)newIndex);

          }
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
  NAType *SQLMPKeytagType = NULL;

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
      unsigned short SQLMPKeytag = 0;
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
		  SQLMPKeytag,

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
    securityLabel_(NULL),
    securityLabelLen_(0),
    schemaLabelFileName_(NULL),
    constraintLabelInfo_(NULL),
    constraintLabelInfoLen_(0),
    parentTableName_(NULL),
    sgAttributes_(NULL),
    isHive_(FALSE),
    isHbase_(FALSE),
    isHbaseCell_(FALSE),
    isHbaseRow_(FALSE),
    isSeabase_(FALSE),
    isSeabaseMD_(FALSE),
    isUserUpdatableSeabaseMD_(FALSE),
    resetHDFSStatsAfterStmt_(FALSE),
    hiveDefaultStringLen_(0),
    hiveTableId_(-1)
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
    }

  // Check if it is a SQL/MP table, and set the flag to indicate this.
  if (table_desc->body.table_desc.underlyingFileType == SQLMP)
  {
     setSQLMPTable(TRUE);
     //need to store guardian name for future use. Ex. fetchHistograms need fully qualified guardian name
     if((fileSetName_.getQualifiedNameAsString().data()[0])!='\\')
      {
      	ComMPLoc mpName(table_desc->body.table_desc.tablename,ComMPLoc::FULLFILE);
      	QualifiedName qualName(mpName.getFileName(),mpName.getSubvolName(),mpName.getSysDotVol(),heap_);
      	fileSetName_=qualName;
      }
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

  if (! isSQLMPTable() )
  {
    switch(table_desc->body.table_desc.rowFormat)
    {
    case COM_PACKED_FORMAT_TYPE:
      setSQLMXTable(TRUE);
      break;
    case COM_ALIGNED_FORMAT_TYPE:
      setSQLMXAlignedTable(TRUE);
      break;
    }
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

  rcb_ = table_desc->body.table_desc.rcb;
  rcbLen_ = table_desc->body.table_desc.rcbLen;
  keyLength_ = table_desc->body.table_desc.keyLen;

  securityLabel_ = (void *)table_desc->body.table_desc.securityLabel;
  securityLabelLen_ = table_desc->body.table_desc.securityLabelLen;

  constraintLabelInfo_ = table_desc->body.table_desc.constraintInfo;
  constraintLabelInfoLen_ = table_desc->body.table_desc.constraintInfoLen;


  if (table_desc->body.table_desc.schemalabelfilename)
  {
    schemaLabelFileName_ = new(heap_) char[strlen(table_desc->body.table_desc.schemalabelfilename) + 1];
    strcpy(schemaLabelFileName_,table_desc->body.table_desc.schemalabelfilename);
    schemaRedefTime_ = table_desc->body.table_desc.schemaRedefTime;
    
    short error = 0;
    NAString vol, subvol, table;
    char localNodeName[MAX_NODE_NAME];
    if((CmpCommon::getDefault(VALIDATE_SCHEMA_REDEF_TS) == DF_ON) &&
       (*(schemaLabelFileName_) == '$'))
    {
      short nodeNameLength;

      if( OSIM_NODENUMBER_TO_NODENAME_
          (-1,                // local node
           localNodeName,      // return node name here
           8,                  // max length
           &nodeNameLength     // actual length
           )
          )
      {
        error = 1;
      }
      localNodeName[nodeNameLength] = 0;
      NAString nameStr(schemaLabelFileName_);
      size_t x, y;

      if ((x = nameStr.first('.')) == NA_NPOS)
        error =1 ;
      else
      {
        vol = (nameStr (0, x));
        y = x + 1;
        if ((x = nameStr.index('.', y)) == NA_NPOS)
          error = 1;
        else
        {
          subvol = (nameStr (y, x-y));
          y = x + 1;
          table = (nameStr (y, nameStr.length() - y));
        }
      }
    }
    else
      error = 1;

    if (!error)
    {
    }
  }

  if (table_desc->body.table_desc.parentTableName)
    {
      parentTableName_ =
	new(heap_) char[strlen(table_desc->body.table_desc.parentTableName) + 1];
      strcpy(parentTableName_, table_desc->body.table_desc.parentTableName);
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
    return;

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

      if (isSQLMPTable() && !isUpdatable())
       {
         setSQLMPShorthandView(TRUE);
       }

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
			   maxIndexLevelsPtr)) {
        return;
      }

      if ((!bindWA->inDDL()) || bindWA->isBindingMvRefresh())
        {
	  //
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
						 sg_desc->objectUID);
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
        error = OSIM_NODENAME_TO_NODENUMBER_ (nodeName, nodeNameLen, &primaryNodeNum);
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

      error = OSIM_NODENAME_TO_NODENUMBER_ (nodeName, nodeNameLen, &primaryNodeNum);
    }
  }

  Lng32 postCreateNATableWarnings = CmpCommon::diags()->getNumber(DgSqlCode::WARNING_);

  if(postCreateNATableWarnings != preCreateNATableWarnings)
    tableConstructionHadWarnings_=TRUE;

  if(isSQLMPTable())
  {
// LCOV_EXCL_START :mp
    //if the catalog name starts with "\" and is not delimited
    //then table name is guardian name and not ansi name
    //guardian names are in the format \<system>.$<volume>.<subvol>
    //if an ansi catalog name start with "\" then it is delimited.

    //the code below checks if this MP table has an ansi name or not
    const QualifiedName & qualNameObj = qualifiedName_.getQualifiedNameObj();
    const CatalogName & catNameObj = qualNameObj.getCatalogName();

    if(catNameObj.isDelimited())
      isAnMPTableWithAnsiName_=TRUE;

    const NAString & catName = catNameObj.getCatalogName();

    if((catName.length() > 1) &&
       (catName(0) != '\\'))
      isAnMPTableWithAnsiName_=TRUE;

  }

  if (hasLobColumn())
    {
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
    securityLabel_(NULL),
    securityLabelLen_(0),
    schemaLabelFileName_(NULL),
    constraintLabelInfo_(NULL),
    constraintLabelInfoLen_(0),
    parentTableName_(NULL),
    sgAttributes_(NULL),
    isHive_(TRUE),
    isHbase_(FALSE),
    isHbaseCell_(FALSE),
    isHbaseRow_(FALSE),
    isSeabase_(FALSE),
    isSeabaseMD_(FALSE),
    isUserUpdatableSeabaseMD_(FALSE),
    resetHDFSStatsAfterStmt_(FALSE),
    hiveDefaultStringLen_(0),
    hiveTableId_(htbl->tblID_)
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

// to check
  securityLabel_ = 0;
  securityLabelLen_ = 0;

  constraintLabelInfo_ = 0;
  constraintLabelInfoLen_ = 0;

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
            CURRSTMT_OPTDEFAULTS->setHistDefaultSampleSize(0);;


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

      if (CmpCommon::getDefault(HBASE_RANGE_PARTITIONING_MC_SPLIT) == DF_ON)
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
	((isUniqueIndex || isPrimaryKey) ? nacArr.entries() :
	 (nacArr.entries() - numBTpkeys));
      if ((inputCols.entries() > 0) && (inputCols.entries() != numKeyCols))
	continue;

      NASet<NAString> keyColNAS(HEAP, inputCols.entries());
      NASet<NAString> indexNAS(HEAP, inputCols.entries());
      for (Int32 j = 0; j < numKeyCols; j++)
	{
	  const NAString &colName = inputCols[j];
	  keyColNAS.insert(colName);
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

NATable::~NATable()
{
  // remove the map entries of associated table identifers in
  // NAClusterInfo::tableToClusterMap_.
  CMPASSERT(gpClusterInfo);
  const LIST(CollIndex) & tableIdList = getTableIdList();
  for(CollIndex i = 0; i < tableIdList.entries(); i++)
  {
    gpClusterInfo->removeFromTableToClusterMap(tableIdList[i]);
  }
  // implicitly destructs all subcomponents allocated out of this' private heap
  // hence no need to write a complicated destructor!
}

//some stuff of historical importance

// Currently we explicitly destruct NATables after compiling each stmt
// (arkcmp calls SchemaDB::cleanupPerStatement -- cf. BindWA::getNATable).
//
// However, should we ever decide to save NATables across statement boundaries,
// to cache and reuse them for performance, the following method will be needed
// to reinit each NATable before it is reused (arkcmp would call this reset()
// for each NATable in the SchemaDB).
// (First we must determine if reuse is even possible by comparing redef
// timestamp with current metadata via some call to Catman SOL -- if they don't
// have some redef-event notification mechanism then they may have to
// go to disk anyway, obviating any real performance boost to begin with.)
//
// There is another situation where reuse is possible:  compound statements.
// The compiler must hold one same transaction open over the course of
// sequentially compiling each of the stmts in the compound -- so no redef
// could take place meanwhile.
//
//    void NATable::reset()	// ## to be implemented?
//    {
//      It is not clear to me whether virtual tables and resource forks
//      (any "special" table type) can/should be reused.  Maybe certain
//      types can; I just have no idea right now.  But as we're not reading
//	metadata tables for them anyway, there seems little savings in
//	caching them; perhaps we should just continue to build them on the fly.
//
//      All the real metadata in NATable members can stay as it is.
//	But there are a few pieces of for-this-query-only data:
//	  referenceCount_ = 0;
//      And we now optimize/filter/reduce histogram statistics for each
//      individual query, so stats and adminicular structures must be reset:
//    	  statsFetched_ = FALSE;
//    	  delete/clearAndDestroy colStats_
//    	  for (i in colArray_) colArray_[i]->setReferenced(FALSE);
//
//	Actually, we'd probably want to call FetchHistograms in our ctor,
//	passing a flag telling it to return the *full* set of stats
//	(don't do all that nice filtering which is now saving us
//	time/space from unnecessary new's and SQL fetches);
//	our ctor would save these into a new fullColStats_ member here,
//	which does *not* get reset.  The method getStatistics() above
//	would traverse the fullColStats list and apply the same rules
//	FH currently follows to decide which full stats elements to
//	deep copy (no pointer sharing) into the per-query colStats_ list.
//
//	Actually, we'd want a fullStatsFetched_ flag -- i.e do not call FH
//	in the ctor -- because if the inDDL flag is passed in TRUE, we don't
//	want any stats at all.
//
//	We could do things such that for single-stmt compiles we continue
//	to reap the performance benefit of doing the pruning within FH;
//	but for a compound stmt we use the approach outlined just above.
//	Obviously we'd need some sort of inCompoundCompile() flag.
//    }

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

#ifndef NDEBUG
  //memory leak, assert in debug
  if ( ! isHiveTable() ) 
    {
     CMPASSERT(sizeAfterLastStatement_ <= initialSize_ );
    }
  else
    {
      Int32 hiveStatsSize = 0;
      if (getClusteringIndex() && getClusteringIndex()->getHHDFSTableStats())
        hiveStatsSize = getClusteringIndex()->getHHDFSTableStats()->getHiveStatsUsedHeap();

      CMPASSERT(sizeAfterLastStatement_ <= (initialSize_ + hiveStatsSize) );
    }

#endif

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

  if (isSQLMPTable() || getViewText())
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

  //Found in cache.  If that's all the caller wanted, return now.
  if ( findInCacheOnly )
     return cachedNATable;

  //This flag determines if a cached object should be deleted and
  //reconstructed
  NABoolean removeEntry = FALSE;

  //if this is the first time this cache entry has been accessed
  //during the current statement
  if(!cachedNATable->accessedInCurrentStatement())
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
    else if ((!cachedNATable->isHiveTable()) &&
	     (!cachedNATable->isHbaseTable()))
    {
      if (CmpCommon::getDefault(MODE_SPECIAL_1) == DF_ON)
      {
        Int32 errorValue = -1;
        Int64 cacheTime = cachedNATable->getCacheTime();

        // Read the OBJECTS table get the cache_time.  If changed
        // refresh NATable cache.

        // get catalog name, must be in external format
        QualifiedName objName = cachedNATable->getTableName();
        const NAString cName = objName.getCatalogNameAsAnsiString();

        // Generate the fully qualified name for the OBJECTS table
        NAString oTbl = (cName);
        oTbl.append(".HP_DEFINITION_SCHEMA");
        oTbl.append(".OBJECTS");

        NAWString * pObjectsMDTblExtNameInUCS2 = 
          charToUnicode ( (Lng32) CharInfo::UTF8
                        , (const char *) oTbl.data()
                        , (Int32) oTbl.length()
                        , STMTHEAP // in - NAMemory * h = NULL
                        );
        NAWString oTblExtNameInUCS2(*pObjectsMDTblExtNameInUCS2); // deep copy
        delete pObjectsMDTblExtNameInUCS2;
        pObjectsMDTblExtNameInUCS2 = NULL;

        // set up remaining input variables:
        //   in: UID of schema
        //   in: object name, must be in internal format
        //   in: object type - assume TA
        Int64 sUID = ((ComUID &)cachedNATable->getSchemaUid()).get_value();
        NAString oName = objName.getObjectName(); // in UTF8 for SeaQuest
        NAString nameSpace ("TA ");

        // Execute metadata query that does a keyed access on the OBJECTS
        // table for READ UNCOMMITTED ACCESS to retrieve the current
        // cacheTime.  See w:/sqlcat/readdef.mdf statement S4.
        static THREAD_P struct SQLCLI_OBJ_ID __SQL_id4;
        init_SQLCLI_OBJ_ID(&__SQL_id4, SQLCLI_CURRENT_VERSION, cursor_name,
            &__SQL_mod_natable, "S4", 0,
            SQLCHARSETSTRING_ISO88591, 2);

        // The internal-format object name in oName must be in UTF8.
        // because we TRANSLATE it from UTF8TOUCS2 in statement S4
        // in w:/sqlcat/readdef.mdf.
        errorValue = SQL_EXEC_ClearExecFetchClose(&__SQL_id4, // statement id
                                               NULL, // input descriptor
                                               NULL, // output descriptor
                                               4, // num inputs
                                               1, // num outputs
                                               5, // total
                                               oTblExtNameInUCS2.data(),
                                               NULL,
                                               &sUID,NULL,
                                               oName.data(),NULL,
                                               nameSpace.data(),NULL,
                                               &cacheTime,NULL);

        // If the record was found and the cache times differ,
        // refresh the entry
        if ((errorValue == 0) && (cacheTime != cachedNATable->getCacheTime()))
        {
          removeEntry = TRUE;
          cachedNATable->setCacheTime(cacheTime);
        }
      } // mode_special_1, check objects table
    } // label timestamps match
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
      currentCacheSize_ -= cachedNATable->getSize();

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

// get details of this NATable cache entry
void NATableDB::getEntryDetails(
     Int32 ii,                      // (IN) : NATable cache iterator entry
     NATableEntryDetails &details)  // (OUT): cache entry's details
{
  NATableDB * Table = ActiveSchemaDB()->getNATableDB() ;
  Int32      NumEnt = Table->cachedTableList_.entries();
  if  ( ( NumEnt == 0 ) || ( NumEnt <= ii ) )
  {
    memset(&details, 0, sizeof(details));
  }
  else {
    NATable * object = Table->cachedTableList_[ii];
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

  return FALSE;
}


NABoolean NATableDB::isSQUmdTable(CorrName& corrName)
{
  static const char* sqUmdTableNames[] = {
    "HISTOGRAMS", 
    "HISTOGRAM_INTERVALS", 
    "HISTOGRAMS_FREQ_VALS",
    "MVS_UMD",
    "MVS_TABLE_INFO_UMD",
    "MVS_USED_UMD"
  };

  const char* tblName = corrName.getQualifiedNameObj().getObjectName();

  if (tblName[0] == 'H' || tblName[0] == 'M') {
     for (Int32 i=0; i<sizeof(sqUmdTableNames)/sizeof(const char*); i++ ) {
        if ( strcmp(tblName, sqUmdTableNames[i]) == 0 )
          return TRUE;
     }
  }

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

      //if caching NATable objects create them on a persistent heap
      //create a new NATable heap for used by the NATable Object
      size_t memLimit = (size_t) 1024 * CmpCommon::getDefaultLong(MEMORY_LIMIT_NATABLECACHE_UPPER_KB);
      const Lng32 initHeapSize = 16 * 1024;    // ## 16K
      naTableHeap = new
                    CTXTHEAP NAHeap("NATable Heap", (NAHeap *)CTXTHEAP, initHeapSize, memLimit);
      naTableHeap->setJmpBuf(CmpInternalErrorJmpBufPtr);
    }

    //if table is in cache tableInCache will be non-NULL
    //otherwise it is NULL.
    NATable * tableInCache = table;

    if ((corrName.isHbase() || corrName.isSeabase()) &&
	(!isSQUmdTable(corrName)) &&
	(!isSQUtiDisplayExplain(corrName)) &&
	(!isSQInternalStoredProcedure(corrName))
	) {
      CmpSeabaseDDL cmpSBD((NAHeap *)heap_);

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
					   "IX");
	    }
	  else
	    {
	      tableDesc = 
		cmpSBD.getSeabaseTableDesc(
					   corrName.getQualifiedNameObj().getCatalogName(),
					   corrName.getQualifiedNameObj().getSchemaName(),
					   corrName.getQualifiedNameObj().getObjectName(),
					   "BT");
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
      else if (corrName.isSpecialTable() && corrName.getSpecialType() == ExtendedQualName::INDEX_TABLE)
	{
	  tableDesc = 
	    cmpSBD.getSeabaseTableDesc(
				       corrName.getQualifiedNameObj().getCatalogName(),
				       corrName.getQualifiedNameObj().getSchemaName(),
				       corrName.getQualifiedNameObj().getObjectName(),
				       "IX");
	  isSeabase = TRUE;
	}
      else 
	{
	  tableDesc = 
	    cmpSBD.getSeabaseTableDesc(
				       corrName.getQualifiedNameObj().getCatalogName(),
				       corrName.getQualifiedNameObj().getSchemaName(),
				       corrName.getQualifiedNameObj().getObjectName(),
				       "BT");
	  isSeabase = TRUE;
	}

      if (inTableDescStruct)
	tableDesc = inTableDescStruct;

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
    if (table->getColumnCount() == 0) {

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
        delete naTableHeap;
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
          currentCacheSize_ += table->getSize();

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

void NATableDB::removeNATable(CorrName &corrName, NABoolean matchNameOnly)
{
  const ExtendedQualName* toRemove = &(corrName.getExtendedQualNameObj());
  NAHashDictionaryIterator<ExtendedQualName,NATable> iter(*this); 
  ExtendedQualName *key = NULL;
  NATable *cachedNATable = NULL;

  if (matchNameOnly)
    {
      // iterate over all entries and remove the ones that match the name
      // ignoring any partition clauses and other additional info
      iter.getNext(key,cachedNATable);
    }
  else
    {
      // delete only a perfect match
      key = const_cast<ExtendedQualName *>(toRemove);
      cachedNATable = NAKeyLookup<ExtendedQualName,NATable>::get(key);
    }

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
                currentCacheSize_ -= cachedNATable->getSize();

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

          statementCachedTableList_.remove(cachedNATable);
          statementTableList_.remove(cachedNATable);
        }
      
      // advance the iterator (if matching names only), or end the loop
      if (matchNameOnly)
        iter.getNext(key,cachedNATable);
      else
        key = NULL;
    }
}

//This method is called at the end of each statement to reset statement
//specific stuff in the NATable objects in the cache.
void NATableDB::resetAfterStatement(){

  //Variable used for iteration in loops below
  CollIndex i = 0;
  //Variable used to point to a table's heap
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
           (statementCachedTableList_[i]->getClearHDFSStatsAfterStmt()) ||
	   (statementCachedTableList_[i]->isSQLMPTable()
	      AND statementCachedTableList_[i]->getViewText())){
          //remove from list of cached Tables
          cachedTableList_.remove(statementCachedTableList_[i]);
          //remove from the cache itself
          remove(statementCachedTableList_[i]->getKey());
          //keep track of change in cache size
          currentCacheSize_-= statementCachedTableList_[i]->getSize();
          //delete the NATable by deleting the heap it is on
          //the heap has everything that belongs to the NATable
          //so this should hopefully avoid memory leaks
          statementCachedTableList_[i]->removeTableToClusterMapInfo();
          tableHeap = statementCachedTableList_[i]->heap_;
          delete tableHeap;
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
      delete nonCacheableTableList_[i];
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
    tableHeap = tablesToDeleteAfterStatement_[i]->heap_;
    delete tableHeap;
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
        NAMemory * tableHeap = cachedTableList_[i]->heap_;
        delete tableHeap;
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
  if (currentCacheSize_ <= maxCacheSize_)
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
  while (currentCacheSize_ > maxCacheSize_){

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
        currentCacheSize_-= statementCachedTableList_[i]->getSize();
        //delete the NATable by deleting the heap it is on
        //the heap has everything that belongs to the NATable
        //so this should hopefully avoid memory leaks
        NAMemory * tableHeap = statementCachedTableList_[i]->heap_;
        delete tableHeap;
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

// Static member function to return number of entries in cachedTableList_ LIST.
Int32 NATableDB::end()
{
   return ActiveSchemaDB()->getNATableDB()->cachedTableList_.entries() ;
}

void
NATableDB::free_entries_with_QI_key( Int32 NumSiKeys, SQL_SIKEY * pSiKeyEntry )
{
}

//
// Remove a specifed NATable entry from the NATable Cache
//
void
NATableDB::RemoveFromNATableCache( NATable * NATablep , UInt32 currIndx )
{
   NAMemory * tableHeap = NATablep->heap_;
   NABoolean InStatementHeap = (tableHeap == (NAMemory *)CmpCommon::statementHeap());

   Lng32 removedSize = NATablep->getSize();
   if ( ! InStatementHeap )
      currentCacheSize_-=removedSize;
   remove(NATablep->getKey());
   NATablep->removeTableToClusterMapInfo();
   cachedTableList_.removeAt( currIndx );

   if ( ! InStatementHeap )
      delete tableHeap;
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

//-----------------------------------------------------------------------
// NATableCacheStoredProcedure is a class that contains functions used by
// the NATableCache virtual table, whose purpose is to serve as an interface
// to the SQL/MX NATable cache statistics. This table is implemented as
// an internal stored procedure.
//-----------------------------------------------------------------------
const Lng32 NUM_OF_OUTPUT = 5;

SP_STATUS NATableCacheStatStoredProcedure::sp_NumOutputFields(
  Lng32 *numFields,
  SP_COMPILE_HANDLE spCompileObj,
  SP_HANDLE spObj,
  SP_ERROR_STRUCT *error)
{
  *numFields = NUM_OF_OUTPUT;
  return SP_SUCCESS;
}

SP_STATUS NATableCacheStatStoredProcedure::sp_OutputFormat(
  SP_FIELDDESC_STRUCT *format,
  SP_KEYDESC_STRUCT keyFields[],
  Lng32 *numKeyFields,
  SP_HANDLE spCompileObj,
  SP_HANDLE spObj,
  SP_ERROR_STRUCT *error)
{
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_lookups      INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_cache_hits   INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "Current_cache_size   INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "High_watermark   INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "Max_cache_size   INT UNSIGNED");

  return SP_SUCCESS;
}

SP_STATUS NATableCacheStatStoredProcedure::sp_Process(
  SP_PROCESS_ACTION action,
  SP_ROW_DATA inputData,
  SP_EXTRACT_FUNCPTR eFunc,
  SP_ROW_DATA outputData,
  SP_FORMAT_FUNCPTR fFunc,
  SP_KEY_VALUE keys,
  SP_KEYVALUE_FUNCPTR kFunc,
  SP_PROCESS_HANDLE *spProcHandle,
  SP_HANDLE spObj,
  SP_ERROR_STRUCT *error)
{
  struct InfoStruct
  {
    ULng32 counter;
  };

  SP_STATUS status = SP_SUCCESS;
  InfoStruct *is = NULL;

  NATableDB * tableDB = ActiveSchemaDB()->getNATableDB();

  switch (action)
  {
  case SP_PROC_OPEN:
    is = new InfoStruct;
    is->counter = 0;
    *spProcHandle = is;
    break;

  case SP_PROC_FETCH:
    is = (InfoStruct*)(*spProcHandle);
    if (is == NULL )
    {
      status = SP_FAIL;
      break;
    }

    if (is->counter > 0) break;
    is->counter++;
    fFunc(0, outputData, sizeof(ULng32), &(tableDB->totalLookupsCount_), 0);
    fFunc(1, outputData, sizeof(ULng32), &(tableDB->totalCacheHits_), 0);
    fFunc(2, outputData, sizeof(ULng32), &(tableDB->currentCacheSize_), 0);
    fFunc(3, outputData, sizeof(ULng32), &(tableDB->highWatermarkCache_), 0);
    fFunc(4, outputData, sizeof(ULng32), &(tableDB->maxCacheSize_), 0);
    status = SP_MOREDATA;
    break;

  case SP_PROC_CLOSE:
    delete (InfoStruct*) (*spProcHandle);
    break;
  }
  return status;
}

void NATableCacheStatStoredProcedure::Initialize(SP_REGISTER_FUNCPTR regFunc)
{
  regFunc("NATABLECACHE",
          sp_Compile,
          sp_InputFormat,
          0,
          sp_NumOutputFields,
          sp_OutputFormat,
          sp_Process,
          0,
	  CMPISPVERSION);
}

//-----------------------------------------------------------------------
// NATableCacheDeleteStoredProcedure is a class that contains functions used
// to delete the contents of the  NATableCache virtual table. The delete 
// function is implemented as an internal stored procedure.
//-----------------------------------------------------------------------


SP_STATUS NATableCacheDeleteStoredProcedure::sp_Process(
  SP_PROCESS_ACTION action,
  SP_ROW_DATA inputData,
  SP_EXTRACT_FUNCPTR eFunc,
  SP_ROW_DATA outputData,
  SP_FORMAT_FUNCPTR fFunc,
  SP_KEY_VALUE keys,
  SP_KEYVALUE_FUNCPTR kFunc,
  SP_PROCESS_HANDLE *spProcHandle,
  SP_HANDLE spObj,
  SP_ERROR_STRUCT *error)
{
  struct InfoStruct
  {
    ULng32 counter;
  };

  SP_STATUS status = SP_SUCCESS;
  InfoStruct *is = NULL;
  NATableDB * tableDB = NULL;

  switch (action)
  {
  case SP_PROC_OPEN:
    // No inputs to process
    is = new InfoStruct;
    is->counter = 0;
    *spProcHandle = is;
    break;

  case SP_PROC_FETCH:
    is = (InfoStruct*)(*spProcHandle);
    tableDB = ActiveSchemaDB()->getNATableDB();
    if (is == NULL )
    {
      status = SP_FAIL;
      break;
    }

    //clear out NATableCache
    tableDB->setCachingOFF();
    tableDB->setCachingON();
    break;

  case SP_PROC_CLOSE:
    delete (InfoStruct*) (*spProcHandle);
    break;
  }
  return status;
}

void NATableCacheDeleteStoredProcedure::Initialize(SP_REGISTER_FUNCPTR regFunc)
{
  regFunc("NATABLECACHEDELETE",
          sp_Compile,
          sp_InputFormat,
          0,
          sp_NumOutputFields,
          sp_OutputFormat,
          sp_Process,
          0,
	  CMPISPVERSION);
}
