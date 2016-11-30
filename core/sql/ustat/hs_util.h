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
#ifndef HSUTIL_H
#define HSUTIL_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         hs_util.h
 * Description:  Utility functions.
 * Created:      03/25/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Int64.h"

#include "hs_const.h"
#include "hs_cli.h"


struct HSColumnStruct;
Lng32 FormatRow(const HSColumnStruct *srcDesc,
               const char *src,
               HSDataBuffer &target);


NABoolean isHBaseMeta(const ComObjectName& objectName);
NABoolean isHBaseMeta(const QualifiedName& qualifiedName);

NABoolean isHBaseUmdHistograms(const ComObjectName& objectName);
NABoolean isHBaseUmdHistograms(const QualifiedName& qualifiedName);

NABoolean isSpecialObject(const NAString& tableName);
NABoolean isSpecialObject(const ComObjectName& objectName);
NABoolean isSpecialObject(const QualifiedName& qualifiedName);

NAString getTableName(const NAString name, const ComAnsiNameSpace nameSpace);

void TrafToHiveSampleTableName(NAString& name);

// -----------------------------------------------------------------------
// Return the root of fx = 0.
// -----------------------------------------------------------------------
double xValue( const double x0
             , const double n
             );

// -----------------------------------------------------------------------
// Return the uppercase value of passed character.
// -----------------------------------------------------------------------
inline Int32 hs_toupper(Int32 c) 
{
  if (c >= 'a' && c<='z') return c-32;
  else                    return c;
}

// -----------------------------------------------------------------------
// A strcasecmp implementation.  Note that this does not return if there is
// a null terminator earlier than 'length'.
// -----------------------------------------------------------------------
inline Int32 hs_strncasecmp(const char *s1, const char *s2, Int32 length)
{
  Int32 diff;
  for (Int32 i=0; i<length; i++, s1++, s2++)
    if ((diff = hs_toupper(*s1) - hs_toupper(*s2))) return diff;
  return 0;
}

// -----------------------------------------------------------------------
// UEC estimator, to estimate the UEC in a population, based on a sample.
// The sample UEC, sample row count, estimated population size, and
// sample frequency information are provided as input.  The estimation
// method is a linear weighted combination of the unsmoothed jacknife and
// Shlosser methods.  The unsmoothed jacknife and Shlosser UEC estimates
// are returned via ref. parameters Duj and Dsh, respectively.  The
// coefficient of variation, a measure of data skew, is returned via
// ref. parameter coeffOfVar.  The weighting of the Shlosser method is
// defined to be the computed coefficient of variation, divided by
// DshMax, with a maximum weight value of 1.  The weighting of the
// unsmoothed jacknife estimate is 1 minus the Shlosser weight.
// -----------------------------------------------------------------------
class FrequencyCounts;
double lwcUecEstimate(double sampleUec, double sampleRowCnt, 
                      double estTotalRowCnt, FrequencyCounts *fi,
                      double DshMax, double &coeffOfVar, double &Duj, double &Dsh);

// -----------------------------------------------------------------------
// Make a dual histid by toggling the last digit: even to odd, odd to even.
// -----------------------------------------------------------------------
inline ULng32 DualHistid(ULng32 histid)
{
  if (histid & 0x1)
    return (histid & 0xFFFFFFFE);
  else
    return (histid | 0x1);
}

// -----------------------------------------------------------------------
// Defines to extract individual bytes from an unsigned long.
// -----------------------------------------------------------------------
#define byte0(value) (Int32)((value & 0xFF000000) >> 24)
#define byte1(value) (Int32)((value & 0x00FF0000) >> 16)
#define byte2(value) (Int32)((value & 0x0000FF00) >> 8)
#define byte3(value) (Int32)((value & 0xFF))

inline ULng32 roundup4(ULng32 value)
{
  ULng32 floor = value & 0xFFFFFFFC;

  return((value != floor) ? (floor + 4) : value);
}

// LCOV_EXCL_START :cnu
inline ULng32 roundup8(ULng32 value)
{
  ULng32 floor = value & 0xFFFFFFF8;

  return((value != floor) ? (floor + 8) : value);
}
// LCOV_EXCL_STOP


// -----------------------------------------------------------------------
// Class for managing an array of pointers.
//
// HSPtrObj<T> *t = new HSPtrObj<T>[100];
// delete [] t will delete each individual element which pt_ points to.
// -----------------------------------------------------------------------
// LCOV_EXCL_START :cnu  Used only by HSCursor::buildNAType(), which is cnu
template <class T> class HSPtrObj {

public:

  HSPtrObj()
    : pt_(NULL) {}

  ~HSPtrObj()
    {
      delete pt_;
      pt_ = NULL;
    }

  // Add a conversion operator to T* if needed.
  // operator T*() { return pt_; }

  // pt points to a single object.
  T *pt_;
};
// LCOV_EXCL_STOP

template <class T> class HSPtrArray {

public:

  HSPtrArray()
    : pt_(NULL), len_(0) {}

  ~HSPtrArray()
    {
      delete [] pt_;
      pt_ = NULL;
    }

  // Add a conversion operator to T* if needed.
  // operator T*() { return pt_; }

  // pt points to an array of T.
  T *pt_;
  Lng32 len_;
};

void ConvWcharToHexadecimal(const NAWchar *source,
                            ULng32 sourceLength,
                            NAString &output);

COM_VERSION getTableSchemaVersion(const NAString& tableName);

//
Int64 getDefaultSampleSize(Int64 tblRowCount);
Int64 getDefaultSlidingSampleSize(Int64 tblRowCount);

//=============================================================================
// time related routines for use w/ readTime and statsTime
//
// Guardian JulianTimestamp time of UNIX "epoch", 00:00:00 Jan 1, 1970
//const Int64 HS_EPOCH_TIMESTAMP=210866760000000000LL;
const Int64 HS_EPOCH_TIMESTAMP=COM_EPOCH_TIMESTAMP;

Int64 hs_getBaseTime();
Int64 hs_getEpochTime();
Int64 hs_getEpochTime(Int64);
char *hs_formatTimestamp(char *);
char *hs_formatTimestamp(Int64, char *);
//=============================================================================

Int64 getTimeDiff(NABoolean reset = FALSE);

//==========================================================================
// This method returns the location of histogram tables.
// If the location is to be returned for an InMemory table, then it returns
// it based on the volatile schema location.
// If CQD HISTOGRAMS_SCHEMA is set and not null, then its contents are 
// returned as the location.
// Otherwise, the input regularLocation is returned.
//
// INPUT: regularLocation: contains the location that was generated by
//                         the caller. See caller for how this is generated.
//        inMemObj:        if this is an InMemory object.
//==========================================================================
NAString getHistogramsTableLocation(
     NAString regularLocation, NABoolean inMemObj);

//==========================================================================
// getRowCountForFetchFuncs() - this function returns the rowcount based
//                              on whether this is NT, Linux, or NSK.
//==========================================================================
double getRowCountForFetchFuncs(HSTableDef *tabDef, NABoolean &isEstimate);

// RIAA class to propagate WMS monitoring CQD values to secondary compiler, and
// reset them when the object goes out of scope. Used in entry points to Update Stats.
//
// NOTE: When an instance of this class is declared in the same scope as an instance
//       of HSErrorCatcher, declare the WMSController variable BEFORE the 
//       HSErrorCatcher. This ensures that the HSErrorCatcher destructor will be
//       called first, and avoids the situation where the calls to HSFuncExecQuery
//       in WMSController's destructor would cause the CLI diagnostics area to be
//       cleared before HSErrorCatcher's dtor had a chance to extract any errors
//       from it.
class WMSController
{
  public:
    WMSController()
      : needReset_(FALSE),
        needChildReset_(FALSE)
      {
        // Turn CQDs off in our secondary compiler if necessary.
        if (CmpCommon::getDefault(WMS_QUERY_MONITORING) == DF_OFF)
          {
            HSFuncExecQuery("CONTROL QUERY DEFAULT WMS_QUERY_MONITORING 'OFF'");
            needReset_ = TRUE;
          }
        if (CmpCommon::getDefault(WMS_CHILD_QUERY_MONITORING) == DF_OFF)
          {
            HSFuncExecQuery("CONTROL QUERY DEFAULT WMS_CHILD_QUERY_MONITORING 'OFF'");
            needChildReset_ = TRUE;
          }
      }

    ~WMSController()
      {
        // Restore original settings if they were changed.
        if (needReset_)
          HSFuncExecQuery("CONTROL QUERY DEFAULT WMS_QUERY_MONITORING 'RESET'");
        if (needChildReset_)
          HSFuncExecQuery("CONTROL QUERY DEFAULT WMS_CHILD_QUERY_MONITORING 'RESET'");
      }

  private:
    NABoolean needReset_;
    NABoolean needChildReset_;
};

#endif /* HSUTIL_H */
