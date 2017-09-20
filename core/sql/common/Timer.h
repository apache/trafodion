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
#ifndef TIMER_H
#define TIMER_H

#include "ComCextdecs.h"

class Timer
{
private:

  enum TimerConstants {
    ONE_THOUSAND     = 1000,
    ONE_MILLION      = 1000000
  };

  Int64     startTime_;
  Int64     endTime_;
  Int64     accumTime_;
  NABoolean running_;

public:

  // A timer needs to be explicitly started using 'start' or 'restart'
  Timer()
    : running_(false),
      startTime_(0),
      accumTime_(0)
  { }

  NABoolean start()
  {
    // Return immediately if the timer is already running
    if (! running_)
    {
      // Set timer status to running and set the start time
      running_ = TRUE;
      accumTime_ = 0;
      startTime_ = NA_JulianTimestamp();
    }

    return running_;
  }

  //
  // If timer is running, then stop the timer, accumulate any elapsed time.
  // Returns accumulated time.
  Int64 stop()
  {
    if (running_)
    {
      endTime_ = NA_JulianTimestamp();
      running_ = FALSE;
      accumTime_ += endTime_ - startTime_;
    }

    return accumTime_;
  }

  //
  // If timer is not running, then set to running and capture a new start time.
  // Returns current accumulated time.
  Int64 restart()
  {
    if (! running_)
    {
      running_ = TRUE;
      endTime_ = 0;
      startTime_ = NA_JulianTimestamp();
      // accumTime_   this is incremented when stop is called
    }
    return accumTime_;
  }

  //
  // If timer is running, then stop it and accumulate elapsed time.
  // If timer is not running, then start it.
  // Returns current accumulated time.
  Int64 startStop()
  {
    return( running_ ? stop() : restart() );
  }

  //
  // If timer is running, then return the accumulated time so far plus
  // the current time minus the current start time.
  // If timer is not running, then just return the accumulated time.
  Int64 elapsedTime()
  {
    if (running_)
      return ( (NA_JulianTimestamp() - startTime_) + accumTime_ );
    else
    {
      return ( accumTime_ );
    }
  }

};

#endif // TIMER_H
