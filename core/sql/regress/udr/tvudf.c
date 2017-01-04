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

/*
  gcc -g -m32 -I$TRAF_HOME/../sql/sqludr -I$TRAF_HOME/../sql/cli \
   -shared -o tvudf.so tvudf.c
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "sqludr.h"
#include "sqlcli.h"

const char *getSqlType(int t)
{
  switch (t)
  {
    case SQLTYPECODE_CHAR:
      return "CHAR";
    case SQLTYPECODE_NUMERIC:
      return "NUMERIC";
    case SQLTYPECODE_NUMERIC_UNSIGNED:
      return "NUMERIC UNSIGNED";
    case SQLTYPECODE_DECIMAL:
      return "DECIMAL";
    case SQLTYPECODE_DECIMAL_UNSIGNED:
      return "DECIMAL UNSIGNED";
    case SQLTYPECODE_DECIMAL_LARGE:
      return "DECIMAL LARGE";
    case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED:
      return "DECIMAL LARGE UNSIGNED";
    case SQLTYPECODE_INTEGER:
      return "INTEGER";
    case SQLTYPECODE_INTEGER_UNSIGNED:
      return "INTEGER UNSIGNED";
    case SQLTYPECODE_LARGEINT:
      return "LARGEINT";
    case SQLTYPECODE_SMALLINT:
      return "SMALLINT";
    case SQLTYPECODE_SMALLINT_UNSIGNED:
      return "SMALLINT UNSIGNED";
    case SQLTYPECODE_BPINT_UNSIGNED:
      return "BPINT UNSIGNED";
    case SQLTYPECODE_IEEE_FLOAT:
      return "IEEE FLOAT";
    case SQLTYPECODE_IEEE_REAL:
      return "IEEE REAL";
    case SQLTYPECODE_IEEE_DOUBLE:
      return "IEEE DOUBLE";
    case SQLTYPECODE_TDM_FLOAT:
      return "TDM FLOAT";
    case SQLTYPECODE_TDM_REAL:
      return "TDM REAL";
    case SQLTYPECODE_TDM_DOUBLE:
      return "TDM_DOUBLE";
    case SQLTYPECODE_DATETIME:
      return "DATETIME";
    case SQLTYPECODE_INTERVAL:
      return "INTERVAL";
    case SQLTYPECODE_VARCHAR:
      return "VARCHAR";
    case SQLTYPECODE_VARCHAR_WITH_LENGTH:
      return "VARCHAR WITH LENGTH";
    case SQLTYPECODE_VARCHAR_LONG:
      return "VARCHAR LONG";
    case SQLTYPECODE_BIT:
      return "BIT";
    case SQLTYPECODE_BITVAR:
      return "BITVAR";
  }
  return "UNKNOWN";
}

void printUdrInfo(SQLUDR_UDRINFO *udrInfo)
{
  SQLUDR_PARAM *inParam = udrInfo->inputs;
  SQLUDR_PARAM *outParam = udrInfo->return_values;
  SQLUDR_PARAM *p = NULL;
  int i;

  printf("[UDRINFO]\n"
         "  name [%s]\n"
         "  num in %d, in row len %d\n"
         "  num out %d, out row len %d\n",
         udrInfo->routine_name,
         (int) udrInfo->num_inputs,
         (int) udrInfo->in_row_length,
         (int) udrInfo->num_return_values,
         (int) udrInfo->out_row_length);

  for (i = 0; i < udrInfo->num_inputs; i++)
  {
    p = &(inParam[i]);
    printf("[IN %ld]\n"
           "  name [%s], type %s, offset %d, len %d\n"
           "  ind offset %d, vc ind offset %d, vc ind len %d\n",
           i + 1,
           p->name,
           getSqlType(p->datatype),
           (int) p->data_offset,
           (int) p->data_len,
           (int) p->ind_offset,
           (int) p->vc_ind_offset,
           (int) p->vc_ind_len);
  }

  for (i = 0; i < udrInfo->num_return_values; i++)
  {
    p = &(outParam[i]);
    printf("[OUT %ld]\n"
           "  name [%s], type %s, offset %d, len %d\n"
           "  ind offset %d, vc ind offset %d, vc ind len %d\n",
           i + 1,
           p->name,
           getSqlType(p->datatype),
           (int) p->data_offset,
           (int) p->data_len,
           (int) p->ind_offset,
           (int) p->vc_ind_offset,
           (int) p->vc_ind_len);
  }
}

SQLUDR_INT32 UDR_ASSERT(char *expr, char *file, int line,
                        char *sqlstate, char *msgtext)
{
  /*
  printf("\n*** UDR ASSERTION FAILURE ***\n");
  printf("Process: %d\n", (int) getpid());
  printf("File: %s\n", file);
  printf("Line: %d\n", line);
  printf("Condition: %s\n", expr);
  fflush(stdout);
  sleep(1);
  */

  sprintf(sqlstate, "38999");
#ifdef _WIN32
  _snprintf(msgtext, SQLUDR_MSGTEXT_SIZE,
           "UDR ASSERTION FAILURE: pid %d, line %d: %s",
           (int) getpid(), line, expr);
#else
  snprintf(msgtext, SQLUDR_MSGTEXT_SIZE,
           "UDR ASSERTION FAILURE: pid %d, line %d: %s",
           (int) getpid(), line, expr);
#endif
  return SQLUDR_ERROR;
}

#define udr_assert(e) \
{ if (!(e)) \
    return UDR_ASSERT("" # e "", __FILE__, __LINE__, sqlstate, msgtext); }

SQLUDR_LIBFUNC
SQLUDR_INT32 TVUDF(SQLUDR_CHAR *in_data,
                   SQLUDR_TMUDF_TRAIL_ARGS)
{
  SQLUDR_Q_STATE qstate;
  int *col1;
  int numOutRows;
  int inLen = udrinfo->in_row_length;
  int outLen = udrinfo->out_row_length;
  int i;

  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (0)
    printUdrInfo(udrinfo);

  udr_assert(inLen == outLen);
  memcpy(rowDataSpace2, in_data, inLen);

  /* Assumptions
   * - Output columns are same as input colums
   * - First column is INTEGER
   * - Value of first input column is N
   * - N output rows will be returned
   * - First column in first output row will be 1
   * - First column is incremented by 1 in successive output rows
   * - All output columns after first column are zeroed out
   */

  col1 = (int *) (rowDataSpace2 + 4);
  numOutRows = *col1;
  *col1 = 0;

  qstate = SQLUDR_Q_MORE;
  for (i = 0; i < numOutRows; i++)
  {
    (*col1)++;
    emitRow(rowDataSpace2, 0, &qstate);
  }

  qstate = SQLUDR_Q_EOD;
  emitRow(rowDataSpace2, 0, &qstate);

  return SQLUDR_SUCCESS;
}
