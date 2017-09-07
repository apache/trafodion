#ifndef KEYSINGLESUBSET_H
#define KEYSINGLESUBSET_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         key_single_subset.h
 * Description:  
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

/////////////////////////////////////////////////////////////////////////
//
// Class keySingleSubsetGen
//
// This class contains compiler-generated information used by scan
// operators that access a single key range.  It encapsulates methods
// needed to compute the begin and end key values and their exclusion
// flags.
//
//
/////////////////////////////////////////////////////////////////////////

#include "ComKeySingleSubset.h"


/////////////////////////////////////////////////////////////////////////
//
// Class keySingleSubsetEx
//
// This class contains run-time state used by scan operators that
// access a single key range.  It encapsulates the state and methods
// needed to compute begin and end key values and their exclusion
// flags.
//
/////////////////////////////////////////////////////////////////////////

class keySingleSubsetEx : public keyRangeEx
{
private:

  // set to FALSE by initNextKeyRange(), to TRUE by getNextKeyRange()
  NABoolean keyReturned_;  

public:

  keySingleSubsetEx(const keyRangeGen & tdb_key,
			       const short in_version,
			       sql_buffer_pool *pool,
			       ex_globals *g,
			       unsigned short mode,
                               const ex_tcb * tcb);
				  

  virtual ~keySingleSubsetEx();

  //  virtual void freeResources(); -- just inherit from base class

  virtual void display() const {};

  // key range iterator methods
  virtual ExeErrorCode initNextKeyRange(sql_buffer_pool *pool,
			        		   atp_struct *atp0);
  virtual getNextKeyRangeReturnType getNextKeyRange(
       atp_struct *atp0,NABoolean fetchRangeHadRows,
       NABoolean detectNullRange = TRUE);  

  // inlines to save some typing
  inline keySingleSubsetGen & tdbBeginEndKey() const
    { return (keySingleSubsetGen &)tdbKey_; } 

  inline ex_expr * bkPred() const
    { return tdbBeginEndKey().bkPred(); };

  inline ex_expr * ekPred() const
    { return tdbBeginEndKey().ekPred(); };

  inline ex_expr * bkExcludedExpr() const
    { return tdbBeginEndKey().bkExcludedExpr(); }

  inline ex_expr * ekExcludedExpr() const
    { return tdbBeginEndKey().ekExcludedExpr(); }

  inline short isBkeyExcluded() const 
    { return tdbBeginEndKey().isBkeyExcluded();}

  inline short isEkeyExcluded() const 
    { return tdbBeginEndKey().isEkeyExcluded();}

  inline Lng32 getExcludeFlagValue() const 
    { return excludeFlag_; }
};







#endif

