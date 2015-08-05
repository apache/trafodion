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
#ifndef COMMVDEFS_H
#define COMMVDEFS_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComMvDefs.h
 * Description:  Small definitions are declared here that are used throughout
 *               the SQL/ARK product.
 *
 * Created:      07/02/2000
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

#include "ComSmallDefs.h"

#define    COMMV_CTRL_PREFIX          "@"

#define    COMMV_IUD_LOG_SUFFIX     ""
#define    COMMV_RANGE_LOG_SUFFIX   ""
#define    COMMV_CTX_LOG_SUFFIX     ""

#define    COMMV_BEGINRANGE_PREFIX  COMMV_CTRL_PREFIX "BR_"
#define    COMMV_ENDRANGE_PREFIX    COMMV_CTRL_PREFIX "ER_"

#define    COMMV_EPOCH_COL          COMMV_CTRL_PREFIX "EPOCH"
#define    COMMV_CURRENT_EPOCH_COL  COMMV_CTRL_PREFIX "CURRENT_EPOCH"
#define    COMMV_OPTYPE_COL         COMMV_CTRL_PREFIX "OPERATION_TYPE"
#define    COMMV_IGNORE_COL         COMMV_CTRL_PREFIX "IGNORE"
#define    COMMV_BITMAP_COL         COMMV_CTRL_PREFIX "UPDATE_BITMAP"
#define    COMMV_RANGE_SIZE_COL     COMMV_CTRL_PREFIX "RANGE_SIZE"
#define    COMMV_ALIGNMENT_COL      COMMV_CTRL_PREFIX "ALIGNMENT"
#define    COMMV_BASE_SYSKEY_COL    COMMV_CTRL_PREFIX "SYSKEY"
#define    COMMV_RANGE_ID_COL	    COMMV_CTRL_PREFIX "RANGE_ID"
#define    COMMV_RANGE_TYPE_COL	    COMMV_CTRL_PREFIX "RANGE_TYPE"
#define    COMMV_RANGE_SYSKEY_COL   COMMV_CTRL_PREFIX "RANGE_SYSKEY"
#define    COMMV_SYSKEY_COL	    COMMV_CTRL_PREFIX "SYSKEY"
#define    COMMV_TS_COL             COMMV_CTRL_PREFIX "TS"

#define    COMMV_EPOCH_QCOL          "\"" COMMV_CTRL_PREFIX "EPOCH\""
#define    COMMV_CURRENT_EPOCH_QCOL  "\"" COMMV_CTRL_PREFIX "CURRENT_EPOCH\""
#define    COMMV_OPTYPE_QCOL         "\"" COMMV_CTRL_PREFIX "OPERATION_TYPE\""
#define    COMMV_IGNORE_QCOL         "\"" COMMV_CTRL_PREFIX "IGNORE\""
#define    COMMV_BITMAP_QCOL         "\"" COMMV_CTRL_PREFIX "UPDATE_BITMAP\""
#define    COMMV_RANGE_SIZE_QCOL     "\"" COMMV_CTRL_PREFIX "RANGE_SIZE\""
#define    COMMV_ALIGNMENT_QCOL      "\"" COMMV_CTRL_PREFIX "ALIGNMENT\""
#define    COMMV_BASE_SYSKEY_QCOL    "\"" COMMV_CTRL_PREFIX "SYSKEY\""
#define    COMMV_RANGE_ID_QCOL	     "\"" COMMV_CTRL_PREFIX "RANGE_ID\""
#define    COMMV_RANGE_TYPE_QCOL     "\"" COMMV_CTRL_PREFIX "RANGE_TYPE\""
#define    COMMV_SYSKEY_QCOL	     "\"" COMMV_CTRL_PREFIX "SYSKEY\""
#define    COMMV_TS_QCOL             "\"" COMMV_CTRL_PREFIX "TS\""

// The Epoch number for multi-transactional context rows in the IUD log.
enum { EPOCH_FOR_MULTI_TXN  = 1 };

// Possible values for the ROW_TYPE column of the IUD log.
enum ComMvIudLogRowType {	
    ComMvRowType_Insert         =  0, 
    ComMvRowType_Delete         =  1,
    ComMvRowType_Update         =  2,
    ComMvRowType_InsertOfUpdate =  ComMvRowType_Insert | ComMvRowType_Update, // 2
    ComMvRowType_DeleteOfUpdate =  ComMvRowType_Delete | ComMvRowType_Update, // 3
    ComMvRowType_EndRange       =  4, 
    ComMvRowType_BeginRange     = 12, 
    ComMvRowType_MidRange       = 16
  };

// The four possible types of ranges in the range log.
enum ComMvRangeType {
    ComMvRangeClosedBothBounds  = 0, // ()
    ComMvRangeClosedLowerBound  = 1, // [)
    ComMvRangeClosedUpperBound  = 2, // (]
    ComMvRangeOpenBothBounds    = 3  // []
  };

class RcbMvRelatedInfo
{
public:
  RcbMvRelatedInfo() : 
  epochDirty(FALSE),
  bitmapDirty(FALSE),
  refreshedAtDirty(FALSE),
  epochNewVal(0),
  bitmapNewVal(0), 
  refreshedAtTime(0)
  {}
  
  ComSInt32     epochNewVal;
  ComSInt32     bitmapNewVal; 
  ComTimestamp	refreshedAtTime; // for MVs only
	  
  ComBoolean	epochDirty;
  ComBoolean	bitmapDirty;
  ComBoolean	refreshedAtDirty;

  void trace() const
  {
    ComMvAttributeBitmap bitmap;
    bitmap.initBitmap(bitmapNewVal);
    
    cout << endl << "LABLE MV RELATED INFO" << endl;

    cout << "EPOCH: DIRTY "   << epochDirty << " VAL " << epochNewVal << endl;
    cout << "BITMAP: DIRTY "  << bitmapDirty << endl;
    bitmap.trace();

//    cout << "REFRESHEDAT: DIRTY " << refreshedAtDirty << " VAL " << refreshedAtTime << endl;
//    cout << "IUDCOUNTER: DIRTY " << << " VAL " << << endl;
//    cout << "SELECTCOUTNER: DIRTY " << << " VAL " << << endl;

  }

}; 

#endif // COMMVDEFS_H
