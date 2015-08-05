/* -*-C++-*-
 *****************************************************************************
 *
 * File:         dstestpoint.h
 * Description:  Interface to the testing hooks that are used to test the
 *               recovery of failed utility operations.
 *
 *
 * Created:      December 12, 2003
 * Modified:	 July 20, 2006
 * Language:     C++
 *
 *
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
 *
 *****************************************************************************
*/
#ifndef NATESTPOINT_H
#define NATESTPOINT_H

#include "Collections.h"
#include "Platform.h"
#include "NABoolean.h"
#include "NAString.h"
#include "ComASSERT.h"

#define DETAILS_NOT_DEFINED -1
#define RQST_LEN 10
#define IDS_PM_ERROR_MSG_TEST_POINT 0x00005207L //MSG_20999 from sqlutils_msg.h
#define NSK_FILE_SYSTEM_ERROR 8551

// Lists the available test points.
enum ETestPointValue 
{
  DS0_UNKNOWN_TEST_POINT                        = 0,

  // Note that test points 1-4, 11-12, and 32-33 are currently
  // still used by the populate index utility - 8/22/2005.

  // Range 1 - 99 reserved for DDOL                     number of iterators expected
  DDOL51_EXECUTE_DDL_RETRY_LOOP                 = 51,   // 1

  // Range 100 - 399 reserved for Modify                number of iterators expected
  // Modify errors 100 - 299: normal flow              (no iterator if not mentioned)
  MD102_READY_TO_CREATE_TGT_PARTN 		= 102,  // 0 - e.g. TESTPOINT_102
  MD104_TGT_PARTN_CREATED 			= 104,
  MD106_READY_FOR_DM_PHASE 			= 106,
  MD110_READY_FOR_COMMIT_PHASE 			= 110,
  MD112_DM_DONE 				= 112,
  MD113_DM_DONE_NO_DDL_LOCK_UPDATE 		= 113,
  MD114_DM_ONLINE_READY_FOR_DM 			= 114,
  MD120_REUSE_PARTNS_CORRUPT 			= 120,
  MD126_COMMIT_SOURCE_PARTN_DROPPED 		= 126,
  MD130_REUSE_SOURCE_DATA_REMOVED 		= 130,
  MD150_REUSE_DONE 				= 150,
  MD174_COMMIT_IXMAP_ARRAY_UPDATED 		= 174,
  MD180_COMMIT_MIDDLE_PHASE 			= 180,
  MD181_COMMIT_XCL_LOCK_TABLE 			= 181,
  MD182_COMMIT_UPDATE_MAX_EXTENTS 		= 182,
  MD183_COMMIT_XCL_LOCK_FOR_INDEX 		= 183,
  MD184_COMMIT_SOURCE_DATA_REMOVED 		= 184,
  MD185_COMMIT_FINAL_AUDIT_APPLIED 		= 185,
  MD186_COMMIT_READY_TO_END_AFR 		= 186,
  MD187_COMMIT_APPLY_ROW_HIDING 		= 187,
  MD188_COMMIT_READY_TO_COMMIT 			= 188,
  MD197_COMMIT_END_AFR 				= 197,
  MD202_STARTED_ORSERV 				= 202,
  MD208_STARTED_ORSERV_UPDATED_DDL_LOCK 	= 208,
  MD210_BULK_IO_COPY_RETRY_LOOP                 = 210,  // 1 - e.g. TESTPOINT_210_1
  MD212_SET_EOF_ZERO_RETRY_LOOP                 = 212,  // 1
  MD220_COMMIT_RETRY_LOOP			= 220,  // 1
  MD230_COMMIT_RETRY_LOOP			= 230,  // 1

  // Modify errors 300 - 399:  error flow
  MD300_RECOVERY_STARTED 			= 300,
  MD301_READY_TO_REMOVE_DDL_LOCK 		= 301,
  MD302_REUSE_RESUME_READY_TO_COMMIT 		= 302,
  MD303_REUSE_RESUME_COMMITTED 			= 303,
  MD304_REUSE_RESUME_CORRUPT_FLAG_CLEARED	= 304,
  MD305_REUSE_RESUME_DATA_REMOVED 		= 305,
  MD311_RECOVERY_TGT_PARTN_DROPPED 		= 311,
  MD312_RECOVERY_TGT_PARTN_DROPPED_COMMITTED 	= 312,
  MD313_RECOVERY_PARTN_ONLINE 			= 313,
  MD314_RECOVER_PARTN_ONLINE_COMMITTED 		= 314,
  MD315_RECOVER_REMOVED_ROWS_DROP 		= 315,
  MD316_RECOVER_REMOVED_ROWS_DROP_COMMITTED 	= 316,
  MD317_RECOVER_REMOVE_ROWS_MOVE 		= 317,
  MD318_RECOVER_REMOVE_ROWS_MOVE_COMMITTED 	= 318,

  // Range 400 - 499: DMO
  DMO400_PLAN_LOGGING 				= 400,

  // Range 500 - 599: DS
  // Range 600 - 699: UOFS

  // Range 700 - 799: UOL                               number of iterators expected
  UOL700_MTS_RETRY_LOOP_READY_TO_COMMIT		= 700,  // 2 - e.g. TESTPOINT_700_1_1
  UOL701_MTS_RETRY_LOOP_COMMITTED		= 701,  // 2
  UOL702_MTS_NEXT_ROWS				= 702,  // 1 - e.g. TESTPOINT_703_1
  UOL703_SET_EOF_ZERO_RETRY_LOOP                = 703,  // 1
  UOL704_EXECUTE_UPDATE_RETRY_LOOP              = 704,  // 1
  UOL705_SIDE_INSERTS_RETRY_LOOP                = 705,  // 1

  // Range 800 - 899: UTILSP
  // Range 900 - 999: UTIPARSER
  UTIPARS900_MD_PARSE_DONE 			= 900,
  UTIPARS901_MD_DDL_CREATED			= 901,
  UTIPARS902_MD_DDL_CREATED_AND_COMMITTED	= 902,
  UTIPARS903_MD_DONE_EXECUTE			= 903,
  UTIPARS904_MD_DONE_EPILOGUE			= 904,

  // Range 1000 - 1199: Backup/Restore

  // Range 1200 - 1399: DUP
  DUP1200_DDL_LOCK_OBTAINED			= 1200,
  DUP1201_TGT_CREATED				= 1201,
  DUP1202_SRC_TABLE_OPENED			= 1202,
  DUP1203_SRC_INDEXES_OPENED			= 1203, 
  DUP1204_TGT_OPENED				= 1204,
  DUP1205_TGT_INDEXES_OPENED			= 1205,
  DUP1206_TABLE_PARTNS_COPIED			= 1206,
  DUP1207_INDEX_PARTNS_COPIED			= 1207,
  DUP1208_REMOVED_TGT_CORRUPT_FLAG_DDL_LOCK	= 1208,

  // Range 1400 - 1599: Import
  IMP1400_AUDIT_OFF				= 1400,

  // Range 1600 - 1799:  ICT
  // Range 1800 - 1999:  mxtool
  // Range 2000 - 2199: 
  // Populate index 2000 - 2099  Normal Flow
  PI2000_VERIFICATION_COMPLETED			= 2000,
  PI2001_INDEX_POPULATION_STARTED		= 2001,
  PI2002_SRC_TABLE_OPENED			= 2002,
  PI2003_IDXTRANSFORM_STMT_GENERATED		= 2003,
  PI2004_POPINDEX_INIT_PHASE_COMPLETED		= 2004,
  PI2005_AFR_INITIALIZED			= 2005,
  PI2006_AUDSRV_STARTED				= 2006,
  PI2007_AUDIT_COMPRESS_ALTERED			= 2007,
  PI2008_AUDIT_FIXUP_APPLY_RQST_SENT		= 2008,
  PI2009_ENTERED_COMMIT_PHASE			= 2009,
  PI2010_COMMIT_PHASE_RETRY			= 2010,
  PI2011_LABELS_UPDATED				= 2011,
  PI2012_INDEX_POPULATION_COMPLETED		= 2012,
  PI2013_SRC_TABLE_CLOSED			= 2013,
  PI2015_RESTART_AUDIT_APPLY			= 2015,
  PI2016_ROLLBACK_INDEX				= 2016,

  // Populate index 2100 - 2199  Error Flow
  PI2100_RECOVERY_STARTED			= 2100,
  PI2101_RECOVERY_COMPLETED			= 2101,
  PI2102_READY_TO_REMOVE_DDL_LOCK 		= 2102,
  PI2103_RECOVERY_INDEX_ONLINE 			= 2103,
  PI2104_RECOVER_INDEX_ONLINE_COMMITTED 	= 2104,


  // Validate 2200 - 2299
  VA2200_INITIALIZE_DONE			= 2200,
  VA2201_SETUP_DONE				= 2201,
  VA2202_VALIDATE_DONE				= 2202,
  VA2203_RECOVER_SETUP				= 2203,
  VA2204_RECOVER_VALIDATE			= 2204,

  // Range 2200 - 2399:  Purgedata

  // Range 2400 - 2499:  Transform
  TR2400_DDLLOCK_CREATED			= 2400,
  TR2401_VERIFICATION_COMPLETED 		= 2401,
  TR2402_TARGET_OBJECT_CREATED_OPENED		= 2402,
  TR2403_DATA_LOADED             		= 2403,
  TR2404_CREATING_DDLLOCK       		= 2404,
  TR2405_CREATING_TARGET			= 2405,
  TR2406_SWITCHING_OBJECT			= 2406,
  TR2407_DROPPING_DDLLOCK			= 2407,
  TR2408_DROPPING_DDLLOCK_ONBASE		= 2408,
  TR2409_RECOVER_DDLLOCK_STATUS 		= 2409,
  TR2410_RECOVERING_DDLLOCK_CREATE		= 2410,
  TR2411_RECOVER_DDLLOCK_CREATE_DONE		= 2411,
  TR2412_RECOVERING_TARGET_CREATE_LOAD		= 2412,
  TR2413_RECOVER_TARGET_CREATE_LOAD_DONE        = 2413,
  TR2414_IUDLOG_DATA_LOADED            		= 2414,

  // Temporary range 8000 - 8019
  TEMP_TP_1					= 8001,
  TEMP_TP_2					= 8002,
  TEMP_TP_3					= 8003,
  TEMP_TP_4					= 8004,
  TEMP_TP_5					= 8005,
  TEMP_TP_6					= 8006,
  TEMP_TP_7					= 8007,
  TEMP_TP_8					= 8008,
  TEMP_TP_9					= 8009,
  TEMP_TP_10					= 8010,
  TEMP_TP_11					= 8011,
  TEMP_TP_12					= 8012,
  TEMP_TP_13					= 8013,
  TEMP_TP_14					= 8014,
  TEMP_TP_15					= 8015,
  TEMP_TP_16					= 8016,
  TEMP_TP_17					= 8017,
  TEMP_TP_18					= 8018,
  TEMP_TP_19					= 8019,


  LAST_TP					= 200000
};

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Definition of class CNATestPoint
//
// This class contains the basic support for test points
//
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CNATestPoint
{

public:
  // --------------------------------------------------------------------------
  // enums:
  // --------------------------------------------------------------------------

  // Describes the different types of test point requests
  enum ETestPointRqst { eUNKNOWN, eKILL , eERROR, eFSERROR, eTRAP, eDELAY };
  
  CNATestPoint ( Lng32 number, 
                 Lng32 iterator = 1, 
                 ETestPointRqst rqst = eKILL); 

  // --------------------------------------------------------------------------
  // constructors/destructors
  // --------------------------------------------------------------------------
  CNATestPoint ( const CNATestPoint &testPoint );
  virtual ~CNATestPoint();
  
  // accessors
  Lng32 GetTestPoint() const { return m_iTestPoint; }
  Lng32 GetIterator() const { return m_iIterator; }
  Lng32 GetInnerLoopIterator() const { return m_iInnerLoopIterator; }
  CNATestPoint::ETestPointRqst GetRqst() const { return m_eRqst; }
  void GetRqstText(char *text);
  Lng32 GetDelayTime() const { return m_iDelayTime; }
  Int32 GetError() const { return m_iError; }
  Int32 GetFSError() const { return m_iFSError; }
  Int32 GetTrapError() const { return m_iTrapError; }
  Lng32 GetDetails();

  // mutators
  void SetTestPoint ( const Lng32 number ) { m_iTestPoint = number; }
  void SetIterator ( const Lng32 iterator) { m_iIterator = iterator; }
  void SetInnerLoopIterator ( const Lng32 innerLoopIterator)
      { m_iInnerLoopIterator = innerLoopIterator; }
  void SetRqst ( const CNATestPoint::ETestPointRqst rqst ) 
      { m_eRqst = rqst; }
  void SetDelayTime ( const Lng32 delayTime );
  void SetError ( const Int32 error ) { m_iError = error; }
  void SetFSError ( const Int32 fsError ) { m_iFSError = fsError; }
  void SetTrapError ( const Int32 trapError ); 

  Int32 Execute ( void );
  void Wait ( Lng32 delayTime_in_millisecs );
  
protected:
  // --------------------------------------------------------------------------
  // constructors/destructors
  // --------------------------------------------------------------------------
  CNATestPoint();

private:  

  Lng32 m_iTestPoint;
  Lng32 m_iIterator;           // iteration of the outermost loop
  Lng32 m_iInnerLoopIterator;  // iteration of inner loop - 0 if no inner loop
  ETestPointRqst m_eRqst;
  Lng32 m_iDelayTime;
  Int32 m_iError;
  Int32 m_iFSError;
  Int32 m_iTrapError;
 
  void RecursiveCall( char buffer[100000] );
};

class CNATestPointList : public NAList<CNATestPoint *>
{
private:
    
  CNATestPoint::ETestPointRqst ConvertStrToENum(const NAString rqstStr);

public:
  enum EOwnership {eItemsAreOwned, eItemsArentOwned};

  CNATestPointList( EOwnership ownership = eItemsAreOwned);  
  ~CNATestPointList();

  inline void AddTestPoint ( const Lng32 number,
                             const Lng32 iterator,
                             const NAString rqstStr,
                             const Int32 details);

  void AddTestPoint ( const Lng32 number,
                      const Lng32 outermostLoopIterator,
                      const Lng32 innerLoopIterator,
                      const NAString rqstStr,
                      const Int32 details);

  CNATestPoint * Find ( const Lng32 number,
                        const Lng32 iterator,  // iteration of outermost loop
                        const Lng32 innerLoopIterator = 0);  // 0: no inner loop
private:
  EOwnership m_ownership;
};

// =======================================================================
// In-line methods for class CNATestPointList
// =======================================================================


// ---------------------------------------------------------------------
// Method: AddTestPoint
//
// Adds a test point to the test point list
//
// Input:
//   number - the test point number
//   iterator - the iteration for the test point, that is - what to do
//              on the nth iteration of the testpoint
//   rqstStr - what to do when executed (TRAP, ERROR, FSERROR, DELAY, KILL)
//   details - optional details: e.g. how long to wait for a DELAY
// ---------------------------------------------------------------------
inline void CNATestPointList::AddTestPoint ( const Lng32 number,
                                             const Lng32 iterator,
                                             const NAString rqstStr,
                                             const Int32 details)
{
  AddTestPoint ( number,
                 iterator,
                 0,  // no inner loop
                 rqstStr,
                 details);
}

#endif // NATESTPOINT_H
