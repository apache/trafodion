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

#include <string.h>
#include <sys/time.h>
#include "ExSMEvent.h"
#include "ExSMCommon.h"

// Max number of events in the in-memory trace
const int EXSM_EVENT_SIZE = 1024;

// Character labels for each event type
const char *ExSMEvent::Init = "INIT";
const char *ExSMEvent::Finalize = "FINA";
const char *ExSMEvent::Register = "REGI";
const char *ExSMEvent::Cancel = "CANC";
const char *ExSMEvent::Send = "SEND";
const char *ExSMEvent::SendRetry = "SRET";
const char *ExSMEvent::Post = "POST";
const char *ExSMEvent::Receive = "RECV";
const char *ExSMEvent::Chunk = "CHNK";
const char *ExSMEvent::IDNotActive = "IDNA";
const char *ExSMEvent::ReceiveDone = "RDON";
const char *ExSMEvent::SMError = "SMER";
const char *ExSMEvent::ControlConnectionError = "CCER";
const char *ExSMEvent::Exit = "EXIT";

// struct ExSMEventTrace is a container for the in-memory trace
// associated with a given thread. Each thread (main thread and reader
// thread) will have its own instance of this structure.
struct ExSMEventTrace
{
  int32_t size_;
  int32_t idx_;
  int32_t count_;
  bool use_gettime_;
  ExSMEvent *events_;

  ExSMEventTrace()
    : size_(0),
      idx_(0),
      count_(0),
      use_gettime_(false),
      events_(NULL)
  {
  }

  void init()
  {
    if (events_ != NULL)
      return;
    
    // Should timestamps be generated with gettimeofday() or
    // clock_gettime()
    const char *envvar = getenv("EXSM_CLOCK_GETTIME");
    if (envvar && *envvar)
      use_gettime_ = true;

    // Size of the array can be set with an environment variable
    size_ = EXSM_EVENT_SIZE;
    envvar = getenv("EXSM_EVENT_SIZE");
    if (envvar && *envvar)
    {
      int i = atoi(envvar);
      if (i > 0)
        size_ = i;
    }
    
    // Allocate the array
    int32_t numBytes = size_ * sizeof(ExSMEvent);
    events_ = (ExSMEvent *) new char[numBytes];
    memset(events_, 0, numBytes);
  }

  // Print all events to stdout
  void print()
  {
    // Find the starting position
    int32_t start = 0;
    if (count_ >= size_)
      start = idx_;

    // Print the number of events
    printf("\n");
    printf("Number of events: %d\n", (int) count_);

    // Print contents of each event. The total number of events in the
    // collection is count_.
    for (int32_t i = 0; i < count_; i++)
    {
      int32_t index = (start + i) % size_;
      
      const ExSMEvent &e = events_[index];
      const sm_target_t &t = e.target_;
      const int64_t *opt = &(e.optional_[0]);

      printf("[%d] %d.%09ld %s ", (int) index,
             (int) e.ts_.tv_sec, (long int) e.ts_.tv_nsec, e.fn_);
      printf("%d:%d:%ld :%d:0x%c ", (int) t.node, (int) t.pid, t.id,
             (int) ExSMTag_GetTagWithoutQualifier(t.tag),
             (char) ExSMTag_GetQualifierDisplay(t.tag));
      printf("%ld %ld %ld %ld %ld\n",
             opt[0], opt[1], opt[2], opt[3], opt[4]);
    }

    fflush(stdout);
  }
};

// Thread-specific pointer to an event array
__thread ExSMEventTrace *EXSM_EVENTS = NULL;

// Global pointers to trace for main and reader threads. These are
// only used because sometimes we have had a core file in gdb and gdb
// wasn't able to display thread-local storage.
//
// Note: this naming scheme will need to change if the executor can be
// hosted in different threads during the lifetime of the process. For
// example: thread 1 executes a set of queries, then thread 1 goes
// away, then thread 2 executes another set of queries.
ExSMEventTrace EXSM_EVENTS_MAIN = ExSMEventTrace();
ExSMEventTrace EXSM_EVENTS_READER = ExSMEventTrace();

void ExSMEvent::initMainThread()
{
  if (EXSM_EVENTS == NULL)
  {
    EXSM_EVENTS_MAIN.init();
    EXSM_EVENTS = &EXSM_EVENTS_MAIN;
  }
}

void ExSMEvent::initReaderThread()
{
  if (EXSM_EVENTS == NULL)
  {
    EXSM_EVENTS_READER.init();
    EXSM_EVENTS = &EXSM_EVENTS_READER;
  }
}

void ExSMEvent::printMainThread()
{
  EXSM_EVENTS_MAIN.print();
}

void ExSMEvent::printReaderThread()
{
  EXSM_EVENTS_READER.print();
}

ExSMEvent *ExSMEvent::add(const char *fn, const sm_target_t *target,
                          int64_t i1, int64_t i2, int64_t i3, int64_t i4, int64_t i5)
{
  if (EXSM_EVENTS == NULL)
    return NULL;

  ExSMEventTrace &trace = *EXSM_EVENTS;
  if (trace.events_ == NULL || trace.size_ == 0)
    return NULL;

  // Identify the next event slot
  ExSMEvent &event = trace.events_[trace.idx_];
  memset(&event, 0, sizeof(event));

  // Store a timestamp in the event
  if (trace.use_gettime_)
  {
    clock_gettime(CLOCK_MONOTONIC, &event.ts_);
  }
  else
  {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    event.ts_.tv_sec = tv.tv_sec;
    event.ts_.tv_nsec = tv.tv_usec * 1000;
  }

  // Copy all arguments into the event
  if (fn)
    strncpy(event.fn_, fn, 4);

  if (target)
    event.target_ = *target;

  event.optional_[0] = i1;
  event.optional_[1] = i2;
  event.optional_[2] = i3;
  event.optional_[3] = i4;
  event.optional_[4] = i5;

  // Move idx_ to the next available slot
  trace.idx_ = ((trace.idx_ + 1) % trace.size_);

  // Keep a count of how many events are in the trace
  if (trace.count_ < trace.size_)
    trace.count_++;

  return &event;
}
