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
#ifndef EXSM_SHORT_MESSAGE_H
#define EXSM_SHORT_MESSAGE_H

#include "ExSMCommon.h"

class ExSMShortMessage;

class ExSMShortMessage
{
public:
  ExSMShortMessage();

  virtual ~ExSMShortMessage();

  const sm_target_t &getTarget() const { return target_; }
  void setTarget(const sm_target_t &t) { target_ = t; }

  size_t getNumValues() const { return numValues_; }
  void setNumValues(size_t n) { numValues_ = n; }

  int32_t getValue(size_t i) const
  { return (i < numValues_ ? values_[i] : 0); }

  void setValue(size_t i, int32_t val)
  { if (i < numValues_) values_[i] = val; }

  int32_t send() const;
  void receive(const sm_chunk_t &chunk);

  void writeToTrace(uint32_t trace_level,
                    const char *prefix1 = "",
                    const char *prefix2 = "") const;

  enum MsgType
    {
      UNKNOWN = 0,
      SHUTDOWN,
      SIZE,
      ACK,
      FIXUP_REPLY
    };

protected:

  static const size_t MAX_VALUES = 8;

  sm_target_t target_;
  size_t numValues_;
  int32_t values_[MAX_VALUES];

}; // class ExSMShortMessage

#endif // EXSM_SHORT_MESSAGE_H
