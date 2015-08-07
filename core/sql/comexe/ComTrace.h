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

#ifndef COM_TRACE_H
#define COM_TRACE_H

#include "NAVersionedObject.h"

//
// class ComTracePointInfo
// Description:
//   Used to store a trace point and its related info in ComTdb at codegen time.
//   A flat memory buffer will be allocated for each TP set.
//   This information is used to create an ExDp2TracePoint object at runtime.
class ComTracePointInfo
{
private:
  UInt32 tracePoint_;             // trace point
  UInt32 tpCount_;                // count max if specified
  UInt16 tpActions_;              // max of 4 actions per TracePoint
  UInt16 filler_;

public:

  enum TPSeparators {
    TP_OP_START_SEP      = '(',
    TP_OP_END_SEP        = ')',
    TP_TP_ACTION_SEP     = ':',
    TP_ACTION_SEP        = ',',
    TP_ACTION_COUNT_SEP  = '.',
    TP_TP_SEP            = '|',
    TP_SPACE             = ' '
  };


  //
  // Class Methods
  //

  static Int16 parseTPString( char                   *traceString,
                              UInt32                  tdbOperator,
                              Space                  *space,
                              ComTracePointInfo     **tpInfo,
                              UInt32                 &tpCount );


  //
  // Object Methods
  //

  //
  // Ctor
  ComTracePointInfo( UInt32                            tracePoint,
                    UInt32                            counterMax,
                    UInt16                            actions )
    : tracePoint_(tracePoint),
      tpCount_(counterMax),
      tpActions_(actions),
      filler_((UInt16)0)
  { }

  //
  // Ctor
  ComTracePointInfo( UInt32                            tracePoint,
                    UInt16                            actions )
    : tracePoint_(tracePoint),
      tpCount_(0),
      tpActions_(actions),
      filler_((UInt16)0)
  { }

  void init( UInt32                                   tracePoint,
             UInt32                                   counterMax,
             UInt16                                   actions )
  {
    tracePoint_ = tracePoint;
    tpCount_    = counterMax;
    tpActions_  = actions;
    filler_     = (UInt16)0;
  }
                    
  UInt32 getTracePoint()
  {  return tracePoint_; }

  UInt32 getCounterMax()
  {  return tpCount_; }

  UInt16 getActions()
  {  return tpActions_; }

};


#endif  // COM_TRACE_H
