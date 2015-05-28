// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <math.h>

#include "sqludr.h"

extern "C" {

/* ADD2 */
SQLUDR_LIBFUNC SQLUDR_INT32 add2(SQLUDR_INT32 *in1,
                                 SQLUDR_INT32 *in2,
                                 SQLUDR_INT32 *out,
                                 SQLUDR_INT16 *in1Ind,
                                 SQLUDR_INT16 *in2Ind,
                                 SQLUDR_INT16 *outInd,
                                 SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*in1Ind == SQLUDR_NULL || *in2Ind == SQLUDR_NULL)
  {
    *outInd = SQLUDR_NULL;
  }
  else
  {
    (*out) = (*in1) + (*in2);
  }

  return SQLUDR_SUCCESS;
}

/* Params to the corresponding UDF are DATE (IN), DAYS(IN) and DATE(OUT) */
SQLUDR_LIBFUNC SQLUDR_INT32 addDaysToDate(SQLUDR_CHAR  *in_date,
		                          SQLUDR_INT32 *in_days,
				          SQLUDR_CHAR  *out_date,
                                          SQLUDR_INT16 *in_date_Ind,
                                          SQLUDR_INT16 *in_days_Ind,
                                          SQLUDR_INT16 *out_date_Ind,
                                          SQLUDR_TRAIL_ARGS)
{
  char year[5], month[3], date[3];
  int newDate;

  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*in_date_Ind == SQLUDR_NULL || *in_days_Ind == SQLUDR_NULL)
  {
    *out_date_Ind = SQLUDR_NULL;
  }
  else
  {
    sscanf(in_date, "%4s-%2s-%2s", year, month, date);

    /* Let's assume the simple case that the newDate is a valid
     * date */
    newDate = atoi(date) + (*in_days);

    sprintf(out_date, "%4s-%2s-%02d", year, month, newDate);
  }

  return SQLUDR_SUCCESS;

}

/*---------------------------------------------------------------------------
  ECHO: INTEGER
  --------------------------------------------------------------------------*/

/* echoINT */
SQLUDR_LIBFUNC SQLUDR_INT32 echoINT(SQLUDR_INT32 *in,
                                    SQLUDR_INT32 *out,
                                    SQLUDR_INT16 *inInd,
                                    SQLUDR_INT16 *outInd,
                                    SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
    *outInd = SQLUDR_NULL;
  else
    *out = *in;

  return SQLUDR_SUCCESS;
}

/* echoUINT */
SQLUDR_LIBFUNC SQLUDR_INT32 echoUINT(SQLUDR_UINT32 *in,
                                     SQLUDR_UINT32 *out,
                                     SQLUDR_INT16 *inInd,
                                     SQLUDR_INT16 *outInd,
                                     SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
    *outInd = SQLUDR_NULL;
  else
    *out = *in;

  return SQLUDR_SUCCESS;
}

/* echoSMALL */
SQLUDR_LIBFUNC SQLUDR_INT32 echoSMALL(SQLUDR_INT16 *in,
                                      SQLUDR_INT16 *out,
                                      SQLUDR_INT16 *inInd,
                                      SQLUDR_INT16 *outInd,
                                      SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
    *outInd = SQLUDR_NULL;
  else
    *out = *in;

  return SQLUDR_SUCCESS;
}

/* echoUSMALL */
SQLUDR_LIBFUNC SQLUDR_INT32 echoUSMALL(SQLUDR_UINT16 *in,
                                       SQLUDR_UINT16 *out,
                                       SQLUDR_INT16 *inInd,
                                       SQLUDR_INT16 *outInd,
                                       SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
    *outInd = SQLUDR_NULL;
  else
    *out = *in;

  return SQLUDR_SUCCESS;
}

/* echoLARGE */
SQLUDR_LIBFUNC SQLUDR_INT32 echoLARGE(SQLUDR_INT64 *in,
                                      SQLUDR_INT64 *out,
                                      SQLUDR_INT16 *inInd,
                                      SQLUDR_INT16 *outInd,
                                      SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
    *outInd = SQLUDR_NULL;
  else
    *out = *in;

  return SQLUDR_SUCCESS;
}

/*---------------------------------------------------------------------------
  ECHO: FLOATING POINT
  --------------------------------------------------------------------------*/

/* echoDBL */
SQLUDR_LIBFUNC SQLUDR_INT32 echoDBL(SQLUDR_DOUBLE *in,
                                    SQLUDR_DOUBLE *out,
                                    SQLUDR_INT16 *inInd,
                                    SQLUDR_INT16 *outInd,
                                    SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
    *outInd = SQLUDR_NULL;
  else
    *out = *in;

  return SQLUDR_SUCCESS;
}

/* echoFLT */
SQLUDR_LIBFUNC SQLUDR_INT32 echoFLT(SQLUDR_DOUBLE *in,
                                    SQLUDR_DOUBLE *out,
                                    SQLUDR_INT16 *inInd,
                                    SQLUDR_INT16 *outInd,
                                    SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
    *outInd = SQLUDR_NULL;
  else
    *out = *in;

  return SQLUDR_SUCCESS;
}

/* echoREAL */
SQLUDR_LIBFUNC SQLUDR_INT32 echoREAL(SQLUDR_REAL *in,
                                     SQLUDR_REAL *out,
                                     SQLUDR_INT16 *inInd,
                                     SQLUDR_INT16 *outInd,
                                     SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
    *outInd = SQLUDR_NULL;
  else
    *out = *in;

  return SQLUDR_SUCCESS;
}

/*---------------------------------------------------------------------------
  ECHO: CHARACTER
  --------------------------------------------------------------------------*/

/* echoCHAR */
SQLUDR_LIBFUNC SQLUDR_INT32 echoCHAR(SQLUDR_CHAR *in,
                                     SQLUDR_CHAR *out,
                                     SQLUDR_INT16 *inInd,
                                     SQLUDR_INT16 *outInd,
                                     SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
    *outInd = SQLUDR_NULL;
  else
    strcpy(out, in);

  return SQLUDR_SUCCESS;
}

/* echoCHAR2 */
SQLUDR_LIBFUNC SQLUDR_INT32 echoCHAR2(SQLUDR_CHAR *in,
                                      SQLUDR_CHAR *out,
                                      SQLUDR_INT16 *inInd,
                                      SQLUDR_INT16 *outInd,
                                      SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
    *outInd = SQLUDR_NULL;
  else
    wcscpy((wchar_t *) out, (wchar_t *) in);

  return SQLUDR_SUCCESS;
}

/* echoVC */
SQLUDR_LIBFUNC SQLUDR_INT32 echoVC(SQLUDR_VC_STRUCT *in,
                                   SQLUDR_VC_STRUCT *out,
                                   SQLUDR_INT16 *inInd,
                                   SQLUDR_INT16 *outInd,
                                   SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
  {
    *outInd = SQLUDR_NULL;
  }
  else
  {
    SQLUDR_UINT32 inLen = in->length;
    memcpy(out->data, in->data, inLen);
    out->length = inLen;
  }

  return SQLUDR_SUCCESS;
}

/* echoVC2 */
SQLUDR_LIBFUNC SQLUDR_INT32 echoVC2(SQLUDR_VC_STRUCT *in,
                                    SQLUDR_VC_STRUCT *out,
                                    SQLUDR_INT16 *inInd,
                                    SQLUDR_INT16 *outInd,
                                    SQLUDR_TRAIL_ARGS)
{
  return echoVC(in, out, inInd, outInd, sqlstate, msgtext,
                calltype, statearea, udrinfo);
}

/*---------------------------------------------------------------------------
  ECHO: DATETIME
  --------------------------------------------------------------------------*/

/* echoDATE */
SQLUDR_LIBFUNC SQLUDR_INT32 echoDATE(SQLUDR_CHAR *in,
                                     SQLUDR_CHAR *out,
                                     SQLUDR_INT16 *inInd,
                                     SQLUDR_INT16 *outInd,
                                     SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
  {
    *outInd = SQLUDR_NULL;
  }
  else
  {
    /* DATE format is "yyyy-mm-dd" */
    int year = 0;
    int month = 0;
    int day = 0;
    int result = sscanf(in, "%d-%d-%d", &year, &month, &day);
    if (result < 3)
    {
      strcpy(sqlstate, "38001");
      sprintf(msgtext, "Invalid DATE: %s", in);
      return SQLUDR_ERROR;
    }
    sprintf(out, "%04d-%02d-%02d", year, month, day);
  }

  return SQLUDR_SUCCESS;
}

/* echoTIME0 */
SQLUDR_LIBFUNC SQLUDR_INT32 echoTIME0(SQLUDR_CHAR *in,
                                      SQLUDR_CHAR *out,
                                      SQLUDR_INT16 *inInd,
                                      SQLUDR_INT16 *outInd,
                                      SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
  {
    *outInd = SQLUDR_NULL;
  }
  else
  {
    /* TIME(0) format is "hh:mm:ss" */
    int hour = 0;
    int min = 0;
    int sec = 0;
    int result = sscanf(in, "%d:%d:%d", &hour, &min, &sec);
    if (result < 3)
    {
      strcpy(sqlstate, "38001");
      sprintf(msgtext, "Invalid TIME: %s", in);
      return SQLUDR_ERROR;
    }
    sprintf(out, "%02d:%02d:%02d", hour, min, sec);
  }

  return SQLUDR_SUCCESS;
}

/* echoTIME6 */
SQLUDR_LIBFUNC SQLUDR_INT32 echoTIME6(SQLUDR_CHAR *in,
                                      SQLUDR_CHAR *out,
                                      SQLUDR_INT16 *inInd,
                                      SQLUDR_INT16 *outInd,
                                      SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
  {
    *outInd = SQLUDR_NULL;
  }
  else
  {
    /* TIME(6) format is "hh:mm:ss.ffffff" */
    int hour = 0;
    int min = 0;
    int sec = 0;
    int fract = 0;
    int result = sscanf(in, "%d:%d:%d.%d", &hour, &min, &sec, &fract);
    if (result < 4)
    {
      strcpy(sqlstate, "38001");
      sprintf(msgtext, "Invalid TIME: %s", in);
      return SQLUDR_ERROR;
    }

    {
      char buf[7];
      size_t buflen = 0;
      if (fract > 999999)
        fract = 999999;
      sprintf(buf, "%d", fract);
      buflen = strlen(buf);
      while (buflen < 6)
        buf[buflen++] = '0';
      buf[6] = 0;

      sprintf(out, "%02d:%02d:%02d.%s", hour, min, sec, buf);
    }
  }

  return SQLUDR_SUCCESS;
}

/* echoTS0 */
SQLUDR_LIBFUNC SQLUDR_INT32 echoTS0(SQLUDR_CHAR *in,
                                    SQLUDR_CHAR *out,
                                    SQLUDR_INT16 *inInd,
                                    SQLUDR_INT16 *outInd,
                                    SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
  {
    *outInd = SQLUDR_NULL;
  }
  else
  {
    /* TIMESTAMP(0) format is "yyyy-mm-dd hh:mm:ss" */
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int min = 0;
    int sec = 0;
    int result = sscanf(in, "%d-%d-%d %d:%d:%d",
                        &year, &month, &day, &hour, &min, &sec);
    if (result < 6)
    {
      strcpy(sqlstate, "38001");
      sprintf(msgtext, "Invalid TIME: %s", in);
      return SQLUDR_ERROR;
    }
    sprintf(out, "%04d-%02d-%02d %02d:%02d:%02d",
            year, month, day, hour, min, sec);
  }

  return SQLUDR_SUCCESS;
}

/* echoTS6 */
SQLUDR_LIBFUNC SQLUDR_INT32 echoTS6(SQLUDR_CHAR *in,
                                    SQLUDR_CHAR *out,
                                    SQLUDR_INT16 *inInd,
                                    SQLUDR_INT16 *outInd,
                                    SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
  {
    *outInd = SQLUDR_NULL;
  }
  else
  {
    /* TIMESTAMP(6) format is "yyyy-mm-dd hh:mm:ss.ffffff" */
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int min = 0;
    int sec = 0;
    int fract = 0;
    int result = sscanf(in, "%d-%d-%d %d:%d:%d.%d",
                        &year, &month, &day, &hour, &min, &sec, &fract);
    if (result < 7)
    {
      strcpy(sqlstate, "38001");
      sprintf(msgtext, "Invalid TIME: %s", in);
      return SQLUDR_ERROR;
    }

    {
      char buf[7];
      size_t buflen = 0;
      if (fract > 999999)
        fract = 999999;
      sprintf(buf, "%d", fract);
      buflen = strlen(buf);
      while (buflen < 6)
        buf[buflen++] = '0';
      buf[6] = 0;

      sprintf(out, "%04d-%02d-%02d %02d:%02d:%02d.%s",
              year, month, day, hour, min, sec, buf);
    }
  }

  return SQLUDR_SUCCESS;
}

/*---------------------------------------------------------------------------
  ECHO: NUMERIC
  --------------------------------------------------------------------------*/

/* echoNUMERIC */
SQLUDR_LIBFUNC SQLUDR_INT32 echoNUMERIC(int numBits, /* 16, 32, or 64 */
                                        int isSigned,
                                        void *in,
                                        void *out,
                                        SQLUDR_INT16 *inInd,
                                        SQLUDR_INT16 *outInd,
                                        SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
  {
    *outInd = SQLUDR_NULL;
  }
  else
  {
    /* When precision is <= 18, NUMERIC(prec,scale) format is a binary
       integer: ( value * 10**scale ) */
    SQLUDR_PARAM *inParam = &(udrinfo->inputs[0]);
    SQLUDR_INT16 scale = inParam->u1.scale;
    double powDouble = pow((double) 10, (double) scale);
    int powerOf10 = (int) powDouble;

    /* Translate the input value to two integers x and y, where x
       represents digits before the decimal point and y represents
       digits after */
    SQLUDR_INT64 x;
    SQLUDR_INT64 y;
    if (numBits == 16 && isSigned)
    {
      SQLUDR_INT16 *input = (SQLUDR_INT16 *) in;
      SQLUDR_INT16 *result = (SQLUDR_INT16 *) out;
      x = *input / powerOf10;
      y = *input - (x * powerOf10);
      *result = (SQLUDR_INT16) ((x * powerOf10) + y);
    }
    else if (numBits == 16 && !isSigned)
    {
      SQLUDR_UINT16 *input = (SQLUDR_UINT16 *) in;
      SQLUDR_UINT16 *result = (SQLUDR_UINT16 *) out;
      x = *input / powerOf10;
      y = *input - (x * powerOf10);
      *result = (SQLUDR_UINT16) ((x * powerOf10) + y);
    }
    else if (numBits == 32 && isSigned)
    {
      SQLUDR_INT32 *input = (SQLUDR_INT32 *) in;
      SQLUDR_INT32 *result = (SQLUDR_INT32 *) out;
      x = *input / powerOf10;
      y = *input - (x * powerOf10);
      *result = (SQLUDR_INT32) ((x * powerOf10) + y);
    }
    else if (numBits == 32 && !isSigned)
    {
      SQLUDR_UINT32 *input = (SQLUDR_UINT32 *) in;
      SQLUDR_UINT32 *result = (SQLUDR_UINT32 *) out;
      x = *input / powerOf10;
      y = *input - (x * powerOf10);
      *result = (SQLUDR_UINT32) ((x * powerOf10) + y);
    }
    else
    {
      SQLUDR_INT64 *input = (SQLUDR_INT64 *) in;
      SQLUDR_INT64 *result = (SQLUDR_INT64 *) out;
      x = *input / powerOf10;
      y = *input - (x * powerOf10);
      *result = (SQLUDR_INT64) ((x * powerOf10) + y);
    }
  }

  return SQLUDR_SUCCESS;
}

/* echoNUM4 */
SQLUDR_LIBFUNC SQLUDR_INT32 echoNUM4(SQLUDR_INT16 *in,
                                     SQLUDR_INT16 *out,
                                     SQLUDR_INT16 *inInd,
                                     SQLUDR_INT16 *outInd,
                                     SQLUDR_TRAIL_ARGS)
{
  return echoNUMERIC(16, 1,
                     in, out, inInd, outInd, sqlstate, msgtext,
                     calltype, statearea, udrinfo);
}

/* echoNUM8 */
SQLUDR_LIBFUNC SQLUDR_INT32 echoNUM8(SQLUDR_INT32 *in,
                                     SQLUDR_INT32 *out,
                                     SQLUDR_INT16 *inInd,
                                     SQLUDR_INT16 *outInd,
                                     SQLUDR_TRAIL_ARGS)
{
  return echoNUMERIC(32, 1,
                     in, out, inInd, outInd, sqlstate, msgtext,
                     calltype, statearea, udrinfo);
}

/* echoNUM12 */
SQLUDR_LIBFUNC SQLUDR_INT32 echoNUM12(SQLUDR_INT64 *in,
                                      SQLUDR_INT64 *out,
                                      SQLUDR_INT16 *inInd,
                                      SQLUDR_INT16 *outInd,
                                      SQLUDR_TRAIL_ARGS)
{
  return echoNUMERIC(64, 1,
                     in, out, inInd, outInd, sqlstate, msgtext,
                     calltype, statearea, udrinfo);
}

/* echoNUM0 */
SQLUDR_LIBFUNC SQLUDR_INT32 echoNUM0(SQLUDR_INT32 *in,
                                     SQLUDR_INT32 *out,
                                     SQLUDR_INT16 *inInd,
                                     SQLUDR_INT16 *outInd,
                                     SQLUDR_TRAIL_ARGS)
{
  return echoNUMERIC(32, 1,
                     in, out, inInd, outInd, sqlstate, msgtext,
                     calltype, statearea, udrinfo);
}

/* echoUNUM4 */
SQLUDR_LIBFUNC SQLUDR_INT32 echoUNUM4(SQLUDR_UINT16 *in,
                                      SQLUDR_UINT16 *out,
                                      SQLUDR_INT16 *inInd,
                                      SQLUDR_INT16 *outInd,
                                      SQLUDR_TRAIL_ARGS)
{
  return echoNUMERIC(16, 0,
                     in, out, inInd, outInd, sqlstate, msgtext,
                     calltype, statearea, udrinfo);
}

/* echoUNUM8 */
SQLUDR_LIBFUNC SQLUDR_INT32 echoUNUM8(SQLUDR_INT32 *in,
                                      SQLUDR_INT32 *out,
                                      SQLUDR_INT16 *inInd,
                                      SQLUDR_INT16 *outInd,
                                      SQLUDR_TRAIL_ARGS)
{
  return echoNUMERIC(32, 0,
                     in, out, inInd, outInd, sqlstate, msgtext,
                     calltype, statearea, udrinfo);
}

/* echoUNUM12 */
SQLUDR_LIBFUNC SQLUDR_INT32 echoUNUM12(SQLUDR_INT64 *in,
                                       SQLUDR_INT64 *out,
                                       SQLUDR_INT16 *inInd,
                                       SQLUDR_INT16 *outInd,
                                       SQLUDR_TRAIL_ARGS)
{
  return echoNUMERIC(64, 0,
                     in, out, inInd, outInd, sqlstate, msgtext,
                     calltype, statearea, udrinfo);
}

/* echoUNUM0 */
SQLUDR_LIBFUNC SQLUDR_INT32 echoUNUM0(SQLUDR_INT32 *in,
                                      SQLUDR_INT32 *out,
                                      SQLUDR_INT16 *inInd,
                                      SQLUDR_INT16 *outInd,
                                      SQLUDR_TRAIL_ARGS)
{
  return echoNUMERIC(32, 0,
                     in, out, inInd, outInd, sqlstate, msgtext,
                     calltype, statearea, udrinfo);
}

/*---------------------------------------------------------------------------
  ECHO: BIGNUM
  --------------------------------------------------------------------------*/

/* echoBNUM32 */
SQLUDR_LIBFUNC SQLUDR_INT32 echoBNUM32(SQLUDR_CHAR *in,
                                       SQLUDR_CHAR *out,
                                       SQLUDR_INT16 *inInd,
                                       SQLUDR_INT16 *outInd,
                                       SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
  {
    *outInd = SQLUDR_NULL;
  }
  else
  {
    SQLUDR_PARAM *inParam = &(udrinfo->inputs[0]);
    SQLUDR_UINT32 numBytes = inParam->data_len;
    memcpy(out, in, numBytes);
  }

  return SQLUDR_SUCCESS;
}

/* echoBNUM0 */
SQLUDR_LIBFUNC SQLUDR_INT32 echoBNUM0(SQLUDR_CHAR *in,
                                      SQLUDR_CHAR *out,
                                      SQLUDR_INT16 *inInd,
                                      SQLUDR_INT16 *outInd,
                                      SQLUDR_TRAIL_ARGS)
{
  return echoBNUM32(in, out, inInd, outInd, sqlstate, msgtext,
                    calltype, statearea, udrinfo);
}

/*---------------------------------------------------------------------------
  ECHO: DECIMAL SIGNED and UNSIGNED
  --------------------------------------------------------------------------*/

/* echoDEC */
SQLUDR_LIBFUNC SQLUDR_INT32 echoDEC(SQLUDR_CHAR *in,
                                    SQLUDR_CHAR *out,
                                    SQLUDR_INT16 *inInd,
                                    SQLUDR_INT16 *outInd,
                                    SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
  {
    *outInd = SQLUDR_NULL;
  }
  else
  {
    /* DECIMAL(prec,scale) format is an ASCII string, left-justified
       and padded on the right with blanks */
    SQLUDR_PARAM *inParam = &(udrinfo->inputs[0]);
    SQLUDR_INT16 prec = inParam->u2.precision;
    SQLUDR_INT16 scale = inParam->u1.scale;
    char *dotPointer = strchr(in, '.');
    if (dotPointer)
    {
      int bytesBefore = dotPointer - in;
      memcpy(out, in, bytesBefore);
      out[bytesBefore] = '.';
      memcpy(out + bytesBefore + 1, dotPointer + 1, scale);
    }
    else
    {
      memcpy(out, in, (scale > 0 ? prec + 2 : prec + 1));
    }
  }

  return SQLUDR_SUCCESS;
}

/* echoUDEC */
SQLUDR_LIBFUNC SQLUDR_INT32 echoUDEC(SQLUDR_CHAR *in,
                                     SQLUDR_CHAR *out,
                                     SQLUDR_INT16 *inInd,
                                     SQLUDR_INT16 *outInd,
                                     SQLUDR_TRAIL_ARGS)
{
  return echoDEC(in, out, inInd, outInd, sqlstate, msgtext,
                 calltype, statearea, udrinfo);
}

/* echoDEC0 */
SQLUDR_LIBFUNC SQLUDR_INT32 echoDEC0(SQLUDR_CHAR *in,
                                     SQLUDR_CHAR *out,
                                     SQLUDR_INT16 *inInd,
                                     SQLUDR_INT16 *outInd,
                                     SQLUDR_TRAIL_ARGS)
{
  return echoDEC(in, out, inInd, outInd, sqlstate, msgtext,
                 calltype, statearea, udrinfo);
}

/* getMXV */
SQLUDR_LIBFUNC SQLUDR_INT32 getMXV(SQLUDR_INT16 *out,
                                   SQLUDR_INT16 *outInd,
                                   SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;
  *out = (SQLUDR_INT16) udrinfo->sql_version;
  return SQLUDR_SUCCESS;
}

/* sleep */
SQLUDR_LIBFUNC SQLUDR_INT32 Sleep(SQLUDR_INT32 *in,
                                  SQLUDR_INT32 *out,
                                  SQLUDR_INT16 *inInd,
                                  SQLUDR_INT16 *outInd,
                                  SQLUDR_TRAIL_ARGS)
{
  int secondsToSleep;
  int i;

  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
    secondsToSleep = 0;
  else
    secondsToSleep = *in;

  if (secondsToSleep > 0)
  {
    printf("\n");
    printf("[SLEEP] Time is %ld\n", (long) time(0));
    printf("[SLEEP] About to sleep for %d seconds...\n", secondsToSleep);
    fflush(stdout);

    for (i = 0; i < secondsToSleep; i++)
    {
      sleep(1 * 1000);
    }

    printf("[SLEEP] Done\n");
    printf("[SLEEP] Time is %ld\n\n", (long) time(0));
    fflush(stdout);
  }

  *out = secondsToSleep;
  return SQLUDR_SUCCESS;
}

/* err */
SQLUDR_LIBFUNC SQLUDR_INT32 err(SQLUDR_INT16 *out,
                                SQLUDR_INT16 *outInd,
                                SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;
  sprintf(msgtext, "This function always returns an error.");
  sprintf(sqlstate, "38001");
  return SQLUDR_ERROR;
}

/* pname */
SQLUDR_LIBFUNC SQLUDR_INT32 pname(SQLUDR_CHAR *out,
                                  SQLUDR_INT16 *outInd,
                                  SQLUDR_TRAIL_ARGS)
{
  char *pname;
  unsigned int len;

  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  pname = udrinfo->return_values[0].name;
  len = strlen(pname);
  memcpy(out, pname, len);

  return SQLUDR_SUCCESS;
}

/* pname_const */
SQLUDR_LIBFUNC SQLUDR_INT32 pname_const(SQLUDR_CHAR *out,
                                        SQLUDR_INT16 *outInd,
                                        SQLUDR_TRAIL_ARGS)
{
  char *pname;
  unsigned int len;

  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  pname = "PNAME";
  len = strlen(pname);
  memcpy(out, pname, len);

  return SQLUDR_SUCCESS;
}

/* getCollation */
SQLUDR_LIBFUNC SQLUDR_INT32 getCollation(SQLUDR_CHAR *in,
                                         SQLUDR_CHAR *out,
                                         SQLUDR_INT16 *inInd,
                                         SQLUDR_INT16 *outInd,
                                         SQLUDR_TRAIL_ARGS)
{
  SQLUDR_INT32 result = SQLUDR_SUCCESS;
  SQLUDR_PARAM *inParam;
  SQLUDR_INT16 collation;

  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  inParam = &(udrinfo->inputs[0]);
  collation = inParam->u2.collation;

  switch (collation)
  {
    case SQLUDR_COLLATION_DEFAULT:
      strncpy(out, "DEFAULT", 7);
      break;
    case SQLUDR_COLLATION_CZECH:
      strncpy(out, "CZECH", 5);
      break;
    case SQLUDR_COLLATION_CZECH_CI:
      strncpy(out, "CZECH_CI", 8);
      break;
    case SQLUDR_COLLATION_SJIS:
      strncpy(out, "SJIS", 4);
      break;
    case SQLUDR_COLLATION_UNKNOWN:
      strncpy(out, "UNKNOWN", 7);
      break;
    default:
      sprintf(msgtext, "Invalid collation: %d", (int) collation);
      sprintf(sqlstate, "38001");
      result = SQLUDR_ERROR;
      break;
  }

  return result;
}

SQLUDR_LIBFUNC SQLUDR_INT32 errOrWarn(SQLUDR_INT32 *in1,
                                      SQLUDR_INT32 *out,
                                      SQLUDR_INT16 *in1Ind,
                                      SQLUDR_INT16 *outInd,
                                      SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*in1Ind == SQLUDR_NULL)
  {
    *outInd = SQLUDR_NULL;
  }
  else
  {
    SQLUDR_INT32 result = (*in1);

    if (result % 10 == 0)
    {
      /* Multiples of 10 generate an ERROR */
      strcpy(sqlstate, "38001");
      sprintf(msgtext, "Value is %d. This is an error.", result);
      return SQLUDR_ERROR;
    }
    else if (result % 2 == 1)
    {
      /* Odd numbers generate a WARNING */
      (*out) = result;
      strcpy(sqlstate, "38101");
      sprintf(msgtext, "Value is %d. This is a warning.", result);
      return SQLUDR_SUCCESS_WITH_WARNING;
    }
    else
    {
      /* Even numbers do not return an ERROR or WARNING */
      (*out) = result;
    }
  }

  return SQLUDR_SUCCESS;
}

/* MMSC: Returns the min, max, sum, count of 4 integers */
SQLUDR_LIBFUNC SQLUDR_INT32 mmsc(SQLUDR_INT32 *in1,
                                 SQLUDR_INT32 *in2,
                                 SQLUDR_INT32 *in3,
                                 SQLUDR_INT32 *in4,
                                 SQLUDR_INT32 *out1,
                                 SQLUDR_INT32 *out2,
                                 SQLUDR_INT32 *out3,
                                 SQLUDR_INT32 *out4,
                                 SQLUDR_INT16 *in1Ind,
                                 SQLUDR_INT16 *in2Ind,
                                 SQLUDR_INT16 *in3Ind,
                                 SQLUDR_INT16 *in4Ind,
                                 SQLUDR_INT16 *out1Ind,
                                 SQLUDR_INT16 *out2Ind,
                                 SQLUDR_INT16 *out3Ind,
                                 SQLUDR_INT16 *out4Ind,
                                 SQLUDR_TRAIL_ARGS)
{
  SQLUDR_INT32 min;
  SQLUDR_INT32 max;
  SQLUDR_INT32 sum;
  SQLUDR_INT32 count;

  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  /* Return all NULLs if all inputs are NULL */
  if (*in1Ind == SQLUDR_NULL &&
      *in2Ind == SQLUDR_NULL &&
      *in3Ind == SQLUDR_NULL &&
      *in4Ind == SQLUDR_NULL)
  {
    *out1Ind = SQLUDR_NULL;
    *out2Ind = SQLUDR_NULL;
    *out3Ind = SQLUDR_NULL;
    *out4Ind = SQLUDR_NULL;
    return SQLUDR_SUCCESS;
  }

  min = 0;
  max = 0;
  sum = 0;
  count = 0;

  if (*in1Ind != SQLUDR_NULL)
  {
    min = *in1;
    max = *in1;
    sum += *in1;
    count++;
  }

  if (*in2Ind != SQLUDR_NULL)
  {
    if (*in2 < min)
      min = *in2;
    if (*in2 > max)
      max = *in2;
    sum += *in2;
    count++;
  }

  if (*in3Ind != SQLUDR_NULL)
  {
    if (*in3 < min)
      min = *in3;
    if (*in3 > max)
      max = *in3;
    sum += *in3;
    count++;
  }

  if (*in4Ind != SQLUDR_NULL)
  {
    if (*in4 < min)
      min = *in4;
    if (*in4 > max)
      max = *in4;
    sum += *in4;
    count++;
  }

  *out1 = min;
  *out2 = max;
  *out3 = sum;
  *out4 = count;

  return SQLUDR_SUCCESS;
}

/* Return session user from udrinfo structure */
SQLUDR_LIBFUNC SQLUDR_INT32 session_user_name(SQLUDR_CHAR *out,
                                    SQLUDR_INT16 *outInd,
                                    SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  strncpy(out, udrinfo->session_user_name, strlen(udrinfo->session_user_name));


  return SQLUDR_SUCCESS;
}

/* Validate session user in udrinfo struct with passed in SESSION_USER name  */
SQLUDR_LIBFUNC SQLUDR_INT32 validate_session_user_name(SQLUDR_CHAR *in,
                                              SQLUDR_CHAR *out,
                                              SQLUDR_INT16 *inInd,
                                              SQLUDR_INT16 *outInd,
                                              SQLUDR_TRAIL_ARGS)
{

  SQLUDR_INT32 status;

  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
  {
    strcpy(sqlstate, "38001");
    strcpy(msgtext, "Invalid Input");
    status = SQLUDR_ERROR;
  }
  else
  {
    if (strncmp(in, udrinfo->session_user_name,
                strlen(udrinfo->session_user_name)) == 0) {
       strncpy(out, "PASS", 4);
       status = SQLUDR_SUCCESS;
     } else {
        strncpy(out, "FAIL", 4);
        status = SQLUDR_SUCCESS;
     }
  }

  return status;
}

/* Return current user from udrinfo structure */
SQLUDR_LIBFUNC SQLUDR_INT32 current_user_name(SQLUDR_CHAR *out,
                                    SQLUDR_INT16 *outInd,
                                    SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  strncpy(out, udrinfo->current_user_name, strlen(udrinfo->current_user_name));


  return SQLUDR_SUCCESS;
}

/* Validate current user in udrinfo struct with passed in CURRENT_USER name  */
SQLUDR_LIBFUNC SQLUDR_INT32 validate_current_user_name(SQLUDR_CHAR *in,
                                              SQLUDR_CHAR *out,
                                              SQLUDR_INT16 *inInd,
                                              SQLUDR_INT16 *outInd,
                                              SQLUDR_TRAIL_ARGS)
{

  SQLUDR_INT32 status;

  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
  {
    strcpy(sqlstate, "38001");
    strcpy(msgtext, "Invalid Input");
    status = SQLUDR_ERROR;
  }
  else
  {
    if (strncmp(in, udrinfo->current_user_name,
                strlen(udrinfo->current_user_name)) == 0) {
       strncpy(out, "PASS", 4);
       status = SQLUDR_SUCCESS;
     } else {
        strncpy(out, "FAIL", 4);
        status = SQLUDR_SUCCESS;
     }
  }

  return status;
}


/* Return current role from udrinfo structure */
SQLUDR_LIBFUNC SQLUDR_INT32 current_role_name(SQLUDR_CHAR *out,
                                    SQLUDR_INT16 *outInd,
                                    SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  strncpy(out, udrinfo->current_user_name, strlen(udrinfo->current_user_name));


  return SQLUDR_SUCCESS;
}

/* Validate current role in udrinfo struct with passed in CURRENT_ROLE name  */
SQLUDR_LIBFUNC SQLUDR_INT32 validate_current_role_name(SQLUDR_CHAR *in,
                                              SQLUDR_CHAR *out,
                                              SQLUDR_INT16 *inInd,
                                              SQLUDR_INT16 *outInd,
                                              SQLUDR_TRAIL_ARGS)
{

  SQLUDR_INT32 status;

  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*inInd == SQLUDR_NULL)
  {
    strcpy(sqlstate, "38001");
    strcpy(msgtext, "Invalid Input");
    status = SQLUDR_ERROR;
  }
  else
  {
    if (strncmp(in, udrinfo->current_user_name,
                strlen(udrinfo->current_user_name)) == 0) {
       strncpy(out, "PASS", 4);
       status = SQLUDR_SUCCESS;
     } else {
        strncpy(out, "FAIL", 4);
        status = SQLUDR_SUCCESS;
     }
  }

  return status;
}

} /* extern "C" */
