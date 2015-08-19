/* -*-C++-*-
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
 *****************************************************************************
 *
 * File:         ParScannedTokenQueue.h
 * Description:  definitions of non-inline methods for class(es)
 *               ParScannedTokenQueue
 *
 * Created:      5/31/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ComOperators.h"
#include "ComSmallDefs.h"
#include "ParScannedTokenQueue.h"

// -----------------------------------------------------------------------
// definitions of non-inline methods of class ParScannedTokenQueue
// -----------------------------------------------------------------------

//
// constructor
//

ParScannedTokenQueue::ParScannedTokenQueue()
: currentPos_(-1)
{
  for (Int32 i = 0; i < getQueueSize(); i++)
  {
    scannedTokens_[i].tokenStrPos = 0;
    scannedTokens_[i].tokenStrLen = 0;
    scannedTokens_[i].tokenStrInputLen = 0;
    scannedTokens_[i].tokenStrOffset = 0;
    scannedTokens_[i].tokenIsComment = FALSE;
  }
}

//
// virtual destructor
//

ParScannedTokenQueue::~ParScannedTokenQueue()
{
}

//
// mutator
//
  
void
ParScannedTokenQueue::insert(const size_t tokenStrPos,
                             const size_t tokenStrLen,
                             const size_t tokenStrOffset,
                             NABoolean tokenIsComment)
{
  currentPos_ = (currentPos_ + 1) % getQueueSize();
  scannedTokenInfo & tokInfo = scannedTokens_[currentPos_];
  tokInfo.tokenStrPos = tokenStrPos + tokenStrOffset;
  tokInfo.tokenStrLen = tokenStrLen;
  tokInfo.tokenStrInputLen = tokenStrLen;
  tokInfo.tokenStrOffset = tokenStrOffset;
  tokInfo.tokenIsComment = tokenIsComment;
}

void
ParScannedTokenQueue::updateInputLen(const size_t tokenStrInputLen)
{
  scannedTokenInfo & tokInfo = scannedTokens_[currentPos_];
  tokInfo.tokenStrInputLen = tokenStrInputLen;  
}

// accessor 

const ParScannedTokenQueue::scannedTokenInfo &
ParScannedTokenQueue::getScannedTokenInfo(const Int32 tokenInfoIndex) const
{
  ComASSERT(tokenInfoIndex <= 0 AND
            getQueueSize() > - tokenInfoIndex);
  return scannedTokens_[(currentPos_ + getQueueSize()
                         + tokenInfoIndex) % getQueueSize()];
}


ParScannedTokenQueue::scannedTokenInfo *
ParScannedTokenQueue::getScannedTokenInfoPtr(const Int32 tokenInfoIndex)
{
  ComASSERT(tokenInfoIndex <= 0 AND
            getQueueSize() > - tokenInfoIndex);
  return &scannedTokens_[(currentPos_ + getQueueSize()
                          + tokenInfoIndex) % getQueueSize()];
}

//
// End of File
//
