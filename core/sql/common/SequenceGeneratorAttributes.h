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
****************************************************************************
*
* File:         SequenceGeneratorAttributes.h        
* Description:  The attributes of the sequence generator
* Created:      4/22/08
* Language:     C++
*
****************************************************************************/
 
#ifndef SEQUENCEGENERATORATTRIBUTES_H
#define SEQUENCEGENERATORATTRIBUTES_H

#include "ComSmallDefs.h"

class ComSpace;

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class SequenceGeneratorAttributes;

// ***********************************************************************
// SequenceGeneratorAttributes contains all the attributes of a
// sequence generator
// ***********************************************************************
class SequenceGeneratorAttributes : public NABasicObject
{
public:
  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  SequenceGeneratorAttributes(const Int64                     psgStartValue,
                              const Int64                     psgIncrement,
                              const Int64                     psgMaxValue,
                              const Int64                     psgMinValue,                    
                              const ComSequenceGeneratorType  psgSGType,
                              const ComSQLDataType            psgSQLDataType,
                              const ComFSDataType             psgFSDataType,
                              const NABoolean                 psgCycleOption,
                              const NABoolean                 psgResetOption,
			      const ComUID                    psgObjectUID,
			      const Int64                         psgCache,
			      const Int64                         psgNextValue,
			      const Int64                         psgEndValue = 0,
                              const Int64                         psgRedefTime = 0,
                              CollHeap * h=0)
    : sgStartValue_(psgStartValue),
    sgIncrement_(psgIncrement),
    sgMaxValue_(psgMaxValue),
    sgMinValue_(psgMinValue),
    sgSGType_(psgSGType),
    sgSQLDataType_(psgSQLDataType),
    sgFSDataType_(psgFSDataType),
    sgCycleOption_(psgCycleOption),
    sgResetOption_(psgResetOption),
    sgObjectUID_(psgObjectUID),
    sgCache_(psgCache),
    sgNextValue_(psgNextValue),
    sgEndValue_(psgEndValue),
    sgRedefTime_(psgRedefTime),
    sgRetryNum_(100)
  {}

      
 SequenceGeneratorAttributes(CollHeap * h=0)
   : sgStartValue_(0),
    sgIncrement_(0),
    sgMaxValue_(0),
    sgMinValue_(0),
    sgSGType_(COM_UNKNOWN_SG),
    sgSQLDataType_(COM_UNKNOWN_SDT),
    sgFSDataType_(COM_UNKNOWN_FSDT),
    sgCycleOption_(FALSE),
    sgResetOption_(FALSE),
    sgObjectUID_(0),
    sgCache_(0),
    sgNextValue_(0),
    sgEndValue_(0),
    sgRedefTime_(0),
    sgRetryNum_(100)
      {}
  
  // copy ctor
  SequenceGeneratorAttributes (const SequenceGeneratorAttributes & sga, CollHeap * h=0) 
    : 
  sgStartValue_(sga.sgStartValue_),
    sgIncrement_(sga.sgIncrement_),
    sgMaxValue_(sga.sgMaxValue_),
    sgMinValue_(sga.sgMinValue_),
    sgSGType_(sga.sgSGType_),
    sgSQLDataType_(sga.sgSQLDataType_),
    sgFSDataType_(sga.sgFSDataType_),
    sgCycleOption_(sga.sgCycleOption_),
    sgResetOption_(sga.sgResetOption_),
    sgObjectUID_(sga.sgObjectUID_),
    sgCache_(sga.sgCache_),
    sgNextValue_(sga.sgNextValue_),
    sgEndValue_(sga.sgEndValue_),
    sgRedefTime_(sga.sgRedefTime_),
    sgRetryNum_(100)
      {}
  
  // ---------------------------------------------------------------------
  // Sequence generator functions
  // ---------------------------------------------------------------------

  const Int64                      &getSGStartValue()   const     { return sgStartValue_; }
  const Int64                      &getSGIncrement()    const     { return sgIncrement_; }
  const Int64                      &getSGMaxValue()     const     { return sgMaxValue_; }
  const Int64                      &getSGMinValue()     const     { return sgMinValue_; }
  const ComSequenceGeneratorType   &getSGSGType()       const     { return sgSGType_; }
  const ComSQLDataType             &getSGSQLDataType()  const     { return sgSQLDataType_; }
  const ComFSDataType              &getSGFSDataType()   const     { return sgFSDataType_; }
  const NABoolean                   &getSGCycleOption()  const	  { return sgCycleOption_; }
  const NABoolean                  &getSGResetOption() const           {return sgResetOption_;}
  const ComUID                     &getSGObjectUID()    const     { return sgObjectUID_; }
  const Int64                        &getSGCache()   const     { return sgCache_; }
  const Int64                      &getSGNextValue()   const     { return sgNextValue_; }
  const Int64                      &getSGEndValue()   const     { return sgEndValue_; }
  const Int64                      &getSGRedefTime() const     { return sgRedefTime_; }
  const UInt32			&getSGRetryNum() const     { return sgRetryNum_; }

  void setSGRetryNum(const UInt32 v) 
  { sgRetryNum_ = v; }

  void setSGStartValue(const Int64 psgStartValue)
  { sgStartValue_= psgStartValue; }

  void setSGIncrement(const Int64 psgIncrement)
  { sgIncrement_ = psgIncrement; }

  void setSGMaxValue(const Int64 psgMaxValue)
  { sgMaxValue_ = psgMaxValue; }

  void setSGMinValue(const Int64 psgMinValue)
  { sgMinValue_ = psgMinValue; }

  void setSGSGType(const ComSequenceGeneratorType psgSGType)
  { sgSGType_ = psgSGType; }

  void setSGSQLDataType(const ComSQLDataType psgSQLDataType)
  { sgSQLDataType_ = psgSQLDataType; }

  void setSGFSDataType(const ComFSDataType psgFSDataType)
  { sgFSDataType_ = psgFSDataType; }

 void  setSGCycleOption(const NABoolean psgCycleOption)
  { sgCycleOption_ = psgCycleOption; }

 void  setSGResetOption(const NABoolean psgResetOption)
  { sgResetOption_ = psgResetOption; }

 void setSGObjectUID(const ComUID psgObjectUID)
  { sgObjectUID_ = psgObjectUID; }
  
  void setSGCache(const Int64 psgCache)
  { sgCache_= psgCache; }

  void setSGNextValue(const Int64 psgNextValue)
  { sgNextValue_= psgNextValue; }

  void setSGEndValue(const Int64 psgEndValue)
  { sgEndValue_= psgEndValue; }

  void setSGRedefTime(const Int64 psgRedefTime)
  { sgRedefTime_= psgRedefTime; }

  static void genSequenceName(const NAString &catName, const NAString &schName, 
                              const NAString &tabName, const NAString &colName,
                              NAString &seqName);
  
  const void display (ComSpace *space, NAString * nas, 
                      NABoolean noNext = FALSE) const;
 
private:

  // Sequence generator

  Int64                     sgStartValue_;
  Int64                     sgIncrement_;
  Int64                     sgMaxValue_;
  Int64                     sgMinValue_;
  ComSequenceGeneratorType  sgSGType_;
  ComSQLDataType            sgSQLDataType_;
  ComFSDataType             sgFSDataType_;
  NABoolean                 sgCycleOption_;
  NABoolean                 sgResetOption_;
  ComUID                    sgObjectUID_;
  Int64                        sgCache_;
  Int64                        sgNextValue_;
  Int64                        sgEndValue_;
  Int64                        sgRedefTime_;
  UInt32			sgRetryNum_;
}; // class SequenceGeneratorAttributes

#endif  /* SEQUENCEGENERATORATTRIBUTES_H */
