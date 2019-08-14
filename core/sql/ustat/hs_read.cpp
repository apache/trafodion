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
 * File:         hs_read.C
 * Description:  Functions to read histogram tables.
 * Created:      09/25/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


// See general strategy notes preceding FetchHistogram implementation below.

#define   SQLPARSERGLOBALS_FLAGS   // must precede all #include's
#include "SqlParserGlobalsCmn.h"

#define HS_FILE "hs_read"

#include "Platform.h"

#include <iostream>
#include <fstream>

#include <stdlib.h>
  #include <sys/io.h>      // To pick up "access" function.

#include "cli_stdh.h"
#include "str.h"

#include "BaseTypes.h"
#include "CmpCommon.h"
#include "NAType.h"
#include "ObjectNames.h"
#include "RelPackedRows.h"
#include "Stats.h"
#include "CmpSqlSession.h"

#include "ExSqlComp.h"          // for NAExecTrans

#include "hs_globals.h"
#include "hs_const.h"
#include "hs_cli.h"
#include "hs_la.h"
#include "hs_auto.h"
#include "sql_id.h"
#include "Cost.h"      /* for lookups in the defaults table */
#include "hs_read.h"
#include "hs_update.h"
#include "ComMPLoc.h"

#include "NLSConversion.h"
#include "CompException.h"
#include "ComCextdecs.h"
#include "OptimizerSimulator.h"
#include "CmpStatement.h"
#include "CmpDescribe.h"


// Global pointer to OptimizerSimulator
#define CMPNEW new (heap_)
  // for getBootstrapCompile method

// These should be made members of "control/monitor/state" class HSColStats
static Lng32 TotalHistintsDBG = 0;
static Lng32 TotalHistogramDBG = 0;

// -----------------------------------------------------------------------------
//                                 CLASSES
// -----------------------------------------------------------------------------
class HSColStats;
class HSHistogrmCursor;
class HSHistintsCursor;
class HSStatsTimeCursor;

// -----------------------------------------------------------------------------
//                                  FUNCTIONS
// -----------------------------------------------------------------------------
static Lng32 OpenCursor( const char *descID
                      , void *  param1Addr
                      , void *  param2Addr
                      , SQLDESC_ID *&desc
                      );
Lng32 updateHistogram(const char *histogramTableName, Int32 stmtNum, short readCount);
Lng32 setPersistentSampleTable(Int64 &sampleRows, Int64 &actualRows, NABoolean isEstimate,
                              float allowedDiff, NABoolean createSampleIfNecessary, 
                              HSTableDef *tabDef, NAString &table);
Lng32 readHistograms(HSTableDef *tabDef  
                    , NAString fullQualName
                    , NAString histogramTableName
                    , NAString histintsTableName
                    , NABoolean specialTable
                    , const ExtendedQualName::SpecialTableType type
                    , const NAColumnArray &colArray
                    , Int64 &statsTime
                    , NABoolean &allFakeStat
                    , const NABoolean preFetch
                    , NABoolean *fakeHistogram
                    , NABoolean *emptyHistogram
                    , NABoolean *smallSampleHistogram
                    , double    *smallSampleSize
                    , Lng32      *colmap
                    , double    &histogramRowCount
                    , HSColStats *cs
                    , Lng32      offset
                    );
Lng32 stripCatSchTab(const QualifiedName &qualifiedName, NAString &pred);

// *****************************************************************************
// CLASS      HSStatsTimeCursor
// PURPOSE    Uses CURSOR C3, in SQLHIST.MDF, to determine the most recent
//            timestamp of the latest update statistics on interested table.
// METHODS    open()   - opens the cursor.
//            get()    - retrieves the most recent timestamp.
//            update() - updates the READ_TIME and READ_COUNT columns of the
//                       HISTOGRAMS table if histogram requested by compiler.
//            close()  - closed the cursor.
// *****************************************************************************
class HSStatsTimeCursor
{
public:
  HSStatsTimeCursor(const char *tablename,
                    const char *histogramTableName,
                    ComUID tableUID,
                    ComDiskFileFormat fileType,
                    NABoolean updatable);
  ~HSStatsTimeCursor();

  Lng32 open(NABoolean updateReadTime);
  void close();
  Lng32 get(Int64 &statTime, const NAColumnArray &colArray, NABoolean &updateReadTime);
  Lng32 update(const NAColumnArray &colArray);

private:
  NABoolean   validCursor_;
  const char *histogramTableName_;
  ComDiskFileFormat fileType_;
  ComUID      tableUID_;
  Int64       statsTime_;
  NABoolean   updatable_;
  NABoolean   startedTrans_;
  SQLDESC_ID *desc_;
  Lng32       update_retcode_;
  Lng32       retcode_;
  // Cursor for dynamic query to get read time and read count.
  HSCursor   *cursor102_;
  // Same as cursor102_, but updateable.
  HSCursor   *cursor106_;
};
                                                 /*========CONSTRUCTOR========*/
HSStatsTimeCursor::HSStatsTimeCursor
                   ( const char *tablename,
                     const char *histogramTableName,
                     ComUID tableUID,
                     ComDiskFileFormat fileType,
                     NABoolean updatable)
 : validCursor_(TRUE),
   histogramTableName_(histogramTableName),
   fileType_(fileType),
   tableUID_(tableUID),
   statsTime_(0), updatable_(updatable),
   desc_(NULL), update_retcode_(0), retcode_(0),
   cursor102_(NULL),
   cursor106_(NULL)
 {}

                                                 /*========DESTRUCTOR=========*/
HSStatsTimeCursor::~HSStatsTimeCursor()
{
  close();
}

Lng32 HSStatsTimeCursor::open(NABoolean updateReadTime)
{
  Lng32 retcode;
  HSGlobalsClass::autoInterval = CmpCommon::getDefaultLong(USTAT_AUTOMATION_INTERVAL);

  NAString qry;
  char sbuf[25];
  sprintf(sbuf, PF64, tableUID_.get_value());
  
  if (updateReadTime) 
    {  
      HSTranMan *TM = HSTranMan::Instance();
      startedTrans_ = (((retcode_ = TM->Begin("CURSOR106")) == 0) ? TRUE : FALSE);
      HSHandleError(retcode_);
      qry =      "SELECT JULIANTIMESTAMP(STATS_TIME), COLUMN_NUMBER, JULIANTIMESTAMP(READ_TIME),"
                 " READ_TIME, READ_COUNT "
                 " FROM ";
      qry.append(histogramTableName_);
      qry.append(" WHERE TABLE_UID = ");
      qry.append(sbuf);
      qry.append("   AND COLCOUNT = 1 AND REASON <> ' '");
      qry.append(" ORDER BY TABLE_UID, HISTOGRAM_ID, COL_POSITION");
      qry.append(" FOR READ COMMITTED ACCESS");
      qry.append(" FOR UPDATE OF READ_COUNT, READ_TIME");

      if (cursor106_)
        delete cursor106_;
      cursor106_ = new HSCursor;
      cursor106_->setCursorName("CURSOR106_MX_2300");
      retcode = cursor106_->prepareQuery(qry.data(), 0, 5);
      HSHandleError(retcode);

      retcode = cursor106_->open();
      HSHandleError(retcode);
    }
  else
    {
      qry =      "SELECT JULIANTIMESTAMP(STATS_TIME), COLUMN_NUMBER,"
                 " JULIANTIMESTAMP(READ_TIME), READ_COUNT"
                 " FROM ";
      qry.append(histogramTableName_);
      qry.append(" WHERE TABLE_UID = ");
      qry.append(sbuf);
      qry.append("   AND COLCOUNT = 1 AND REASON <> ' '");
      qry.append(" ORDER BY TABLE_UID, HISTOGRAM_ID, COL_POSITION");
      qry.append(" FOR READ UNCOMMITTED ACCESS");

      if (cursor102_)
        delete cursor102_;
      cursor102_ = new HSCursor;
      retcode = cursor102_->prepareQuery(qry.data(), 0, 4);
      HSHandleError(retcode);

      retcode = cursor102_->open();
      HSHandleError(retcode);
    }

  return retcode;
}

void HSStatsTimeCursor::close()
{
  // Commit transaction if started for updatable CURSOR106 (for READ_TIME).
  HSTranMan *TM = HSTranMan::Instance();
  if (startedTrans_)
  {
    if (!cursor106_ || update_retcode_) TM->Rollback();
    else                           TM->Commit();  // doesn't issue COMMIT if trans not started.
  }
  if (desc_)
    {
      SQL_EXEC_ClearDiagnostics(desc_);
      Lng32 retcode = SQL_EXEC_CloseStmt(desc_);
      if (retcode)
        HSLogError(retcode);
      SQL_EXEC_ClearDiagnostics(desc_);
      delete (char*)(desc_->identifier);
      delete desc_;
      desc_ = NULL;
    }

  delete cursor102_;
  cursor102_ = NULL;
  delete cursor106_;
  cursor106_ = NULL;
}

// -----------------------------------------------------------------------------
// METHOD     HSStatsTimeCursor::get
// PURPOSE    1. Sets 'maxStatTime' to the most recent STATS_TIME timestamp of 
//               any requested histogram.   STATS_TIME is the last time 
//               update statistics generated a histogram.  
//            2. Sets updateReadTime flag if necessary.
// ARGUMENTS  maxStatsTime - the most recent generation time for a requested 
//                           histogram. OUTPUT
//            colArray - the column array of the table in question.  This array
//                       indicates whether the optimizer needs histograms for a
//                       column, via needFullHistogram(). INPUT
// -----------------------------------------------------------------------------
Lng32 HSStatsTimeCursor::get(Int64 &maxStatTime, const NAColumnArray &colArray, 
                            NABoolean &updateReadTime)
{
  Int32 colNumber;
  Int64 statTime, readTime;
  short readCount;

  maxStatTime = 0;
  HSLogMan *LM = HSLogMan::Instance();
  HSTranMan *TM = HSTranMan::Instance();
  updateReadTime = FALSE;
  
  LM->LogTimeDiff("Entering: HSStatsTimeCursor::get()");
  while(TRUE) // Loop until break statement below.
  {
    retcode_ = SQL_EXEC_Fetch(cursor102_->getStmt(), cursor102_->getOutDesc(), 4,
                                (void *)(&statTime),   NULL, 
                                (void *)(&colNumber),  NULL,
                                (void *)(&readTime),   NULL,
                                (void *)(&readCount),  NULL);
    if (retcode_ == HS_EOF || retcode_ < 0) break;

    LM->Log("While Fetching StatsTime: Check for update of READ_TIME/READ_COUNT.");
    if (colNumber < (signed) colArray.entries())
    {
      maxStatTime = MAXOF(maxStatTime, statTime);
      const NAColumn *nacol = colArray.getColumnByPos(colNumber);
      // Only update READ_TIME if automation is on and the column is requested.
      if (!updateReadTime && // If already set, no need to check again.
          updatable_ &&
          nacol && 
          nacol->needFullHistogram()
          )
      {
        // Check if READ_TIME is such that it needs to be updated.
        Int64 readTm      = hs_getEpochTime(readTime);
        Int64 currentTime = hs_getEpochTime();
        Int64 readCountUpdateInterval =
            CmpCommon::getDefaultLong(USTAT_AUTO_READTIME_UPDATE_INTERVAL);

        if (readTm == 0 ||  readTm <= currentTime - readCountUpdateInterval)
        {
          // Update the READ_TIME and READ_COUNT if automation is on and read
          // count refresh interval time has passed since current READ_TIME. 
          updateReadTime = TRUE;
        }
        if (LM->LogNeeded())
        {
          sprintf(LM->msg, "Checking READ_TIME of column %s", nacol->getColName().data());
          LM->Log(LM->msg);
          sprintf(LM->msg,
            "HSStatsTimeCursor: USTAT_AUTO_READTIME_UPDATE_INTERVAL=" PF64
            ", currentTime=" PF64
            ", readTime=" PF64 "\n",
            readCountUpdateInterval, currentTime, readTm);
          LM->Log(LM->msg);
        }
      }
    }
  }
  LM->LogTimeDiff("Exiting: HSStatsTimeCursor::get()");

  if (retcode_ < 0)
    {
      HSFuncMergeDiags(-UERR_INTERNAL_ERROR, "fetching from HSStatsTimeCursor", NULL, TRUE);
      return -1;
    }
  else
    return 0;
}

// -----------------------------------------------------------------------------
// METHOD     HSStatsTimeCursor::update
// PURPOSE    1. Updates READ_TIME and READ_COUNT for any requested histogram.
// ARGUMENTS  colArray - the column array of the table in question.  This array
//                       indicates whether the optimizer needs histograms for a
//                       column, via needFullHistogram(). INPUT
// -----------------------------------------------------------------------------
Lng32 HSStatsTimeCursor::update(const NAColumnArray &colArray)
{
  Int32 colNumber;
  Int64 statTime, readTime;
  char readTimeTstmp[HS_TIMESTAMP_SIZE];
  short readCount;

  HSLogMan *LM = HSLogMan::Instance();
  HSTranMan *TM = HSTranMan::Instance();
  
  LM->LogTimeDiff("Entering: HSStatsTimeCursor::update()");
  while(TRUE) // Loop until break statement below.
  {
    retcode_ = SQL_EXEC_Fetch(cursor106_->getStmt(), cursor106_->getOutDesc(), 5,
                              (void *)(&statTime),   NULL, 
                              (void *)(&colNumber),  NULL,
                              (void *)(&readTime),   NULL,// for read
                              (void *)readTimeTstmp, NULL,// for update
                              (void *)(&readCount),  NULL);
    if (retcode_ == HS_EOF || retcode_ < 0 || update_retcode_ != 0) break;

    LM->Log("While performing update: Check for update of READ_TIME/READ_COUNT.");
    if (colNumber < (signed) colArray.entries())
    {
      const NAColumn *nacol = colArray.getColumnByPos(colNumber);
      // Only update READ_TIME if column is requested.
      if (nacol && 
          nacol->needFullHistogram()
          )
      {
        // Update the READ_TIME and READ_COUNT.
        // If updateHistogram() is unsuccessful, it will return a non-zero
        // return code.
        if (readCount < SHRT_MAX) readCount++;
        update_retcode_ = updateHistogram(histogramTableName_, 106, readCount);
      }
    }
  }
  LM->LogTimeDiff("Exiting: HSStatsTimeCursor::update()");

  if (retcode_ < 0)
    {
      HSFuncMergeDiags(-UERR_INTERNAL_ERROR, "fetching from HSStatsTimeCursor for update", NULL, TRUE);
      return -1;
    }
  else
    return 0;
}

// -----------------------------------------------------------------------
// Cursor class to read HISTOGRAMS table.
// -----------------------------------------------------------------------
class HSHistogrmCursor {
  friend void getPreviousUECRatios(/*HSGlobalsClass*,*/ HSColGroupStruct*); //@ZXuec
public:

  HSHistogrmCursor( const char *tablename
                  , const char *histogramTableName
                  , ComUID tableUID
                  , ComDiskFileFormat fileType
                  , NABoolean updatable
                  , NAHeap *h=STMTHEAP
                  );
  ~HSHistogrmCursor();

  Lng32 open();

  Lng32 fetch( HSColStats &cs
            , HSHistintsCursor &cursor2
            , Lng32 *colmap
            , NABoolean *fakeHistogram
            , NABoolean *emptyHistogram
            , NABoolean *smallSampleHistogram
            , double    *smallSampleSize
            , double &fakeRowCount
            , Int64 &statsTime
            , NABoolean &allFakeStat
            , const NABoolean preFetch
            // Used to align the columns of the subject table with the columns of
            // the tmp trigger table. See HSHistogrmCursor::fetch below for more details.
            , Lng32 offset
            , HSTableDef *tabDef
            , NABoolean cmpContextSwitched=FALSE
            );

  Lng32 get();

  // accessors
  ULng32 getMaxHistid() { return maxHistid_; };

  static THREAD_P Lng32 fetchCount_;

private:

  NABoolean validCursor_;
  fstream fs_;

  const char *histogramTableName_;
  ComDiskFileFormat fileType_;
  ComUID tableUID_;
  ULng32 histid_;

  Lng32 tableColNum_;
  Lng32 colCount_;
  short intCount_;
  wchar_t *buf1_, *buf2_, *buf3_, *buf4_;

  NABoolean adjRowCount_;
  double totalRowCount_;
  double correctedRowCount_;
  double totalUec_;
  Int64 statsTime_;
  Int64 maxStatsTime_;
  NAWchar *lowval_, *highval_;

  SQLDESC_ID *desc_;
  Lng32 update_retcode_;
  Lng32 retcode_;

  HSCursor* cursor101_;   // for dynamic query alternative

  // The following are columns for automation
  // The readTime is retrieved in two variables:
  // The string (TIMESTAMP(0)) is used to retrieve "raw" READ_TIME without conversion
  // so that it can be updated when necessary. See updateHistogram()
  // The number is the READ_TIME converted to JULIANTIMESTAMP using the SQL function;
  // it is used for computation.
  char readTime_timestamp0_[HS_TIMESTAMP_SIZE];
  Int64 readTime_;
  short readCount_;
  double sampleSecs_;
  double colSecs_;
  short samplePercent_;
  double cv_;
  char reason_;
  double avgVarCharCount_;
  Int64 v1_; //,v3_,v4_;
  //wchar_t *v5_, *v6_;
  NABoolean updatable_;

  ULng32 maxHistid_;
  NAHeap *heap_;

  HSHistogrmCursor(const HSHistogrmCursor &other);
  HSHistogrmCursor& operator=(const HSHistogrmCursor &other);
};

// -----------------------------------------------------------------------
// Cursor class to read HISTOGRAM_INTERVALS table.
// -----------------------------------------------------------------------
class HSHistintsCursor {

public:

  HSHistintsCursor( const char *tablename
                  , const char *histintsTableName
                  , const ComUID tableUID
                  , const ComDiskFileFormat fileType
                  , NAHeap *h=STMTHEAP
                  );
  ~HSHistintsCursor();

  Lng32 open();

  Lng32 fetch( HSColStats &cs
            , const NABoolean needHistints
            , const ULng32 histid
            , const double totalRowCount
            , const double totalUec
            , const NAWchar *highval
            );

  Lng32 get( const ULng32 histid,
            const Lng32 i
          );

  static THREAD_P Lng32 fetchCount_;

private:

  NABoolean validCursor_;
  fstream fs_;

  const char *histintsTableName_;
  ComDiskFileFormat fileType_;
  const ComUID tableUID_;
  ULng32 histid_;
  short short1_;
  NAWchar *buf_;

  double rowCount_;
  double uec_;
  NAWchar *boundary_;
  Lng32 intNum_;

  SQLDESC_ID *desc_;
  Lng32 retcode_;
  NAHeap *heap_;

  HSCursor* cursor201_;  // Used for dynamic version of query that reads intervals

  // the following are added in R2.3
  double stdDevOfFreq_;
  double mfvRowCnt_, mfv2RowCnt_;
  //double v3_, v4_;
  wchar_t *buf_mfv_;
  //wchar_t *buf_v6_;
  wchar_t *mfv_;
  //wchar_t *v6_;

  HSHistintsCursor(const HSHistintsCursor &other);
  HSHistintsCursor& operator=(const HSHistintsCursor &other);
};

// -----------------------------------------------------------------------
// Class to construct ColStats.
// -----------------------------------------------------------------------
class HSColStats {

public:

  HSColStats( const NAColumnArray &colArray
            , StatsList &colStatsList
            , NAMemory *heap
            )
    : colArray_(colArray), colStatsList_(colStatsList),
      heap_(heap), lastInt_(-1),
      usedHistogramCount_(0), usedHistintsCount_(0)
  {}

  const NAColumnArray &colArray() const { return colArray_; }

  Lng32 lastInt() const                  { return lastInt_; }

  void addHistogram( const ULng32 histid
                   , const Lng32 colCount
                   , const Lng32 *colmap
                   , const Lng32 intCount
                   , const double totalRowCount
                   , double & totalUec /* can change, if column is unique */
                   , const double vCharSize
                   , const NAWchar *lowval
                   , const NAWchar *highval
                   , NABoolean isSmallSample
                   , NABoolean isFakeHistogram
                   );

  void addHistint( NABoolean needHistints
                 , Int32 intnum
                 , const NAWchar *boundary
                 , double rowCoun
                 , double uec
                 , double stdDevOfFreq
                 , double mfv_rows
                 , double mfv2_rows
                 , const wchar_t *mfv
                 , NABoolean fakeHist = FALSE
                 );

  void fixRedFactor( double rowRedFactor
                   , double uecRedFactor
                   );

  // normalize total row count and uec.
  void normalize(const CostScalar newRowCount);

  Lng32 histogramCount() const { return usedHistogramCount_; }
  Lng32 histintsCount() const  { return usedHistintsCount_; }

  // Accessor functions.
  NABoolean isUnique() const { return ( colStats_ ? colStats_->isUnique(): FALSE ); }
  double getRowCount() const { return ( colStats_ ? colStats_->getRowcount().value(): -1 ); }
    // Will return row count of last colStats_. Should be same as all others (after normalize()).

private:

  const NAColumnArray &colArray_;
  StatsList &colStatsList_;
  NAMemory *heap_;

  ColStatsSharedPtr colStats_;
  HistogramSharedPtr hist_;         // pointer to colStats_->histogram, for convenience
  Lng32 lastInt_;

  Lng32 usedHistogramCount_;
  Lng32 usedHistintsCount_;

  HSColStats(const HSColStats &other);
  HSColStats& operator=(const HSColStats &other);
};

// -----------------------------------------------------------------------
// Fetch histogram rows and construct colStats.
//
// Note that in this file, "histogram" sometimes refers to a ColStats class
// and sometimes to a Histogram class (ColStats::histogram_).
//
//
// Here are the requirements, reproduced from BindWA::markAsReferencedColumn():
//   Optimizer will expect:
//
//   - Full stats (a ColStats header and as many HistInts as there are)
//   from each referenced column's *single-column* histogram, and
//   will want full stats from any *multi-column* histogram
//   that contains a referenced column (for MDAM).
//
//   - Of columns that are not referenced in this query, those that belong to
//   an index (any one) must have short stats (a ColStats header, no intervals)
//   from their *single-column* histogram.  As there will always be at least
//   one key column in a table (SYSKEY at the least), each table will end up
//   having at least one ColStat, even if no refd cols ("SELECT c FROM t;"),
//   which is another Optimizer assumption.
//
//   - All other columns -- those not deemed referenced by the criteria below
//   which also are not index keys -- the Optimizer has no use for any stats.
//
// FetchHistograms applies these rules and delivers the minimum required stats,
// not creating any object that will not be needed.
//
// For normal tables, static-SQL cursors read all relevant rows from the
// histogram tables.  See HSHistogrmCursor::fetch for discussion of how
// some fetches can be avoided, others not.
//
// Note that using dynamic-SQL to restrict the rows read from disk to
// only those for columns of interest is problematic:
// - Say we build the query text, with a new predicate
//      COLUMN_NUMBER IN ( list of refd/indexed columns from colArray )
//   Then we must SQL-prepare this, which is time-consuming.
// - A static parametrized query like the above would not work,
//   unless we had one for each size of list -- i.e.,
//      COLUMN_NUMBER IN (?)  and  IN (?,?)  and  IN (up to 100's of ?'s!)
// - Finally, a simple IN-list as above might work for single columns,
//   but would not always return all histogram rows for multi-column histograms
//   (e.g., multi-col hist on (P,Q,R) where only R is refd/ixd...).
//
//Assumption: a multi-column histogram is retrieved when 
//histograms for any of its columns are retrieved.
//e.g. Table T1(a int, b int, c int)
//histograms: {a},{b},{c},{a,b},{a,c},{b,c},{a,b,c}
//If histograms for column a are fetched we will get 
//histograms: {a}, {a,b}, {a,c}, {a,b,c}
//If histograms for column b are fetched we will get
//histograms: {b}, {a,b}, {b,c}, {a,b,c}
// -----------------------------------------------------------------------

THREAD_P Lng32 HSHistogrmCursor::fetchCount_ = 0;
THREAD_P Lng32 HSHistintsCursor::fetchCount_ = 0;

Lng32 FetchHistograms( const QualifiedName & qualifiedName
                    , const ExtendedQualName::SpecialTableType type
                    , const NAColumnArray & colArray
                    , StatsList & colStatsList
                    , NABoolean isSQLMPTable
                    , NAMemory * heap
                    , Int64 & statsTime
                    , NABoolean & allFakeStat
                    , const NABoolean preFetch
                    , Int64 createStatsSize
                    )
{
  HSLogMan *LM = HSLogMan::Instance();
  LM->LogTimeDiff("\nEntering FetchHistograms ================================", TRUE);
  LM->StartTimer("FetchHistograms()");
  HSPrologEpilog pe("FetchHistograms()");

  HSGlobalsClass::schemaVersion = COM_VERS_UNKNOWN;
  HSGlobalsClass::autoInterval = 0; 

  allFakeStat = TRUE;

  Lng32 retcode = 0;
  ComUID tableUID;
  TotalHistintsDBG = 0;
  TotalHistogramDBG = 0;
  double defaultFakeRowCount = CostPrimitives::getBasicCostFactor(HIST_NO_STATS_ROWCOUNT);
  double defaultFakeUec = CostPrimitives::getBasicCostFactor(HIST_NO_STATS_UEC);
  NABoolean isEstimate = TRUE;

  //NATable Caching

  HSHistogrmCursor::fetchCount_ = 0;
  HSHistintsCursor::fetchCount_ = 0;
  
  NAString fullQualName;
  NAString histogramTableName;
  NAString histintsTableName;
  NABoolean specialTable = TRUE;
  ComDiskFileFormat fileType = (isSQLMPTable) ? SQLMP : SQLMX;
  ComObjectName *objectName;
  HSTableDef *tabDef;
  char uidStr[30];

  // Check for debug info set in CQD
  NAString debugFile = ActiveSchemaDB()->getDefaults().getValue(USTAT_DEBUG_TEST);
  if (debugFile != "")
  {
    // Extract variables.  Only allow no spaces or 1 space surrounding '='.
    Lng32 statsSize=0;
    sscanf(debugFile, "createStatsSize=%d", &statsSize);
    sscanf(debugFile, "createStatsSize = %d", &statsSize);
    createStatsSize = statsSize;
  }
                                                 /*===========================*/
                                                 /*=  DETERMINE TABLE NAMES  =*/
                                                 /*===========================*/
  // Special treatment for tmp trigger tables
  // Statistics are not updated on such a table. Therefore, in order to
  // get a valid histogram for such a table, it is read from the subject table.
  // The cardinality of the histogram is then adjusted during optimization.
  ExtendedQualName::SpecialTableType actualType = type; // By default, don't fake
  QualifiedName  actualQualifiedName (qualifiedName,
                                      STMTHEAP); // By default, don't change the name,
  Lng32 offset = 0;
  if (type == ExtendedQualName::TRIGTEMP_TABLE)
  {
    actualType = ExtendedQualName::NORMAL_TABLE; // Simulate a normal table
    // Compute the name of the subject table
    NAString fakedObjectName = qualifiedName.getObjectName();
    fakedObjectName.remove(fakedObjectName.index("__TEMP"));

    actualQualifiedName.setObjectName(fakedObjectName);
    // If we fetch the histograms of the subject table
    // in order to get statistics for a tmp trigger table,
    // the offset is used to align the columns of the subject table
    // with the columns of the tmp trigger table. The first two
    // columns of the tmp trigger table (uniquifier) does not have
    // corresponding columns in the subject table. Therefore, we must
    // add 2 to the tableColNum_ read from the histogram
    // of the subject table in order to compute the column number in the
    // tmp trigger table.
    offset = 2;
  }

  //ComObjectName does not handle all types of table format correctly, i.e.
  //ANSI, SHORTANSI and GUARDIAN. Basically, we can categorize table types into
  //two types: MX/MP. For MX, ComObjectName works fine. For MP, we need use
  //ComMPLoc prior to using ComObjectName. This will guarantee a proper guardian
  //table format.
  if (fileType == SQLMX)
    {
      objectName = new(STMTHEAP) ComObjectName(actualQualifiedName.getQualifiedNameAsAnsiString(),
                                                COM_UNKNOWN_NAME,
                                                TRUE,
                                                STMTHEAP);
      tabDef = HSTableDef::create(STMTHEAP,
                                  *objectName,
                                  ANSI_TABLE,
                                  ExtendedQualName::convSpecialTableTypeToAnsiNameSpace(actualType));
    }
  else
    {
      ComMPLoc tempObj (actualQualifiedName.getQualifiedNameAsString().data(),
                        ComMPLoc::FILE);
      objectName = new(STMTHEAP) ComObjectName(tempObj.getSysDotVol(),
                                               tempObj.getSubvolName(),
                                               tempObj.getFileName(),
                                               COM_UNKNOWN_NAME,
                                               ComAnsiNamePart::INTERNAL_FORMAT,
                                               STMTHEAP);
      tabDef = HSTableDef::create(STMTHEAP,
                                  *objectName,
                                  GUARDIAN_TABLE,
                                  ExtendedQualName::convSpecialTableTypeToAnsiNameSpace(actualType));
    }

  if ((actualType == ExtendedQualName::NORMAL_TABLE ||
       actualType == ExtendedQualName::IUD_LOG_TABLE ||
       actualType == ExtendedQualName::MV_TABLE) &&
      NOT isSpecialObject(*objectName))
    {
      if (tabDef->objExists())
        {

          specialTable = FALSE;

          if (fileType == SQLMX)
            {
              NAString histLoc =
                getHistogramsTableLocation
                (tabDef->getHistLoc(HSTableDef::EXTERNAL_FORMAT),
                 tabDef->isInMemoryObjectDefn());
              if (HSGlobalsClass::isHbaseCat(objectName->getCatalogNamePart().getInternalName()) ||
                  HSGlobalsClass::isHiveCat(objectName->getCatalogNamePart().getInternalName()))
                {
                  histogramTableName = histLoc + "." + HBASE_HIST_NAME;
                  histintsTableName  = histLoc + "." + HBASE_HISTINT_NAME;
                }
              else
                {
                  histogramTableName = histLoc + ".HISTOGRAMS";
                  histintsTableName  = histLoc + ".HISTOGRAM_INTERVALS";
                }

              /*
              histogramTableName = tabDef->getHistLoc(HSTableDef::EXTERNAL_FORMAT) + ".HISTOGRAMS";
              histintsTableName  = tabDef->getHistLoc(HSTableDef::EXTERNAL_FORMAT) + ".HISTOGRAM_INTERVALS";
                */
            }
          else
            {
              histogramTableName = tabDef->getCatalogLoc(HSTableDef::EXTERNAL_FORMAT) + ".HISTOGRM";
              histintsTableName  = tabDef->getCatalogLoc(HSTableDef::EXTERNAL_FORMAT) + ".HISTINTS";
            }

          fullQualName = tabDef->getObjectFullName();
          tableUID = tabDef->getObjectUID();

          if (LM->LogNeeded())
            {
              convertInt64ToAscii(tabDef->getObjectUID(), uidStr);
              sprintf(LM->msg, "\nHSREAD: Object: %s(%s), type=%d, modtype=%d", fullQualName.data(), uidStr, type, actualType);
              LM->Log(LM->msg);
            }
        }
    }

  // -----------------------------------------------------------------------
  // fakeHistogram:  Entry k is FALSE if there exists a single-column 
  //                 histogram for table column k.
  //                 This flag array is used to determine if we need to 
  //                 supply fake histograms.
  // emptyHistogram: Entry k is TRUE if there exists a single-column 
  //                 empty histogram for table column k.
  //                 This flag array is used to avoid duplicate entries of 
  //                 empty histograms for the same table column.
  //                 The empty histograms are used for histogram automation.
  // smallSampleHistogram: Entry k is TRUE if the histogram read is a small
  //                 sample histogram (REASON = 'S').
  // smallSampleSize: Entry k is the size of the sample used for the 
  //                 histogram.
  // -----------------------------------------------------------------------
  CollIndex numCols = colArray.entries();
  NABoolean *fakeHistogram;
  NABoolean *emptyHistogram; 
  NABoolean *smallSampleHistogram; 
  double    *smallSampleSize; 
  NANewArray<NABoolean> tmpfh(fakeHistogram, numCols);
  NANewArray<NABoolean> tmpeh(emptyHistogram, numCols);
  NANewArray<NABoolean> tmpsh(smallSampleHistogram, numCols);
  NANewArray<double>    tmpzh(smallSampleSize, numCols);
  CollIndex k = 0;
  for (k = 0; k < numCols; k++) 
  {
    fakeHistogram[k]        = TRUE;  // Init to TRUE, will set to FALSE if hist read.
    emptyHistogram[k]       = FALSE; // Init to FALSE, will set to TRUE if reason=' '.
    smallSampleHistogram[k] = FALSE; // Init to FALSE, will set to TRUE if reason='S'.
    smallSampleSize[k]      = 0;     // Init to 0, will set to val if reason='S'.
  }

  // -----------------------------------------------------------------------
  // Other structures.
  // -----------------------------------------------------------------------
  HSColStats cs(colArray, colStatsList, heap);
  Lng32 *colmap;                                 // Lng32 = column position in tbl
  NANewArray< Lng32 > tmpcm(colmap, numCols);      // no need to initialize

  // -----------------------------------------------------------------------
  // For packed tables (which must be a VP table as well), code is already
  // in place in UPDATE STATISTICS to put an estimated block count in the
  // smd of the base table of the root table of the VP table. This count can
  // give us a good estimate of the number of packed rows in the table. The
  // code below enables the block count to be fetched by changing objName to
  // the real table name from the fabricated name. Note that, however, we
  // don't fetch the histograms for the packed table since the histograms
  // reflect the statistics on the unpacked version of the table. They are
  // obtained at the UnPackRows node instead.                 - (10/97)
  // -----------------------------------------------------------------------
  NABoolean isPackedTable = FALSE;
  if (actualType == ExtendedQualName::VIRTUAL_TABLE &&
      FUNNY_ANSI_IDENT_HAS_PREFIX(actualQualifiedName.getObjectName(), PACKED__))
    {
      NAString objName(actualQualifiedName.getObjectName());
      isPackedTable = TRUE;

      // Get real name of the table which is packed.
      FUNNY_ANSI_IDENT_REMOVE_PREFIX(objName, PACKED__);
      fullQualName = actualQualifiedName.getCatalogName();
      fullQualName.append(".");
      fullQualName.append(actualQualifiedName.getSchemaName());
      fullQualName.append(".");
      fullQualName.append(objName);
    }
  double rowCount          = -1;
  double histogramRowCount = -1;
  retcode = readHistograms(tabDef, fullQualName, histogramTableName, histintsTableName, 
                           specialTable, type, colArray, statsTime, allFakeStat, preFetch, 
                           fakeHistogram, emptyHistogram, smallSampleHistogram, 
                           smallSampleSize, colmap, histogramRowCount, &cs, offset);

  // Obtain OSIM mode
  OptimizerSimulator::osimMode osimMode = OptimizerSimulator::OFF;
  if(CURRCONTEXT_OPTSIMULATOR && !CURRCONTEXT_OPTSIMULATOR->isCallDisabled(12)) 
    osimMode = CURRCONTEXT_OPTSIMULATOR->getOsimMode();

  // Check if small sample statistics should be created (because no histograms exist and
  // createStatsSize != 0. Only perform when OSIM is OFF.

  // Generation of small-sample stats on demand currently conflicts with hbase
  // row count estimation code. There is an LP case filed for this, but in the
  // meantime, createStatsSize is set to 0 to disable on-demand stats.
  createStatsSize = 0;

  // FetchHistograms does not allow calling update stats to create recursion. 
  // To avoid this, a CQD is checked and if OFF will set it to ON.
  // On demand stats are not created if createStatsSize == 0, the table has 0 rows, 
  // the table is an SMD, the table is volatile, or the table is not a normal table.
  if (createStatsSize != 0  && 
      !tabDef->isMetadataObject() &&
      !tabDef->isVolatile() &&
      type == ExtendedQualName::NORMAL_TABLE && 
      osimMode != OptimizerSimulator::SIMULATE &&
      CmpCommon::getDefault(USTAT_FETCHCOUNT_ACTIVE) != DF_ON)       
  {
    HSFuncExecQuery("CONTROL QUERY DEFAULT USTAT_FETCHCOUNT_ACTIVE 'ON'");
    // If there are no histograms (histogramRowCount == -1) or the histograms indicate 0
    // rowcount, get actual rowcount now.
    if (histogramRowCount > 0 || 
        (rowCount = getRowCountForFetchFuncs(tabDef, isEstimate)) > 0)
    {
      NAString colNames;
      NAString dblQuote="\"";
      double dblCreateStatsSize = convertInt64ToDouble(createStatsSize);
      double dblDefSampleSize = convertInt64ToDouble(
                                 getDefaultSampleSize((rowCount > 0) ? 
                                                       (Int64)rowCount : 
                                                       (Int64)histogramRowCount));
      for (k = 0; k < numCols; k++)
        {
          // Add column to update stats list if none exists and the compiler needs it,
          // OR a histogram exists that was created using small sampling, it's needed,
          // and the sample size previously used is less than createStatsSize and less
          // than the default sample size by 10%.
          if ((fakeHistogram[k]        && colArray[k]->needFullHistogram()) ||
              (smallSampleHistogram[k] && colArray[k]->needFullHistogram() &&
               smallSampleSize[k] < dblCreateStatsSize * 0.9 &&
               smallSampleSize[k] < dblDefSampleSize   * 0.9 )   )
            // Add column to list.
            // Surround column name with double quotes, if not already delimited.
            if (colArray[k]->getColName().data()[0] == '"') 
                 colNames += colArray[k]->getColName()+",";
            else colNames += dblQuote+colArray[k]->getColName()+dblQuote+",";
        }
      if (colNames.length() != 0)  // Some column(s) need histograms to be generated.
        {
          colNames.remove(colNames.length() - 1);    // remove last commma  

          NAString ustatCmd = "UPDATE STATISTICS FOR TABLE ";
          ustatCmd += getTableName(tabDef->getObjectFullName(), tabDef->getNameSpace());
          ustatCmd += " ON ";
          ustatCmd += colNames;

          // Get actual row count if not already obtained.
          if (rowCount < 0) rowCount = getRowCountForFetchFuncs(tabDef, isEstimate);  

          // If # sample rows is -1 or is greater than default sample size, use default sampling.
          if (createStatsSize == -1 || rowCount == -1 ||
              createStatsSize > (float) getDefaultSampleSize((Int64)rowCount))
            ustatCmd += " SAMPLE;";
          else 
          {
            char percentStr[30];
            sprintf(percentStr, "%f", (float)createStatsSize/(float)rowCount * 100);
            ustatCmd += " SAMPLE RANDOM ";
            ustatCmd += percentStr;
            ustatCmd += " PERCENT;";
          }
          // Perform update statistics for small sample tables.  Note that the REASON
          // field is set to 'S' by update statistics based on its own criteria.
          NABoolean requestedByOptimizer = TRUE;
          if (UpdateStats((char *)ustatCmd.data(), requestedByOptimizer) == 0)
          // Call readHistograms() again if quick stats was successful.

          retcode = readHistograms(tabDef, fullQualName, histogramTableName, histintsTableName, 
                                   specialTable, type, colArray, statsTime, allFakeStat, preFetch, 
                                   fakeHistogram, emptyHistogram, smallSampleHistogram, 
                                   smallSampleSize, colmap, histogramRowCount, &cs, offset);
       }
    }
    HSFuncExecQuery("CONTROL QUERY DEFAULT USTAT_FETCHCOUNT_ACTIVE 'OFF'");// Always returns 0.
  }

  // -----------------------------------------------------------------------
  // Add fake histograms for single columns if they don't have any.
  // -----------------------------------------------------------------------
  if (NOT specialTable) defaultFakeRowCount = histogramRowCount;

  if (defaultFakeRowCount <= 0 && NOT specialTable)
    {
      NABoolean isEstimate = FALSE;
      // Check for OSIM mode
      switch (osimMode)
      {
        case OptimizerSimulator::OFF:
        case OptimizerSimulator::CAPTURE:
        case OptimizerSimulator::LOAD:
        
          if (tabDef->isInMemoryObjectDefn())
            {
              // not sure yet what to do about stats for in-memory definitions.
              // Just put in 100.
              rowCount = 100;
            }
          else
            {
              if (rowCount == -1) // Not set yet, get rowcount from table.
                {
                  if (!specialTable &&
                      CmpCommon::getDefault(USTAT_FETCHCOUNT_ACTIVE) != DF_ON)
                    {
                      rowCount=getRowCountForFetchFuncs(tabDef, isEstimate);
                    }
                }
            }
          if(osimMode == OptimizerSimulator::CAPTURE)
            CURRCONTEXT_OPTSIMULATOR->capture_getEstimatedRows(tabDef->getObjectFullName().data(),
                                                   rowCount);
          break;
        case OptimizerSimulator::SIMULATE:
          rowCount = CURRCONTEXT_OPTSIMULATOR->simulate_getEstimatedRows(tabDef->getObjectFullName().data());
          break;
        default:
          ABORT("The OSIM must run under OFF (normal), CAPTURE or SIMULATE mode");
          break;
      }
      double estUec = rowCount * CostPrimitives::getBasicCostFactor(USTAT_MODIFY_DEFAULT_UEC);
      defaultFakeRowCount = MAXOF(rowCount, CostPrimitives::getBasicCostFactor(HIST_NO_STATS_ROWCOUNT));

      if (estUec)
      {
        defaultFakeUec = MAXOF (estUec, CostPrimitives::getBasicCostFactor(HIST_NO_STATS_UEC));
        defaultFakeUec = MINOF (defaultFakeUec, defaultFakeRowCount);
      }
      else
        defaultFakeUec = MINOF (defaultFakeRowCount, CostPrimitives::getBasicCostFactor(HIST_NO_STATS_UEC));
    } // if (histogramRowCount <= 0 && NOT specialTable)

                                              /*==============================*/
                                              /*= GENERATE DEFAULT HISTOGRAMS */
                                              /*==============================*/
  for (k = 0; k < numCols; k++)
    {
      if (fakeHistogram[k])
        {
          double fakeRowCount = defaultFakeRowCount;
          double fakeUec = defaultFakeUec;

          TotalHistogramDBG++;

          if (preFetch || colArray[k]->needHistogram())
            {
              NABoolean needHistints  = preFetch ||
                          colArray[k]->needFullHistogram() ||
                          (CURRSTMT_OPTDEFAULTS->cacheHistograms()
                           && colArray[k]->needHistogram());

              colmap[0] = (Lng32)k;

              // soln 10-030307-4739 Selectivity too low when comparing CHAR(1)
              // The possible uses of CHAR(1) are 2(boolean),3(3-way logic),
              // 26(English chars), and 255(all characters). The suggestion was
              // made to use the value 10 for fakeUec.
              const NAType *type = colArray[k]->getType();
              if ((type->getTypeName() == "CHAR") &&
                  (type->getNominalSize() == 1))
                {
                  fakeUec = CostPrimitives::getBasicCostFactor(HIST_NO_STATS_UEC_CHAR1);
                }

              //10-030428-5954
              //For unique columns, make sure that uec = rowcount
              if(colArray[k]->isUnique())
                fakeUec = fakeRowCount;

              //make sure that the UEC is never greater than the ROWCOUNT
              fakeUec = MINOF(fakeUec, fakeRowCount);

              // Create default fake histograms only if optimizer has set REFERENCED_FOR_HISTOGRAM
              // flag on the column or the column is index or partitioning key.
              // This is to avoid creating histograms for columns that appear in the select list only.              
                 cs.addHistogram( k + 1 // fake histid
                                      , 1     // colCount
                                      , colmap
                                      , 1     // intCount
                                      , fakeRowCount
                                      , fakeUec
                                      , -1
                                      , L"(>)"
                                      , L"(<)"
                                      , FALSE
                                      , TRUE
                                     );

                cs.addHistint(needHistints, 0, L"(>)", 0, 0, 0, 0, 0, L"()", TRUE);
                cs.addHistint(needHistints, 1, L"(<)", fakeRowCount, fakeUec, 0, 0, 0, L"()", TRUE);
               }
          else
            {
              TotalHistintsDBG += 2;
            }
        }
    }


  LM->StopTimer();  // FetchHistograms()

  HSClearCLIDiagnostics(); // Clear CLI diagnostics area.
  LM->LogTimeDiff("\nExiting FetchHistograms() ---------------------------------");
  // to prevent false alarms for statement heap memory allocation "tabDef"
  // coverity[leaked_storage]

  return 0;
}
// *****************************************************************************
// FUNCTION   readHistograms()
// PURPOSE    Bundles the read of histograms from UMD tables and assignment of
//            colStats array (cs) into one function.  This can be called multiple
//            time.
// INPUTS     tabDef, fullQualName, histogramTableName, histintsTableName,
//            specialTable, type, preFetch - table/other info from FetchHistograms.
//            colArray - info on columns requested by optimizer.
//            offset         a value used for aligning column numbers of histograms.
// INPUT/OUTPUT
//            allFakeStats - set to TRUE on input, will be set to FALSE
//                           if any histogram read.
//            fakeHistogram - an array set to TRUE on input, for every histogram
//                           read, the corresponding entry will be cleared.
//            emptyHistogram - an array set to FALSE on input, for every histogram
//                           read that is empty, the corresponding entry will be
//                           set to TRUE.
//            smallSampleHistogram - an array set to FALSE on input, for every histogram
//                           read that is small sample, the corresponding entry will be
//                           set to TRUE.
// OUTPUTS    statsTime -    the STATS_TIME entry of last histogram read.
//            colmap -       a list of column numbers of the histograms read.
//            histogramRowCount - set to the normalized rowcounts across all histograms
//                           read.
//            cs             an array of HSColStats created from the stats in the 
//                           histograms that were read.
// *****************************************************************************
Lng32 readHistograms(HSTableDef *tabDef  
                    , NAString fullQualName
                    , NAString histogramTableName
                    , NAString histintsTableName
                    , NABoolean specialTable
                    , const ExtendedQualName::SpecialTableType type
                    , const NAColumnArray &colArray
                    , Int64 &statsTime
                    , NABoolean &allFakeStat
                    , const NABoolean preFetch
                    , NABoolean *fakeHistogram
                    , NABoolean *emptyHistogram
                    , NABoolean *smallSampleHistogram
                    , double    *smallSampleSize
                    , Lng32      *colmap
                    , double    &histogramRowCount
                    , HSColStats *cs
                    , Lng32      offset
                    )
{
  Lng32 retcode = 0;  // only used for internally called functions.
                     // readHistograms always returns 0.
  // -----------------------------------------------------------------------
  // Get real histograms for normal tables except histogram tables
  // (in particular, get only fake histograms for CorrName::VIRTUAL_TABLEs
  // such as DESCRIBE__, EXPLAIN__, PACKED__ from the Binder).
  // We also do not get real histograms when doing a bootstrap compile --
  // that is to build our SQL modules for accessing SMD tables & rforks
  // -----------------------------------------------------------------------
  HSLogMan *LM = HSLogMan::Instance();
  ULng32 maxHistid = 0;

  if (NOT specialTable)
    {
      // histogram versioning, only apply to MX
      if (tabDef->getObjectFormat() == SQLMX)
        {
          HSGlobalsClass::schemaVersion = colArray[0]->getNATable()->getObjectSchemaVersion();
          if (HSGlobalsClass::schemaVersion == COM_VERS_UNKNOWN)
            {
              LM->Log("\nFetchHistograms: Unable to get table schema version.");
              return -1;
            }
          HSGlobalsClass::autoInterval = CmpCommon::getDefaultLong(USTAT_AUTOMATION_INTERVAL);
          if (LM->LogNeeded())
           {
             sprintf(LM->msg, "\nFetchHistograms: TABLE: %s; SCHEMA VERSION: %d; AUTOMATION INTERVAL: %d\n", 
                      fullQualName.data(),
                      HSGlobalsClass::schemaVersion, 
                      HSGlobalsClass::autoInterval);
              LM->Log(LM->msg);
           }
        }

      NAString histogramsName = histogramTableName; // It points to Histogram or Histints table name. soln#:10-030910-9505 
      histogramRowCount = -1;

      ULng32 savedParserFlags = 0;
      SQL_EXEC_GetParserFlagsForExSqlComp_Internal(savedParserFlags);

      // save the statement heap from the current context so it can be used to allocate memory
      // for the cursor attributes
      NAHeap *curStmtHeap = STMTHEAP;
      NABoolean switched = FALSE;
      CmpContext* prevContext = CmpCommon::context();

      try
      {
         // set parserflag to avoid privilege checks
         SQL_EXEC_SetParserFlagsForExSqlComp_Internal(INTERNAL_QUERY_FROM_EXEUTIL);
      
         // switch to another context to avoid spawning an arkcmp process when compiling
         // the user metadata queries on the histograms tables
         if (IdentifyMyself::GetMyName() == I_AM_EMBEDDED_SQL_COMPILER)
         {
            if (SQL_EXEC_SWITCH_TO_COMPILER_TYPE(CmpContextInfo::CMPCONTEXT_TYPE_META))
            {
               //failed to switch/create MD CmpContext, continue using current CmpContext
            }
            else
            {
               switched = TRUE;
            }
         }

         HSHistogrmCursor cursor(fullQualName /*in*/,
                                 histogramTableName /*in*/,
                                 tabDef->getObjectUID() /*in*/,
                                 tabDef->getObjectFormat()  /*in*/,
                                 (HSGlobalsClass::autoInterval > 0 && /*in*/
                                  (!tabDef->isVolatile() || 
                                   CmpCommon::getDefault(USTAT_AUTO_FOR_VOLATILE_TABLES) == DF_ON)),
                                   curStmtHeap /*in*/);
         if ((retcode = cursor.open()) == 0)
           {
             HSHistintsCursor cursor2(fullQualName /*in*/,
                                      histintsTableName /*in*/,
                                      tabDef->getObjectUID() /*in*/,
                                      tabDef->getObjectFormat() /*in*/,
                                      curStmtHeap /*in*/);
             histogramsName = histintsTableName;
             LM->LogTimeDiff("START FETCH EXISTING HISTOGRAMS");
             retcode = cursor.fetch( *cs
                                   , cursor2
                                   , colmap
                                   , fakeHistogram
                                   , emptyHistogram
                                   , smallSampleHistogram
                                   , smallSampleSize
                                   , histogramRowCount
                                   , statsTime
                                   , allFakeStat
                                   , preFetch
                                   , offset
                                   , tabDef
                                   , switched
                                   );
             LM->LogTimeDiff("END FETCH EXISTING HISTOGRAMS");
             maxHistid = cursor.getMaxHistid();
           }
          else
           {
              // switch back to previous compiler context in case cursor.open above failed
              // if open succeeds, we switch back in the cursor.fetch method above
              if (switched == TRUE)
                  SQL_EXEC_SWITCH_BACK_COMPILER();
           }
         }
         catch (...)
         {
            // Restore parser flags settings to what they originally were
            SQL_EXEC_ResetParserFlagsForExSqlComp_Internal(savedParserFlags);
            throw;
         }

      SQL_EXEC_ResetParserFlagsForExSqlComp_Internal(savedParserFlags);
    }
    return 0;
}



// -----------------------------------------------------------------------
// Constructor and destructor for the first cursor.
// -----------------------------------------------------------------------
HSHistogrmCursor::HSHistogrmCursor
                   ( const char *tablename
                   , const char *histogramTableName
                   , ComUID tableUID
                   , ComDiskFileFormat fileType
                   , NABoolean updatable
                   , NAHeap *heap
                   )
 : heap_(heap),
   validCursor_(FALSE),
   histogramTableName_(histogramTableName),
   fileType_(fileType),
   tableUID_(tableUID),
   histid_(0),
   updatable_(updatable),

   //Allocate a large chunk of storage and split it up between low and high
   //values, leaving the first 2bytes available for the length of each value.
   // |-----|------------|-----|------------|
   // | len | low value  | len | high value |
   // |-----|------------|-----|------------|
   buf1_(new(heap) NAWchar[HS_MAX_BOUNDARY_LEN * 2 + 2]),
   buf2_(&buf1_[HS_MAX_BOUNDARY_LEN + 1]),
//#ifndef SQ_LINUX
   lowval_(&buf1_[1]), highval_(&buf2_[1]),
//#endif
   totalRowCount_(-1),
   correctedRowCount_(-1),
   totalUec_(-1),
   statsTime_(0),
   maxStatsTime_(0),
   adjRowCount_(FALSE),
   desc_(NULL),
   update_retcode_(0),
   retcode_(0),
   cursor101_(NULL),

   // the following are columns for automation
   readTime_(0),
   readCount_(0),
   sampleSecs_(0),
   colSecs_(0),
   samplePercent_(0),
   cv_(0),
   reason_(HS_REASON_EMPTY),
   v1_(0),
   avgVarCharCount_(0),
   // v3_(0),v4_(0),v5_(&buf3_[1]), v6_(&buf4_[1])
   maxHistid_(0)

{
  *readTime_timestamp0_ = '\0';
  validCursor_ = TRUE;
}

HSHistogrmCursor::~HSHistogrmCursor()
{
  NADELETEBASIC(buf1_, heap_);
  buf1_ = NULL;

  if (desc_)
    {
      HSTranMan *TM = HSTranMan::Instance();
      SQL_EXEC_ClearDiagnostics(desc_);
      Lng32 retcode = SQL_EXEC_CloseStmt(desc_);
      if (retcode)
        HSLogError(retcode);
      SQL_EXEC_ClearDiagnostics(desc_);

      delete (char*)(desc_->identifier);
      delete desc_;
      desc_ = NULL;
    }

  delete cursor101_;
}

// -----------------------------------------------------------------------
// Fetch rows until EOF using
// SELECT HISTOGRAM_ID, COLUMN_NUMBER, COLCOUNT,
//        INTERVAL_COUNT, ROWCOUNT, TOTAL_UEC,
//        LOW_VALUE, HIGH_VALUE
// FROM   HISTOGRAMS
// WHERE  TABLE_UID = :"tableUID" AND INTERVAL_COUNT > 0;
//
// **Assumes that rows return sorted by HISTOGRAM_ID ascending; see [PK] below.
//
// MV
// The offset parameter shifts the column number read from the histogram.
// Used for tmp trigger table.
// -----------------------------------------------------------------------
Lng32 HSHistogrmCursor::fetch( HSColStats &cs
                            , HSHistintsCursor &cursor2
                            , Lng32 *colmap
                            , NABoolean *fakeHistogram
                            , NABoolean *emptyHistogram
                            , NABoolean *smallSampleHistogram
                            , double *smallSampleSize
                            , double &fakeRowCount
                            , Int64 &statsTime
                            , NABoolean &allFakeStats  // OPT
                            , const NABoolean preFetch
                            , Lng32 offset // MV
                            , HSTableDef *tabDef
                            , NABoolean cmpContextSwitched
                            )
{
  Int32 i = 0; // index for multi-column histograms
  ULng32 newHistid = 0;
  Int32 numHistograms = 0;
  NABoolean updateReadTime = FALSE, continueAfterReadTimeCheck = FALSE;
  HSLogMan *LM = HSLogMan::Instance();
  HSTranMan *TM = HSTranMan::Instance();  // Do not reset, this is not an entry point.
  NABoolean nonExistentColumnWarningIssued = FALSE;

  // These are local variables, not in HSColStats class, as their state is
  // really local to this loop.  They are distinct from the booleans in
  // the fakeHistograms loop earlier.
  NABoolean needHistints = FALSE, needHistogram = FALSE;

  // prepare and open the cursor for the histInt query
  NABoolean cursor2Failed = (cursor2.open() != 0);

  // switch back to previous compiler context
  if (cmpContextSwitched == TRUE)
      SQL_EXEC_SWITCH_BACK_COMPILER();

  // If it weren't for the possibility of multi-column histograms,
  // we could stop fetching rows as soon as we'd processed the n'th
  // "needHistogram" row (with our caller precalculating "n" by applying
  // the "needH*" rules over the colArray).

  retcode_ = get();

  // may get HS_EOF if there isn't any histogram
  if (retcode_)
    return retcode_;


  do
    {
      // Note that CURSOR101_MX_2300 has been changed to not read empty
      // histograms.  So, this code could be removed.
      // if it is an empty histogram for automation (it has been needed before),
      // flag it, so it will not be added again
      if ( reason_ == HS_REASON_EMPTY )
        {
          if (colCount_ == 1) // an empty histogram exists for this single column histogram
            emptyHistogram[tableColNum_] = TRUE;
          continue;
        }
      // update the latest statsTime only when it is not an empty histogram
      if (statsTime < statsTime_) statsTime = statsTime_;

      // if this is a small sample histogram and not multi-column, 
      // set flag and size.
      if (reason_ == HS_REASON_SMALL_SAMPLE && colCount_ == 1)
      {
        smallSampleHistogram[tableColNum_] = TRUE;
        smallSampleSize[tableColNum_] = (double) v1_;  // Col V1 holds sample size.
      }

      // assume the results are ordered by histogram_id
      continueAfterReadTimeCheck = FALSE;
      if (newHistid == histid_)
        {
          if (i == 0) continue;
        }
      else
        {
          newHistid = histid_;
          i = 0;
          needHistints = needHistogram = FALSE;
          if (totalRowCount_ == 0) continueAfterReadTimeCheck = TRUE;
        }

      tableColNum_ += offset; // Align columns. See comment above.
      colmap[i] = tableColNum_;

      if (tableColNum_ >= cs.colArray().entries())
        {
          // We have encountered a histogram for a column that does not exist.
          // This can happen if a Hive table has histograms, and then is dropped
          // and recreated with fewer columns outside of Trafodion. It could
          // also happen if someone foolishly, maliciously or otherwise 
          // manually modifies the SB_HISTOGRAMS.COLUMN_NUMBER column. Ignore
          // this histogram, but also raise a warning.
          ComDiagsArea *ptrDiags = CmpCommon::diags();
          if (ptrDiags && !nonExistentColumnWarningIssued)
            {
              nonExistentColumnWarningIssued = TRUE;  // just issue the warning once
              *ptrDiags << DgSqlCode(UERR_WARNING_NONEXISTENT_COLUMN)
                 << DgString0(tabDef->getObjectFullName().data());
            }
          continue;
        }
      const NAColumn *nacol = cs.colArray()[tableColNum_];
      if (preFetch || nacol->needHistogram())
        needHistogram = TRUE;
	  if (preFetch || nacol->needFullHistogram() ||
          (CURRSTMT_OPTDEFAULTS->cacheHistograms() && nacol->needHistogram()))
        needHistints = TRUE;

      // Automation 3360.3.1; When a histogram is requested by the optimizer that already exists,
      // automation is on and READ_TIME is more than CACHE_HISTOGRAM_REFRESH_INTERVAL -1 seconds
      // old, set READ_TIME to the current time, and increment READ_COUNT (upto a value of SHRT_MAX).
      LM->Log("While Fetching Histograms: Check for update of READ_TIME/READ_COUNT.");
      if (updatable_ && nacol->needFullHistogram())
        {
          Int64 readTm = hs_getEpochTime(readTime_);
          Int64 readCountUpdateInterval =
            CmpCommon::getDefaultLong(USTAT_AUTO_READTIME_UPDATE_INTERVAL);

          Int64 currentTime = hs_getEpochTime();
          if (LM->LogNeeded())
            {
              sprintf(LM->msg, "Checking READ_TIME of column %s", nacol->getColName().data());
              LM->Log(LM->msg);
              sprintf(LM->msg,
                "USTAT_AUTO_READTIME_UPDATE_INTERVAL=" PF64
                ", currentTime=" PF64
                ", readTime=" PF64 "\n",
                readCountUpdateInterval, currentTime, readTm);
              LM->Log(LM->msg);
            }
          // When READ_TIME for the histogram is more than USTAT_AUTO_READTIME_UPDATE_INTERVAL-1
          // seconds old or when readTm has not been set, (0001-01-01 00:00:00), update
          // READ_TIME and READ_COUNT.
          if (readTm == 0
              ||  readTm < currentTime - readCountUpdateInterval)
            {
              updateReadTime = TRUE;
            }
          else LM->Log("Not updating read time.");
        }
      // check if empty table.
      if (continueAfterReadTimeCheck == TRUE) continue;

      // only the single column histogram or 
      // the last one of MC histograms will go on
      if (++i < colCount_)
        continue;

      try
        {
          // Histograms need to be generated for only those columns that are key columns
          // or have been marked as "Referenced for Histograms" by binder. This change has
          // been added to reduce histogram-related memory usage.
          if(needHistogram)
          {
            cs.addHistogram( histid_
                           , colCount_
                           , colmap
                           , intCount_
                           , totalRowCount_
                           , totalUec_ // might change by this call to be == totalRowCount_
                           , avgVarCharCount_
                           , lowval_
                           , highval_
                           , reason_ == HS_REASON_SMALL_SAMPLE
                           , FALSE
                           );
          }

          // Add the lowval interval
          cs.addHistint(needHistints, 0, lowval_, 0, 0, 0, 0, 0, L"()");

          if ((cursor2Failed) || (!needHistints))
            cs.addHistint(needHistints, 1, highval_, totalRowCount_, totalUec_, 0, 0, 0, L"()");
          else if (needHistints)
            //
            // If the last bunch of histograms are for unreferenced columns,
            // we will avoid doing the last bunch of SQL_EXEC_Fetch calls,
            // hopefully a nice little savings.
            //
            retcode_ = cursor2.fetch( cs
                                    , needHistints
                                    , histid_
                                    , totalRowCount_
                                    , totalUec_
                                    , highval_
                                    );

          // Turn off a flag for single columns that have histogram data.
          if (colCount_ == 1)  
            {
              // the histogram for this column has been found
              // will not need to generate a fake histogram.
              fakeHistogram[tableColNum_] = FALSE;
              allFakeStats = FALSE;
            }
 
          numHistograms++;

        }
      catch (CmpInternalException&)
        {
          //No need to handle exception because default histograms will be
          //provided.
        }
      catch (...)
        {
          LM->Log("INTERNAL ERROR (HSHistogrmCursor::fetch):");
          sprintf(LM->msg, "Failure in HSHistogrmCursor::fetch.");
          LM->Log(LM->msg);
          *CmpCommon::diags() << DgSqlCode(-UERR_GENERIC_ERROR)
                              << DgString0("fetch")
                              << DgString1("N/A")
                              << DgString2(LM->msg);
          throw;
        }
    }
  while ((retcode_ = get()) == 0);

  if (adjRowCount_ == TRUE)
      cs.normalize(CostScalar(correctedRowCount_));

  fakeRowCount = correctedRowCount_;
  TotalHistogramDBG += numHistograms;

  if (updateReadTime)
  {
    // Update requested histograms' READ_TIME and READ_COUNT entries using mechanism
    // employed by FetchStatsTime().
    HSStatsTimeCursor cursor3(histogramTableName_,
                              histogramTableName_,
                              tableUID_,
                              fileType_,
                              updatable_);
    if (NOT cursor3.open(TRUE)) cursor3.update(cs.colArray());
  }

  return 0;
}

// -----------------------------------------------------------------------
// Get one qualified row.
// -----------------------------------------------------------------------
Lng32 HSHistogrmCursor::get()
{
  Int64 tempRowCount;
  Int64 tempUEC;
  Int64 vCharSize;

  SQL_EXEC_ClearDiagnostics(desc_);

  retcode_ = SQL_EXEC_Fetch(cursor101_->getStmt(), 
                            cursor101_->getOutDesc(), 18,
                            (void *)&histid_, NULL,
                            (void *)&tableColNum_, NULL,
                            (void *)&colCount_, NULL,
                            (void *)&intCount_, NULL,
                            (void *)&tempRowCount, NULL,
                            (void *)&tempUEC, NULL,
                            (void *)&statsTime_, NULL,
                            (void *)buf1_, NULL,
                            (void *)buf2_, NULL,
                            (void *)&readTime_, NULL,
                            // readTime_timestamp0_ is absent here
                            (void *)&readCount_, NULL,
                            (void *)&sampleSecs_, NULL,
                            (void *)&colSecs_, NULL,
                            (void *)&samplePercent_, NULL,
                            (void *)&cv_, NULL,
                            (void *)&reason_, NULL,
                            (void *)&v1_, NULL,
                            (void *)&vCharSize, NULL
                            //(void *)&v3_, NULL,
                            //(void *)&v4_, NULL,
                            //(void *)buf3_, NULL,
                            //(void *)buf4_, NULL
                            );

  if (retcode_ < 0)
    {
      HSFuncMergeDiags(- UERR_INTERNAL_ERROR, "HSHistogrmCursor::get()", NULL, TRUE);
      retcode_ = -1;
      return retcode_;
    }
  else if (retcode_ == 100)
    return retcode_;

  maxHistid_ = MAXOF(maxHistid_, histid_);     

  fetchCount_++;
  totalRowCount_ = (double)tempRowCount;
  totalUec_ = (double)tempUEC;
  avgVarCharCount_ = (double)vCharSize;

  //The first 2bytes of each buffer contains the length of each value. Since
  //the data is retrieved in UCS2, each character is 2 bytes each. Hence, we
  //divide the length by 2 to get the actual number of characters and NULL
  //terminate the buffer.
//#ifndef SQ_LINUX
  lowval_[buf1_[0]/2] = '\0';
  highval_[buf2_[0]/2] = '\0';
//#else
//  lowval_ [((short)buf1_[0])/sizeof(NAWchar)] = '\0';
//  highval_[((short)buf2_[0])/sizeof(NAWchar)] = '\0';
//#endif

  // If the current row count and the corrected one don't agree (and corrected
  // is initialized), set the flag to adjust row count for all histograms.
  if (correctedRowCount_ != -1 &&
      correctedRowCount_ != totalRowCount_) adjRowCount_ = TRUE;

  // Check to see if the time that statistics were created for this histogram
  // is greater than other histograms.
  if (statsTime_ > maxStatsTime_)
  {
    // If maxStatsTime is 0, then it and correctedRowCount_ will be
    // initialized here.  In any case, the row count of the histogram
    // with the latest time will be used for correctedRowCount_.
    correctedRowCount_ = totalRowCount_;
    maxStatsTime_      = statsTime_;
  }
  return 0;
}

Lng32 HSHistogrmCursor::open()
{
  char sbuf[25];
  NABoolean needMCStats = (CmpCommon::getDefault(HIST_MC_STATS_NEEDED) == DF_ON);
  NAString qry = "SELECT HISTOGRAM_ID, COLUMN_NUMBER, COLCOUNT, INTERVAL_COUNT, "
                        "ROWCOUNT, TOTAL_UEC, JULIANTIMESTAMP(STATS_TIME), "
                        "LOW_VALUE, HIGH_VALUE, JULIANTIMESTAMP(READ_TIME), "
                        "READ_COUNT, SAMPLE_SECS, COL_SECS, SAMPLE_PERCENT, CV, "
                        "REASON, V1, V2 ";
  qry.append(" FROM ");
  qry.append(histogramTableName_);
  qry.append(" WHERE TABLE_UID = ");
  sprintf(sbuf, PF64, tableUID_.get_value());
  qry.append(sbuf);
  qry.append(" AND REASON != ' '");
  if (!needMCStats)
    qry.append(" AND COLCOUNT = 1");
  qry.append(" ORDER BY TABLE_UID, HISTOGRAM_ID, COL_POSITION");
  qry.append(" FOR READ UNCOMMITTED ACCESS");

  if (cursor101_)
    delete cursor101_;
  cursor101_ = new HSCursor(heap_); 
  Lng32 retcode = cursor101_->prepareQuery(qry.data(), 0, 18);
  HSHandleError(retcode);

  retcode = cursor101_->open();
  HSHandleError(retcode);
  return retcode;
}


/******************************************************************************/
/*                             updateHistogram()                              */
/* FUNCTION   Updates the READ_TIME of the the HISTOGRAM table at the current */
/*            cursor position using statement UPD104_MX_2300 or UPD106_MX_2300*/
/*            NOTE: stmt and ivar_stmt must point to static character arrays. */
/* RETCODE    Success: (0)                                                    */
/*                                                                            */
/******************************************************************************/
Lng32 updateHistogram(const char *histogramTableName, Int32 stmtNum, short readCount)
{
  HSLogMan* LM = HSLogMan::Instance();
  NAString catName(histogramTableName, strcspn(histogramTableName, "."));
  char time_str[HS_TIMESTAMP_SIZE];
  hs_formatTimestamp(time_str);      // current time
  char count_str[10];
  sprintf(count_str, "%d", readCount);

  NAString qry = "UPDATE ";
  qry.append(histogramTableName);
  qry.append(" SET READ_TIME=");
  qry.append(time_str);
  qry.append(", READ_COUNT=");
  qry.append(count_str);
  qry.append(" WHERE CURRENT OF CURSOR106_MX_2300");

  // Note that the UPDATE cannot be retried since the transaction will
  // likely abort on any failure, invalidating the cursor.
  Lng32 retcode = HSFuncExecQuery(qry.data(), -UERR_INTERNAL_ERROR, NULL,
                                  HS_QUERY_ERROR, NULL, NULL);
  HSFilterWarning(retcode);
  if (LM->LogNeeded())
  {
    if (retcode)
      sprintf(LM->msg, "updateHistogram: ***[FAIL=%d]Unable to update read count/time", retcode);
    else
      sprintf(LM->msg, "updateHistogram: READ_COUNT/TIME updated; %d, %s", readCount, time_str);
    LM->Log(LM->msg);
  }

  return (retcode);
}

// -----------------------------------------------------------------------
// Constructor and destructor for the second cursor.
// -----------------------------------------------------------------------
HSHistintsCursor::HSHistintsCursor
                    ( const char *tablename
                    , const char *histintsTableName
                    , const ComUID tableUID
                    , const ComDiskFileFormat fileType
                    , NAHeap *heap
                    )
 : validCursor_(FALSE),
   heap_(heap),
   histintsTableName_(histintsTableName),
   tableUID_(tableUID),
   fileType_(fileType),
   histid_(0),

   //Allocate a chunk of storage for the boundary value, leaving the first
   //2bytes available for the length of the buffer.
   //   |--------|-----------------|
   //   | length | boundary value  |
   //   |--------|-----------------|
   buf_(new(heap) NAWchar[ HS_MAX_BOUNDARY_LEN * 2 + 2 ]),
   buf_mfv_(&buf_[HS_MAX_BOUNDARY_LEN + 1]),
   //buf_v6_(&buf_v5_[HS_MAX_BOUNDARY_LEN + 1]),

   boundary_(&buf_[1]),
   mfv_(&buf_mfv_[1]),
   //v6_(&buf_v6_[1]),

   rowCount_(-1),
   uec_(-1),
   intNum_(0),
   desc_(NULL),
   retcode_(0),
   cursor201_(NULL),

   stdDevOfFreq_(0),
   mfvRowCnt_(0), mfv2RowCnt_(0)
   //v3_(0), v4_(0)
{
//#ifdef SQ_LINUX
//  char *valPtr = (char *) &buf_[0];
//  valPtr += 2;
//  boundary_ = (NAWchar *) valPtr;
//#endif
  validCursor_ = TRUE;
}

HSHistintsCursor::~HSHistintsCursor()
{
  NADELETEBASIC(buf_, heap_);
  buf_ = NULL;
  if (desc_)
    {
      SQL_EXEC_ClearDiagnostics(desc_);
      Lng32 retcode = SQL_EXEC_CloseStmt(desc_);
      if (retcode)
        HSLogError(retcode);
      SQL_EXEC_ClearDiagnostics(desc_);

      delete (char*)(desc_->identifier);
      delete desc_;
      desc_ = NULL;
    }

  delete cursor201_;
}

Lng32 HSHistintsCursor::open()
{
  char sbuf[25];
  NAString qry = "SELECT HISTOGRAM_ID, INTERVAL_NUMBER, INTERVAL_ROWCOUNT, INTERVAL_UEC, INTERVAL_BOUNDARY, "
                 "CAST(STD_DEV_OF_FREQ AS DOUBLE PRECISION), V1, V2, V5 "
                 "FROM ";
  qry.append(histintsTableName_);
  qry.append(    " WHERE TABLE_UID = ");
  sprintf(sbuf, PF64, tableUID_.get_value());
  qry.append(sbuf);
  qry.append(    " ORDER BY TABLE_UID, HISTOGRAM_ID, INTERVAL_NUMBER");
  qry.append(    " FOR READ UNCOMMITTED ACCESS");

  if (cursor201_)
    delete cursor201_;
  cursor201_ = new HSCursor(heap_, "HIST_INTS"); 
  Lng32 retcode = cursor201_->prepareQuery(qry.data(), 0, 9);
  HSHandleError(retcode);

  retcode = cursor201_->open();
  HSHandleError(retcode);
  return retcode;
}

// -----------------------------------------------------------------------
// Fetch interval rows and set the ColStats object.
//
// Fetch rows until EOF using
// SELECT HISTOGRAM_ID,
//        INTERVAL_NUMBER,
//        INTERVAL_ROWCOUNT,
//        INTERVAL_UEC,
//        INTERVAL_BOUNDARY
// FROM   HISTOGRAM_INTERVALS
// WHERE  TABLE_UID = :"tableUID" AND INTERVAL_ROWCOUNT >= 0 AND
//        INTERVAL_UEC >= 0;
//
// **Assumes that rows return sorted by HISTOGRAM_ID ascending; see [PK] below.
//
// -----------------------------------------------------------------------
Lng32 HSHistintsCursor::fetch
                   ( HSColStats &cs
                   , const NABoolean needHistints
                   , const ULng32 histid
                   , const double totalRowCount
                   , const double totalUec
                   , const NAWchar *highval
                   )
{
  double sumRowCount = 0;
  double sumUec      = 0;
  Int32 i = 0;
  HSLogMan *LM = HSLogMan::Instance();

  if ( cs.isUnique() )
    CMPASSERT (totalUec == totalRowCount) ;

  while ((retcode_ = get(histid, ++i)) == 0)
  {
    sumRowCount += rowCount_ ;
    sumUec      += uec_ ;
    cs.addHistint(needHistints, i, boundary_, rowCount_, uec_,
                  stdDevOfFreq_, mfvRowCnt_, mfv2RowCnt_, mfv_);
  }

  // now the cleanup, if necessary

  rowCount_ = totalRowCount - sumRowCount;
  uec_ = totalUec - sumUec;
  if (rowCount_ > 0 &&
      uec_      > 0 &&
      intNum_ < cs.lastInt())
    {
      // add one extra interval to make up the difference
      if (LM->LogNeeded())
        {
          sprintf(LM->msg, "HSREAD: Extra Interval Added (HISTID=%u)", histid);
          LM->Log(LM->msg);
        }

      if (rowCount_ < uec_)
        uec_ = rowCount_;

        cs.addHistint(needHistints, i, (NAWchar*)highval, rowCount_, uec_,
                      0, 0, 0, L"()");
    }
  else if (rowCount_ || uec_)
    {
      // adjust redFactors to make up the difference.
      if (sumRowCount>0 && sumUec>0)
        cs.fixRedFactor(totalRowCount/sumRowCount,
                        totalUec/sumUec);
    }

  return retcode_;
}

// -----------------------------------------------------------------------
// Get one qualified row with matching histid.
//
// [PK]:
// **Assumes that rows return sorted by HISTOGRAM_ID ascending** --
// which should be correct, given that the histogram tables are defined
// with PRIMARY KEY including this column ...
// -----------------------------------------------------------------------
Lng32 HSHistintsCursor::get( const ULng32 histid
                          , const Lng32 i
                          )
{
  Int64 tempIntRowCount;
  Int64 tempIntUec;
  Int64 mfvRowCnt;
  Int64 mfv2RowCnt;

  while (histid_ <= histid)     // assumes rows are sorted! [PK]
    {
      intNum_++;
      SQL_EXEC_ClearDiagnostics(desc_);

      retcode_ = SQL_EXEC_Fetch(cursor201_->getStmt(), 
                                 cursor201_->getOutDesc(), 9,
                                 (void *)&histid_, NULL,
                                 (void *)&short1_, NULL,
                                 (void *)&tempIntRowCount, NULL,
                                 (void *)&tempIntUec, NULL,
                                 (void *)buf_, NULL,
                                 // the following are added in R2.3
                                 (void *)&stdDevOfFreq_, NULL,
                                 (void *)&mfvRowCnt, NULL,
                                 (void *)&mfv2RowCnt, NULL,
                                 //(void *)&v3_, NULL,
                                 //(void *)&v4_, NULL,
                                 (void *)buf_mfv_, NULL
                                 //(void *)buf_v6_, NULL
                                 );

      if (retcode_ && retcode_ != HS_EOF)
        HSLogError(retcode_);
      if (retcode_)
        {
          if (retcode_ < 0)
            {
              HSFuncMergeDiags(-UERR_INTERNAL_ERROR, "HSHistintsCursor::get()",
                               NULL, TRUE);
              return -1;
            }
          else
            return retcode_;
        }
      fetchCount_++;

      //added short1_ !=0 to skip the interval num zero
      //since interval num zero has already been added
      if ((histid_ == histid)&&(short1_!=0))
                break;
    }
  if (histid_ > histid)
    {
      // This is the lowval interval for the following histid.
      // It's thrown away here but its equivalent is added in
      // the loop in HSHistogrmCursor::fetch().
      return HS_EOF;
    }
  intNum_ = short1_;
  //The first 2bytes of the boundary buffer contains the length of the value.
  //Since data is retrieved in UCS2, each character is 2 bytes each. Hence, we
  //divide the length by 2 to get the actual number of characters and NULL
  //terminate the buffer.
//#ifndef SQ_LINUX
  boundary_[buf_[0]/2] = '\0';
//#else
//  boundary_[((short)buf_[0])/sizeof(NAWchar)] = '\0';
//#endif
  rowCount_ = (double)tempIntRowCount;
  uec_ = (double)tempIntUec;
  mfvRowCnt_ = (double)mfvRowCnt;
  mfv2RowCnt_ = (double)mfv2RowCnt;

  mfv_[buf_mfv_[0]/2] = '\0';
  //v6_[buf_v6_[0]/2] = '\0';

  return 0;
}

// -----------------------------------------------------------------------
// Open a cursor using a static module file.
// -----------------------------------------------------------------------
Lng32 OpenCursor( const char *descID
               , void * param1Addr
               , void * param2Addr
               , SQLDESC_ID *&desc
               )
{
  static SQLMODULE_ID module;
  init_SQLMODULE_ID(&module);
  static char moduleName[HS_MODULE_LENGTH];
  Lng32 retcode;
  HSLogMan *LM = HSLogMan::Instance();

  strncpy(moduleName, HS_MODULE, HS_MODULE_LENGTH);
  module.module_name = (char*)moduleName;
  module.module_name_len = strlen((char*)moduleName);
  module.creation_timestamp = 1234567890;

  if (desc)
    {
      SQL_EXEC_ClearDiagnostics(desc);
      retcode = SQL_EXEC_CloseStmt(desc);
      if (retcode)
        HSLogError(retcode);
      SQL_EXEC_ClearDiagnostics(desc);

      delete (char*)(desc->identifier);
      delete desc;
      desc = NULL;
    }

  desc = new SQLDESC_ID;
  init_SQLCLI_OBJ_ID(desc);

  desc->name_mode = cursor_name;
  desc->module = &module;
  desc->identifier = new char[HS_STMTID_LENGTH];
  desc->handle     = 0;

  sprintf((char*)desc->identifier, descID);
  desc->identifier_len = strlen(descID);

  SQL_EXEC_ClearDiagnostics(desc);
  retcode = SQL_EXEC_Exec(desc, NULL, 2, param1Addr, NULL,
                                         param2Addr, NULL);
  if (retcode)
    HSFuncMergeDiags(-UERR_INTERNAL_ERROR, "OpenCursor", NULL, TRUE);
  if (retcode < 0 && LM->LogNeeded())
    {
      sprintf(LM->msg, "HSREAD: ***[FAIL=%d]Unable to load module %s", retcode, descID);
      LM->Log(LM->msg);
    }


  return ((retcode < 0) ? -1 : retcode);
}

// -----------------------------------------------------------------------
// Initialize ColStats for a column group.
// -----------------------------------------------------------------------
void HSColStats::addHistogram
                 ( const ULng32 histid
                 , const Lng32 colCount
                 , const Lng32 *colmap
                 , const Lng32 intCount
                 , const double totalRowCount
                 , double & totalUec
                 , const double vCharSize
                 , const NAWchar *lowval
                 , const NAWchar *highval
                 , NABoolean isSmallSample
                 , NABoolean isFakeHistogram
                 )
{
  HSLogMan *LM = HSLogMan::Instance();
  // now determine: is this a single-column primary key?  If so, set the isUnique_ flag
  // appropriately
  const NAColumn * nac  = colArray_.getColumn(colmap[0]) ;

  NABoolean isUnique = FALSE ;

  if ( nac->isPrimaryKey() )
    {
      const NATable       * nat  = nac->getNATable() ;
      const NAColumnArray & naca = nat->getNAColumnArray() ;
      // $$$NB: it might be possible to use colArray_ instead of naca ... set
      // up an assertion to test this idea ...

      CollIndex primaryKeyCount = 0 ;
      for ( CollIndex i = 0 ; i < naca.entries() ; i++ )
        {
          // if there's another column marked "primary key",
          // then this isn't a single-column primary key, so don't
          // mark it as Unique
          if ( naca[i]->isPrimaryKey() )
            {
              primaryKeyCount++ ;
            }
        }
      if ( primaryKeyCount == 1 )
        {
          isUnique = TRUE ;
          totalUec = totalRowCount ;
        }
    }
  ComUID id((long)histid);
  colStats_ = ColStatsSharedPtr( CMPNEW ColStats(id,
                              totalUec, /* possibly changed by the uniqueness determination above*/
                              (CostScalar) totalRowCount,
        // baseRowCount=totalRowCount initially. Added 12/01 RV
                              (CostScalar) totalRowCount,
                              isUnique, FALSE, 0, FALSE,
                              1.0, 1.0,
                              (csZero <= vCharSize) ? UInt32(vCharSize) : 0,
                              heap_));

  colStats_->setFakeHistogram(isFakeHistogram);
  colStats_->setOrigFakeHist(isFakeHistogram);

  lastInt_ = intCount;
  if (colCount == 1)
    {
      const NAType *pType = colArray_.getColumn(colmap[0])->getType();
      if (pType->supportsSQLnull())
        lastInt_ = intCount - 1;
    }


  // Build the NAColumnArray consisting of histogram columns.
  for (Int32 i = 0; i < colCount; i++)
    {
      NAColumn *tableColumn = colArray_.getColumn(colmap[i]);
      colStats_->statColumns().insert(tableColumn);
    }

  // Encode the MIN and MAX values.
  colStats_->setMinValue(lowval);
  colStats_->setMaxValue(highval);

  // Set flags indicating whether fake or small sample histograms.
  colStats_->setFakeHistogram(isFakeHistogram);
  colStats_->setSmallSampleHistogram(isSmallSample);

  hist_ = HistogramSharedPtr(CMPNEW Histogram(heap_));
  colStats_->setHistogram(hist_);       // empty now, will be added to

  colStatsList_.insert(colStats_);
  usedHistogramCount_++;

  if (LM->LogNeeded())
    {
      NAWcharBuf wLowBuf((wchar_t *)lowval,na_wcslen(lowval));
      NAWcharBuf wHiBuf((wchar_t *)highval,na_wcslen(highval));
      charBuf* isoLowVal = NULL;
      charBuf* isoHiVal = NULL;
      isoLowVal = unicodeToISO88591(wLowBuf, heap_, isoLowVal);
      isoHiVal = unicodeToISO88591(wHiBuf, heap_, isoHiVal);

      sprintf(LM->msg, "\tAdd to ColStats histogram(%s, %d, %d, %d, %f, %f, %s, %s )",
                          nac->getColName().data(),
                          histid,
                          colCount,
                          intCount,
                          totalRowCount,
                          totalUec,
                          isoLowVal->data(),
                          isoHiVal->data());
      LM->Log(LM->msg);
      NADELETEBASIC(isoLowVal, heap_);
      NADELETEBASIC(isoHiVal, heap_);
    }
}

// -----------------------------------------------------------------------
// Add a hist entry for a histogram interval.
// -----------------------------------------------------------------------
void HSColStats::addHistint( NABoolean needHistints
                           , Int32 intnum
                           , const NAWchar *boundary
                           , double rowCount
                           , double uec
                           , double stdDevOfFreq
                           , double mfv_rows
                           , double mfv2_rows
                           , const wchar_t *mfv
                           , NABoolean fakeHist
                           )
{
  HSLogMan *LM = HSLogMan::Instance();

//  if ( mfv_rows > 0 ) {
//    CMPASSERT (rowCount-uec+1 >= mfv_rows) ;

    // The lower bound of mfv_rows should be at least the average frequence.
    // We add 1 here to cover the current abnormally as demonstrated below, where MFV should be 22380!
//Number   Rowcount         UEC   MFVRowCount 2ndMFVRowCount MFV
//====== =========== ===========  =========== ============== =================================
//     0           0           0            0              0 ()
//     1       22380           1        22379              0 ('xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx')

    // Comment out because we now can reproduce the root cause of SQ3805 with optdml05. This assertion
    // should be added back when that problem is fixed.
    //CMPASSERT (mfv_rows * uec + 1 >= rowCount ) ;
    //CMPASSERT (mfv_rows >= mfv2_rows) ;
  //}

  NABoolean useHighFreq = CURRSTMT_OPTDEFAULTS->useHighFreqInfo();

  if (needHistints)
    {
      HistInt histInt(intnum,
                      boundary,
                      colStats_->getStatColumns(),
                      (CostScalar)rowCount,
                      (CostScalar)uec,
                      // stdDevOfFreq,
                      (intnum != 0),
                      mfv2_rows);
      hist_->insert(histInt);
      usedHistintsCount_++;

      if (!fakeHist &&
          (hist_->entries() > 1) &&
          (colStats_->getStatColumns().entries() == 1) &&
          (CmpCommon::getDefault(COMP_BOOL_70) == DF_ON))
      {
        Interval iter = hist_->getLastInterval() ;
        colStats_->createAndAddSkewedValue(boundary, iter);

        if ( useHighFreq && mfv && mfv[0] == L'(' ) {
          // Add the mfv_rows to the frequent value list, only when mfv
          // starts with L'('. It will prevents pre-R2.5 MFV values (
          // represented as L' ') to be added.

          // Create a temp. entry and populate it with the mfv info. Later on
          // we remove it.
          Int32 intnumMFV = intnum+1;
          HistInt histIntMFV(intnumMFV,
                      (CostScalar(uec) == csOne) ? boundary : mfv,
                      colStats_->getStatColumns(),
                      (CostScalar)mfv_rows,
                      (CostScalar)1,
                      // stdDevOfFreq,
                      (intnumMFV != 0),
                      0);
          hist_->insert(histIntMFV);
          Interval iterMFV = hist_->getLastInterval() ;
          colStats_->createAndAddFrequentValue(mfv, iterMFV);

          // remove the last entry created for inserting MFV
          hist_->removeAt(hist_->entries() - 1) ;
        }

      }

      // Store MC skew values
      if(uec == 1 && colStats_->getStatColumns().entries() > 1 )
        colStats_->addMCSkewedValue(boundary, rowCount);

      if (LM->LogNeeded())
        {
          NAWcharBuf wBoundBuf((wchar_t *)boundary,na_wcslen(boundary));
          NAWcharBuf wMfvBuf((wchar_t *)mfv, na_wcslen(mfv));
          charBuf* isoBoundVal = NULL;
          charBuf* isoMfvVal = NULL;
          isoBoundVal = unicodeToISO88591(wBoundBuf, heap_, isoBoundVal);
          isoMfvVal = unicodeToISO88591(wMfvBuf, heap_, isoMfvVal);

          sprintf(LM->msg, "\t\tinterval(%d, %f, %f, %s, %f, %f, %f, %s )",
                              intnum,
                              rowCount,
                              uec,
                              isoBoundVal->data(),
                              mfv_rows,
                              mfv2_rows,
                              stdDevOfFreq,
                              isoMfvVal->data());
          LM->Log(LM->msg);
          NADELETEBASIC(isoBoundVal, heap_);
          NADELETEBASIC(isoMfvVal, heap_);

        }
    }

  TotalHistintsDBG++;
}

// -----------------------------------------------------------------------
// Fix redFactor to distribute balance in rowcount or uec to all intervals.
// -----------------------------------------------------------------------
void HSColStats::fixRedFactor( double rowRedFactor
                             , double uecRedFactor
                             )
{
   if (rowRedFactor < uecRedFactor)
     uecRedFactor = rowRedFactor;
   if (rowRedFactor != 1)
     colStats_->setRedFactor(CostScalar(rowRedFactor));
   if (uecRedFactor != 1)
     colStats_->setUecRedFactor(CostScalar(uecRedFactor));
}

// -----------------------------------------------------------------------
// Normalize total row count and uec.
// -----------------------------------------------------------------------
void HSColStats::normalize(const CostScalar newRowCount)
{
  HSLogMan *LM = HSLogMan::Instance();
  CostScalar rowRedFactor, uecRedFactor;
  CostScalar uec;

  for (CollIndex i = 0; i < colStatsList_.entries(); i++)
    {
      colStats_ = colStatsList_[i];
  
      // an obsolete flag is set if the row count is adjusted
      if (newRowCount != colStats_->getRowcount())
        {
           colStats_->setObsoleteHistogram();
           if (LM->LogNeeded())
           {
             if (colStats_->isObsoleteHistogram())
               {
                  sprintf(LM->msg, "\n--- Obsolete flag was set for Histogram id: " PF64, 
                    colStats_->getHistogramId().get_value());
                  LM->Log(LM->msg);
               }
           }
        }

      rowRedFactor = (newRowCount / colStats_->getRowcount());
      if (colStats_->getTotalUec() / colStats_->getRowcount() > 0.5)
         uecRedFactor = rowRedFactor;
      else uecRedFactor = 1;

      //case#:10-021022-4222
      //It is possible to have rowcounts different for all histograms defined
      //for the table. This may cause the normalization of UECs to be less than
      //one, which causes an optimizer assertion. We must ensure at least one
      //UEC.
      //Basically, this is a user-error for ignoring the 9202 warning message
      //during UPDATE STATISTICS execution.
      uec = MAXOF(colStats_->getTotalUec() * uecRedFactor, 1);

      // Set the new row count and uec.  Note that this function will set the
      // UEC equal to row count if it is greator than newRowCount.
      colStats_->setRowsAndUec(newRowCount,uec);

      // Adjust row and uec reduction factor.  The reduction factors in 
      // colStats might get modified in the call to setRowsAndUec().
      rowRedFactor *= colStats_->getRedFactor();
      colStats_->setRedFactor(rowRedFactor);

      uecRedFactor *= colStats_->getUecRedFactor();
      colStats_->setUecRedFactor(uecRedFactor);
    }
}

//@ZXuec
// Looks up existing histogram information (total rows and total UEC), if any,
// for each column in groupList.
//
void getPreviousUECRatios(HSColGroupStruct *groupList)
{
  HSLogMan *LM = HSLogMan::Instance();
  HSGlobalsClass *hsGlobals = GetHSContext();
  HSColGroupStruct *group = groupList;
  Lng32 numCols = hsGlobals->objDef->getNumCols();
  Lng32 colNum;

  // Construct cursor to retrieve existing histogram info for columns of the
  // table being used.
  HSHistogrmCursor cursor(hsGlobals->objDef->getObjectFullName().data(),
                          hsGlobals->hstogram_table->data(),
                          hsGlobals->objDef->getObjectUID(),
                          hsGlobals->tableFormat,
                          FALSE,   // is not updatable
                          STMTHEAP);     

  // Stores rowcount/uec info collected from the query on the histogram table.
  struct PrevUecInfo : public NABasicObject
    {
      PrevUecInfo()
        {
          totalRows = 0;
          totalUec  = 0;
          oldAvgVarcharSize = -1;
        }
      Int64 totalRows;
      Int64 totalUec;
      Int64 oldAvgVarcharSize;
    };

  // Allocate an array big enough to hold info on every column in the table.
  PrevUecInfo* uecInfo = new (STMTHEAP) PrevUecInfo[numCols];
  
  // Read the existing histogram info for each of the table's columns and store
  // it in an array indexed by column number. The columns in 'group' will be
  // looked up in this array.
  Lng32 retcode = cursor.open();
  while (!retcode)
    {
      retcode = cursor.get();
      if (!retcode && cursor.colCount_ == 1)  // single-col groups only
        {
          uecInfo[cursor.tableColNum_].totalRows = (Int64)cursor.totalRowCount_;
          uecInfo[cursor.tableColNum_].totalUec = (Int64)cursor.totalUec_;
          // the avgVarCharCount_ (V2 column in SB_HISTOGRAMS) is 100 times the
          // average varchar length
          if (cursor.avgVarCharCount_ > 0)  // if the column is set
            uecInfo[cursor.tableColNum_].oldAvgVarcharSize = 
              (Int64)(cursor.avgVarCharCount_+99)/100;
        }
    }

  // Iterate through the group list, find the corresponding histogram info in the
  // array we built when reading the results of the query on the histogram table,
  // and store the info in the HSColGroupStruct for the column.
  while (group)
    {
      colNum = group->colSet[0].colnum;
      group->prevRowCount = uecInfo[colNum].totalRows;
      group->prevUEC = uecInfo[colNum].totalUec;
      group->oldAvgVarCharSize = uecInfo[colNum].oldAvgVarcharSize;
      if (LM->LogNeeded())
        {
          sprintf(LM->msg, "Existing histogram for column %s: rows = " PF64 ", UEC = " PF64 ", avgVarCharSize = %f",
                  group->colSet[0].colname->data(), group->prevRowCount, group->prevUEC, group->oldAvgVarCharSize);
          LM->Log(LM->msg);
        }
      group = group->next;
    }

  //NADELETEARRAY(uecInfo, numCols, PrevUecInfo, STMTHEAP)
}
