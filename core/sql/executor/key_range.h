#ifndef KEYRANGE_H
#define KEYRANGE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         key_range.h
 * Description:  defines base classes for objects describing key ranges
 *               
 *               
 * Created:      11/8/96
 * Language:     C++
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
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "ex_ex.h"
#include "ExpSqlTupp.h"
#include "ex_error.h"

// forward references
class ex_globals;
class atp_struct;
class sql_buffer_pool;

/////////////////////////////////////////////////////////////////////////
//
// Class keyRangeGen
//
// This class contains compiler-generated information used by scan
// operators that can access ordered data directly.  It is a virtual
// class that defines a common interface for different types of key
// access.  The idea is to encapsulate the generation of key ranges.
//
//
/////////////////////////////////////////////////////////////////////////

#include "ComKeyRange.h"

/////////////////////////////////////////////////////////////////////////
//
// Class keyRangeEx
//
// This class contains compiler-generated information used by scan
// operators that can access ordered data directly.  It is a virtual
// class that defines a common interface for different types of key
// access.  The idea is to encapsulate the generation of key ranges.
//
//
/////////////////////////////////////////////////////////////////////////
    
class keyRangeEx : public ExGod
{
protected:

  const keyRangeGen & tdbKey_;	  // reference to TDB side structure
  ex_globals * globals_;          // so freeResources() can get to it

  // key range information computed by getNextKeyRange() is stored here
  tupp bkData_;
  tupp ekData_;
  short bkExcludeFlag_;
  short ekExcludeFlag_;

  // work variables
  atp_struct * workAtp_;
  Lng32 excludeFlag_;
  tupp_descriptor excludeFlagTupp_;
  Lng32 dataConvErrorFlag_;
  tupp_descriptor dataConvErrorFlagTupp_;

public:

  NA_EIDPROC keyRangeEx(const keyRangeGen & tdb_key,
		   ex_globals * globals,
		   const short in_version,
		   sql_buffer_pool *pool);

  NA_EIDPROC virtual ~keyRangeEx();

  NA_EIDPROC virtual void freeResources();	// free resources

  NA_EIDPROC virtual void release(); // release

  NA_EIDPROC virtual void display() const {}; // LCOV_EXCL_LINE

  // key range iterator methods
  NA_EIDPROC virtual ExeErrorCode initNextKeyRange(sql_buffer_pool *pool,
				        	   atp_struct *atp0) = 0;

  enum getNextKeyRangeReturnType { NO_MORE_RANGES,
				   FETCH_RANGE,
				   PROBE_RANGE,
				   EXPRESSION_ERROR };

  NA_EIDPROC virtual getNextKeyRangeReturnType getNextKeyRange(
       atp_struct *atp0,NABoolean fetchRangeHadRows,
       NABoolean detectNullRange = TRUE) = 0;

  // on the next method, we pass in NULL if a probe resulted in no
  // data; we pass in the encoded key value if data was obtained
// LCOV_EXCL_START
  NA_EIDPROC virtual void reportProbeResult(char *)
  { 
    ex_assert(0,"reportProbeResult() was called when it shouldn't have been");
  };
// LCOV_EXCL_STOP

  // when getNextKeyRange() returns FETCH_RANGE or PROBE_RANGE, the
  // caller should call the next four methods to get its hands on the
  // key values
  NA_EIDPROC tupp & getBkData()
  { return bkData_; };

  NA_EIDPROC tupp & getEkData()
  { return ekData_; };

  NA_EIDPROC short getBkExcludeFlag()
  { return bkExcludeFlag_; };

  NA_EIDPROC short getEkExcludeFlag()
  { return ekExcludeFlag_; };

  NA_EIDPROC ULng32 getKeyLength()
  { return tdbKey_.getKeyLength(); };

};

#endif


