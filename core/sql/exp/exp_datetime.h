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
#ifndef EXP_DATETIME_H
#define EXP_DATETIME_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         exp_datetime.h
 * Description:  Datetime Type
 *
 *
 * Created:      8/19/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include "ExpError.h"
#include "exp_attrs.h"
#include "Int64.h"

#pragma warning ( disable : 4251 )

class SQLEXP_LIB_FUNC  ExpDatetime : public SimpleType {

public:
  // these enums must be in the same order as the datetimeFormat[] array 
  // (defined in exp_datetime.cpp).
  enum DatetimeFormats 
  { 
    DATETIME_FORMAT_MIN       =  0,
    DATETIME_FORMAT_MIN_DATE  =  DATETIME_FORMAT_MIN,
    DATETIME_FORMAT_DEFAULT   =  DATETIME_FORMAT_MIN_DATE, // YYYY-MM-DD
    DATETIME_FORMAT_USA,        // MM/DD/YYYY AM|PM
    DATETIME_FORMAT_EUROPEAN,   // DD.MM.YYYY
    DATETIME_FORMAT_DEFAULT2,   // YYYY-MM
    DATETIME_FORMAT_USA2,       // MM/DD/YYYY
    DATETIME_FORMAT_USA3,       // YYYY/MM/DD
    DATETIME_FORMAT_USA4,       // YYYYMMDD
    DATETIME_FORMAT_USA5,       // YY/MM/DD
    DATETIME_FORMAT_USA6,       // MM/DD/YY
    DATETIME_FORMAT_USA7,       // MM-DD-YYYY
    DATETIME_FORMAT_USA8,       // YYYYMM
    DATETIME_FORMAT_EUROPEAN2,  // DD-MM-YYYY
    DATETIME_FORMAT_EUROPEAN3,  // DD-MON-YYYY
    DATETIME_FORMAT_EUROPEAN4,  // DDMONYYYY
    DATETIME_FORMAT_MAX_DATE  = DATETIME_FORMAT_EUROPEAN4,

    DATETIME_FORMAT_MIN_TIME,
    DATETIME_FORMAT_TS4       = DATETIME_FORMAT_MIN_TIME, // HH24:MI:SS
    DATETIME_FORMAT_MAX_TIME  = DATETIME_FORMAT_TS4,

    DATETIME_FORMAT_MIN_TS,
    DATETIME_FORMAT_TS1       = DATETIME_FORMAT_MIN_TS, // YYYYMMDDHH24MISS
    DATETIME_FORMAT_TS2,     // DD.MM.YYYY:HH24:MI:SS
    DATETIME_FORMAT_TS3,     // YYYY-MM-DD HH24:MI:SS
    DATETIME_FORMAT_TS5,     // YYYYMMDD:HH24:MI:SS
    DATETIME_FORMAT_TS6,     // MMDDYYYY HH24:MI:SS
    DATETIME_FORMAT_TS7,     // MM/DD/YYYY HH24:MI:SS
    DATETIME_FORMAT_TS8,     // DD-MON-YYYY HH:MI:SS
    DATETIME_FORMAT_TS9,     // MONTH DD, YYYY, HH:MI AM|PM
    DATETIME_FORMAT_TS10,    // DD.MM.YYYY HH24:MI:SS
    DATETIME_FORMAT_MAX_TS    = DATETIME_FORMAT_TS10,

    DATETIME_FORMAT_MAX       = DATETIME_FORMAT_MAX_TS,

    DATETIME_FORMAT_MIN_NUM = DATETIME_FORMAT_MAX,
    DATETIME_FORMAT_NUM1,     // 99:99:99:99
    DATETIME_FORMAT_NUM2,     // -99:99:99:99
    DATETIME_FORMAT_MAX_NUM = DATETIME_FORMAT_NUM2,

    DATETIME_FORMAT_DATE_STR, // format in str
    DATETIME_FORMAT_TIME_STR, // format in str
    DATETIME_FORMAT_NONE,
    DATETIME_FORMAT_ERROR     = -1
  };

  struct DatetimeFormatInfo
  {
    Lng32 format;       // defined by enum DatetimeFormats
    const char * str;   // string representation of datetime format
    Lng32 minLen;       // minimum length to hold this format
    Lng32 maxLen;
  };
  
  static const DatetimeFormatInfo datetimeFormat[];

  enum { DATETIME_MAX_NUM_FIELDS = 7 };
  enum { MAX_DATETIME_SIZE = 11 };

  enum { MAX_DATETIME_FRACT_PREC = 6 };

  // MAX Length of Datetime string is 50 -
  // "DATE 'YYYY-MM-DD';"
  // "TIME 'HH:MM:SS';"
  // "TIMESTAMP 'YYYY-MM-DD:HH:MM:SS.MSSSSS';"
  // "DATETIME 'YYYY-MM-DD:HH:MM' YEAR TO MINUTE;"
  // "DATETIME 'MM-DD:HH:MM:SS.MSSSSS' MONTH TO SECOND;"
  // 012345678901234567890123456789012345678901234567890
  //
  enum { MAX_DATETIME_STRING_LEN = 50 };

  enum arithOps {DATETIME_ADD, DATETIME_SUB};

NA_EIDPROC
  ExpDatetime();
NA_EIDPROC
  ~ExpDatetime();

NA_EIDPROC
  static Int64 getTotalDays(short year, short month, short day);

NA_EIDPROC
  static short getDatetimeFields(Lng32 datetimeCode,
                                 rec_datetime_field &startField,
                                 rec_datetime_field &endField);

NA_EIDPROC
  void convertDatetimeToInterval(rec_datetime_field datetimeStartField,
                                 rec_datetime_field datetimeEndField,
                                 short fractionPrecision,
                                 rec_datetime_field intervalEndField,
                                 char *datetimeOpData,
                                 Int64 &interval) const;

NA_EIDPROC
  static short getYearMonthDay(Int64 totalDays,
                               short &year,
                               char &month,
                               char &day);

NA_EIDPROC
  short convertIntervalToDatetime(Int64 interval,
                                  rec_datetime_field startField,
                                  rec_datetime_field endField,
                                  short fractionPrecision,
                                  char *datetimeOpData) const;

NA_EIDPROC
  static short validateDate(rec_datetime_field startField,
                            rec_datetime_field endField,
                            char *datetimeOpData,
                            ExpDatetime *attr,
                            short intervalFlag,
                            NABoolean &LastDayPrevMonth);

NA_EIDPROC
  short compDatetimes(char *datetimeOpData1,
                      char *datetimeOpData2);

NA_EIDPROC
  short arithDatetimeInterval(ExpDatetime::arithOps operation,
                              ExpDatetime *datetimeOpType,
                              Attributes *intervalOpType,
                              char *datetimeOpData,
                              char *intervalOpData,
                              char *resultData,
                              CollHeap *heap,
                              ComDiagsArea** diagsArea);

NA_EIDPROC
  short subDatetimeDatetime(Attributes *datetimeOpType,
                            Attributes *intervalOpType,
                            char *datetimeOpData1,
                            char *datetimeOpData2,
                            char *resultData,
                            CollHeap *heap,
                            ComDiagsArea** diagsArea) const;

NA_EIDPROC
  static short getDisplaySize(Lng32 datetimeCode,
			      short fractionPrecision);
		       
NA_EIDPROC
  static Lng32 getDatetimeFormatLen(Lng32 format, NABoolean to_date,
				   rec_datetime_field startField,
				   rec_datetime_field endField);

NA_EIDPROC
  Attributes * newCopy();

NA_EIDPROC
  Attributes * newCopy(CollHeap *);

NA_EIDPROC
  void copyAttrs(Attributes *source_); // copy source attrs to this.

  // ---------------------------------------------------------------------
  // Perform type-safe pointer casts.
  // ---------------------------------------------------------------------
NA_EIDPROC
  virtual
  ExpDatetime* castToExpDatetime();

NA_EIDPROC
  short convDatetimeDatetime(char *srcData,
                             rec_datetime_field dstStartField,
                             rec_datetime_field dstEndField,
                             short dstFractPrec,
                             char *dstData,
                             Lng32 dstLen,
			     short validateFlag,
                             NABoolean *roundedDownFlag = NULL);

NA_EIDPROC
  static short currentTimeStamp(char *dstData,
                                rec_datetime_field startField,
                                rec_datetime_field endField,
                                short fractPrec);

NA_EIDPROC
  short extractDatetime(rec_datetime_field srcStartField,
                        rec_datetime_field srcEndField,
                        short srcFractPrec,
                        char *srcData,
                        char *dstData);


  static short convAsciiToDatetime(char *source,
                                   Lng32 sourceLen,
                                   char *target,
                                   Lng32 targetLen,
                                   rec_datetime_field dstStartField,
                                   rec_datetime_field dstEndField,
                                   Lng32 format,
                                   Lng32 &scale,
                                   CollHeap *heap,
                                   ComDiagsArea** diagsArea,
                                   ULng32 flags);
  
  short convAsciiToDatetime(char *source,
                            Lng32 sourceLen,
                            char *target,
                            Lng32 targetLen,
                            Lng32 format,
                            CollHeap *heap,
                            ComDiagsArea** diagsArea,
                            ULng32 flags);

NA_EIDPROC
  short convAsciiToDate(char *target,
                        Lng32 targetLen,
                        char *source,
                        Lng32 sourceLen,
			Int32 format,
                        CollHeap *heap,
                        ComDiagsArea** diagsArea,
                        ULng32 flags);

NA_EIDPROC
  Lng32 convDatetimeToASCII(char *srcData,
                           char *dstData,
                           Lng32 dstLen,
                           Int32 format,
			   char *formatStr,
                           CollHeap *heap,
                           ComDiagsArea** diagsArea);

NA_EIDPROC
  static Lng32 convNumericTimeToASCII(char *srcData,
				     char *dstData,
				     Lng32 dstLen,
				     Int32 format,
				     char *formatStr,
				     CollHeap *heap,
				     ComDiagsArea** diagsArea);
  
NA_EIDPROC
static
  short convAsciiDatetimeToASCII(char *srcData,
				 Lng32 srcPrecision,
				 Lng32 srcScale,
				 Lng32 srcLen,
				 char *dstData,
				 Lng32 dstLen,
				 Int32 format,
				 CollHeap *heap,
				 ComDiagsArea** diagsArea);
  
  char *getDefaultStringValue(CollHeap *heap);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  NA_EIDPROC virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  NA_EIDPROC virtual void populateImageVersionIDArray()
  {
    setImageVersionID(2,getClassVersionID());
    SimpleType::populateImageVersionIDArray();
  }

  NA_EIDPROC virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------

  static const char * getDatetimeFormatStr(Lng32 frmt)
  {
    return datetimeFormat[frmt].str;
  }

  static const Lng32 getDatetimeFormat(const char * formatStr)
  {
    for (Lng32 i = DATETIME_FORMAT_MIN; i <= DATETIME_FORMAT_MAX; i++)
      {
        if (strcmp(formatStr, datetimeFormat[i].str) == 0)
          {
            if (datetimeFormat[i].format != i)
              return -1;

            return i;
          }
      }

    for (Lng32 i = DATETIME_FORMAT_MIN_NUM; i <= DATETIME_FORMAT_MAX_NUM; i++)
      {
        if (strcmp(formatStr, datetimeFormat[i].str) == 0)
          {
            if (datetimeFormat[i].format != i)
              return -1;

            return i;
          }
      }

    return -1;
  }
  
  static NABoolean isDateTimeFormat(Lng32 frmt)
  {
    return ((frmt >= DATETIME_FORMAT_MIN) &&
            (frmt <= DATETIME_FORMAT_MAX));
  }

  static NABoolean isDateFormat(Lng32 frmt)
  {
    return ((frmt >= DATETIME_FORMAT_MIN_DATE) &&
            (frmt <= DATETIME_FORMAT_MAX_DATE));
  }

  static NABoolean isTimestampFormat(Lng32 frmt)
  {
    return ((frmt >= DATETIME_FORMAT_MIN_TS) &&
            (frmt <= DATETIME_FORMAT_MAX_TS));
  }

  static NABoolean isTimeFormat(Lng32 frmt)
  {
    return ((frmt >= DATETIME_FORMAT_MIN_TIME) &&
            (frmt <= DATETIME_FORMAT_MAX_TIME));
  }

  static NABoolean isNumericFormat(Lng32 frmt)
  {
    return ((frmt == DATETIME_FORMAT_NUM1) || (frmt == DATETIME_FORMAT_NUM2));
  }

  static Lng32 getDatetimeFormatLen(Lng32 frmt)
  {
    return datetimeFormat[frmt].minLen;
  }

  static Lng32 getDatetimeFormatMaxLen(Lng32 frmt)
  {
    return datetimeFormat[frmt].maxLen;
  }

private:

};

#pragma warning ( default : 4251 )

#endif

