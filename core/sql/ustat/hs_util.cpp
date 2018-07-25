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
 * File:         hs_util.C
 * Description:  Utility functions.
 * Created:      03/25/96
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */


#define HS_FILE "hs_util"

#include "Platform.h"

#define  SQLPARSERGLOBALS_NADEFAULTS            // must be first

#include <math.h>
#include <time.h>

#include "hs_util.h"
#include "hs_log.h"
#include "hs_globals.h"
#include "CompException.h"                      // CmpInternalException()
#include "ComCextdecs.h"                        // NA_JulianTimestamp()
#include "NAHeap.h"                             // For NADELETEARRAY.
#include "CmpContext.h"
#include "CmpSeabaseDDL.h"


#include "ComSmallDefs.h"
#include "NLSConversion.h"
#include "SqlParserGlobals.h"                   // must be last #include

static NAWString appendFraction (Lng32 scale);
static unsigned char toHexDecimalDigit(unsigned char c)
  {
    return ( c <= 9 ) ? c + '0' : c - 10 + 'A';
  }


NABoolean isSpecialObject(const NAString &tableName)
{
    return ((tableName == "HISTOGRM")   ||
            (tableName == "HISTINTS")   ||
            (tableName == "HISTOGRAMS") ||
            (tableName == "HISTOGRAM_INTERVALS") ||
            (tableName == HBASE_HIST_NAME)       ||
            (tableName == HBASE_HISTINT_NAME)
           );
}

NABoolean isSpecialObject(const ComObjectName& objectName)
{
    if ( isSpecialObject(objectName.getObjectNamePartAsAnsiString()) )
      return TRUE;

    return isHBaseMeta(objectName);
}

NABoolean isHBaseUmdHistograms(const ComObjectName& objectName)
{
    return (
             HSGlobalsClass::isHBaseUMDHistogram(
                   objectName.getObjectNamePart().getInternalName()
                                                 ) &&
             HSGlobalsClass::isTrafodionCatalog(
                   objectName.getCatalogNamePart().getInternalName()
                                               )
           );
}

NABoolean isHBaseMeta(const ComObjectName& objectName)
{
    return
    ( CmpSeabaseDDL::isSeabaseMD (objectName) || 
      CmpSeabaseDDL::isSeabasePrivMgrMD(objectName) ||
      isHBaseUmdHistograms(objectName)
    );
}

NABoolean isHBaseMeta(const QualifiedName& qualifiedName)
{
    return
    ( qualifiedName.isSeabaseMD() || qualifiedName.isSeabasePrivMgrMD() ||
      isHBaseUmdHistograms(qualifiedName)
    );
}

NABoolean isHBaseUmdHistograms(const QualifiedName& qualifiedName)
{
   return (
             HSGlobalsClass::isHBaseUMDHistogram(
                   qualifiedName.getQualifiedNameAsAnsiString()
                                                 ) &&
             HSGlobalsClass::isTrafodionCatalog(qualifiedName.getCatalogName())
           );
}

NABoolean isSpecialObject(const QualifiedName& qualifiedName)
{
    if ( isSpecialObject(ToAnsiIdentifier(qualifiedName.getObjectName())) )
      return TRUE;

    return isHBaseMeta(qualifiedName);
}

// Convert a fully-qualified Trafodion table name to the name used for its
// backing sample table, which is a Hive table. Hive table names only allow
// letters, digits, and underscores, and are case-insensitive.
// @ZXbl -- need to do some kind of conversion for other chars besides periods,
//          to handle delimited ids.
void TrafToHiveSampleTableName(NAString& name)
{
  size_t len = name.length();
  const char* oldName = name.data();
  char* newName = new(STMTHEAP) char[len];
  for (size_t i=0; i<len; i++)
    {
      if (oldName[i] == '.')
        newName[i] = '_';
      else
        newName[i] = oldName[i];
    }
  strcpy((char*)newName+len, "_SAMPLE");
  name = newName;
}

// -----------------------------------------------------------------------
// Return the root of fx = 0 for uec estimate.
// -----------------------------------------------------------------------
double xValue( const double x0
             , const double n
             )
{
  double x = x0;
  double fx, gx, t;

  for (Int32 i = 0; i < 100; i++)
    {
      t = exp(-n/x);
      fx = x * (1 - t) - x0;
      // ok to return if fx is very close to 0.
      if (fx < 1e-8 &&
          fx > -1e-8)
        break;

      gx = 1 - (1 + n/x) * t;
      x = x - fx / gx;
    }
  return x;
}

// Compute UEC using a first-order unsmoothed jacknife estimator.
//
// For details on this method, see the paper Estimating the Number of
// Classes in a Finite Population, IBM Research Report, by Haas and
// Stokes, pg. 6.
//
static double computeUJack(double d, double n, double N,
                           double q, double f1)
{
  // input parameters:
  //  d - number of distinct values in sample
  //  n - number of rows in sample
  //  N - number of rows in full table
  //  q - sampling fraction (e.g., .01 for 1% sample)
  // f1 - number of distinct values that occur exactly once in sample

  // Duj - unsmoothed jacknife estimate of D, the number of distinct
  // values in the full table
  //
  double Duj = d / (1.0 - ((1.0 - q)*f1) / n);

  // return min(Duj, N), to make sure that UEC estimate never
  // exceeds the row count
  //
  return Duj > N ? N : Duj;
}

//
// Compute an estimate, gamma squared, of the squared coefficient
// of variation of the class sizes (i.e., the sizes, or number of
// rows, with each unique value in the sample).  This quantity is
// used by the second order unsmoothed jacknife estimator.
//
// For more information, see the paper Estimating the Number of
// Classes in a Finite Population, IBM Research Report, by Haas and
// Stokes, pgs. 7-8.
//
static double gamma_2(FrequencyCounts &fi, double D, double n, double N)
{
  double sum = 0;
  double n2 = n*n;
  ULng32 sampleRowCnt = (ULng32) n;
  for (ULng32 i=1; i<=sampleRowCnt; i++)
    {
      ULng32 cnt = fi[i];

      if (cnt) sum = sum + ((double)i * (double)(i-1) * (double)cnt/n2);
    }

  double g2 = D * sum + D/N - 1;

  return g2 < 0 ? 0 : g2;
}

// Compute UEC using a second-order unsmoothed jacknife estimator.
//
// For details on this method, see the paper Estimating the Number of
// Classes in a Finite Population, IBM Research Report, by Haas and
// Stokes, pg. 9.
//
static double computeUJack2(double d, double n, double N,
                            double q, FrequencyCounts &fi, double D)
{
  // input parameters:
  //  d - number of distinct values in sample
  //  n - number of rows in sample
  //  N - number of rows in full table
  //  q - sampling fraction (e.g., .01 for 1% sample)
  // fi - sample frequency counts
  //  D - number of distinct values in full table (estimate)

  // Duj2 - second order unsmoothed jacknife estimate of D, the number
  // of distinct values in the full table
  //
  double g2 = gamma_2(fi, D, n, N);
  double numer = d - (fi[1] * (1-q) * log(1-q) * g2)/q;
  double denom = 1 - (fi[1] * (1-q) / n);
  double Duj2 = numer / denom;

  // return min(Duj2, N), to make sure that UEC estimate never
  // exceeds the row count
  //
  return Duj2 > N ? N : Duj2;
}

//
// Compute UEC using a variant of Shlosser's method.
//
// For details on this method, see the paper Estimating the Number of
// Classes in a Finite Population, IBM Research Report, by Haas and
// Stokes, pg. 14.
//
static double computeShloss(double d, Int64 n, double N,
                            double q, FrequencyCounts &fi)
{
  // input parameters:
  //  d - number of distinct values in sample
  //  n - number of rows in sample
  //  N - number of rows in full table
  //  q - sampling fraction (e.g., .01 for 1% sample)
  // fi - frequency counts (for each i, the number of distinct
  //      values in the sample that occur exactly i times)

  // the Shloss estimator, Dsh, is defined as follows:
  //
  // Dsh = d + fi[1] * (n2i/d2i) * (ni/di)^2
  //
  // where d and fi are as described above.  the other terms
  // (ni, n2i, di, d2i) are summations that are accumulated as
  // i goes from 1 to n.  the summations involve i, q, and fi[i]
  // (see the reference above for details).

  // the summations include (1-q)^i, (1-q^2)^i, and (1+q)^i.  these
  // values are stored in the following variables at each step in
  // the summation
  //
  double qi = 1;         // (1-q)^i
  double q2i = 1;        // (1-q^2)^i
  double q3i = 1;        // (1+q)^i

  double di = 0, d2i = 0;
  double ni = 0, n2i = 0;

  // used to update qi, q2i and q3i, at each step in the summation
  //
  double q1 = 1.0 - q;
  double q2 = 1.0 - q*q;
  double q3 = 1.0 + q;

  double qsquared = q*q;

  // for each i value from 1 to sample size, accumulate
  // summations ni, di, n2i, d2i
  //
  for (Int64 i=1; i<=n; i++)
    {
      double idbl = (double) i;
      ULng32 cnt = fi[i];

      if (cnt) di = di + qi * cnt * idbl * q;
      qi = qi * q1;
      if (cnt) ni = ni + qi * cnt;

      if (cnt) n2i = n2i + idbl * qsquared * q2i * cnt;
      q2i = q2i * q2;
      q3i = q3i * q3;
      if (cnt) d2i = d2i + qi * (q3i - 1) * cnt;

      // break if qi or q2i get too small
      if (qi < .000001 || q2i < .000001) break;
    }

  double Dsh =
        (fi[1]==0) ? d : (d + fi[1] * (n2i/d2i) * ((ni/di) * (ni/di)));

  // return min(Dsh, N), to ensure UEC estimate doesn't exceed row count
  //
  return Dsh > N ? N : Dsh;
}

//
// Compute the coefficient of variation of the class sizes
// in the sample.  (Each distinct value in the sample represents
// a "class" and the class size is the number of times the
// class value appears in the sample.)
//
// The coefficient of variation is a measure of the skew of
// the class sizes.
//
static double computeCoeffOfVar(double d, Int64 n, double D, double N,
                                FrequencyCounts &fi)
{
  // input parameters:
  //  d - number of distinct values in sample
  //  n - number of rows in sample
  //  D - number of distinct values in full table (estimate)
  //  N - number of rows in full table
  // fi - frequency counts (for each i, the number of distinct
  //      values in the sample that occur exactly i times)

  double sum = 0;

  for (Int64 i=1; i<=n; i++)
    sum += (double) (i * (i-1) * fi[i]);

  double est = (D / (double) (n * n)) * sum + D/N - 1;

  return est > 0 ? est : 0;
}

double lwcUecEstimate(double sampleUec, double sampleRowCnt,
                      double estTotalRowCnt, FrequencyCounts *fi,
                      double DshMax, double &coeffOfVar, double &Duj, double &Dsh)
{
  // q is the sample fraction, e.g., .01 for a 1% sample
  double q = sampleRowCnt / estTotalRowCnt;

  // Duj (first order unsmoothed jacknife) is used as the
  // estimate of D, when computing Duj2
  //
  Duj = computeUJack(sampleUec, sampleRowCnt, estTotalRowCnt,
                     q, (double) ((*fi)[1]));

  // Duj2 - second order unsmoothed jacknife
  //
  double Duj2 = computeUJack2(sampleUec, sampleRowCnt, estTotalRowCnt,
                              q, *fi, Duj);

  // Dsh - Shloss estimate
  //
  Dsh = computeShloss(sampleUec, (Int64) sampleRowCnt, estTotalRowCnt,
                      q, *fi);

  coeffOfVar = computeCoeffOfVar(sampleUec, (Int64) sampleRowCnt,
                                 Duj, estTotalRowCnt, *fi);

  // at this point, we have two estimates of D (the actual UEC), Duj2
  // and Dsh.  Dlwc is a weighted combination of these two estimates.
  // two weights are computed, DujWt and DshWt.  the sum of these
  // weights is 1, and each is in the range 0-1.  Dlwc is
  // DujWt * Duj2 + DshWt * Dsh.
  //
  // DshWt is coeffOfVar / DshMax, with a max value of 1.  (DshMax has
  // a default value of 50.) So DshWt becomes larger as the coefficient
  // of variation gets larger, and is capped at 1 for coeffOfVar values
  // above DshMax.  This weights Dsh more as the skew of the data increases.
  //
  // DujWt is 1 - DshWt.

  double DshWt = coeffOfVar / DshMax;
  if (DshWt > 1.0) DshWt = 1;
  double DujWt = 1.0 - DshWt;

  double Dlwc = ceil(DujWt*Duj2 + DshWt*Dsh);

  // make sure Dlwc <= max(Duj2,Dsh)
  //
  if (Dlwc > Duj2 && Dlwc > Dsh)
    {
      Dlwc = (Duj2 < Dsh) ? Dsh : Duj2;
    }

  // pass Duj2 estimate back to caller through ref param Duj
  //
  Duj = Duj2;

  return Dlwc;
}


Lng32 FormatRow(const HSColumnStruct *srcDesc,
               const char *src,
               HSDataBuffer &target)
{
    const Lng32 REC_INTERVAL = REC_MIN_INTERVAL;
    Lng32 retcode = 0;
    const Lng32 workBufLen = 4096;
    NAWchar workBuf[workBufLen];
    Lng32 type = srcDesc->datatype;
    NAWString wStr;

    //The input source buffer will always be in the following form and will
    //contain unicode format. We need to separate the buffer accordingly.
    //         |-------|--------------|
    //  SRC -->|  LEN  |  DATA        |
    //         |-------|--------------|
    short inDataLen;
    memcpy((char*)&inDataLen, src, sizeof(short));
    const NAWchar *inData = (NAWchar*)(src + sizeof(short));


    if (DFS2REC::isInterval(type))
      type = REC_INTERVAL;

    if (DFS2REC::isAnyCharacter(type))
      {
         wStr = WIDE_("'");
         for (short i = 0; i < inDataLen/sizeof(NAWchar); i++)
           {
             if (inData[i] == NAWchar('\0'))
               wStr += NAWchar('\1');                /* convert x00 to x01      */
             else
               {
                 wStr += inData[i];
                 if (inData[i] == NAWchar('\''))
                   wStr.append(WIDE_("'"));
               }
           }
         wStr.append(WIDE_("'"));

         target = wStr.data();
      }
    else
      {
        switch (type)
          {
            case REC_DATETIME:
              {
                switch (srcDesc->precision)
                  {
                    case REC_DTCODE_DATE:
                      {
                        wStr = WIDE_("DATE '");
                        wStr.append(inData, inDataLen/sizeof(NAWchar));
                        wStr.append(WIDE_("'"));
                        break;
                      }

                    case REC_DTCODE_TIME:
                      {
                        wStr = WIDE_("TIME '");
                        wStr.append(inData, inDataLen/sizeof(NAWchar));
                        wStr.append(WIDE_("'"));
                        break;
                      }

                    case REC_DTCODE_TIMESTAMP:
                      {
                        wStr = WIDE_("TIMESTAMP '");
                        wStr.append(inData, inDataLen/sizeof(NAWchar));
                        wStr.append(WIDE_("'"));
                        break;
                      }

// Here begin a number of cases that are only possible with MP datetime types.
                    case REC_DTCODE_YEAR:
                      {
                        wStr = WIDE_("DATETIME '");
                        wStr.append(inData, inDataLen/sizeof(NAWchar));
                        wStr.append(WIDE_("' YEAR"));
                        break;
                      }

                    case REC_DTCODE_YEAR_MONTH:
                      {
                        wStr = WIDE_("DATETIME '");
                        wStr.append(inData, inDataLen/sizeof(NAWchar));
                        wStr.append(WIDE_("' YEAR TO MONTH"));
                        break;
                      }

                    case REC_DTCODE_YEAR_HOUR:
                      {
                        wStr = WIDE_("DATETIME '");
                        wStr.append(inData, inDataLen/sizeof(NAWchar));
                        wStr.append(WIDE_("' YEAR TO HOUR"));
                        break;
                      }

                    case REC_DTCODE_YEAR_MINUTE:
                      {
                        wStr = WIDE_("DATETIME '");
                        wStr.append(inData, inDataLen/sizeof(NAWchar));
                        wStr.append(WIDE_("' YEAR TO MINUTE"));
                        break;
                      }

                    case REC_DTCODE_MONTH:
                      {
                        wStr = WIDE_("DATETIME '");
                        wStr.append(inData, inDataLen/sizeof(NAWchar));
                        wStr.append(WIDE_("' MONTH"));
                        break;
                      }

                    case REC_DTCODE_MONTH_DAY:
                      {
                        wStr = WIDE_("DATETIME '");
                        wStr.append(inData, inDataLen/sizeof(NAWchar));
                        wStr.append(WIDE_("' MONTH TO DAY"));
                        break;
                      }

                    case REC_DTCODE_MONTH_HOUR:
                      {
                        wStr = WIDE_("DATETIME '");
                        wStr.append(inData, inDataLen/sizeof(NAWchar));
                        wStr.append(WIDE_("' MONTH TO HOUR"));
                        break;
                      }

                    case REC_DTCODE_MONTH_MINUTE:
                      {
                        wStr = WIDE_("DATETIME '");
                        wStr.append(inData, inDataLen/sizeof(NAWchar));
                        wStr.append(WIDE_("' MONTH TO MINUTE"));
                        break;
                      }

                    case REC_DTCODE_MONTH_SECOND:
                      {
                        wStr = WIDE_("DATETIME '");
                        wStr.append(inData, inDataLen/sizeof(NAWchar));
                        if (srcDesc->scale == 0)
                          wStr.append(WIDE_("' MONTH TO SECOND"));
                        else
                          {
                            wStr.append(WIDE_("' MONTH TO "));
                            wStr.append(appendFraction(srcDesc->scale));
                          }
                        break;
                      }

                    case REC_DTCODE_DAY:
                      {
                        wStr = WIDE_("DATETIME '");
                        wStr.append(inData, inDataLen/sizeof(NAWchar));
                        wStr.append(WIDE_("' DAY"));
                        break;
                      }

                    case REC_DTCODE_DAY_HOUR:
                      {
                        wStr = WIDE_("DATETIME '");
                        wStr.append(inData, inDataLen/sizeof(NAWchar));
                        wStr.append(WIDE_("' DAY TO HOUR"));
                        break;
                      }

                    case REC_DTCODE_DAY_MINUTE:
                      {
                        wStr = WIDE_("DATETIME '");
                        wStr.append(inData, inDataLen/sizeof(NAWchar));
                        wStr.append(WIDE_("' DAY TO MINUTE"));
                        break;
                      }

                    case REC_DTCODE_DAY_SECOND:
                      {
                        wStr = WIDE_("DATETIME '");
                        wStr.append(inData, inDataLen/sizeof(NAWchar));
                        if (srcDesc->scale == 0)
                          wStr.append(WIDE_("' DAY TO SECOND"));
                        else
                          {
                            wStr.append(WIDE_("' DAY TO "));
                            wStr.append(appendFraction(srcDesc->scale));
                          }
                        break;
                      }

                    case REC_DTCODE_HOUR:
                      {
                        wStr = WIDE_("DATETIME '");
                        wStr.append(inData, inDataLen/sizeof(NAWchar));
                        wStr.append(WIDE_("' HOUR"));
                        break;
                      }

                    case REC_DTCODE_HOUR_MINUTE:
                      {
                        wStr = WIDE_("DATETIME '");
                        wStr.append(inData, inDataLen/sizeof(NAWchar));
                        wStr.append(WIDE_("' HOUR TO MINUTE"));
                        break;
                      }

                    case REC_DTCODE_MINUTE:
                      {
                        wStr = WIDE_("DATETIME '");
                        wStr.append(inData, inDataLen/sizeof(NAWchar));
                        wStr.append(WIDE_("' MINUTE"));
                        break;
                      }

                    case REC_DTCODE_MINUTE_SECOND:
                      {
                        wStr = WIDE_("DATETIME '");
                        wStr.append(inData, inDataLen/sizeof(NAWchar));
                        if (srcDesc->scale == 0)
                          wStr.append(WIDE_("' MINUTE TO SECOND"));
                        else
                          {
                            wStr.append(WIDE_("' MINUTE TO "));
                            wStr.append(appendFraction(srcDesc->scale));
                          }
                        break;
                      }

                    case REC_DTCODE_SECOND:
                      {
                        wStr = WIDE_("DATETIME '");
                        wStr.append(inData, inDataLen/sizeof(NAWchar));
                        if (srcDesc->scale == 0)
                          wStr.append(WIDE_("' SECOND"));
                        else
                          {
                            wStr.append(WIDE_("' SECOND TO "));
                            wStr.append(appendFraction(srcDesc->scale));
                          }
                        break;
                      }

                    default:
                      {
                        HS_ASSERT(FALSE);
                        break;
                      }
                  }

                target = wStr.data();
                break;
              }

            case REC_INTERVAL:
              {
                //The INTERVAL may contain spaces and the negative sign
                //in front of the number.
                //We must capture the sign, but do not copy the extra character.
                Int32 spaceLen = 0;
                NABoolean signPresent = FALSE;
                spaceLen = wcsspn(inData, L" ");
                if (inData[spaceLen] == L'-')
                  {
                    signPresent = TRUE;
                    wStr = WIDE_("INTERVAL -'");
                  }
                else
                  wStr = WIDE_("INTERVAL '");
                for (short i=0; i < spaceLen; i++)
                  wStr.append(L" ");
                wStr.append( (inData+((signPresent) ? 1 : 0)+spaceLen),
			     (inDataLen/sizeof(NAWchar)-((signPresent) ? 1 : 0)-spaceLen));
                wStr.append(WIDE_("'"));

                switch (srcDesc->datatype)
                  {
                    case REC_INT_YEAR:
                      {
                        na_wsprintf(workBuf, WIDE_("%s YEAR(%d)"), wStr.data(), srcDesc->precision);
                        break;
                      }
                    case REC_INT_YEAR_MONTH:
                      {
                        na_wsprintf(workBuf, WIDE_("%s YEAR(%d) TO MONTH"), wStr.data(), srcDesc->precision);
                        break;
                      }
                    case REC_INT_MONTH:
                      {
                        na_wsprintf(workBuf, WIDE_("%s MONTH(%d)"), wStr.data(), srcDesc->precision);
                        break;
                      }
                    case REC_INT_DAY:
                      {
                        na_wsprintf(workBuf, WIDE_("%s DAY(%d)"), wStr.data(), srcDesc->precision);
                        break;
                      }
                    case REC_INT_DAY_HOUR:
                      {
                        na_wsprintf(workBuf, WIDE_("%s DAY(%d) TO HOUR"), wStr.data(), srcDesc->precision);
                        break;
                      }
                    case REC_INT_DAY_MINUTE:
                      {
                        na_wsprintf(workBuf, WIDE_("%s DAY(%d) TO MINUTE"), wStr.data(), srcDesc->precision);
                        break;
                      }
                    case REC_INT_DAY_SECOND:
                      {
                        na_wsprintf(workBuf, WIDE_("%s DAY(%d) TO SECOND(%d)"), wStr.data(), srcDesc->precision, srcDesc->scale);
                        break;
                      }
                    case REC_INT_HOUR:
                      {
                        na_wsprintf(workBuf, WIDE_("%s HOUR(%d)"), wStr.data(), srcDesc->precision);
                        break;
                      }
                    case REC_INT_HOUR_MINUTE:
                      {
                        na_wsprintf(workBuf, WIDE_("%s HOUR(%d) TO MINUTE"), wStr.data(), srcDesc->precision);
                        break;
                      }
                    case REC_INT_HOUR_SECOND:
                      {
                        na_wsprintf(workBuf, WIDE_("%s HOUR(%d) TO SECOND(%d)"), wStr.data(), srcDesc->precision, srcDesc->scale);
                        break;
                      }
                    case REC_INT_MINUTE:
                      {
                        na_wsprintf(workBuf, WIDE_("%s MINUTE(%d)"), wStr.data(), srcDesc->precision);
                        break;
                      }
                    case REC_INT_MINUTE_SECOND:
                      {
                        na_wsprintf(workBuf, WIDE_("%s MINUTE(%d) TO SECOND(%d)"), wStr.data(), srcDesc->precision, srcDesc->scale);
                        break;
                      }
                    case REC_INT_SECOND:
                      {
                        na_wsprintf(workBuf, WIDE_("%s SECOND(%d, %d)"), wStr.data(), srcDesc->precision, srcDesc->scale);
                        break;
                      }
                    default:
                      {
                        HS_ASSERT(FALSE);
                        break;
                      }
                  }

                target = workBuf;
                break;
              }

            default:
              {
                wStr.replace(0, wStr.length(), inData, inDataLen/sizeof(NAWchar));
                target = wStr.data();
                break;
              }
          }
      }

    return retcode;
  }

void ConvWcharToHexadecimal(const NAWchar *source,
                            ULng32 sourceLength,
                            NAString &output)
  {
    unsigned char ucs2Hex[9];
    unsigned char ascii2Hex[7];
    unsigned char ascii[2];

    ucs2Hex[0] = ' ';
    ucs2Hex[1] = '0';
    ucs2Hex[2] = 'x';
    ucs2Hex[7] = ' ';
    ucs2Hex[8] = 0;

    ascii2Hex[0] = ' ';
    ascii2Hex[1] = '0';
    ascii2Hex[2] = 'x';
    ascii2Hex[5] = ' ';
    ascii2Hex[6] = 0;

    ascii[1] = 0;

    output = "";
    for (ULng32 i = 0; i < sourceLength; i++)
      {
        if ( source[i] <= (NAWchar)0xFF )
          {
             if (isprint((unsigned char)source[i]))
               {
                 ascii[0] = (unsigned char)source[i];
                 output += (const char*)ascii;
               }
             else
               {
                 ascii2Hex[3] = toHexDecimalDigit((unsigned char)((source[i] >> 4) & 0xF));
                 ascii2Hex[4] = toHexDecimalDigit((unsigned char)((source[i]) & 0xF));
                 output += (const char*)ascii2Hex;
               }
          }
        else
          {
            ucs2Hex[3] = toHexDecimalDigit((unsigned char)((source[i] >> 12) & 0xF));
            ucs2Hex[4] = toHexDecimalDigit((unsigned char)((source[i] >> 8) & 0xF));
            ucs2Hex[5] = toHexDecimalDigit((unsigned char)((source[i] >> 4) & 0xF));
            ucs2Hex[6] = toHexDecimalDigit((unsigned char)((source[i]) & 0xF));
            output += (const char*)ucs2Hex;
          }
      }
  }

NAString getTableName(const NAString name, const ComAnsiNameSpace nameSpace)
  {
    switch(nameSpace)
      {
        case COM_TABLE_NAME:
          return name;
        case COM_IUD_LOG_TABLE_NAME:
          return "TABLE(IUD_LOG_TABLE " + name + " )";
        case COM_INDEX_NAME:
          return "TABLE(INDEX_TABLE " + name + " )";
        default:
          return name;
      }
  }


static NAWString appendFraction (Lng32 scale)
  {
    NAWString wStr;

    switch (scale)
      {
        case 1: wStr = WIDE_("FRACTION(1)"); break;
        case 2: wStr = WIDE_("FRACTION(2)"); break;
        case 3: wStr = WIDE_("FRACTION(3)"); break;
        case 4: wStr = WIDE_("FRACTION(4)"); break;
        case 5: wStr = WIDE_("FRACTION(5)"); break;
        case 6: wStr = WIDE_("FRACTION(6)"); break;
        default:
            NABoolean invalid_FRACTION = FALSE;
            HS_ASSERT(invalid_FRACTION);
            break;
      }

    return wStr;
  }

// Calculate default sample size for table.
Int64 getDefaultSampleSize(Int64 tblRowCount)
{
   Int64 result = (Int64) ceil(convertInt64ToDouble(tblRowCount) *
                          CmpCommon::getDefaultNumeric(HIST_DEFAULT_SAMPLE_RATIO));
   result = MINOF(result, (Int64)CmpCommon::getDefaultLong(HIST_DEFAULT_SAMPLE_MAX));
   return result;
}

// Calculate default sample size for table, given its cardinality and current
// CQD values.
Int64 getDefaultSlidingSampleSize(Int64 tblRowCount)
{
   // Minimum sample size.
   Int64 minSampleRows = (Int64)CmpCommon::getDefaultLong(HIST_DEFAULT_SAMPLE_MIN);
   // Maximum sample size.
   Int64 maxSampleRows = (Int64)CmpCommon::getDefaultLong(HIST_DEFAULT_SAMPLE_MAX);
   // Minimum table size for which to use sampling.
   Int64 minTblRows = HSGlobalsClass::getMinRowCountForSample();
   // Minimum table size for which to use lowest sampling rate.
   Int64 minTblRowsLowSamp = HSGlobalsClass::getMinRowCountForLowSample();

   // We won't ordinarily be sampling if the following condition is true, but
   // this function is called when the bulk load utility is creating a persistent
   // Hive sample table, and in that case the sample will be done anyway.
   if (tblRowCount < minTblRows)
     return tblRowCount;

   Int64 sampleRows;
   if (tblRowCount < minTblRowsLowSamp)
     sampleRows = minSampleRows;
   else
     {
       sampleRows = (Int64)
                      ((double)CmpCommon::getDefaultNumeric(HIST_DEFAULT_SAMPLE_RATIO)
                       * tblRowCount);
       if (sampleRows < minSampleRows)
         sampleRows = minSampleRows;
       else if (sampleRows > maxSampleRows)
         sampleRows = maxSampleRows;
     }

   return sampleRows;
}

// use CATMAN API to get the schema version of a table
COM_VERSION getTableSchemaVersion(const NAString& tableName)
  {
    return COM_VERS_CURR_SCHEMA;
  }

/***************************************************************************/
/* METHOD:  hs_getBaseTime()                                               */
/* PURPOSE: Gets the base time (in seconds) for zero hour Jan 1, 1970      */
/*          This is an efficient implementation since the calls            */
/*          time() and gettimeofday() are extremely slow.                  */
/* NOTES  : An exception occurs if there is an error [debug build only]    */
/*          This method is overloaded; see below                           */
/* INPUT  : None                                                           */
/* RETURNS: current time in seconds                                        */
/***************************************************************************/
Int64 hs_getBaseTime()
  {
#ifdef _DEBUG
    static Int64 baseTs;

        HSLogMan *LM = HSLogMan::Instance();
        // Verify that the number that we have pre-computed (HS_EPOCH_TIMESTAMP) is the correct value
        // Adjust the time to time since zero hour Jan 1, 1970
        short baseTsStr[] = {1970, 1, 1, 0, 0, 0, 0, 0};
        short error;
        baseTs = COMPUTETIMESTAMP(baseTsStr, &error);
        if (error)
          {
            sprintf(LM->msg,
                    "INTERNAL ERROR: error in COMPUTETIMESTAMP: %d", error);
          }
        else
        if (baseTs != HS_EPOCH_TIMESTAMP)
          {
            sprintf(LM->msg,
                    "INTERNAL ERROR: wrong baseTS in getEpochTime(): " PF64", HS_EPOCH_TIMESTAMP=" PF64"",
                    baseTs, HS_EPOCH_TIMESTAMP);
            error = 1;
          }
        if (error)
        {
            if (LM->LogNeeded())
              {
                LM->Log(LM->msg);
              }
            throw CmpInternalException("failure in getEpochTime()",
                                       __FILE__, __LINE__);
        }
        // baseTs == HS_EPOCH_TIMESTAMP;

    return baseTs / 1000000;
#else
    return HS_EPOCH_TIMESTAMP / 1000000;
#endif
  }

/***************************************************************************/
/* METHOD:  hs_getEpochTime()                                              */
/* PURPOSE: Gets the current time (in seconds) since zero hour Jan 1, 1970 */
/*          This is an efficient implementation since the calls            */
/*          time() and gettimeofday() are extremely slow.                  */
/* NOTES  : An exception occurs if there is an error [debug build only]    */
/*          This method is overloaded; see below                           */
/* INPUT  : None                                                           */
/* RETURNS: current time in seconds                                        */
/***************************************************************************/
Int64 hs_getEpochTime()
  {
#ifdef _DEBUG
    Int64 jt;
    static Int64 baseTs;

    if (! baseTs)
      {
        HSLogMan *LM = HSLogMan::Instance();
        // Verify that the number that we have pre-computed (HS_EPOCH_TIMESTAMP) is the correct value
        // Adjust the time to time since zero hour Jan 1, 1970
        short baseTsStr[] = {1970, 1, 1, 0, 0, 0, 0, 0};
        short error;
        baseTs = COMPUTETIMESTAMP(baseTsStr, &error);
        if (error)
          {
            sprintf(LM->msg,
                    "INTERNAL ERROR: error in COMPUTETIMESTAMP: %d", error);
          }
        else
        if (baseTs != HS_EPOCH_TIMESTAMP)
          {
            sprintf(LM->msg,
                    "INTERNAL ERROR: wrong baseTS in getEpochTime(): " PF64 "; HS_EPOCH_TIMESTAMP=" PF64 ,
                    baseTs, HS_EPOCH_TIMESTAMP);
            error = 1;
          }
        if (error)
        {
            if (LM->LogNeeded())
              {
                LM->Log(LM->msg);
              }
            throw CmpInternalException("failure in getEpochTime()",
                                       __FILE__, __LINE__);
        }
        // baseTs == HS_EPOCH_TIMESTAMP;
      }

    jt=NA_JulianTimestamp();
    return (jt - baseTs) / 1000000;
#else
    return (NA_JulianTimestamp() - HS_EPOCH_TIMESTAMP) / 1000000;
#endif
  }


/***************************************************************************/
/* METHOD:  hs_getEpochTime()                                              */
/* PURPOSE: Converts the Julian time passed in as argument to an Epoch Time*/
/* NOTES  : This method is overloaded; see above                           */
/* INPUT  : tm - JULIANTIMESTAMP                                           */
/* RETURNS: Epoch time corresponding to the Julian timestamp passed in     */
/*          0 - if the Julian timestamp passed in is before the UNIX Epoch */
/***************************************************************************/
Int64 hs_getEpochTime(Int64 tm)
  {
    return (tm > HS_EPOCH_TIMESTAMP ? (tm - HS_EPOCH_TIMESTAMP) / 1000000 : 0);
  }

/***************************************************************************/
/* METHOD:  hs_formatTimestamp()                                           */
/* PURPOSE: Formats the current Epoch timestamp in a TIMESTAMP(0) format   */
/*          string. The time is converted to GMT before formatting.        */
/* NOTES  : This method is overloaded; see below.                          */
/* INPUT  : -                                                              */
/* OUTPUT : time_string - the formatted Epoch Timestamp                    */
/* RETURNS: -                                                              */
/***************************************************************************/
char *hs_formatTimestamp(char *time_string)
  {
    time_t  tm = (time_t) hs_getEpochTime();
    strftime(time_string, HS_TIMESTAMP_SIZE, "%Y-%m-%d %H:%M:%S", gmtime(&tm));
    return time_string;
  }

/***************************************************************************/
/* METHOD:  hs_formatTimestamp()                                           */
/* PURPOSE: Formats the timestamp passed in in a TIMESTAMP(0) format       */
/*          The time is converted to GMT before formatting.                */
/* NOTES  : This method is overloaded; see above.                          */
/* INPUT  : tm - Epoch timestamp to be formatted                           */
/* OUTPUT : time_string - the formatted Epoch Timestamp                    */
/* RETURNS: -                                                              */
/***************************************************************************/
char *hs_formatTimestamp(Int64 tm, char *time_string)
  {
    const time_t time = (const time_t) tm;
    strftime(time_string, HS_TIMESTAMP_SIZE, "%Y-%m-%d %H:%M:%S", gmtime(&time));
    return time_string;
  }

/***********************************************/
/* METHOD:  getTimeDiff()                      */
/* PURPOSE: get time difference                */
/*          between calls.                     */
/* INPUT:   reset flag                         */
/* OUTPUT:  elapsed time in seconds            */
/***********************************************/
Int64 getTimeDiff(NABoolean reset)
  {
    static Int64 prevTm;
    Int64 tm;
    Int64 elapsed = 0;

    tm = hs_getEpochTime();
    if (!reset)
      elapsed = tm - prevTm;

    prevTm = tm;
    return elapsed;
  }

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
     NAString regularLocation, NABoolean inMemObj)
{
  NAString histLoc;

  if (inMemObj)
    {
      histLoc =
        CmpCommon::context()->sqlSession()->volatileCatalogName()
        + "."
        + CmpCommon::context()->sqlSession()->volatileSchemaName();
    }
  else
    {
      NAString catName = regularLocation;
      catName.remove(catName.first('.'));
      if ( HSGlobalsClass::isNativeHbaseCat(catName) )
        histLoc = HBASE_STATS_CATALOG "." HBASE_STATS_SCHEMA;
      else
      if (HSGlobalsClass::isHiveCat(catName))
        histLoc = HIVE_STATS_CATALOG "." HIVE_STATS_SCHEMA;
      else
        {
          CmpCommon::getDefault(HISTOGRAMS_SCHEMA, histLoc, FALSE);
          if (histLoc.isNull())
            return regularLocation;
        }
    }

  return histLoc;
}

//==========================================================================
// getRowCountForFetchFuncs() - this function returns the rowcount based
//                              on whether this is NT, Linux, or NSK.
//==========================================================================
double getRowCountForFetchFuncs(HSTableDef *tabDef, NABoolean &isEstimate)
{
  NABoolean isHbaseTable = HSGlobalsClass::isHbaseCat(tabDef->getCatName());
  NABoolean isHiveTable  = HSGlobalsClass::isHiveCat(tabDef->getCatName());

  // getRowCount below does not use SQL for Hbase and Hive tables, so there
  // is no need to set the CQD for these tables
  if (!isHbaseTable && !isHiveTable)
     HSFuncExecQuery("CONTROL QUERY DEFAULT USTAT_FETCHCOUNT_ACTIVE 'ON'");

  isEstimate = FALSE;
  Int64 rows=-1;
  // On NSK and Linux, getRowCount() will return an accurate count
  // (from DP2 file label), in all testing environments (and in almost
  //  all other cases).
  Int32 errorCode = 0;
  Int32 breadCrumb = 0;
  rows = tabDef->getRowCount(isEstimate, errorCode /* out */, breadCrumb /* out */);

  if (!isHbaseTable && !isHiveTable)
    HSFuncExecQuery("CONTROL QUERY DEFAULT USTAT_FETCHCOUNT_ACTIVE 'OFF'");

  return convertInt64ToDouble(rows);
}
