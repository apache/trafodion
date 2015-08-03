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
//
// PREPROC: start of section: 
#if (defined(ptimez_h_) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_
//

#include "rosetta\rosgen.h" /* rosetta utilities */
//
#endif
// PREPROC: end of section: 
//
// #pragma section DELTLIST
//
// PREPROC: start of section: deltlist
#if (defined(ptimez_h_deltlist) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_deltlist
//
 _resident _priv void DELTLIST(NSK_tle *element);
//
#endif
// PREPROC: end of section: deltlist
//
// #pragma section ADDTLIST
//
// PREPROC: start of section: addtlist
#if (defined(ptimez_h_addtlist) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_addtlist
//
 _resident _priv void ADDTLIST(NSK_tle *element);
//
#endif
// PREPROC: end of section: addtlist
//
// #pragma section TLE_PUTENTRY_
//
// PREPROC: start of section: tle_putentry_
#if (defined(ptimez_h_tle_putentry_) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_tle_putentry_
//

 _resident _priv void TLE_PUTENTRY_(int_16 *e);
//
#endif
// PREPROC: end of section: tle_putentry_
//
// #pragma section TIMESYNCMSG
//
// PREPROC: start of section: timesyncmsg
#if (defined(ptimez_h_timesyncmsg) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_timesyncmsg
//

 _resident void TIMESYNCMSG(NSKtimeTSync *tsync);
//
#endif
// PREPROC: end of section: timesyncmsg
//
// #pragma section CONTIME
//
// PREPROC: start of section: contime
#if (defined(ptimez_h_contime) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_contime
//
#ifdef __cplusplus
 extern "C"
#endif
DllImport
void CONTIME(int_16  *a,
                                  int_16   t,
                                  int_16   t1,
                                  int_16   t2);
//
#endif
// PREPROC: end of section: contime
//
// #pragma section TIMESTAMP
//
// PREPROC: start of section: timestamp
#if (defined(ptimez_h_timestamp) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_timestamp
//
#ifdef __cplusplus
 extern "C"
#endif
DllImport
void TIMESTAMP(int_16 *a);
//
#endif
// PREPROC: end of section: timestamp
//
// #pragma section TIME
//
// PREPROC: start of section: time
#if (defined(ptimez_h_time) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_time
//
#ifdef __cplusplus
 extern "C"
#endif
DllImport void TIME(int_16 *a);
//
#endif
// PREPROC: end of section: time
//
// #pragma section PK_ALARM_START_
//
// PREPROC: start of section: pk_alarm_start_
#if (defined(ptimez_h_pk_alarm_start_) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_pk_alarm_start_
//

 _callable _resident _variable _cc_status PK_ALARM_START_
  (int_32   toval,
   int_16   parm1,
   int_32   parm2,
   int_16  *tleid);
//
#endif
// PREPROC: end of section: pk_alarm_start_
//
// #pragma section SIGNALTIMEOUT
//
// PREPROC: start of section: signaltimeout
#if (defined(ptimez_h_signaltimeout) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_signaltimeout
//
#ifdef __cplusplus
extern "C"
#endif
DllImport
_cc_status SIGNALTIMEOUT
  (int_32   toval,
   int_16   parm1,
   int_32   parm2,
   int_16  *tleid);
//
#endif
// PREPROC: end of section: signaltimeout
//
// #pragma section CLEARALLTIMERS
//
// PREPROC: start of section: clearalltimers
#if (defined(ptimez_h_clearalltimers) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_clearalltimers
//
 _priv _resident void CLEARALLTIMERS(int_16  pin);
//
#endif
// PREPROC: end of section: clearalltimers
//
// #pragma section CANCELTIMEOUT_PRIV_
//
// PREPROC: start of section: canceltimeout_priv_
#if (defined(ptimez_h_canceltimeout_priv_) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_canceltimeout_priv_
//

 _priv _resident int_16 CANCELTIMEOUT_PRIV_
  (int_16   tleid,             // <INPUT> IDENTIFIES THE TIMER TO BE CANCELLED.
                               //         <0:14> : UNDEFINED, MUST BE 0.
                               //         .<15> = 1 : MEMBER OF A PROCESS GROUP.
                               //
   int_16   expected_tle_type, // <INPUT> THE EXPECTED TYPE OF TIMER.
                               //
   int_16   flags,             // <INPUT> THE VARIOUS FLAGS ARE
                               //
   int_32  *time_to_timeout)   // <OUTPUT> THE TIME AFTER WHICH THE TIMER
//          WOULD HAVE EXPIRED.
;
//
#endif
// PREPROC: end of section: canceltimeout_priv_
//
// #pragma section CANCELTIMEOUT_GROUP
//
// PREPROC: start of section: canceltimeout_group
#if (defined(ptimez_h_canceltimeout_group) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_canceltimeout_group
//

_alias("CANCELTIMEOUT^GROUP") _callable _resident
_cc_status CANCELTIMEOUT_GROUP(int_16  tleid);
//
#endif
// PREPROC: end of section: canceltimeout_group
//
// #pragma section CANCELTIMEOUT
//
// PREPROC: start of section: canceltimeout
#if (defined(ptimez_h_canceltimeout) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_canceltimeout
//
#ifdef __cplusplus
extern "C"
#endif
 DllImport _cc_status CANCELTIMEOUT
  (int_16  tleid);
//
#endif
// PREPROC: end of section: canceltimeout
//
// #pragma section INTERPRETJULIANDAYNO
//
// PREPROC: start of section: interpretjuliandayno
#if (defined(ptimez_h_interpretjuliandayno) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_interpretjuliandayno
//
 #ifdef __cplusplus
 extern "C"
#endif
DllImport
void INTERPRETJULIANDAYNO (int_32   juliandayno,
                                                int_16  *year,
                                                int_16  *month,
                                                int_16  *day);
//
#endif
// PREPROC: end of section: interpretjuliandayno
//
// #pragma section COMPUTEJULIANDAYNO
//
// PREPROC: start of section: computejuliandayno
#if (defined(ptimez_h_computejuliandayno) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_computejuliandayno
//
#ifdef __cplusplus
 extern "C"
#endif
// Originally, this routine returned a long.  It was changed to return an
// int_32 so that it would be compatible with the cextdecs.h declaration.
// Callers won't notice any difference.
DllImport int_32 COMPUTEJULIANDAYNO
  (int_16   year,
   int_16   month,
   int_16   day,
   int_16  *error);
//
#endif
// PREPROC: end of section: computejuliandayno
//
// #pragma section INTERPRETTIMESTAMP
//
// PREPROC: start of section: interprettimestamp
#if (defined(ptimez_h_interprettimestamp) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_interprettimestamp
//
#ifdef __cplusplus
 extern "C"
#endif
DllImport
int_32 INTERPRETTIMESTAMP (fixed_0   juliantimestamp,
                                                int_16   *date_n_time);
//
#endif
// PREPROC: end of section: interprettimestamp
//
// #pragma section COMPUTETIMESTAMP
//
// PREPROC: start of section: computetimestamp
#if (defined(ptimez_h_computetimestamp) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_computetimestamp
//
#ifdef __cplusplus
 extern "C"
#endif
DllImport _int64	COMPUTETIMESTAMP
  (int_16 *date_n_time,
   int_16 *error);
//
#endif
// PREPROC: end of section: computetimestamp
//
// #pragma section JULIANTIMESTAMP
//
// PREPROC: start of section: juliantimestamp
#if (defined(ptimez_h_juliantimestamp) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_juliantimestamp
//
#ifdef __cplusplus
 extern "C"
#endif
DllImport
fixed_0 JULIANTIMESTAMP
  (int_16   type,
   int_16  *tuid,
   int_16  *error,
   int_16   node);
//
#endif
// PREPROC: end of section: juliantimestamp
//
// #pragma section USA66_DST
//
// PREPROC: start of section: usa66_dst
#if (defined(ptimez_h_usa66_dst) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_usa66_dst
//
#ifdef __cplusplus
extern "C"
#endif
DllImport fixed_0 USA66_DST
  (fixed_0   gmt,
   fixed_0   lstoffset,
   fixed_0  *nextchangegmt)// CALLED BY DP2
;
//
#endif
// PREPROC: end of section: usa66_dst
//
// #pragma section DSTOFFSET
//
// PREPROC: start of section: dstoffset
#if (defined(ptimez_h_dstoffset) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_dstoffset
//
 _variable _priv _resident fixed_0 DSTOFFSET
  (fixed_0   gmttime,
   fixed_0  *nextchangegmt,
   int_16   *cc)// NOW USED BY DP2
;
//
#endif
// PREPROC: end of section: dstoffset
//
// #pragma section CONVERTTIMESTAMP
//
// PREPROC: start of section: converttimestamp
#if (defined(ptimez_h_converttimestamp) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_converttimestamp
//
#ifdef __cplusplus
extern "C" 
#endif
DllImport
fixed_0 CONVERTTIMESTAMP (fixed_0   timestamp,
                          int_16    direction,
                          int_16    node,
                          int_16   *error);
//
#endif
// PREPROC: end of section: converttimestamp
//
// #pragma section SETSYSTEMCLOCKONLY
//
// PREPROC: start of section: setsystemclockonly
#if (defined(ptimez_h_setsystemclockonly) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_setsystemclockonly
//
 _resident _extensible _callable _cc_status SETSYSTEMCLOCKONLY
  (fixed_0  juliangmt,
   int_16   mode,
   int_16   tuid);
//
#endif
// PREPROC: end of section: setsystemclockonly
//
// #pragma section SETSYSTEMCLOCK
//
// PREPROC: start of section: setsystemclock
#if (defined(ptimez_h_setsystemclock) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_setsystemclock
//
 _resident _extensible _callable _cc_status SETSYSTEMCLOCK
  (fixed_0  juliangmt,
   int_16   mode,
   int_16   tuid);
//
#endif
// PREPROC: end of section: setsystemclock
//
// #pragma section ADDDSTTRANSITION
//
// PREPROC: start of section: adddsttransition
#if (defined(ptimez_h_adddsttransition) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_adddsttransition
//
 _extensible _callable _cc_status ADDDSTTRANSITION
  (fixed_0  lowgmt,
   fixed_0  highgmt,
   int_16   offset,
   int_16   type);
//
#endif
// PREPROC: end of section: adddsttransition
//
// #pragma section TIMESETMSG
//
// PREPROC: start of section: timesetmsg
#if (defined(ptimez_h_timesetmsg) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_timesetmsg
//

 _resident _priv void TIMESETMSG(NSKtimeTSet *tset);
//
#endif
// PREPROC: end of section: timesetmsg
//
// #pragma section TIMEADJMSG
//
// PREPROC: start of section: timeadjmsg
#if (defined(ptimez_h_timeadjmsg) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_timeadjmsg
//

 _resident _priv void TIMEADJMSG(NSKtimeTAdj *tadj);
//
#endif
// PREPROC: end of section: timeadjmsg
//
// #pragma section INSERTDSTTRANS
//
// PREPROC: start of section: insertdsttrans
#if (defined(ptimez_h_insertdsttrans) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_insertdsttrans
//

 _resident _priv void INSERTDSTTRANS(int_16 *data);
//
#endif
// PREPROC: end of section: insertdsttrans
//
// #pragma section CHKDSTLIM
//
// PREPROC: start of section: chkdstlim
#if (defined(ptimez_h_chkdstlim) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_chkdstlim
//
 _resident _priv int_16 CHKDSTLIM();
//
#endif
// PREPROC: end of section: chkdstlim
//
// #pragma section LABELTIMESTAMP
//
// PREPROC: start of section: labeltimestamp
#if (defined(ptimez_h_labeltimestamp) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_labeltimestamp
//
 _callable _resident fixed_0 LABELTIMESTAMP();
//
#endif
// PREPROC: end of section: labeltimestamp
//
// #pragma section TIME_SINCE_COLDLOAD
//
// PREPROC: start of section: time_since_coldload
#if (defined(ptimez_h_time_since_coldload) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_time_since_coldload
//
#ifdef __cplusplus
extern "C" 
#endif
DllImport
_int64 TIME_SINCE_COLDLOAD(void);
//
#endif
// PREPROC: end of section: time_since_coldload
//
// #pragma section THREEWORDTIMESTAMP
//
// PREPROC: start of section: threewordtimestamp
#if (defined(ptimez_h_threewordtimestamp) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_threewordtimestamp
//
#ifdef __cplusplus
 extern "C"
#endif
DllImport
void THREEWORDTIMESTAMP (fixed_0   juliantimestamp,
                                              int_16   *threewordts);
//
#endif
// PREPROC: end of section: threewordtimestamp
//
// #pragma section CONVERTOLDTIMESTAMP
//
// PREPROC: start of section: convertoldtimestamp
#if (defined(ptimez_h_convertoldtimestamp) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_convertoldtimestamp
//
#ifdef __cplusplus
extern "C" 
#endif
DllImport
fixed_0 CONVERTOLDTIMESTAMP
  (int_16 *threewordts);
//
#endif
// PREPROC: end of section: convertoldtimestamp
//
// #pragma section INTERPRETINTERVAL
//
// PREPROC: start of section: interpretinterval
#if (defined(ptimez_h_interpretinterval) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_interpretinterval
//
 _extensible int_32 INTERPRETINTERVAL
  (fixed_0   time,
   int_16   *hours,
   int_16   *minutes,
   int_16   *seconds,
   int_16   *milsecs,
   int_16   *microsecs);
//
#endif
// PREPROC: end of section: interpretinterval
//
// #pragma section DAYOFWEEK
//
// PREPROC: start of section: dayofweek
#if (defined(ptimez_h_dayofweek) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_dayofweek
//
 #ifdef __cplusplus
 extern "C"
#endif
DllImport
int_16 DAYOFWEEK (int_32  jdn);
//
#endif
// PREPROC: end of section: dayofweek
//
// #pragma section SETRMICLOCK
//
// PREPROC: start of section: setrmiclock
#if (defined(ptimez_h_setrmiclock) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_setrmiclock
//
 _extensible int_16 SETRMICLOCK (fixed_0 *jts);
//
#endif
// PREPROC: end of section: setrmiclock
//
// #pragma section READRMICLOCK
//
// PREPROC: start of section: readrmiclock
#if (defined(ptimez_h_readrmiclock) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_readrmiclock
//
 _extensible int_16 READRMICLOCK (fixed_0 *jts);
//
#endif
// PREPROC: end of section: readrmiclock
//
// #pragma section FIND_TLEADDR_
//
// PREPROC: start of section: find_tleaddr_
#if (defined(ptimez_h_find_tleaddr_) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_find_tleaddr_
//

 _priv _resident int_16 FIND_TLEADDR_(int_16  tleid);
//
#endif
// PREPROC: end of section: find_tleaddr_
//
// #pragma section FIND_TLEADDR_X_
//
// PREPROC: start of section: find_tleaddr_x_
#if (defined(ptimez_h_find_tleaddr_x_) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_find_tleaddr_x_
//

 _priv _resident extaddr FIND_TLEADDR_X_
  (int_16  tleid);
//
#endif
// PREPROC: end of section: find_tleaddr_x_
//
// #pragma section TLE_ITERATE_
//
// PREPROC: start of section: tle_iterate_
#if (defined(ptimez_h_tle_iterate_) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_tle_iterate_
//
 _priv _resident int_32 TLE_ITERATE_ (int_16 *tle_iterator);
//
#endif
// PREPROC: end of section: tle_iterate_
//
// #pragma section TLE_ITERATE_TEST_
//
// PREPROC: start of section: tle_iterate_test_
#if (defined(ptimez_h_tle_iterate_test_) || (!defined(ptimez_h_including_section) && !defined(ptimez_h_including_self)))
#undef ptimez_h_tle_iterate_test_
//
 _priv _resident int_32 TLE_ITERATE_TEST_
  (int_16 *tle_iterator); //~ source file above = $QUINCE.GRZDV.STIME

#endif
// PREPROC: end of section: tle_iterate_test_
//
//
#if (!defined(ptimez_h_including_self))
#undef ptimez_h_including_section
#endif
// end of file
