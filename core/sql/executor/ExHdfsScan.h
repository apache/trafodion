// **********************************************************************
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
// **********************************************************************
#ifndef EX_HDFS_SCAN_H
#define EX_HDFS_SCAN_H

//
// Task Definition Block
//
#include "ComTdbHdfsScan.h"
#include "ExSimpleSqlBuffer.h"
#include "ExStats.h"
#include "sql_buffer.h"
#include "ex_queue.h"

#include "hdfs.h"

#include <time.h>
#include "ExHbaseAccess.h"
#include "ExpHbaseInterface.h"

#define HIVE_MODE_DOSFORMAT    1
#define HIVE_MODE_CONV_ERROR_TO_NULL 2

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExHdfsScanTdb;
class ExHdfsScanTcb;
#ifdef NEED_PSTATE
class ExBlockingHdfsScanPrivateState;
#endif

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;
class ExHdfsScanStats;
class SequenceFileReader;
class ExpORCinterface;

// -----------------------------------------------------------------------
// ExHdfsScanTdb
// -----------------------------------------------------------------------
class ExHdfsScanTdb : public ComTdbHdfsScan
{
  friend class ExOrcScanTdb;

public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  NA_EIDPROC ExHdfsScanTdb()
  {}

  NA_EIDPROC virtual ~ExHdfsScanTdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  NA_EIDPROC virtual ex_tcb *build(ex_globals *globals);

private:
  // ---------------------------------------------------------------------
  // !!!!!!! IMPORTANT -- NO DATA MEMBERS ALLOWED IN EXECUTOR TDB !!!!!!!!
  // *********************************************************************
  // The Executor TDB's are only used for the sole purpose of providing a
  // way to supplement the Compiler TDB's (in comexe) with methods whose
  // implementation depends on Executor objects. This is done so as to
  // decouple the Compiler from linking in Executor objects unnecessarily.
  //
  // When a Compiler generated TDB arrives at the Executor, the same data
  // image is "cast" as an Executor TDB after unpacking. Therefore, it is
  // a requirement that a Compiler TDB has the same object layout as its
  // corresponding Executor TDB. As a result of this, all Executor TDB's
  // must have absolutely NO data members, but only member functions. So,
  // if you reach here with an intention to add data members to a TDB, ask
  // yourself two questions:
  //
  // 1. Are those data members Compiler-generated?
  //    If yes, put them in the appropriate ComTdb subclass instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

class ExHdfsScanTcb  : public ex_tcb
{
#ifdef NEED_PSTATE
  friend class ExHdfsScanPrivateState;
#endif

public:
  ExHdfsScanTcb( const ComTdbHdfsScan &tdb,
                         ex_globals *glob );

  ~ExHdfsScanTcb();

  void freeResources();
  virtual void registerSubtasks();  // register work procedures with scheduler
  virtual Int32 fixup();

  virtual ExWorkProcRetcode work(); 
  
  // The static work procs for scheduler. 
  static ExWorkProcRetcode sWorkUp(ex_tcb *tcb) 
      { return ((ExHdfsScanTcb *) tcb)->work(); }

  ex_queue_pair getParentQueue() const { return qparent_;}
  virtual Int32 numChildren() const { return 0; }
  virtual const ex_tcb* getChild(Int32 /*pos*/) const { return NULL; }
  virtual NABoolean needStatsEntry();
  virtual ExOperStats *doAllocateStatsEntry(CollHeap *heap,
					    ComTdb *tdb);
  virtual ex_tcb_private_state *allocatePstates(
    Lng32 &numElems,
    Lng32 &pstateLength);

  ExHdfsScanStats *getHfdsScanStats()
  {
    if (getStatsEntry())
      return getStatsEntry()->castToExHdfsScanStats();
    else
      return NULL;
  }
#ifdef NEED_PSTATE
  // For dynamic queue resizing.
  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element
#endif

protected:
  enum {
    NOT_STARTED
  , INIT_HDFS_CURSOR
  , OPEN_HDFS_CURSOR
  , CHECK_FOR_DATA_MOD
  , CHECK_FOR_DATA_MOD_AND_DONE
  , GET_HDFS_DATA
  , CLOSE_HDFS_CURSOR
  , PROCESS_HDFS_ROW
  , RETURN_ROW
  , REPOS_HDFS_DATA
  , CLOSE_FILE
  , ERROR_CLOSE_FILE
  , COLLECT_STATS
  , HANDLE_ERROR
  , HANDLE_EXCEPTION
  , DONE
  , HANDLE_ERROR_WITH_CLOSE
  , HANDLE_ERROR_AND_DONE
  } step_,nextStep_;

  /////////////////////////////////////////////////////
  // Private methods.
  /////////////////////////////////////////////////////

  inline ExHdfsScanTdb &hdfsScanTdb() const
    { return (ExHdfsScanTdb &) tdb; }

  inline ex_expr *selectPred() const 
    { return hdfsScanTdb().selectPred_; }

  inline ex_expr *moveExpr() const 
    { return hdfsScanTdb().moveExpr_; }

  inline ex_expr *convertExpr() const 
    { return hdfsScanTdb().convertExpr_; }

  inline ex_expr *moveColsConvertExpr() const 
    { return hdfsScanTdb().moveColsConvertExpr_; }

  inline bool isSequenceFile() const
  {return hdfsScanTdb().isSequenceFile(); }

  // returns ptr to start of next row, if any. Returning NULL 
  // indicates that no complete row was found. Beware of boundary 
  // conditions. Could be an incomplete row in a buffer returned by
  // hdfsRead. Or it could be the eof (in which case there is a good
  // row still waiting to be processed).
  char * extractAndTransformAsciiSourceToSqlRow(int &err,
						ComDiagsArea * &diagsArea, int mode);

  short moveRowToUpQueue(const char * row, Lng32 len, 
                         short * rc, NABoolean isVarchar);

  short handleError(short &rc);
  short handleDone(ExWorkProcRetcode &rc);

  short setupError(Lng32 exeError, Lng32 retcode, 
                   const char * str, const char * str2, const char * str3);

  /////////////////////////////////////////////////////
  // Private data.
  /////////////////////////////////////////////////////

  ex_queue_pair  qparent_;
  Int64 matches_;
  Int64 matchBrkPoint_;
  atp_struct     * workAtp_;
  Int64 bytesLeft_;
  hdfsFile hfdsFileHandle_;
  char * hdfsScanBuffer_;
  char * hdfsBufNextRow_;

  char * debugPrevRow_;             // Pointer to help with debugging.
  Int64 debugtrailingPrevRead_;
  char *debugPenultimatePrevRow_;
  
  ExSimpleSQLBuffer *hdfsSqlBuffer_;  // this buffer for one row, converted
                                      // from ascii to SQL for select pred.
  tupp hdfsSqlTupp_;                  // tupp for this one row.
  char *hdfsSqlData_;                 // this one row. 

  ExSimpleSQLBuffer *moveExprColsBuffer_; // this buffer for one row, converted
                                      // from ascii to SQL for move expr only.
  tupp moveExprColsTupp_;             // tupp for this one row.
  char *moveExprColsData_;                 // this one row. 

  // this is where delimited columns that are read from hdfs rows will be moved to.
  // They will become source for convertExpr which will convert
  // them to sql row that will be returned by the scan operator.
  ExSimpleSQLBuffer *hdfsAsciiSourceBuffer_;
  tupp hdfsAsciiSourceTupp_;
  char * hdfsAsciiSourceData_;

  sql_buffer_pool * pool_;            // row images after selection pred,
                                      // with only the required columns. 
  hdfsFile hdfsFp_;
  hdfsFS hdfsFs_;     // shallow copy from cliGlobals.

  void * lobGlob_;

  Int64 requestTag_;
  Int64 hdfsScanBufMaxSize_;
  Int64 bytesRead_;
  Int64 trailingPrevRead_;     // for trailing bytes at end of buffer.
  Int64 numBytesProcessedInRange_;  // count bytes of complete records.
  bool  firstBufOfFile_;
  ExHdfsScanStats * hdfsStats_;
  HdfsFileInfo *hdfo_;
  Int64 hdfsOffset_;
  Lng32 myInstNum_;
  Lng32 beginRangeNum_;
  Lng32 numRanges_;
  Lng32 currRangeNum_;
  char *endOfRequestedRange_ ; // helps rows span ranges.
  char * hdfsFileName_;
  SequenceFileReader* sequenceFileReader_;
  Int64 stopOffset_;
  bool  seqScanAgain_;
  char cursorId_[8];

  char loggingFileName_[1000];
  NABoolean LoggingFileCreated_ ;
  char * hdfsLoggingRow_;
  char * hdfsLoggingRowEnd_;
  tupp_descriptor * defragTd_;

  ExpHbaseInterface * ehi_;

  NABoolean exception_;
  ComCondition * lastErrorCnd_;
  NABoolean checkRangeDelimiter_;

  NABoolean dataModCheckDone_;
};

class ExOrcScanTcb  : public ExHdfsScanTcb
{
  friend class ExOrcFastAggrTcb;

public:
  ExOrcScanTcb( const ComTdbHdfsScan &tdb,
                 ex_globals *glob );

  ~ExOrcScanTcb();

  virtual ExWorkProcRetcode work(); 
  
protected:
  enum {
    NOT_STARTED
  , INIT_ORC_CURSOR
  , OPEN_ORC_CURSOR
  , GET_ORC_ROW
  , PROCESS_ORC_ROW
  , CLOSE_ORC_CURSOR
  , RETURN_ROW
  , CLOSE_FILE
  , ERROR_CLOSE_FILE
  , COLLECT_STATS
  , HANDLE_ERROR
  , DONE
  } step_;

  /////////////////////////////////////////////////////
  // Private methods.
  /////////////////////////////////////////////////////
 private:
  short extractAndTransformOrcSourceToSqlRow(
                                             char * orcRow,
                                             Int64 orcRowLen,
                                             Lng32 numOrcCols,
                                             ComDiagsArea* &diagsArea);
  
  /////////////////////////////////////////////////////
  // Private data.
  /////////////////////////////////////////////////////

  ExpORCinterface * orci_;

  Int64 orcStartRowNum_;
  Int64 orcNumRows_;
  Int64 orcStopRowNum_;

  // returned row from orc scanFetch
  char * orcRow_;
  Int64 orcRowLen_;
  Int64 orcRowNum_;
  Lng32 numOrcCols_;
};

// -----------------------------------------------------------------------
// ExOrcFastAggrTdb
// -----------------------------------------------------------------------
class ExOrcFastAggrTdb : public ComTdbOrcFastAggr
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  NA_EIDPROC ExOrcFastAggrTdb()
  {}

  NA_EIDPROC virtual ~ExOrcFastAggrTdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  NA_EIDPROC virtual ex_tcb *build(ex_globals *globals);

private:
  // ---------------------------------------------------------------------
  // !!!!!!! IMPORTANT -- NO DATA MEMBERS ALLOWED IN EXECUTOR TDB !!!!!!!!
  // *********************************************************************
  // The Executor TDB's are only used for the sole purpose of providing a
  // way to supplement the Compiler TDB's (in comexe) with methods whose
  // implementation depends on Executor objects. This is done so as to
  // decouple the Compiler from linking in Executor objects unnecessarily.
  //
  // When a Compiler generated TDB arrives at the Executor, the same data
  // image is "cast" as an Executor TDB after unpacking. Therefore, it is
  // a requirement that a Compiler TDB has the same object layout as its
  // corresponding Executor TDB. As a result of this, all Executor TDB's
  // must have absolutely NO data members, but only member functions. So,
  // if you reach here with an intention to add data members to a TDB, ask
  // yourself two questions:
  //
  // 1. Are those data members Compiler-generated?
  //    If yes, put them in the appropriate ComTdb subclass instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

// class ExOrcFastAggrTcb
class ExOrcFastAggrTcb  : public ExOrcScanTcb
{
public:
  ExOrcFastAggrTcb( const ComTdbOrcFastAggr &tdb,
                    ex_globals *glob );
  
  ~ExOrcFastAggrTcb();

  virtual ExWorkProcRetcode work(); 

  inline ExOrcFastAggrTdb &orcAggrTdb() const
  { return (ExOrcFastAggrTdb &) tdb; }
  
protected:
  enum {
    NOT_STARTED
    , ORC_AGGR_INIT
    , ORC_AGGR_EVAL
    , ORC_AGGR_PROJECT
    , ORC_AGGR_RETURN
    , CLOSE_FILE
    , ERROR_CLOSE_FILE
    , COLLECT_STATS
    , HANDLE_ERROR
    , DONE
  } step_;

  /////////////////////////////////////////////////////
  // Private data.
  /////////////////////////////////////////////////////
 private:
  Int64 rowCount_;
  char * aggrRow_;
};

#define RANGE_DELIMITER '\002'

inline char *hdfs_strchr(char *s, int c, const char *end, NABoolean checkRangeDelimiter, int mode , int *changedLen)
{
  char *curr = (char *)s;
  int count=0;
  //changedLen is lenght of \r which removed by this function
  *changedLen = 0;
  if( (mode & HIVE_MODE_DOSFORMAT ) == 0)
  {
   while (curr < end) {
    if (*curr == c)
    {
       return curr;
    }
    if (checkRangeDelimiter &&*curr == RANGE_DELIMITER)
       return NULL;
    curr++;
   }
  }
  else
  {
   while (curr < end) {
     if (*curr == c)
     {
         if(count>0 && c == '\n')
         {
           if(s[count-1] == '\r') 
             *changedLen = 1;
         }
         return curr - *changedLen;
      }
      if (checkRangeDelimiter &&*curr == RANGE_DELIMITER)
         return NULL;
    curr++;
    count++;
   }
  }
  return NULL;
}


inline char *hdfs_strchr(char *s, int rd, int cd, const char *end, NABoolean checkRangeDelimiter, NABoolean *rdSeen, int mode, int* changedLen)
{
  char *curr = (char *)s;
  int count = 0;
  //changedLen is lenght of \r which removed by this function
  *changedLen = 0;
  if( (mode & HIVE_MODE_DOSFORMAT)>0 )  //check outside the while loop to make it faster
  {
    while (curr < end) {
      if (*curr == rd) {
         if(count>0 && rd == '\n')
         {
             if(s[count-1] == '\r') 
               *changedLen = 1;
         }
         *rdSeen = TRUE;
         return curr - *changedLen;
      }
      else
      if (*curr == cd) {
         *rdSeen = FALSE;
         return curr;
      }
      else
      if (checkRangeDelimiter && *curr == RANGE_DELIMITER) {
         *rdSeen = TRUE;
         return NULL;
      }
      curr++;
      count++;
    }
  }
  else
  {
    while (curr < end) {
      if (*curr == rd) {
         *rdSeen = TRUE;
         return curr;
      }
      else
      if (*curr == cd) {
         *rdSeen = FALSE;
         return curr;
      }
      else
      if (checkRangeDelimiter && *curr == RANGE_DELIMITER) {
         *rdSeen = TRUE;
         return NULL;
      }
      curr++;
    }
  }
  *rdSeen = FALSE;
  return NULL;
}



#endif
