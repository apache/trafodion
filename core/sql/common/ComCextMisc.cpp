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

#include "seaquest/sqtypes.h"
#include "Platform.h"
#include "ComCextMisc.h"

#include "seabed/ms.h"
#include "seabed/fs.h"
#include <string.h>
#include "ComRtUtils.h"

typedef SB_Phandle_Type *PNSK_PORT_HANDLE;

static short const          OMITSHORT         = -291;
#ifdef __linux__
__int64 const        OMIT__INT64       = 0xfedd000000000001LL;
#else
static __int64 const        OMIT__INT64       = -81909218222800895;
#endif

typedef _int64         NSKTIMESTAMP;
typedef NSKTIMESTAMP * PNSKTIMESTAMP;

#define MS_ADJ 11644473600LL // diff between mic epoch 1/1/1601 & 1/1/1970

const NSKTIMESTAMP  JTS_MIN             = 148731163200000000LL; //    1/ 1/ 1  0:00:00.000000
const NSKTIMESTAMP  JTS_1975            = 211024526400000000LL; // 1975/ 1/ 1  0:00:00.000000
//const NSKTIMESTAMP  JTS_MAX             = 274958971199999999LL; // 4000/12/31 23:59:59.999999
const NSKTIMESTAMP  JTS_MAX             = 464300683199999999LL;   // 10000/12/31 23:59:59.999999
const unsigned long JULIAN_DATE_OFFSET  = 1721119L;             // the base JDN
const unsigned long JDN_MIN             = 1721426L;             //    1/ 1/ 1  0:00:00.000000
//const unsigned long JDN_MAX             = 3182395L;             // 4000/12/31 23:59:59.999999
const unsigned long JDN_MAX             = 5373850L;             // 10000/12/31 23:59:59.999999
const NSKTIMESTAMP  LARGE_NEXTCHANGEGMT = 9223372036854775807LL;// 63 "1" bits
const NSKTIMESTAMP  MICROSOFT_EPOCH     = 199222286400000000LL; // 1601/ 1/ 1  0:00:00.000000
const NSKTIMESTAMP  ONE_DAY             = 86400000000LL;        // microseconds in a day
const NSKTIMESTAMP  ONE_MINUTE          = 60000000;             // microseconds in a minute
const NSKTIMESTAMP  ONE_YEAR            = 31536000000000LL;     // microseconds in a year
const NSKTIMESTAMP  TANDEM_EPOCH        = 211024440000000000LL; // 1974/12/31  0:00:00.000000

static BOOL sv_envJulianTimestamp = FALSE;
static BOOL sv_useLinuxJulianTimestamp = FALSE;

BOOL GetMicroseconds( _int64 * t, short type = 0 );

// *****************************************************************************
// *                                                                           *
// * Function: GetMicroseconds                                                 *
// *    This function returns the time in microseconds as a 64-bit integer.    *
// *    The time returned depends on the "type" parameter.                     *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <t>                       _int64 *                        Out            *
// *    is a pointer to the location to store the requested time.              *
// *                                                                           *
// *  <type>                    short                           In             *
// *    is the type of time requested.                                         *
// *    type = 0 Current UTC time.                                             *
// *    type = 1 System start time rounded to nearest second.                  *
// *    type = 2 Time since system start.                                      *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns: TRUE if successful, otherwise FALSE.                            *
// *                                                                           *
// *****************************************************************************

inline int CHECK_LIMIT_JTS( _int64 juliantimestamp )
{
	return ((juliantimestamp < JTS_MIN) || (juliantimestamp > JTS_MAX));
}

BOOL GetMicroseconds( _int64 * t, short type )
{

   static THREAD_P BOOL
      initialized;

   static THREAD_P double
      counterResolutionToTimeResolution;

   static THREAD_P double
      microsecondsPerCounterTick;

   static THREAD_P double
      reasonableCounterDrift;

   static THREAD_P unsigned _int64
      baseCounter;

   static THREAD_P unsigned _int64
      baseMicroseconds;

   static THREAD_P unsigned _int64
      coldLoadMicroseconds;

   static THREAD_P unsigned _int64
      lastCounter;

   static THREAD_P unsigned _int64
      lastTime;

   unsigned _int64
      currentTime;

   unsigned _int64
      currentCounter;

   if (!initialized)
      {
      BOOL
         adjusting;

      DWORD
         timeAdjustment;

      DWORD
         timeAdjustmentPeriod;

      struct LargeInt
         counterFrequency;

      //
      // If the counterFrequency equals zero, that means that we ended up in this code
      // prior to calculate_performance_frequency being called in the initialization flow.
      // We never want this to happen, so bug check if it does.
      //

      counterFrequency.QuadPart = 1;
      if (!GetSystemTimeAdjustment( &timeAdjustment, &timeAdjustmentPeriod, &adjusting ))
         return( FALSE );

      counterResolutionToTimeResolution = ((double) counterFrequency.QuadPart) / 10000000.0;
      microsecondsPerCounterTick        = 10000;


      //
      // Compute how far it's reasonable to expect the counter to drift from
      // the time-of-day clock.
      //

      reasonableCounterDrift = ((double) (adjusting ? timeAdjustmentPeriod : 0))
                               * counterResolutionToTimeResolution;

      GetSystemTimeAsFileTime( (FILETIME *) &lastTime );
      QueryPerfCounter( (struct LargeInt *) &baseCounter );

      lastCounter          = baseCounter;

      baseMicroseconds     = lastTime / 10;

      coldLoadMicroseconds =   baseMicroseconds
                             - ((_int64) (((double) ((_int64) baseCounter)) / microsecondsPerCounterTick));

      coldLoadMicroseconds = ((coldLoadMicroseconds + 500000) / 1000000) * 1000000;   // Round to clock precision.

      initialized          = TRUE;
      }

   QueryPerfCounter( (struct LargeInt *) &currentCounter );

   if (currentCounter < lastCounter)
      {
      currentCounter = lastCounter;
      }

   if (type == 2)
     {
      *t = (_int64) (((double) ((_int64) currentCounter)) / microsecondsPerCounterTick);
      *t = (_int64) ( currentCounter / microsecondsPerCounterTick );
     }
   else
      {
      GetSystemTimeAsFileTime( (FILETIME *) &currentTime );

      if (currentTime <= lastTime)
         {
         if ((currentTime < lastTime)      // Time must have been set back.
                ||
             (((double) ((_int64) (currentCounter - lastCounter)))
                 >
              (counterResolutionToTimeResolution + reasonableCounterDrift)
             )
            )
            {
            baseCounter          = currentCounter;

            baseMicroseconds     = currentTime / 10;

            coldLoadMicroseconds =
                 baseMicroseconds
               - ((_int64) (((double) ((_int64)currentCounter)) / microsecondsPerCounterTick));

            coldLoadMicroseconds = ((coldLoadMicroseconds + 500000) / 1000000) * 1000000;   // Round to clock precision.
            }
         }
      else
         {
         if (((double) ((_int64) (currentTime - lastTime)))
                 >
             ((((double) ((_int64) (currentCounter - lastCounter))) + reasonableCounterDrift
              ) / counterResolutionToTimeResolution
             )
            )
            {
            baseCounter          = currentCounter;

            baseMicroseconds     = currentTime / 10;

            coldLoadMicroseconds =
                 baseMicroseconds
               - ((_int64) (((double) ((_int64)currentCounter)) / microsecondsPerCounterTick));

            coldLoadMicroseconds = ((coldLoadMicroseconds + 500000) / 1000000) * 1000000;   // Round to clock precision.
            }
         }

      if (type == 0)
         {
	   *t =   currentTime / 10;
         }
      else
         {
         if (type == 1)
            *t = coldLoadMicroseconds;
         }

      lastTime = currentTime;
      }

   lastCounter = currentCounter;

   return( ((type >= 0) && (type <= 2)) );
}

// CONVERTOLDTIMESTAMP takes a three-word timestamp and
// converts it into a Julian timestamp.
_int64 CONVERTOLDTIMESTAMP (short * threewordts)
{
   union
      {
      _int64 juliants;

      struct
         {
         short lowestshort;
         short lowshort;
         short highshort;
         short highestshort;
         };
      } timeunion;

   timeunion.lowestshort  = threewordts [0];
   timeunion.lowshort     = threewordts [1];
   timeunion.highshort    = threewordts [2];
   timeunion.highestshort = 0;

   // Convert centisecs to microsecs and adjust the base
   return ( timeunion.juliants * 10000 + TANDEM_EPOCH );
}

extern "C"
DLLEXPORT
long INTERPRETTIMESTAMP (_int64 juliantimestamp, short * date_n_time)

//  INTERPRETTIMESTAMP converts a Julian timestamp to an array of integers
//  representing the same Gregorian date and time of day.  It also returns
//  (as its value) the Julian Day Number corresponding to that date.

   // _int64 juliantimestamp;            // input,  required, any valid Julian timestamp

   // short date_n_time;                 // output, required, array [8],
                                         //         date_n_time [0] = year
                                         //         date_n_time [1] = month
                                         //         date_n_time [2] = day
                                         //         date_n_time [3] = hours
                                         //         date_n_time [4] = minutes
                                         //         date_n_time [5] = seconds
                                         //         date_n_time [6] = milliseconds
                                         //         date_n_time [7] = microseconds
   // long result                        // output, the Julian Day Number (jdn)


{

   long dayno;                           // result: Julian Day Number

#  define year   date_n_time[0]
#  define month  date_n_time[1]
#  define day    date_n_time[2]
#  define hour   date_n_time[3]
#  define minute date_n_time[4]
#  define second date_n_time[5]
#  define millis date_n_time[6]
#  define micros date_n_time[7]

//  The JULIANTIMESTAMP that is to be interpreted by this procedure is
//  basically a julian day number encoded with microsecond resolution.
//  Interpreting this value happens in three steps:
//
//  1) Break JULIANTIMESTAMP into "julian day number" and "time of day".
//
//     This can be achieved by dividing 86,400,000,000 into JULIANTIMESTAMP;
//     since this number represents one day in microsecond resolution, the
//     quotient from the division will be the number of full days, i.e, the
//     "julian day number", and the remainder will be the "time of day" in
//     microsecond resolution.
//
//  2) Break "time of day" into hour/minute/second/millisecond/microsecond
//
//     Repeated "modulo-division" of the remainder calculated in step 1)
//     by 1000, 1000, 60, 60 will yield microseconds, milliseconds, seconds,
//     and minutes, respectively; what's left after this will be the "hours".
//
//  3) Break "julian day number" into Gregorian year/month/day
//
//     The algorithm used to break up the "julian day number" is implemented
//     after a model printed in a 1963 ACM Communications publication:
//
//        j = j - 1721119;                // j : julian day number (input)
//        y = (j * 4 - 1) / 146097;
//        j = (j * 4 - 1) - 146097 * y;
//        d = j / 4;
//        j = (d * 4 + 3) / 1461;
//        d = (d * 4 + 3) - 1461 * j;
//        d = (d + 4) / 4;
//        m = (d * 5 - 3) / 153;          // m : month (output)
//        d = (d * 5 - 3) - 153 * m;
//        d = (d + 5) / 5;                // d : day   (output)
//        y = y * 100 + j;                // y : year  (output)
//        if (m > 9)
//           {
//           m = m - 9;
//           y = y + 1;
//           }
//        else
//           m = m + 3;
//
//
//  a) Given the upper limit of JTS_MAX (and hence JDN_MAX), dividing
//     3,600,000,000 into JULIANTIMESTAMP yields a quotient of less than
//     32 bits that represents the "julian hour", and a 32-bit remainder
//     that represents the minute/second/millisecond/microsecond part of
//     of "time of day".  (Note that this is an unsigned 32-bit value!!)
//
//  b) Given the 32-bit quotient from a), 2-word arithmetic can now be
//     used to add 12 hours (to get to the start of the astronomical
//     day) and then divide 24 into this value yielding the "julian day
//     number" as a quotient and leaving the "hour" part of the "time of
//     day" as the remainder.
//
//  c) The 32-bit unsigned remainder from a) is broken up by dividing
//     1,000,000 into it in two steps: a logical shift to the right by
//     6 bit positions (which is equivalent to a division by 64), and
//     then a division by 15,625; (64 * 15,625 = 1,000,000).
//     The quotient will contain the minute/second part of the "time of
//     day", the remainder will contain the millisecond/microsecond part.
//
//  d) The two results from c) are broken up further by simple modulo-
//     divisions into minute and second, and millisecond and microsecond,
//     respectively.
   struct
      {
      union
         {
         _int64 quadpart;

         struct
            {
            unsigned long   lowpart;
            unsigned long   highpart;
            };
         };
      } _jdnhh;

   struct
      {
      union
         {
         _int64 quadpart;

         struct
            {
            unsigned long   lowpart;
            unsigned long   highpart;
            };
         };
      } _mmssmmmuuu;

#  define jdnhh_64      _jdnhh.quadpart
#  define jdnhh         _jdnhh.lowpart
#  define mmssmmmuuu_64 _mmssmmmuuu.quadpart
#  define mmssmmmuuu    _mmssmmmuuu.lowpart
#  define mmmuuu        _mmssmmmuuu.highpart
#  define mmss          _jdnhh.highpart

   const unsigned long DAYS_PER_400_YEARS = 146097L,
                       DAYS_PER_4_YEARS   =   1461L;

   unsigned long century;
   unsigned short yoc;   // YEAR OF CENTURY

   unsigned long  temp1; // to hold   ((JDN - JDO) * 4 - 1)
   unsigned long  temp2; // to hold ((((JDN - JDO) * 4 - 1) '\' 146097) / 4) * 4 + 3
   unsigned short temp3; // to hold  (((TEMP2 '\' 1461) + 4) / 4) * 5 - 3
   unsigned short temp4; // to hold     TEMP3 / 153



   if (CHECK_LIMIT_JTS ( juliantimestamp ))
      {
      year = -1;

      return ((unsigned long) -1L);    // cast eliminates a compiler warning.
      }

   // For explanation of the next two lines see comment a) from above
   jdnhh_64 = juliantimestamp / 3600000000LL;   // 60 * 60 * 1000 * 1000 usecs
   mmssmmmuuu_64 = (((_int64) jdnhh_64) * (-((_int64) 3600000000LL))) + juliantimestamp;

   // For explanation of the next three lines see comment b) from above
   jdnhh = jdnhh + 12L;
   dayno = jdnhh / 24L;
   hour  = (unsigned short) (dayno * (-24L) + jdnhh);

   // For explanation of the next four lines see comments c) and d) from above
   mmss = (mmssmmmuuu / 1000000);
   minute = (unsigned short) (mmss / 60);
   second = (unsigned short) (mmss - (minute * 60));
   mmmuuu = (unsigned long) ((((_int64) mmss) * (-1000000)) + mmssmmmuuu_64);

   millis = (unsigned short) (mmmuuu / 1000);
   micros = (unsigned short) (mmmuuu - (millis * 1000));

   temp1  = (dayno - JULIAN_DATE_OFFSET) * 4 - 1L;
   temp2  = ((temp1 - ((century = temp1 / DAYS_PER_400_YEARS)
                     ) * DAYS_PER_400_YEARS
            ) / 4
           ) * 4 + 3L;

   yoc   = (unsigned short) (temp2 / DAYS_PER_4_YEARS);

   temp3 = ((((unsigned short) (temp2 - (yoc * DAYS_PER_4_YEARS))
             ) + 4
            ) / 4
           ) * 5 - 3;

   temp4 = temp3 / 153;

   day = ((temp3 - (temp4 * 153)) + 5) / 5;

   if (temp4 > 9)
      {
      month = temp4 - 9;
      yoc   = yoc + 1;
      }
   else
      month = temp4 + 3;

   year = ((short) century) * 100 + yoc;

   return (dayno);

#  undef year
#  undef month
#  undef day
#  undef hour
#  undef minute
#  undef second
#  undef millis
#  undef micros

#  undef jdnhh_64
#  undef jdnhh
#  undef mmssmmmuuu_64
#  undef mmssmmmuuu
#  undef mmmuuu
#  undef mmss

}

extern "C"
DLLEXPORT
void CONTIME( short * a, short t, short t1, short t2 )
   // short * a               // time is returned here, in the form:
                              //   0:  year      (1975-2074+)
                              //   1:  month     (1-12)
                              //   2:  day       (1-31)
                              //   3:  hour      (0-23)
                              //   4:  minute    (0-59)
                              //   5:  seconds   (0-59)
                              //   6:  .01 secs  (0-99)
                              //
                              // time stamp:
  // short t                  //   low order 16 bits
  // short t1                 //   middle 16 bits
  // short t2                 //   high 16 bits
   {
   _int64 jt;
   short dnt[8];
   short copyt[3];

   copyt[ 0 ] = t;
   copyt[ 1 ] = t1;
   copyt[ 2 ] = t2;

   jt = CONVERTOLDTIMESTAMP( copyt );
   INTERPRETTIMESTAMP( jt, dnt );
   a[0] = dnt[0];
   a[1] = dnt[1];
   a[2] = dnt[2];
   a[3] = dnt[3];
   a[4] = dnt[4];
   a[5] = dnt[5];
   a[6] = dnt[6] / 10;               // Convert millis to centis
}


_int64 LCTBias2 (_int64                  GMTtime,
                 short *                 CC
                )
{
   short     ldt[8];
   long      ljdn;
   _int64    loffset64;
   struct tm ltm;
   time_t    ltime;
   time_t    ltimegm;
   time_t    ltimeloc;
   struct tm ltmgm;
   struct tm ltmloc;

   loffset64 = 0;
   *CC = 0;
   // get dt
   ljdn = INTERPRETTIMESTAMP (GMTtime, ldt);
   if (ljdn == -1) {
      *CC = 1;
      return loffset64;
   }
   if (ldt[0] < 1900) {
      *CC = 1;
      return loffset64;
   }
   // ldt[0] - year  - e.g. 1984
   // ldt[1] - month - 1-12
   // ldt[2] - day   - 1-31
   ltm.tm_year = ldt[0] - 1900;
   ltm.tm_mon = ldt[1] - 1;
   ltm.tm_mday = ldt[2];
   ltm.tm_hour = ldt[3];
   ltm.tm_min = ldt[4];
   ltm.tm_sec = ldt[5];

   // create time_t (ltime) out of ldt
   // create tm's (ltmgm/ltmloc) out of time_t
   // create time_t's (ltimegm/ltimeloc) out of tm's
   // compute local bias
   ltm.tm_isdst = -1;
   ltime = mktime(&ltm);
   gmtime_r(&ltime, &ltmgm);
   localtime_r(&ltime, &ltmloc);
   ltmgm.tm_isdst = -1;
   ltimegm = mktime(&ltmgm);
   ltmloc.tm_isdst = -1;
   ltimeloc = mktime(&ltmloc);
   loffset64 = ltimegm - ltimeloc;
   loffset64 = loffset64 * 1000000; // convert to usec
   return (loffset64);
}


extern "C"
DLLEXPORT
_int64 CONVERTTIMESTAMP (_int64  timestamp,
                                   short   direction,
                                   short   node,
                                   short * error
                                  )
{
   DWORD                 timeZoneID = 0;

   short                 err        = 0;

   TIME_ZONE_INFO        tzi;

   if (direction == OMITSHORT)
      direction = 0;

   if (node == OMITSHORT)
      node = -1;

   if (error == 0)
      error = &err;

   if ((direction < 0) || (direction > 3))
      {
      *error = -3;   // direction has invalid value
      return (0);
      }

   // timestamp parameter must be passed and check if it's within range

   if ((timestamp == OMIT__INT64)
          ||
       (CHECK_LIMIT_JTS ( timestamp ) )
      )
      {
      *error = -4;  // timestamp has invalid value
      return (0);
      }

   if (node == -1)
      {   // Local
      timeZoneID = GetTimeZoneInformation (&tzi);

      if (timeZoneID == TIME_ZONE_ID_INVALID)
         *error = 2;
      else
         {
         *error = 0;

         switch (direction)
            {
            case 0:   // GMT->LCT
               {
               _int64 LCT_bias;

               short  cc_err = 0;

               LCT_bias = LCTBias2 (timestamp, &cc_err);

               if (cc_err)
                  *error = ((cc_err < 0) ? 2 : 1);

               timestamp = timestamp - LCT_bias;

               break;
               }

            case 1:   // GMT->LST
               {
               timestamp = timestamp - (tzi.Bias * ONE_MINUTE);

               break;
               }

            case 2:   // LCT->GMT
               {
               _int64 LCT_bias;

               short  cc_err = 0;

               LCT_bias = LCTBias2 (timestamp, &cc_err);

               if (cc_err)
                  *error = ((cc_err < 0) ? 2 : 1);

               timestamp = timestamp + LCT_bias;

               break;
               }

            case 3:   // LST->GMT
               {
               timestamp = timestamp + (tzi.Bias * ONE_MINUTE);

               break;
               }
            }
         }
      }    // Local
   else
      {    // Remote
      *error = -5;
      return (0);
      }    // Remote

   return (timestamp);
}


void TIMESTAMP( short * a )
{
   short error = 0;
   union
      {
      _int64 ftime;

      struct
         {
         short lowestshort;
         short lowshort;
         short highshort;
         short highestshort;
         };
      } timeunion;

   if (!GetMicroseconds (&timeunion.ftime, 0))
      timeunion.ftime = -1;
   else
      {
      //
      // Convert to LCT.
      //
      timeunion.ftime =
         CONVERTTIMESTAMP ((timeunion.ftime + MICROSOFT_EPOCH), 0, -1, &error);

      //
      // Adjust back to 00:00 12/31/1974, convert to centi-seconds.
      //
      timeunion.ftime = (timeunion.ftime - TANDEM_EPOCH) / 10000;

      if (error != 0)
         timeunion.ftime = -1;

      a [0] = timeunion.lowestshort;
      a [1] = timeunion.lowshort;
      a [2] = timeunion.highshort;
     }
}

extern "C"
DLLEXPORT
void TIME( short * a )
// time returned here is previously described format (in CONTIME)
{
   short ticks[ 3 ];

   TIMESTAMP( ticks );  // get the current time
   CONTIME( a, ticks[0], ticks[ 1 ], ticks[ 2 ] );
}

extern "C"
DLLEXPORT
_int64 TIME_SINCE_COLDLOAD (void)
//
//  This procedure returns a four-word timestamp which is the
//  number of microseconds since this processor was loaded.
//  The value returned is always monotonically increasing in real time;
//  it is not affected by SetSystemTime or anything else.
//
{
   _int64 microseconds;

   GetMicroseconds (&microseconds, 2);

   return (microseconds);
}

Int64 julianTimestampLinux() 
{
   struct timespec timeVal;
   int retcode = clock_gettime(CLOCK_REALTIME, &timeVal);
   if (retcode != 0)
      return -1;
   Int64 julianTimeVal = ComRtGetJulianFromUTC(timeVal);
   return julianTimeVal;
}

extern "C"
DLLEXPORT
_int64 JULIANTIMESTAMP (short   type,
                                  short * tuid,
                                  short * error,
                                  short   node
                                 )
   // short type;              // input,  optional, request type code:
                               //         0: return current time (GMT) (default)
                               //         1: return system cold load time
                               //         2: >>>>>> NOT YET IMPLEMENTED ON NT <<<<<<
                               //         3: return time elapsed since coldload

   // short * tuid;            // output, optional, "time update ID"
                               //         see SETSYSTEMCLOCK for description
                               //         >>>>>> NOT YET IMPLEMENTED ON NT <<<<<<

   // short * error;           // output, optional, error status returned from
                               //         remote node requests.

   // short   node;            // input,  optional, a system number
                               //         allows caller to specify the node
                               //         for which the request is being made.
                               //         -1: current node (same as not
                               //             supplying a value)
                               //         Only -1 is currently accepted on NT.

   // _int64 result            // output, the requested Julian timestamp
{
   _int64 time;

   if (type == OMITSHORT)
      type = 0;

   if (node == OMITSHORT)
      node = -1;
   
   if (node == -1)
      {  // local request
      switch (type)
         {
         case 0:  // GMT
            {
            if (! sv_envJulianTimestamp) {
               sv_useLinuxJulianTimestamp = TRUE;
               const char *strUseLinuxJulianTimestamp = getenv("USE_LINUX_JULIANTIMESTAMP");
               if (strUseLinuxJulianTimestamp != NULL) {
                  if (atoi(strUseLinuxJulianTimestamp) == 0)
                     sv_useLinuxJulianTimestamp = FALSE;
               }
               sv_envJulianTimestamp = TRUE;
            }
            if (sv_useLinuxJulianTimestamp) {
               time = julianTimestampLinux(); 
               if (time == -1) {
                  if (error != NULL)
                     *error = errno;
               } 
               return time;
            }
            
            if (!GetMicroseconds (&time, 0))
               time = (_int64) -1;
            else
               time = time + MICROSOFT_EPOCH;

            break;
            }

         case 1:  // SYSTEM LOAD TIME
            {
            if (!GetMicroseconds (&time, 1))
               time = (_int64) -1;
            else
               time = time + MICROSOFT_EPOCH;

            break;
            }

         case 2:  // SYSGEN TIME
            {
            time = (_int64) -1;

            break;
            }

         case 3:
            {
            time = TIME_SINCE_COLDLOAD ();

            break;
            }

         default: // TYPE parameter value error
            {
            time = (_int64) -1;

            break;
            }
         }

      if (error != 0)
         *error = ((time == (_int64) -1) ? -1 : 0);
      }
   else
      {
      time = (_int64) -1;

      if (error != 0)
         *error = -1;
      }

   return( time );
}



extern "C"
DLLEXPORT
int INTERPRETINTERVAL(_int64   time,        // INPUT,  REQUIRED - time in microseconds
						short   *hours,       // OUTPUT, optional - hours
						short   *minutes,     // OUTPUT, optional - minutes
						short   *seconds,     // OUTPUT, optional - seconds
						short   *milsecs,     // OUTPUT, optional - milliseconds
						short   *microsecs)   // OUTPUT, optional - microseconds
											  // returned value   - days or -1 for error
{
    _int64 microsecs_;
    _int64 milsecs_;
    _int64 seconds_;
    _int64 minutes_;
    _int64 hours_;
    _int64 days_;

    if (time < 0)
        return -1;          // time intervals can only be positive

    seconds_ = time / 1000000; // divide time in microseconds by 10**6 to get sec.
    microsecs_ = seconds_ * 1000000; //multiply seconds_ by 10**6 to get microsec.
    microsecs_ = time - microsecs_;  // compute time mod 10**6
    milsecs_ = microsecs_ / 1000;
    minutes_ = seconds_   /   60;
    hours_   = minutes_   /   60;
    days_    = hours_     /   24;

    //  at this point,
    //  microsec_ has the microsec and MILLISEC portions of time in microseconds
    //  milsecs_  has the correct value
    //  seconds_  has time in seconds
    //  minutes_  has time in minutes
    //  hours_    has time in hours
    //  days_     has time in days

    if (microsecs != NULL)
        *microsecs = (microsecs_ - milsecs_ * 1000);
    if (milsecs != NULL)
        *milsecs   = (milsecs_ );
    if (seconds != NULL)
        *seconds   = (seconds_ - minutes_ * 60);
    if (minutes != NULL)
        *minutes   = (minutes_ - hours_   * 60);
    if (hours != NULL)
        *hours     = (hours_   - days_    * 24);
    return ( days_ );
}


extern "C"
DLLEXPORT
void INTERPRETJULIANDAYNO (long    julianDayNo,
                                     short * year,
                                     short * month,
                                     short * day
                                    )
{
   const unsigned long DAYS_PER_400_YEARS = 146097L,
                       DAYS_PER_4_YEARS   =   1461L;

   unsigned long     century;     // No longer than most, actually.
   unsigned short    year_of_century;

   unsigned long     temp1;   // to hold   ((JDN - JDO) * 4 - 1)
   unsigned long     temp2;   // to hold ((((JDN - JDO) * 4 - 1) '\' 146097) / 4) * 4 + 3
   unsigned short    temp3;   // to hold  (((TEMP2 '\' 1461) + 4) / 4) * 5 - 3
   unsigned short    temp4;   // to hold     TEMP3 / 153

                          // 0x1A431F                        0x308F3B
   if ((julianDayNo <= JULIAN_DATE_OFFSET) || (julianDayNo > JDN_MAX))
      {
      *year = -1;

      return;
      }

   // see proc INTERPRETTIMESTAMP for documentation of the algorithm
   // used to break up JULIANDAYNO into Gregorian Year/Month/Day.
   //
   // this can be a min of 0 and a max of 0x59306F

   temp1 = (julianDayNo - JULIAN_DATE_OFFSET) * 4 - 1L;

   temp2 = ((temp1 - ((century = temp1 / DAYS_PER_400_YEARS)
                     ) * DAYS_PER_400_YEARS
            ) / 4
           ) * 4 + 3L;

   year_of_century = (unsigned short) (temp2 / DAYS_PER_4_YEARS);

   temp3 = ((((unsigned short) (temp2 - (year_of_century * DAYS_PER_4_YEARS))
             ) + 4
            ) / 4
           ) * 5 - 3;

   temp4 = temp3 / 153;

   *day = ((temp3 - (temp4 * 153)) + 5) / 5;

   if (temp4 > 9)
      {
      *month = temp4 - 9;
      year_of_century = year_of_century + 1;
      }
   else
      *month = temp4 + 3;

   *year = ((short) century) * 100 + year_of_century;

   return;
}

short RANGE_CHECKER (const short params[],
                     const short lowlimit[],
                     const short highlimit[],
                     const short count
                    )
{
   short          errors    = 0;
   unsigned short errorbit  = 0100000;  // Octal constant.

   for (short x = 0;(x < (count - 1)); x++)
      {
      if ((params [x] < lowlimit [x]) ||
          (params [x] > highlimit [x])
         )
         {
         errors = errors | errorbit;
         }

      errorbit = errorbit >> 1;
      }

   return (errors);
}

DLLEXPORT
long COMPUTEJULIANDAYNO (short year, short month, short day, short * error)
   {
   const unsigned long DAYS_PER_400_YEARS = 146097L,
                       DAYS_PER_4_YEARS   =   1461L;

   const short limits [6] = {1, 1, 1, 10000, 12, 31};

   short    cyear   = limits[0];
   short    cmonth  = limits[1];
   short    cday    = limits[2];
   short    century = limits[3];

   short    local_params[3];

   long     jdn;

   // Check that date supplied is between 1/1/1 and 10000/12/31

   if ((year  < 1) || (year  > 10000) ||
       (month < 1) || (month > 12)   ||
       (day   < 1) || (day   > 31)
      )
      {
      if (error)
         {
         local_params[0] = year;
         local_params[1] = month;
         local_params[2] = day;

         *error = RANGE_CHECKER (local_params, &limits[0], &limits[3], 3);
         }

      return (-1L);
      }

   if (month <= 2)
      {
      cmonth = month + 9;
      cyear  = year - 1;
      }
   else
      {
      cmonth = month - 3;
      cyear  = year;
      }

   century = cyear/100;

   jdn =   ((((unsigned long) century) * DAYS_PER_400_YEARS) / 4)
         + (((((unsigned long) cyear) - (((unsigned long) century)*100)) * DAYS_PER_4_YEARS) / 4)
         + ((unsigned long) ((cmonth*153+2) / 5 + day)) + JULIAN_DATE_OFFSET;

   if (day > 28)
      {
      INTERPRETJULIANDAYNO( jdn, &cyear, &cmonth, &cday);

      if (cday != day)
         {
         if (error)
            {
            *error = 060000;   // Octal constant.

            return (-1L);
            }
         }
      }

   if (error)
      *error = 0;

   return (jdn);
   }

extern "C"
DLLEXPORT
_int64 COMPUTETIMESTAMP (short * date_n_time, short * error)
// COMPUTETIMESTAMP computes a Julian timestamp from an integer array
// that represents a Gregorian date and time of day.

   // const short date_n_time; // input,  required, array [7]
                               //         date_n_time [0] = year
                               //         date_n_time [1] = month
                               //         date_n_time [2] = day
                               //         date_n_time [3] = hours
                               //         date_n_time [4] = minutes
                               //         date_n_time [5] = seconds
                               //         date_n_time [6] = milliseconds
                               //         date_n_time [7] = microseconds

   // short * error;           // output, optional, if passed, indicates
                               //         error checking is requested;
                               //         used to return error status code.
                               //         The error status code consists of
                               //         bits which indicate the element of
                               //         date_n_time that was out of range;
                               //         e.g., 0x8000, bit 31, indicates the
                               //         year was out of range.

   // _int64 result            // output, a Julian timestamp equivalent to
                               //         the value in date_n_time.
{
   const short limits_set[16] = {    1,   1,   1,   0,   0,   0,   0,   0,
                                  10000,  12,  31,  23,  59,  59, 999, 999
                                };

   short limits[16];

#  define year   date_n_time[0]
#  define month  date_n_time[1]
#  define day    date_n_time[2]
#  define hour   date_n_time[3]
#  define minute date_n_time[4]
#  define second date_n_time[5]
#  define millis date_n_time[6]
#  define micros date_n_time[7]

   unsigned long  jdn;
   short date_error;

   // Let COMPUTEJULIANDAYNO do half the work which, of course,
   // includes checking the parameters but only those passed to it.
   //
   jdn = COMPUTEJULIANDAYNO( year, month, day, &date_error );

   // If COMPUTEJULIANDAYNO found no error then check the remaining
   // parameters.  A > compare will do since it is done unsigned
   // and the lower bounds for all parameters left to be checked is
   // zero.
   //
   if (date_error || (hour   > limits_set[11]) || (minute > limits_set[12])
                  || (second > limits_set[13]) || (millis > limits_set[14])
                  || (micros > limits_set[15])
      )
      {
      // At least one parameter has an incorrect value so let
      // RANGE_CHECKER build the error mask but only if the caller
      // asked for it.
      // In any case, no attempt should be made to compute a timestamp
      // from the erroneous parameters; instead, a value of -1 will
      // be returned.
      //
      if (error)
         {
         for (int i=0; i < 16; i++)
            limits [i] = limits_set [i];

         *error =   RANGE_CHECKER( date_n_time, limits, &limits[8], 8 )
                  | date_error;
         }

      return ((_int64) -1);
      }

   // The parameters passed all checks so let's announce a good
   // result to the caller - if that result status was asked for.
  //
   if (error)
      *error = 0;

   // Finally, there's nothing left to do but to compute that
   // timestamp.  Since COMPUTEJULIANDAYNO has already done the harder
   // part of this task, all that is needed here is to bring all
   // pieces together for one microseconds resolution timestamp.
   // Note the "half-day correction" done by subtracting 12 hours;
   // see comments in INTERPRETTIMESTAMP to learn all about it.
   //

   return (((((_int64) (jdn * 24L + ((unsigned long)(hour - 12)))) * 3600)
            + ((_int64) (minute * 60 + second))
           ) * 1000000
           + ((_int64) (((unsigned long) millis) * 1000L + ((unsigned long) micros)))
          );

#  undef year
#  undef month
#  undef day
#  undef hour
#  undef minute
#  undef second
#  undef millis
#  undef micros

}


extern "C"
DLLEXPORT
short NODENAME_TO_NODENUMBER_
							#ifdef TDM_ROSETTA_COMPATIBILITY_
							(unsigned char  *sysname,    //IN OPTIONAL
							#else
							(char           *sysname,    //IN OPTIONAL
							#endif
							short sysnamelen, int  *nodenumber)
{
  *nodenumber = 0;
  return 0;
}

extern "C"
DLLEXPORT
int_16 NODENUMBER_TO_NODENAME_ (int_32 sysnum,
								#ifdef TDM_ROSETTA_COMPATIBILITY_
								unsigned char  *sysname, //OUT
								#else
								char           *sysname, //OUT
								#endif
							    int_16 maxlen, int_16 *syslen)
{
    *syslen = 4;

    if ( maxlen < 4 )
        return FEBUFTOOSMALL;

    memcpy (sysname, "\\NSK", 4);

    return FEOK;
}

extern "C"
DLLEXPORT
int_16 PROCESSHANDLE_DECOMPOSE_
  (int_16        *prochand, // INPUT
   int        *cpu, // OUTPUT
   int        *pin, // OUTPUT
   int_32        *node,     // OUTPUT  THE NODE NUMBER
   unsigned_char *nn,       // OUTPUT  NODE NAME
   int_16         nnml,     // INPUT   MAXIMUM OUTPUT LENGTH
   int_16        *nnl,      // OUTPUT  RETURNED LENGTH
   unsigned_char *name,     // OUTPUT
   int_16         nml,      // INPUT
   int_16        *nl,       // OUTPUT
   fixed_0       *seq )
{
   PNSK_PORT_HANDLE    phandle = (PNSK_PORT_HANDLE)prochand;
   return XPROCESSHANDLE_DECOMPOSE_(phandle, cpu, pin, node, (char *)nn,
                                    nnml, nnl, (char *)name, nml, nl, seq);
}

extern "C"
DLLEXPORT
int_16 MYSYSTEMNUMBER ()
{
	return 0;
}

extern "C"
DLLEXPORT
int_16 GETSYSTEMNAME (int_16 sysnum, int_16 *sysname)
{
    if (sysnum)
        return 0;

    memcpy ((char *)sysname, "\\NSK    ", 8);
    return 1;
}

extern "C"
DLLEXPORT
void DELAY( int_32 nsktimeout )
{
   DWORD timeout;

   if( nsktimeout < 0 )
      timeout = 0;
   else
   {
      // we will assume no one is silly enough to specify something that would
      // cause an overflow on the multiply, because it would make no sense to care
      // about a weeks long timeout.  In any event, we will clip the max.
      // wait time.  BTW, NSK is 10ms and NT is 1ms.

      if( nsktimeout < ( INFINITE / 10 ) )
         timeout = nsktimeout * 10;
      else
         timeout = INFINITE - 1;
   }

   Sleep( timeout );
}

extern "C"
DLLEXPORT
#ifdef TDM_ROSETTA_COMPATIBILITY_
void	NUMOUT (unsigned char  *str,    //OUT
#else
void	NUMOUT (char           *str,    //OUT
#endif
  int_16    number, // logical 16 bit numeric value
  int_16    base,   // conversion base, 2 - 10 allowed
  int_16    width)  // cpnverted number will occupy str to
                    // str[width - 1]
{
  int_32 dnum;

  while ((int_16)(--width) >= 0)
  {
    dnum = (double)number;
    str[width] = (dnum % base) + '0';
    number = dnum / base;
  };
}

