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
#ifndef CONST_H
#define CONST_H

#include "Platform.h"
#include "BaseTypes.h"

const Lng32 ONE_MB=1048576;  //1024 * 1024

const Int32 SORT_SUCCESS = 0;
const Int32 SORT_FAILURE = 1;
const Int32 SORT_IO_IN_PROGRESS =2;

const Int32 SORT_MERGENODE_NUM_BUFFERS=2;

const short REPL_SELECT = 1;
const short QUICKSORT   = 2;
const short ITER_QUICKSORT =3;
const Int32 SCRATCH_BLOCK_SIZE = 56*1024;
const Int32 MAXSCRFILES = 4096; // Must be equal to MAXRUNS
const Int32 FILENAMELEN = 48;   // For NSK
const Int32 MAX_PATH_LEN = 256; // For NT
const Int32 MAXRUNS     = 4096;
const Int32 OVERHEAD    = 20;    // The overhead for Scratch Buffer header struct
const Int32 MAX_SCRATCH_FILE_OPENS = 4;

// These extent sizes are recommende dby DP2 in
// support for setmode(141,5) and setmode(141,9) and setmode(141,11)options.
const Int32 PRIMARY_EXTENT_SIZE = 32452;
const Int32 SECONDARY_EXTENT_SIZE = 32788;
const Int32 MAX_EXTENTS = 16;
const Lng32 SCRATCH_FILE_SIZE = 2147483647; //for NT, UNIX, 2GB

// For setmode(141,11), dp2 requires last 8 bytes of 56kb block free.
// Dp2 recommend using setmode(141,11) instead of setmode(141,9) for performance
const Int32 DP2_CHECKSUM_BYTES = 8; 

const short KEYS_ARE_EQUAL  = 0;
const short KEY1_IS_SMALLER = -1;
const short KEY1_IS_GREATER = 1;

typedef Lng32  SBN;
const Int32 TRUE_L  = 1;
const Int32 FALSE_L = 0;

const Int32 MAX_ALLOC_SIZE= 127 * 1024 * 1024;

enum RESULT {SCRATCH_SUCCESS = 0,
            SCRATCH_FAILURE = 1,
            IO_NOT_COMPLETE,
            PREVIOUS_FAIL, DISK_FULL, FILE_FULL, READ_EOF,
	     WRITE_EOF, IO_COMPLETE, NEGATIVE_SEEK,
            END_OF_RUN,OTHER_ERROR};

enum SORT_STATE
{
  SORT_INIT = 0,
  SORT_SEND = 1,
  SORT_SEND_END = 2,
  SORT_RECEIVE,
  SORT_MERGE,
  SORT_INTERMEDIATE_MERGE,
  SORT_FINAL_MERGE,
  SORT_END
};

#endif









