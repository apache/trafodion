// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2000-2015 Hewlett-Packard Development Company, L.P.
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
#include <limits>
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

// C++ Sessionize TMUDF
// Note: This code is the same as shown in the tutorial on the wiki
// https://wiki.trafodion.org/wiki/index.php/Tutorial:_The_object-oriented_UDF_interface

class Sessionize : public UDRInterface
{
  // override any methods where the UDF author would
  // like to change the default behavior

  void describeParamsAndColumns(UDRInvocationInfo &info); // Binder
  void processData(UDRInvocationInfo &info,
                   UDRPlanInfo &plan);

};

extern "C" UDRInterface * SESSIONIZE_DYNAMIC(
     const UDRInvocationInfo *info)
{
  return new Sessionize();
}

void Sessionize::describeParamsAndColumns(UDRInvocationInfo &info)
{
  // First, do some validation of the parameters

  // Make sure we have exactly one table-valued input, otherwise
  // generate a compile error
  if (info.getNumTableInputs() != 1)
    throw UDRException(38000,
                       "%s must be called with one table-valued input",
                       info.getUDRName().data());

  // check whether the first two arguments identify
  // an arbitrary column and an exact numeric column
  if (info.par().canGetString(0))
    // This will raise an error if the column name
    // specified in the first parameter doesn't exist
    info.in().getColumn(info.par().getString(0));
  else
    throw UDRException(38001,"First scalar parameter must be a string constant");

  // make sure the second parameter specifies the name of
  // an existing input column of type exact numeric
  if (info.par().canGetString(1))
    {
      const TypeInfo &typ = info.in().getColumn(
        info.par().getString(1)).getType();

      if (typ.getSQLTypeSubClass() != TypeInfo::EXACT_NUMERIC_TYPE)
        throw UDRException(38002, "Second scalar parameter must be the name of an exact numeric column");
    }
  else
    throw UDRException(38003,"Second scalar parameter must be a string constant");
 
  // Second, define the output parameters

  // add the column for the session id
  info.getOutputTableInfo().addLongColumn("SESSION_ID");
 
  // Make all the input table columns also output columns,
  // those are called "pass-through" columns. The default
  // parameters of this method add all the columns of the
  // first input table.
  info.addPassThruColumns();
}

void Sessionize::processData(UDRInvocationInfo &info,
                             UDRPlanInfo &plan)
{
  // read the three parameters and convert the first two into column numbers
  int userIdColNum    = info.in(0).getColNum(info.par().getString(0));
  int timeStampColNum = info.in(0).getColNum(info.par().getString(1));
  long timeout        = info.par().getLong(2);

  // variables needed for computing the session id
  long lastTimeStamp = 0;
  std::string lastUserId;
  long currSessionId = 0;

  // loop over input rows
  while (getNextRow(info))
  {
    long timeStamp = info.in(0).getLong(timeStampColNum);
    std::string userId = info.in(0).getString(userIdColNum);

    if (lastUserId != userId)
      {
        // reset timestamp check and start over with session id 0
        lastTimeStamp = 0;
        currSessionId = 1;
        lastUserId = userId;
      }

    long tsDiff = timeStamp - lastTimeStamp;

    if (tsDiff > timeout && lastTimeStamp > 0)
      currSessionId++;
    else if (tsDiff < 0)
      throw UDRException(
           38001,
           "Got negative or descending timestamps %ld, %ld",
           lastTimeStamp, timeStamp);

    lastTimeStamp = timeStamp;

    // produce session id output column
    info.out().setLong(0, currSessionId);

    // produce the remaining columns and emit the row
    info.copyPassThruData();
    emitRow(info);
  }
}

class FibonacciUDF : public UDRInterface
{
  // override any methods where the UDF author would
  // like to change the default behavior

  void processData(UDRInvocationInfo &info,
                   UDRPlanInfo &plan);

};

extern "C" UDRInterface * Fibonacci(
     const UDRInvocationInfo *info)
{
  return new FibonacciUDF();
}

void FibonacciUDF::processData(UDRInvocationInfo &info,
                               UDRPlanInfo &plan)
{
  // input parameters: (int startRow, int numResultRows)
  int startRow = info.par().getInt(0);
  int numResultRows = info.par().getInt(1);
  long fibonacciNumber = 0;
  long previousResult = 1;
  long temp = 0;
  int ordinal=0;

  // produce fibonacci numbers and emit rows
  // ---------------------------------------
  while (1)
    {
      if (ordinal >= startRow)
        {
          // set result parameters (int ordinal, long fibonacci_number)
          info.out().setInt(0, ordinal);
          info.out().setLong(1, fibonacciNumber);
          emitRow(info);
        }

      // did we produce numResultRows already?
      if (++ordinal >= startRow+numResultRows)
        break;

      if (fibonacciNumber > std::numeric_limits<long>::max()/2)
        throw UDRException(38001, "Upper limit exceeded");

      // pre-compute the next row
      temp = fibonacciNumber;
      fibonacciNumber += previousResult;
      previousResult = temp;
    }
}
