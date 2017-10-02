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
 *****************************************************************************
 *
 * File:         key_range.C
 * Description:  Implements methods for the base classes of key range objects
 *               
 *               
 * Created:      11/11/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "ex_stdh.h"
#include "key_range.h"





keyRangeEx::keyRangeEx(const keyRangeGen & tdb_key,
	     ex_globals * g,
	     const short in_version,
	     sql_buffer_pool *pool
	     ) :
     tdbKey_(tdb_key),globals_(g),bkExcludeFlag_(0),ekExcludeFlag_(0)
{
  pool->get_free_tuple(bkData_, tdb_key.getKeyLength());
  pool->get_free_tuple(ekData_, tdb_key.getKeyLength());

  workAtp_ = allocateAtp(tdbKey_.getWorkCriDesc(),g->getSpace());
  pool->get_free_tuple(workAtp_->getTupp(tdbKey_.getKeyValuesAtpIndex()),
		       0);

  // note that begin and end key buffers are hooked up to the work atp
  // dynamically by getNextKeyRange()
  //   workAtp->getTupp(tdbKey_.getKeyValuesAtpIndex()) = bkData_;
  //   workAtp->getTupp(tdbKey_.getKeyValuesAtpIndex()) = ekData_;
      
  // hook up the exclusion flag tupp to the exclusion flag data member
  excludeFlagTupp_.init(sizeof(excludeFlag_),
			NULL,
			(char *) &excludeFlag_);
  workAtp_->getTupp(tdbKey_.getExcludeFlagAtpIndex()) = &excludeFlagTupp_;

  // hook up the data conversion error flag to appropriate member
  dataConvErrorFlagTupp_.init(sizeof(dataConvErrorFlag_),
			      NULL,
			      (char *) &dataConvErrorFlag_);
  workAtp_->getTupp(tdbKey_.getDataConvErrorFlagAtpIndex()) =
    &dataConvErrorFlagTupp_;
    
};



keyRangeEx::~keyRangeEx()
{
  freeResources();
};


void keyRangeEx::freeResources()
{
bkData_.release();
ekData_.release();

workAtp_->release();
deallocateAtp(workAtp_,globals_->getSpace());
workAtp_ = NULL;
}

void keyRangeEx::release()
{
// do nothing in base class for now
}






