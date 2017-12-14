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
 * File:         ComTransInfo.cpp
 * Description:
 *
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *****************************************************************************
 */


#include "ComTransInfo.h"
#include <iostream>

// returns -1, if a transaction is needed for these lock flags.
short DP2LockFlags::transactionNeeded()
{
  // QSTUFF
  // we also don't require a transaction when skipping locked rows
  // this is automatically overwritten when we have a delete or update.
  if (getSkipLockedRows() && (!getUpdateOrDeleteFlag()))
    return 0;
  // QSTUFF
  return (getConsistencyLevel() == READ_UNCOMMITTED) ? 0 : -1;
}

void StmtLevelAccessOptions::updateAccessOptions(TransMode::AccessType at, LockMode lm)
{
  if (at != TransMode::ACCESS_TYPE_NOT_SPECIFIED_)
    if (accessType_ < at)
      accessType_ = at;
  
  if (lm != LOCK_MODE_NOT_SPECIFIED_)
    if (lockMode_ < lm)
      lockMode_ = lm;
}

DP2LockFlags StmtLevelAccessOptions::getDP2LockFlags()
{
  DP2LockFlags flags;

  switch (accessType_)
    {
    case TransMode::READ_UNCOMMITTED_ACCESS_:
      flags.setConsistencyLevel(DP2LockFlags::READ_UNCOMMITTED);
      break;
      
    case TransMode::ACCESS_TYPE_NOT_SPECIFIED_:
    case TransMode::READ_COMMITTED_ACCESS_:
      flags.setConsistencyLevel(DP2LockFlags::READ_COMMITTED);
      // QSTUFF
      // make sure table scans for embedded deletes or updates
      // cause exclusive locks to be set
      if (isUpdateOrDelete())
             flags.setUpdateOrDeleteFlag();
      // QSTUFF

      break;
      
    case TransMode::REPEATABLE_READ_ACCESS_:
      flags.setConsistencyLevel(DP2LockFlags::SERIALIZABLE);
      break;
        
    // QSTUFF
    // we currently use a combination of read committed and skip, 
    // i.e. check for lock and skip conflicting lock, 
    // to implement SKIP CONFLICT - but we also support it when 
    // executing without a transaction thus this really makes a
    // statement insensitive to updates within the same transaction
    // while this could be argued either way the advantages for 
    // efficient subscriptions outweight any short commings.

    case TransMode::SKIP_CONFLICT_ACCESS_:
      flags.setConsistencyLevel(DP2LockFlags::READ_COMMITTED);
      flags.setSkipLockedRows();
      // makes sure scans for embedded updates or deletes cause 
      // exclusive locks to be acquired
      if (isUpdateOrDelete())
        flags.setUpdateOrDeleteFlag();
      break;
    // QSTUFF ^^
          
    }
  
  switch (lockMode_)
    {
    case LOCK_MODE_NOT_SPECIFIED_:
      // BertBert VV
      // original code follows
      /*
      if (flags.getConsistencyLevel() == DP2LockFlags::READ_UNCOMMITTED)
        flags.setLockState(DP2LockFlags::LOCK_CURSOR);
      else
        flags.setLockState(DP2LockFlags::LOCK_SHARED);
      */
      // LOCK_CURSOR is the same as LOCK_SHARED if the updateOrDeleteFlag_ is not set
      // If the updateOrDeleteFlag_ is set, then DP2 gets exclusive locks on selected
      //  rows and shared locks on non selected rows.  This is a good default.
      flags.setLockState(DP2LockFlags::LOCK_CURSOR);
      // BertBert ^^
      break;
      
    case SHARE_:
      flags.setLockState(DP2LockFlags::LOCK_SHARED);
      break;
      
    case EXCLUSIVE_:
      flags.setLockState(DP2LockFlags::LOCK_EXCLUSIVE);
      break;
      
    }
  return flags;
}


NABoolean TransMode::operator==(const TransMode &other) const
{
  return (il_ == other.il_ && 
	  am_ == other.am_ && 
	  rm_ == other.rm_ &&
	  autoAbortInterval_ == other.autoAbortInterval_ &&
	  multiCommit_ == other.multiCommit_);
}


void TransMode::updateTransMode(TransMode * trans_mode)
{
  if (trans_mode->isolationLevel() != TransMode::IL_NOT_SPECIFIED_)
    isolationLevel() = trans_mode->isolationLevel();

  if (trans_mode->accessMode() != TransMode::AM_NOT_SPECIFIED_)
    accessMode() = trans_mode->accessMode();

  if (trans_mode->autoCommit() != TransMode::AC_NOT_SPECIFIED_)
    autoCommit() = trans_mode->autoCommit();
  
   if (trans_mode->rollbackMode() != TransMode::ROLLBACK_MODE_NOT_SPECIFIED_)
    rollbackMode() = trans_mode->rollbackMode();
  
 if (trans_mode->diagAreaSize() >= 0)
    diagAreaSize() = trans_mode->diagAreaSize();

 if (trans_mode->autoAbortIntervalInSeconds() >= -3)
    autoAbortIntervalInSeconds() = trans_mode->getAutoAbortIntervalInSeconds();

 if (trans_mode->multiCommit() != multiCommit())
   {
     multiCommit() = trans_mode->multiCommit();
     multiCommitSize() = trans_mode->getMultiCommitSize();
   }
 

  // sets either one of these flag bits to on to indicate whether auto begin
  // is turned on or off.  Only sets it if the user explicitly issues an
  // on or off setting eg. (set transaction autobegin on|off).
  if (trans_mode->getAutoBeginOn() != 0)  // 0 means it has not been set for 
                                          // changing by the user.
     {
     setAutoBeginOff(FALSE); // to clear AUTO_BEGIN_OFF_ bit
     setAutoBeginOn(trans_mode->getAutoBeginOn());
     }
  else
  if (trans_mode->getAutoBeginOff() != 0)
     {
     setAutoBeginOn(FALSE); // to clear AUTO_BEGIN_ON_ bit
     setAutoBeginOff(trans_mode->getAutoBeginOff());
     }

}

TransMode::IsolationLevel TransMode::getIsolationLevel(const char *value)
{
  // This method assumes the value has been validated already!
  // This method is coded for speed, for Executor.
  // E.g., ExControlArea can use it on CQD's returned from arkcmp.
  TransMode::IsolationLevel il = TransMode::SERIALIZABLE_;
  if (str_len(value) > 5)
    switch (value[5]) {	    //  012345.  value[5] is {C|U|T|L}.
    case 'C':		il = TransMode::READ_COMMITTED_;   break;
    case 'U':		il = TransMode::READ_UNCOMMITTED_; break;
    case 'T':		il = TransMode::REPEATABLE_READ_;  break;
      //case 'L':	il = TransMode::SERIALIZABLE_;     break;
    }
  return il;
}

DP2LockFlags TransMode::getDP2LockFlags()
{
  DP2LockFlags flags;

  switch (isolationLevel())
    {
    case READ_UNCOMMITTED_:
      flags.setConsistencyLevel(DP2LockFlags::READ_UNCOMMITTED);
      flags.setLockState(DP2LockFlags::LOCK_CURSOR);
      break;
      
    case READ_COMMITTED_:
      flags.setConsistencyLevel(DP2LockFlags::READ_COMMITTED);
      // BertBert VV
      // Default is LOCK_CURSOR: exclusive on selected rows if update/delete, shared otherwise
      // flags.setLockState(DP2LockFlags::LOCK_SHARED);
      flags.setLockState(DP2LockFlags::LOCK_CURSOR);
      // BertBert ^^
      break;
      
    case REPEATABLE_READ_:
      flags.setConsistencyLevel(DP2LockFlags::SERIALIZABLE);
      // BertBert VV
      // Default is LOCK_CURSOR: exclusive on selected rows if update/delete, shared otherwise
      // flags.setLockState(DP2LockFlags::LOCK_SHARED);
      flags.setLockState(DP2LockFlags::LOCK_CURSOR);
      // BertBert ^^
      break;

    case IL_NOT_SPECIFIED_:
    case SERIALIZABLE_:
      flags.setConsistencyLevel(DP2LockFlags::SERIALIZABLE);
      // BertBert VV
      // Default is LOCK_CURSOR: exclusive on selected rows if update/delete, shared otherwise
      // flags.setLockState(DP2LockFlags::LOCK_SHARED);
      flags.setLockState(DP2LockFlags::LOCK_CURSOR);
      // BertBert ^^
      break;
    }
  
  return flags;
}

const char * 
getStrOfIsolationLevel(TransMode::IsolationLevel isl, 
                       NABoolean doNotReportNotSpecified) 
{
  switch(isl){
  case TransMode::READ_UNCOMMITTED_:
    return "READ UNCOMMITTED";
    break;
  case TransMode::READ_COMMITTED_:
    return "READ COMMITTED";
    break;
  case TransMode::REPEATABLE_READ_:
    return "REPEATABLE READ";
    break;
  case TransMode::SERIALIZABLE_:
    return "SERIALIZABLE";
    break;
  default:
    if ( doNotReportNotSpecified == TRUE )
      return NULL;
    else
      return "NOT SPECIFIED";
  }
}

const char * getStrOfAccessMode(TransMode::AccessMode am){
  switch(am){
  case TransMode::READ_ONLY_:
  case TransMode::READ_ONLY_SPECIFIED_BY_USER_:
    return "READ ONLY";
    break;
  case TransMode::READ_WRITE_:
    return "READ WRITE";
    break;
  default:
    return NULL;
  }
}

const char * getStrOfRollbackMode(TransMode::RollbackMode am){
  switch(am){
  case TransMode::NO_ROLLBACK_:
  case TransMode::NO_ROLLBACK_IN_IUD_STATEMENT_:
    return "NO ROLLBACK";
    break;
  case TransMode::ROLLBACK_MODE_WAITED_:
    return "ROLLBACK WAITED";
    break;
  case TransMode::ROLLBACK_MODE_NOWAITED_:
    return "ROLLBACK NOWAITED";
    break;
  default:
    return NULL;
  }
}

void verifyUpdatableTrans(StmtLevelAccessOptions * sAxOpt, 
			  TransMode * tm, 
			  TransMode::IsolationLevel isolationLevelForUpdate,
			  Lng32 &errCodeA, Lng32 &errCodeB)
{
  if (sAxOpt && sAxOpt->accessType() == TransMode::READ_UNCOMMITTED_ACCESS_)
    errCodeA = -3140;
  else if (((! sAxOpt) ||
	    (sAxOpt->accessType() == TransMode::ACCESS_TYPE_NOT_SPECIFIED_)) &&
           (tm->isolationLevel() == TransMode::READ_UNCOMMITTED_) &&
	   ((isolationLevelForUpdate == TransMode::IL_NOT_SPECIFIED_) ||
	    (isolationLevelForUpdate == TransMode::READ_UNCOMMITTED_)))
    errCodeA = -3140;
  else if (((! sAxOpt) ||
	    (sAxOpt->accessType() == TransMode::ACCESS_TYPE_NOT_SPECIFIED_)) &&
           (tm->accessMode() != TransMode::READ_WRITE_) &&
     	   ((isolationLevelForUpdate == TransMode::IL_NOT_SPECIFIED_) ||
    	    (isolationLevelForUpdate == TransMode::READ_UNCOMMITTED_) ||
	    (tm->accessMode() == TransMode::READ_ONLY_SPECIFIED_BY_USER_)))
    errCodeB = -3141;
  else {
    errCodeA = 0;
    errCodeB = 0;
  }
}



