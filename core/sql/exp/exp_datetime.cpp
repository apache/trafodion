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
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         exp_datetime.C
 * Description:  Datetime Type
 *
 *
 * Created:      8/20/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"


#include "exp_stdh.h"
#include "SQLTypeDefs.h"
#include "exp_datetime.h"
#include "exp_interval.h"
#include "exp_clause_derived.h"
#include "NAAssert.h"
#include "exp_bignum.h"


#undef DllImport
#define DllImport __declspec ( dllimport )
#include "rosetta/rosgen.h"

#define ptimez_h_juliantimestamp
#define ptimez_h_including_section
#include "guardian/ptimez.h"
#ifdef ptimez_h_juliantimestamp
Section missing, generate compiler error
#endif

#define ptimez_h_converttimestamp
#define ptimez_h_including_section
#include "guardian/ptimez.h"
#ifdef ptimez_h_converttimestamp
Section missing, generate compiler error
#endif

#define ptimez_h_interprettimestamp
#define ptimez_h_including_section
#include "guardian/ptimez.h"
#ifdef ptimez_h_interprettimestamp
Section missing, generate compiler error
#endif

#define ptimez_h_computetimestamp
#define ptimez_h_including_section
#include "guardian/ptimez.h"
#ifdef ptimez_h_computetimestamp
Section missing, generate compiler error
#endif

// Forword declaration of static helper function.
//
static short
copyDatetimeFields(rec_datetime_field startField,
                   rec_datetime_field endField,
                   short srcFractPrec,
                   short dstFractPrec,
                   char *srcData,
                   char *dstData,
                   Lng32 dstLen,
                   NABoolean *roundedDownFlag);

// Helper function to format extra error message text and report error
// srcData is not null terminated, so need a buffer copy to build a C-style string
// 
static void 
raiseDateConvErrorWithSrcData(int srcLen, ComDiagsArea** diagsArea, char *srcData, CollHeap *heap)
{
    char errstr[MAX_OFFENDING_SOURCE_DATA_DISPLAY_LEN];
    str_pad(errstr, sizeof(errstr), 0 );
    if(srcLen > MAX_OFFENDING_SOURCE_DATA_DISPLAY_LEN -1 )
      srcLen = MAX_OFFENDING_SOURCE_DATA_DISPLAY_LEN -1;
    str_cpy_all(errstr, srcData, srcLen);
    ExRaiseSqlError(heap, diagsArea, EXE_CONVERT_DATETIME_ERROR,NULL,NULL,NULL,NULL,errstr);
}

// Helper function to format extra error message text and report error
// 
static void 
raiseDateConvErrorWithSrcDataNumeric(ComDiagsArea** diagsArea, long srcData, CollHeap *heap)
{
   char errstr[MAX_OFFENDING_SOURCE_DATA_DISPLAY_LEN];
   str_pad(errstr, sizeof(errstr), 0 );
   str_sprintf(errstr,"%ld",srcData);
   ExRaiseSqlError(heap, diagsArea, EXE_CONVERT_DATETIME_ERROR,NULL,NULL,NULL,NULL,errstr);
}
//////////////////////////////////////////////
// Defined in exp_datetime.h
//
//  struct DatetimeFormatInfo
//  {
//    Lng32 format;
//    const char * str;
//    Lng32 minLen;
//    Lng32 maxLen
//  };
////////////////////////////////////////////// 
const ExpDatetime::DatetimeFormatInfo ExpDatetime::datetimeFormat[] =
  {
    {ExpDatetime::DATETIME_FORMAT_DEFAULT,   "YYYY-MM-DD",            10, 10},
    {ExpDatetime::DATETIME_FORMAT_USA,       "MM/DD/YYYY",            10, 10},
    {ExpDatetime::DATETIME_FORMAT_EUROPEAN,  "DD.MM.YYYY",            10, 10},
    {ExpDatetime::DATETIME_FORMAT_DEFAULT2,  "YYYY-MM",                7,  7},
    {ExpDatetime::DATETIME_FORMAT_USA2,      "MM/DD/YYYY",            10, 10},
    {ExpDatetime::DATETIME_FORMAT_USA3,      "YYYY/MM/DD",            10, 10},
    {ExpDatetime::DATETIME_FORMAT_USA4,      "YYYYMMDD",               8,  8},
    {ExpDatetime::DATETIME_FORMAT_USA5,      "YY/MM/DD",               8,  8},
    {ExpDatetime::DATETIME_FORMAT_USA6,      "MM/DD/YY",               8,  8},
    {ExpDatetime::DATETIME_FORMAT_USA7,      "MM-DD-YYYY",            10, 10},
    {ExpDatetime::DATETIME_FORMAT_USA8,      "YYYYMM",                 6,  6},
    {ExpDatetime::DATETIME_FORMAT_EUROPEAN2, "DD-MM-YYYY",            10, 10},
    {ExpDatetime::DATETIME_FORMAT_EUROPEAN3, "DD-MON-YYYY",           11, 11},
    {ExpDatetime::DATETIME_FORMAT_EUROPEAN4, "DDMONYYYY",              9,  9},

    {ExpDatetime::DATETIME_FORMAT_TS4,       "HH24:MI:SS",             8,  8},

    {ExpDatetime::DATETIME_FORMAT_TS1,       "YYYYMMDDHH24MISS",      14, 14},
    {ExpDatetime::DATETIME_FORMAT_TS2,       "DD.MM.YYYY:HH24.MI.SS", 19, 19},
    {ExpDatetime::DATETIME_FORMAT_TS3,       "YYYY-MM-DD HH24:MI:SS", 19, 19},
    {ExpDatetime::DATETIME_FORMAT_TS5,       "YYYYMMDD:HH24:MI:SS",   17, 17},
    {ExpDatetime::DATETIME_FORMAT_TS6,       "MMDDYYYY HH24:MI:SS",   17, 17},
    {ExpDatetime::DATETIME_FORMAT_TS7,       "MM/DD/YYYY HH24:MI:SS", 19, 19},
    {ExpDatetime::DATETIME_FORMAT_TS8,       "DD-MON-YYYY HH:MI:SS",  20, 20},
    {ExpDatetime::DATETIME_FORMAT_TS9,       "MONTH DD, YYYY, HH:MI", 19, 25},
    {ExpDatetime::DATETIME_FORMAT_TS10,      "DD.MM.YYYY HH24.MI.SS", 19, 19},
    {ExpDatetime::DATETIME_FORMAT_TS11,      "YYYY/MM/DD HH24:MI:SS", 19, 19},
 
    {ExpDatetime::DATETIME_FORMAT_NUM1,      "99:99:99:99",           11, 11},
    {ExpDatetime::DATETIME_FORMAT_NUM2,      "-99:99:99:99",          12, 12},

    {ExpDatetime::DATETIME_FORMAT_EXTRA_HH,  "HH",                     2,  2},
    {ExpDatetime::DATETIME_FORMAT_EXTRA_HH12,"HH12",                   2,  2},
    {ExpDatetime::DATETIME_FORMAT_EXTRA_HH24,"HH24",                   2,  2},
    {ExpDatetime::DATETIME_FORMAT_EXTRA_MI,  "MI",                     2,  2},
    {ExpDatetime::DATETIME_FORMAT_EXTRA_SS,  "SS",                     2,  2},
    {ExpDatetime::DATETIME_FORMAT_EXTRA_YYYY,"YYYY",                   4,  4},
    {ExpDatetime::DATETIME_FORMAT_EXTRA_YYY, "YYY",                    3,  3},
    {ExpDatetime::DATETIME_FORMAT_EXTRA_YY,  "YY",                     2,  2},
    {ExpDatetime::DATETIME_FORMAT_EXTRA_Y,   "Y",                      1,  1},
    {ExpDatetime::DATETIME_FORMAT_EXTRA_MON, "MON",                    3,  3},
    {ExpDatetime::DATETIME_FORMAT_EXTRA_MM,  "MM",                     2,  2},
    {ExpDatetime::DATETIME_FORMAT_EXTRA_DY,  "DY",                     3,  3},
    {ExpDatetime::DATETIME_FORMAT_EXTRA_DAY, "DAY",                    6,  9},
    {ExpDatetime::DATETIME_FORMAT_EXTRA_CC,  "CC",                     2,  2},
    {ExpDatetime::DATETIME_FORMAT_EXTRA_D,   "D",                      1,  1},
    {ExpDatetime::DATETIME_FORMAT_EXTRA_DD,  "DD",                     2,  2},
    {ExpDatetime::DATETIME_FORMAT_EXTRA_DDD, "DDD",                    1,  3},
    {ExpDatetime::DATETIME_FORMAT_EXTRA_W,   "W",                      1,  1},
    {ExpDatetime::DATETIME_FORMAT_EXTRA_WW,  "WW",                     1,  2},
    {ExpDatetime::DATETIME_FORMAT_EXTRA_J,   "J",                      7,  7},
    {ExpDatetime::DATETIME_FORMAT_EXTRA_Q,   "Q",                      1,  1},

    // formats that are replaced by one of the other formats at bind time
    {ExpDatetime::DATETIME_FORMAT_UNSPECIFIED,   "UNSPECIFIED",       11, 11}
  };

UInt32 Date2Julian(int y, int m ,int d)
{
  int myjulian = 0;
  int mycentury = 0;
  if ( m <= 2)
    {
      m = m+13;
      y = y+4799;
    }
  else
    {
      m = m+1;
      y = y+4800;
    }

  mycentury = y / 100;
  myjulian = y * 365 - 32167;
  myjulian += y/4 - mycentury + mycentury / 4;
  myjulian += 7834 * m / 256 + d;
  return myjulian;
}

ExpDatetime::ExpDatetime()
{
  setClassID(SimpleTypeID);
}

ExpDatetime::~ExpDatetime()
{
}
Attributes * ExpDatetime::newCopy()
{
  ExpDatetime * new_copy = new ExpDatetime();
  *new_copy = *this;
  return new_copy;
};
Attributes * ExpDatetime::newCopy(CollHeap * heap)
{
  ExpDatetime * new_copy = new(heap) ExpDatetime();
  *new_copy = *this;
  return new_copy;
};
void ExpDatetime::copyAttrs(Attributes *source_) // copy source attrs to this.
{
  *this = *((ExpDatetime *)source_);
  return;
};
ExpDatetime * 
ExpDatetime::castToExpDatetime()
{
  return this;
} 

Int64 ExpDatetime::getTotalDays(short year, short month, short day)
{
  //
  // Return the number of days since 01/01/0001.
  //
  static const short daysBeforeMonth[] = {
    /* Jan */  0,
    /* Feb */  31,
    /* Mar */  31 + 28,
    /* Apr */  31 + 28 + 31,
    /* May */  31 + 28 + 31 + 30,
    /* Jun */  31 + 28 + 31 + 30 + 31,
    /* Jul */  31 + 28 + 31 + 30 + 31 + 30,
    /* Aug */  31 + 28 + 31 + 30 + 31 + 30 + 31,
    /* Sep */  31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
    /* Oct */  31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
    /* Nov */  31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
    /* Dec */  31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30
  };
  Int64 totalDays = ((year - 1) * 365) + daysBeforeMonth[month - 1] + (day - 1);
  //
  // If the month is January or February, don't include the current year when
  // adjusting for leap years.
  //
  if (month <= 2)
    year--;
  totalDays += (year / 4) - (year / 100) + (year / 400);
  return totalDays;
}

// getDatetimeFields() ===============================================
// This static method returns the start and end fields based on the
// given REC_DATETIME_CODE value.  This method should be the only
// place in the expression evaluator which deals with the
// REC_DATETIME_CODE enumeration values.  All other uses should call
// this method and be based on the start and end fields.
//
// This method has been modified as part of the MP Datetime
// Compatibility project.  It has been extended to handle the
// non-standard SQL/MP datetime types.
//
short ExpDatetime::getDatetimeFields(Lng32 datetimeCode,
                                     rec_datetime_field &startField,
                                     rec_datetime_field &endField)
{
  switch (datetimeCode) {
  case REC_DTCODE_DATE:
  case REC_DTCODE_TIMESTAMP:
  case REC_DTCODE_YEAR:
  case REC_DTCODE_YEAR_MONTH:
  case REC_DTCODE_YEAR_DAY:
  case REC_DTCODE_YEAR_HOUR:
  case REC_DTCODE_YEAR_MINUTE:
  case REC_DTCODE_YEAR_SECOND:
    startField = REC_DATE_YEAR;
    break;
  case REC_DTCODE_MONTH:
  case REC_DTCODE_MONTH_DAY:
  case REC_DTCODE_MONTH_HOUR:
  case REC_DTCODE_MONTH_MINUTE:
  case REC_DTCODE_MONTH_SECOND:
    startField = REC_DATE_MONTH;
    break;
  case REC_DTCODE_DAY:
  case REC_DTCODE_DAY_HOUR:
  case REC_DTCODE_DAY_MINUTE:
  case REC_DTCODE_DAY_SECOND:
    startField = REC_DATE_DAY;
    break;
  case REC_DTCODE_TIME:
  case REC_DTCODE_HOUR:
  case REC_DTCODE_HOUR_MINUTE:
  case REC_DTCODE_HOUR_SECOND:
    startField = REC_DATE_HOUR;
    break;
  case REC_DTCODE_MINUTE:
  case REC_DTCODE_MINUTE_SECOND:
    startField = REC_DATE_MINUTE;
    break;
  case REC_DTCODE_SECOND:
    startField = REC_DATE_SECOND;
    break;
  default:
    return -1;
  }

  switch (datetimeCode) {
  case REC_DTCODE_YEAR:
    endField = REC_DATE_YEAR;
    break;
  case REC_DTCODE_YEAR_MONTH:
  case REC_DTCODE_MONTH:
    endField = REC_DATE_MONTH;
    break;
  case REC_DTCODE_DATE:
  case REC_DTCODE_YEAR_DAY:
  case REC_DTCODE_MONTH_DAY:
  case REC_DTCODE_DAY:
    endField = REC_DATE_DAY;
    break;
  case REC_DTCODE_YEAR_HOUR:
  case REC_DTCODE_MONTH_HOUR:
  case REC_DTCODE_DAY_HOUR:
  case REC_DTCODE_HOUR:
    endField = REC_DATE_HOUR;
    break;
  case REC_DTCODE_YEAR_MINUTE:
  case REC_DTCODE_MONTH_MINUTE:
  case REC_DTCODE_DAY_MINUTE:
  case REC_DTCODE_HOUR_MINUTE:
  case REC_DTCODE_MINUTE:
    endField = REC_DATE_MINUTE;
    break;
  case REC_DTCODE_TIMESTAMP:
  case REC_DTCODE_YEAR_SECOND:
  case REC_DTCODE_MONTH_SECOND:
  case REC_DTCODE_DAY_SECOND:
  case REC_DTCODE_TIME:
  case REC_DTCODE_HOUR_SECOND:
  case REC_DTCODE_MINUTE_SECOND:
  case REC_DTCODE_SECOND:
    endField = REC_DATE_SECOND;
    break;
  default:
    return -1;
  }

  return 0;
}

static const Lng32 powersOfTen[] = {1, 10 ,100, 1000, 10000, 100000, 1000000,
                                    10000000, 100000000, 1000000000};

void ExpDatetime::convertDatetimeToInterval
( rec_datetime_field datetimeStartField
, rec_datetime_field datetimeEndField
, short fractionPrecision
, rec_datetime_field intervalEndField
, char *datetimeOpData
, Int64 &interval
, char * intervalBignum
, NABoolean &isBignum
) const
{
  short rc = 0;

  isBignum = FALSE;

  interval = 0;
  short year;
  char month;
  char day;
  for (Int32 field = datetimeStartField; field <= intervalEndField; field++) {
    switch (field) {
    case REC_DATE_YEAR:
      str_cpy_all((char *) &year, datetimeOpData, sizeof(year));
      datetimeOpData += sizeof(year);
      interval = year - 1;
      break;
    case REC_DATE_MONTH:
      str_cpy_all((char *) &month, datetimeOpData, sizeof(month));
      datetimeOpData += sizeof(month);
      interval = (interval * 12) + (month - 1);
      break;
    case REC_DATE_DAY:
      str_cpy_all((char *) &day, datetimeOpData, sizeof(day));
      datetimeOpData += sizeof(day);
      interval = getTotalDays(year, month, day);
      break;
    case REC_DATE_HOUR:
      interval *= 24;
      if (field <= datetimeEndField) {
        char hour;
        str_cpy_all((char *) &hour, datetimeOpData, sizeof(hour));
        datetimeOpData += sizeof(hour);
        interval = interval + hour;
      }
      break;
    case REC_DATE_MINUTE:
      interval *= 60;
      if (field <= datetimeEndField) {
        char minute;
        str_cpy_all((char *) &minute, datetimeOpData, sizeof(minute));
        datetimeOpData += sizeof(minute);
        interval = interval + minute;
      }
      break;
    case REC_DATE_SECOND:
      interval *= 60;

      if (field <= datetimeEndField) {
        char second;
        str_cpy_all((char *) &second, datetimeOpData, sizeof(second));
        datetimeOpData += sizeof(second);
        interval = interval + second;
        if (fractionPrecision > 0) {
          Lng32 fraction;
          Int64 fraction64;

          str_cpy_all((char *) &fraction, datetimeOpData, sizeof(fraction));

          Int64 multiplicator = powersOfTen[fractionPrecision];
          if (fractionPrecision <= MAX_DATETIME_MICROS_FRACT_PREC) 
            {
              interval *= multiplicator;
              interval = interval + fraction;
            }
          else
            {
              // Int64 may run into an overflow if fract precision is > 6
              // Use bignum computation to do:
              //   interval = interval * multiplicator
              //   interval = interval + fraction
              SimpleType op1ST(REC_BIN64_SIGNED, sizeof(Int64), 0, 0,
                               ExpTupleDesc::SQLMX_FORMAT,
                               8, 0, 0, 0, Attributes::NO_DEFAULT, 0);

              char *op_data[3];
              char mulBignum[BigNum::BIGNUM_TEMP_LEN]; // 16 bytes bignum result length

              op_data[0] = mulBignum; // result
              op_data[1] = (char*) &interval;
              op_data[2] = (char*) &multiplicator;
              rc = EXP_FIXED_BIGN_OV_MUL(&op1ST, &op1ST, op_data);

              BigNum op1BN(BigNum::BIGNUM_TEMP_LEN, BigNum::BIGNUM_TEMP_PRECISION, 0, 0);
              
              char addBignum[BigNum::BIGNUM_TEMP_LEN];
              fraction64 = fraction;

              op_data[0] = addBignum;
              op_data[1] = mulBignum;
              op_data[2] = (char*)&fraction64;
              rc = EXP_FIXED_BIGN_OV_ADD(&op1BN, &op1ST, op_data);
              
              if (intervalBignum)
                {
                  str_cpy_all(intervalBignum, addBignum, BigNum::BIGNUM_TEMP_LEN);
                  isBignum = TRUE;
                }
            }
        }
      }
      break;
    }
  }
}

short ExpDatetime::getYearMonthDay(Int64 totalDays,
                                   short &year,
                                   char &month,
                                   char &day)
{
  const unsigned short daysPerYear = 365;
  const unsigned short daysPer4Years = daysPerYear * 4 + 1;
  const unsigned short daysPer100Years = daysPer4Years * 25 - 1;
  const Int64 daysPer400Years = daysPer100Years * 4 + 1;
  Int64 numLeapCenturies = totalDays / daysPer400Years;
  totalDays -= numLeapCenturies * daysPer400Years;
  Int64 numCenturies = totalDays / daysPer100Years;
  //
  // If the date is the last day of a leap century, e.g. 12/31/2000, then
  // numCenturies will be 4 since the century had an extra day.  Correct
  // numCenturies to 3.
  //
  if (numCenturies > 3)
    numCenturies = 3;
  totalDays -= numCenturies * daysPer100Years;
  Int64 numLeapYears = totalDays / daysPer4Years;
  totalDays -= numLeapYears * daysPer4Years;
  Int64 numYears = totalDays / daysPerYear;
  //
  // If the date is the last day of a leap year, e.g. 12/31/1996, numYears
  // will be 4 since the year had an extra day.  Correct numYears to 3.
  //
  if (numYears > 3)
    numYears = 3;
  totalDays -= numYears * daysPerYear;
  numYears += (numLeapCenturies * 400) +
              (numCenturies * 100) +
              (numLeapYears * 4) +
              1;  /* the base year is 0001 */
  if (numYears > 9999)
    return -1;
  year = (short) int64ToInt32(numYears);
  static const short daysInMonth[] = {
    0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
  };
  for (month = 1; totalDays >= daysInMonth[month]; month++) {
    if ((month == 2) &&  /* February */
        ((year % 4) == 0) &&
        (((year % 100) != 0) || ((year % 400) == 0))) {  /* leap year */
      if (totalDays == 28)  /* leap day */
        break;
      totalDays -= 29;
    } else
      totalDays -= daysInMonth[month];
  }
  day = (char) int64ToInt32(totalDays);
  day++;  /* each month begins at day 1 */
  return 0;
}

short ExpDatetime::convertIntervalToDatetime(Int64 interval,
                                             char * intervalBignum,
                                             rec_datetime_field startField,
                                             rec_datetime_field endField,
                                             short fractionPrecision,
                                             char *datetimeOpData) const
{
  short rc = 0;

  short year;
  char month;
  char day;
  char hour;
  char minute;
  char second;
  Lng32 fraction;
  Int32 field = endField;
  for (; field >= startField; field--) {
    switch (field) {
    case REC_DATE_SECOND:
      if (fractionPrecision > 0) {

        //Int64 multiplicator = powersOfTen[fractionPrecision];
        Int64 divisor = powersOfTen[fractionPrecision];
        Int64 dividend = 0;
        if (fractionPrecision <= MAX_DATETIME_MICROS_FRACT_PREC)
          {
            Int64 dividend = interval;
            interval = dividend / divisor;
            dividend = dividend - (interval * divisor);
            //
            // Underflow is allowed for time types, so wrap around if necessary.
            //
            if (dividend < 0) {
              dividend += divisor;
              interval -= 1;
            }
            fraction = int64ToInt32(dividend);
          }
        else
          {
            SimpleType opST(REC_BIN64_SIGNED, sizeof(Int64), 0, 0,
                            ExpTupleDesc::SQLMX_FORMAT,
                            8, 0, 0, 0, Attributes::NO_DEFAULT, 0);
            BigNum opBN(BigNum::BIGNUM_TEMP_LEN, BigNum::BIGNUM_TEMP_PRECISION, 0, 0);
            
            char *op_data[2];
            
            op_data[0] = intervalBignum;
            op_data[1] = (char*) &divisor;
            Int64 quotient = -1;
            short ov;
            dividend = 
              EXP_FIXED_BIGN_OV_MOD(&opBN, &opST, op_data, &ov, &quotient);
            interval = quotient;

            fraction = int64ToInt32(dividend);
          }

      }
      //
      // Underflow is allowed for time types, so wrap around if necessary.
      // But be aware that the sign of the result of a modulus operation
      // involving a negative operand is implementation-dependent according
      // to the C++ reference manual.  In this case, we prefer the result to
      // be negative.
      //
      {
        Int32 sec = (Int32) (interval % 60);
        if ((interval < 0) && ( sec > 0))
          sec = - sec;
        interval /= 60;
        if ( sec < 0) {
          sec+= 60;
          interval -= 1;
        }
        second = (char) sec;
        break;
      }
    case REC_DATE_MINUTE:
      //
      // Underflow is allowed for time types, so wrap around if necessary.
      // But be aware that the sign of the result of a modulus operation
      // involving a negative operand is implementation-dependent according
      // to the C++ reference manual.  In this case, we prefer the result to
      // be negative.
      //
      {
        Int32 mins = (Int32) (interval % 60);
        if ((interval < 0) && (mins > 0))
          mins = - mins;
        interval /= 60;
        if (mins < 0) {
          mins += 60;
          interval -= 1;
        }
        minute = (char) mins;
        break;
      }
    case REC_DATE_HOUR:
      //
      // Underflow is allowed for time types, so wrap around if necessary.
      // But be aware that the sign of the result of a modulus operation
      // involving a negative operand is implementation-dependent according
      // to the C++ reference manual.  In this case, we prefer the result to
      // be negative.
      //
      {
        Int32 hrs = (Int32) (interval % 24);
        if ((interval < 0) && (hrs > 0))
          hrs = - hrs;
        interval /= 24;
        if (hrs < 0) {
          hrs += 24;
          interval -= 1;
        }
        hour = (char) hrs;
        break;
      }
    case REC_DATE_DAY:
      if (getYearMonthDay(interval, year, month, day) != 0)
        return -1;
      break;
    case REC_DATE_MONTH:
      if (endField < REC_DATE_DAY) {
        month = char ((interval % 12) + 1);
        interval /= 12;
      }
      break;
    case REC_DATE_YEAR:
      if (endField < REC_DATE_DAY) {
        interval += 1;  /* the base year is 0001 */
        if (interval > 9999)
          return -1;
        year = (short) int64ToInt32(interval);
      }
      break;
    }
  }
  for (field = startField; field <= endField; field++) {
    switch (field) {
    case REC_DATE_YEAR:
      str_cpy_all(datetimeOpData, (char *) &year, sizeof(year));
      datetimeOpData += sizeof(year);
      break;
    case REC_DATE_MONTH:
      str_cpy_all(datetimeOpData, (char *) &month, sizeof(month));
      datetimeOpData += sizeof(month);
      break;
    case REC_DATE_DAY:
      str_cpy_all(datetimeOpData, (char *) &day, sizeof(day));
      datetimeOpData += sizeof(day);
      break;
    case REC_DATE_HOUR:
      str_cpy_all(datetimeOpData, (char *) &hour, sizeof(hour));
      datetimeOpData += sizeof(hour);
      break;
    case REC_DATE_MINUTE:
      str_cpy_all(datetimeOpData, (char *) &minute, sizeof(minute));
      datetimeOpData += sizeof(minute);
      break;
    case REC_DATE_SECOND:
      str_cpy_all(datetimeOpData, (char *) &second, sizeof(second));
      datetimeOpData += sizeof(second);
      if (fractionPrecision > 0)
        str_cpy_all(datetimeOpData, (char *) &fraction, sizeof(fraction));
      break;
    }
  }
  return 0;
}

short ExpDatetime::validateDate(rec_datetime_field startField,
                                rec_datetime_field endField,
                                char *datetimeOpData,
                                ExpDatetime *attr,
                                short intervalFlag,
                                NABoolean &LastDayPrevMonth)
{

  // Initialize to a valid date.  The year is set to 4 so that if all
  // we have is a month and a day, 02/29 is valid as it should
  // be. Month is set to 1, so that if all we have is a day, 31 is
  // valid.
  //
  short year = 4;
  char month = 1;
  char day = 1;
  short uselastdateflag = FALSE;
  short uselastdateonerrflag = FALSE;
  short ErrorOnInterval = FALSE; 

  for (Int32 field = startField; field <= endField; field++) {
    switch (field) {
    case REC_DATE_YEAR:
      str_cpy_all((char *) &year, datetimeOpData, sizeof(year));
      datetimeOpData += sizeof(year);
      break;
    case REC_DATE_MONTH:
      month = *datetimeOpData;
      datetimeOpData += sizeof(month);
      break;
    case REC_DATE_DAY:
      day = *datetimeOpData;
      datetimeOpData += sizeof(day);
      break;
    default:
      // Only validating date fields. Ignore all others.
      //
      break;
    }
  }

  // Round to the last day of the month.
  if ((intervalFlag) && attr->getlastdaymonthflag())
    uselastdateflag = TRUE;

  // If ending day of resulting date is invalid, day will be rounded
  // down to the last day of the month.
  if ((intervalFlag) && attr->getlastdayonerrflag())
    uselastdateonerrflag = TRUE;

  // Check basic range of the fields of the date.
  //
  if (year > 9999 || year < 1 || month > 12 || month < 1 || day < 1)
    return -1;

  //
  // If the days field is more than the number of days in the month, the date
  // is invalid.
  //
  static const short daysInMonth[] = {
    0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
  };

  static const short daysInMonthNonLeap[] = {
    0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
  };

  if (day > daysInMonth[month])
    // Return error if we're not adjusting for intervals only validating
    // the date.  This is default behavior.  If add_month or date_add 
    // functions are used, then intervalFlag would be set.
    if ((!uselastdateonerrflag) && (!uselastdateflag)) 
       return -1;
    else
    if (!intervalFlag)
       return -1;
    else
    if (uselastdateonerrflag)
       ErrorOnInterval = TRUE; 

  //
  // If the month field is February and the year is not a leap year, the
  // maximum number of days is 28.
  //
  if ((month == 2) && (day > 28) &&
      (((year % 4) != 0) ||
       (((year % 100) == 0) && ((year % 400) != 0))))

    // Return error if we're not adjusting for interval dates, only validating
    // dates. (The default behavior)
    if ((!uselastdateonerrflag) && (!uselastdateflag)) 
       return -1;
    else
    if (!intervalFlag)
       return -1;
    else
    if (uselastdateonerrflag)
       ErrorOnInterval = TRUE; 

  // Check to see whether the month value of the incoming date is the 
  // last day of the month, then set flag here.  We're validating the
  // the incoming 1st date, not the resulting adjusted date.
  if (!intervalFlag)
    {
     // it's a non-leap year
     if (((year % 4) != 0) ||
        (((year % 100) == 0) && ((year % 400) != 0)))
        if (day == daysInMonthNonLeap[month])
           LastDayPrevMonth = TRUE;
        else
           LastDayPrevMonth = FALSE;
     else
        if (day == daysInMonth[month])
           LastDayPrevMonth = TRUE;
        else
           LastDayPrevMonth = FALSE;
    }

  // Adjust resulting date only if one of the flags are set.
  // If not set, then revert to previous behavior and don't adjust dates.
  if ((uselastdateonerrflag) || (uselastdateflag))
    if (intervalFlag)
      {
       if (uselastdateonerrflag) // adjust date only if resulting day is invalid
         {
          // Didn't get an error, so return and don't adjust the date
          if (!ErrorOnInterval)
             return 0;
         }
       else
       if (uselastdateflag)
         {
         // If incoming adjusted day is not at the end of the month,
         // don't adjust the resulting day to the end of the month
         if (!LastDayPrevMonth)
           { 
            // If the final date is in Feb.
            // we may need to adjust, return if not Feb.
            if ((month == 2) && (day > 28))  
              {}
            else
              return 0;
           };
         };

      day = (char) daysInMonth[month];
      if ((month == 2) && (day > 28) &&
         (((year % 4) != 0) ||
         (((year % 100) == 0) && ((year % 400) != 0))))
         day = 28;
  
      datetimeOpData -= sizeof(day);
      *datetimeOpData = day;

      } // if (intervalFlag)

  return 0;
}

// returns 0 if valid time, -1 if not
// validates hour, minute, second
short ExpDatetime::validateTime(const char *datetimeOpData)
{
  int hour = datetimeOpData[0];
  if ((hour < 0) || (hour > 23))
    return -1;

  int minute = datetimeOpData[1];
  if ((minute < 0) || (minute > 59))
    return -1;

  int second = datetimeOpData[2];
  if ((second < 0) || (second > 59))
    return -1;

  return 0;
}

// compDatetimes() ===================================================
// This method compares two datetime values of the same subtype and
// returns a value indicating if the first value is less than (-1),
// equal to (0), or greater than (+1) the second value.
//
// This method has been modified as part of the MP Datetime
// Compatibility project.  It has been made more generic so that it is
// based on the start and end fields of the datetime type.
// ===================================================================
short ExpDatetime::compDatetimes(char *datetimeOpData1,
                                 char *datetimeOpData2)
{
  rec_datetime_field startField;
  rec_datetime_field endField;

  // Get the start and end fields for this Datetime type.
  //
  getDatetimeFields(getPrecision(),
                    startField,
                    endField);

  // Compare each field from highest significance (YEAR) to least
  // significance (FRACTION).  The format is: YYMDHMSFFFF
  //
  for (Int32 field = startField; field <= endField; field++) {
    switch (field) {
    case REC_DATE_YEAR:
      {
        short year1;
        short year2;

        // Don't know if the datetimeOpData{1,2} pointers point to
        // aligned data, so use str_cpy_all.
        //
        str_cpy_all((char *) &year1, datetimeOpData1, sizeof(year1));
        str_cpy_all((char *) &year2, datetimeOpData2, sizeof(year2));

        if (year1 < year2)
          return -1;
        if (year1 > year2)
          return 1;

        datetimeOpData1 += sizeof(year1);
        datetimeOpData2 += sizeof(year2);
        break;
      }
    case REC_DATE_MONTH:
    case REC_DATE_DAY:
    case REC_DATE_HOUR:
    case REC_DATE_MINUTE:
      {
        // 1 byte field comparisons
        //
        if (*datetimeOpData1 < *datetimeOpData2)
          return -1;
        if (*datetimeOpData1 > *datetimeOpData2)
          return 1;

        datetimeOpData1++;
        datetimeOpData2++;
        break;
      }
    case REC_DATE_SECOND:
      {
        // second comparison is also one byte, but has an optional
        // trailing fractional precision
        //
        if (*datetimeOpData1 < *datetimeOpData2)
          return -1;
        if (*datetimeOpData1 > *datetimeOpData2)
          return 1;

        datetimeOpData1++;
        datetimeOpData2++;

        // check for fractional part
        if (getScale() > 0)
          {
            Lng32 fraction1;
            Lng32 fraction2;

            // Don't know if the datetimeOpData{1,2} pointers point to
            // aligned data, so use str_cpy_all.
            //
            str_cpy_all((char *) &fraction1, datetimeOpData1, sizeof(fraction1));
            str_cpy_all((char *) &fraction2, datetimeOpData2, sizeof(fraction2));

            if (fraction1 < fraction2)
              return -1;
            if (fraction1 > fraction2)
              return 1;
          }
        break;
      }
    default:
      // Should never get here.
      //
      assert(0);
      break;
    } // switch ...
  } // for ...

  // All fields compare equal.  Therefore, the values are equal.
  //
  return 0;
}

// arithDatetimeInterval() ===========================================
// This method is used to add or subtract an interval value to/from a
// datetime value.  To perform the arithmetic operation, the following
// steps will be taken:
//
// - The datetime value will have been extended to contain a YEAR
// field if needed (see BiArith::preCodeGen()).  The value will need
// to be extended if it contains a DAY field.  This is necessary since
// with the introduction of non-standard SQL/MP datetime types, it is
// possible to have a datetime value which has a DAY field but not a
// YEAR or not a MONTH field.  In this situation, it is not possible
// to define a meaningful way to do the operation.  Does the DAY field
// wrap at 30, 31, 28, or 29.  So to make this operation meaningful,
// the value is extended to the current timestamp.
//
// - Ensure that this is a valid datetime value (use validateDate).
// For example, the datetime value "DATETIME 02-29 MONTH TO DAY"
// extended to the current timestamp in a non-leap year would result
// in an invalid timestamp.  Presumably, this check should have been
// done before calling this method.
//
// - Convert the given datetime value to an interval scaled to
// match the given interval value (use convertDatetimeToInterval)
//
// - Perform the arithmetic operation on the two interval values (they
// are integer values).
//
// - Convert the resulting value back to a datetime value (use
// convertIntervalToDatetime).
//
// - If necessary, validate the resulting datetime value.  It will be
// necessary if the interval being added/subtract to the datetime
// value is in units of months or years and if the original datetime
// value contained a day field.
//
// - Extract the desired fields from the resulting datetime value as
// the final result (use extractDatetime).  The desired fields are the
// range of fields of the original datetime value.
//
// This method has been modified as part of the MP Datetime
// Compatibility project.  It has been modified to handle non-standard
// datetime types.
// ====================================================================
//
short
ExpDatetime::arithDatetimeInterval(arithOps operation,
                                   ExpDatetime *datetimeOpType,
                                   Attributes *intervalOpType,
                                   char *datetimeOpData,
                                   char *intervalOpData,
                                   char *resultData,
                                   CollHeap *heap,
                                   ComDiagsArea** diagsArea)
{
  short rc = 0;

  if (operation != DATETIME_ADD && 
      operation != DATETIME_SUB) {
    ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
    return -1;
  }

  rec_datetime_field datetimeStartField;
  rec_datetime_field datetimeEndField;

  NABoolean LastDayPrevMonth = FALSE;

  // Get the start and end fields of the given Datetime type.
  //
  if (getDatetimeFields(datetimeOpType->getPrecision(),
                        datetimeStartField,
                        datetimeEndField) != 0) {
    ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
    return -1;
  }

  rec_datetime_field intervalEndField;

  // Get the end field of the given Interval type.
  //
  if (ExpInterval::getIntervalEndField(intervalOpType->getDatatype(),
                       intervalEndField) != 0) {
    ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
    return -1;
  }

  // Make sure that the given datetime value has enough fields to
  // perform this operation.  If there is a DAY field, there also has
  // to be as YEAR field.
  //
  if (datetimeStartField <= REC_DATE_DAY &&
      datetimeEndField >= REC_DATE_DAY &&
      datetimeStartField > REC_DATE_YEAR) {
    ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
    return -1;
  }

  // Copy the given datetime value to a local buffer.
  // This will contain the result of the operation, but may 
  // have more fields that the destination.
  //
  char dateTimeValue[MAX_DATETIME_SIZE];

  // Copy given datetime value to a local buffer.
  //
  if (copyDatetimeFields(datetimeStartField,
                         datetimeEndField,
                         datetimeOpType->getScale(),
                         datetimeOpType->getScale(),
                         datetimeOpData,
                         dateTimeValue,
                         MAX_DATETIME_SIZE,
                         NULL) != 0) {
    ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
    return -1;
  }

  // Ensure that the given value is a valid datetime value.  For
  // example, the datetime value "DATETIME 02-29 MONTH TO DAY"
  // extended with the current timestamp in a non-leap year would
  // result in an invalid date. (This check presumably is also done by
  // the code which performed the extension).
  //
  if ((datetimeStartField == REC_DATE_YEAR) &&
      (datetimeEndField >= REC_DATE_DAY) &&
      (validateDate(REC_DATE_YEAR, REC_DATE_DAY, 
                    dateTimeValue, NULL, FALSE, 
                    LastDayPrevMonth) != 0)) {
    ExRaiseSqlError(heap, diagsArea, EXE_DATETIME_FIELD_OVERFLOW);
    return -1;
  }

  // Convert the datetime value to an interval with the same precision
  // (endField) as the Interval operand.  We want to make sure we
  // add/subtract MONTHs to MONTHs, etc.
  //
  Int64 value = -1;
  char intervalBignum[BigNum::BIGNUM_TEMP_LEN];
  char resultBignum[BigNum::BIGNUM_TEMP_LEN];
  NABoolean isBignum = FALSE;
  convertDatetimeToInterval(datetimeStartField,
                            datetimeEndField,
                            datetimeOpType->getScale(),
                            intervalEndField,
                            dateTimeValue,
                            value,
                            intervalBignum,
                            isBignum);

  // Perform the arithmetic operation.
  //
  Int64 interval64 = 0;
  switch (intervalOpType->getLength()) {
  case SQL_SMALL_SIZE: {
    short interval;
    str_cpy_all((char *) &interval, intervalOpData, sizeof(interval));
    interval64 = interval;
    break;
  }
  case SQL_INT_SIZE: {
    Lng32 interval;
    str_cpy_all((char *) &interval, intervalOpData, sizeof(interval));
    interval64 = interval;
    break;
  }
  case SQL_LARGE_SIZE: {
    Int64 interval;
    str_cpy_all((char *) &interval, intervalOpData, sizeof(interval));
    interval64 = interval;
    break;
  }
  default:
    ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
    return -1;
  }

  if (NOT isBignum) { // result is not a bignum  
    value += ((operation == DATETIME_ADD) ? interval64 : -interval64);
  } else {
    // result is a bignum
    char *op_data[3];
    
    BigNum op1BN(BigNum::BIGNUM_TEMP_LEN, BigNum::BIGNUM_TEMP_PRECISION, 0, 0);
    SimpleType intST(REC_BIN64_SIGNED, sizeof(Int64), 0, 0,
                     ExpTupleDesc::SQLMX_FORMAT,
                     8, 0, 0, 0, Attributes::NO_DEFAULT, 0);

    op_data[0] = resultBignum;
    op_data[1] = intervalBignum;
    op_data[2] = (char*)&interval64;
    
    if (operation == DATETIME_ADD) {
      rc = EXP_FIXED_BIGN_OV_ADD(&op1BN, &intST, op_data);
    } else {
      rc = EXP_FIXED_BIGN_OV_SUB(&op1BN, &intST, op_data);
    }
  }

  // Underflow is ok for time only datetime types.  Arithmetic on the
  // hour field is computed modulo 24.  For datetime types containing
  // a date portion, underflow is an error.
  //
  if ((NOT isBignum) && (value < 0) && 
      (datetimeStartField < REC_DATE_HOUR)) {
    ExRaiseSqlError(heap, diagsArea, EXE_DATETIME_FIELD_OVERFLOW);
    return -1;
  }

  // Convert result back to datetime.  Note that this is overlaying the
  // local copy of the datetime value.
  //
  if (convertIntervalToDatetime(value,
                                (isBignum ?  resultBignum : NULL),
                                datetimeStartField,
                                intervalEndField,
                                datetimeOpType->getScale(),
                                dateTimeValue) != 0) {
    ExRaiseSqlError(heap, diagsArea, EXE_DATETIME_FIELD_OVERFLOW);
    return -1;
  }

  // If we are adding/subtracting in units of MONTHS or YEARS, and the
  // original datetime value contains a DAY field, then the result
  // could be invalid.  For example:
  //
  //   DATETIME '1-31' MONTH TO DAY + INTERVAL '1' MONTH -> ERROR
  //
  //   DATETIME '1' MONTH + INTERVAL '1' MONTH -> DATETIME '2' MONTH
  //   (even if the current date is 2000-1-31)
  //

  if ((intervalEndField <= REC_DATE_MONTH) &&
      (datetimeStartField <= REC_DATE_DAY) &&
      (datetimeEndField >= REC_DATE_DAY) &&
      (validateDate(datetimeStartField,
                    REC_DATE_DAY,
                    dateTimeValue, this, TRUE, LastDayPrevMonth) != 0)) {
    ExRaiseSqlError(heap, diagsArea, EXE_DATETIME_FIELD_OVERFLOW);
    return -1;
  }

  // Extract the result from the datetime value. The result fields
  // could be different from the fields of the given datetime value.
  //
  if (extractDatetime(datetimeStartField,
                      datetimeEndField,
                      datetimeOpType->getScale(),
                      dateTimeValue,
                      resultData) != 0) {
    ExRaiseSqlError(heap, diagsArea, EXE_DATETIME_FIELD_OVERFLOW);
    return -1;
  }

  return 0;
}

short ExpDatetime::subDatetimeDatetime(Attributes *datetimeOpType,
                                       Attributes *intervalOpType,
                                       char *datetimeOpData1,
                                       char *datetimeOpData2,
                                       char *resultData,
                                       CollHeap *heap,
                                       ComDiagsArea** diagsArea) const
{
  short rc = 0;

  rec_datetime_field datetimeStartField;
  rec_datetime_field datetimeEndField;
  if (getDatetimeFields(datetimeOpType->getPrecision(),
                        datetimeStartField,
                        datetimeEndField) != 0) {
    ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
    return -1;
  }
  rec_datetime_field intervalEndField;
  if (DFS2REC::isInterval(intervalOpType->getDatatype()))
    {
      if (ExpInterval::getIntervalEndField(intervalOpType->getDatatype(),
                                           intervalEndField) != 0) {
        ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
        return -1;
      }
    }
  else if ((DFS2REC::isBinaryNumeric(intervalOpType->getDatatype())) &&
           (datetimeEndField == REC_DATE_DAY))
    {
      // special modeSpecial4 case.
      // DATE - DATE with result as NUMBER.
      intervalEndField = REC_DATE_DAY;
    }
  else
    {
      ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
      return -1;
    }

  Int64 value1;
  char intervalBignum1[BigNum::BIGNUM_TEMP_LEN];
  NABoolean isBignum1 = FALSE;
  NABoolean isBignum2 = FALSE;
  convertDatetimeToInterval(datetimeStartField,
                            datetimeEndField,
                            datetimeOpType->getScale(),
                            intervalEndField,
                            datetimeOpData1,
                            value1,
                            intervalBignum1,
                            isBignum1);

  Int64 value2;
  char intervalBignum2[BigNum::BIGNUM_TEMP_LEN];
  convertDatetimeToInterval(datetimeStartField,
                            datetimeEndField,
                            datetimeOpType->getScale(),
                            intervalEndField,
                            datetimeOpData2,
                            value2,
                            intervalBignum2,
                            isBignum2);

  Int64 result = 0;

  if ((NOT isBignum1) && (NOT isBignum2)) // neither is bignum
    result = value1 - value2;
  else
    {
      BigNum opBN(BigNum::BIGNUM_TEMP_LEN, BigNum::BIGNUM_TEMP_PRECISION, 0, 0);
      SimpleType opST(REC_BIN64_SIGNED, sizeof(Int64), 0, 0,
                      ExpTupleDesc::SQLMX_FORMAT,
                      8, 0, 0, 0, Attributes::NO_DEFAULT, 0);

      char *op_data[3];

      char resultBN[BigNum::BIGNUM_TEMP_LEN];
      op_data[0] = resultBN;
      op_data[1] = (isBignum1 ? intervalBignum1 : (char*)&value1);
      op_data[2] = (isBignum2 ? intervalBignum2 : (char*)&value2);
     
      rc = EXP_FIXED_BIGN_OV_SUB((isBignum1 ? (Attributes*)&opBN : (Attributes*)&opST), 
                                 (isBignum2 ? (Attributes*)&opBN : (Attributes*)&opST), 
                                 op_data); 
      if (rc)
        {
          ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
          return -1;
        }

      rc = convDoIt(op_data[0], BigNum::BIGNUM_TEMP_LEN, REC_NUM_BIG_SIGNED, BigNum::BIGNUM_TEMP_PRECISION, 0,
                    (char*)&result,  8, REC_BIN64_SIGNED, 0, 0,
                    NULL, 0, NULL, NULL);
      if (rc)
        {
          // convert interval value to ascii format
          char invalidVal[BigNum::BIGNUM_TEMP_PRECISION+1];
          Int32 len = BigNum::BIGNUM_TEMP_PRECISION;
          memset(invalidVal, ' ', len);
          convDoIt(op_data[0], BigNum::BIGNUM_TEMP_LEN, REC_NUM_BIG_SIGNED, BigNum::BIGNUM_TEMP_PRECISION, 0,
                   invalidVal, BigNum::BIGNUM_TEMP_PRECISION, REC_BYTE_F_ASCII, 0, 0,
                   NULL, 0, NULL, NULL);
          len--;
          while (invalidVal[len] == ' ')
            len--;
          len++;
          invalidVal[len]  = 0;

          ExRaiseSqlError(heap, diagsArea, EXE_INVALID_INTERVAL_RESULT,
                          NULL, NULL, NULL, NULL,
                          invalidVal);
          return -1;
        }
    }

  //
  // Scale the result to the interval qualifier's fractional precision.
  //
  if (intervalEndField == REC_DATE_SECOND) {
    short fpDiff = intervalOpType->getScale() - datetimeOpType->getScale();
    if (fpDiff > 0) {
      do {
        result *= 10;
      } while (--fpDiff > 0);
    } else {
      while (fpDiff < 0) {
        result /= 10;
        fpDiff++;
      }
    }
  }
  switch (intervalOpType->getLength()) {
  case SQL_SMALL_SIZE: {
    short interval = (short) int64ToInt32(result);
    str_cpy_all(resultData, (char*) &interval, sizeof(interval));
    break;
  }
  case SQL_INT_SIZE: {
    Lng32 interval = int64ToInt32(result);
    str_cpy_all(resultData, (char*) &interval, sizeof(interval));
    break;
  }
  case SQL_LARGE_SIZE:
    str_cpy_all(resultData, (char*) &result, sizeof(result));
    break;
  default:
    ExRaiseSqlError(heap, diagsArea, EXE_INTERNAL_ERROR);
    return -1;
  }
  return 0;
};

// scaleFraction() ===================================================
// This static helper function to the ExpDatetime class is used to
// scale the fraction value from one precision to another.  If the
// scaling results in loss of data, the roundedDownFlag, if provided,
// is set to TRUE.
//
// This function was added as part of the MP Datetime Compatibility
// project.
// ===================================================================
//
static Lng32
scaleFraction(Int32 srcFractPrec,
              Lng32 srcFraction,
              Int32 dstFractPrec,
              NABoolean *roundedDownFlag = NULL)
{

  // The roundedDownFlag indicates if any data is lost when scaling
  // the fraction field of the datetime value.
  //
  if (roundedDownFlag)
    *roundedDownFlag = FALSE; // assume success

  Lng32 fraction = 0;
 
  // If there is a fraction value in the destination and there is a
  // fraction value in the source, scale the fraction to the
  // destination precision.
  //
  if (dstFractPrec >= 0 && srcFractPrec > 0) {

    fraction = srcFraction;

    if (srcFractPrec < dstFractPrec) {

      Lng32 multiplier = powersOfTen[dstFractPrec - srcFractPrec];
      fraction *= multiplier;

    } else if (srcFractPrec > dstFractPrec) {

      Lng32 divisor = powersOfTen[srcFractPrec - dstFractPrec];
      fraction /= divisor;

      if (roundedDownFlag && ((fraction * divisor) < srcFraction)) {
          *roundedDownFlag = TRUE;
      }
    }

    // If the two precisions are equal, no scaling is required.
    //
  }
  return fraction;
}

// currentTimeStamp() ================================================
// This static method populates the given buffer (char *) with the
// value of the current timestamp in the format of an ExpDatetime
// value (i.e., 2-1-1-1-1-1-4 for YEAR through FRACTION).  This method
// can be used anywhere the current timestamp is needed in this
// form. For instance ex_function_current::eval() in
// exp/exp_function.cpp.
//
// The result is returned in the buffer pointed to by the parameter
// 'resultData'.  This buffer must be allocated by the caller and it
// must be large enough to hold a complete timestamp (11 bytes).
//
// This method was added as part of the MP Datetime Compatibility
// project.  The code for the method came from
// ex_function_current::eval() in exp_function.cpp
// ===================================================================
short
ExpDatetime::currentTimeStamp(char *dstData,
                              rec_datetime_field startField,
                              rec_datetime_field endField,
                              short fractPrec)
{
  // The Timestamp fields decared to have the same size as the
  // corresponding field of the ExpDatetime format (YYMDHMSFFFF).
  //
  short year;
  char month;
  char day;
  char hour;
  char minute;
  char second;
  Lng32 fraction;

  // Get the value of the current timestamp.
  //
  Int64 juliantimestamp = CONVERTTIMESTAMP(JULIANTIMESTAMP(0,0,0,-1),0,-1,0);

  // Convert the timestamp into an array of shorts with each element
  // containing the value of one field of the timestamp.
  //
  short timestamp[8];
  INTERPRETTIMESTAMP(juliantimestamp, timestamp);

  // Extract the fields into local variables which correspond in size
  // to the fields of the ExpDatetime format (YYMDHMSFFFF).
  //
  year = timestamp[0];
  month = (char) timestamp[1];
  day = (char) timestamp[2];
  hour = (char) timestamp[3];
  minute = (char) timestamp[4];
  second = (char) timestamp[5];
  fraction = timestamp[6] * 1000 + timestamp[7];

  // Copy all the fields to the result value.
  //
  for (Int32 field = startField; field <= endField; field++) {

    switch (field) {
    case REC_DATE_YEAR:
      str_cpy_all(dstData, (char *) &year, sizeof(year));
      dstData += sizeof(year);
      break;
    case REC_DATE_MONTH:
      *dstData++ = month;
      break;
    case REC_DATE_DAY:
      *dstData++ = day;
      break;
    case REC_DATE_HOUR:
      *dstData++ = hour;
      break;
    case REC_DATE_MINUTE:
      *dstData++ = minute;
      break;
    case REC_DATE_SECOND:
      *dstData++ = second;

      // If there is a fractional precision in the destination,
      // copy and scale it from the source.
      //
      if (fractPrec > 0) {

        fraction = scaleFraction(MAX_DATETIME_FRACT_PREC, fraction,fractPrec);
        str_cpy_all(dstData, (char *) &fraction, sizeof(fraction));
      }
      break;
    default:
      return -1;
    }
  }

  return 0;
}

// sizeofDatetimeFields() ==============================================
// This static helper function for the ExpDatetime class is used to
// determine the size in number of bytes, of a range of datetime fields.
//
// This function was added as part of the MP Datetime Compatibility
// project.
// =====================================================================
//
static Lng32
sizeofDatetimeFields(rec_datetime_field startField,
                     rec_datetime_field endField,
                     short fractPrec)
{
  Lng32 sizeInBytes = 0;

  if (startField <= endField) {
    
    // All fields have at least one byte.
    //
    sizeInBytes = endField - startField + 1;
    
    // The YEAR field has two bytes, so add an extra.
    //
    if(startField == REC_DATE_YEAR) {
      sizeInBytes++;
    }

    // The FRACTION field has 4 bytes, so if it is present add 4.
    //
    if(endField == REC_DATE_SECOND && fractPrec > 0) {
      sizeInBytes += sizeof(Lng32);
    }
  }

  return sizeInBytes;
}

// copyDatetimeFields() ==============================================
// This static helper function for the ExpDatetime class is used to
// copy a set of fields from one datetime value to another.  If
// present, the fraction value is scaled to match the destination
// before coping.
//
// This function was added as part of the MP Datetime Compatibility
// project.
// =====================================================================
//
static short
copyDatetimeFields(rec_datetime_field startField,
                   rec_datetime_field endField,
                   short srcFractPrec,
                   short dstFractPrec,
                   char *srcData,
                   char *dstData,
                   Lng32 dstLen,
                   NABoolean *roundedDownFlag)
{

  // get the size of all the fields, excluding the fraction field if
  // present.
  //
  Lng32 size = sizeofDatetimeFields(startField, endField, 0);

  // Copy all the fields, excluding the fraction field.
  //
  str_cpy_all(dstData, srcData, size);

  dstData += size;
  srcData += size;

  // If there is a fractional precision in the destination,
  // copy and scale the fraction from the source.
  //
  if (endField == REC_DATE_SECOND && dstFractPrec >= 0) {
    Lng32 fraction = 0;

    // If there is a fraction precision in the source datetime
    // value, scale it to the precision of the destination
    // datetime If no fractional precision is given, use the
    // default value of 0.
    //
    if (srcFractPrec > 0) {
      str_cpy_all((char *)&fraction, srcData, sizeof(fraction));
      
      fraction = scaleFraction(srcFractPrec,
                               fraction,
                               dstFractPrec,
                               roundedDownFlag);
    }
    // if destination has space for fraction, copy it.
    if ((dstLen > 0) && (dstLen >= (size + sizeof(fraction))))
      str_cpy_all(dstData, (char *) &fraction, sizeof(fraction));
  }
  return 0;
}

// minimumTimeStamp() ================================================
// This static helper function for the ExpDatetime class is used to
// construct a minimum datetime value for the given start and end
// fields.
//
// The Minimum timestamp is:
//
//   0001-01-01 00:00:00.000000
//
// This function was added as part of the MP Datetime Compatibility
// project.
// =====================================================================
//
static short
minimumTimeStamp(char *dstData,
                 rec_datetime_field dstStartField,
                 rec_datetime_field dstEndField,
                 short dstFractPrec)
{

  for (Int32 field = dstStartField; field <= dstEndField; field++) {
    switch (field) {
    case REC_DATE_YEAR:
      {
        short year = 1;
        str_cpy_all(dstData, (char *)&year, sizeof(year));
        dstData += sizeof(year);
        break;
      }
    case REC_DATE_MONTH:
    case REC_DATE_DAY:
      *dstData++ = (char) 1;
      break;
    case REC_DATE_HOUR:
    case REC_DATE_MINUTE:
      *dstData++ = (char) 0;
      break;
    case REC_DATE_SECOND:
      *dstData++ = (char) 0;
      if (dstFractPrec) {
        Lng32 fraction = 0;
        str_cpy_all(dstData, (char *)&fraction, sizeof(Lng32));
        dstData += sizeof(Lng32);
      }
      break;
    default:
      return -1;
    }
  }
  return 0;
}

// convDatetimeDatetime() ============================================
// This method is used to convert the given datetime value to another
// datetime value.  Missing leading fields are handled by
// Cast::preCodeGen() this method should never see them.  Missing
// trailing fields are padded with their minimum value.  This method
// will be used by convDatetimeDatetime (in exp_conv.cpp)
//
// The result is returned in the buffer pointed to by the parameter
// 'dstData'.  This buffer must be allocated by the caller and it
// must be large enough to hold the result.
//
// This method was added as part of the MP Datetime Compatibility
// project.
// =====================================================================
//
short
ExpDatetime::convDatetimeDatetime(char *srcData,
                                  rec_datetime_field dstStartField,
                                  rec_datetime_field dstEndField,
                                  short dstFractPrec,
                                  char *dstData,
                                  Lng32 dstLen,
                                  short validateFlag,
                                  NABoolean *roundedDownFlag)
{

  rec_datetime_field srcStartField;
  rec_datetime_field srcEndField;
  
  rec_datetime_field endField;
  NABoolean LastDayPrevMonth = FALSE;

  // Get the start and end fields for the source datetime type.
  //
  if (getDatetimeFields(getPrecision(),
                        srcStartField,
                        srcEndField) != 0) {
    return -1;
  }

  // Are there any missing leading fields or do the src and dst fields
  // not overlap?  If so, this is an error.  (missing leading fields
  // are handled by Cast::preCodeGen()).
  //
  if (dstStartField < srcStartField ||
      srcEndField < dstStartField ||
      dstEndField < srcStartField) {
    return -1;
  }

  // Skip over source fields that are not in the destination.
  //
  srcData += sizeofDatetimeFields(srcStartField, 
                                  (rec_datetime_field)(dstStartField - 1),
                                  getScale());
  srcStartField = dstStartField;
  endField = srcEndField;
  if (endField > dstEndField) {
    endField = dstEndField;
  }

  // Copy required source fields to destination.
  //
  if (copyDatetimeFields(dstStartField,
                         endField,
                         getScale(),
                         dstFractPrec,
                         srcData,
                         dstData,
                         dstLen,
                         roundedDownFlag) != 0) {
    return -1;
  }

  if (validateFlag && validateDate(dstStartField, endField, 
                                   dstData, NULL, FALSE,
                                   LastDayPrevMonth)) 
    return -1;
 
  // Skip over copied source fields.
  //
  dstData += sizeofDatetimeFields(dstStartField, endField, dstFractPrec);
  dstStartField = (rec_datetime_field)(endField + 1);
  
  // If we still have more fields in the destination, pad them with
  // the minimum timestamp.
  //
  if (dstStartField <= dstEndField) {
    if (minimumTimeStamp(dstData,
                         dstStartField,
                         dstEndField,
                         dstFractPrec) != 0) {
      return -1;
    }
  }

  return 0;
}

// extractDatetime ===================================================
// This method is used to extract a given range of fields from a
// timestamp value.  This method is used by
// ExpDatetime::addDatetimeInterval and ExpDatetime::subDatetimeInterval.
//
// This method was added as part of the MP Datetime Compatibility
// project.
// ===================================================================
//
short
ExpDatetime::extractDatetime(rec_datetime_field srcStartField,
                             rec_datetime_field srcEndField,
                             short srcFractPrec,
                             char *srcData,
                             char *dstData)
{

  rec_datetime_field dstStartField;
  rec_datetime_field dstEndField;

  // Get the start and end fields for the destination datetime type.
  //
  if (getDatetimeFields(getPrecision(),
                        dstStartField,
                        dstEndField) != 0) {
    return -1;
  }

  if (srcStartField > dstStartField || srcEndField < dstEndField) {
    return -1;
  }

  srcData += sizeofDatetimeFields(srcStartField, 
                                  (rec_datetime_field)(dstStartField - 1),
                                  srcFractPrec);
  
  if (copyDatetimeFields(dstStartField,
                         dstEndField,
                         srcFractPrec,
                         getScale(),
                         srcData,
                         dstData,
                         getLength(),
                         NULL) != 0) {
    return -1;
  }
  
  return 0;
}

// determineFormat() =================================================
// This static helper function for the ExpDatetime class is used to
// determine the datetime format (DEFAULT, USA, or EUROPEAN) of an
// ASCII string.
//
// The ASCII string can be in one of three formats:
//
//  Default : yyyy-mm-dd hh:mm:ss.msssss
//  USA     : mm/dd/yyyy hh:mm:ss.msssss [am|pm]
//  European: dd.mm.yyyy hh.mm.ss.msssss
//
// There are some variations on the formats above:
//   - the delimiter between the date and the time portion
//     can be either a ' ' (space) or a ':'
//   - the [am|pm] in the USA format is case insensitive.
//   - any range of consectutive (YEAR to SECOND) fields may
//     be present.
//
// This function was added as part of the MP Datetime Compatibility
// project.
// =====================================================================
//
static
ExpDatetime::DatetimeFormats
determineFormat(char *src,
                rec_datetime_field startField,
                rec_datetime_field endField)
{

  // Find the first non-digit character.  This is the first delimiter
  //
  const char* s = src;
  Int32 firstFieldSize = 0;
  while (isDigit8859_1(*s)) {
    s++;
    firstFieldSize++;
  }

  if ((startField == endField) ||
      (startField == REC_DATE_DAY && endField == REC_DATE_HOUR)) {

    // In this case there is nothing to distinguish the different
    // formats.  Any of them could be used.  Use USA to allow for a
    // possible AM/PM
    //
    return ExpDatetime::DATETIME_FORMAT_USA;

  } else if (((startField == REC_DATE_YEAR) && (endField > REC_DATE_YEAR)) ||
             ((startField < REC_DATE_DAY) && (endField >= REC_DATE_DAY))) {

    // In this case there is a delimiter in the DATE portion of the
    // DATETIME value.  The delimiter will determine the format.
    //
    switch(*s) {
    case '-':
      return ExpDatetime::DATETIME_FORMAT_DEFAULT;
    case '/':
      {
	if (firstFieldSize == 2)
	  return ExpDatetime::DATETIME_FORMAT_USA;
	else
	  return ExpDatetime::DATETIME_FORMAT_ERROR;
      }
    case '.':
      return ExpDatetime::DATETIME_FORMAT_EUROPEAN;
    default:
      return ExpDatetime::DATETIME_FORMAT_ERROR;
    }
  } else if (((startField <= REC_DATE_HOUR) && (endField > REC_DATE_HOUR)) ||
             ((startField < REC_DATE_SECOND) && (endField == REC_DATE_SECOND))) {
    // In this case there is no delimiter in the DATE portion, but
    // there is one in the TIME portion of the DATETIME value.  This
    // delimiter will determine the format.
    //

    // If the is a delimiter before the TIME portion, go to the next
    // delimiter.  This can happen if the DATE portion only has a DAY
    // field.
    //
    if (startField == REC_DATE_DAY) {
      s++;
      while (isDigit8859_1(*s)) {
        s++;
      }
    }

    // : could be either USA or DEFAULT.  Use USA to allow for a
    // possible AM/PM.
    //
    switch(*s) {
    case ':' :
      return ExpDatetime::DATETIME_FORMAT_USA;
    case '.' :
      return ExpDatetime::DATETIME_FORMAT_EUROPEAN;
    default:
      return ExpDatetime::DATETIME_FORMAT_ERROR;
    }
  }
  
  // Any other delimiter is an error.
  //
  return ExpDatetime::DATETIME_FORMAT_ERROR;
}

// containsField() ===================================================
// This static helper function for the ExpDatetime class is used to
// determine if a given field is contained within the range of the
// given start and end fields.
//
// This function was added as part of the MP Datetime Compatibility
// project.
// =====================================================================
//
static
NABoolean
containsField(rec_datetime_field field,
              rec_datetime_field startField,
              rec_datetime_field endField)
{
  // The field is contained within the range if it is bounded by the
  // start and end fields OR if the field is FRACTION and the range
  // contains a SECOND field.  In the latter case, the FRACTION field
  // is optional, but this function still returns TRUE.
  //
  if ((field >= startField && field <= endField) ||
      ((endField == REC_DATE_SECOND) && (field == REC_DATE_FRACTION_MP))) {
    return TRUE;
  } else {
    return FALSE;
  }
}

// scanField() =======================================================
// This static helper function for the ExpDatetime class is used to
// scan the various fields of a ASCII datetime value.
//
// parameters:
//
// src - IN/OUT
//       Pointer to field of the ASCII datetime value.  As an input,
//       it should point to the delimiter before the field to be
//       scanned.  If no delimiter is expected, it should point to the
//       field to be scanned. As an output, upon return, this
//       parameter will point to the character just beyond this field,
//       or the first non-digit character encounter while scanning the
//       field.
//
// srcEnd - IN
//       Pointer to the character just beyond the end of the datetime
//       value.  This is required since the datetime value will not
//       always be NULL terminated.
//
// field - IN
//       The type of field to be scanned.
//
// exptDelim - IN
//       The expected leading delimiter. The NULL character if a
//       delimiter is not expected.
//
// fractPrec - IN
//       The fractional precision of the destination.  Used to perform
//       scaling of the sources fraction.
//
// value - OUT
//       The numeric value of the scanned field
//
// heap and diagsArea
//       Used to report some errors while scanning.
//
// return - OUT
//       returns FALSE if an error occurred while scanning, TRUE
//       otherwise.
//
// This function was added as part of the MP Datetime Compatibility
// project.
// =====================================================================
//
static
NABoolean
scanField(char *&src,
          char *srcEnd,
          rec_datetime_field field,
          char exptDelim,
          Lng32 &fractPrec,
          Lng32 &value,
          CollHeap *heap,
          ComDiagsArea** diagsArea,
	  ULng32 flags) 
{
  NABoolean noDatetimeValidation = (flags & CONV_NO_DATETIME_VALIDATION) != 0;
  NABoolean noHadoopDateFix = (flags & CONV_NO_HADOOP_DATE_FIX) != 0;

  // The maximum lengths of the various fields.  Since the value of
  // REC_DATE_YEAR is 1, the first entry is just a place holder.
  //
  static const Lng32 maxLens[] =  { 0,    4,  2,  2,  2,  2,  2,         9 };
  static const Lng32 minValue[] = { 0, 0001, 01, 01, 00, 00, 00, 000000000 };
  static const Lng32 maxValue[] = { 0, 9999, 12, 31, 23, 59, 59, 999999999 };

  // The length of the scanned field.
  //
  short len = 0;

  // The numeric value of the scanned field.
  //
  value = 0;

  // The leading delimiter in the source string.
  //
  char delim;

  // If we are expecting a leading delimiter, make sure the actual
  // delimiter matches the expected.
  //
  if (exptDelim) {

    // Get the actual delimiter
    //
    if (src < srcEnd && *src) {
      delim = *src++;
    } else {
      delim = '\0';
    }

    // Check actual delimiter against the expected delimiter. 
    // An expected delimiter of ';' matches a ':' or a ' ' (space).
    // An expected delimiter of '|' matches a ':' or a ' ' (space) or 'T' (ISO format).
    // A NULL actual delimiter matches any expected delimiter.
    //
    if (delim) {
      if (exptDelim == '-' ||
          exptDelim == '/' ||
          exptDelim == '.' ||
          exptDelim == ':' ||
          exptDelim == ' ') {
        if (delim != exptDelim) {
          return FALSE;
        }
      } else if (exptDelim == ';') {
        if (delim != ':' && delim != ' ') {
          return FALSE;
        }
      } else if (exptDelim == '|') {
        if (delim != ':' && delim != ' ' && delim != 'T') {
          return FALSE;
        }
      } else {
        return FALSE;
      }
    }
  }

  
  // Convert the string of digits into a numeric value.  Stop scanning
  // when the max. number of digits for this field have been scanned
  // or if a non-digit is encountered or if the end of the source was
  // reached
  //

  for (len = 0;
       len < maxLens[field] && src < srcEnd && isDigit8859_1(*src);
       len++) {
    value = (value * 10) + (*src++ - '0');
  }

  // Scale the fraction of the source to match the fractional
  // precision of the destination.
  //
  if (field == REC_DATE_FRACTION_MP) {
    value = scaleFraction(len, value, fractPrec);
    fractPrec = len;
  }

  // For all but the FRACTION field, datetime fields are required to
  // have the maximum length.
  //
  if (len < maxLens[field] && field != REC_DATE_FRACTION_MP) {

    if ((NOT noHadoopDateFix) &&
        (field >= REC_DATE_HOUR)) {
      // extend with zeroes
      value = 0;
    }
    else {
      // An unknown character was encountered in the string.
      //
      char hexstr[MAX_OFFENDING_SOURCE_DATA_DISPLAY_LEN];
      ExRaiseSqlError(heap, diagsArea, EXE_CONVERT_STRING_ERROR,NULL,NULL,NULL,NULL,stringToHex(hexstr, sizeof(hexstr), src, srcEnd-src));

      return FALSE;
    }
  } else if (src < srcEnd && isDigit8859_1(*src)) {
    return FALSE;
  }

  // If the value is too small or too large, the field is invalid.
  //
  if ((NOT noDatetimeValidation) &&
      (value < minValue[field] || value > maxValue[field])) {
    return FALSE;
  }

  return TRUE;
}

short
ExpDatetime::convAsciiToDatetime(char *srcData,
                                 Lng32 srcLen,
                                 char *dstData,
                                 Lng32 dstLen,
                                 rec_datetime_field dstStartField,
                                 rec_datetime_field dstEndField,
                                 Lng32 format,
                                 Lng32 &scale,
                                 CollHeap *heap,
                                 ComDiagsArea** diagsArea,
				 ULng32 flags)
{

  NABoolean noDatetimeValidation = (flags & CONV_NO_DATETIME_VALIDATION) != 0;
  Lng32 originalSrcLen = srcLen;
  char * originalSrcData = srcData; 
  // skip leading and trailing blanks and adjust srcData and srcLen
  // accordingly
  //
  NABoolean LastDayPrevMonth = FALSE;

  Lng32 i;
  for (i = 0; i < srcLen && *srcData == ' '; i++) {
    srcData++;
  }

  if (i == srcLen) {
    // string contains only blanks.
    //
    ExRaiseSqlError(heap, diagsArea, EXE_CONVERT_DATETIME_ERROR);
    if(*diagsArea != NULL)
      **diagsArea << DgString0("(string contains only blanks)");   
    return -1;
  };

  srcLen -= i;

  // we know that we found at least one non-blank character.
  // so we just decrement srcLen till we scanned all
  // trailing blanks
  //
  while (srcData[srcLen - 1] == ' ') {
    srcLen--;
  }

  // Indicates if an " AM" or " PM" strings appears at the end of the
  // source.
  // 0 - means no AM/PM indicator
  // 1 - means AM 
  // 2 - means PM
  //
  short usaAmPm = 0;

  // Check for the " AM" or " PM" at the end of the source.  If it is
  // there, record and adjust the srcLen to in effect remove it from
  // the source.
  //
  if ((srcData[srcLen - 3] == ' ') &&
      ((srcData[srcLen - 2] == 'A') || (srcData[srcLen - 2] == 'a')) &&
      ((srcData[srcLen - 1] == 'M') || (srcData[srcLen - 1] == 'm'))) {
    usaAmPm = 1;
    srcLen -= 3;
  } else if ((srcData[srcLen - 3] == ' ') &&
             ((srcData[srcLen - 2] == 'P') || (srcData[srcLen - 2] == 'p')) &&
             ((srcData[srcLen - 1] == 'M') || (srcData[srcLen - 1] == 'm'))) {
    usaAmPm = 2;
    srcLen -= 3;
  }    

  // Indicates if a 'Z' appears at the end of source.
  // 'Z' is a local timezone indicator for ISO format datetime values.
  // 0 - means no 'Z' indicator
  // 1 - means Z
  //
  short defZ = 0;

  // Check for the " AM" or " PM" at the end of the source.  If it is
  // there, record and adjust the srcLen to in effect remove it from
  // the source.
  //
  if ((usaAmPm == 0) && (srcData[srcLen - 1] == 'Z')) {
    defZ = 1;
    srcLen -= 1;
  } 

  char *src = srcData;
  char *srcEnd = srcData + srcLen;

  // Determine the format of the source string.
  //
  if (format == DATETIME_FORMAT_NONE)
    format = determineFormat(src, dstStartField, dstEndField);
  
  // If the format could not be determined, issue an error.
  //
  if (format == DATETIME_FORMAT_ERROR) {
    raiseDateConvErrorWithSrcData(originalSrcLen,diagsArea, originalSrcData, heap);
    return -1;
  }

  // check to see if they are timezone adjustment designator
  // as per ISO8601 datetime format.
  // TZD is of the form: +HH:MM pr -HH:MM
  //
  NABoolean TZD = FALSE;
  NABoolean isAdd = FALSE;
  Lng32 hh = 0;
  Lng32 mm = 0;
  Lng32 tzdSize = strlen("+HH:MM");
  char * tzd = NULL;
  if ((srcLen > tzdSize) &&
      (tzd = (srcEnd - tzdSize)) &&
      ((tzd[0] == '+') ||
       (tzd[0] == '-')) &&
      (tzd[3] == ':')) {
    hh = str_atoi(&tzd[1], strlen("HH"));
    mm = str_atoi(&tzd[4], strlen("MM"));
    
    if (tzd[0] == '+')
      isAdd = FALSE;
    else
      isAdd = TRUE;
    
    TZD = TRUE;
    
    srcLen -= tzdSize;
    srcEnd -= tzdSize;
  } // tzd specified

  // if timezone is specified and end field is not DAY, return error.
  if ((defZ || TZD) && (dstEndField == REC_DATE_DAY))
    {
      raiseDateConvErrorWithSrcData(originalSrcLen,diagsArea, originalSrcData, heap);
      return -1;
    }

  //  The order of the fields for the various formats.
  //
  static const rec_datetime_field realFields[][DATETIME_MAX_NUM_FIELDS] = {
    { REC_DATE_YEAR,
      REC_DATE_MONTH,
      REC_DATE_DAY,
      REC_DATE_HOUR,
      REC_DATE_MINUTE,
      REC_DATE_SECOND,
      REC_DATE_FRACTION_MP},   // DEFAULT

    { REC_DATE_MONTH,
      REC_DATE_DAY,
      REC_DATE_YEAR,
      REC_DATE_HOUR,
      REC_DATE_MINUTE,
      REC_DATE_SECOND,
      REC_DATE_FRACTION_MP},   // USA

    { REC_DATE_DAY,
      REC_DATE_MONTH,
      REC_DATE_YEAR,
      REC_DATE_HOUR,
      REC_DATE_MINUTE,
      REC_DATE_SECOND,
      REC_DATE_FRACTION_MP}    // EUROPEAN

  };
    
  // The order of the leading delimiters for the various formats.  
  // 'x' - indicates the no leading delimiter is expected for the
  // first field.
  //
  static const char delimiters[][DATETIME_MAX_NUM_FIELDS+1] = {
    "x--|::.",  // DEFAULT
    "x//;::.",  // USA
    "x..;..."   // EUROPEAN
  };

  // Used to hold the scanned values of the datetime value.
  //
  Lng32 datetimeValues[DATETIME_MAX_NUM_FIELDS+1];
  
  // Until the first field is scanned, no leading delimiter is
  // expected.
  //
  NABoolean needDelimiter = FALSE;

  // The expected delimiter.  The Null character indicates to
  // scanField that no leading delimiter is expected.
  //
  char delim = '\0';

  // Iterate over all the fields in the order dictated by the format.
  // Only fields the should be present are actually scanned.
  //
  Int32 field;
  Lng32 trueScale = scale;
  for (field = 0; field < DATETIME_MAX_NUM_FIELDS; field++) {
    
    // Determine the field expected for this format.
    //
    rec_datetime_field realField = realFields[format][field];
  
    // Scan this field only if it is contained within the destinations
    // start and end field.
    //
    if (containsField(realField, dstStartField, dstEndField)) {

      // Get the expected leading delimiter if one is required.
      //
      if(needDelimiter)
        delim = delimiters[format][field];
      
      // After the first field is scanned, a leading delimiter is
      // always required.
      //
      needDelimiter = TRUE;

      // Scan the field.
      // src - advanced to the next field.
      // datetimeVValues[realField] is set to the scanned value.
      //
      if (!scanField(src,
                     srcEnd,
                     realField,
                     delim,
                     trueScale,
                     datetimeValues[realField],
                     heap,
                     diagsArea,
   	             flags)) {
        raiseDateConvErrorWithSrcData(originalSrcLen,diagsArea, originalSrcData, heap);
        return -1;
      }
    }
  }

  // If there are any remaining characters in the input string.
  if (src != srcEnd) {
      raiseDateConvErrorWithSrcData(originalSrcLen,diagsArea, originalSrcData, heap);
      return -1;
  }

  // Adjust the value of the hour field if an "AM" or "PM" was
  // present.
  //
  if (format == DATETIME_FORMAT_USA && 
      containsField(REC_DATE_HOUR, dstStartField, dstEndField) &&
      usaAmPm) {
    if (datetimeValues[REC_DATE_HOUR] > 12) {
      raiseDateConvErrorWithSrcData(originalSrcLen,diagsArea, originalSrcData, heap);
      return -1;
    }

    if (usaAmPm == 1) {

      // " AM" present.
      //
      if (datetimeValues[REC_DATE_HOUR] == 12) {
        datetimeValues[REC_DATE_HOUR] = 0;
      }
    } else {

      // " PM" present.
      //
      if (datetimeValues[REC_DATE_HOUR] < 12) {
        datetimeValues[REC_DATE_HOUR] += 12;
      }
    }
  } else if (usaAmPm) {
    raiseDateConvErrorWithSrcData(originalSrcLen,diagsArea, originalSrcData, heap);
    return -1;
  }
    
  short year = 1900;
  char month = 1;
  char day = 1;
  char hour = 0;
  char minute = 0;
  char second = 0;
  Lng32 fraction = 0;
  
  // Copy the parsed values to the destination.
  //
  char *dst = dstData;
  for (field = dstStartField; field <= dstEndField ; field++) {
    switch (field) {
    case REC_DATE_YEAR:
      {
        year = (short)datetimeValues[field];
        str_cpy_all(dst, (char *)&year, sizeof(year));
        dst += sizeof(year);
      }
      break;
    case REC_DATE_MONTH:
      month = (char)datetimeValues[field];
      *dst++ = month;
      break;
    case REC_DATE_DAY:
      day = (char)datetimeValues[field];
      *dst++ = day;
      break;
    case REC_DATE_HOUR:
      hour = (char)datetimeValues[field];
      *dst++ = hour;
      break;
    case REC_DATE_MINUTE:
      minute = (char)datetimeValues[field];
      *dst++ = minute;
      break;
    case REC_DATE_SECOND:
      second = (char)datetimeValues[field];
      *dst++ = second;
      if (scale) {
        fraction = datetimeValues[field + 1];
        str_cpy_all(dst, (char *)&fraction, sizeof(fraction));
        dst += sizeof(fraction);
      }
      break;
    default:
      raiseDateConvErrorWithSrcData(originalSrcLen,diagsArea, originalSrcData, heap);
      return -1;
    }
  }

  scale = trueScale;

  // Validate the date fields of the result.
  //
  if (NOT noDatetimeValidation)
    if (validateDate(dstStartField, dstEndField, 
		     dstData, NULL, FALSE, 
		     LastDayPrevMonth)) {
      raiseDateConvErrorWithSrcData(originalSrcLen,diagsArea, originalSrcData, heap);
      return -1;
    };

  if (TZD) {
    // timezone specified. Compute the new datetime value.

    // first, convert current datetime value to juliantimestamp
    Lng32 jtsFraction = fraction / 1000;
    short timestamp[] = {
      year, month, day, hour, minute, second, 
      (short)(jtsFraction / 1000), (short)(jtsFraction % 1000)
    };
    
    short error;
    Int64 juliantimestamp = COMPUTETIMESTAMP(timestamp, &error);
    if (error) {
      raiseDateConvErrorWithSrcData(originalSrcLen,diagsArea, originalSrcData, heap);
      return -1;
    }

    Int64 usec = (hh*60L + mm) * 60L * 1000000L;
    if (isAdd)
      juliantimestamp += usec;
    else
      juliantimestamp -= usec;

    INTERPRETTIMESTAMP(juliantimestamp, timestamp);
    
    char *dst = dstData;
    for (field = dstStartField; field <= dstEndField ; field++) {
      switch (field) {
      case REC_DATE_YEAR:
        {
          year = timestamp[0];
          str_cpy_all(dst, (char *)&year, sizeof(year));
          dst += sizeof(year);
        }
        break;
      case REC_DATE_MONTH:
        month = (char) timestamp[1];
        *dst++ = month;
        break;
      case REC_DATE_DAY:
        day = (char) timestamp[2];
        *dst++ = day;
        break;
      case REC_DATE_HOUR:
        hour = (char) timestamp[3];
        *dst++ = hour;
        break;
      case REC_DATE_MINUTE:
        minute = (char) timestamp[4];
        *dst++ = minute;
        break;
      case REC_DATE_SECOND:
        second = (char) timestamp[5];
        *dst++ = second;
        if (scale) {
          //fraction = timestamp[6] * 1000 + timestamp[7];
          str_cpy_all(dst, (char *)&fraction, sizeof(fraction));
          dst += sizeof(fraction);
        }
        break;
      default:
        raiseDateConvErrorWithSrcData(originalSrcLen,diagsArea, originalSrcData, heap);
        return -1;
      }
    }
    
  } // TZD
  
  // Success
  //
  return 0;
    
}

short
ExpDatetime::convAsciiToDatetime(char *srcData,
                                 Lng32 srcLen,
                                 char *dstData,
                                 Lng32 dstLen,
                                 Lng32 format,
                                 CollHeap *heap,
                                 ComDiagsArea** diagsArea,
				 ULng32 flags)
{
  rec_datetime_field dstStartField;
  rec_datetime_field dstEndField;

  if (getDatetimeFields(getPrecision(),
                        dstStartField,
                        dstEndField) != 0) {
    return -1;
  }

  Lng32 scale = getScale();
  return convAsciiToDatetime(srcData, srcLen, dstData, dstLen,
                             dstStartField, dstEndField, format, scale,
                             heap, diagsArea, flags);
}

static NABoolean convertStrToMonth(char* &srcData, char *result,
                                   const char * nextByte,
                                   CollHeap * heap, ComDiagsArea** diagsArea)
{
  int copyLen = strlen(srcData);
  char * originalSrcData = srcData;
  const char * months[] = 
  {
    "JAN", 
    "FEB", 
    "MAR", 
    "APR", 
    "MAY", 
    "JUN", 
    "JUL", 
    "AUG", 
    "SEP", 
    "OCT",
    "NOV", 
    "DEC"
  };

  for (Int32 i = 0; i < 12; i++)
    {
      char upVal[3];
      str_cpy_convert(upVal, srcData, 3, 1);
      if (memcmp(upVal, months[i], 3) == 0)
	{
	  *result = (char)(i + 1);

          srcData += 3;

          if (nextByte)
            {
              if (*srcData != *nextByte)
                {
                  // string contains non-digit
                  raiseDateConvErrorWithSrcData(copyLen,diagsArea, originalSrcData, heap);
                  return FALSE; // error
                }    
              srcData++;
            }

	  return TRUE;
	}
    } // for
  
  // error
  raiseDateConvErrorWithSrcData(copyLen,diagsArea, originalSrcData, heap);
  return FALSE;
}

static NABoolean 
convertStrToMonthLongFormat(char* &value, char *result)
{
  const char * months[] = 
  {
    "JANUARY", 
    "FEBRUARY", 
    "MARCH", 
    "APRIL", 
    "MAY", 
    "JUNE", 
    "JULY", 
    "AUGUST", 
    "SEPTEMBER", 
    "OCTOBER",
    "NOVEMBER", 
    "DECEMBER"
  };

  for (Int32 i = 0; i < 12; i++)
    {
      char upVal[10];
      str_cpy_convert(upVal, value, strlen(months[i]), 1);
      if (memcmp(upVal, months[i], strlen(months[i])) == 0)
	{
	  *result = (char)(i + 1);
          value += strlen(months[i]);
	  return TRUE;
	}
    }

  // error
  return FALSE;
}

static short convSrcDataToDst(Lng32 numSrcBytes, char* &srcData, 
                               Lng32 numTgtBytes, char *dstData,
                               const char * nextByte,
                               CollHeap * heap, ComDiagsArea** diagsArea)
{
  Lng32 src = 0;
  Lng32 val = 0;
  for (val = 0, src = 0; src < numSrcBytes && isDigit8859_1(*srcData); 
       src++, srcData++)
    val = val * 10 + (*srcData - '0');
  
  if (src < numSrcBytes) 
    {
      // string contains non-digit
      raiseDateConvErrorWithSrcData(numSrcBytes,diagsArea, srcData, heap);
      return -1;
    }

  if (numTgtBytes == sizeof(Lng32))
    *(Lng32*)dstData = val;
  else if (numTgtBytes == sizeof(short))
    *(short*)dstData = val;
  else if (numTgtBytes == sizeof(char))
    *(char*)dstData = val;
  else 
    return -1;

  if (nextByte && (strlen(nextByte) > 0))
    {
      if (*srcData != *nextByte)
        {
          // string contains non-digit
          raiseDateConvErrorWithSrcData(numSrcBytes,diagsArea, srcData, heap);
          return -1;
        }    

      srcData++;
    }

  return 0;
}

//////////////////////////////////////////////////////////////////////////
// ExpDatetime::convAsciiToDate() ================================
// This method is used to convert the given ASCII string
// to a datetime date value.
//
// The result is returned in the buffer pointed to by the parameter
// 'dstData'. This buffer must be allocated by the caller and it
// must be large enough to hold the result.
//
// This method is called assuming the correct source format. The source
// string must contain date and, possibly, leading and trailing blanks
// only. The size of destination buffer should be just enough to hold
// internal representation of the date value, i.e. 4 bytes.
//
// target dstData has the format:
//  Timestamp:
//    dstData[0..1]               2-bytes for year.
//    dstData[2] .. dstData[6]    1-byte for month through second.
//    dstData[7..10]              4-bytes for fraction.
// Date:
//    dstData[0..1]               2-bytes for year.
//    dstData[2] .. dstData[3]    1-byte for month through day.
//  Time:
//    dstData[0] .. dstData[2]    1-byte for hour through second.
// =====================================================================
//
short
ExpDatetime::convAsciiToDate(char *srcData,
                             Lng32 inSrcLen,
                             char *dstData,
                             Lng32 dstLen,
			     Int32 format,
                             CollHeap *heap,
                             ComDiagsArea** diagsArea,
			     ULng32 flags)
{
  NABoolean noDatetimeValidation = (flags & CONV_NO_DATETIME_VALIDATION) != 0;
  char * timeData = NULL;  // assume no time data
  char * origSrcData = srcData;

  short year;
  Lng32  srcFormat, i;
  NABoolean LastDayPrevMonth = FALSE;

  Lng32 srcLen = inSrcLen;
  if (*srcData == ' ') {
    // skip leading blanks and adjust srcData and srcLen accordingly
    //
    for (i = 0; i < srcLen && *srcData == ' '; i++) {
      srcData++;
    }

    if (i == srcLen) {
      // string contains only blanks.
      raiseDateConvErrorWithSrcData(inSrcLen,diagsArea, srcData, heap);
      return -1;
    };

    srcLen -= i;
  };

  // need to decide what the source format is
  // 
  if (format == DATETIME_FORMAT_NONE)
    srcFormat = determineFormat(srcData, REC_DATE_YEAR, REC_DATE_DAY);
  else
    srcFormat = format;

  Lng32 minLength = getDatetimeFormatLen(srcFormat, TRUE,
					REC_DATE_YEAR, REC_DATE_DAY);
  if ((minLength <= 0) || (srcLen < minLength)) {
    // string doesn't seem to contain all date fields.
    //
    raiseDateConvErrorWithSrcData(inSrcLen,diagsArea, srcData, heap);
    return -1;
  };

  switch (srcFormat) {
  case DATETIME_FORMAT_DEFAULT: // YYYY-MM-DD
    {
      // the year
      if (convSrcDataToDst(4, srcData, 2, dstData, "-", heap, diagsArea))
        return -1;

      // the month
      if (convSrcDataToDst(2, srcData, 1, &dstData[2], "-", heap, diagsArea))
        return -1;

      // the day
      if (convSrcDataToDst(2, srcData, 1, &dstData[3], NULL, heap, diagsArea))
        return -1;
    }; 
    break;

  case DATETIME_FORMAT_DEFAULT2: // YYYY-MM
    {
      // the year
      if (convSrcDataToDst(4, srcData, 2, dstData, "-", heap, diagsArea))
        return -1;

      // the month
      if (convSrcDataToDst(2, srcData, 1, &dstData[2], NULL, heap, diagsArea))
        return -1;

      // the day
      // day is not specified, fill in as '1' (first day of month).
      dstData[3] = 1;
    }; 
    break;

  case DATETIME_FORMAT_TS3: // YYYY-MM-DD HH24:MI:SS
    {
      // the year
      if (convSrcDataToDst(4, srcData, 2, dstData, "-", heap, diagsArea))
        return -1;

      // the month
      if (convSrcDataToDst(2, srcData, 1, &dstData[2], "-", heap, diagsArea))
        return -1;

      // the day
      if (convSrcDataToDst(2, srcData, 1, &dstData[3], " ", heap, diagsArea))
        return -1;
      
      // the hour
      if (convSrcDataToDst(2, srcData, 1, &dstData[4], ":", heap, diagsArea))
        return -1;

      // the minute
      if (convSrcDataToDst(2, srcData, 1, &dstData[5], ":", heap, diagsArea))
        return -1;

      // the second
      if (convSrcDataToDst(2, srcData, 1, &dstData[6], NULL, heap, diagsArea))
        return -1;

      dstData[7]  = 0;
      dstData[8]  = 0;
      dstData[9]  = 0;
      dstData[10] = 0;

      timeData = &dstData[4];
    };  
    break;

  case DATETIME_FORMAT_USA:   // MM/DD/YYYY AM|PM
  case DATETIME_FORMAT_USA2:  // MM/DD/YYYY
  case DATETIME_FORMAT_USA6:  // MM/DD/YY
  case DATETIME_FORMAT_USA7:  // MM-DD-YYYY
    {
      char sep = (srcFormat == DATETIME_FORMAT_USA7 ? '-' : '/');

      // the month
      if (convSrcDataToDst(2, srcData, 1, &dstData[2], &sep, heap,diagsArea))
        return -1;

      // the day
      if (convSrcDataToDst(2, srcData, 1, &dstData[3], &sep, heap, diagsArea))
        return -1;

      // the year
      Int32 numOfYdigits = (srcFormat == DATETIME_FORMAT_USA6 ? 2 : 4);
      if (convSrcDataToDst(numOfYdigits, srcData, 2, dstData, NULL, heap, diagsArea))
        return -1;
    }; 
    break;

  case DATETIME_FORMAT_TS6:   // MMDDYYYY HH24:MI:SS
  case DATETIME_FORMAT_TS7:   // MM/DD/YYYY HH24:MI:SS
    {
      char sep = '/';
      char * septr = (srcFormat == DATETIME_FORMAT_TS7 ? &sep : NULL);

      // the month
      if (convSrcDataToDst(2, srcData, 1, &dstData[2], septr, heap, diagsArea))
        return -1;

      // the day
      if (convSrcDataToDst(2, srcData, 1, &dstData[3], septr, heap, diagsArea))
        return -1;
      
      // the year
      if (convSrcDataToDst(4, srcData, 2, dstData, " ", heap, diagsArea))
        return -1;

      // the hour
      if (convSrcDataToDst(2, srcData, 1, &dstData[4], ":", heap, diagsArea))
        return -1;

      // the minute
      if (convSrcDataToDst(2, srcData, 1, &dstData[5], ":", heap, diagsArea))
        return -1;

      // the second
      if (convSrcDataToDst(2, srcData, 1, &dstData[6], NULL, heap, diagsArea))
        return -1;

      dstData[7]  = 0;
      dstData[8]  = 0;
      dstData[9]  = 0;
      dstData[10] = 0;

      timeData = &dstData[4];
     };
    break;

  case DATETIME_FORMAT_USA3: // YYYY/MM/DD
  case DATETIME_FORMAT_USA4: // YYYYMMDD
  case DATETIME_FORMAT_USA5: // YY/MM/DD
    {
      // the year
      Lng32 numYearDigits = (srcFormat == DATETIME_FORMAT_USA5 ? 2 : 4);
      char sep = '/';
      char * septr = (srcFormat == DATETIME_FORMAT_USA4 ? NULL : &sep);

      // the year
      if (convSrcDataToDst(numYearDigits, srcData, 2, dstData, septr, heap, diagsArea))
        return -1;

      // the month
      if (convSrcDataToDst(2, srcData, 1, &dstData[2], septr, heap, diagsArea))
        return -1;

      // the day
      if (convSrcDataToDst(2, srcData, 1, &dstData[3], NULL, heap, diagsArea))
        return -1;

     };
    break;

  case DATETIME_FORMAT_USA8: // YYYYMM
    {
      // the year
      if (convSrcDataToDst(4, srcData, 2, dstData, NULL, heap, diagsArea))
        return -1;

      // the month
      if (convSrcDataToDst(2, srcData, 1, &dstData[2], NULL, heap, diagsArea))
        return -1;

      // the day
      // day is not specified, fill in as '1' (first day of month).
      dstData[3] = 1;
    };
    break;

  case DATETIME_FORMAT_TS1: // YYYYMMDDHH24MISS
  case DATETIME_FORMAT_TS5: // YYYYMMDD:HH24:MI:SS
    {
      char sep = ':';
      char * septr = (srcFormat == DATETIME_FORMAT_TS1 ? NULL : &sep);
      
      // the year
      if (convSrcDataToDst(4, srcData, 2, dstData, NULL, heap, diagsArea))
        return -1;

      // the month
      if (convSrcDataToDst(2, srcData, 1, &dstData[2], NULL, heap, diagsArea))
        return -1;

      // the day
      if (convSrcDataToDst(2, srcData, 1, &dstData[3], septr, heap, diagsArea))
        return -1;
      
      // the hour
      if (convSrcDataToDst(2, srcData, 1, &dstData[4], septr, heap, diagsArea))
        return -1;

      // the minute
      if (convSrcDataToDst(2, srcData, 1, &dstData[5], septr, heap, diagsArea))
        return -1;

      // the second
      if (convSrcDataToDst(2, srcData, 1, &dstData[6], NULL, heap, diagsArea))
        return -1;

      dstData[7]  = 0;
      dstData[8]  = 0;
      dstData[9]  = 0;
      dstData[10] = 0;

      timeData = &dstData[4];
     };  
    break;

  case DATETIME_FORMAT_TS9: // MONTH DD, YYYY, HH:MI AM|PM
    {
      // the month
      char * prevSrcData = srcData;
      if (! convertStrToMonthLongFormat(srcData, &dstData[2])) {
        raiseDateConvErrorWithSrcData(inSrcLen,diagsArea, srcData, heap);
        return -1;
      }
      minLength += (srcData - prevSrcData);
      srcData += 1; // skip blank after "Month"

      // the day
     if (convSrcDataToDst(2, srcData, 1, &dstData[3], ",", heap, diagsArea))
        return -1;
      srcData++;  // skip over blank

      // the year
      if (convSrcDataToDst(4, srcData, 2, dstData, ",", heap, diagsArea))
        return -1;
      srcData++;  // skip over blank
      
      // the hour
      if (convSrcDataToDst(2, srcData, 1, &dstData[4], ":", heap, diagsArea))
        return -1;
      
      // the minute
      if (convSrcDataToDst(2, srcData, 1, &dstData[5], NULL, heap, diagsArea))
        return -1;
      
      dstData[6]  = 0;
      dstData[7]  = 0;
      dstData[8]  = 0;
      dstData[9]  = 0;
      dstData[10] = 0;

      timeData = &dstData[4];
    }
    break;

  case DATETIME_FORMAT_EUROPEAN:  // DD.MM.YYYY
  case DATETIME_FORMAT_EUROPEAN2: // DD-MM-YYYY
    {
      char sep = (srcFormat == DATETIME_FORMAT_EUROPEAN ? '.' : '-');

      // the day
      if (convSrcDataToDst(2, srcData, 1, &dstData[3], &sep, heap, diagsArea))
        return -1;
      
      // the month
      if (convSrcDataToDst(2, srcData, 1, &dstData[2], &sep, heap, diagsArea))
        return -1;
      
      // the year
      if (convSrcDataToDst(4, srcData, 2, dstData, NULL, heap, diagsArea))
        return -1;
    };  
    break;

  case DATETIME_FORMAT_EUROPEAN3: // DD-MON-YYYY
  case DATETIME_FORMAT_EUROPEAN4: // DDMONYYYY
    {
      char sep = '-';
      char * septr = (srcFormat == DATETIME_FORMAT_EUROPEAN3 ? &sep : NULL);
      
      // the day
      if (convSrcDataToDst(2, srcData, 1, &dstData[3], septr, heap, diagsArea))
        return -1;
      
      // the month
      if (! convertStrToMonth(srcData, &dstData[2], septr, heap, diagsArea))
        return -1;

      // the year
      if (convSrcDataToDst(4, srcData, 2, dstData, NULL, heap, diagsArea))
        return -1;
    };  
    break;

  case DATETIME_FORMAT_TS2:  // DD.MM.YYYY:HH24.MI.SS
  case DATETIME_FORMAT_TS10: // DD.MM.YYYY HH24.MI.SS
    {
      // the day
      if (convSrcDataToDst(2, srcData, 1, &dstData[3], ".", heap, diagsArea))
        return -1;
      
      // the month
      if (convSrcDataToDst(2, srcData, 1, &dstData[2], ".", heap, diagsArea))
        return -1;
      
      // the year
      if (srcFormat == DATETIME_FORMAT_TS2)
        {
          if (convSrcDataToDst(4, srcData, 2, dstData, ":", heap, diagsArea))
            return -1;
        }
      else
        {
          if (convSrcDataToDst(4, srcData, 2, dstData, " ", heap, diagsArea))
            return -1;
        }
        
      // the hour
      if (convSrcDataToDst(2, srcData, 1, &dstData[4], ".", heap, diagsArea))
        return -1;

      // the minute
      if (convSrcDataToDst(2, srcData, 1, &dstData[5], ".", heap, diagsArea))
        return -1;

      // the second
      if (convSrcDataToDst(2, srcData, 1, &dstData[6], NULL, heap, diagsArea))
        return -1;

      dstData[7]  = 0;
      dstData[8]  = 0;
      dstData[9]  = 0;
      dstData[10] = 0;

      timeData = &dstData[4];
    };  
    break;

  case DATETIME_FORMAT_TS11: // YYYY/MM/DD HH24:MI:SS
    {
      char sep = '/';

      // the year
      if (convSrcDataToDst(4, srcData, 2, dstData, &sep, heap, diagsArea))
        return -1;

      // the month
      if (convSrcDataToDst(2, srcData, 1, &dstData[2], &sep, heap, diagsArea))
        return -1;

      // the day
      if (convSrcDataToDst(2, srcData, 1, &dstData[3], " ", heap, diagsArea))
        return -1;

      // the hour
      if (convSrcDataToDst(2, srcData, 1, &dstData[4], ":", heap, diagsArea))
        return -1;

      // the minute
      if (convSrcDataToDst(2, srcData, 1, &dstData[5], ":", heap, diagsArea))
        return -1;

      // the second
      if (convSrcDataToDst(2, srcData, 1, &dstData[6], NULL, heap, diagsArea))
        return -1;

      dstData[7]  = 0;
      dstData[8]  = 0;
      dstData[9]  = 0;
      dstData[10] = 0;

      timeData = &dstData[4];
    };  
    break;

  case DATETIME_FORMAT_TS8: // DD-MON-YYYY HH:MI:SS
    {
      // the day
      if (convSrcDataToDst(2, srcData, 1, &dstData[3], "-", heap, diagsArea))
        return -1;
      
      // the month
      if (! convertStrToMonth(srcData, &dstData[2], "-", heap, diagsArea))
        return -1;
      
      // the year
      if (convSrcDataToDst(4, srcData, 2, dstData, " ", heap, diagsArea))
        return -1;

      // the hour
      if (convSrcDataToDst(2, srcData, 1, &dstData[4], ":", heap, diagsArea))
        return -1;

      // the minute
      if (convSrcDataToDst(2, srcData, 1, &dstData[5], ":", heap, diagsArea))
        return -1;

      // the second
      if (convSrcDataToDst(2, srcData, 1, &dstData[6], NULL, heap, diagsArea))
        return -1;

      dstData[7]  = 0;
      dstData[8]  = 0;
      dstData[9]  = 0;
      dstData[10] = 0;

      timeData = &dstData[4];
     };  
    break;

  case DATETIME_FORMAT_TS4: // HH24:MI:SS
    {
      // the hour
      if (convSrcDataToDst(2, srcData, 1, &dstData[0], ":", heap, diagsArea))
        return -1;

      // the minute
      if (convSrcDataToDst(2, srcData, 1, &dstData[1], ":", heap, diagsArea))
        return -1;

      // the second
      if (convSrcDataToDst(2, srcData, 1, &dstData[2], NULL, heap, diagsArea))
        return -1;

      timeData = &dstData[0];
    };
  break;

  default:
    {
    // Format could not be determined, issue an error.
    raiseDateConvErrorWithSrcData(inSrcLen,diagsArea, srcData, heap);
    return -1;
    }
  };

  // done for all fields. if the source string is not exhausted
  // make sure it contains only blanks
  if (srcLen > minLength) {
    srcLen -= minLength;
    for (i = 0; i < srcLen; i++) {
      if (srcData[i] != ' ') {
        // string contains only blanks.
        raiseDateConvErrorWithSrcData(inSrcLen,diagsArea,srcData,heap);
        return -1;
      }
    }
  }  // if srcLen > 10

  // Validate the date fields of the result.
  //
  if (srcFormat != DATETIME_FORMAT_TS4)
    {
      if (NOT noDatetimeValidation)
	if (validateDate(REC_DATE_YEAR, REC_DATE_DAY, 
			 dstData, NULL, FALSE, 
			 LastDayPrevMonth)) {
          raiseDateConvErrorWithSrcData(inSrcLen,diagsArea,origSrcData,heap);
	  return -1;
	};
    }

  // Validate the time fields of the result
  //
  if (timeData)
    {
      if (NOT noDatetimeValidation)
        if (validateTime(timeData))
          {
            raiseDateConvErrorWithSrcData(inSrcLen,diagsArea,origSrcData,heap);
	    return -1;
          }
    }

  // Success
  //
  return 0;
}

// convertToAscii() ==============================================
// This static helper function of the ExpDatetime class is used by
// convDatetimeToASCII() to convert a numeric value to a string of the
// given width.
//
// 'value' is an input containing the numeric value to be formatted.
//
// 'result' is an input/output parameter pointing to the destination.
// As an output it points to the next available byte in the result
// buffer.
// 
// 'width' is an input specifying the required width of the value.
//
// This function assumes that the value will fit within the specified
// width.
//
// This function was added as part of the MP Datetime Compatibility
// project.
// =====================================================================
//
static void
convertToAscii(Lng32 value, char *&result, UInt32 width)
{
  UInt32 i = width;
  
  // Format value as a string.
  //
  while ((value != 0) && (i > 0)) {
    result[--i] = '0' + (char) (value % 10);
    value /= 10;
  }

  // Fill in remaining leading characters with '0'
  //
  while (i > 0)
    result[--i] = '0';

  // Update result pointer to point to end of string.
  //
  result += width;
}

static void 
convertMonthToStr(Lng32 value, char *&result, UInt32 width)
{
  const char * months[] = 
  {
    "JAN", 
    "FEB", 
    "MAR", 
    "APR", 
    "MAY", 
    "JUN", 
    "JUL", 
    "AUG", 
    "SEP", 
    "OCT",
    "NOV", 
    "DEC"
  };

  strcpy(result, months[value-1]);

  // Update result pointer to point to end of string.
  //
  result += width;
}

static void
convertDayOfWeekToStr(Lng32 value, char *&result, NABoolean bAbbreviation, UInt32 width)
{
  const char* dayofweek[] =
  {
    "SUNDAY   ",
    "MONDAY   ",
    "TUESDAY  ",
    "WEDNESDAY",
    "THURSDAY ",
    "FRIDAY   ",
    "SATURDAY "
  };

  const char* dayofweek_abb[] =
  {
    "SUN",
    "MON",
    "TUE",
    "WED",
    "THU",
    "FRI",
    "SAT"
  };

  if (bAbbreviation)
    strcpy(result, dayofweek_abb[value-1]);
  else
    strcpy(result, dayofweek[value-1]);
  // Update result pointer to point to end of string.
  result += width;
}

static void 
convertMonthToStrLongFormat(Lng32 value, char *&result, UInt32 width)
{
  const char * months[] = 
  {
    "January", 
    "February", 
    "March", 
    "April", 
    "May", 
    "June", 
    "July", 
    "August", 
    "September", 
    "October",
    "November", 
    "December"
  };

  strcpy(result, months[value-1]);

  // Update result pointer to point to end of string.
  //
  result += strlen(months[value-1]);
}

Lng32 ExpDatetime::getDatetimeFormatLen(Lng32 format, NABoolean to_date,
				       rec_datetime_field startField,
				       rec_datetime_field endField)
{
  switch (format)
    {
    case DATETIME_FORMAT_DEFAULT:   
    case DATETIME_FORMAT_USA:       
    case DATETIME_FORMAT_EUROPEAN:       
      {
	if (to_date)
	  {
            return ExpDatetime::getDatetimeFormatLen(format);
 	  }
	else
	  {
	    Lng32 minReqDstLen = 0;
	    Int32 field;
	    for (field = startField; field <= endField; field++) {
	      switch (field) {
	      case REC_DATE_YEAR:
		minReqDstLen += 5;
		break;
	      case REC_DATE_MONTH:
		minReqDstLen += 3;
		break;
	      case REC_DATE_DAY:
	      case REC_DATE_MINUTE:
		minReqDstLen += 3;
		break;
	      case REC_DATE_HOUR:
		minReqDstLen += 3;
		if (format == DATETIME_FORMAT_USA)
		  minReqDstLen += 3;
		break;
	      case REC_DATE_SECOND:
		minReqDstLen += 3;
		break;
	      default:
		return -1;
	      }
	    }
	    
	    // No trailing delimiter required.
	    //
	    minReqDstLen--;
	    return minReqDstLen;
	  }
      }
    break;

    default:
      return ExpDatetime::getDatetimeFormatLen(format);
    }

  return 0;
}

// convDatetimeToASCII() ============================================
// This method is used to convert the given datetime value to an ASCII
// string in one of three formats (DEFAULT, EUROPEAN, and USA).
//
// This method was added as part of the MP Datetime Compatibility
// project.
// =====================================================================
//
Lng32
ExpDatetime::convDatetimeToASCII(char *srcData,
                                 char *dstData,
                                 Lng32 dstLen,
                                 Int32 format,
				 char *formatStr,
                                 CollHeap *heap,
                                 ComDiagsArea** diagsArea)
{
  // Get the start and end fields of the datetime value.
  //
  rec_datetime_field startField;
  rec_datetime_field endField;
  if (getDatetimeFields(getPrecision(),
                        startField,
                        endField) != 0) {
    return -1;
  }

  // Variable for the Date portion of the datetime value.  We need to
  // store these values since the order in which they appear in the
  // string result, depends on the format.
  //
  short year=0;
  char month=0;
  char day=0;
  
  // Remember the original hour in case we need to output an 'AM' or
  // 'PM'
  //
  char militaryHour = 0;

  char *dstDataPtr = dstData;

  // Calculate the minimum length required to store the string
  // result. The fraction portion of the second field can be left off
  // or only partially present, so do not include it in this
  // calculation.
  //
  Lng32 minReqDstLen = 0;

  minReqDstLen = getDatetimeFormatLen(format, FALSE, startField, endField);
  
  // Make sure we have enough room for at least the minimum.
  //
  if((minReqDstLen <= 0) || (minReqDstLen > dstLen)) {
    ExRaiseSqlError(heap, diagsArea, EXE_STRING_OVERFLOW);
    return -1;
  }

  // Capture the date portion of the datetime value in the
  // corresponding variable.
  //
  Int32 field;
  for (field = startField;
       field <= endField && field <= REC_DATE_DAY;
       field++) {

    switch (field) {
    case REC_DATE_YEAR:
      str_cpy_all((char *) &year, srcData, sizeof(year));
      srcData += sizeof(year);
      break;
    case REC_DATE_MONTH:
      month = *srcData++;
      break;
    case REC_DATE_DAY:
      day = *srcData++;
      break;
    default:
      return -1;

    }
  }

  // Format the Date portion in the proper format.
  //
  switch (format) {
  case DATETIME_FORMAT_DEFAULT:
  case DATETIME_FORMAT_DEFAULT2:
  case DATETIME_FORMAT_USA3:
  case DATETIME_FORMAT_USA4:
  case DATETIME_FORMAT_USA5:
  case DATETIME_FORMAT_USA8:
  case DATETIME_FORMAT_TS1:
  case DATETIME_FORMAT_TS3:
  case DATETIME_FORMAT_TS5:
  case DATETIME_FORMAT_TS11:
    if (year) {
      if (format == DATETIME_FORMAT_USA5)
	convertToAscii(year, dstDataPtr, 2);
      else
	convertToAscii(year, dstDataPtr, 4);
      if (endField > REC_DATE_YEAR) {
	if ((format == DATETIME_FORMAT_DEFAULT) ||
	    (format == DATETIME_FORMAT_DEFAULT2) ||
	    (format == DATETIME_FORMAT_TS3))
	  *dstDataPtr++ = '-';
	else if ((format == DATETIME_FORMAT_USA3) ||
		 (format == DATETIME_FORMAT_USA5) ||
                 (format == DATETIME_FORMAT_TS11))
	  *dstDataPtr++ = '/';
      }
    }
    if (month) {
      convertToAscii(month, dstDataPtr, 2);
      if (endField > REC_DATE_MONTH) {
	if ((format == DATETIME_FORMAT_DEFAULT) ||
	    (format == DATETIME_FORMAT_TS3))
	  *dstDataPtr++ = '-';
	else if ((format == DATETIME_FORMAT_USA3) ||
		 (format == DATETIME_FORMAT_USA5) ||
		 (format == DATETIME_FORMAT_TS11))
	  *dstDataPtr++ = '/';
      }
    }
  if (day) {
      convertToAscii(day, dstDataPtr, 2);
    }
    break;

  case DATETIME_FORMAT_USA:
  case DATETIME_FORMAT_USA2:
  case DATETIME_FORMAT_USA6:
  case DATETIME_FORMAT_USA7:
  case DATETIME_FORMAT_TS6:
  case DATETIME_FORMAT_TS7:
    {
      char delim = (format == DATETIME_FORMAT_USA7 ? '-' 
		    : (format == DATETIME_FORMAT_TS6 ? 0 : '/'));
      
      if (month) {
	convertToAscii(month, dstDataPtr, 2);
	if ((startField < REC_DATE_MONTH || endField > REC_DATE_MONTH) &&
	    (delim != 0)) {
	  *dstDataPtr++ = delim;
	}
      }
      if (day) {
	convertToAscii(day, dstDataPtr, 2);
	if (startField < REC_DATE_MONTH && 
	    (delim != 0)) {
	  *dstDataPtr++ = delim;
	}
      }
      if (year) {
	Int32 numOfYdigits = (format == DATETIME_FORMAT_USA6 ? 2 : 4);
	
	convertToAscii(year, dstDataPtr, numOfYdigits);
      }
    }
    break;

  case DATETIME_FORMAT_EUROPEAN:
  case DATETIME_FORMAT_EUROPEAN2:
  case DATETIME_FORMAT_EUROPEAN3:
  case DATETIME_FORMAT_EUROPEAN4:
  case DATETIME_FORMAT_TS2:
  case DATETIME_FORMAT_TS8:
  case DATETIME_FORMAT_TS10:
    if (day) {
      convertToAscii(day, dstDataPtr, 2);
      if (startField < REC_DATE_DAY) {
	if ((format == DATETIME_FORMAT_EUROPEAN) ||
	    (format == DATETIME_FORMAT_TS2) ||
	    (format == DATETIME_FORMAT_TS10))
	  *dstDataPtr++ = '.';
	else if (format != DATETIME_FORMAT_EUROPEAN4)
	  *dstDataPtr++ = '-';
      }
    }
    if (month) {
      if ((format == DATETIME_FORMAT_EUROPEAN3) ||
	  (format == DATETIME_FORMAT_EUROPEAN4) ||
          (format == DATETIME_FORMAT_TS8))
	convertMonthToStr(month, dstDataPtr, 3);
      else
	convertToAscii(month, dstDataPtr, 2);
      if (startField < REC_DATE_MONTH) {
	if ((format == DATETIME_FORMAT_EUROPEAN) ||
	    (format == DATETIME_FORMAT_TS2) ||
	    (format == DATETIME_FORMAT_TS10))
	  *dstDataPtr++ = '.';
	else if (format != DATETIME_FORMAT_EUROPEAN4)
	  *dstDataPtr++ = '-';
      }
    }
    if (year) {
      convertToAscii(year, dstDataPtr, 4);
    }
    break;

  case DATETIME_FORMAT_TS9:
    {
      Lng32 length = ExpDatetime::getDatetimeFormatMaxLen(DATETIME_FORMAT_TS9);
      memset(dstDataPtr, ' ', length);
      convertMonthToStrLongFormat(month, dstDataPtr, 3);
      *dstDataPtr++ = ' ';
      
      convertToAscii(day, dstDataPtr, 2);
      *dstDataPtr++ = ',';
      *dstDataPtr++ = ' ';
      
      convertToAscii(year, dstDataPtr, 4);
      *dstDataPtr++ = ',';
    }
    break;

  case DATETIME_FORMAT_TS4:
    {
      // do nothing for date part.
    }
  break;

  case DATETIME_FORMAT_EXTRA_HH:
  case DATETIME_FORMAT_EXTRA_HH24:
  case DATETIME_FORMAT_EXTRA_HH12:
    {
      char hour = *srcData++;
      if ( DATETIME_FORMAT_EXTRA_HH12 == format )
        {
          if (hour > 12)
            hour = hour - 12;
        }
      convertToAscii(hour, dstDataPtr, 2);
      return (dstDataPtr - dstData);
    }
    break;

  case DATETIME_FORMAT_EXTRA_MI:
    {
      char minute = *(srcData+1);
      convertToAscii(minute, dstDataPtr, 2);
      return (dstDataPtr - dstData);
    }
    break;

  case DATETIME_FORMAT_EXTRA_SS:
    {
      char second = *(srcData+2);
      convertToAscii(second, dstDataPtr, 2);
      return (dstDataPtr - dstData);
    }
    break;

  case DATETIME_FORMAT_EXTRA_YYYY:
  case DATETIME_FORMAT_EXTRA_YYY:
  case DATETIME_FORMAT_EXTRA_YY:
  case DATETIME_FORMAT_EXTRA_Y:
    {
      UInt32 nw = 4; //DATETIME_FORMAT_EXTRA_YYYY
      if ( DATETIME_FORMAT_EXTRA_YYY == format )
        {
          nw = 3;
          year = year % 1000;
        }
      else if ( DATETIME_FORMAT_EXTRA_YY == format )
        {
          nw = 2;
          year = year % 100;
        }
      else if ( DATETIME_FORMAT_EXTRA_Y == format )
        {
          nw = 1;
          year = year % 10;
        }
      convertToAscii(year, dstDataPtr, nw);
      return (dstDataPtr - dstData);
    }
    break;
  case DATETIME_FORMAT_EXTRA_CC:
    {
      year = (year+99)/100;
      convertToAscii(year, dstDataPtr,2);
      return (dstDataPtr - dstData);
    }
    break;
  case DATETIME_FORMAT_EXTRA_MON:
  case DATETIME_FORMAT_EXTRA_MM:
    {
      if (DATETIME_FORMAT_EXTRA_MM == format)
        convertToAscii(month, dstDataPtr,2);
      else if (DATETIME_FORMAT_EXTRA_MON == format)
        {
          if (0 == month)
            return -1;
          convertMonthToStr(month, dstDataPtr, 3);
        }
      return (dstDataPtr - dstData);
    }
    break;
  case DATETIME_FORMAT_EXTRA_DY:
  case DATETIME_FORMAT_EXTRA_DAY:
  case DATETIME_FORMAT_EXTRA_D:
    {
      Int64 interval = getTotalDays(year, month, day);
      short dayofweek = (short)(((interval + 1) % 7) + 1);
      if (DATETIME_FORMAT_EXTRA_D == format)
        {
          convertToAscii(dayofweek,dstDataPtr,1);
        }
      else if (DATETIME_FORMAT_EXTRA_DAY == format
               || DATETIME_FORMAT_EXTRA_DY == format)
        {
          if (0 == day)
            return -1;
          //SUNDAY or SUN
          NABoolean bAbbr = (DATETIME_FORMAT_EXTRA_DY == format ? TRUE:FALSE);
          UInt32 width = 9;
          if (bAbbr)
            width = 3;
          convertDayOfWeekToStr(dayofweek, dstDataPtr, bAbbr, width);
        }
      return (dstDataPtr - dstData);
    }
    break;
  case DATETIME_FORMAT_EXTRA_DD:
    {
      convertToAscii(day, dstDataPtr, 2);
      return (dstDataPtr - dstData);
    }
    break;
  case DATETIME_FORMAT_EXTRA_DDD:
    {
      int dayofyear = 0;
      if( day )
        dayofyear = Date2Julian(year,month,day)-Date2Julian(year,1,1)+1;
      convertToAscii(dayofyear,dstDataPtr,3);
      return (dstDataPtr - dstData);
    }
    break;
  case DATETIME_FORMAT_EXTRA_W:
    {
      int weekofmonth = 0;
      if (day)
        weekofmonth = (day-1)/7+1;
      convertToAscii(weekofmonth,dstDataPtr,1);
      return (dstDataPtr - dstData);
    }
    break;
  case DATETIME_FORMAT_EXTRA_WW:
    {
      //same with built-in function week
      int weekofmonth = 0;
      if ( day )
        {
          Int64 interval = getTotalDays(year, 1, 1);
          int dayofweek = (int)(((interval + 1) % 7) + 1);
          int dayofyear = Date2Julian(year,month,day)-Date2Julian(year,1,1)+1;
          weekofmonth = (dayofyear-1+dayofweek-1)/7+1;
        }
      convertToAscii(weekofmonth,dstDataPtr,2);
      return (dstDataPtr - dstData);
    }
    break;
  case DATETIME_FORMAT_EXTRA_J:
    {
      int julianday = Date2Julian(year,month,day);
      convertToAscii(julianday,dstDataPtr,7);
      return (dstDataPtr - dstData);
    }
    break;
  case DATETIME_FORMAT_EXTRA_Q:
    {
      if (month)
        {
          month = (month-1)/3+1;
        }
      convertToAscii(month,dstDataPtr,1);
      return (dstDataPtr - dstData);
    }
    break;

  default:
    return -1;
  }


  // Add a delimiter between the date and time portion if required.
  //
  if (field > startField && field <= endField)
    {
      switch (format) {
      case DATETIME_FORMAT_TS1:
	{
	}
      break;

      case DATETIME_FORMAT_TS2:
      case DATETIME_FORMAT_TS5:
	{
	  *dstDataPtr++ = ':';
	}
      break;
	
      default:
	{
	  *dstDataPtr++ = ' ';
	}
      break;
      }
    }

  // Format the Time portion in the proper format.
  //
  for (; field <= endField; field++) {
    switch (field) {
    case REC_DATE_HOUR: {
      char hour = militaryHour = *srcData++;

      // USA format uses AM|PM format.
      //
      if ((format == DATETIME_FORMAT_USA) ||
          (format == DATETIME_FORMAT_TS7)) {
        if (hour < 1)
          hour += 12;
        else if (hour > 12)
          hour -= 12;
      }
      convertToAscii(hour, dstDataPtr, 2);
      if (endField > REC_DATE_HOUR) {
	if ((format == DATETIME_FORMAT_EUROPEAN) ||
            (format == DATETIME_FORMAT_TS10))
	  *dstDataPtr++ = '.';
	else if (format != DATETIME_FORMAT_TS1)
	  *dstDataPtr++ = ':';
      }
      break;
    }
    case REC_DATE_MINUTE: {
      char minute = *srcData++;
      convertToAscii(minute, dstDataPtr, 2);
      if (endField > REC_DATE_MINUTE) {
        if (format == DATETIME_FORMAT_TS9)
          return (dstDataPtr - dstData);
        else if ((format == DATETIME_FORMAT_EUROPEAN) ||
            (format == DATETIME_FORMAT_TS10))
          *dstDataPtr++ = '.';
        else if (format != DATETIME_FORMAT_TS1)
          *dstDataPtr++ = ':';
       }
      break;
    }
    case REC_DATE_SECOND: {
      char second = *srcData++;
      convertToAscii(second, dstDataPtr, 2);

      // If there is a fraction portion of the second field and there
      // is room in the destination string, format as much of the
      // fraction as possible.
      //
      Int32 fractionPrecision = getScale();
      Lng32 fraction;
      if (fractionPrecision > 0) {

        // dstPrecision is the available space in the destination
        // string (minus 1 for delimiter)
        //
        Lng32 dstPrecision = dstLen - minReqDstLen - 1;

        // Get the fraction value.
        //
        str_cpy_all((char *) &fraction, srcData, sizeof(fraction));

        // If it won't all fit, scale it down so it will.
        //
        if(dstPrecision < fractionPrecision) {
          fraction = scaleFraction(fractionPrecision, fraction, dstPrecision);
          fractionPrecision = dstPrecision;
        }
      }

      // If we still have a fraction precision left, format it into
      // the result string.
      //
      if(fractionPrecision > 0) {
	if (format == DATETIME_FORMAT_USA2)
	  {
	    *dstDataPtr++ = ':';
	    convertToAscii(fraction, dstDataPtr, 2);
	  }
	else
	  {
	    *dstDataPtr++ = '.';
	    convertToAscii(fraction, dstDataPtr, fractionPrecision);
	  }
      }
      break;
    }
    default:
      return -1;
    }
  }

  // If the format is USA and there is an HOUR field, add AM or PM.
  //
  if (((format == DATETIME_FORMAT_USA) ||
       (format == DATETIME_FORMAT_TS7)) &&
      startField <= REC_DATE_HOUR &&
      endField >= REC_DATE_HOUR) {
    if (militaryHour < 12) {
      str_cpy_all(dstDataPtr, " AM", 3);
    } else {
      str_cpy_all(dstDataPtr, " PM", 3);
    }

    dstDataPtr += 3;
  }

  // if format includes time field but source is a DATE datatype, extend
  // the returned string with zeroes
  if (isTimestampFormat(format))
    {
      if (format == DATETIME_FORMAT_TS1)
        {
        }
      else if ((format == DATETIME_FORMAT_TS2) ||
               (format == DATETIME_FORMAT_TS5))
        {
          *dstDataPtr = ':';
          dstDataPtr++;
        }
      else
        {
          *dstDataPtr = ' ';
          dstDataPtr++;
        }
      
      if (format == DATETIME_FORMAT_TS1)
        {
          str_cpy_all(dstDataPtr, "000000", 6);
          dstDataPtr += 6;
        }
      else if (format == DATETIME_FORMAT_EUROPEAN)
        {
          str_cpy_all(dstDataPtr, "00.00.00", 8);
          dstDataPtr += 8;
        }
      else
        {
          str_cpy_all(dstDataPtr, "00:00:00", 8);
          dstDataPtr += 8;
        }
    }
      
  // Return the actual number of bytes formatted.
  //
  return dstDataPtr - dstData;
}

// convNumericTimeToASCII() ============================================
// This method is used to convert the given numeric time value to an ASCII
// string in the provided format. It is based on special1 behavior.
//
// Input numeric value is an Int64.
// 
// DATETIME_FORMAT_TIME1:  99:99:99:99
//                         an 8-digit number is broken up into 2 digit
//                         parts separated by : (colon)
//                         Ex, 12345 will be returned as 00:01:23:45
//                         If negative number, sign is ignored.
// 
// DATETIME_FORMAT_TIME2:  -99:99:99:99
//                         an 8-digit number is broken up into 2 digit
//                         parts separated by : (colon)
//                         Ex, -12345 will be returned as -00:01:23:45
// 
// If value is less than -long max or greater than + long max, the
// result will be "**********".
//
// DATETIME_FORMAT_TIME_STR: format is in formatStr. Not yet supported.
// 
// =====================================================================
//
Lng32
ExpDatetime::convNumericTimeToASCII(char *srcData,
				    char *dstData,
				    Lng32 dstLen,
				    Int32 format,
				    char *formatStr,
				    CollHeap *heap,
				    ComDiagsArea** diagsArea)
{
  if ((format != DATETIME_FORMAT_NUM1) &&
      (format != DATETIME_FORMAT_NUM2))
    return -1;

  if ((format == DATETIME_FORMAT_NUM1) &&
      (dstLen < 11))
    return -1;

  if ((format == DATETIME_FORMAT_NUM2) &&
      (dstLen < 12))
    return -1;

  Int64 temp = *(Int64*)srcData;
  NABoolean negative = FALSE;
  if (temp < 0)
    {
      // cannot convert negative number with NUM1 format
      if (format == DATETIME_FORMAT_NUM1)
        {
          raiseDateConvErrorWithSrcDataNumeric(diagsArea,temp,heap);
          return -1;
        }

      temp = -temp;

     if (format == DATETIME_FORMAT_NUM2)
       {
	 negative = TRUE;
       }
    }

  NABoolean overflow = FALSE;
  if ((temp > INT_MAX) ||
      (temp < -INT_MAX))
    {
      overflow = TRUE;
    }
  
  if (overflow)
    {
      str_pad(dstData, dstLen, '*');
    }
  else
    {
      Lng32 part1, part2, part3, part4;
      part4 = (Lng32)(temp - (temp/100)*100);
      temp = temp/100;
      part3 = (Lng32)(temp - (temp/100)*100);
      temp = temp/100;
      part2 = (Lng32)(temp - (temp/100)*100);
      temp = temp/100;
      part1 = (Lng32)(temp - (temp/100)*100);
      temp = temp/100;
  
      // if more digits left in input, error out.
      if (temp > 0)
        {
          raiseDateConvErrorWithSrcDataNumeric(diagsArea,temp,heap);
          return -1;
        }

      if (format == DATETIME_FORMAT_NUM2)
	{
	  if (negative)
	    str_sprintf(dstData, "-%02d:%02d:%02d:%02d", part1, part2, part3, part4);
	  else
	    str_sprintf(dstData, " %02d:%02d:%02d:%02d", part1, part2, part3, part4);
	}
      else
	str_sprintf(dstData, "%02d:%02d:%02d:%02d", part1, part2, part3, part4);
    }

  // number of bytes formatted.
  return 11;
}

static const UInt32 maxFieldLen[]   = {    4,  2,  2,  2,  2,  2, 6 };

short ExpDatetime::getDisplaySize(Lng32 datetimeCode,
                  short fractionPrecision)
{
  rec_datetime_field startField, endField;

  getDatetimeFields(datetimeCode, startField, endField);
  Int32 field = startField - REC_DATE_YEAR;
  size_t displayLength = maxFieldLen[field];
  while (field < (endField - REC_DATE_YEAR))
    displayLength += 1 /* for separator */ + maxFieldLen[++field];
  if (fractionPrecision > 0)
  {
    if (startField == REC_DATE_FRACTION_MP)
    {
      displayLength = fractionPrecision;
    }
    else
    {
     displayLength += 1 /* for separator */ + fractionPrecision;
    }
  }
  return displayLength;
}

short ExpDatetime::convAsciiDatetimeToASCII(char *srcData,
                        Lng32 srcPrecision,
                        Lng32 srcScale,
                        Lng32 srcLen,
                        char *dstData,
                        Lng32 dstLen,
                        Int32 format,
                        CollHeap *heap,
                        ComDiagsArea** diagsArea)
{
  short rc = 0;

  SimpleType tempST(REC_DATETIME, 12, 
            srcScale, srcPrecision,
            ExpTupleDesc::SQLMX_FORMAT,
            0, 0, 0, 0, Attributes::NO_DEFAULT, 0);

  char tempDTBuf[12]; // max length for an internal datetime value.

  ExpDatetime &tempDT = (ExpDatetime&)tempST;
  rc = 
    tempDT.convAsciiToDatetime
    (srcData, srcLen, tempDTBuf, 12, DATETIME_FORMAT_NONE, heap, diagsArea, 0);
  if (rc)
    return rc;

  rc =
    tempDT.convDatetimeToASCII(tempDTBuf, dstData, dstLen, format, NULL,
                   heap, diagsArea);
  if (rc < 0)
    return rc;

  return 0;
}


// getFieldName() ====================================================
// This static helper function of the ExpDatetime class returns the
// string (char *) value of the given datetime field.  For example, if
// given the value REC_DATE_MONTH, this method returns the string
// "MONTH".  This function is used by getDefaultStringValue (see above).
//
// This function was added as part of the MP Datetime Compatibility
// project.
// ===================================================================
//
static
const char*
getFieldName(rec_datetime_field field)
{
  switch (field) {
  case REC_DATE_YEAR:
    return "YEAR";
  case REC_DATE_MONTH:
    return "MONTH";
  case REC_DATE_DAY:
    return "DAY";
  case REC_DATE_HOUR:
    return "HOUR";
  case REC_DATE_MINUTE:
    return "MINUTE";
  case REC_DATE_SECOND:
    return "SECOND";
  default:
    return NULL;
  }
}

// getDefaultStringValue() ===========================================
// This method constructs a string (char *) value representing the
// default value for a datetime type. This method is used to construct
// values for columns that have been added to a table through an ALTER
// TABLE statement.  When a column is added to a table, the existing
// records are not immediately updated.  Rather, when these records
// are selected, a default value for the column is constructed.  As
// described in the SQL/MP reference manual (ALTER TABLE, p..A-37) the
// default value used for all datetime values is based on the
// TimeStamp: '0001-01-01:12:00:00.00000'.  The default value is the
// appropriate range of fields taken from the default TimeStamp.  For
// the non-standard datetime types, the default value must also
// include the datetime qualifier.  For example the default value for
// a non-standard datetime of MONTH TO DAY would be: "DATETIME '01-01'
// MONTH TO DAY".
//
// This method is only called from the generator
// (ExpGenerator::addDefaultValue() in generator/GenExpGenerator.cpp)
//
// This method was added as part of the MP Datetime Compatibility
// project.
// ===================================================================
//
char *
ExpDatetime::getDefaultStringValue(CollHeap *heap)
{

  rec_datetime_field startField;
  rec_datetime_field endField;
  char *ptr;
  Int32 len;

  // Get the start and end fields for this datetime type.
  //
  if(getDatetimeFields(getPrecision(),
                       startField,
                       endField) != 0) {
    return NULL;
  }

  char buffer[MAX_DATETIME_STRING_LEN];
  ptr = buffer;

  // Construct the type name
  //

  const char *dateKW = "DATE '";
  const char *timeKW = "TIME '";
  const char *timeStampKW = "TIMESTAMP '";
  const char *datetimeKW = "DATETIME '";

  switch (getPrecision()) {
  case REC_DTCODE_DATE:
    len = str_len(dateKW);
    str_cpy_all(ptr,dateKW, len);
    break;

  case REC_DTCODE_TIME:
    len = str_len(timeKW);
    str_cpy_all(ptr,timeKW, len);
    break;

  case REC_DTCODE_TIMESTAMP:
    len = str_len(timeStampKW);
    str_cpy_all(ptr,timeStampKW, len);
    break;

  default:
    len = str_len(datetimeKW);
    str_cpy_all(ptr,datetimeKW, len);
    break;
  }

  static const char * const defValues[] = {"", "0001", "01", "01", "12", "00", "00"};

  static const char delims[6] = {'x','-','-',':',':',':'};
    
  // Construct the default value string, based on the start and end
  // fields.
  //
  for (Int32 field = startField; field <= endField; field++) {
    Int32 flen = str_len(defValues[field]);
    str_cpy_all(ptr+len, defValues[field], flen);
    len += flen;
    if(field != endField) {
      ptr[len++] = delims[field];
    }
  }

  ptr[len++] = '\'';

  // Construct the datetime qualifier if needed.
  //
  if(getPrecision() > REC_DTCODE_TIMESTAMP) {
    ptr[len++] = ' ';
    
    Int32 flen = str_len(getFieldName(startField));
    str_cpy_all(ptr+len, getFieldName(startField), flen);
    len += flen;
    if(startField != endField) {
      flen = str_len(" TO ");
      str_cpy_all(ptr+len, " TO ", flen);
      len += flen;
      
      flen = str_len(getFieldName(endField));
      str_cpy_all(ptr+len, getFieldName(endField), flen);
      len += flen;
    }
  }

  ptr[len++] = ';';
  ptr[len++] = '\0';

  // Allocate space for the default string (including NULL char) and
  // copy from local buffer
  //
  ptr = new (heap) char[str_len(buffer) + 1];
  str_cpy_all(ptr, buffer, str_len(buffer) + 1);

  return ptr;
}
