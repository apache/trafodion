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
#include "sqludr.h"

/*
SELECT ts, userid, session-id
FROM sessionize (
TABLE(SELECT * FROM clicks PARTITION BY userid ORDER BY ts),
’ts’,
60
) WHERE userid = 'super-user';

create table clicks (userid char(32), ts TIME(6), ip-addr char(15)) ;

*/


SQLUDR_LIBFUNC SQLUDR_INT32 SESSIONIZE(
  SQLUDR_CHAR           *in_data,
  SQLUDR_TMUDF_TRAIL_ARGS)
{
  char            *inData = NULL;
  SQLUDR_PARAM    *inParam = NULL;
  int             inDataLen = 0;

  char            *outData = NULL;
  SQLUDR_PARAM    *outParam = NULL;
  int             outDataLen = 0;

  char            *clicksColumn = NULL;
  int             interval;
  unsigned int    i;

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
        clicksColumn = (char *) malloc(inDataLen + 1);
        strncpy(clicksColumn,inData,inDataLen);
        clicksColumn[inDataLen] = '\0';
	endCursor = &(clicksColumn[inDataLen-1]);
	/* remove trailing pattern matches */
	while((*endCursor == ' ') && (clicksColumn < endCursor))
	  endCursor--;
	*(endCursor+1) = '\0';
        break;

      case 1:
        memcpy(&interval, inData, sizeof(int));
        break;

      default:
        return SQLUDR_ERROR;
    }
  }

  maxUidLen = (udrinfo->table_inputs[0].params[0]).data_len;
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

    /* Extract row contents */
    for(i = 0; i < udrinfo->table_inputs[0].num_params; i++)
    {
      inParam = &udrinfo->table_inputs[0].params[i];
      inData = rowDataSpace1 + inParam->data_offset;
      inDataLen = (int) inParam->data_len;

      /* if name == "TS"; */
      if(strcmp(inParam->name, clicksColumn) == 0)
      {
        memcpy(&in_TS, inData, inDataLen);
      }
      else if(strcmp(inParam->name, "USERID") == 0)
      {
        SQLUDR_UINT32 bytesToCopy = inDataLen;
        if (bytesToCopy > maxUidLen)
          bytesToCopy = maxUidLen;
        strncpy(inUserID, inData, bytesToCopy);
      }
      else
      {
        /* expect only two inputs? */
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

    for(i = 0; i < udrinfo->num_return_values; i++)
    {
      outParam = &udrinfo->return_values[i];
      outData = rowDataSpace2 + outParam->data_offset;
      outDataLen = (int) outParam->data_len;

      /* if name == "TS"; */
      if(strcmp(outParam->name, clicksColumn) == 0)
      {
        memcpy(outData, &in_TS, outDataLen);
      }
      else if(strcmp(outParam->name, "USERID") == 0)
      {
	memset(outData, ' ', outDataLen);
        strcpy(outData,inUserID); /* assumes strlen(inUserID) <= outDataLen */
      }
      else if(strcmp(outParam->name, "SESSION_ID") == 0)
      {
        memcpy(outData,&sessionID,outDataLen);
      }
      else
      {
        /* I informed MXCMP that I will produce only 3 values. */
        return SQLUDR_ERROR;
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
