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
#ifndef THBASE_H
#define THBASE_H 1

#include "Collections.h"
#include "ValueDesc.h"
#include "NABasicObject.h"
#include "CmpCommon.h"

struct HbaseSearchSpec : public NABasicObject
{

 HbaseSearchSpec(NAHeap* h = NULL) 
   : colNames_(h ? h : STMTHEAP), rowTS_(0) {};
  void addColumnNames(const ValueIdSet& vs);
  const NAString getText() const;

  // column names to be retrieved.
  // Same set to be retrieved for all row id entries in rowIds_
  // Format of each entry:
  //         colfam:colname      ==> to retrieve 'colname' from 'colfam'
  //         colfam:             ==> to retrieve all columns in that family
  NAList<NAString> colNames_;
  
  // row timestamp at which the row is to be retrieved
  // If -1, latest timestamp
  Int64 rowTS_;
  
 protected:
};

// This struct is used to specify unique rowids at runtime by
// calling 'Get' methods of hbase api.
// A list of these structs is created and each entry is evaluated
// at runtime.
// If rowIds_ contains one entry, then 'getRow' methods are used.
// If rowIds_ contain multiple entries, then 'getRows' methods are used.

struct HbaseUniqueRows : public HbaseSearchSpec
{
  HbaseUniqueRows(NAHeap* h = NULL) 
   : HbaseSearchSpec(h), rowIds_(h ? h : STMTHEAP) {};
    const NAString getText() const;

    // list of rowIds 
    NAList<NAString> rowIds_;
};


// This struct is used to specify range of rowids at runtime by
// calling 'scanner' methods of hbase api.
// A list of these structs is created and each entry is evaluated
// at runtime.

struct HbaseRangeRows : public HbaseSearchSpec
{
    // range of rowids .
    // If begin is null, start at beginning.
    // If end is null, stop at end.
    // If both are null, scan all rows.
    NAString beginRowId_;
    NAString endRowId_;

    NABoolean beginKeyExclusive_;
    NABoolean endKeyExclusive_;

 HbaseRangeRows(NAHeap* h = NULL) 
   : HbaseSearchSpec(h) {} ; 

    const NAString getText() const;
};

struct ListOfUniqueRows : public NAList<HbaseUniqueRows> 
{
 ListOfUniqueRows(NAHeap* h = NULL) : 
  NAList<HbaseUniqueRows>(h ? h : STMTHEAP) {};
  const NAString getText() const;
};

struct ListOfRangeRows : public NAList<HbaseRangeRows>
{
 ListOfRangeRows(NAHeap* h = NULL) : 
  NAList<HbaseRangeRows>(h ? h : STMTHEAP) {};
  const NAString getText() const;
};

#endif
