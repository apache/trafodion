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
* File:         UdrTableDescInfo.h
* Description:  Metadata for child table descriptor info of a TMUDF tcb
* Created:      2/10/2010
* Language:     C++
*
****************************************************************************
*/

#ifndef UDR_TABLE_DESC_INFO_H
#define UDR_TABLE_DESC_INFO_H

#include "UdrFormalParamInfo.h"
#include "NAVersionedObject.h"
#include "ComSmallDefs.h"


//
// Contents of this file
//
class UdrTableDescInfo;

//
// Template instantiation to produce a 64-bit pointer emulator class
// for UdrTableDescInfo
//
typedef NAVersionedObjectPtrTempl<UdrTableDescInfo> UdrTableDescInfoPtr;

//
// Template instantiation to produce a 64-bit pointer emulator class
// for UdrFormalParamInfoPtr
//
typedef NAVersionedObjectPtrArrayTempl<UdrTableDescInfoPtr> UdrTableDescInfoPtrPtr;
typedef UdrFormalParamInfo UdrColumnDescInfo;
typedef NAVersionedObjectPtrTempl<UdrColumnDescInfo> UdrColumnDescInfoPtr;

//
// Template instantiation to produce a 64-bit pointer emulator class
// for UdrFormalParamInfoPtr
//
typedef NAVersionedObjectPtrArrayTempl<UdrColumnDescInfoPtr> UdrColumnDescInfoPtrPtr;
//
// class UdrTableDescInfo
//
// Each instance of this class describes a  table descritpor of a child table
//
class UdrTableDescInfo : public NAVersionedObject
{
 public:
  UdrTableDescInfo(const char *corrName,
		   Int16 numColumns,
                   UInt32 outputRowLength,
                   UInt16 outputTuppIndex,
		   UdrColumnDescInfoPtrPtr colDescList);

  void setCorrName(char *name) { corrName_=name;}
  char *getCorrName() const {return corrName_.getPointer(); }

  void setNumColumns(Int16 numcols) { numColumns_ = numcols;}
  Int16 getNumColumns() const { return numColumns_; }

  void setRowLength(UInt32 val) { outputRowLen_ = val;}
  UInt32 getRowLength() const { return outputRowLen_; }

  void setOutputTuppIndex(UInt16 val) { outputTuppIndex_ = val;}
  UInt16 getOutputTuppIndex() const { return outputTuppIndex_; }

  inline const UdrColumnDescInfo *getColumnDescInfo(UInt32 i) const
  {
    return colDescList_[i];
  }
  inline void setColumnDescInfo(UInt32 i, UdrColumnDescInfo *info)
  {
    colDescList_[i] = info;
  }
   // Redefine virtual functions required for versioning
  UdrTableDescInfo() : NAVersionedObject(-1)
  {}
  virtual unsigned char getClassVersionID()
  { return 1; }
  virtual void populateImageVersionIDArray()
  { setImageVersionID(0, getClassVersionID()); }
  virtual short getClassSize()
  { return sizeof(UdrTableDescInfo); }

  virtual Long pack(void *);
  virtual Lng32 unpack(void *, void *);
 private:
  // Fields in this class describe a UDR table desc . They do not
  // necessarily describe how an actual data value will be represented in
  // the message that gets sent to the UDR server.
   
  NABasicPtr corrName_;  // corr name of the table //00 -07
  Int16 numColumns_;  // number of columns i the table// 08-09
  UInt16 outputTuppIndex_; // tuppindex in workCriDesc // 10-11
  UInt32 outputRowLen_;   // length of output row // 12-15
  UdrColumnDescInfoPtrPtr colDescList_; // an array of pointers to each //16 -23
                                          // column descriptor
  char fillersUdrTableDescInfo[8];           // 24 -31
};

#endif
