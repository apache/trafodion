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
 * File:         ComMvAttributeBitmap.cpp
 * Description:  
 *               
 *               
 * Created:      04/04/2000
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include <NAAssert.h>
#include "ComMvAttributeBitmap.h"

void ComMvAttributeBitmap::initBitmap(ComSInt32 initialiVal)
{
  bitmap_ = initialiVal;
}


ComSInt32 ComMvAttributeBitmap::getBitmap() const
{
  return bitmap_;
}


//----------------------------------------------------------------------------
// exclude from coverage since this is a debugging method
void ComMvAttributeBitmap::trace()
{
#ifdef _DEBUG    

  ComSInt32 initOnStOnMe  = IS_INIT_ON_STMT_ON_ME_MASK & bitmap_;
  ComSInt32 isMV	  = IS_MV_MASK		& bitmap_;
  ComSInt32 rangeLog	  = RANGE_LOGGING_MASK	& bitmap_;
  ComSInt32 insertLog	  = INSERT_LOGGING_MASK & bitmap_;
  ComSInt32 lockOnRefresh = LOCKONREFRESH_MASK	& bitmap_;
  ComSInt32 rewrite	  = REWRITE_MASK	& bitmap_;
  ComSInt32 mvsAllowed	  = MVS_ALLOWED_MASK	& bitmap_;
  ComSInt32 mvStatus	  = MVSTATUS_MASK	& bitmap_;
  ComSInt32 mvAudit	  = MV_AUDIT_MASK	& bitmap_;

  
  
  cout << endl << "MV ATTRIBUTE BITMAP" << endl;

  cout.flags(ios::hex);
  cout << "bitmap val: " << bitmap_ << endl;
  cout.flags(ios::dec);


  if(IsBitSet(NO_LOGS))
    cout << "NO_LOGS" << endl;
  if(IsBitSet(LOGS_CREATED))
    cout << "LOGS_CREATED" << endl;
  if(IsBitSet(LOGS_ON_CREATION))
    cout << "LOGS_ON_CREATION" << endl;
  if(IsBitSet(LOGGING_REQUIRED))
    cout << "LOGGING_REQUIRED" << endl;
  if(IsBitSet(MVS_ON_ME))
    cout << "MVS_ON_ME" << endl;


  switch(initOnStOnMe)
  {
  case NO_INIT_ON_STMT_ON_ME:
    cout << "NO_INIT_ON_STMT_ON_ME" << endl;
    break;
  case INIT_ON_STMT_ON_ME:
    cout << "INIT_ON_STMT_ON_ME" << endl;
    break;
  }
  
  switch(isMV)
  {
  case NOT_AN_MV:
    cout << "NOT_AN_MV" << endl;
    break;
  case IS_AN_MV:
    cout << "IS_AN_MV" << endl;
    break;

  }
  switch(rangeLog)
  {
  case NO_RANGELOG:
    cout << "NO_RANGELOG" << endl;
    break;
  case MANUAL_RANGELOG:
    cout << "MANUAL_RANGELOG" << endl;
    break;
  case AUTO_RANGELOG:
    cout << "AUTO_RANGELOG" << endl;
    break;
  case MIXED_RANGELOG:
    cout << "MIXED_RANGELOG" << endl;
    break;


  }
  switch(insertLog)
  {
  case NO_INSERTLOG:
    cout << "NO_INSERTLOG" << endl;
    break;
  case INSERTLOG:
    cout << "INSERTLOG" << endl;
    break;


  }
  switch(lockOnRefresh)
  {
  case NO_LOCKONREFRESH:
    cout << "NO_LOCKONREFRESH" << endl;
    break;
  case LOCKONREFRESH:
    cout << "IS_LOCKONREFRESH" << endl;
    break;


  }
  switch(rewrite)
  {
  case DISABLE_REWRITE:	
    cout << "DISABLE_REWRITE" << endl;
    break;
  case ENABLE_REWRITE:
    cout << "DISABLE_REWRITE" << endl;
    break;


  }
  switch(mvsAllowed)
  {
  case NO_MVS_ALLOWED:
    cout << "NO_MVS_ALLOWED" << endl;
    break;
  case ALL_MVS_ALLOWED:
    cout << "ALL_MVS_ALLOWED" << endl;
    break;
  case ONSTATEMENT_MVS_ALLOWED:
    cout << "ONSTATEMENT_MVS_ALLOWED" << endl;
    break;
  case ONREQUEST_MVS_ALLOWED:
    cout << "ONREQUEST_MVS_ALLOWED" << endl;
    break;
  case RECOMPUTE_MVS_ALLOWED:
    cout << "RECOMPUTE_MVS_ALLOWED" << endl;
    break;


  }
  switch(mvStatus)
  {
  case MV_NOT_INITIALIZED:
    cout << "MV_NOT_INITIALIZED" << endl;
    break;
  case MV_INITIALIZED:
    cout << "MV_INITIALIZED" << endl;
    break;
  case MV_UNAVAILABLE:
    cout << "MV_UNAVAILABLE" << endl;
    break;


  }
  switch(mvAudit)
  {
  case MV_NO_AUDIT:
    cout << "MV_NO_AUDIT" << endl;
    break;
  case MV_AUDIT:
    cout << "MV_AUDIT" << endl;
    break;
  case MV_NO_AUDITONREFRESH:
    cout << "MV_NO_AUDITONREFRESH" << endl;
    break;
  }
    

  cout << "LOG ON MV CREATION " << getLogCreatedOnMVCreation() << endl;
  cout << "LOGGING REQUIRED " << getLoggingRequired() << endl;
  cout << "AUTO RANGELOG REQUIRED " << getAutomaticRangeLoggingRequired() << endl;
  cout << "ENABLE MVLOG " << getEnableMVLOGExecution() << endl;
  

  cout << endl;
#else
  assert(0);
#endif

} // trace

//----------------------------------------------------------------------------
// exclude from coverage - this code is not used for now
ComMvAttributeBitmap::Result 
ComMvAttributeBitmap::setLogsCreatedOnMVCreation()
{
  ComSInt32 initialVal = bitmap_;

  SetBit(LOGS_CREATED | LOGS_ON_CREATION);

  return (initialVal != bitmap_) ? ATTRIBUTE_SET : NO_CHAGE;
}

//----------------------------------------------------------------------------
ComMvAttributeBitmap::Result
ComMvAttributeBitmap::setLogCreated()
{
  ComSInt32 initialVal = bitmap_;

  SetBit(LOGS_CREATED | MVS_ON_ME);

  if(!IsBitSet(MANUAL_RANGELOG))
  {
    SetBit(LOGGING_REQUIRED);
  }

  return (initialVal != bitmap_) ? ATTRIBUTE_SET : NO_CHAGE;
}


//----------------------------------------------------------------------------
ComMvAttributeBitmap::Result
ComMvAttributeBitmap::setLastOnRequestMvOnMe()
{
  ComSInt32 initialVal = bitmap_;

  if(!IsBitSet(LOGS_ON_CREATION))
  {
    ClearMask(LOGS_STATUS_MASK);
  }
  else
  {
    ClearBit(LOGGING_REQUIRED | MVS_ON_ME);
  }

  return (initialVal != bitmap_) ? ATTRIBUTE_SET : NO_CHAGE;
}


//----------------------------------------------------------------------------
ComMvAttributeBitmap::Result
ComMvAttributeBitmap::setIsAnMv(NABoolean value)
{
  ComSInt32 initialVal = bitmap_;

  if(TRUE == value)
  {
    SetBit(IS_AN_MV);
  }
  else
  {
    ClearBit(IS_AN_MV);
  }

  return (initialVal != bitmap_) ? ATTRIBUTE_SET : NO_CHAGE;
}


//----------------------------------------------------------------------------
ComMvAttributeBitmap::Result
ComMvAttributeBitmap::setInitOnStmtMvOnMe(NABoolean value)
{
  ComSInt32 initialVal = bitmap_;

  if(TRUE == value)
  {
    SetBit(INIT_ON_STMT_ON_ME);
  }
  else
  {
    ClearBit(INIT_ON_STMT_ON_ME);
  }

  return (initialVal != bitmap_) ? ATTRIBUTE_SET : NO_CHAGE;
}


//----------------------------------------------------------------------------
void ComMvAttributeBitmap::clearBaseTableAttributes()
{
  ClearBit(BASE_TABLE_ATTRIBUTES_MASK);
}


//----------------------------------------------------------------------------
void ComMvAttributeBitmap::clearMvAttributes()
{
  ClearBit(MV_ATTRIBUTES_MASK);
}


//----------------------------------------------------------------------------
ComMvAttributeBitmap::Result
ComMvAttributeBitmap::setRangeLogType(ComRangeLogType rangeLogType)
{
  ComSInt32 initialVal = bitmap_;
  NABoolean wasManualRangeLog = IsBitSet(MANUAL_RANGELOG);
  NABoolean setToOtherThanManualRangeLog = FALSE;

  ClearMask(RANGE_LOGGING_MASK);
  
  switch(rangeLogType)
  {
    case COM_MANUAL_RANGELOG:
      ClearBit(LOGGING_REQUIRED);
      SetBit(MANUAL_RANGELOG);
      break;

    case COM_NO_RANGELOG:
      SetBit(NO_RANGELOG);
      setToOtherThanManualRangeLog = TRUE;
      break;

    case COM_AUTO_RANGELOG:
      SetBit(AUTO_RANGELOG);
      setToOtherThanManualRangeLog = TRUE;
      break;

    case COM_MIXED_RANGELOG:
      SetBit(MIXED_RANGELOG);
      setToOtherThanManualRangeLog = TRUE;
      break;
	    
// exclude from coverage - range logging is not supported
// and default condition not reacheable
    default:
      bitmap_ = initialVal;
      return INTERNAL_ERROR;
  }
  
  if (TRUE == wasManualRangeLog            && 
      TRUE == setToOtherThanManualRangeLog && 
      IsBitSet(MVS_ON_ME))
  {
    SetBit(LOGGING_REQUIRED);
  }
  
  return (initialVal != bitmap_) ? ATTRIBUTE_SET : NO_CHAGE;
}



//----------------------------------------------------------------------------
ComMvAttributeBitmap::Result
ComMvAttributeBitmap::setInsertLog(NABoolean value)
{
  ComSInt32 initialVal = bitmap_;

  if(TRUE == value)
  {
    SetBit(INSERTLOG);
  }
  else
  {
    ClearBit(INSERTLOG);
  }

  return (initialVal != bitmap_) ? ATTRIBUTE_SET : NO_CHAGE;
}

//----------------------------------------------------------------------------
ComMvAttributeBitmap::Result
ComMvAttributeBitmap::setLockOnRefresh(NABoolean value)
{
  ComSInt32 initialVal = bitmap_;

  if(TRUE == value)
  {
    SetBit(LOCKONREFRESH);
  }
  else
  {
    ClearBit(LOCKONREFRESH);
  }

  return (initialVal != bitmap_) ? ATTRIBUTE_SET : NO_CHAGE;
}


//----------------------------------------------------------------------------
ComMvAttributeBitmap::Result
ComMvAttributeBitmap::setEnableRewrite(NABoolean value)
{
  ComSInt32 initialVal = bitmap_;

  if(TRUE == value)
  {
    SetBit(ENABLE_REWRITE);
  }
  else
  {
    ClearBit(ENABLE_REWRITE);
  }

  return (initialVal != bitmap_) ? ATTRIBUTE_SET : NO_CHAGE;
}



//----------------------------------------------------------------------------
ComMvAttributeBitmap::Result
ComMvAttributeBitmap::setMvsAllowed(ComMvsAllowed mvsAllowedType)
{
  ComSInt32 initialVal = bitmap_;

  ClearMask(MVS_ALLOWED_MASK);

  switch(mvsAllowedType)
  {
    case COM_NO_MVS_ALLOWED:
      SetBit(NO_MVS_ALLOWED);
      break;

    case COM_ALL_MVS_ALLOWED:
      SetBit(ALL_MVS_ALLOWED);
      break;

    case COM_ON_STATEMENT_MVS_ALLOWED:
      SetBit(ONSTATEMENT_MVS_ALLOWED);
      break;

    case COM_ON_REQUEST_MVS_ALLOWED:
      SetBit(ONREQUEST_MVS_ALLOWED);
      break;

    case COM_RECOMPUTE_MVS_ALLOWED:
      SetBit(RECOMPUTE_MVS_ALLOWED);
      break;

// an exception code that we should not hit
    case COM_MVS_ALLOWED_UNKNOWN:
      bitmap_ = initialVal;
      return INTERNAL_ERROR;

    default:
      assert(0); 
  }
  
  return (initialVal != bitmap_) ? ATTRIBUTE_SET : NO_CHAGE;
} // setMvsAllowed



//----------------------------------------------------------------------------
ComMvAttributeBitmap::Result
ComMvAttributeBitmap::setMvStatus(ComMVStatus value)
{
  ComSInt32 initialVal = bitmap_;

  ClearMask(MVSTATUS_MASK);

  switch(value)
  {
    case COM_MVSTATUS_INITIALIZED:
    case COM_MVSTATUS_NO_INITIALIZATION:
      SetBit(MV_INITIALIZED);
      break;

    case COM_MVSTATUS_NOT_INITIALIZED:
      SetBit(MV_NOT_INITIALIZED);
      break;

    case COM_MVSTATUS_UNAVAILABLE:
      SetBit(MV_UNAVAILABLE);
      break;

    case COM_MVSTATUS_UNKNOWN:
    default:
      assert(0);
  }
  
  return (initialVal != bitmap_) ? ATTRIBUTE_SET : NO_CHAGE;
}

//----------------------------------------------------------------------------
ComMvAttributeBitmap::Result
ComMvAttributeBitmap::setMvAudit(ComMvAuditType value)
{
  ComSInt32 initialVal = bitmap_;

  ClearMask(MV_AUDIT_MASK);
  
  switch(value)
  {
    case COM_MV_AUDIT:
      SetBit(MV_AUDIT);
      break;

    case COM_MV_NO_AUDIT:
      SetBit(MV_NO_AUDIT);
      break;

    case COM_MV_NO_AUDIT_ON_REFRESH:
      SetBit(MV_NO_AUDITONREFRESH);
      break;

    case COM_MV_AUDIT_UNKNOWN:
    default:
      assert(0);
  }
  
  return (initialVal != bitmap_) ? ATTRIBUTE_SET : NO_CHAGE;
}

//----------------------------------------------------------------------------
ComMvsAllowed ComMvAttributeBitmap::getMvsAllowed() const
{
  if(IsBitSet(ONSTATEMENT_MVS_ALLOWED)	&&
     IsBitSet(ONREQUEST_MVS_ALLOWED)	&&
     IsBitSet(RECOMPUTE_MVS_ALLOWED)	)
  {
    return COM_ALL_MVS_ALLOWED;
  }

  if(IsBitSet(ONSTATEMENT_MVS_ALLOWED))
  {
    return COM_ON_STATEMENT_MVS_ALLOWED;
  }

  if(IsBitSet(ONREQUEST_MVS_ALLOWED))
  {
    return COM_ON_REQUEST_MVS_ALLOWED;
  }

  if(IsBitSet(RECOMPUTE_MVS_ALLOWED))
  {
    return COM_RECOMPUTE_MVS_ALLOWED;
  }

  return COM_NO_MVS_ALLOWED;
}

//----------------------------------------------------------------------------
ComRangeLogType ComMvAttributeBitmap::getRangeLogType() const
{
  if(IsBitSet(MANUAL_RANGELOG))
  {
    return COM_MANUAL_RANGELOG;
  }

  if(IsBitSet(AUTO_RANGELOG))
  {
    return COM_AUTO_RANGELOG;
  }

  if(IsBitSet(MIXED_RANGELOG))
  {
    return COM_MIXED_RANGELOG;
  }

  return COM_NO_RANGELOG;
}

//----------------------------------------------------------------------------
ComMVStatus		
ComMvAttributeBitmap::getMvStatus() const
{
  if(IsBitSet(MV_INITIALIZED))
  {
    return COM_MVSTATUS_INITIALIZED;
  }

  if(IsBitSet(MV_UNAVAILABLE))
  {
    return COM_MVSTATUS_UNAVAILABLE;
  }

  return COM_MVSTATUS_NOT_INITIALIZED;
}

//----------------------------------------------------------------------------
ComMvAuditType	
ComMvAttributeBitmap::getMvAuditType() const
{
  if(IsBitSet(MV_AUDIT))
  {
    return COM_MV_AUDIT;
  }

  if(IsBitSet(MV_NO_AUDITONREFRESH))
  {
    return COM_MV_NO_AUDIT_ON_REFRESH;
  }

  return COM_MV_NO_AUDIT;
}

