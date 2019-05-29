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
#ifndef COM_TRANS_INFO_H
#define COM_TRANS_INFO_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComTransInfo.h
 * Description:
 *
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include "NABoolean.h"


#include "NABoolean.h"
#include "NAVersionedObject.h"


enum TransStmtType
{
  BEGIN_, COMMIT_, ROLLBACK_, ROLLBACK_WAITED_, 
  SET_TRANSACTION_
};

  
// PROTECTED_ mode allows read operation against the table but prevents
// writes to it. Currently used with lock table operation only.
enum LockMode
{
  SHARE_, EXCLUSIVE_, PROTECTED_, LOCK_MODE_NOT_SPECIFIED_
};

// Define a type to be used inside an unnamed Union
typedef 
    struct
    {
      unsigned short consistencyLevel_;
      
      unsigned char lockSize_:1;
      unsigned char lockState_:2;
      unsigned char bounceLockFlag_:1;
      unsigned char noLockEscalation_:1;
      unsigned char updateOrDeleteFlag_:1;
      unsigned char updateIndex_:1;
      unsigned char backoutAccess_:1;
      // BertBert VV
      unsigned char invisible_:1;
      unsigned char skipLockedRows_:1;
      unsigned char unLockRowFlag_:1;
      unsigned char selfReferencingUpdate_:1;
      unsigned char eidCleanup_:1;
      unsigned char bitFiller_:3;
      // BertBert ^^
      // unsigned char bitFiller_;  // originally...now changed to
                                        // accomodate new flags
    } LockFlagsType ;


///////////////////////////////////////////////////////////////////////
// This class defines the lock and consistency flags that are
// passed to DP2. Do not add or change any private members to this class
// as the value passed to DP2 is a long (INT(32)) and the bit position
// of each field must be as it is laid out in this class.
////////////////////////////////////////////////////////////////////////
class DP2LockFlags
{
public:
  enum BounceLock {
    WAIT_LOCK = 0,
    BOUNCE_LOCK = 1
  };

  enum ConsistencyLevel {
    READ_UNCOMMITTED,     // browse access
    READ_COMMITTED,       // clean access
    STABLE,               // stable access
    SERIALIZABLE,         // repeatable access
    // QSTUFF
    SKIP_CONFLICT           // skip conflict access, 
                            // skips tuples with conflicting lock mode
    //QSTUFF
  };
  
  enum LockSize {
    ROW_LOCK = 0,
    GROSS_FILE_LOCK = 1
  };

  enum LockState {
    LOCK_CURSOR    = 0,
    LOCK_SHARED    = 1,
    LOCK_EXCLUSIVE = 2,
    LOCK_SELECT    = 3
  };
  
  DP2LockFlags() : value_(0) { }
  
  void setBounceLock(BounceLock bl)
                            { flags_.bounceLockFlag_ = (unsigned char ) bl; }
  short getBounceLock() const              { return flags_.bounceLockFlag_; }

  void setConsistencyLevel(ConsistencyLevel cl)
                          { flags_.consistencyLevel_ = (unsigned short) cl; }
  short getConsistencyLevel() const {return (short)flags_.consistencyLevel_;}

  void setLockSize(LockSize ls)    { flags_.lockSize_ = (unsigned char) ls; }
  short getLockSize() const                      { return flags_.lockSize_; }

  void setLockState(LockState ls) { flags_.lockState_ = (unsigned char) ls; }
  short getLockState() const                    { return flags_.lockState_; }

  void setNoLockEscalation(unsigned char v) { flags_.noLockEscalation_ = v; }
  short getNoLockEscalation() const      { return flags_.noLockEscalation_; }
  
  Lng32 getValue() const                                    { return value_; }
   
  // BertBert VV
  void setBounceLockFlag(){flags_.bounceLockFlag_ = 1;};
  void resetBounceLockFlag(){flags_.bounceLockFlag_ = 0;};
  short getBounceLockFlag(){return flags_.bounceLockFlag_;};
  void setSkipLockedRows(){flags_.skipLockedRows_ = 1;};               
  void resetSkipLockedRows(){flags_.skipLockedRows_ = 0;};
  short getSkipLockedRows(){return flags_.skipLockedRows_;};
  void setUpdateOrDeleteFlag(){flags_.updateOrDeleteFlag_ = 1;};               
  void resetUpdateOrDeleteFlag(){flags_.updateOrDeleteFlag_ = 0;};
  short getUpdateOrDeleteFlag(){return flags_.updateOrDeleteFlag_;};
  void setUnLockRowFlag(){flags_.unLockRowFlag_ = 1;}; 
  void resetUnLockRowFlag(){flags_.unLockRowFlag_ = 0;}; 
  // BertBert ^^

  void setSelfReferencingUpdate(unsigned char v) 
                                       { flags_.selfReferencingUpdate_ = v; }
  short getSelfReferencingUpdate() const     
                                    { return flags_.selfReferencingUpdate_; }

  void setEidCleanup(unsigned char v) { flags_.eidCleanup_ = v; }
  short getEidCleanup() const { return flags_.eidCleanup_; }

  DP2LockFlags operator=(DP2LockFlags src)
  {
    value_ = src.getValue();
    return *this;
  }

  // returns -1, if a transaction is needed for these lock flags.
  short transactionNeeded();
  
private:
  union
  {
    Lng32 value_;
    LockFlagsType flags_ ;
  };
};


class TransMode : public NAVersionedObject
{
public:

  enum AccessType
    {
      READ_UNCOMMITTED_ACCESS_   = 00,
      SKIP_CONFLICT_ACCESS_      = 05,
      READ_COMMITTED_ACCESS_     = 10,
      REPEATABLE_READ_ACCESS_    = 20,
      SERIALIZABLE_ACCESS_       = 30,
      ACCESS_TYPE_NOT_SPECIFIED_ = -1
    };

  // These values get stored in generated code, so MUST NOT BE CHANGED.
  // The values must form a logical sequence -- see the xxCompatible()
  // code below, and StmtLevelAccessOptions::updateAccessOptions.
  //
  // Enum TransMode::IsolationLevel maps to enum AccessType.
  // The static funcs that follow do this mapping, in both directions.
  //

  enum IsolationLevel
  {
    READ_UNCOMMITTED_ = READ_UNCOMMITTED_ACCESS_,
    READ_COMMITTED_   = READ_COMMITTED_ACCESS_,
    REPEATABLE_READ_  = REPEATABLE_READ_ACCESS_,
    SERIALIZABLE_     = SERIALIZABLE_ACCESS_,
    IL_NOT_SPECIFIED_ = ACCESS_TYPE_NOT_SPECIFIED_
  };

  static AccessType     ILtoAT(IsolationLevel il)
  { return (il == SERIALIZABLE_) ? REPEATABLE_READ_ACCESS_ : (AccessType)il; }

  static IsolationLevel ATtoIL(AccessType at)
  { return (IsolationLevel)at; }

  enum AccessMode
  {
    READ_ONLY_, READ_WRITE_, AM_NOT_SPECIFIED_, READ_ONLY_SPECIFIED_BY_USER_
  };

  enum AutoCommit
  {
    ON_, OFF_, AC_NOT_SPECIFIED_
  };

  enum MultiCommit
  {
    MC_NOT_SPECIFIED_, MC_ON_, MC_OFF_
  };

  // whether to rollback waited or nowaited. Default is waited.
  enum RollbackMode
  {
    ROLLBACK_MODE_WAITED_, ROLLBACK_MODE_NOWAITED_, ROLLBACK_MODE_NOT_SPECIFIED_, 
    NO_ROLLBACK_, NO_ROLLBACK_IN_IUD_STATEMENT_
  };

  enum Flags
  {
    STMTLEVELACCESSOPTIONS_ = 0x2,
    AUTO_BEGIN_ON_          = 0x4,
    AUTO_BEGIN_OFF_         = 0x8,
    USER_TRANS_MODE_        = 0x10
  };

  TransMode(IsolationLevel il = IL_NOT_SPECIFIED_,
            AccessMode am = AM_NOT_SPECIFIED_,
            AutoCommit ac = AC_NOT_SPECIFIED_,
	    RollbackMode rm = ROLLBACK_MODE_NOT_SPECIFIED_,
            Lng32 das = -1,
	    Lng32 aiVal = -1,
            MultiCommit mc = MC_NOT_SPECIFIED_,
            ULng32 mcs = 0)
    : il_(il), am_((short) am), autoCommit_((short) ac), diagAreaSize_(das),
      flags_(0), rm_((short) rm), autoAbortInterval_(aiVal), 
      multiCommit_((short) mc), multiCommitSize_(mcs), NAVersionedObject(-1)
  {}


  virtual unsigned char getClassVersionID()                   { return 1; }
  virtual void populateImageVersionIDArray()
                              { setImageVersionID(0,getClassVersionID()); }
  virtual short getClassSize()         { return (short)sizeof(TransMode); }

  /*IsolationLevel&*/   Int16& isolationLevel()   { return il_; }
  /*IsolationLevel */   Int16  isolationLevel() const { return il_; }
  /*AccessMode&*/       Int16& accessMode()       { return am_; }
  /*AccessMode */       Int16  accessMode() const { return am_; }
  /*AutoCommit&*/       Int16& autoCommit()       { return autoCommit_; }
  /*RollbackMode&*/     Int16& rollbackMode()     { return rm_; }
  /*MultiCommit&*/      Int16& multiCommit()       { return multiCommit_; }
  Int32&                       diagAreaSize()     { return diagAreaSize_; }
  Int32&                       autoAbortIntervalInSeconds() { return autoAbortInterval_; }
  UInt32&                      multiCommitSize() { return multiCommitSize_; }
  IsolationLevel               getIsolationLevel(){ return (IsolationLevel)il_;}
  AccessMode                   getAccessMode()    { return (AccessMode)am_; }
  AutoCommit                   getAutoCommit()    { return (AutoCommit)autoCommit_; }
  RollbackMode                 getRollbackMode()  { return (RollbackMode)rm_;}
  NABoolean                    anyNoRollback() { return
                                 (rm_ == NO_ROLLBACK_ || 
                                  rm_ == NO_ROLLBACK_IN_IUD_STATEMENT_); }
  Lng32                         getDiagAreaSize()  { return diagAreaSize_; }
  Int16                        getFlags() const { return flags_; }
  Int16&                       setFlags()         { return flags_; }
  Int32                        getAutoAbortIntervalInSeconds() const { return autoAbortInterval_; }
  MultiCommit                  getMultiCommit()    { return (MultiCommit)multiCommit_; }
  UInt32                       getMultiCommitSize() const { return multiCommitSize_; }

  NABoolean                    stmtLevelAccessOptions() const
  { return flags_ & STMTLEVELACCESSOPTIONS_; }
  void                         setStmtLevelAccessOptions(Int32 v=1)
  { if (v) flags_ |= STMTLEVELACCESSOPTIONS_; 
    else   flags_ &= ~STMTLEVELACCESSOPTIONS_; }

  void setAutoBeginOn(NABoolean vi)
  {
    if (vi == TRUE)
      flags_ |= AUTO_BEGIN_ON_;
    else
      flags_ &= ~AUTO_BEGIN_ON_;
  }

  NABoolean getAutoBeginOn()
  {
    if (flags_ & AUTO_BEGIN_ON_)
      return TRUE;
    else
      return FALSE;
  }

  void setAutoBeginOff(NABoolean vi)
  {
    if (vi == TRUE)
      flags_ |= AUTO_BEGIN_OFF_;
    else
      flags_ &= ~AUTO_BEGIN_OFF_;
  }

  NABoolean getAutoBeginOff()
  {
    if (flags_ & AUTO_BEGIN_OFF_)
      return TRUE;
    else
      return FALSE;
  }

  NABoolean                    userTransMode() const
  { return (flags_ & USER_TRANS_MODE_) != 0; }
  void                         setUserTransMode(NABoolean v)
  { if (v) flags_ |= USER_TRANS_MODE_; 
    else   flags_ &= ~USER_TRANS_MODE_; }

  NABoolean isolationLevelCompatible(const TransMode &o) const
  { return il_ <= o.il_; }

  NABoolean accessModeCompatible(const TransMode &o) const
  { if (o.am_ == READ_ONLY_SPECIFIED_BY_USER_)
      return am_ <= READ_ONLY_;
    else
      return am_ <= o.am_;
  }

  NABoolean operator==(const TransMode &o) const;

  void updateTransMode(TransMode * trans_mode);

  ULng32 getByteSize() const { return sizeof(*this); }

  static IsolationLevel getIsolationLevel(const char *value);

  // Update per Ansi 14.1 SR 4.
  // Should ONLY be used when an access mode was not explicitly specified
  // in the SET TRANSACTION stmt.
  void updateAccessModeFromIsolationLevel(IsolationLevel il,
					  NABoolean affectIL = TRUE)
  {
    Int16 tempIL = il_;
    if (il != IL_NOT_SPECIFIED_)
      tempIL = (Int16) il;

    if (am_ != READ_ONLY_SPECIFIED_BY_USER_)
      am_ = (Int16) ((tempIL == READ_UNCOMMITTED_) ? READ_ONLY_ : READ_WRITE_);

    if (affectIL)
      il_ = tempIL;
  }

  void updateAccessModeAndResetIsolationLevel()
  { 
    il_ = IL_NOT_SPECIFIED_;
    updateAccessModeFromIsolationLevel(IL_NOT_SPECIFIED_);
  }
  
  NABoolean invalidAccessModeAndIsolationLevel()        // ret TRUE if invalid
  { return (il_ == READ_UNCOMMITTED_ && (am_ != READ_ONLY_ || am_ != READ_ONLY_SPECIFIED_BY_USER_)); }

  NABoolean invalidMultiCommitCompatibility()        // ret TRUE if invalid
  { return (il_ == READ_UNCOMMITTED_            ||
            am_ == READ_ONLY_                   ||
            am_ == READ_ONLY_SPECIFIED_BY_USER_ ||
            rm_ == NO_ROLLBACK_ ); }

  // multi commit modes are compatible
  NABoolean multiCommitCompatible(const TransMode &o) const
  { 
    switch (o.multiCommit_)
      {
      case MC_OFF_:
      case MC_NOT_SPECIFIED_:
	return multiCommit_ != MC_ON_;
	break;

      case MC_ON_:
	return multiCommit_ == MC_ON_ ;
	break;
	
      default:
	return FALSE;      // Not compatible
	break;
      }
  }

  // based on the current access options or SET TRANSACTION settings,
  // return the corresponding DP2LockFlags.
  DP2LockFlags getDP2LockFlags();

  // Fast encoding for DgInt0 diags display.
  // Shift things left (more zeroes in each xx_*10) if want to add more members
  Lng32 display() const
  { return  multiCommit_*1000000 + il_*10000 + am_*100 + rm_*10 + autoCommit_; }

private:

  // see enum IsolationLevel
  Int16 il_;                                                         // 00-01

  // see enum AccessMode
  Int16 am_;                                                         // 02-03

  // if ON_, then implicitly commit each SQL statement (see enum AutoCommit)
  Int16 autoCommit_;                                                 // 04-05

  Int16 flags_;                                                      // 06-07

  Int32 diagAreaSize_;                                               // 08-11

  // see enum RollbackMode
  Int16 rm_;                                                         // 12-13

  Int16 multiCommit_;                                                // 14-15

  Int32 autoAbortInterval_;                                          // 16-19

  UInt32 multiCommitSize_;                                           // 20-23
};

////////////////////////////////////////////////////////////////
// These are the access options that are specified in a SQL
// statements (FOR BROWSE ACCESS, IN SHARE MODE, etc).
// This is a Tandem extension. Do HELP from sql/mp sqlci or
// look at sql/mp manual or SQL/ARK Language spec
// for details on these options. If these are specified in a
// query, they override any options specified via a SET 
// TRANSACTION statement.
////////////////////////////////////////////////////////////////
class StmtLevelAccessOptions
{
public:
  StmtLevelAccessOptions(TransMode::AccessType at = TransMode::ACCESS_TYPE_NOT_SPECIFIED_,
                         LockMode lm = LOCK_MODE_NOT_SPECIFIED_)
    : accessType_(at), lockMode_(lm)
    ,updateOrDelete_ (FALSE)
    ,scanLockForIM_(FALSE)
  {}

  NABoolean operator==(const StmtLevelAccessOptions &o) const
  { return accessType_ == o.accessType_ && lockMode_ == o.lockMode_; }

  TransMode::AccessType& accessType()      { return accessType_; }
  TransMode::AccessType accessType() const { return accessType_; }
  LockMode& lockMode()          { return lockMode_;   }

  NABoolean userSpecified()
  {
    return (accessType_ != TransMode::ACCESS_TYPE_NOT_SPECIFIED_ ||
            lockMode_   != LOCK_MODE_NOT_SPECIFIED_);
  }
  
  void updateAccessOptions(TransMode::AccessType at, 
                           LockMode lm = LOCK_MODE_NOT_SPECIFIED_);

  // based on the current access options, return the corresponding DP2LockFlags
  DP2LockFlags getDP2LockFlags();

  // QSTUFF
  void setUpdateOrDelete (NABoolean u) { updateOrDelete_ = u; };
  NABoolean isUpdateOrDelete() { return updateOrDelete_; };
  // QSTUFF
 
  void setScanLockForIM(NABoolean u) { scanLockForIM_ = u; };
  NABoolean scanLockForIM(){ return scanLockForIM_; };

private:
  TransMode::AccessType accessType_;
  LockMode   lockMode_;
  NABoolean  updateOrDelete_;

  //-------------------------------------------------------------------------
  // This option attribute is set for all scans that are on the left side of
  // an TSJ "for write" when there is an update or delete and Index Maintenance.
  // This is needed by the executor to ensure that selected rows are 
  // appropriately locked, even if scanned from an index, to protect against 
  // index corruption.  The attribute is propagated to ComTdbDp2SubsOper and 
  // ComTdbDp2UniqueOper and used during execution to ask DP2 for serializable
  // locks.
  //
  // This is to address genesis solution 10-040802-8483, and also addresses 
  // solution 10-031111-1215.  The original fix for 10-040802-8483 caused
  // deadlocks in "refresh MVGROUP" to get error 73 when 
  // MV_REFRESH_MAX_PARALLELISM > 1 (see solution 10-070322-3467), so it has
  // been modified to limit the scan nodes that will use serializable locking
  // to those nodes on the left side of the TSJforWrite.
  //------------------------------------------------------------------------- 
  NABoolean  scanLockForIM_;
};

// verify that statement-level access and session-level setting are OK.
// set errCodeA/errCodeB to 0/0 if all OK, else to -3140/-3141.
void verifyUpdatableTrans(StmtLevelAccessOptions * sAxOpt, 
			  TransMode * tm, 
			  TransMode::IsolationLevel isolationLevelForUpdate,
			  Lng32 &errCodeA, Lng32 &errCodeB);

// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for TransMode
// ---------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<TransMode> TransModePtr;

const char * getStrOfIsolationLevel(TransMode::IsolationLevel isl, NABoolean doNotReportNotSpecified = TRUE);

const char * getStrOfAccessMode(TransMode::AccessMode am);

const char * getStrOfRollbackMode(TransMode::RollbackMode am);


#endif // COM_TRANS_INFO_H
