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
#ifndef EX_EXE_UTIL_H
#define EX_EXE_UTIL_H


/* -*-C++-*-
*****************************************************************************
*
* File:         ExExeUtil.h
* Description:
*
*
* Created:      9/12/2005
* Language:     C++
*
*
*
*
*****************************************************************************
*/

// forward
class ex_expr;
class Queue;
class ExTransaction;
class ContextCli;
class IpcServer;
class IpcServerClass;
class ExStatisticsArea;
class CmpDDLwithStatusInfo;
class ExSqlComp;

class ExProcessStats;

class ExpHbaseInterface;
class HdfsClient;

//class FILE_STREAM;
#include "ComAnsiNamePart.h"
#include "ComTdbExeUtil.h"
#include "ComTdbRoot.h"
#include "ComRtUtils.h"
#include "ExExeUtilCli.h"
#include "ExpLOBstats.h"
#include "hiveHook.h"
#include "ExpHbaseDefs.h"

#include "SequenceFileReader.h"

#define TO_FMT3u(u) MINOF(((u)+500)/1000, 999)
#define MAX_ACCUMULATED_STATS_DESC 2
#define MAX_PERTABLE_STATS_DESC    256
#define MAX_PROGRESS_STATS_DESC    256 
#define MAX_OPERATOR_STATS_DESC    512
#define MAX_RMS_STATS_DESC         512
#define BUFFER_SIZE 4000
#define MAX_AUTHIDTYPE_CHAR        11
#define MAX_USERINFO_CHAR          257
// max number of attempts to create ORSERV upon failure
#define MAX_CREATE_ORSERV_COUNT 5

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExExeUtilTdb;
class ExExeUtilDisplayExplainTdb;
class ExExeUtilDisplayExplainComplexTdb;
class ExExeUtilHiveTruncateTdb;
class ExExeUtilHiveQueryTdb;
class ExExeUtilSuspendTdb;
class ExExeUtilSuspendTcb;
class ExpHbaseInterface;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ExExeUtilTdb
// -----------------------------------------------------------------------
class ExExeUtilTdb : public ComTdbExeUtil
{
 public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilTdb()
    {}

  virtual ~ExExeUtilTdb()
    {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};


struct QueryString {
  public:
const char * str;
};

//
// Task control block
//
class ExExeUtilTcb : public ex_tcb
{
  friend class ExExeUtilTdb;
  friend class ExExeUtilPrivateState;

 public:
  enum Step
    {
      EMPTY_,
      PROCESSING_,
      DONE_,
      HANDLE_ERROR_,
      CANCELLED_
    };

  enum ProcessQueryStep
  {
    PROLOGUE_,
    EXECUTE_,
    FETCH_ROW_,
    RETURN_ROW_,
    ERROR_RETURN_,
    CLOSE_,
    EPILOGUE_,
    ALL_DONE_
  };


  // Constructor
  ExExeUtilTcb(const ComTdbExeUtil & exe_util_tdb,
               const ex_tcb * child_tcb, // for child queue
               ex_globals * glob = 0);

  ~ExExeUtilTcb();

  virtual short work();

  ex_queue_pair getParentQueue() const;
  Int32 orderedQueueProtocol() const;

  virtual void freeResources();

  virtual Int32 numChildren() const;
  virtual const ex_tcb* getChild(Int32 pos) const;

  void glueQueryFragments(Lng32 queryArraySize,
                          const QueryString * queryArray,
                          char * &gluedQuery,
                          Lng32 &gluedQuerySize);

  // extract parts from 'objectName' and fixes up delimited names.
  Lng32 extractParts
  (char * objectName,
   char ** parts0,
   char ** parts1,
   char ** parts2
  );

   Lng32 changeAuditAttribute(char * tableName,
                             NABoolean makeAudited,
                             NABoolean isVolatile = FALSE,
                             NABoolean isIndex = FALSE,
                             NABoolean parallelAlter = FALSE
                             );

  virtual short moveRowToUpQueue(const char * row, Lng32 len = -1,
                                 short * rc = NULL, NABoolean isVarchar = TRUE);

  NABoolean isUpQueueFull(short size);

  static char * getTimeAsString(Int64 t, char * timeBuf,
                                NABoolean noUsec = FALSE);
  char * getTimestampAsString(Int64 t, char * timeBuf);

  short initializeInfoList(Queue* &infoList);
  short fetchAllRows(Queue * &infoList,
                     char * query,
                     Lng32 numOutputEntries,
                     NABoolean varcharFormat,
                     short &rc,
		     NABoolean monitorThis=FALSE);
    
  ex_expr::exp_return_type evalScanExpr(char * ptr, Lng32 len, 
                                        NABoolean copyToVCbuf);

  char * getStatusString(const char * operation,
                         const char * status,
                         const char * object,
                         char * outBuf,
                         NABoolean isET = FALSE,
                         char * timeBuf = NULL,
                         char * queryBuf = NULL,
                         char * sqlcodeBuf = NULL);

  short executeQuery(char * step, char * object,
                     char * query,
                     NABoolean displayStartTime, NABoolean displayEndTime,
                     short &rc, short * warning, Lng32 * errorCode = NULL,
                     NABoolean moveErrorRow = TRUE,
                     NABoolean continueOnError = FALSE,
		     NABoolean monitorThis=FALSE);

  ExeCliInterface * cliInterface() { return cliInterface_; };
  ExeCliInterface * cliInterface2() { return cliInterface2_; };

  ComDiagsArea *&getDiagsArea() { return diagsArea_; }

  void setDiagsArea(ComDiagsArea * d) { diagsArea_ = d; }

  short setSchemaVersion(char * param1);

  short setSystemVersion();

  short holdAndSetCQD(const char * defaultName, const char * defaultValue,
                      ComDiagsArea * globalDiags = NULL);

  short restoreCQD(const char * defaultName, ComDiagsArea * globalDiags = NULL);

  short setCS(const char * csName, char * csValue,
              ComDiagsArea * globalDiags = NULL);
  short resetCS(const char * csName, ComDiagsArea * globalDiags = NULL);

  void setMaintainControlTableTimeout(char * catalog);
  void restoreMaintainControlTableTimeout(char * catalog);

  static Lng32 holdAndSetCQD(const char * defaultName, const char * defaultValue,
                         ExeCliInterface * cliInterface,
                         ComDiagsArea * globalDiags = NULL);

  static Lng32 restoreCQD(const char * defaultName,
                         ExeCliInterface * cliInterface,
                         ComDiagsArea * globalDiags = NULL);

  static Lng32 setCS(const char * csName, char * csValue,
                    ExeCliInterface * cliInterface,
                    ComDiagsArea * globalDiags = NULL);
  static Lng32 resetCS(const char * csName,
                      ExeCliInterface * cliInterface,
                      ComDiagsArea * globalDiags = NULL);

  short disableCQS();
  short restoreCQS();

  short doubleQuoteStr(char * str, char * newStr,
                       NABoolean singleQuote);

  char * getSchemaVersion()
  { return (strlen(versionStr_) == 0 ? (char *)"" : versionStr_); }

  char * getSystemVersion()
  { return (strlen(sysVersionStr_) == 0 ? NULL : sysVersionStr_); }

  Lng32 getSchemaVersionLen() { return versionStrLen_; }

  short getObjectUid(char * catName, char * schName,
                     char * objName,
                     NABoolean isIndex, NABoolean isMv,
                     char * uid);

  short lockUnlockObject(char * tableName,
                         NABoolean lock,
                         NABoolean parallel,
                         char * failReason);

  short alterObjectState(NABoolean online, char * tableName,
                         char * failReason, NABoolean forPurgedata);
  short alterDDLLock(NABoolean add, char * tableName,
                     char * failReason, NABoolean isMV,
                     Int32 lockType,
                     const char * lockPrefix = NULL,
                     NABoolean skipDDLLockCheck = FALSE);
  short alterCorruptBit(short val, char * tableName,
                        char * failReason, Queue* indexList);

  short alterAuditFlag(NABoolean audited, char * tableName,
                       NABoolean isIndex);

  short handleError();

  short handleDone();

  NABoolean &infoListIsOutputInfo() { return infoListIsOutputInfo_; }

  // Error: -1, creation of IpcServerClass failed.
  //        -2, PROCESSNAME_CREATE_ failed.
  //        -3, allocateServerProcess failed.
  short createServer(char *serverName,
                     const char *inPName,
                     IpcServerTypeEnum serverType,
                     IpcServerAllocationMethod servAllocMethod,
                     char *nodeName,
                     short cpu,
                     const char *partnName,
                     Lng32 priority,
                     IpcServer* &ipcServer,
                     NABoolean logError,
                     const char * operation);

  void deleteServer(IpcServer *ipcServer);

  NABoolean isProcessObsolete(short cpu, pid_t pin, short segmentNum,
                              Int64 procCreateTime);

protected:

  const ex_tcb * childTcb_;

  ex_queue_pair         qparent_;
  ex_queue_pair  qchild_;

  atp_struct * workAtp_;

  ComDiagsArea * diagsArea_;

  char * query_;

  unsigned short tcbFlags_;

  char * explQuery_;

  ExExeUtilTdb & exeUtilTdb() const{return (ExExeUtilTdb &) tdb;};
  void handleErrors(Lng32 error);

  CollHeap * getMyHeap()
    {
      return (getGlobals()->getDefaultHeap());
    };

  void  AddCommas (char *outStr, Lng32 &intSize) const;
  void FormatFloat (char *outStr, Lng32 &intSize,
                    Lng32 &fullSize, double floatVal,
                    NABoolean normalMode, NABoolean expertMode)const;

  Queue * infoList_;
  NABoolean infoListIsOutputInfo_;
  char *childQueryId_;
  Lng32 childQueryIdLen_;
  SQL_QUERY_COST_INFO childQueryCostInfo_;
  SQL_QUERY_COMPILER_STATS_INFO childQueryCompStatsInfo_;
  char *outputBuf_;

  char failReason_[2000];

private:
  ExeCliInterface * cliInterface_;
  ExeCliInterface * cliInterface2_;

  ProcessQueryStep pqStep_;

  char   versionStr_[10];
  Lng32   versionStrLen_;

  char   sysVersionStr_[10];
  Lng32   sysVersionStrLen_;

  NABoolean restoreTimeout_;

  AnsiName *extractedPartsObj_;

  Int64 startTime_;
  Int64 endTime_;
  Int64 elapsedTime_;

  short warning_;
};

class ExExeUtilPrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilTcb;
  friend class ExExeUtilCleanupVolatileTablesTcb;
  friend class ExExeUtilCreateTableAsTcb;
  friend class ExExeUtilHiveTruncateTcb;
  friend class ExExeUtilHiveQueryTcb;
  friend class ExExeUtilAQRTcb;
  friend class ExExeUtilHBaseBulkLoadTcb;
  friend class ExExeUtilHBaseBulkUnLoadTcb;


 public:
  ExExeUtilPrivateState(const ExExeUtilTcb * tcb); //constructor
  ~ExExeUtilPrivateState();        // destructor
  ex_tcb_private_state * allocate_new(const ex_tcb * tcb);
 protected:
  ExExeUtilTcb::Step step_;
  Int64 matches_;
};

// -----------------------------------------------------------------------
// ExExeUtilDisplayExplainTdb
// -----------------------------------------------------------------------
class ExExeUtilDisplayExplainTdb : public ComTdbExeUtilDisplayExplain
{
 public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilDisplayExplainTdb()
    {}

  virtual ~ExExeUtilDisplayExplainTdb()
    {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};


//
// Task control block
//
class ExExeUtilDisplayExplainTcb : public ExExeUtilTcb
{
  friend class ExExeUtilDisplayExplainTdb;
  friend class ExExeUtilPrivateState;

 public:

  // Constructor
  ExExeUtilDisplayExplainTcb(const ComTdbExeUtilDisplayExplain & exe_util_tdb,
                             ex_globals * glob = 0);

  ~ExExeUtilDisplayExplainTcb();

  virtual short work();

  virtual ex_tcb_private_state * allocatePstates(
                                                            Lng32 &numElems,      // inout, desired/actual elements
                                                            Lng32 &pstateLength); // out, length of one element

  short processExplainRows();

  //state machine states for work() method
  enum Step
    {
      EMPTY_,
      PREPARE_,
      SETUP_EXPLAIN_,
      FETCH_PROLOGUE_,
      FETCH_EXPLAIN_ROW_,
      FETCH_FIRST_EXPLAIN_ROW_,
      GET_COLUMNS_,
      DO_HEADER_,
      DO_OPERATOR_,
      RETURN_FMT_ROWS_,
      RETURN_EXPLAIN_ROW_,
      FETCH_EPILOGUE_,
      FETCH_EPILOGUE_AND_RETURN_ERROR_,
      DONE_,
      HANDLE_ERROR_,
      RETURN_ERROR_,
      CANCELLED_
    };

 protected:

 private:
  // Enums
  //  enum { MLINE = 74    };          // max number of lines in the array, 3000/50 + 14
  //  enum { MLEN  = 80    };          // max length of each line
  enum { MNAME = 60    };          // width of the returned SQL name columns
  enum { MOPER = 30    };          // width of the returned SQL operator column
  enum { MCOST = 200   };          // width of the returned SQL detailed_cost column
  enum { MDESC = 30000  };          // width of the returned SQL description column
  enum { MUSERSP = 20  };          // max output number space for user
  //  enum { MWIDE = MLEN-1};          // max line width in char for output
  enum { COL2  = 28    };          // position of column 2 (value) on the line
  enum Option                      //which output format we are using
    {
      N_,
      E_,
      F_,
      M_
    };

  // Variables
  SQLMODULE_ID * module_;
  SQLSTMT_ID   * stmt_;
  SQLDESC_ID   * sql_src_;
  SQLDESC_ID   * input_desc_;
  SQLDESC_ID   * output_desc_;
  char         * outputBuf_;
  char * explainQuery_;

  Option optFlag_;              // option flag
  char ** lines_;
  //  char  lines_[MLINE][MLEN];    // array of MLINE lines MLEN char wide
  Lng32  cntLines_;              // count of lines in lines_ array
  Lng32  nextLine_;              // number of next line to output from array
  char* parsePtr_;              // location in input string being parsed
  Lng32  header_;                // flag saying current node is root if 1
  Lng32  lastFrag_;              // previous row fragment number
  char  lastOp_[MOPER+1];       // previous row operator name

  char * optFOutput;
  //  char  optFOutput[MLEN];        // formatted line for optionsF

  // Next 13 are the local column data, storage for one node's info
  char  moduleName_[MNAME+1];   // module name column, sometimes null, MNAME=60
  char  statementName_[MNAME+1];// stmt name column, sometimes null, MNAME=60
  Int64 planId_;                // large number, unique per plan
  Lng32  seqNum_;                // number of this node
  char  operName_[MOPER+1];     // operator name, MOPER=30
  Lng32  leftChild_;             // number of left child
  Lng32  rightChild_;            // number of right child
  char  tName_[MNAME+1];        // table name, often null, MNAME=60
  float cardinality_;           // number of rows returned by this node
  float operatorCost_;          // cost of this node alone, in seconds
  float totalCost_;             // cost of this node and all children, in seconds
  char  detailCost_[MCOST+1];   // node cost in 5 parts in key-value format, MCOST=200
  char  description_[MDESC+1];  // other attributs in key-value format, MDESC=3000

  Lng32 MLINE;
  Lng32 MLEN;
  Lng32 MWIDE;

  // Methods
  short GetColumns();
  void  FormatForF();                // formats the line for optionsF and places into optFOutput
  void  DoHeader();
  void  DoOperator();
  short OutputLines();
  void  truncate_whitespace(char * str) const;
  void  DoSeparator();
  //void  AddCommas (char *outStr, long &intSize) const;
  //void  FormatFloat (char *outStr, long &intSize, long &fullSize, float floatVal) const;
  void  FormatNumber (char *outStr, Lng32 &intSize, Lng32 &fullSize, char *strVal) const;
  Lng32  GetField (char *col, const char *key, char *&fieldptr, Lng32 &fullSize) const;
  Lng32  ParseField (char *&keyptr, char *&fieldptr, Lng32 &keySize, Lng32 &fullSize,
                    Lng32 &done);
  Lng32  IsNumberFmt(char *fieldptr) const;
  void  FormatFirstLine (void);
  NABoolean filterKey(
       const char *key, Lng32 keySize, char * value, char * retVal,
       Lng32 &decLoc);
  void  FormatLine (const char *key, const char *val, Lng32 keySize, Lng32 valSize,
                    Lng32 indent = 0, Lng32 decLoc = 0);
  void  FormatLongLine (const char *key, char *val, Lng32 keySize, Lng32 valSize,
                        Lng32 indent = 0);
  void  FormatSQL (const char *key, char *val, Lng32 keySize, Lng32 valSize, Lng32 indent = 0);
  Lng32  FindParens(char *inStr, Lng32 par[]) const;

  ExExeUtilDisplayExplainTdb & exeUtilTdb() const
    {return (ExExeUtilDisplayExplainTdb &) tdb;};
};


class ExExeUtilDisplayExplainPrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilDisplayExplainTcb;

 public:
  ExExeUtilDisplayExplainPrivateState();
  ~ExExeUtilDisplayExplainPrivateState();        // destructor
 protected:
  ExExeUtilDisplayExplainTcb::Step step_;
  Int64 matches_;
};

//////////////////////////////////////////////////////////////////////////


// -----------------------------------------------------------------------
// ExExeUtilDisplayExplainComplexTdb
// -----------------------------------------------------------------------
class ExExeUtilDisplayExplainComplexTdb : public ComTdbExeUtilDisplayExplainComplex
{
 public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilDisplayExplainComplexTdb()
    {}

  virtual ~ExExeUtilDisplayExplainComplexTdb()
    {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};


//
// Task control block
//
class ExExeUtilDisplayExplainComplexTcb : public ExExeUtilTcb
{
  friend class ExExeUtilDisplayExplainComplexTdb;
  friend class ExExeUtilPrivateState;

 public:

  // Constructor
  ExExeUtilDisplayExplainComplexTcb(const ComTdbExeUtilDisplayExplainComplex & exe_util_tdb,
                                    ex_globals * glob = 0);

  ~ExExeUtilDisplayExplainComplexTcb();

  virtual short work();

  virtual ex_tcb_private_state *
  allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

private:
  //state machine states for work() method
  enum Step
    {
      EMPTY_,
      TURN_ON_IMOD_,
      IN_MEMORY_BASE_TABLE_CREATE_,
      IN_MEMORY_CREATE_,
      REGULAR_CREATE_,
      ALTER_TO_NOAUDIT_,
      ALTER_TO_AUDIT_,
      EXPLAIN_CREATE_,
      EXPLAIN_INSERT_SELECT_,
      DROP_AND_ERROR_,
      DROP_AND_DONE_,
      DONE_,
      ERROR_,
      CANCELLED_
    };

  ExExeUtilDisplayExplainComplexTdb & exeUtilTdb() const
    {return (ExExeUtilDisplayExplainComplexTdb &) tdb;};

  Step step_;
};

class ExExeUtilDisplayExplainComplexPrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilDisplayExplainComplexTcb;

public:
  ExExeUtilDisplayExplainComplexPrivateState();
  ~ExExeUtilDisplayExplainComplexPrivateState();        // destructor
protected:
  Int64 matches_;
};

class ExExeUtilDisplayExplainShowddlTcb : public ExExeUtilTcb
{
  friend class ExExeUtilDisplayExplainComplexTdb;
  friend class ExExeUtilPrivateState;

 public:

  // Constructor
  ExExeUtilDisplayExplainShowddlTcb(const ComTdbExeUtilDisplayExplainComplex & exe_util_tdb,
                                    ex_globals * glob = 0);

  ~ExExeUtilDisplayExplainShowddlTcb();

  virtual short work();

  virtual ex_tcb_private_state *
  allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

private:
  //state machine states for work() method
  enum Step
    {
      EMPTY_,
      GET_LABEL_STATS_,
      EXPLAIN_CREATE_,
      DONE_,
      ERROR_,
      CANCELLED_
    };

  ExExeUtilDisplayExplainComplexTdb & exeUtilTdb() const
    {return (ExExeUtilDisplayExplainComplexTdb &) tdb;};

  Step step_;

  char * newQry_;
};

class ExExeUtilDisplayExplainShowddlPrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilDisplayExplainShowddlTcb;

public:
  ExExeUtilDisplayExplainShowddlPrivateState();
  ~ExExeUtilDisplayExplainShowddlPrivateState();        // destructor
protected:
  Int64 matches_;
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilCreateTableAsTdb
// -----------------------------------------------------------------------
class ExExeUtilCreateTableAsTdb : public ComTdbExeUtilCreateTableAs
{
 public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilCreateTableAsTdb()
    {}

  virtual ~ExExeUtilCreateTableAsTdb()
    {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

class ExExeUtilCreateTableAsTcb : public ExExeUtilTcb
{
  friend class ExExeUtilCreateTableAsTdb;
  friend class ExExeUtilPrivateState;

 public:
  // Constructor
  ExExeUtilCreateTableAsTcb(const ComTdbExeUtil & exe_util_tdb,
                            ex_globals * glob = 0);

  virtual short work();

  ExExeUtilCreateTableAsTdb & ctaTdb() const
    {
      return (ExExeUtilCreateTableAsTdb &) tdb;
    };

  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

 private:
  enum Step
    {
      INITIAL_,
      CREATE_,
      TRUNCATE_TABLE_,
      INSERT_SIDETREE_,
      INSERT_VSBB_,
      ALTER_TO_NOAUDIT_,
      ALTER_TO_AUDIT_,
      ALTER_TO_AUDIT_AND_INSERT_VSBB_,
      UPD_STATS_,
      DONE_,
      HANDLE_ERROR_, TRUNCATE_TABLE_AND_ERROR_, ERROR_,
      DROP_AND_ERROR_,
      DROP_AND_DONE_,
      INSERT_SIDETREE_EXECUTE_,
      INSERT_VSBB_EXECUTE_
    };


  Step step_;

  NABoolean doSidetreeInsert_;

  NABoolean tableExists_;
};

class ExExeUtilCreateTableAsPrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilCreateTableAsTcb;

 public:
  ExExeUtilCreateTableAsPrivateState();
  ~ExExeUtilCreateTableAsPrivateState();        // destructor
 protected:
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilCleanupVolatileTablesTdb
// -----------------------------------------------------------------------
class ExExeUtilCleanupVolatileTablesTdb : public ComTdbExeUtilCleanupVolatileTables
{
 public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilCleanupVolatileTablesTdb()
    {}

  virtual ~ExExeUtilCleanupVolatileTablesTdb()
    {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

class ExExeUtilVolatileTablesTcb : public ExExeUtilTcb
{
 public:
  // Constructor
  ExExeUtilVolatileTablesTcb(const ComTdbExeUtil & exe_util_tdb,
                             ex_globals * glob = 0);

  virtual ex_tcb_private_state * allocatePstates(
                                                 Lng32 &numElems,      // inout, desired/actual elements
                                                 Lng32 &pstateLength); // out, length of one element

 protected:
  short isCreatorProcessObsolete(const char * name,
                                 NABoolean includesCat,
                                 NABoolean isCSETableName);
};

class ExExeUtilCleanupVolatileTablesTcb : public ExExeUtilVolatileTablesTcb
{
  friend class ExExeUtilCleanupVolatileTablesTdb;
  friend class ExExeUtilPrivateState;

 public:
  // Constructor
  ExExeUtilCleanupVolatileTablesTcb(const ComTdbExeUtil & exe_util_tdb,
                                    ex_globals * glob = 0);

  virtual short work();

  ExExeUtilCleanupVolatileTablesTdb & cvtTdb() const
    {
      return (ExExeUtilCleanupVolatileTablesTdb &) tdb;
    };

  static short dropVolatileSchema(ContextCli * currContext,
                                  char * schemaName,
                                  CollHeap * heap,
                                  ComDiagsArea *&diagsArea,
                                  ex_globals *globals = NULL);
  static short dropVolatileTables(ContextCli * currContext, CollHeap * heap);
  short dropHiveTempTablesForCSEs();

 private:
  enum Step
    {
      INITIAL_,
      FETCH_SCHEMA_NAMES_,
      START_CLEANUP_,
      CHECK_FOR_OBSOLETE_CREATOR_PROCESS_,
      BEGIN_WORK_,
      DO_CLEANUP_,
      COMMIT_WORK_,
      END_CLEANUP_,
      CLEANUP_HIVE_TABLES_,
      DONE_,
      ERROR_
    };

  Step step_;

  Queue * schemaNamesList_;

  NABoolean someSchemasCouldNotBeDropped_;
  char errorSchemas_[1010];

  char * schemaQuery_;
};

class ExExeUtilVolatileTablesPrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilVolatileTablesTcb;

 public:
  ExExeUtilVolatileTablesPrivateState();
  ~ExExeUtilVolatileTablesPrivateState();        // destructor
 protected:
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilGetVolatileInfoTdb
// -----------------------------------------------------------------------
class ExExeUtilGetVolatileInfoTdb : public ComTdbExeUtilGetVolatileInfo
{
 public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilGetVolatileInfoTdb()
    {}

  virtual ~ExExeUtilGetVolatileInfoTdb()
    {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilGetVolatileInfoTcb
// -----------------------------------------------------------------------
class ExExeUtilGetVolatileInfoTcb : public ExExeUtilVolatileTablesTcb
{
  friend class ExExeUtilGetVolatileInfoTdb;
  friend class ExExeUtilPrivateState;

 public:
  // Constructor
  ExExeUtilGetVolatileInfoTcb(const ComTdbExeUtil & exe_util_tdb,
                              ex_globals * glob = 0);

  virtual short work();

  ExExeUtilGetVolatileInfoTdb & gviTdb() const
    {
      return (ExExeUtilGetVolatileInfoTdb &) tdb;
    };

 private:
  enum Step
    {
      INITIAL_,
      GET_SCHEMA_VERSION_,
      GET_ALL_NODE_NAMES_,
      APPEND_NEXT_QUERY_FRAGMENT_,
      FETCH_ALL_ROWS_,
      RETURN_ALL_SCHEMAS_,
      RETURN_ALL_TABLES_,
      RETURN_TABLES_IN_A_SESSION_,
      DONE_,
      ERROR_
    };

  Step step_;

  OutputInfo * prevInfo_;

  char * infoQuery_;
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilGetErrorInfoTdb
// -----------------------------------------------------------------------
class ExExeUtilGetErrorInfoTdb : public ComTdbExeUtilGetErrorInfo
{
 public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilGetErrorInfoTdb()
    {}

  virtual ~ExExeUtilGetErrorInfoTdb()
    {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilGetErrorInfoTcb
// -----------------------------------------------------------------------
class ExExeUtilGetErrorInfoTcb : public ExExeUtilTcb
{
  friend class ExExeUtilGetErrorInfoTdb;
  friend class ExExeUtilPrivateState;

 public:
  // Constructor
  ExExeUtilGetErrorInfoTcb(const ComTdbExeUtilGetErrorInfo & exe_util_tdb,
			   ex_globals * glob = 0);

  virtual short work();

  ExExeUtilGetErrorInfoTdb & geiTdb() const
    {
      return (ExExeUtilGetErrorInfoTdb &) tdb;
    };

  ex_tcb_private_state * 
    allocatePstates(
		    Lng32 &numElems,      // inout, desired/actual elements
		    Lng32 &pstateLength)  // out, length of one element
    ;

 private:
  enum Step
    {
      INITIAL_,
      RETURN_TEXT_,
      DONE_,
      ERROR_
    };

  Step step_;

  char * outputBuf_;
};

class ExExeUtilGetErrorInfoPrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilGetErrorInfoTcb;

 public:
  ExExeUtilGetErrorInfoPrivateState();
  ~ExExeUtilGetErrorInfoPrivateState();        // destructor
 protected:
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilLoadVolatileTableTdb
// -----------------------------------------------------------------------
class ExExeUtilLoadVolatileTableTdb : public ComTdbExeUtilLoadVolatileTable
{
 public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilLoadVolatileTableTdb()
    {}

  virtual ~ExExeUtilLoadVolatileTableTdb()
    {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

class ExExeUtilLoadVolatileTableTcb : public ExExeUtilTcb
{
  friend class ExExeUtilLoadVolatileTableTdb;
  friend class ExExeUtilPrivateState;

 public:
  // Constructor
  ExExeUtilLoadVolatileTableTcb(const ComTdbExeUtil & exe_util_tdb,
                                ex_globals * glob = 0);

  virtual short work();

  ExExeUtilLoadVolatileTableTdb & lvtTdb() const
    {
      return (ExExeUtilLoadVolatileTableTdb &) tdb;
    };

  ex_tcb_private_state * allocatePstates(
                                         Lng32 &numElems,      // inout, desired/actual elements
                                         Lng32 &pstateLength)  // out, length of one element
    ;

 private:
  enum Step
    {
      INITIAL_,
      INSERT_,
      UPD_STATS_,
      DONE_,
      ERROR_
    };

  Step step_;

};

class ExExeUtilLoadVolatileTablePrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilLoadVolatileTableTcb;

 public:
  ExExeUtilLoadVolatileTablePrivateState();
  ~ExExeUtilLoadVolatileTablePrivateState();        // destructor
 protected:
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilGetStatisticsTdb
// -----------------------------------------------------------------------
class ExExeUtilGetStatisticsTdb : public ComTdbExeUtilGetStatistics
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilGetStatisticsTdb()
  {}

  virtual ~ExExeUtilGetStatisticsTdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilGetStatisticsTcb
// -----------------------------------------------------------------------
class ExExeUtilGetStatisticsTcb : public ExExeUtilTcb
{
  friend class ExExeUtilGetStatisticsTdb;
  friend class ExExeUtilPrivateState;
  friend class ExExeUtilGetRTSStatisticsTcb;

public:
  // Constructor
  ExExeUtilGetStatisticsTcb(const ComTdbExeUtilGetStatistics & exe_util_tdb,
                            ex_globals * glob = 0);

  ~ExExeUtilGetStatisticsTcb();

  virtual short work();

  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

  ExExeUtilGetStatisticsTdb & getStatsTdb() const
  {
    return (ExExeUtilGetStatisticsTdb &) tdb;
  };
 protected:
  void moveCompilationStatsToUpQueue(CompilationStatsData *cmpStats);

  enum Step
  {
    INITIAL_,
    RETURN_COMPILER_STATS_,
    RETURN_EXECUTOR_STATS_,
    RETURN_OTHER_STATS_,
    SETUP_DETAILED_STATS_,
    FETCH_PROLOGUE_,
    FETCH_FIRST_STATS_ROW_,
    DISPLAY_HEADING_,
    FETCH_STATS_ROW_,
    RETURN_STATS_ROW_,
    FORMAT_AND_RETURN_PERTABLE_STATS_,
    FORMAT_AND_RETURN_ACCUMULATED_STATS_,
    FORMAT_AND_RETURN_ALL_STATS_,
    HANDLE_ERROR_,
    FETCH_EPILOGUE_,
    DONE_,
  };

  Step step_;

  ExStatisticsArea * stats_;

  char * statsQuery_;

  char * statsBuf_;

  char * statsRow_;
  Lng32   statsRowlen_;

  char detailedStatsCQDValue_[40];

  short statsMergeType_;

  short hdfsAccess_;
};

class ExExeUtilGetStatisticsPrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilGetStatisticsTcb;

public:
  ExExeUtilGetStatisticsPrivateState();
  ~ExExeUtilGetStatisticsPrivateState();        // destructor
protected:
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilGetProcessStatisticsTdb
// -----------------------------------------------------------------------
class ExExeUtilGetProcessStatisticsTdb : public ComTdbExeUtilGetProcessStatistics
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilGetProcessStatisticsTdb()
  {}

  virtual ~ExExeUtilGetProcessStatisticsTdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

private:
};


//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilGetUIDTdb
// -----------------------------------------------------------------------
class ExExeUtilGetUIDTdb : public ComTdbExeUtilGetUID
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilGetUIDTdb()
  {}

  virtual ~ExExeUtilGetUIDTdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilGetUIDTcb
// -----------------------------------------------------------------------
class ExExeUtilGetUIDTcb : public ExExeUtilTcb
{
  friend class ExExeUtilGetUIDTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExExeUtilGetUIDTcb(const ComTdbExeUtilGetUID & exe_util_tdb,
                            ex_globals * glob = 0);

  ~ExExeUtilGetUIDTcb();

  virtual short work();

  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

  ExExeUtilGetUIDTdb & getUIDTdb() const
  {
    return (ExExeUtilGetUIDTdb &) tdb;
  };

private:
  enum Step
  {
    INITIAL_,
    RETURN_UID_,
    ERROR_,
    DONE_,
  };

  Step step_;
};

class ExExeUtilGetUIDPrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilGetUIDTcb;

public:
  ExExeUtilGetUIDPrivateState();
  ~ExExeUtilGetUIDPrivateState();        // destructor
protected:
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilGetQIDTdb
// -----------------------------------------------------------------------
class ExExeUtilGetQIDTdb : public ComTdbExeUtilGetQID
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilGetQIDTdb()
  {}

  virtual ~ExExeUtilGetQIDTdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilGetQIDTcb
// -----------------------------------------------------------------------
class ExExeUtilGetQIDTcb : public ExExeUtilTcb
{
  friend class ExExeUtilGetQIDTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExExeUtilGetQIDTcb(const ComTdbExeUtilGetQID & exe_util_tdb,
                            ex_globals * glob = 0);

  ~ExExeUtilGetQIDTcb();

  virtual short work();

  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

  ExExeUtilGetQIDTdb & getQIDTdb() const
  {
    return (ExExeUtilGetQIDTdb &) tdb;
  };

private:
  enum Step
  {
    INITIAL_,
    RETURN_QID_,
    ERROR_,
    DONE_,
  };

  Step step_;
};

class ExExeUtilGetQIDPrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilGetQIDTcb;

public:
  ExExeUtilGetQIDPrivateState();
  ~ExExeUtilGetQIDPrivateState();        // destructor
protected:
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilPopulateInMemStatsTdb
// -----------------------------------------------------------------------
class ExExeUtilPopulateInMemStatsTdb : public ComTdbExeUtilPopulateInMemStats
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilPopulateInMemStatsTdb()
  {}

  virtual ~ExExeUtilPopulateInMemStatsTdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilPopulateInMemStatsTcb
// -----------------------------------------------------------------------
class ExExeUtilPopulateInMemStatsTcb : public ExExeUtilTcb
{
  friend class ExExeUtilPopulateInMemStatsTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExExeUtilPopulateInMemStatsTcb(const ComTdbExeUtilPopulateInMemStats & exe_util_tdb,
                                 ex_globals * glob = 0);

  ~ExExeUtilPopulateInMemStatsTcb();

  virtual short work();

  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

  ExExeUtilPopulateInMemStatsTdb & pimsTdb() const
  {
    return (ExExeUtilPopulateInMemStatsTdb &) tdb;
  };

private:
  enum Step
  {
    INITIAL_,
    PROLOGUE_,
    DELETE_STATS_,
    POPULATE_HISTOGRAMS_STATS_,
    POPULATE_HISTINTS_STATS_,
    EPILOGUE_,
    EPILOGUE_AND_ERROR_RETURN_,
    ERROR_,
    ERROR_RETURN_,
    DONE_,
  };

  Step step_;
};

class ExExeUtilPopulateInMemStatsPrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilPopulateInMemStatsTcb;

public:
  ExExeUtilPopulateInMemStatsPrivateState();
  ~ExExeUtilPopulateInMemStatsPrivateState();        // destructor
protected:
};

///////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilAqrWnrInsertTdb
// -----------------------------------------------------------------------
class ExExeUtilAqrWnrInsertTdb : public ComTdbExeUtilAqrWnrInsert
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilAqrWnrInsertTdb()
  {}

  virtual ~ExExeUtilAqrWnrInsertTdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

///////////////////////////////////////////////////////////////
// ExExeUtilAqrWnrInsertTcb
///////////////////////////////////////////////////////////////
class ExExeUtilAqrWnrInsertTcb : public ExExeUtilTcb
{
public:
  // Constructor
  ExExeUtilAqrWnrInsertTcb(const ComTdbExeUtilAqrWnrInsert & exe_util_tdb,
							const ex_tcb * child_tcb,
							ex_globals * glob = 0);

  ~ExExeUtilAqrWnrInsertTcb();

  virtual Int32 fixup();

  virtual short work();

  virtual short workCancel();

protected:
  enum Step
  {
    INITIAL_,
    LOCK_TARGET_,
    IS_TARGET_EMPTY_,
    SEND_REQ_TO_CHILD_,
    GET_REPLY_FROM_CHILD_,
    CLEANUP_CHILD_,
    ERROR_,
    CLEANUP_TARGET_,
    DONE_
  };

private:
  ExExeUtilAqrWnrInsertTdb & ulTdb() const
  {return (ExExeUtilAqrWnrInsertTdb &) tdb;}

  void setStep(Step s, int lineNum);

  Step step_;
  bool targetWasEmpty_;

};

// -----------------------------------------------------------------------
// ExExeUtilLongRunningTdb
// -----------------------------------------------------------------------
class ExExeUtilLongRunningTdb : public ComTdbExeUtilLongRunning
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilLongRunningTdb()
  {}

  virtual ~ExExeUtilLongRunningTdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

///////////////////////////////////////////////////////////////
// ExExeUtilLongRunningTcb
///////////////////////////////////////////////////////////////
class ExExeUtilLongRunningTcb : public ExExeUtilTcb
{
public:
  // Constructor
  ExExeUtilLongRunningTcb(const ComTdbExeUtilLongRunning & exe_util_tdb,

                       ex_globals * glob = 0);

  ~ExExeUtilLongRunningTcb();

  virtual Int32 fixup();

  virtual short work();

  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

  void registerSubtasks();

  ExExeUtilLongRunningTdb & lrTdb() const{return (ExExeUtilLongRunningTdb &) tdb;};

  short doLongRunning();

  short executeLongRunningQuery();

  short processInitial(Lng32 &rc);

  short processContinuing(Lng32 &rc);

  short finalizeDoLongRunning();

  void addTransactionCount() { transactions_++; };
  Int64 getTransactionCount() { return transactions_; };

  void addRowsDeleted(Int64 rows) { rowsDeleted_ += rows; };
  Int64 getRowsDeleted() { return rowsDeleted_; };

  void setInitial(short initial) { initial_ = initial; };
  short getInitial() { return initial_; };

  void setInitialOutputVarPtrList(Queue * outputVarPtrList)
  { initialOutputVarPtrList_ = outputVarPtrList; };
  Queue * getInitialOutputVarPtrList() { return initialOutputVarPtrList_; };

  void setContinuingOutputVarPtrList(Queue * outputVarPtrList)
  { continuingOutputVarPtrList_ = outputVarPtrList; };
  Queue * getContinuingOutputVarPtrList() { return continuingOutputVarPtrList_; };

  ComDiagsArea *getDiagAreaFromUpQueueTail();

private:
  enum Step
  {
    INITIAL_,
    BEGIN_WORK_,
    LONG_RUNNING_,
    ERROR_,
    DONE_
  };

  Step step_;
  Int64 transactions_;
  Int64 rowsDeleted_;
  short initial_;

  Queue * initialOutputVarPtrList_;
  Queue * continuingOutputVarPtrList_;
  char    * lruStmtAndPartInfo_;
  char    * lruStmtWithCKAndPartInfo_;
  ExTransaction * currTransaction_;
};

class ExExeUtilLongRunningPrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilLongRunningTcb;

public:
  ExExeUtilLongRunningPrivateState();
  ~ExExeUtilLongRunningPrivateState();        // destructor

protected:
};


class ExExeUtilGetRTSStatisticsTcb : public ExExeUtilTcb
{
  friend class ExExeUtilGetStatisticsTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExExeUtilGetRTSStatisticsTcb(const ComTdbExeUtilGetStatistics & exe_util_tdb,
                            ex_globals * glob = 0);

  ~ExExeUtilGetRTSStatisticsTcb();

  virtual short work();

  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

  ExExeUtilGetStatisticsTdb & getStatsTdb() const
  {
    return (ExExeUtilGetStatisticsTdb &) tdb;
  };

  void deleteSqlStatItems(SQLSTATS_ITEM *sqlStatsItem,
                              ULng32  noOfStatsItem);
  void initSqlStatsItems(SQLSTATS_ITEM *sqlStatsItem,
                                  ULng32  noOfStatsItem,
                                  NABoolean initTdbIdOnly);
  NABoolean singleLineFormat() { return singleLineFormat_; } 

private:
  enum Step
  {
    INITIAL_,
    GET_NEXT_STATS_DESC_ENTRY_,
    GET_MASTER_STATS_ENTRY_,
    FORMAT_AND_RETURN_MASTER_STATS_,
    GET_MEAS_STATS_ENTRY_,
    FORMAT_AND_RETURN_MEAS_STATS_,
    GET_ROOTOPER_STATS_ENTRY_,
    FORMAT_AND_RETURN_ROOTOPER_STATS_,
    GET_PERTABLE_STATS_ENTRY_,
    DISPLAY_PERTABLE_STATS_HEADING_,
    FORMAT_AND_RETURN_PERTABLE_STATS_,
    GET_PARTITION_ACCESS_STATS_ENTRY_,
    FORMAT_AND_RETURN_PARTITION_ACCESS_STATS_,
    GET_OPER_STATS_ENTRY_,
    FORMAT_AND_RETURN_OPER_STATS_,
    GET_DP2_OPER_STATS_ENTRY_,
    FORMAT_AND_RETURN_DP2_OPER_STATS_,
    HANDLE_ERROR_,
    DONE_,
    EXPAND_STATS_ARRAY_,
    GET_RMS_STATS_ENTRY_,
    FORMAT_AND_RETURN_RMS_STATS_,
    GET_BMO_STATS_ENTRY_,
    DISPLAY_BMO_STATS_HEADING_,
    FORMAT_AND_RETURN_BMO_STATS_,
    GET_UDR_BASE_STATS_ENTRY_,
    DISPLAY_UDR_BASE_STATS_HEADING_,
    FORMAT_AND_RETURN_UDR_BASE_STATS_,
    GET_REPLICATE_STATS_ENTRY_,
    FORMAT_AND_RETURN_REPLICATE_STATS_,
    GET_REPLICATOR_STATS_ENTRY_,
    FORMAT_AND_RETURN_REPLICATOR_STATS_,
    GET_PROCESS_STATS_ENTRY_,
    FORMAT_AND_RETURN_PROCESS_STATS_,
    GET_HBASE_STATS_ENTRY_,
    GET_SE_STATS_ENTRY_ = GET_HBASE_STATS_ENTRY_,
    DISPLAY_HBASE_STATS_HEADING_,
    FORMAT_AND_RETURN_HBASE_STATS_,
    GET_HIVE_STATS_ENTRY_,
    DISPLAY_HIVE_STATS_HEADING_,
    FORMAT_AND_RETURN_HIVE_STATS_
  };

  Step step_;
  char * statsBuf_;
  short statsCollectType_;
  SQLSTATS_DESC *sqlStatsDesc_;
  Lng32 maxStatsDescEntries_;
  Lng32 retStatsDescEntries_;
  Lng32 currStatsDescEntry_;
  Lng32 currStatsItemEntry_;

  SQLSTATS_ITEM *masterStatsItems_;
  SQLSTATS_ITEM *measStatsItems_;
  SQLSTATS_ITEM* operatorStatsItems_;
  SQLSTATS_ITEM *rootOperStatsItems_;
  SQLSTATS_ITEM* partitionAccessStatsItems_;
  SQLSTATS_ITEM *pertableStatsItems_;
  SQLSTATS_ITEM *rmsStatsItems_;
  SQLSTATS_ITEM *bmoStatsItems_;
  SQLSTATS_ITEM *udrbaseStatsItems_;
  SQLSTATS_ITEM *replicateStatsItems_;
  SQLSTATS_ITEM *replicatorStatsItems_;
  SQLSTATS_ITEM *processStatsItems_;
  SQLSTATS_ITEM *hbaseStatsItems_;
  SQLSTATS_ITEM *hiveStatsItems_;
  Lng32 maxMasterStatsItems_;
  Lng32 maxMeasStatsItems_;
  Lng32 maxOperatorStatsItems_;
  Lng32 maxRootOperStatsItems_;
  Lng32 maxPartitionAccessStatsItems_;
  Lng32 maxPertableStatsItems_;
  Lng32 maxRMSStatsItems_;
  Lng32 maxBMOStatsItems_;
  Lng32 maxUDRBaseStatsItems_;
  Lng32 maxReplicateStatsItems_;
  Lng32 maxReplicatorStatsItems_;
  Lng32 maxProcessStatsItems_;
  Lng32 maxHbaseStatsItems_;
  Lng32 maxHiveStatsItems_;

  NABoolean isHeadingDisplayed_;
  NABoolean isBMOHeadingDisplayed_;
  NABoolean isUDRBaseHeadingDisplayed_;
  NABoolean isHbaseHeadingDisplayed_;
  NABoolean isHiveHeadingDisplayed_;

  static const Int32 numOperStats = 14;
  void formatOperStatItems(SQLSTATS_ITEM operStatsItems[]);
  void formatOperStats(SQLSTATS_ITEM operStatsItems[]);
  //  void formatDouble(SQLSTATS_ITEM stat, char* targetString);
  void formatInt64(SQLSTATS_ITEM stat, char* targetString);
  // void formatWDouble(SQLSTATS_ITEM stat, char* targetString);
  void formatWInt64(SQLSTATS_ITEM stat, char* targetString);
  char *formatTimestamp(char *buf, Int64 inTime);
  char *formatElapsedTime(char *buf, Int64 inTime);
  NABoolean singleLineFormat_;
};

class ExExeUtilGetRTSStatisticsPrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilGetRTSStatisticsTcb;

public:
  ExExeUtilGetRTSStatisticsPrivateState();
  ~ExExeUtilGetRTSStatisticsPrivateState();        // destructor
protected:
};
//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilGetMetadataInfoTdb
// -----------------------------------------------------------------------
class ExExeUtilGetMetadataInfoTdb : public ComTdbExeUtilGetMetadataInfo
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilGetMetadataInfoTdb()
  {}

  virtual ~ExExeUtilGetMetadataInfoTdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilGetMetadataInfoTcb
// -----------------------------------------------------------------------
class ExExeUtilGetMetadataInfoTcb : public ExExeUtilTcb
{
  friend class ExExeUtilGetMetadataInfoTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExExeUtilGetMetadataInfoTcb(const ComTdbExeUtilGetMetadataInfo & exe_util_tdb,
                              ex_globals * glob = 0);

  ~ExExeUtilGetMetadataInfoTcb();

  virtual short work();

  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

  ExExeUtilGetMetadataInfoTdb & getMItdb() const
  {
    return (ExExeUtilGetMetadataInfoTdb &) tdb;
  };

protected:
  enum
  {
    NUM_MAX_PARAMS_ = 25
  };

  enum Step
  {
    INITIAL_,
    DISABLE_CQS_,
    GET_SCHEMA_VERSION_,
    GET_OBJECT_UID_,
    SETUP_QUERY_,
    SETUP_HBASE_QUERY_,
    FETCH_ALL_ROWS_,
    FETCH_ALL_ROWS_FOR_OBJECTS_,
    FETCH_ALL_ROWS_IN_SCHEMA_,
    DISPLAY_HEADING_,
    PROCESS_NEXT_ROW_,
    EVAL_EXPR_,
    RETURN_ROW_,
    ENABLE_CQS_,
    GET_USING_VIEWS_,
    GET_USED_OBJECTS_,
    HANDLE_ERROR_,
    DONE_,
  };

  enum ViewsStep
  {
    VIEWS_INITIAL_,
    VIEWS_FETCH_PROLOGUE_,
    VIEWS_FETCH_ROW_,
    VIEWS_FETCH_EPILOGUE_,
    VIEWS_ERROR_,
    VIEWS_DONE_
  };

  enum AuthIdType
  {
    USERS_ROLES_ = 0,
    ROLES_,
    USERS_
  };

  Step step_;
  ViewsStep vStep_;

  char * metadataQuery_;

  char objectUid_[25];

  char * queryBuf_;
  char * outputBuf_;
  char * headingBuf_;

  char * patternStr_;

  Lng32 numOutputEntries_;

  char * param_[NUM_MAX_PARAMS_];

  NABoolean headingReturned_;

  short displayHeading();

  Lng32 getUsingView(Queue * infoList,

                    // TRUE: shorthand view, FALSE: Materialized View
                    NABoolean isShorthandView,

                    char* &viewName, Lng32 &len);

  Lng32 getUsedObjects(Queue * infoList,
                      NABoolean isShorthandView,
                      char* &viewName, Lng32 &len);
  void setReturnRowCount( Lng32 n) { returnRowCount_ = n; }

  Lng32 getReturnRowCount() {return returnRowCount_;}

  void incReturnRowCount() {returnRowCount_++; }

  Lng32 returnRowCount_;

private:

  NABoolean checkUserPrivs(ContextCli * currConnext, const ComTdbExeUtilGetMetadataInfo::QueryType queryType);
  Int32 getAuthID(
    const char *authName,
    const char *catName,
    const char *schName,
    const char *objName);

  Int32 colPrivsFrag(
    const char *authName,
    const char *catName,
    const NAString &privWhereClause,
    NAString &colPrivsStmt);

  NAString getGrantedPrivCmd(
    const NAString &roleList,
    const char * cat,
    const NAString &inColumn = NAString("object_uid"));

  char * getRoleList(
    const Int32 userID,
    const char *catName,
    const char *schName,
    const char *objName);


};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilGetMetadataInfoComplexTcb
// -----------------------------------------------------------------------
class ExExeUtilGetMetadataInfoComplexTcb : public ExExeUtilGetMetadataInfoTcb
{
  friend class ExExeUtilGetMetadataInfoTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExExeUtilGetMetadataInfoComplexTcb(
       const ComTdbExeUtilGetMetadataInfo & exe_util_tdb,
       ex_globals * glob = 0);

  ~ExExeUtilGetMetadataInfoComplexTcb();

  virtual short work();

protected:
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilGetMetadataInfoVersionTcb
// -----------------------------------------------------------------------
class ExExeUtilGetMetadataInfoVersionTcb : public ExExeUtilGetMetadataInfoTcb
{
  friend class ExExeUtilGetMetadataInfoTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExExeUtilGetMetadataInfoVersionTcb(
       const ComTdbExeUtilGetMetadataInfo & exe_util_tdb,
       ex_globals * glob = 0);

  virtual short work();

protected:
  UInt32 maxObjLen_;
  char formatStr_[100];
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilGetHbaseObjectsTcb
// -----------------------------------------------------------------------
class ExExeUtilGetHbaseObjectsTcb : public ExExeUtilGetMetadataInfoTcb
{
  friend class ExExeUtilGetMetadataInfoTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExExeUtilGetHbaseObjectsTcb(
       const ComTdbExeUtilGetMetadataInfo & exe_util_tdb,
       ex_globals * glob = 0);

  ~ExExeUtilGetHbaseObjectsTcb();

  virtual short work();

 private:
  ExpHbaseInterface * ehi_;
  NAArray<HbaseStr> *hbaseTables_;
  Int32 currIndex_;

  NAString extTableName_;

  char * hbaseName_;
  char * hbaseNameBuf_;
  char * outBuf_;
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilGetHiveMetadataInfoTdb
// -----------------------------------------------------------------------
class ExExeUtilGetHiveMetadataInfoTdb : public ComTdbExeUtilGetHiveMetadataInfo
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilGetHiveMetadataInfoTdb()
  {}

  virtual ~ExExeUtilGetHiveMetadataInfoTdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilGetHiveMetadataInfoTcb
// -----------------------------------------------------------------------
class ExExeUtilGetHiveMetadataInfoTcb : public ExExeUtilGetMetadataInfoTcb
{
  friend class ExExeUtilGetHiveMetadataInfoTdb;

public:
  // Constructor
  ExExeUtilGetHiveMetadataInfoTcb(
       const ComTdbExeUtilGetHiveMetadataInfo & exe_util_tdb,
       ex_globals * glob = 0);

  ~ExExeUtilGetHiveMetadataInfoTcb();

  virtual short work();

  ExExeUtilGetHiveMetadataInfoTdb & getMItdb() const
  {
    return (ExExeUtilGetHiveMetadataInfoTdb &) tdb;
  };

protected:
short fetchAllHiveRows(Queue * &infoList, 
		       Lng32 numOutputEntries,
		       short &rc);

  HiveMetaData* hiveMetaDB_;

  void * mdInfo_;
};

//////////////////////////////////////////////////////////////////////////
class ExExeUtilGetMetadataInfoPrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilGetMetadataInfoTcb;

public:
  ExExeUtilGetMetadataInfoPrivateState();
  ~ExExeUtilGetMetadataInfoPrivateState();        // destructor
protected:
};


// See ExCancel.cpp


//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilShowSetTdb
// -----------------------------------------------------------------------
class ExExeUtilShowSetTdb : public ComTdbExeUtilShowSet
{
 public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilShowSetTdb()
    {}

  virtual ~ExExeUtilShowSetTdb()
    {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilShowSetTcb
// -----------------------------------------------------------------------
class ExExeUtilShowSetTcb : public ExExeUtilTcb
{
  friend class ExExeUtilShowSetTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExExeUtilShowSetTcb(const ComTdbExeUtilShowSet & exe_util_tdb,
                      ex_globals * glob = 0);

  virtual short work();

  ExExeUtilShowSetTdb & ssTdb() const
  {
    return (ExExeUtilShowSetTdb &) tdb;
  };

protected:
  enum Step
  {
    EMPTY_,
    RETURN_HEADER_,
    RETURNING_DEFAULT_,
    DONE_,
    HANDLE_ERROR_,
    CANCELLED_
  };

  Step step_;
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilAQRTdb
// -----------------------------------------------------------------------
class ExExeUtilAQRTdb : public ComTdbExeUtilAQR
{
 public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilAQRTdb()
    {}

  virtual ~ExExeUtilAQRTdb()
    {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilAQRTcb
// -----------------------------------------------------------------------
class ExExeUtilAQRTcb : public ExExeUtilTcb
{
  friend class ExExeUtilAQRTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExExeUtilAQRTcb(const ComTdbExeUtilAQR & exe_util_tdb,
                  ex_globals * glob = 0);

  virtual short work();

  ExExeUtilAQRTdb & aqrTdb() const
  {
    return (ExExeUtilAQRTdb &) tdb;
  };

protected:
  enum Step
  {
    EMPTY_,
    SET_ENTRY_,
    RETURN_HEADER_,
    RETURNING_ENTRY_,
    DONE_,
    HANDLE_ERROR_,
    CANCELLED_
  };

  Step step_;
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilLobExtractTdb
// -----------------------------------------------------------------------
class ExExeUtilLobExtractTdb : public ComTdbExeUtilLobExtract
{
 public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilLobExtractTdb()
    {}

  virtual ~ExExeUtilLobExtractTdb()
    {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilLobExtractTcb
// -----------------------------------------------------------------------
class ExExeUtilLobExtractTcb : public ExExeUtilTcb
{
  friend class ExExeUtilLobExtractTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExExeUtilLobExtractTcb(const ComTdbExeUtilLobExtract & exe_util_tdb,
			 const ex_tcb * child_tcb, 
			 ex_globals * glob = 0);
  
  virtual short work();
  virtual ~ExExeUtilLobExtractTcb();
  virtual void freeResources();
  virtual ExOperStats *doAllocateStatsEntry(CollHeap *heap,
                                            ComTdb *tdb);
  
  ExExeUtilLobExtractTdb & lobTdb() const
  {
    return (ExExeUtilLobExtractTdb &) tdb;
  };
  ExLobGlobals *&getLobGlobals() { return exLobGlobals_;}
 protected:
  enum Step
  {
    EMPTY_,
    GET_NO_CHILD_HANDLE_,
    GET_LOB_HANDLE_,
    RETRIEVE_LOB_LENGTH_,
    EXTRACT_HDFSFILENAME_,
    RETRIEVE_OFFSET_,
    EXTRACT_LOB_DATA_,
    RETURN_STATUS_,
    SEND_REQ_TO_CHILD_,
    GET_REPLY_FROM_CHILD_,
    ERROR_FROM_CHILD_,
    OPEN_CURSOR_,
    READ_CURSOR_,
    CLOSE_CURSOR_,
    RETURN_STRING_,
    INSERT_FROM_STRING_,
    INSERT_FROM_SOURCE_FILE_,
    READ_STRING_FROM_SOURCE_FILE_,
    CREATE_TARGET_FILE_,
    OPEN_TARGET_FILE_,
    CLOSE_TARGET_FILE_,
    COLLECT_STATS_,
    DONE_,
    CANCEL_,
    HANDLE_ERROR_,
    CANCELLED_
  };

  Step step_;
  Lng32 lobHandleLen_;
  char  lobHandle_[2050];
  char lobInputHandleBuf_[4096];
  char lobNameBuf_[1024];
  Lng32 lobNameLen_;
  char * lobName_;
  Lng32 lobType_;
  char * lobData_;
  char * lobData2_;
  Int64 lobDataSpecifiedExtractLen_;
  Int64 lobDataLen_;
  Int64 remainingBytes_;
  Lng32 currPos_;
  Lng32 numChildRows_;
  Int64 requestTag_;
  char lobLoc_[1024];
  ExLobStats lobStats_;
  char statusString_[200];
  fstream indata_;
  ExLobGlobals *exLobGlobals_;
};



//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilLobUpdateTdb
// -----------------------------------------------------------------------
class ExExeUtilLobUpdateTdb : public ComTdbExeUtilLobUpdate
{
 public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilLobUpdateTdb()
    {}

  virtual ~ExExeUtilLobUpdateTdb()
    {}


  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);
private:
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilLobUpdateTcb
// -----------------------------------------------------------------------


class ExExeUtilLobUpdateTcb : public ExExeUtilTcb
{
  friend class ExExeUtilLobUpdateTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExExeUtilLobUpdateTcb(const ComTdbExeUtilLobUpdate & exe_util_tdb,
			 const ex_tcb * child_tcb, 
			 ex_globals * glob = 0);
  
  virtual short work();
  virtual ~ExExeUtilLobUpdateTcb();
  virtual void freeResources();
  virtual ExOperStats *doAllocateStatsEntry(CollHeap *heap,
                                            ComTdb *tdb);
  ExExeUtilLobUpdateTdb & lobTdb() const
  {
    return (ExExeUtilLobUpdateTdb &) tdb;
  };
  ExLobGlobals *&getLobGlobals() { return exLobGlobals_;}
 protected:
  enum Step
    {
      EMPTY_,
      GET_HANDLE_,
      UPDATE_LOB_DATA_,
      EMPTY_LOB_DATA_,
      APPEND_LOB_DATA_,
      RETURN_STATUS_,
      DONE_,
      CANCEL_,
      HANDLE_ERROR_,
      CANCELLED_
    };
  Step step_;
  Lng32 lobHandleLen_;
  char  lobHandle_[2050];
  char lobInputHandleBuf_[4096];
  char lobNameBuf_[1024];
  Lng32 lobNameLen_;
  char * lobName_;
  Lng32 lobType_;
  char * lobData_;
  Int64 lobDataLen_;
  Int64 requestTag_;
  char lobLoc_[1024];
  ExLobStats lobStats_;
  char statusString_[200];
  fstream indata_;
  char lobLockId_[LOB_LOCK_ID_SIZE];
  ExLobGlobals *exLobGlobals_;
};
// -----------------------------------------------------------------------
// ExExeUtilFileUpdateTcb
// -----------------------------------------------------------------------
class ExExeUtilFileExtractTcb : public ExExeUtilLobExtractTcb
{
  friend class ExExeUtilLobExtractTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExExeUtilFileExtractTcb(const ComTdbExeUtilLobExtract & exe_util_tdb,
			  const ex_tcb * child_tcb, 
			  ex_globals * glob = 0);
  
  virtual NABoolean needStatsEntry();

  virtual ExOperStats *doAllocateStatsEntry(CollHeap *heap,
                                            ComTdb *tdb);

  virtual short work();

 private:
  NABoolean  eodReturned_;
};

// -----------------------------------------------------------------------
// ExExeUtilFileLoadTcb
// -----------------------------------------------------------------------
class ExExeUtilFileLoadTcb : public ExExeUtilLobExtractTcb
{
  friend class ExExeUtilLobExtractTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExExeUtilFileLoadTcb(const ComTdbExeUtilLobExtract & exe_util_tdb,
		       const ex_tcb * child_tcb, 
		       ex_globals * glob = 0);
  
  virtual ExOperStats *doAllocateStatsEntry(CollHeap *heap,
                                            ComTdb *tdb);
  virtual short work();

 private:
  Int64 srcFileRemainingBytes_;
};


//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilLobShowddlTdb
// -----------------------------------------------------------------------
class ExExeUtilLobShowddlTdb : public ComTdbExeUtilLobShowddl
{
 public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilLobShowddlTdb()
    {}

  virtual ~ExExeUtilLobShowddlTdb()
    {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilLobShowddlTcb
// -----------------------------------------------------------------------
class ExExeUtilLobShowddlTcb : public ExExeUtilTcb
{
  friend class ExExeUtilLobShowddlTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExExeUtilLobShowddlTcb(const ComTdbExeUtilLobShowddl & exe_util_tdb,
			 ex_globals * glob = 0);

  virtual short work();

  ExExeUtilLobShowddlTdb & lobTdb() const
  {
    return (ExExeUtilLobShowddlTdb &) tdb;
  };
  
 private:
  short fetchRows(char * query, short &rc);
  short returnRows(short &rc);
 
  enum Step
  {
    INITIAL_,
    FETCH_TABLE_SHOWDDL_,
    RETURN_TABLE_SHOWDDL_,
    FETCH_METADATA_SHOWDDL_,
    RETURN_METADATA_SHOWDDL_,
    RETURN_LOB_NAME_,
    FETCH_LOB_DESC_HANDLE_SHOWDDL_,
    RETURN_LOB_DESC_HANDLE_SHOWDDL_,
    FETCH_LOB_DESC_CHUNKS_SHOWDDL_,
    RETURN_LOB_DESC_CHUNKS_SHOWDDL_,
    DONE_,
    HANDLE_ERROR_,
    CANCELLED_
  };

  Step step_;

  char lobMDNameBuf_[1024];
  Lng32 lobMDNameLen_;
  char * lobMDName_;
  
  Lng32 currLobNum_;
  
  char sdOptionStr_[100];
};

class ExExeUtilGetProcessStatisticsTcb : public ExExeUtilGetStatisticsTcb
{
  friend class ExExeUtilGetProcessStatisticsTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExExeUtilGetProcessStatisticsTcb
      (const ComTdbExeUtilGetProcessStatistics & exe_util_tdb,
         ex_globals * glob = 0);
 
  //  ~ExExeUtilGetProcessStatisticsTcb();

  virtual short work();

  ExExeUtilGetProcessStatisticsTdb & getStatsTdb() const
  { 
    return (ExExeUtilGetProcessStatisticsTdb &) tdb;
  };

private:
  enum ProcessStatsStep
  {
     INITIAL_,
     GET_PROCESS_STATS_AREA_,
     GET_PROCESS_STATS_ENTRY_,
     FORMAT_AND_RETURN_PROCESS_STATS_, 
     HANDLE_ERROR_,
     DONE_
  };
   
  ProcessStatsStep step_;
  const ExStatisticsArea *statsArea_;
  ExProcessStats *processStats_;
};
  


//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilHiveMDaccessTdb
// -----------------------------------------------------------------------
class ExExeUtilHiveMDaccessTdb : public ComTdbExeUtilHiveMDaccess
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilHiveMDaccessTdb()
  {}

  virtual ~ExExeUtilHiveMDaccessTdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilHiveMDaccessTcb
// -----------------------------------------------------------------------
class ExExeUtilHiveMDaccessTcb : public ExExeUtilTcb
{
  friend class ExExeUtilHiveMDaccessTdb;

public:
  // Constructor
  ExExeUtilHiveMDaccessTcb(
       const ComTdbExeUtilHiveMDaccess & exe_util_tdb,
       ex_globals * glob = 0);

  ~ExExeUtilHiveMDaccessTcb();

  virtual short work();

  ExExeUtilHiveMDaccessTdb & hiveMDtdb() const
  {
    return (ExExeUtilHiveMDaccessTdb &) tdb;
  };

virtual ex_tcb_private_state *
  allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

protected:
  Lng32 getTypeAttrsFromHiveColType(const char* hiveType,
                                    NABoolean isORC,
                                    Lng32 &fstype,
                                    Lng32 &length,
                                    Lng32 &precision,
                                    Lng32 &scale,
                                    char *sqlType,
                                    char *displayType,
                                    char *charset);
  enum Step
  {
    INITIAL_,
    SETUP_SCHEMAS_,
    GET_ALL_SCHEMAS_,
    GET_ALL_TABLES_,
    GET_ALL_TABLES_IN_SCHEMA_,
    POSITION_,
    FETCH_SCHEMA_,
    FETCH_TABLE_,
    FETCH_COLUMN_,
    FETCH_PKEY_,
    APPLY_PRED_,
    RETURN_ROW_,
    ADVANCE_ROW_,
    ADVANCE_SCHEMA_,
    DONE_,
    HANDLE_ERROR_,
    CANCELLED_
  };

  Step step_;

  HiveMetaData* hiveMD_;

  hive_column_desc * currColDesc_;
  hive_pkey_desc * currPartnDesc_;
  hive_bkey_desc * currKeyDesc_;
  Int32 currSchNum_;
  Int32 currColNum_;

  char * mdRow_;
  LIST (NAText *) schNames_;
  LIST (NAText *) tblNames_;

  char hiveCat_[1024];
  char hiveSch_[1024];
  char schForHive_[1024];
};

//////////////////////////////////////////////////////////////////////////
class ExExeUtilHiveMDaccessPrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilGetMetadataInfoTcb;

public:
  ExExeUtilHiveMDaccessPrivateState();
  ~ExExeUtilHiveMDaccessPrivateState();        // destructor
protected:
};

// -----------------------------------------------------------------------
// ExExeUtilHiveTruncateTdb
// -----------------------------------------------------------------------
class ExExeUtilHiveTruncateTdb : public ComTdbExeUtilHiveTruncate
{
 public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilHiveTruncateTdb()
    {}

  virtual ~ExExeUtilHiveTruncateTdb()
    {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

///////////////////////////////////////////////////////////////
// ExExeUtilHiveTruncateLegacyTcb
///////////////////////////////////////////////////////////////
class ExExeUtilHiveTruncateLegacyTcb : public ExExeUtilTcb
{
 public:
  // Constructor
  ExExeUtilHiveTruncateLegacyTcb(const ComTdbExeUtilHiveTruncate & exe_util_tdb,
                           ex_globals * glob = 0);

  ~ExExeUtilHiveTruncateLegacyTcb();

  virtual void freeResources();
  virtual short work();

  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element
  virtual Int32 fixup();
 private:
  enum Step
    {
      INITIAL_,
      ERROR_,
      DATA_MOD_CHECK_,
      EMPTY_DIRECTORY_,
      DONE_
    };

  ExExeUtilHiveTruncateTdb & htTdb() const
    {return (ExExeUtilHiveTruncateTdb &) tdb;};

  short injectError(const char * val);

  Step step_;

  int   numExistingFiles_;
  ExLobGlobals * lobGlob_;
};

class ExExeUtilHiveTruncateLegacyPrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilHiveTruncateLegacyTcb;

 public:
  ExExeUtilHiveTruncateLegacyPrivateState();
  ~ExExeUtilHiveTruncateLegacyPrivateState();        // destructor
 protected:
};

///////////////////////////////////////////////////////////////
// ExExeUtilHiveTruncateTcb
///////////////////////////////////////////////////////////////
class ExExeUtilHiveTruncateTcb : public ExExeUtilTcb
{
 public:
  // Constructor
  ExExeUtilHiveTruncateTcb(const ComTdbExeUtilHiveTruncate & exe_util_tdb,
                                ex_globals * glob = 0);

  ~ExExeUtilHiveTruncateTcb();

  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element
  virtual void freeResources();
  virtual short work();

 private:
  enum Step
    {
      INITIAL_,
      ALTER_TO_MANAGED_,
      TRUNCATE_TABLE_,
      ALTER_TO_EXTERNAL_,
      ALTER_TO_EXTERNAL_AND_ERROR_,
      ERROR_,
      DONE_
    };

  ExExeUtilHiveTruncateTdb & htTdb() const
    {return (ExExeUtilHiveTruncateTdb &) tdb;};

  Step step_;
};

class ExExeUtilHiveTruncatePrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilHiveTruncateTcb;

 public:
  ExExeUtilHiveTruncatePrivateState();
  ~ExExeUtilHiveTruncatePrivateState();        // destructor
 protected:
};

//////////////////////////////////////////////////////////////
// ExExeUtilHiveQueryTdb
//////////////////////////////////////////////////////////////
class ExExeUtilHiveQueryTdb : public ComTdbExeUtilHiveQuery
{
 public:
  ExExeUtilHiveQueryTdb()
    {}
  virtual ~ExExeUtilHiveQueryTdb()
    {}
  virtual ex_tcb *build(ex_globals *globals);
 private:
};
class ExExeUtilHiveQueryTcb : public ExExeUtilTcb
{
 public:
  ExExeUtilHiveQueryTcb(const ComTdbExeUtilHiveQuery & exe_util_tdb,
                        ex_globals * glob = 0);
  ~ExExeUtilHiveQueryTcb();
  virtual short work();
  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element
 private:
  enum Step
    {
      INITIAL_,
      ERROR_,
      PROCESS_QUERY_,
      DONE_
    };
  ExExeUtilHiveQueryTdb & htTdb() const
    {return (ExExeUtilHiveQueryTdb &) tdb;};
  Step step_;
};
class ExExeUtilHiveQueryPrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilHiveQueryTcb;
 public:
  ExExeUtilHiveQueryPrivateState();
  ~ExExeUtilHiveQueryPrivateState();        // destructor
 protected:
};
//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilHbaseLoadTdb
// -----------------------------------------------------------------------
class ExExeUtilHBaseBulkLoadTdb : public ComTdbExeUtilHBaseBulkLoad
{
 public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilHBaseBulkLoadTdb()
    {}

  virtual ~ExExeUtilHBaseBulkLoadTdb()
    {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};


class ExExeUtilHBaseBulkLoadTcb : public ExExeUtilTcb
{
  friend class ExExeUtilHBaseBulkLoadTdb;
  friend class ExExeUtilPrivateState;

 public:
  // Constructor
  ExExeUtilHBaseBulkLoadTcb(const ComTdbExeUtil & exe_util_tdb,
                            ex_globals * glob = 0);
  ~ExExeUtilHBaseBulkLoadTcb();
  virtual short work();

  ExExeUtilHBaseBulkLoadTdb & hblTdb() const
  {
    return (ExExeUtilHBaseBulkLoadTdb &) tdb;
  };

  virtual short moveRowToUpQueue(const char * row, Lng32 len = -1,
                                 short * rc = NULL, NABoolean isVarchar = TRUE);

  short printLoggingLocation(int bufPos);
 
  void setEndStatusMsg(const char * operation,
                                       int bufPos = 0,
                                       NABoolean   withtime= FALSE);

  short setStartStatusMsgAndMoveToUpQueue(const char * operation,
                                       short * rc,
                                       int bufPos = 0,
                                       NABoolean   withtime = FALSE);
  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element
  void setLoggingLocation();
 private:
  enum Step
    {
    //initial state
      INITIAL_,
      //cleanup leftover files
      PRE_LOAD_CLEANUP_,
      LOAD_START_,
      LOAD_END_,
      LOAD_END_ERROR_,
      PREPARATION_,
      LOADING_DATA_,
      COMPLETE_BULK_LOAD_, //load incremental
      POST_LOAD_CLEANUP_,
      TRUNCATE_TABLE_,
      DISABLE_INDEXES_,
      POPULATE_INDEXES_,
      POPULATE_INDEXES_EXECUTE_,
      UPDATE_STATS_,
      UPDATE_STATS_EXECUTE_,
      RETURN_STATUS_MSG_,
      DONE_,
      HANDLE_ERROR_, DELETE_DATA_AND_ERROR_,
      LOAD_ERROR_
    };


  Step step_;
  Step nextStep_;

  Int64 startTime_;
  Int64 endTime_;
  Int64 rowsAffected_;
  char statusMsgBuf_[BUFFER_SIZE];
  ExpHbaseInterface * ehi_;
  char *loggingLocation_;
  short setCQDs();
  short restoreCQDs();
};

class ExExeUtilHbaseLoadPrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilHBaseBulkLoadTcb;

 public:
  ExExeUtilHbaseLoadPrivateState();
  ~ExExeUtilHbaseLoadPrivateState();        // destructor
 protected:
};




//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilHbaseUnLoadTdb
// -----------------------------------------------------------------------
class ExExeUtilHBaseBulkUnLoadTdb : public ComTdbExeUtilHBaseBulkUnLoad
{
 public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilHBaseBulkUnLoadTdb()
    {}

  virtual ~ExExeUtilHBaseBulkUnLoadTdb()
    {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};


class ExExeUtilHBaseBulkUnLoadTcb : public ExExeUtilTcb
{
  friend class ExExeUtilHBaseBulkUnLoadTdb;
  friend class ExExeUtilPrivateState;

 public:
  // Constructor
  ExExeUtilHBaseBulkUnLoadTcb(const ComTdbExeUtil & exe_util_tdb,
                            ex_globals * glob = 0);
  ~ExExeUtilHBaseBulkUnLoadTcb();

  void freeResources();

  virtual short work();

  ExExeUtilHBaseBulkUnLoadTdb & hblTdb() const
  {
    return (ExExeUtilHBaseBulkUnLoadTdb &) tdb;
  };

  virtual short moveRowToUpQueue(const char * row, Lng32 len = -1,
                                 short * rc = NULL, NABoolean isVarchar = TRUE);

  void setEndStatusMsg(const char * operation,
                                       int bufPos = 0,
                                       NABoolean   withtime= FALSE);

  short setStartStatusMsgAndMoveToUpQueue(const char * operation,
                                       short * rc,
                                       int bufPos = 0,
                                       NABoolean   withtime = FALSE);
  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

  short getTrafodionScanTables();

  short resetExplainSettings();

  char * setSnapshotScanId(char * str2)
  {
    assert (str2 != NULL);
    char  str[30];
    time_t t;
    time(&t);
    struct tm * curgmtime = gmtime(&t);
    strftime(str, 30, "%Y%m%d%H%M%S", curgmtime);
    srand(getpid());
    sprintf (str2,"%s_%d", str, rand()% 1000);
    return str2;
  }
 private:
  struct snapshotStruct
  {
    NAString * fullTableName;
    NAString * snapshotName;
  };
  void createHdfsFileError(Int32 sfwRetCode);
  enum Step
    {
    //initial state
      INITIAL_,
      EMPTY_TARGET_,
      //cleanup leftover files
      UNLOAD_START_,
      UNLOAD_END_,
      UNLOAD_END_ERROR_,
      UNLOAD_,
      MERGE_FILES_,
      RETURN_STATUS_MSG_,
      DONE_,
      HANDLE_ERROR_,
      UNLOAD_ERROR_,
      CREATE_SNAPSHOTS_,
      VERIFY_SNAPSHOTS_,
      DELETE_SNAPSHOTS_
    };

  void setEmptyTarget( NABoolean v)
  {
    emptyTarget_ = v;
  }
  NABoolean getEmptyTarget() const
  {
    return emptyTarget_;
  }
  void setOneFile( NABoolean v)
  {
    oneFile_ = v;
  }
  NABoolean getOneFile() const
  {
    return oneFile_;
  }
  Step step_;
  Step nextStep_;

  Int64 startTime_;
  Int64 endTime_;
  Int64 rowsAffected_;
  char statusMsgBuf_[BUFFER_SIZE];
  NAList<struct snapshotStruct *> * snapshotsList_;
  NABoolean emptyTarget_;
  NABoolean oneFile_;
  ExpHbaseInterface * ehi_;
};

class ExExeUtilHbaseUnLoadPrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilHBaseBulkLoadTcb;

 public:
  ExExeUtilHbaseUnLoadPrivateState();
  ~ExExeUtilHbaseUnLoadPrivateState();        // destructor
 protected:
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilRegionStatsTdb
// -----------------------------------------------------------------------
class ExExeUtilRegionStatsTdb : public ComTdbExeUtilRegionStats
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilRegionStatsTdb()
  {}

  virtual ~ExExeUtilRegionStatsTdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilRegionStatsTcb
// -----------------------------------------------------------------------
class ExExeUtilRegionStatsTcb : public ExExeUtilTcb
{
  friend class ExExeUtilRegionStatsTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExExeUtilRegionStatsTcb(const ComTdbExeUtilRegionStats & exe_util_tdb,
				ex_globals * glob = 0);

  ~ExExeUtilRegionStatsTcb();

  virtual short work();

  ExExeUtilRegionStatsTdb & getDLStdb() const
  {
    return (ExExeUtilRegionStatsTdb &) tdb;
  };

private:
  enum Step
  {
    INITIAL_,
    EVAL_INPUT_,
    COLLECT_STATS_,
    POPULATE_STATS_BUF_,
    EVAL_EXPR_,
    RETURN_STATS_BUF_,
    HANDLE_ERROR_,
    DONE_
  };
  Step step_;

protected:
  Int64 getEmbeddedNumValue(char* &sep, char endChar, 
                            NABoolean adjustLen = TRUE);

  short collectStats(char * tableName);
  short populateStats(Int32 currIndex);

  char * hbaseRootdir_;

  char * tableName_;

  char * inputNameBuf_;

  char * statsBuf_;
  Lng32 statsBufLen_;
  ComTdbRegionStatsVirtTableColumnStruct* stats_;  

  ExpHbaseInterface * ehi_;
  NAArray<HbaseStr> *regionInfoList_;

  Int32 currIndex_;

  Int32 numRegionStatsEntries_;

  char * catName_;
  char * schName_;
  char * objName_;
  char * regionName_;

  NAString extNameForHbase_;
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilRegionStatsFormatTcb
// -----------------------------------------------------------------------
class ExExeUtilRegionStatsFormatTcb : public ExExeUtilRegionStatsTcb
{
  friend class ExExeUtilRegionStatsTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExExeUtilRegionStatsFormatTcb(const ComTdbExeUtilRegionStats & exe_util_tdb,
                                      ex_globals * glob = 0);

  virtual short work();

private:
  enum Step
  {
    INITIAL_,
    COLLECT_STATS_,
    EVAL_INPUT_,
    COMPUTE_TOTALS_,
    RETURN_SUMMARY_,
    RETURN_DETAILS_,
    POPULATE_STATS_BUF_,
    RETURN_REGION_INFO_,
    HANDLE_ERROR_,
    DONE_
  };

  Step step_;

  char * statsTotalsBuf_;
  ComTdbRegionStatsVirtTableColumnStruct* statsTotals_;  

  short initTotals();
  short computeTotals();
};

////////////////////////////////////////////////////////////////////////////
class ExExeUtilRegionStatsPrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilRegionStatsTcb;
  
public:	
  ExExeUtilRegionStatsPrivateState();
  ~ExExeUtilRegionStatsPrivateState();	// destructor
protected:
};


//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilClusterStatsTcb
// -----------------------------------------------------------------------
class ExExeUtilClusterStatsTcb : public ExExeUtilRegionStatsTcb
{
  friend class ExExeUtilClusterStatsTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExExeUtilClusterStatsTcb(const ComTdbExeUtilRegionStats & exe_util_tdb,
                           ex_globals * glob = 0);

  ~ExExeUtilClusterStatsTcb();

  virtual short work();

private:
  enum Step
  {
    INITIAL_,
    EVAL_INPUT_,
    COLLECT_STATS_,
    POPULATE_STATS_BUF_,
    EVAL_EXPR_,
    RETURN_STATS_BUF_,
    HANDLE_ERROR_,
    DONE_
  };
  Step step_;

  short collectStats();
  short populateStats(Int32 currIndex, NABoolean nullTerminate = FALSE);

protected:
  ComTdbClusterStatsVirtTableColumnStruct* stats_;  

};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilLobInfoTdb
// -----------------------------------------------------------------------
class ExExeUtilLobInfoTdb : public ComTdbExeUtilLobInfo
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExeUtilLobInfoTdb()
  {}

  virtual ~ExExeUtilLobInfoTdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilLobInfoTcb
// -----------------------------------------------------------------------
class ExExeUtilLobInfoTcb : public ExExeUtilTcb
{
  friend class ExExeUtilLobInfoTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExExeUtilLobInfoTcb(const ComTdbExeUtilLobInfo & exe_util_tdb,
				ex_globals * glob = 0);

  ~ExExeUtilLobInfoTcb();

  virtual short work();


private:
  enum Step
  {
    INITIAL_,
    EVAL_INPUT_,
    COLLECT_LOBINFO_,
    POPULATE_LOBINFO_BUF_,
    RETURN_LOBINFO_BUF_,
    HANDLE_ERROR_,
    DONE_
  };
  Step step_;

protected:
 
  short collectAndReturnLobInfo(char * tableName, Int32 currLobNum, ContextCli *context);

  ExExeUtilLobInfoTdb & getLItdb() const
  {
    return (ExExeUtilLobInfoTdb &) tdb;
  };
  
  char * tableName_;
  char * inputNameBuf_;
  Int32 currLobNum_;
 
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExExeUtilLobInfoTableTcb
// -----------------------------------------------------------------------
class ExExeUtilLobInfoTableTcb : public ExExeUtilTcb
{
  friend class ExExeUtilLobInfoTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExExeUtilLobInfoTableTcb(const ComTdbExeUtilLobInfo & exe_util_tdb,
				ex_globals * glob = 0);

  ~ExExeUtilLobInfoTableTcb();

  virtual short work();


private:
  enum Step
  {
    INITIAL_,
    EVAL_INPUT_,
    COLLECT_LOBINFO_,
    POPULATE_LOBINFO_BUF_,
    RETURN_LOBINFO_BUF_,
    HANDLE_ERROR_,
    DONE_
  };
  Step step_;

protected:
  Int64 getEmbeddedNumValue(char* &sep, char endChar, 
                            NABoolean adjustLen = TRUE);
  short collectLobInfo(char * tableName, Int32 currLobNum, ContextCli *context);
  short populateLobInfo(Int32 currLobNum, NABoolean nullTerminate = FALSE);

  ExExeUtilLobInfoTdb & getLItdb() const
  {
    return (ExExeUtilLobInfoTdb &) tdb;
  };
 
  char * tableName_;
  char * inputNameBuf_;
  
  char * lobInfoBuf_;
  Lng32 lobInfoBufLen_;
  ComTdbLobInfoVirtTableColumnStruct* lobInfo_;  
  Int32 currLobNum_;
 
};


////////////////////////////////////////////////////////////////////////////
class ExExeUtilLobInfoPrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilLobInfoTcb;
  
public:	
  ExExeUtilLobInfoPrivateState();
  ~ExExeUtilLobInfoPrivateState();	// destructor
protected:
};

////////////////////////////////////////////////////////////////////////////
class ExExeUtilLobInfoTablePrivateState : public ex_tcb_private_state
{
  friend class ExExeUtilLobInfoTableTcb;
  
public:	
  ExExeUtilLobInfoTablePrivateState();
  ~ExExeUtilLobInfoTablePrivateState();	// destructor
protected:
};

short ExExeUtilLobExtractLibrary(ExeCliInterface *cliInterface,char *libHandle, char *cachedLibName,ComDiagsArea *toDiags);
#endif



