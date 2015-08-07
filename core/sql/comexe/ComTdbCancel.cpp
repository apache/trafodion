// **********************************************************************
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
// File:         ComTdbCancel.cpp
// Description:  Class implementation for ComTdbCancel.
//
// Created:      Oct 15, 2009
// **********************************************************************

#include "ComTdbCancel.h"
#include "ComTdbCommon.h"

// Dummy constructor for "unpack" routines.
ComTdbCancel::ComTdbCancel()
: ComTdb(ComTdb::ex_CANCEL, eye_CANCEL)
  , qid_(NULL)
  , cancelPname_(NULL)
  , cancelNid_(-1)
  , cancelPid_(-1)
  , cancelPidBlockThreshold_(0)
  , comment_(NULL)
{
};

// Constructor

ComTdbCancel::ComTdbCancel( char *qid, char *pname, Int32 nid, Int32 pid,
                            Int32 minAge, Int16 action, Int16 forced, 
                            char *comment,
                            ex_cri_desc *given_cri_desc,
                            ex_cri_desc *returned_cri_desc,
                            queue_index down,
                            queue_index up)
  : ComTdb( ComTdb::ex_CANCEL,
            eye_CANCEL,
            1,        // estimatedRowCount
            given_cri_desc,
            returned_cri_desc,
            down,
            up,
            0,        // num_buffers.
            0),       // buffer_size.
     qid_(qid)
   , cancelPname_(pname)
   , cancelNid_(nid)
   , cancelPid_(pid)
   , cancelPidBlockThreshold_(minAge)
   , action_(action)
   , forced_(forced)
   , comment_(comment)
{};

void ComTdbCancel::display() const 
{};

Long ComTdbCancel::pack(void * space)
{
  qid_.pack(space);
  comment_.pack(space);
  cancelPname_.pack(space);
  return ComTdb::pack(space);
}

Lng32 ComTdbCancel::unpack(void * base, void * reallocator)
{
  if(qid_.unpack(base)) return -1;

  if(comment_.unpack(base)) return -1;
  if(cancelPname_.unpack(base)) return -1;
  return ComTdb::unpack(base, reallocator);
}

void ComTdbCancel::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);

  if(flag & 0x00000008)
    {
    char buf[200];

    str_sprintf(buf, "\nFor ComTdbCancel :");
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

    if (action_ == CancelByQid)
    {
      str_sprintf(buf, "action_ = cancel by queryId ");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      str_sprintf(buf, "qid_ = %s ", getQidText());
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
    else if (action_ == CancelByPname)
    {
      str_sprintf(buf, "action_ = cancel by process name "
                       "with minimum blocking interval of %d seconds ", 
                  cancelPidBlockThreshold_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      str_sprintf(buf, "cancelPname_ = %s ", getCancelPname());
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
    else if (action_ == CancelByNidPid)
    {
      str_sprintf(buf, "action_ = cancel by nid,pid "
                       "with minimum blocking interval of %d seconds ",
                  cancelPidBlockThreshold_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      str_sprintf(buf, "cancelNid_,cancelPid_ = %d,%d ", 
                  getCancelNid(), getCancelPid());
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
    else 
    {
      if (action_ == Activate)
      {
        str_sprintf(buf, "action_ = activate ");
        space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      }
      else    // must be Suspend
      {
        if (forced_ == Force)
          str_sprintf(buf, "action_ = suspend (forced) ");
        else
          str_sprintf(buf, "action_ = suspend (safe) ");
        space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      }
      str_sprintf(buf, "qid_ = %s ", getQidText());
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }

    str_sprintf(buf, "comment_ = %s ", getCommentText());
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }

  if(flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

