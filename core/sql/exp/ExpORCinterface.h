/* -*-C++-*-
****************************************************************************
*
* File:             ExpHbaseInterface.h
* Description:  Interface to Hbase world
* Created:        5/26/2013
* Language:      C++
*
*
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
*
****************************************************************************
*/

#ifndef EXP_ORC_INTERFACE_H
#define EXP_ORC_INTERFACE_H

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <iostream>

#include "Platform.h"
#include "Collections.h"
#include "NABasicObject.h"

#include "OrcFileReader.h"

typedef std::string Text;

class ExpORCinterface : public NABasicObject
{
 public:

  static ExpORCinterface* newInstance(CollHeap* heap, 
                                      const char* server = NULL, 
                                      const Lng32 port = -1);
  
  ExpORCinterface(CollHeap* heap, const char* server, 
                  const Lng32 port);
                    
  ~ExpORCinterface();
  
  Lng32 init();
  
  // orcFileName:   location and name of orc file
  // startRowNum: first rownum to be returned. 
  // stopRowNum:  last rownum to be returned
  //    Rownums start at 1 and stop at N. If N is -1, then all rows are to be returned.
  Lng32 scanOpen(
                 char * orcFileName,
                 const Int64 startRowNum, 
                 const Int64 stopRowNum);

  // orcRow:   pointer to buffer where ORC will return the row.
  //                Buffer is allocated by caller.
  //                Row format: 4-bytes len followed by len bytes data for each col returned.
  //                                   Len of zero indicates NULL value.
  // rowLen:   On input, length of allocated orcRow.
  //                On output, length of row returned
  // rowNum:  rownum of returned row. Must be >= startRowNum and <= stopRowNum.
  // numCols: number of columns that are part of this row.
  //
  // Return Code:
  //                          0, if a row is returned
  //                         -ve num, if error
  //                          100: EOD
  //
  Lng32 scanFetch(char* row, Int64 &rowLen, Int64 &rowNum, Lng32 &numCols);

  Lng32 scanClose();

  Lng32 getRowCount(char * orcFileName, Int64 &count);

  char * getErrorText(Lng32 errEnum);
protected:
    
private:
  Int32  retCode_;

  CollHeap * heap_;
  
  char server_[1000];
  Lng32 port_;

  OrcFileReader * ofr_;
  Int64 startRowNum_;
  Int64 stopRowNum_;
  Int64 currRowNum_;
 };

#endif
