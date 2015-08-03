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
#ifndef PARSCANNEDTOKENQUEUE_H
#define PARSCANNEDTOKENQUEUE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ParScannedTokenQueue.h
 * Description:  definitions of class ParScannedTokenQueue.
 *
 *               The ParScannedTokenQueue object represents circular
 *               queue containing information about recently scanned
 *               tokens.
 *
 *               
 * Created:      5/31/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------
// Change history:
// 
// -----------------------------------------------------------------------

#include "ComASSERT.h"
#include "ComOperators.h"
#include "NAString.h"
#include "NABasicObject.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ParScannedTokenQueue;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// Definition of class ParScannedTokenQueue
// -----------------------------------------------------------------------
class ParScannedTokenQueue : public NABasicObject
{
public:
  
  enum { QUEUE_SIZE = 9 };

  struct scannedTokenInfo
  {
    size_t tokenStrPos;
    size_t tokenStrLen;		// Length in UCS2 characters
    size_t tokenStrInputLen;    // Length in original input stream bytes.
    size_t tokenStrOffset;	// track extra offset for Wide characters
    NABoolean tokenIsComment;   // TRUE if this token is a comment
  };

  //
  // constructor
  //
  
  ParScannedTokenQueue();

  //
  // virtual destructor
  //
  
  virtual ~ParScannedTokenQueue();

  //
  // accessors
  //
  
  inline Int32 getQueueSize() const;

  const scannedTokenInfo & getScannedTokenInfo
                (const Int32 tokenInfoIndex = 0) const;
  inline const scannedTokenInfo * getScannedTokenInfoPtr
                (const Int32 tokenInfoIndex = 0) const;
        scannedTokenInfo * getScannedTokenInfoPtr
                (const Int32 tokenInfoIndex = 0);

        //  0 : index of token most recently scanned
        // -1 : index of token scanned before the most-recently-scanned token
        // -2 : index of the token scanned before the -1 indexed token
        // 
        // index should never be a positive value

  inline NABoolean isQueueIndexOutOfRange(Int32 i) const
  {
    // Valid indexes are: -(getQueueSize()-1), ... , -2, -1, 0
    return ( i > 0  OR  i <= (-getQueueSize()) );
  }

  inline NABoolean isQueueIndexWithinRange(Int32 i) const
  {
    // Valid indexes are: -(getQueueSize()-1), ... , -2, -1, 0
    return ( (-i) < getQueueSize() );
  }

  //
  // mutators
  //

  void insert(const size_t tokenStrPos,
              const size_t tokenStrLen,
              const size_t tokenStrOffset,
              NABoolean tokenIsComment);

  void updateInputLen(const size_t tokenStrInputLen);
  
private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  //
  // copy constructor
  //
  
  ParScannedTokenQueue(const ParScannedTokenQueue &queue);  // DO NOT USE

  //
  // assignment operator
  //

  ParScannedTokenQueue & operator=
                (const ParScannedTokenQueue &queue);  // DO NOT USE

  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------
  
  scannedTokenInfo scannedTokens_[QUEUE_SIZE];
  Int32 currentPos_;
  
}; // class ParScannedTokenQueue

// -----------------------------------------------------------------------
// definitions of inline methods for class ParScannedTokenQueue
// -----------------------------------------------------------------------

//
// accessors
//

inline Int32
ParScannedTokenQueue::getQueueSize() const
{
  return QUEUE_SIZE;
}

inline const ParScannedTokenQueue::scannedTokenInfo *
ParScannedTokenQueue::getScannedTokenInfoPtr(const Int32 tokenInfoIndex) const
{
  ComASSERT(tokenInfoIndex <= 0 AND
            getQueueSize() > - tokenInfoIndex);
  return &scannedTokens_[(currentPos_ + getQueueSize()
                          + tokenInfoIndex) % getQueueSize()];
}

#endif // PARSCANNEDTOKENQUEUE_H
