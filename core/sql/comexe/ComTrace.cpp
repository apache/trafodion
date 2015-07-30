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

#include "ComTrace.h"

// ==========================================================
// ComTracePointInfo Methods
// ==========================================================

Int16 ComTracePointInfo::parseTPString( char               *traceString, // in
                                       UInt32               tdbOperator, // in
                                       Space               *space,       // in
                                       ComTracePointInfo  **tpInfo,      // out
                                       UInt32              &numTPs )     // out
{
  Int16  rc = 0;
  char  *tpStr = traceString;
  char   tpChar;
  UInt32 num = 0;
  UInt16 actions = 0;
  Int32  countMax = 0;
  UInt32 op = 0;

  if ( NULL == traceString )
    return rc;

  // These next 3 arrays are maintained for a given operator to collect
  // up all the trace points for the operator and then allocate them all
  // in 1 chunk.
  const Int32 tpCount = 8;
  UInt32 tps[ tpCount ];
  UInt16 tpActions[ tpCount ];
  UInt32 counters[ tpCount ];
  UInt32 tpIdx = 0;

  while( TRUE )
  {
    tpChar = *tpStr;

    switch( tpChar )
    {
      case TP_OP_START_SEP:
      {
        if ( num == tdbOperator )
          op = num;

        num = 0;
        break;
      }

      case TP_TP_ACTION_SEP:
      {
        if ( op > 0 )
        {
          tps[ tpIdx ] = num;
          num = 0;
          countMax = 0;
          actions = 0;
        }
        break;
      }

      case TP_ACTION_SEP:
      {
        if ( op > 0 )
        {
          if ( countMax < 0 )
            countMax = num;
          else
            actions ^= num;

          num = 0;
        }
        break;
      }

      case TP_ACTION_COUNT_SEP:
      {
        if ( op > 0 )
        {
          actions ^= num;
          countMax = -1;
          num = 0;
        }
        break;
      }

      case TP_OP_END_SEP:
      case TP_TP_SEP:
      {
        if ( op > 0 )
        {
          if ( countMax < 0 )
            countMax = num;
          else
            actions ^= num;

          num = 0;

          tpActions[ tpIdx ] = actions;
          counters[ tpIdx ] = countMax;
          tpIdx++;
        }

        if ( (char)TP_OP_END_SEP == tpChar )
          op = 0;

        break;
      }

      case TP_SPACE:
      {
        break;
      }

      default:
      {
        num = (num * 10) + (tpStr[0] - '0');
        break;
      }
    }

    tpStr++;
    if ( '\0' == *tpStr )
      break;
  }

  // Now if we found the operator we were looking for then allocate the
  // memory for all the trace points.
  if ( tpIdx > 0 )
  {
    ComTracePointInfo *tmp;
    Int32 sz = (sizeof(ComTracePointInfo) * tpIdx);
    char *buff = (char *)space->allocateMemory( sz, FALSE );

    *tpInfo = (ComTracePointInfo *)buff;
    numTPs  = tpIdx;

    for( UInt32 k = 0; k < tpIdx; k++ )
    {
      tmp = (ComTracePointInfo *)buff;
      tmp->init( tps[ k ], counters[ k ], tpActions[ k ] );

      buff = buff + sizeof(ComTracePointInfo);
    }
  }

  return rc;
}
