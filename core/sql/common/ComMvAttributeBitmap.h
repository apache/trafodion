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
#ifndef COMMVATTRIBUTEBITMAP_H
#define COMMVATTRIBUTEBITMAP_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComMvAttributeBitmap.h
 * Description:  
 *
 * Created:      04/04/2000
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

#include "ComSmallDefs.h"

class ComMvAttributeBitmap : public NABasicObject
{
public:

  enum Result
  {
    ATTRIBUTE_SET,
    NO_CHAGE,
    INTERNAL_ERROR
  };
  
  ComMvAttributeBitmap()
    : bitmap_(0)
  {}

  void initBitmap(ComSInt32 initialiVal);

  ComSInt32 getBitmap() const;

  void trace();

  Result setLogsCreatedOnMVCreation();
  Result setLogCreated();
  Result setLastOnRequestMvOnMe();

  Result setIsAnMv(NABoolean value);

  Result setInitOnStmtMvOnMe(NABoolean value);

  // BASE TABLE ATTRIBS
  void clearBaseTableAttributes();

  Result setRangeLogType(ComRangeLogType rangeLogType);
  Result setInsertLog(NABoolean value);
  Result setLockOnRefresh(NABoolean value);
  Result setMvsAllowed(ComMvsAllowed mvsAllowedType); 

  // MV ATTRIBS
  void clearMvAttributes();

  Result setEnableRewrite(NABoolean value);
  Result setMvStatus(ComMVStatus value);
  Result setMvAudit(ComMvAuditType value);

  ComMvsAllowed	  getMvsAllowed() const;
  ComRangeLogType getRangeLogType() const;
  ComMVStatus	  getMvStatus() const;
  ComMvAuditType  getMvAuditType() const;

  inline NABoolean getIsAnMv() const
  { return IsBitSet(IS_AN_MV); }

  inline NABoolean getIsInsertLog() const
  { return IsBitSet(INSERTLOG); }

  inline NABoolean getIsLockOnRefresh() const
  { return IsBitSet(LOCKONREFRESH); }

  inline NABoolean getIsEnableRewrite() const
  { return IsBitSet(ENABLE_REWRITE); }

  inline NABoolean getIsMvUnInitialized() const
  { return !IsBitSet(MV_INITIALIZED); }

  inline NABoolean getIsMvUnAvailable() const
  { return IsBitSet(MV_UNAVAILABLE); }

  inline NABoolean getLogCreatedOnMVCreation() const
  { return IsBitSet(LOGS_ON_CREATION); }

  inline NABoolean getLoggingRequired() const
  { return IsBitSet(LOGGING_REQUIRED); }

  // Note: mixed is auto logging but not auto range logging.
  inline NABoolean getAutomaticRangeLoggingRequired() const
  { return (IsBitSet(LOGGING_REQUIRED | AUTO_RANGELOG) ); }

  // exclude from coverage since this method is used for similarity
  // check and now we use AQR instead
  inline NABoolean getInitOnStmtMvOnMe() const
  { return (IsBitSet(INIT_ON_STMT_ON_ME)); }

  // exclude from coverage since Range Logging is not supported
  inline NABoolean getEnableMVLOGExecution() const
  { 
    return (IsBitSet(MVS_ON_ME | MANUAL_RANGELOG) || 
	    IsBitSet(MVS_ON_ME | MIXED_RANGELOG) );
  }

private:

  enum MvBitmapValues
  {
    // What is the logging status for this base table?
    NO_LOGS			= 0x00000000,
    LOGS_CREATED		= 0x00000001,
    LOGS_ON_CREATION		= 0x00000002,
    LOGGING_REQUIRED		= 0x00000004,
    MVS_ON_ME			= 0x00000010,

    // Is an initialized ON STATEMENT MV is defined on me?
    NO_INIT_ON_STMT_ON_ME	= 0x00000000,
    INIT_ON_STMT_ON_ME		= 0x00000020,

    // Is this base table also an MV?
    NOT_AN_MV			= 0x00000000,
    IS_AN_MV			= 0x00000040,  

    // The value of the base table's RANGELOG attribute.
    NO_RANGELOG			= 0x00000000,
    MANUAL_RANGELOG		= 0x00000100,
    AUTO_RANGELOG		= 0x00000200,
    MIXED_RANGELOG		= 0x00000400,
    
    // Was this base table defined with the INSERTLOG attribute?
    NO_INSERTLOG		= 0x00000000,
    INSERTLOG			= 0x00001000,

    // Was this base table defined with the NO LOCKONREFRESH attribute?
    NO_LOCKONREFRESH		= 0x00000000,
    LOCKONREFRESH		= 0x00002000,

    // Is query rewrite disabled for this MV?
    DISABLE_REWRITE		= 0x00000000,
    ENABLE_REWRITE		= 0x00004000,

    // What type of MVs are allowed on this base table?
    NO_MVS_ALLOWED		= 0x00000000,
    ALL_MVS_ALLOWED		= 0x00070000,
    ONSTATEMENT_MVS_ALLOWED	= 0x00010000,
    ONREQUEST_MVS_ALLOWED	= 0x00020000,
    RECOMPUTE_MVS_ALLOWED	= 0x00040000,

    // What is the status of this MV?
    MV_NOT_INITIALIZED		= 0x00000000,
    MV_INITIALIZED		= 0x00100000,
    MV_UNAVAILABLE		= 0x00200000,

    // What is the audit mose for this MV?
    MV_NO_AUDIT			= 0x00000000,
    MV_AUDIT			= 0x01000000,
    MV_NO_AUDITONREFRESH	= 0x02000000,

    LAST_LOG_ATTRIBUTE		= 0x77777777
  };

  enum BitmapMasks
  {
    LOGS_STATUS_MASK		= 0x00000017,
    IS_INIT_ON_STMT_ON_ME_MASK  = 0x00000020,
    IS_MV_MASK			= 0x00000040,
    RANGE_LOGGING_MASK		= 0x00000700,
    INSERT_LOGGING_MASK		= 0x00001000,
    LOCKONREFRESH_MASK		= 0x00002000,
    REWRITE_MASK		= 0x00004000,
    MVS_ALLOWED_MASK		= 0x00070000,
    MVSTATUS_MASK		= 0x00300000,
    MV_AUDIT_MASK		= 0x03000000,


    BASE_TABLE_ATTRIBUTES_MASK	= ( RANGE_LOGGING_MASK	| 
				    INSERT_LOGGING_MASK | 
				    LOCKONREFRESH_MASK	| 
				    MVS_ALLOWED_MASK ),

    MV_ATTRIBUTES_MASK		= ( REWRITE_MASK  | 
				    MVSTATUS_MASK | 
				    MV_AUDIT_MASK ),

    LAST_BITMAP_MASK		= 0x77777777
  };

  inline NABoolean IsBitSet(ComSInt32 flag) const 
    { return ((bitmap_ & flag) == flag) ? TRUE : FALSE; }
  inline void SetBit(ComSInt32 flag)    { bitmap_ |=  flag; }
  inline void ClearBit(ComSInt32 flag)  { bitmap_ &= ~flag; }
  inline void ClearMask(ComSInt32 mask) { bitmap_ &= ~mask; }

  ComSInt32 bitmap_;
};


#endif // COMMVATTRIBUTEBITMAP_H

