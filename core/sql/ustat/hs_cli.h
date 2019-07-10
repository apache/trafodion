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
#ifndef HSCLI_H
#define HSCLI_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         hs_cli.h
 * Description:  Classes for accessing tables using CLI calls.
 * Created:      03/25/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "Platform.h"
#include <stdarg.h>

#include "NAType.h"
#include "Int64.h"
#include "SQLCLIdev.h"

#include "hs_cont.h"
#include "hs_const.h"
#include "hs_log.h"

// -----------------------------------------------------------------------
// Externals.
// -----------------------------------------------------------------------
class HSGlobalsClass;
class HSDataBuffer;
class HSTableDef;
struct HSColDesc;
struct HSColGroupStruct;
struct HSColumnStruct;
class ISFixedChar;
class ISVarChar;
class MCWrapper;
template <class T> class HSPtrObj;
template <class T> class HSPtrArray;

// -----------------------------------------------------------------------
// Functions.
// -----------------------------------------------------------------------
// Execute a standalone dml/ddl operation, without retry.
Lng32 HSFuncExecQuery( const char *dml
                    , short sqlcode = - UERR_INTERNAL_ERROR
                    , Int64 *rowsAffected = NULL
                    , const char *errorToken = HS_QUERY_ERROR
                    , Int64 *srcTabRowCount = NULL
                    , const HSTableDef *tabDef = NULL
                    , short errorToIgnore = 0
                    , NABoolean checkMdam = FALSE
                    , NABoolean inactivateErrorCatcher = FALSE
                    );

// Body shared between HSFuncExecQuery and 
// HSFuncExecTransactionalQueryWithRetry
Lng32 HSFuncExecQueryBody( const char *dml
                    , short sqlcode
                    , Int64 *rowsAffected
                    , const char *errorToken
                    , NABoolean printPlan
                    , const HSTableDef *tabDef
                    , short errorToIgnore
                    , NABoolean checkMdam
                    );

// Execute a standalone dml/ddl operation, with retry. Note
// that this method handles starting and committing or rolling
// back the transaction (as that needs to be part of the retry
// loop since Trafodion often aborts transactions when
// statements fail). Therefore, this function cannot be called
// within a transaction. In fact it guards against this and will
// return an error if it is called within a transaction. One
// consequence is that calls to this function cannot be
// done within the scope of an HSTranController object.
//
// Some thought should go into the choice of whether to use
// this function vs. HSFuncExecQuery. It is appropriate to use
// this function if the effect on the database is idempotent.
// Examples of this are most DDL operations, DELETE, UPDATE
// and INSERT statements that don't depend on prior state and
// don't include non-deterministic semantics such as SAMPLE.
// Operations that have non-transactional effects and non-
// deterministic semantics (such as UPSERT USING LOAD with a
// SAMPLE clause) should not be retried at this level.
//
Lng32 HSFuncExecTransactionalQueryWithRetry( const char *dml
                    , short sqlcode = - UERR_INTERNAL_ERROR
                    , Int64 *rowsAffected = NULL
                    , const char *errorToken = HS_QUERY_ERROR
                    , Int64 *srcTabRowCount = NULL
                    , const HSTableDef *tabDef = NULL
                    , short errorToIgnore = 0
                    , NABoolean checkMdam = FALSE
                    );

Lng32 HSFuncExecDDL( const char *dml
                  , short sqlcode = -UERR_INTERNAL_ERROR
                  , Int64 *rowsAffected = NULL
                  , const char *errorToken = HS_DDL_ERROR
                  , const HSTableDef *tabDef = NULL
                  );
Lng32 HSClearCLIDiagnostics();

// Obtain any JNI diagnostic text stored in the CLI
const char * HSFuncGetJniErrorStr();

// Create histogram tables if they don't exist.
Lng32 CreateHistTables (const HSGlobalsClass* hsGlobal);
Lng32 CreateHistView   (const HSGlobalsClass* hsGlobal);

// Drop the sample table.
Lng32 DropSampleTable();

Lng32 checkMdam(SQLSTMT_ID *stmt);
Lng32 printPlan(SQLSTMT_ID *stmt);
void getRowCountFromStats(Int64 * rowsAffected , const HSTableDef *tabDef = NULL);

struct myVarChar
  {
    short len;
    char val[HS_MAX_BOUNDARY_LEN];

    myVarChar() : len(0) {}
    myVarChar& operator=(const struct myVarChar &other)
      {
        len = other.len;
        memmove(val, other.val, len);
        return *this;
      }

  };

template <class T>
struct boundarySet
  {
    Lng32 size;
    short nullInd[MAX_ROWSET];
    short avgVarCharNullInd[MAX_ROWSET];
    T data[MAX_ROWSET];
    Int64 dataSum[MAX_ROWSET];
    Int64 avgVarCharSize[MAX_ROWSET];

    // This is used in gap processing.
    double gapMagnitude[MAX_ROWSET];

    boundarySet() : size(0) {}
  };

void mergeAverage(NABoolean firstRowset,
                  double rowsetGapAvg, Int64 rowsetGapCount,
                  double &overallGapAvg_, Int64 &overallGapCount_);

// Convert UCS2 string represented by len (# bytes, not chars) and
// val (ptr to first byte of UCS2 string) to double.
double ucsToDouble(Int16 len, const char* val);
double ucsToDouble(myVarChar* ucs);
double ucsToDouble(const HSDataBuffer& hsDataBuff);

/*****************************************************************************/
/* CLASS:    HSSample                                                        */
/* FUNCTION: Used for creating a single sample table, possibly persistent.   */
/*****************************************************************************/
class HSSample
{
  public:
    HSSample(HSTableDef *tableDef,           // input
             Lng32        type,               // input - sample type used for table creation
             double      sampleTblPercent,   // input 
             NABoolean   persistent = FALSE, // input - used to determine whether to drop
             NABoolean   isIUS = FALSE       // input - whether the sample table
                                             // is created for IUS
             ) : objDef(tableDef), sampleType(type), 
                 samplePercent(sampleTblPercent), isPersistent(persistent),
                 isIUS_(isIUS),
                 makeAccessible_(FALSE)
             {};
    ~HSSample() { if (!isPersistent) drop(); }

    // Drop a sample table. Used internally by member function drop(), and from
    // outside the class to drop an IUS persistent sample table that is replaced
    // by a new one. This function only drops the table, it does not remove the
    // row for it in the PERSISTENT_SAMPLES metadata table.
    static Lng32 dropSample(NAString& sampTblName, HSTableDef *sourceTblDef);

    // Set up vars for creating sample table, create table (by calling create), then
    // insert appropriately from source table.
    Lng32 make(NABoolean rowCountIsEstimate, // input - used for assigning tableRowCount
              NAString &sampleTable,        // output
              Int64    &tableRowCnt,        // output - rowcount of original table
              Int64    &sampleRowCnt,       // output
              NABoolean isPersSample=FALSE, //input - used to specify if the 
                                            // method is being called to create
                                            // persistent sample table for fetch
                                            // count or not
              NABoolean unpartitioned=FALSE, //input - used to specify if the 
                                            // method is being called to create
                                            // unpartitioned persistent sample 
              Int64 minRowCtPerPartition = -1 // minimal row RC per partition
              );
    // Create sample table (called by make).
    Lng32 create(NABoolean unpartitioned = FALSE,
                NABoolean isPersSample = FALSE
                );
    Lng32 create(NAString& tblName,
                NABoolean unpartitioned = FALSE,
                NABoolean isPersSample = FALSE);

    // Drop the sample table associated with this object if one exists. The member
    // variable 'sampleTable' will contain the name of the sample table if so.
    Lng32 drop() { return dropSample(sampleTable, objDef); }

    NABoolean isIUS() { return isIUS_; }

  private:
    // Member function
    void makeTableName(NABoolean isPersSample = FALSE);
    NAString getTempTablePartitionInfo(NABoolean unpartitionedSample = FALSE,
                                       NABoolean isPersSample=FALSE);

    // Member variables
    HSTableDef     *objDef;
    NAString        sampleTable;              
    Lng32            sampleType;             
    NABoolean       isPersistent;
    double          samplePercent;
    Int64           TableRowCount;                
    Int64           sampleRowCount;               
    NABoolean       unpartitionedSample;
    NABoolean       isIUS_;
    NABoolean       makeAccessible_;
};


/*****************************************************************************/
/* CLASS:    HSDataBuffer                                                    */
/* FUNCTION: Maintains the interval VARACHR attributes of data.              */
/*               |------------------                                         */
/*               | LEN  | DATA     |                                         */
/*               |------------------                                         */
/*****************************************************************************/
class HSDataBuffer
  {
    public:
      HSDataBuffer() :
          len(0), val(NULL)
        {}

     ~HSDataBuffer() 
        {
          NADELETEBASIC (val, STMTHEAP);
        }
  
      HSDataBuffer(const NAWchar *initVal)
        {
          len = NAWstrlen(initVal) * sizeof(NAWchar);
          val = new (STMTHEAP) char[len];
          memmove(val, initVal, len);
        }
  
      HSDataBuffer& operator=(const HSDataBuffer &other)
        {
          if (other.len > len)
          {
            NADELETEBASIC (val, STMTHEAP);
            val = new (STMTHEAP) char[other.len]; 
          }
          len = other.len;
          memmove(val, other.val, other.len);
          return *this;
        }

      HSDataBuffer& operator=(const NAWchar *dataVal) // replace buffer contents
        {
          short newlen = NAWstrlen(dataVal) * sizeof(NAWchar);
          if (newlen > len)
          {
            NADELETEBASIC (val, STMTHEAP);
            val = new (STMTHEAP) char[newlen];
          }
          len = newlen;
          memmove(val, dataVal, len);
          return *this;
        }

      // Fill the buffer from newVal for newLen bytes. If nullTerm is true,
      // terminate the string with a wide null character that is not included
      // in the stored length. The null terminator is needed for certain
      // operations performed by IUS for intervals read in from the metadata.
      HSDataBuffer& copyFrom(const char *newVal, short newLen, NABoolean nullTerm = FALSE)
        {
          short extra = nullTerm ? 2 : 0;
          if (newLen + extra > len)
          {
            NADELETEBASIC (val, STMTHEAP);
            val = new (STMTHEAP) char[newLen+extra];
          }
          len = newLen;
          memmove(val, newVal, len);
          if (nullTerm)
            *(((NAWchar*)val) + (len/sizeof(NAWchar))) = L'\0';
          return *this;
        }
      
      inline short length() const {return len;}
      inline short numChars() const {return len / sizeof(NAWchar);}
      inline const char* data() const {return val;}
      Lng32 addParenthesis();
      Lng32 append(HSDataBuffer &dataBuffer);

    private:
      short len;
      char *val;
  };

/*****************************************************************************/
/* CLASS:    HSTranMan                                                       */
/* FUNCTION: Transaction manager for all of Update Statistics needs.         */
/* NOTES:    This is a singleton class, which means that there could only be */
/*           one instance of this class.                                     */
/*****************************************************************************/
class HSTranMan
  {
    public:
         static HSTranMan* Instance();
         /* Begin Transaction               */
         Lng32 Begin(const char *title = "",
                     NABoolean inactivateErrorCatcher=FALSE);
         /* Commit Transaction              */
         Lng32 Commit(NABoolean inactivateErrorCatcher=FALSE);
         /* Rollback Transaction            */
         Lng32 Rollback(NABoolean inactivateErrorCatcher=FALSE);

         // This method will tell you if there is currently a transaction
         // running. The transaction could have been started by USER or this
         // instance.
         // T: transaction is running
         // F: no transaction running
         NABoolean InTransaction();

         // This method tells you if the transaction was started by this
         // instance.
         // T: transaction was started by this instance.
         // F: 1) no transaction running or 2) transaction was started by USER.
         inline NABoolean StartedTransaction() const {return transStarted_;}

         void logXactCode(const char* title);

    protected:
         HSTranMan();                     /* ensure only 1 instance of class */

    private:
         NABoolean transStarted_;         /* T: instance started transaction */
         NABoolean extTrans_;             /* T: external transaction running */
         Lng32 retcode_;
         static THREAD_P HSTranMan* instance_;     /* 1 and only 1 instance           */
  };

/********************************************************************************/
/* CLASS:    HSPrologEpilog                                                     */
/* FUNCTION: An instance of this class with automatic storage class should be   */
/*           used at the beginning of each function that is an entry point to   */
/*           ustat processing (UpdateStats, FetchHistograms). The ctor/dtor     */
/*           work in tandem to do any work or satisfy any invariants required   */
/*           at the beginning/end of processing. Currently, the only necessity  */
/*           is to ensure that any transaction initiated by ustat is terminated */
/*           before returning from the ustat code. This is to catch any case    */
/*           where an exception disrupts the normal control flow such that the  */
/*           commit or rollback of a transaction is bypassed. Normally, this is */
/*           precluded by use of an HSTranController object, but there are still*/
/*           a few places where this is not used (some of which are not amenable*/
/*           to its use).                                                       */
/********************************************************************************/
class HSPrologEpilog
{
  public:
    HSPrologEpilog(NAString scopeName);
    virtual ~HSPrologEpilog();

  private:
    NAString   scopeName_;  // Used in log messages to identify function
    HSTranMan* tranMan_;
    HSLogMan*  logMan_;
    NABoolean  enteredWithTranInProgress_;
};

/*****************************************************************************/
/* CLASS:    HSTranController                                                */
/* FUNCTION:                                                                 */
/* Resource management class used to define a transaction's boundaries and   */
/* ensure that the transaction is terminated. The class uses the RAII        */
/* (Resource Acquisition Is Initialization) design pattern, using the ctor   */
/* to begin a transaction and the dtor to end it. An instance of this class  */
/* should be created on the stack in a function or block, to ensure that the */
/* transaction it starts will be terminated when the function or block exits,*/
/* even if there is an early return, as is the case when the HSHandleError   */
/* define is invoked.                                                        */ 
/*                                                                           */
/* The decision as to whether the transaction should be committed or rolled  */
/* back in the dtor is determined by the value of a return code, the address */
/* of which is passed to HSTranController's ctor.                            */
/*                                                                           */
/* Limitations: Unfortunately, Trafodion often aborts transactions when DDL  */
/* or DML statements fail. This precludes using HSTranController in retry    */
/* scenarios as the retry would either fail due to lack of a transaction or  */
/* (worse) succeed in a separate transaction. The latter is worse because    */
/* any other work done in the original transaction would silently be undone. */
/* To guard against this, HSFuncExecTransactionalQueryWithRetry raises an    */
/* error if done within the scope of an HSTranController object.             */
/*****************************************************************************/
class HSTranController
  {
    public:
      HSTranController(const char* title = "", Lng32* returnCode = NULL);
      ~HSTranController();

      // Function to stop current transaction and start a new one.
      void stopStart(const char* title);

    private:
      HSTranMan* tranMan_;
      HSLogMan*  logMan_;
      NAString title_;
      Lng32* returnCodePtr_;
      NABoolean startedTrans_;

      // Function to end transaction.
      void endTrans();
  };

/*****************************************************************************/
/* CLASS:    HSPersSamples                                                   */
/* FUNCTION: Handles persistent samples created for statistics needs.        */
/* NOTES:    This is a singleton class, which means that there could only be */
/*           one instance of this class.                                     */
/*****************************************************************************/
class HSPersSamples
  {
    public:
         // Creates or returns instance.
         static HSPersSamples* Instance(const NAString &catalog,
                                        const NAString &schema);

         Lng32 updIUSUpdateInfo(HSTableDef* tblDef,
                                const char* updHistory,
                                const char* updTimestampStr,
                                const char* updWhereCondition,
                                const Int64* requestedSampleRows = NULL,
                                const Int64* actualSampleRows = NULL);

         Lng32 readIUSUpdateInfo(HSTableDef* tblDef,
                                 char* updHistory,
                                 Int64* updTimestamp);

         // finds a persistent sample table for UID and reason code and returns it in 'table'.
         // (returns ' ' in table if none is found).
         Lng32 find(HSTableDef *tabDef, char reason, NAString &table,
                    Int64 &requestedRows, Int64 &sampleRows, double &sampleRate);

         // finds a persistent sample table for UID and sample size and returns in 'table'.
         // Will remove any 'obsolete' samples found.
         Lng32 find(HSTableDef *tabDef, Int64 &actualRows, NABoolean isEstimate, 
                   Int64 &sampleRows, double allowedDiff, NAString &table);

         // create a persistent sample table and insert an entry for it into list.
         Lng32 createAndInsert(HSTableDef *tabDef, NAString &sampleName,
                              Int64 &sampleRows, Int64 &actualRows, 
                              NABoolean isEstimate, char reason,
                              NABoolean createDandI=FALSE,
                              Int64 minRowsCtPerPartition = -1);

          // remove persistent sample table(s) based on uid, sampleRows, and the
          // allowed difference between the number of rows and sampleRows.
         Lng32 removeMatchingSamples(HSTableDef *tabDef, Int64 sampleRows, double allowedDiff);

         // drop the named sample table and remove its entry from the
         // PERSISTENT_SAMPLES table.
         Lng32 removeSample(HSTableDef* tabDef, NAString& sampTblName,
                            char reason, const char* txnLabel);

         ~HSPersSamples();

    protected:  /* ensure only 1 instance of class */

         HSPersSamples(const NAString &catalog,
                       const NAString &schema);    

         void setCatalogSchema(const NAString &catalog,
                               const NAString &schema); 
      
    private:

         static THREAD_P HSPersSamples* instance_;     /* 1 and only 1 instance           */
         NAString* catalog_;
         NAString* schema_;
         NABoolean triedCreatingSBPersistentSamples_;
  };

/*****************************************************************************/
/* CLASS:    HSPersData                                                   */
/* FUNCTION: Handles persistent data tables created for statistics needs.        */
/* NOTES:    This is a singleton class, which means that there could only be */
/*           one instance of this class.                                     */
/*****************************************************************************/
class HSPersData
  {
    public:
         static HSPersData* Instance(const NAString &catalog);  // creates or returns instance.

         Lng32 insert(NAString &tableName, ULng32 objectSubId, ULng32 seqNum, NAString &data);
         Lng32 remove(NAString &tableName, ULng32 objectSubId, ULng32 seqNum); 
         Lng32 fetch(NAString &tableName, ULng32 objectSubId, ULng32 seqNum, NAString &data); 

    protected:
         HSPersData();                     /* ensure only 1 instance of class */
    private:
         static THREAD_P HSPersData* instance_;     /* 1 and only 1 instance           */
         static THREAD_P NAList<NAString>* persDataList_;
         static THREAD_P NAString* catalog_;
         static THREAD_P NAString* schema_;
  };

// -----------------------------------------------------------------------
// Class to run a static cli statement without output host variables.
// -----------------------------------------------------------------------
#define HS_MODULE "HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.SQLHIST_N29_000"
#define HS_MODULE_LENGTH  50  // more than needed
#define HS_STMTID "HS_CLI_DYNSTMT"
#define HS_INTERVAL_STMT_ID "HS_INTERVAL_STMT_ID"
#define HS_STMTID_LENGTH  50  
#define HS_FUNC_EXEC_QUERY_STMTID "HS_FUNC_EXEC_QUERY_DYNSTMT"

class HSCliStatement {

public:

  // Encoding indexes for static SQL statements.
  // This index is a 32-bit number
  // First 16 bits stores the number of host variables for that statement.
  // Last 16 bits stores the statement index in the statementNames array.
  //
  // Literal ...1?? stands for operations on HISTOGRAMS table, while
  // ...2?? for operations on HISTOGRAM_INTERVALS table.
  //
  // Statement names and number of host variables must match those in
  // sqlhist.mdf file.
  enum statementIndex {
    BEGINWORK           = 0x00000000,
    COMMITWORK          = 0x00000001,
    ROBACKWORK          = 0x00000002,
    PRINTPLAN           = 0x00010003,
    INSERT101_MP        = 0x000C0004,   INSERT101_MX      = 0x000C0005,   INSERT101_MX_2300       = 0x00190006,
    INSERT201_MP        = 0x00070007,   INSERT201_MX      = 0x00070008,   INSERT201_MX_2300       = 0x000E0009, 
    DELETE101_MP        = 0x0002000A,   DELETE101_MX      = 0x0002000B,   DELETE101_MX_2300       = 0x0002000C,
    DELETE201_MP        = 0x0002000D,   DELETE201_MX      = 0x0002000E,   DELETE201_MX_2300       = 0x0002000F,
    DELETE102_MP        = 0x00040010,   DELETE102_MX      = 0x00040011,   DELETE102_MX_2300       = 0x00040012,
    DELETE202_MP        = 0x00040013,   DELETE202_MX      = 0x00040014,   DELETE202_MX_2300       = 0x00040015, 
    SECURITY101_MP      = 0x00010016,   SECURITY101_MX    = 0x00010017,   SECURITY101_MX_2300     = 0x00010018,
    SECURITY201_MP      = 0x00010019,   SECURITY201_MX    = 0x0001001A,   SECURITY201_MX_2300     = 0x0001001B,
    CURSOR101_MP        = 0x0002001C,   CURSOR101_MX      = 0x0002001D,   CURSOR101_MX_2300       = 0x0002001E,
    CURSOR101_NOMC_MP   = 0x0002001F,   CURSOR101_NOMC_MX = 0x00020020,   CURSOR101_NOMC_MX_2300  = 0x00020021,
    CURSOR102_MP        = 0x00020022,   CURSOR102_MX      = 0x00020023,   CURSOR102_MX_2300       = 0x00020024,
    CURSOR103_MP        = 0x00020025,   CURSOR103_MX      = 0x00020026,   CURSOR103_MX_2300       = 0x00020027,
    CURSOR201_MP        = 0x00020028,   CURSOR201_MX      = 0x00020029,   CURSOR201_MX_2300       = 0x0002002A, 
    SHOWHIST_MP         = 0x0003002B,   SHOWHIST_MX       = 0x0003002C,   SHOWHIST_MX_2300        = 0x0003002D, 
    SHOWINT_MP          = 0x0003002E,   SHOWINT_MX        = 0x0003002F,   SHOWINT_MX_2300         = 0x00030030,
    ROWCOUNT_FROM_STATS = 0x00010031,   CURSOR104_MX_2300 = 0x00020032,   INSERT104_MX_2300       = 0x00190033,
    CURSOR105_MX_2300   = 0X00030034,
    DELETE_PST          = 0X00020035,   INSERT_PST        = 0X000D0036,   CURSOR_PST              = 0X00040037,
    CURSOR_PST_REASON_CODE = 0x00030038,  
    CURSOR107_MX_2300   = 0x00030039,   
    DELETE_PDT          = 0X0004003A,   INSERT_PDT        = 0X0005003B,   CURSOR_PDT              = 0X0004003C,
    CURSOR_PST_UPDATE_INFO = 0x0002003D, UPDATE_PST_UPDATE_INFO = 0x0004003E,
    CURSOR103_MX_2300_X    = 0x0002003F
  };                       

  enum { MAX_NUM_HOST_VARIABLES = 0x19 };
                                      
  HSCliStatement( statementIndex ix,
                  char *  in01 = NULL, char *  in02 = NULL, char *  in03 = NULL,
                  char *  in04 = NULL, char *  in05 = NULL, char *  in06 = NULL,
                  char *  in07 = NULL, char *  in08 = NULL, char *  in09 = NULL,
                  char *  in10 = NULL, char *  in11 = NULL, char *  in12 = NULL,
                  char *  in13 = NULL, char *  in14 = NULL, char *  in15 = NULL,
                  char *  in16 = NULL, char *  in17 = NULL, char *  in18 = NULL,
                  char *  in19 = NULL, char *  in20 = NULL, char *  in21 = NULL,
                  char *  in22 = NULL, char *  in23 = NULL, char *  in24 = NULL,
                  char *  in25 = NULL
                );                
  ~HSCliStatement();              

  Lng32 open();
  Lng32 fetch(Lng32 numParam,
             void *  out01,        void *  out02 = NULL, void *  out03 = NULL,
             void *  out04 = NULL, void *  out05 = NULL, void *  out06 = NULL,
             void *  out07 = NULL, void *  out08 = NULL, void *  out09 = NULL,
             void *  out10 = NULL, void *  out11 = NULL, void *  out12 = NULL,
             void *  out13 = NULL, void *  out14 = NULL, void *  out15 = NULL,
             void *  out16 = NULL, void *  out17 = NULL, void *  out18 = NULL,
             void *  out19 = NULL, void *  out20 = NULL, void *  out21 = NULL,
             void *  out22 = NULL, void *  out23 = NULL, void *  out24 = NULL,
             void *  out25 = NULL
            );
  Lng32 close();
  Lng32 execFetch(const char *dml, NABoolean hideError = FALSE);


  static const Int64 largeZero = 0;             // const Int64(0)
  static const Int64 largeMinusOne = -1;         // const Int64(-1)
  static const short minusOne = -1;              // const -1;

  static THREAD_P Lng32 statementNum;           // statement number


private:

  SQLSTMT_ID stmt_;
  SQLDESC_ID desc_;
  SQLDESC_ID *pInputDesc_;

  char stmtID_[HS_STMTID_LENGTH];     // statement name.
  char descID_[HS_STMTID_LENGTH];     // input descriptor name.

  // number of input host variables.
  Lng32 numVars_;

  // Addresses of input host variables.
  char * in01_, * in02_, * in03_, * in04_,
       * in05_, * in06_, * in07_, * in08_,
       * in09_, * in10_, * in11_, * in12_,
       * in13_, * in14_, * in15_, * in16_,
       * in17_, * in18_, * in19_, * in20_,
       * in21_, * in22_, * in23_, * in24_,
       * in25_;

  NABoolean validStmt_;

  Lng32 retcode_;
};


/*****************************************************************************/
/* CLASS:    HSinsertHist                                                    */
/* FUNCTION: Inserts data into the HISTOGRAM table using ROWSETS.            */
/* NOTES:    File SQLHIST.MDF and this class have dependencies. Make sure if */
/*           any changes are made to either file, the other is maintained    */
/*****************************************************************************/
class HSinsertHist
  {
    public:
      HSinsertHist(const char *stmtID,
                   const char *histTable);
      HSinsertHist(const char *histTable);
      ~HSinsertHist();

      Lng32 initialize();                  //initialize the ROWSET pointers
      Lng32 prepareDynamic();              //after initialize, if using dynamic query
      Lng32 addRow(const Int64 table_uid,  //Add single data row into ROWSET
                  const ULng32 histogram_id,
                  const Lng32 col_position,
                  const Lng32 column_number,
                  const Lng32 colcount,
                  const Int16 interval_count,
                  const Int64 rowcount,
                  const Int64 total_uec,
                  const char *stats_time,
                  const HSDataBuffer &low_value,
                  const HSDataBuffer &high_value,
                  const char *read_time,  
                  const short read_count,
                  const Int64 sample_secs,
                  const Int64 col_secs,
                  const short sample_percent,
                  const double cv,
                  const char reason,
                  const Int64 v1 = 0,
                  const Int64 v2 = 0,
                  const Int64 v3 = 0,
                  const Int64 v4 = 0,
                  const HSDataBuffer &v5 = HSDataBuffer(L"empty"),
                  const HSDataBuffer &v6 = HSDataBuffer(L"empty")
                  );
      Lng32 flush();                       //writes all data rows into table
      void print();
      
    private:
      SQLSTMT_ID stmt_;
      SQLDESC_ID desc_;

      //==========================
      // Used only for dynamic query version.
      SQLDESC_ID srcDesc_;
      NABoolean stmtAllocated_;
      NABoolean srcDescAllocated_;
      NABoolean inputDescAllocated_;
      NAString stmtText_;
      void setText();
      //==========================
    
      char stmtID_[HS_STMTID_LENGTH];     // statement name.
      char descID_[HS_STMTID_LENGTH];     // input descriptor name.
    
      Int64 tableUid_[MAX_ROWSET];
      ULng32 histID_[MAX_ROWSET];
      Lng32 colPosition_[MAX_ROWSET];
      Lng32 colNumber_[MAX_ROWSET];
      Lng32 colcount_[MAX_ROWSET];
      Int16 intCount_[MAX_ROWSET];
      Int64 rowCount_[MAX_ROWSET];
      Int64 totalUEC_[MAX_ROWSET];
      char statsTime_[MAX_ROWSET][TIMESTAMP_CHAR_LEN+1];
      NAWchar lowValue_[MAX_ROWSET][HS_MAX_UCS_BOUNDARY_CHAR+1];
      NAWchar hiValue_[MAX_ROWSET][HS_MAX_UCS_BOUNDARY_CHAR+1];
      // automation histogram columns
      char readTime_[MAX_ROWSET][TIMESTAMP_CHAR_LEN+1];
      short readCount_[MAX_ROWSET];
      Int64 sampleSecs_[MAX_ROWSET];
      Int64 colSecs_[MAX_ROWSET];
      short samplePercent_[MAX_ROWSET];
      double cv_[MAX_ROWSET];
      char reason_[MAX_ROWSET];
      Int64 v1_[MAX_ROWSET];
      Int64 v2_[MAX_ROWSET];
      Int64 v3_[MAX_ROWSET];
      Int64 v4_[MAX_ROWSET];
      wchar_t v5_[MAX_ROWSET][HS_MAX_UCS_BOUNDARY_CHAR+1];
      wchar_t v6_[MAX_ROWSET][HS_MAX_UCS_BOUNDARY_CHAR+1];
      
      const char *tableName_;
      // number of rows in ROWSET
      Lng32 numRows_;
    
      Lng32 retcode_;
  };




/*****************************************************************************/
/* CLASS:    HSinsertHistint                                                 */
/* FUNCTION: Inserts data into the HISTOGRAM_INTERVALS table using ROWSETS.  */
/* NOTES:    File SQLHIST.MDF and this class have dependencies. Make sure if */
/*           any changes are made to either file, the other is maintained    */
/*****************************************************************************/
class HSinsertHistint
  {
    public:
      HSinsertHistint(const char *stmtID,
                      const char *histIntTable);
      HSinsertHistint(const char *histIntTable);
      ~HSinsertHistint();
    
      Lng32 initialize();                  //initialize the ROWSET pointers
      Lng32 prepareDynamic();              //after initialize, if using dynamic query
      Lng32 addRow(const Int64 table_uid,      //Add single data row into ROWSET
                  const ULng32 histogram_id,
                  const Int16 interval_number,
                  const Int64 interval_rowcount,
                  const Int64 interval_uec,
                  const HSDataBuffer &interval_boundary,
                  // the following are added in R2.3
                  const double std_dev_of_freq,
                  const Int64 interval_MFV_rowcount = 0,
                  const Int64 interval_MFV2_rowcount = 0,
                  const Int64 v3 = 0,
                  const Int64 v4 = 0,
                  const HSDataBuffer &mostFreqVal = HSDataBuffer(L"empty"),
                  const HSDataBuffer &v6 = HSDataBuffer(L"empty")
                  );
      Lng32 flush();                           //writes all data rows into table
      void print();
      
      //==========================
      // Used only for dynamic query version.
      SQLDESC_ID srcDesc_;
      NABoolean stmtAllocated_;
      NABoolean srcDescAllocated_;
      NABoolean inputDescAllocated_;
      NAString stmtText_;
      void setText();
      //==========================
    
    private:
      SQLSTMT_ID stmt_;
      SQLDESC_ID desc_;
    
      char stmtID_[HS_STMTID_LENGTH];     // statement name.
      char descID_[HS_STMTID_LENGTH];     // input descriptor name.
    
      Int64 tableUid_[MAX_ROWSET];
      ULng32 histID_[MAX_ROWSET];
      Int16 intNumber_[MAX_ROWSET];
      Int64 intRowcount_[MAX_ROWSET];
      Int64 intUec_[MAX_ROWSET];
      NAWchar intBoundary_[MAX_ROWSET][HS_MAX_UCS_BOUNDARY_CHAR+1];

      //the following columns are added in R2.3
      double stdDevOfFreq_[MAX_ROWSET];
      Int64 intMFVrowcount_[MAX_ROWSET];
      Int64 intMFV2rowcount_[MAX_ROWSET];
      Int64 v3_[MAX_ROWSET];
      Int64 v4_[MAX_ROWSET];
      wchar_t mostFreqVal_[MAX_ROWSET][HS_MAX_UCS_BOUNDARY_CHAR+1];
      wchar_t v6_[MAX_ROWSET][HS_MAX_UCS_BOUNDARY_CHAR+1];
      
      const char *tableName_;
      // number of rows in ROWSET
      Lng32 numRows_;
      Lng32 retcode_;
  };

/*****************************************************************************/
/* CLASS:    HSinsertEmptyHist                                               */
/* FUNCTION: Inserts empty entries into the HISTOGRAM table                  */
/* NOTES:    use HSCliStatement to check existing empty histograms           */
/*           use HSinsertHist to insert new ones                             */
/*           currently only used for multi-column histograms              */
/*****************************************************************************/
class HSinsertEmptyHist
  {
    public:
      HSinsertEmptyHist(const Int64 tableUID,
                        const char *tableName,
                        const char *histTable)
      : tableUID_(tableUID),
        tableName_(tableName),
        histTable_(histTable),
        colCount_(0)
      {};

      virtual ~HSinsertEmptyHist() {};

      Lng32 addColumn(const Lng32 columnNumber);
      Lng32 insert();                       
      
    private:
      HSinsertEmptyHist(const HSinsertEmptyHist&);              // add to prevent the default one to be used
      HSinsertEmptyHist& operator=(const HSinsertEmptyHist&);   // add to prevent the default one to be used
      const Int64 tableUID_;
      const char *tableName_;
      const char *histTable_;
      Lng32 colNumber_[MAX_MC_COLUMNS];
      Lng32 colCount_;

  };

// -----------------------------------------------------------------------
// Cursor class to fetch boundaries, histid, rowcount, and update
// bitmaps, row counters.
// -----------------------------------------------------------------------
class HSCursor :public NABasicObject
  {

public:

  HSCursor(NAHeap *h=STMTHEAP, const char* stmtName = HS_STMTID);
  ~HSCursor();

  // open the cursor.
  inline Lng32 open( const char *clistr
                  , char *&dataBuf
                  , HSColDesc *&colDesc
                  )
  {
    retcode_ = prepare(clistr);

    HSFilterWarning(retcode_);
    if (retcode_)
      {
        dataBuf = NULL;
        colDesc = NULL;
        return retcode_;
      }
    dataBuf = dataBuf_;
    colDesc = colDesc_;
    return 0;
  }

  // get one row.
  inline Lng32 getRow()
  {
    HSLogMan *LM = HSLogMan::Instance();
    LM->StartTimer("SQL_EXEC_Fetch() called from HSinsertEmptyHist::getRow()");
    retcode_ = SQL_EXEC_Fetch(stmt_,outputDesc_, 0,
                              0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
    LM->StopTimer();
    if (retcode_ < 0)
      {
        HSFuncMergeDiags(-UERR_INTERNAL_ERROR, "getRow()", NULL, TRUE);
        return -1;
      }
    else
      return retcode_;
  }

  Lng32 fetchRowset();

  inline Lng32 rowsetSize() {return rowsetSize_;}

  // Determine boundaries of histograms.
  Lng32 fetchBoundaries(HSColGroupStruct *group, Int64 &rowCount, Lng32 intCount,
                    NABoolean sampleUsed);
  Lng32 prepareRowset(const char *cliStr, NABoolean orderAndGroup, 
                     HSColGroupStruct *group, Lng32 maxRows);

  Lng32 setRowsetPointers(HSColGroupStruct *group, Lng32 maxRows);
  
  // Fetch a numerical type column from a table.
  Lng32 fetchNumColumn( const char *clistr
                     , Lng32 *pSmallValue
                     , Int64 *pLargeValue = NULL
                     );
  // Fetch a char type column from a table.
  Lng32 fetchCharNumColumn( const char *clistr
                         , NAString &value1
                         , Int64 &value2
                         , double &value3
                         );

  // Functions below used when the HSCursor is instantiated for a dynamic query.
  Lng32 prepareQuery(const char *cliStr, Lng32 numParams, Lng32 numResults);
  Lng32 open(Lng32 numParams = 0, void *in01 = NULL);
  Lng32 fetch(Lng32 numResults,
              void* out01,        void* out02 = NULL, void* out03 = NULL,
              void* out04 = NULL, void* out05 = NULL, void* out06 = NULL,
              void* out07 = NULL, void* out08 = NULL, void* out09 = NULL,
              void* out10 = NULL, void* out11 = NULL, void* out12 = NULL,
              void* out13 = NULL, void* out14 = NULL, void* out15 = NULL,
              void* out16 = NULL, void* out17 = NULL, void* out18 = NULL,
              void* out19 = NULL, void* out20 = NULL, void* out21 = NULL,
              void* out22 = NULL, void* out23 = NULL, void* out24 = NULL,
              void* out25 = NULL);
  Lng32 close();

  SQLSTMT_ID* getStmt() {return stmt_;}
  SQLDESC_ID* getOutDesc() {return outputDesc_;}
  SQLDESC_ID* getInDesc() {return inputDesc_;}
  Lng32 setCursorName(const char* name);

  boundarySet<myVarChar>* getBoundaryRowset() const
    {
      return boundaryRowSet_;
    }

private:
  Lng32 prepareRowsetInternal(const char *cliStr, NABoolean orderAndGroup, 
                     HSColGroupStruct *group, Lng32 maxRows);
  struct groupMap {

    groupMap()
      : datatype(-1), base(-1), len(-1), startCol(-1), filler(0) {}
    Lng32 datatype;
    Lng32 scale;
    Lng32 base;
    Lng32 len;
    Lng32 startCol;
    Lng32 filler;
  };

  SQLSTMT_ID *stmt_;
  SQLDESC_ID *srcDesc_;
  SQLDESC_ID *outputDesc_;
  SQLDESC_ID *inputDesc_;

  Int32 stmtAllocated_;
  Int32 srcDescAllocated_;
  Int32 outputDescAllocated_;
  Int32 inputDescAllocated_;

  SQLCLI_OBJ_ID *cursorName_;

  // Number of rows in last rowset read by fetchRowset().
  Lng32 rowsetSize_;

  // Column descriptor for output rows.
  HSColDesc *colDesc_;
  Lng32 numEntries_;
  HSPtrObj<NAType> *ptrNAType_;

  // output buffer to hold a row returned from CLI fetch.
  char *dataBuf_;
  Lng32 outputDataLen_;

  // Each returned row may contain multiple column groups.
  // groupMap describes base, len, startCol for each group in the
  // output buffer.
  groupMap *group_;
  
  boundarySet<myVarChar> *boundaryRowSet_;
  SQLCLI_QUAD_FIELDS *rowset_fields_;

  Int32 closeStmtNeeded_;

  Lng32 retcode_;
  
  NAHeap *heap_;

  NABoolean lastFetchReturned100_;

  // prepare a dynamic sql statement.
  Lng32 prepare(const char *cliStr, const Lng32 outDescEntries = 500);

  // Construct a NAType for each column in the output row.
  Lng32 buildNAType();

  HSCursor(const HSCursor &other);
  HSCursor& operator=(const HSCursor &other);
};


// Return the value as a double for computation of the magnitude
// of a gap. See also the template specializations that follow this.
//
template <class T>
static double getValueAsDouble(T* valPtr)
{
  return (double)(*valPtr);
}

// This specialization of the foregoing template handles the
// myVarChar type, which is used for columns not processed by
// internal sort.
//
static double getValueAsDouble(myVarChar* valPtr)
{
  return ucsToDouble(valPtr);
}

// This won't be called, just needs to exist to avoid compilation error for
// a template instantiation using ISFixedChar.
static double getValueAsDouble(ISFixedChar* valPtr)
{
  assert(FALSE);
  return 0;
}

// This won't be called, just needs to exist to avoid compilation error for
// a template instantiation using ISVarChar.
static double getValueAsDouble(ISVarChar* valPtr)
{
  assert(FALSE);
  return 0;
}

// This won't be called, just needs to exist to avoid compilation error for
// a template instantiation using ISVarChar.
static double getValueAsDouble(MCWrapper* valPtr)
{
  assert(FALSE);
  return 0;
}


// NOTE: The following function has to be defined in the header file so the
//       compiler can see the definition to generate instantiations for
//       the different types. Compilation fails if this is not placed here.
//
/****************************************************************************/
/* METHOD:  profileGaps()                                                   */
/* PURPOSE: Go through the distinct values that make up a rowset, and set   */
/*          gap magnitude for each. The running overall gap magnitude       */
/*          average is updated with the information from this rowset.       */
/* PARAMS:  group(in)               -- Group the rowset is for.             */
/*          boundaryRowSet(in)      -- Contains the distinct values and     */
/*                                     their frequencies for this rowset.   */
/*          overallGapAvg(in/out)   -- Average gap size for all values      */
/*                                     seen so far. Updated with            */
/*                                     information from this rowset.        */
/*          overallGapCount(in/out) -- Total number of gaps considered so   */
/*                                     far. Updated with the count from     */
/*                                     this rowset.                         */
/*          firstRowset(in)         -- TRUE if this is the first rowset for */
/*                                     this group. If this is NOT the case, */
/*                                     the static prevValue has the last    */
/*                                     value from the previous rowset.      */
/****************************************************************************/
template <class T>
void profileGaps(HSColGroupStruct *group, boundarySet<T> *boundaryRowSet,
                 double &overallGapAvg, Int64 &overallGapCount,
                 NABoolean firstRowset);
#endif /* HSCLI_H */
