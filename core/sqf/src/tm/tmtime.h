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

#ifndef TMTIME_H_
#define TMTIME_H_

#include <sys/types.h>
#include <sys/time.h>
#include "tmlibmsg.h"

extern timeval gv_startTime;

#ifdef XATM_LIB
// XATM Library definitions
#include "xatmglob.h"
//#include "xatmlib.h"

//#define TMSTARTTIME gv_xaTM.tmTimer()->startTime()
#define TMSTARTTIME gv_startTime
//#define TMTIMER gv_xaTM.tmTimer()
#define TMTIME_TRACE(level, mask, a) XATrace(mask, a)

#else
// TM definitions
#include "tmglob.h"
//#include "tminfo.h"

//#define TMSTARTTIME gv_tm_info.tmTimer()->startTime()
#define TMSTARTTIME gv_startTime
//#define TMTIMER gv_tm_info.tmTimer()
#define TMTIME_TRACE(level, mask, a) TMTrace(level, a)
#endif //XATM_LIB


// Ctimeval class definition
// This class encapsulates timeval providing conversion routines to simplify
// conversion to and from our internal time format of int64 msec since TM started.
class Ctimeval :public timeval
{
public:
   Ctimeval() { tv_sec = 0; tv_usec = 0;}
   Ctimeval(timeval pv_tv) :timeval(pv_tv) {}

   bool operator==(const Ctimeval &rhs)
   {
      if (tv_sec == rhs.tv_sec &&
          tv_usec == rhs.tv_usec)
          return true;
      return false;
   }
   Ctimeval& operator=(const Ctimeval &rhs)
   {
      tv_sec = rhs.tv_sec;
      tv_usec = rhs.tv_usec;
      return *this;
   }

   Ctimeval& operator=(const int64 &rhs_msec)
   {
      tv_sec = rhs_msec / 1000;
      tv_usec = (rhs_msec % 1000) * 1000;
      tv_sec += TMSTARTTIME.tv_sec;
      tv_usec += TMSTARTTIME.tv_usec;
      if (tv_usec >= 1000000)
      {
         tv_sec++;
         tv_usec -= 1000000;
      }
      return *this;
   }

   Ctimeval operator-(const Ctimeval rhs)
   {
      Ctimeval result;
      if (tv_sec >= rhs.tv_sec)
      {
         result.tv_sec = tv_sec - rhs.tv_sec;
         if (tv_usec >= rhs.tv_usec)
            result.tv_usec = tv_usec - rhs.tv_usec;
         else
            // Steal a second
            result.tv_usec = 1000000 + tv_usec - rhs.tv_usec;
      }
      else
      {
         result.tv_sec = 0;
         result.tv_usec = 0;
      }
      return result;
   }
   Ctimeval operator+(const Ctimeval rhs)
   {
      Ctimeval result;
      result.tv_sec = tv_sec + rhs.tv_sec;
      if ((tv_usec + rhs.tv_usec) >= 1000000)
         result.tv_usec = tv_usec + rhs.tv_usec - 1000000;
      else
         result.tv_usec = tv_usec + rhs.tv_usec;
      return result;
   }
   Ctimeval& operator+=(const Ctimeval rhs)
   {
      tv_sec += rhs.tv_sec;
      if ((tv_usec + rhs.tv_usec) >= 1000000)
         tv_usec += rhs.tv_usec - 1000000;
      else
         tv_usec += rhs.tv_usec;
      return *this;
   }
   // timeval + relative time in msec
   Ctimeval operator+(const int rhs)
   {
      Ctimeval result;
      result.tv_sec = tv_sec + (rhs / 1000);
      result.tv_usec = tv_usec + ((rhs %1000) * 1000);
      if (result.tv_usec >= 1000000)
      {
         result.tv_usec -= 1000000;
         result.tv_sec++;
      }
      return result;
   }
   // Should only be used for intervals to ensure we don't overflow!
   Ctimeval operator*(const Ctimeval rhs)
   {
      Ctimeval result;
      double lv_dbllhs = (double) tv_sec + ((double) tv_usec / 1000000);
      double lv_dblrhs = (double) rhs.tv_sec + ((double) rhs.tv_usec / 1000000);
      double lv_dblRslt = lv_dbllhs * lv_dblrhs;

      result.tv_usec = (long) ((lv_dblRslt - (long) lv_dblRslt) * 1000000);
      result.tv_sec = (long) (lv_dblRslt - ((double) (result.tv_usec /  1000000)));
      if (result.tv_usec < 0 || result.tv_sec < 0)
         abort();
      return result;
   }
   bool operator<(const Ctimeval rhs)
   {
      if ((tv_sec < rhs.tv_sec) ||
          (tv_sec == rhs.tv_sec &&
           tv_usec < rhs.tv_usec))
         return true;
      else
         return false;
   }
   static Ctimeval now()
   {  
      struct timezone lv_tz =  {0, NULL};
      Ctimeval lv_now;
      //char la_buf[DTM_STRING_BUF_SIZE];

      int lv_success = gettimeofday(&lv_now, &lv_tz);
      if (lv_success != 0)
      {
         // EMS DTM_GETTIME_FAIL
         //sprintf(la_buf, "Ctimerval::now Fatal error. gettimeofday "
         //   "failed with return value %d\n", lv_success);
         //tm_log_write(DTM_GETTIME_FAIL, SQ_LOG_CRIT, la_buf);
         abort();
      }
      return lv_now;
   }
}; //Ctimeval


// CdblTime class definition
// This class encapsulates a time in seconds.useconds and conversion routines to simplify
// conversion to and from for collecting statistics.
class CdblTime
{
private:
   double iv_sec;

public:
   CdblTime() :iv_sec(0) {}
   CdblTime(Ctimeval pv_tv) 
   {
      iv_sec = pv_tv.tv_sec + ((double) pv_tv.tv_usec / 1000000);
   }
   double get() {return iv_sec;}
   bool operator==(const Ctimeval &rhs)
   {
      double lv_rhs = rhs.tv_sec + ((double) rhs.tv_usec / 1000000);

      if (iv_sec == lv_rhs)
          return true;
      else
         return false;
   }
   bool operator==(CdblTime &rhs)
   {
      if (iv_sec == rhs.get())
          return true;
      else
         return false;
   }
   bool operator!=(double rhs)
   {
      if (iv_sec != rhs)
          return true;
      else
         return false;
   }
   CdblTime& operator=(double rhs)
   {
      iv_sec = rhs;
      return *this;
   }
   CdblTime& operator=(CdblTime &rhs)
   {
      iv_sec = rhs.get();
      return *this;
   }
   CdblTime& operator=(const Ctimeval &rhs)
   {
      iv_sec = rhs.tv_sec + ((double) rhs.tv_usec / 1000000);
      return *this;
   }
   CdblTime operator-(CdblTime rhs)
   {
      CdblTime result;
      if (iv_sec >= rhs.get())
         result = iv_sec - rhs.get();
      else
         result = 0;
      return result;
   }
   CdblTime operator+(CdblTime rhs)
   {
      CdblTime result;
      result = iv_sec + rhs.get();
      return result;
   }
   // Only makes sense to add a relative time (rhs)
   CdblTime& operator+=(CdblTime rhs)
   {
      iv_sec += rhs.get();
      return *this;
   }
   // CdblTime + relative time in msec
   CdblTime operator+(const int rhs)
   {
      double lv_rhs = (double) rhs / 1000;
      CdblTime result;
      result = iv_sec + lv_rhs;
      return result;
   }
   // Should only be used for intervals to ensure we don't overflow!
   CdblTime operator*(CdblTime rhs)
   {
      CdblTime result;
      result = iv_sec * rhs.get();
      return result;
   }
   bool operator<(CdblTime rhs)
   {
      if (iv_sec < rhs.get())
         return true;
      else
         return false;
   }
}; //CdblTime


// CTmTime class definition
// This is a unsigned 64 bit value which cotains the time in 
// milliseconds since the TM started .
class CTmTime
{
private:
   uint64 iv_time;

public:
   // timeval <--> int64 msec conversion routines
   // These routines are for relative time, not offsets from startTime
   static Ctimeval msectotimeval(int64 pv_msec)
   {
      Ctimeval lv_time;
      lv_time.tv_sec =  (pv_msec / 1000);
      lv_time.tv_usec = ((pv_msec % 1000) * 1000);
      return lv_time;
   }
   static uint64 timevaltomsec(Ctimeval pv_tv)
   {
      uint64 lv_msec = 0;
      lv_msec = (pv_tv.tv_sec * 1000) + (pv_tv.tv_usec /1000);
      return lv_msec;
   }

   // Constructors
   CTmTime(uint64 pv_time) :iv_time(pv_time) {}
   CTmTime(Ctimeval pv_time)
   {
      iv_time = timevaltomsec(pv_time);
   }
   //----------------------------------------------------------------------------
   // CTmTime::CTmTime Constructor from millisecond delay
   // This sets the time in milliseconds to TMSTARTTIME + pv_msec.
   //----------------------------------------------------------------------------
   CTmTime(int32 pv_msec)
   {
      Ctimeval lv_now = Ctimeval::now();
      Ctimeval lv_expireTime;

      TMTIME_TRACE(5, XATM_TraceTimer, ("CTmTime::CTmTime(msec) : ENTRY Delay %dmsec.\n", pv_msec));

      lv_expireTime = lv_now + pv_msec;
      *this = lv_expireTime - (Ctimeval) TMSTARTTIME;

      TMTIME_TRACE(5, XATM_TraceTimer, ("CTmTime::CTmTime(msec) : EXIT time " PFLLU ".\n", get()));
   } //CTmTime::CTmTime(msec)

   CTmTime()
   {
      TMTIME_TRACE(5, XATM_TraceTimer, ("CTmTime::CTmTime() : ENTRY\n"));
      Ctimeval lv_expireTime = Ctimeval::now();
      *this = lv_expireTime - (Ctimeval) TMSTARTTIME;
      TMTIME_TRACE(5, XATM_TraceTimer, ("CTmTime::CTmTime() : EXIT time " PFLLU "\n", get()));
   }
   ~CTmTime() {}

   // Member functions
   void set(uint64 pv_time) {iv_time = pv_time;}
   void set(Ctimeval pv_time)
   {
      iv_time = timevaltomsec(pv_time);
   }
   uint64 get() {return iv_time;}

   //----------------------------------------------------------------------------
   // CTmTime::get_tv
   // Purpose : Return a Ctimeval representing this CTmTime value.  
   // Note the Ctimeval is since EPOC, CTmTime is since TM startup.
   //----------------------------------------------------------------------------
   Ctimeval get_tv()
   {
      Ctimeval tv_expire = (Ctimeval) TMSTARTTIME + msectotimeval(iv_time);
      TMTIME_TRACE(5, XATM_TraceTimer, ("CTmTime::get_tv : expireTime %ld:%ld = TMstarttime %ld:%ld + timeInmsec " PFLLU ".\n", 
         (long) tv_expire.tv_sec, (long) tv_expire.tv_usec,
         (long) TMSTARTTIME.tv_sec, (long) TMSTARTTIME.tv_usec, iv_time));
      return tv_expire;
   }

   //----------------------------------------------------------------------------
   // CTmTime::left_tv
   // Purpose : Return the time left until this timer expires.
   // 0 indicates it has already expired.
   //----------------------------------------------------------------------------
   Ctimeval left_tv()
   {
      Ctimeval lv_now = Ctimeval::now();
      Ctimeval lv_expireTime = get_tv();
      Ctimeval lv_left;

      TMTIME_TRACE(5, XATM_TraceSpecial, ("CTmTime::left_tv : ENTRY Time expireTime %ld:%ld, now %ld:%ld.\n", 
         (long) lv_expireTime.tv_sec, (long) lv_expireTime.tv_usec,
         (long) lv_now.tv_sec, (long) lv_now.tv_usec));

      lv_left = lv_expireTime - lv_now;

      TMTIME_TRACE(4, XATM_TraceSpecial, ("CTmTime::left_tv : EXIT Time left %ld:%ld.\n", 
         (long) lv_left.tv_sec, (long) lv_left.tv_usec));
      return lv_left;
   } //CTmTime::left_tv

   CTmTime operator=(Ctimeval rhs)
   {
      if (rhs.tv_sec == 0)
      {
          if ((rhs.tv_usec < 0) || ((rhs.tv_usec > 0) && (rhs.tv_usec < 1000)))
              rhs.tv_usec = 1000;
      }
      iv_time = (rhs.tv_sec * 1000) + (rhs.tv_usec /1000);
      return *this;
   }
}; //CTmTime

#endif //TMTIME_H_
