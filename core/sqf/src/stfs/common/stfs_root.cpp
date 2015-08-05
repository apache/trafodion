///////////////////////////////////////////////////////////////////////////////
// 
/// \file    stfs_root.cpp
/// \brief   Implementation of STFS_Root
//                                                                                      
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
///////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "stfs/stfslib.h"
#include "stfs_root.h"
#include "stfs_defs.h"
#include "stfs_util.h"

using namespace STFS;

STFS_Root::EyeCatcherMap_Def STFS_Root::EyeCatcherMap_[] = {
  {EC_ExternalFileMetadata,         (char *) "EFM"},
  {EC_ExternalFileHandle,           (char *) "EFH"},
  {EC_FragmentFileMetadata,         (char *) "FFM"},
  {EC_FragmentFileHandle,           (char *) "FFH"},
  {EC_ExternalFileMetadataContainer,(char *) "MFC"},
  {EC_ExternalFileHandleContainer,  (char *) "FHC"},
  {EC_ExternalFileOpenerContainer,  (char *) "EFO"},
  {EC_Session,                      (char *) "SES"},
  {EC_Error,                        (char *) "ERR"},
  {EC_Unknown,                      (char *) ""}
};

STFS_Root::STFS_Root(Enum_EC pv_EyeCatcherEnum) 
{
  memset(eyeCatcher_,
	 0,
	 sizeof(eyeCatcher_));
  strncpy(eyeCatcher_,
	  GetEyeCatcherString(pv_EyeCatcherEnum),
	  sizeof(eyeCatcher_) -1);
}

char *
STFS_Root::GetEyeCatcherString(Enum_EC pv_EyeCatcherEnum) {
  
  if ((pv_EyeCatcherEnum >= EC_Unknown) || 
      (pv_EyeCatcherEnum < EC_ExternalFileMetadata)) {
    return EyeCatcherMap_[EC_Unknown].EyeCatcherString;
  }
  
  return EyeCatcherMap_[pv_EyeCatcherEnum].EyeCatcherString;
}

bool
STFS_Root::IsEyeCatcherValid(Enum_EC pv_EyeCatcherEnum)
{
  char *lp_EyeCatcherExpected = 0;

  lp_EyeCatcherExpected = GetEyeCatcherString(pv_EyeCatcherEnum);
  if (strcmp(lp_EyeCatcherExpected,
	     eyeCatcher_) == 0) {
    return true;
  }

  return false;
  
}

size_t STFS_Root::Pack( char *pp_buf, size_t pv_spaceRemaining) {

  // right now, all we have to do is pack the eyecatcher

  if (pv_spaceRemaining >= sizeof (eyeCatcher_) ) {
    memcpy (pp_buf, &eyeCatcher_, sizeof (eyeCatcher_));
    return (sizeof (eyeCatcher_));
  }
  else {
    return 0;
  }
}

size_t STFS_Root::Unpack (char *pp_buf) {

  // right now the only thing we have to unpack is the eyecatcher

  memcpy (&eyeCatcher_, pp_buf, sizeof (eyeCatcher_));
  return ( sizeof (eyeCatcher_) ) ;

}

int STFS_Root::Lock()
{
  const char       *WHERE = "int STFS_Root::Lock";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int lv_Retval = 0;

  mutex_.lock();
  if (lv_Retval != 0) {
    TRACE_PRINTF3(1, "%s: Error %d returned while locking the mutex", WHERE, errno);
    char lv_buf[256];
    sprintf(lv_buf, "Error %d in locking STFS mutex\n", lv_Retval);
    STFS_util::STFS_evlog(STFS_EVENT_ERROR,
			  SQ_LOG_ERR,
			  (char *) &lv_buf);
  }

  return lv_Retval;
}

int STFS_Root::Unlock()
{
  const char       *WHERE = "int STFS_Root::Unlock";
  STFS_ScopeTrace   lv_st(WHERE,2);

  int lv_Retval = 0;

  lv_Retval = mutex_.unlock();
  if (lv_Retval != 0) {
    TRACE_PRINTF3(1, "%s: Error %d returned while unlocking the mutex", WHERE, errno);
    char lv_buf[256];
    sprintf(lv_buf, "Error %d in unlocking STFS mutex\n", lv_Retval);
    STFS_util::STFS_evlog(STFS_EVENT_ERROR,
			  SQ_LOG_ERR,
			  (char *) &lv_buf);
  }

  return lv_Retval;
} 

