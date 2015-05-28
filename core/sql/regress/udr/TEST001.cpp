// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2000-2014 Hewlett-Packard Development Company, L.P.
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
#include <limits.h>
#include "sqludr.h"

using namespace tmudr;

/*
-- how to invoke this UDF:

create table clicks (userid char(32), ts TIME(6), ip-addr char(15)) ;

SELECT ts, userid, session-id
FROM UDF(sessionize(TABLE(SELECT *
                          FROM clicks
                          PARTITION BY userid
                          ORDER BY ts),
                    'TS', -- name of timestamp column to sessionize
                    60))  -- inactivity time limit of a session
WHERE userid = 'super-user';
*/


extern "C" SQLUDR_LIBFUNC SQLUDR_INT32 SESSIONIZE_STATIC(
  SQLUDR_CHAR           *in_data,
  SQLUDR_TMUDF_TRAIL_ARGS)
{
  char            *inData = NULL;
  SQLUDR_PARAM    *inParam = NULL;
  int             inDataLen = 0;

  char            *outData = NULL;
  SQLUDR_PARAM    *outParam = NULL;
  int             outDataLen = 0;

  char            *colName = NULL;
  int             timeStampColNum = -1;
  int             userIdColNum = -1;
  int             outputColOffset = 0;
  int             interval;
  unsigned int    i;

  int             isDynamic = 0;
  SQLUDR_Q_STATE  qstate;
  SQLUDR_INT64    in_TS = 0;      /* TIME format later */
  SQLUDR_INT64    prev_TS = 0;
  char            *inUserID = NULL;
  char            *prevUserID = NULL;
  char            *endCursor = NULL;
  SQLUDR_INT64    sessionID = 0;
  SQLUDR_INT32    retcode = SQLUDR_SUCCESS;

  SQLUDR_UINT32   maxUidLen = 0;

  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  int dd=1;

  // enable this for debugging
  //while (dd < 2)
  //  dd=1-dd;

  if (strcmp(udrinfo->routine_name,
             "SESSIONIZE_DYNAMIC") == 0)
    isDynamic = 1;

  /* extract all the scalar values. */
  for(i= 0; i < udrinfo->num_inputs; i++)
  {
    inParam = &(udrinfo->inputs[i]);
    inData = in_data + inParam->data_offset;
    inDataLen = (int) inParam->data_len;

    /* expect scalar values to be input in desired order. */
    switch(i)
    {
      case 0: /* scalar argument 1, clicksColumn */
      case 1: /* scalar argument 2, useridColumn */
        colName = (char *) malloc(inDataLen + 1);
        strncpy(colName,inData,inDataLen);
        colName[inDataLen] = '\0';
	endCursor = &(colName[inDataLen-1]);
	/* remove trailing pattern matches */
	while((*endCursor == ' ') && (colName < endCursor))
	  endCursor--;
	*(endCursor+1) = '\0';
        for(int c = 0; c < udrinfo->table_inputs[0].num_params; c++)
          {
            if (strcmp(udrinfo->table_inputs[0].params[c].name,
                       colName) == 0)
              if (i == 0)
                timeStampColNum = c;
              else
                userIdColNum = c;
          }
        break;

      case 2:
        memcpy(&interval, inData, sizeof(int));
        break;

      default:
        return SQLUDR_ERROR;
    }
  }

  if (timeStampColNum < 0 || userIdColNum < 0)
    return SQLUDR_ERROR;

  maxUidLen = (udrinfo->table_inputs[0].params[userIdColNum]).data_len;
  inUserID = (char *) malloc(maxUidLen + 1);
  prevUserID = (char *) malloc(maxUidLen + 1);
  memset(inUserID,'\0', maxUidLen + 1);
  memset(prevUserID,'\0', maxUidLen + 1);

  do
  {
    getRow(rowDataSpace1, 0, &qstate);

    /* A row is expected only when qstate is SQLUDR_Q_MORE. */
    if(qstate != SQLUDR_Q_MORE)
      break;

    /* Extract timestamp and user id */
    for(i = 0; i < udrinfo->table_inputs[0].num_params; i++)
    {
      inParam = &udrinfo->table_inputs[0].params[i];
      inData = rowDataSpace1 + inParam->data_offset;
      inDataLen = (int) inParam->data_len;

      if(i == timeStampColNum)
      {
        memcpy(&in_TS, inData, inDataLen);
      }
      else if (i == userIdColNum)
      {
        SQLUDR_UINT32 bytesToCopy = inDataLen;
        if (bytesToCopy > maxUidLen)
          bytesToCopy = maxUidLen;
        strncpy(inUserID, inData, bytesToCopy);
      }
      else if (!isDynamic)
      {
        /* expect only two inputs */
        return SQLUDR_ERROR;
      }
    }

    /* Now process the data */
    if(strcmp(prevUserID,inUserID) != 0)
    {
      /* Reset sessionID if we receive a new userID. */
      sessionID = 0;
      prev_TS = 0;
    }

    if((prev_TS == 0) ||
      (in_TS - prev_TS) > interval)
    {
      /* Time stamp diff is greater than 60 secs or prev_TS is 0 */
      sessionID++;
    }

    prev_TS = in_TS;
    strcpy(prevUserID, inUserID);

    /* Now construct the row and call emitRow() */
    memset(rowDataSpace2, '\0', udrinfo->out_row_length);
    outputColOffset = 0;

    for(i = 0; i < udrinfo->num_return_values; i++)
    {
      outParam = &udrinfo->return_values[i];
      outData = rowDataSpace2 + outParam->data_offset;
      outDataLen = (int) outParam->data_len;

      if(strcmp(outParam->name, "SESSION_ID") == 0)
      {
        memcpy(outData,&sessionID,outDataLen);
        outputColOffset = 1;
      }
      else
      {
        /* get the corresponding input parameter */
        inParam = &udrinfo->table_inputs[0].params[i-outputColOffset];
        inData = rowDataSpace1 + inParam->data_offset;
        inDataLen = (int) inParam->data_len;

        if (inDataLen != outDataLen)
          return SQLUDR_ERROR;

        memcpy(outData, inData, outDataLen);
      }
    }

    emitRow(rowDataSpace2, 0, &qstate);

  }while(qstate == SQLUDR_Q_MORE);

  switch(qstate)
  {
    case SQLUDR_Q_EOD:
      /* emit row to indicate EOD. */
      emitRow(rowDataSpace2,0,&qstate);
      retcode = SQLUDR_SUCCESS;
      break;

    case SQLUDR_Q_CANCEL:
      emitRow(rowDataSpace2,0,&qstate); /* this is a workaround? */
      retcode = SQLUDR_SUCCESS;
      break;

    case SQLUDR_Q_MORE:
    default:
      /* we should not reach here. Means something is wrong. */
      retcode = SQLUDR_ERROR;
      break;
  };

  return retcode;

}/* main */

// This function does not have a compiler interface, all the
// output columns to produce need to be provided in the DDL

extern "C" SQLUDR_LIBFUNC SQLUDR_INT32 SESSIONIZE_DYNAMIC(
  SQLUDR_CHAR           *in_data,
  SQLUDR_TMUDF_TRAIL_ARGS)
{
  return SESSIONIZE_STATIC(in_data,
                           PASS_ON_SQLUDR_TMUDF_TRAIL_ARGS);
}

class SessionizeUDFInterface : public TMUDRInterface
{
  // override any methods where the UDF author would
  // like to change the default behavior

  void describeParamsAndColumns(UDRInvocationInfo &info); // Binder

};

extern "C" TMUDRInterface * SESSIONIZE_DYNAMIC_CreateCompilerInterfaceObject(
     const UDRInvocationInfo *info)
{
  return new SessionizeUDFInterface();
}

void SessionizeUDFInterface::describeParamsAndColumns(
     UDRInvocationInfo &info)
{
  // do this logic only if the UDF name matches
  if (info.getUDRName().find(".SESSIONIZE_DYNAMIC"))
    {
      // validate the input parameters
      if (info.getNumActualParameters() != 3)
        throw UDRException(38080, 
                           "Expecting three input parameters, ts colname, user id colname, timeout");

      for (int colNum=0; colNum<2; colNum++)
        if (info.getActualParameterInfo(colNum).getType().getSQLTypeClass() == TypeInfo::CHARACTER_TYPE &&
            info.getActualParameterInfo(colNum).isAvailable())
          {
            std::string colName =
              info.getActualParameterInfo(colNum).getStringValue();
            // test finding the timestamp column in the input table,
            // an exception will be raised if it is not found
            const ColumnInfo &colInfo =
              info.getInputTableInfo(0).getColumn(colName);
          }
        else
          {
            throw UDRException(
                 38082, 
                 "The %s parameter of TMUDF %s needs to be a compile time character constant",
                 (colNum == 0 ? "first" : "second"),
                 info.getUDRName().data());
          }

      // sessionize is intended to work with a single input table
      if (info.getNumTableInputs() != 1)
        throw UDRException(38085,
                           "Expecting one table-valued input, got %d",
                           info.getNumTableInputs());

      // remove any output columns declared in the DDL
      while (info.getOutputTableInfo().getNumColumns() > 0)
        info.getOutputTableInfo().deleteColumn(0);

      // add the session id column, a single integer
      info.getOutputTableInfo().addColumn(
           ColumnInfo("SESSION_ID",
                      TypeInfo(TypeInfo::INT64)));

      // add all input table columns as output columns
      for (int t=0; t<info.getNumTableInputs(); t++)
        info.addPassThruColumns(t);
    }
  else
    {
      throw UDRException(
           38086, 
           "Called compiler interface for TMUDF SESSIONIZE_DYNAMIC for TMUDF %s",
           info.getUDRName().data());
    }
}

extern "C" SQLUDR_LIBFUNC SQLUDR_INT32 fibonacci(
  SQLUDR_CHAR           *in_data,
  SQLUDR_TMUDF_TRAIL_ARGS)
{
  char            *inData = NULL;
  SQLUDR_PARAM    *inParam = NULL;
  int             inDataLen = 0;

  char            *outData = NULL;
  SQLUDR_PARAM    *outParam = NULL;
  int             outDataLen = 0;

  SQLUDR_Q_STATE  qstate = SQLUDR_Q_MORE;
  int             start = 0;
  int             num_result_rows = 0;
  int             *ordinal = NULL;
  long            *fibonacci_number = NULL;
  long            previous_result = 0;
  long            temp;
  SQLUDR_INT32    retcode = SQLUDR_SUCCESS;

  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  int dd=1;

  // enable this for debugging
  // while (dd < 2)
  //   dd=1-dd;


  /* extract the scalar input parameters */
  for(int i= 0; i < udrinfo->num_inputs; i++)
  {
    inParam = &(udrinfo->inputs[i]);
    inData = in_data + inParam->data_offset;
    inDataLen = (int) inParam->data_len;

    /* expect scalar values to be input in desired order. */
    switch(i)
    {
      case 0: /* scalar argument 1, ordinal of first row */
        if (inDataLen != sizeof(start))
          return SQLUDR_ERROR;
        memcpy(&start, inData, sizeof(start));
        break;

      case 1: /* scalar argument 2, number of result rows */
        if (inDataLen != sizeof(num_result_rows))
          return SQLUDR_ERROR;
        memcpy(&num_result_rows, inData, sizeof(num_result_rows));
        break;

      default:
        return SQLUDR_ERROR;
    }
  }

  // one-time check for output parameters
  if (udrinfo->num_return_values != 2)
    return SQLUDR_ERROR;

  // set result row pointers only once
  // ---------------------------------

  // first output is ordinal
  outParam = &udrinfo->return_values[0];
  outData = rowDataSpace2 + outParam->data_offset;
  outDataLen = (int) outParam->data_len;

  if (outDataLen != sizeof(int))
    return SQLUDR_ERROR;
  else
    ordinal = (int *) (outData);

  // second output is the Fibonacci number
  outParam = &udrinfo->return_values[1];
  outData = rowDataSpace2 + outParam->data_offset;
  outDataLen = (int) outParam->data_len;

  if (outDataLen != sizeof(long))
    return SQLUDR_ERROR;
  else
    fibonacci_number = (long *) (outData);

  // prepare the output row
  memset(rowDataSpace2, '\0', udrinfo->out_row_length);
  if (start < 0)
    start = 0;
  *fibonacci_number = 0;
  previous_result = 1;

  // produce fibonacci numbers and emit rows
  // ---------------------------------------
  for (*ordinal=0;
       *ordinal<start+num_result_rows && qstate == SQLUDR_Q_MORE;
       (*ordinal)++)
    {
      if (*ordinal >= start)
        emitRow(rowDataSpace2, 0, &qstate);

      if (*fibonacci_number > LONG_MAX/2)
        break;

      // pre-compute the next row
      temp = *fibonacci_number;
      *fibonacci_number += previous_result;
      previous_result = temp;
    }

  qstate = SQLUDR_Q_EOD;
  emitRow(rowDataSpace2,0,&qstate);

  return retcode;

}/* main */
