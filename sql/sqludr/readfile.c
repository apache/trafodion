// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2014 Hewlett-Packard Development Company, L.P.
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
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include "sqludr.h"

int getVCLen(SQLUDR_PARAM *inParam, char *input_row)
{
  char *vcInd = ((char *) input_row) + inParam->vc_ind_offset;
  unsigned long vcIndLen = inParam->vc_ind_len;
  int inDataLen = 0;
  if (vcIndLen == 2)
    inDataLen = *((unsigned short *) vcInd);
  else
    inDataLen = *((unsigned long *) vcInd);
  return inDataLen;
}

void setVCLen(SQLUDR_PARAM *outParam,  char *output_row, int len)
{
  char *vcInd = ((char *) output_row) + outParam->vc_ind_offset;
  outParam->vc_ind_len = 2;
  *((unsigned short *) vcInd) = (unsigned short) len;
}

SQLUDR_LIBFUNC SQLUDR_INT32 READFILE(SQLUDR_CHAR *input_row,
                                     SQLUDR_TMUDF_TRAIL_ARGS)
{
  SQLUDR_Q_STATE qstate;
  SQLUDR_PARAM *inParam = NULL;
  SQLUDR_PARAM *outParam = NULL;
  short nullInd = 0;
  char inString[1024] = "";
  int inLen = 0;
  FILE *infile = NULL;
  char *output_row = rowDataSpace2;
  int pid = (int) getpid();
  int doTrace = 0;

  if (doTrace)
  {
    printf("(%d) READFILE calltype %d input %p\n", pid,
           (int) calltype, input_row);
    fflush(stdout);
  }

  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  outParam = &(udrinfo->return_values[0]);
  inParam = &(udrinfo->inputs[0]);

  // Copy null indicator from input row
  memcpy(&nullInd, input_row + inParam->ind_offset, 2);

  // Copy input string from input row
  memset(inString, 0, sizeof(inString));
  inLen = getVCLen(inParam, input_row);
  memcpy(inString, input_row + inParam->data_offset, inLen);

  if (doTrace)
  {
    printf("(%d) READFILE ind %d len %d data [%s]\n", pid,
           (int) nullInd, (int) inLen, inString);
    fflush(stdout);
  }

  /* Open the input file */
  infile = fopen(inString, "r");
  if (infile == NULL)
  {
    strcpy(sqlstate, "38001");
    sprintf(msgtext, "Could not open input file");

    if (doTrace)
    {
      printf("(%d) READFILE ERROR: %s\n", pid, strerror(errno));
      fflush(stdout);
    }
    return SQLUDR_ERROR;
  }

  if (doTrace)
  {
    printf("(%d) READFILE fopen\n", pid);
    fflush(stdout);
  }

  /* Fill output row with zeros */
  memset(output_row, 0, udrinfo->out_row_length);

  qstate = SQLUDR_Q_MORE;
  while (qstate == SQLUDR_Q_MORE)
  {
    char line[1024];
    char *ok;
    int len;

    /* Read the next input line */
    ok = fgets(line, 1024, infile);
    if (ok)
    {
      /* Remove the newline character */
      len = (int) strlen(line);
      if (len > 0 && line[len - 1] == '\n')
      {
        line[len - 1] = '\0';
        len--;
      }

      if (doTrace)
      {
        printf("(%d) READFILE line [%s]\n", pid, line);
        fflush(stdout);
      }

      /* Copy to the output row */
      memcpy(output_row + outParam->data_offset, line, len);
      setVCLen(outParam, output_row, len);

      /* Emit a row */
      emitRow(output_row, 0, &qstate);
      if (doTrace)
      {
        printf("(%d) READFILE emit\n", pid);
        fflush(stdout);
      }
    }
    else
    {
      break;
    }

  } // while (qstate == SQLUDR_Q_MORE)

  /* Emit EOD */
  qstate = SQLUDR_Q_EOD;
  emitRow(output_row, 0, &qstate);
  if (doTrace)
  {
    printf("(%d) READFILE emit EOD\n", pid);
    fflush(stdout);
  }

  /* Close the input file */
  if (infile)
    fclose(infile);
  if (doTrace)
  {
    printf("(%d) READFILE fclose\n", pid);
    fflush(stdout);
  }

  return SQLUDR_SUCCESS;
}


/* Function to read from events.X.log file.
This functions reads from these log files, parses each row and formats into 4 columns.
The columns are event_time (char(19)), event_component (char(10)), event_severity (char(9)), event_text(varchar(1024))

The required TMUDF can be registered with
create table_mapping function readevents(filename varchar(512))
returns (event_time char(19), component char(10), severity char(9), text varchar(1024))
external name 'READEVENTS' library mytmudfs;

Salient points
1) udf reads from 10 files $MY_SQROOT/logs/events.X.log, where X is [1,10]
2) If no files are present it returns 0 rows.
3) The files are read in numeric name order.
4) this UDF is meant to be executed seraially
5) if it sees an row format it does not understand, most of thw row is put in the text column and remaining cols are null
6) UDF can continue after it sees a row with an unexpected format
7) event_time column can be cast to timestamp(0) in select statement

Calling select statment can be something like

select cast(evt_time as timestamp(0)), component, severity, cast(txt as char(350))
from UDF(readevents(TABLE(select * from (values
('no-op')) as val(noop)),
'no-op')) XO(evt_time, component, severity, txt);

*/

SQLUDR_LIBFUNC SQLUDR_INT32 READEVENTS(SQLUDR_CHAR *input_row,
                                     SQLUDR_TMUDF_TRAIL_ARGS)
{
  SQLUDR_Q_STATE qstate;
  SQLUDR_PARAM *evt_time_param = NULL;
  SQLUDR_PARAM *evt_component_param = NULL;
  SQLUDR_PARAM *evt_severity_param = NULL;
  SQLUDR_PARAM *evt_text_param = NULL;
  short nullInd = 0;
  char inString[1024] = "";
  int inLen = 0;
  FILE *infile = NULL;
  char *output_row = rowDataSpace2;
  int pid = (int) getpid();
  int doTrace = 0;

  if (doTrace)
  {
    printf("(%d) READFILE calltype %d input %p\n", pid,
           (int) calltype, input_row);
    fflush(stdout);
  }

  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  // the four output columns
  evt_time_param = &(udrinfo->return_values[0]);
  evt_component_param = &(udrinfo->return_values[1]);
  evt_severity_param = &(udrinfo->return_values[2]);
  evt_text_param = &(udrinfo->return_values[3]);

  /*
  // to be uncommented when parameters to push predicates are supported
  SQLUDR_PARAM *inParam = NULL;
  inParam = &(udrinfo->inputs[0]);

  // Copy null indicator from input row
  memcpy(&nullInd, input_row + inParam->ind_offset, 2);

  // Copy input string from input row
  memset(inString, 0, sizeof(inString));
  inLen = getVCLen(inParam, input_row);
  memcpy(inString, input_row + inParam->data_offset, inLen);
  */

  char* sqroot = getenv("MY_SQROOT");
  if (strlen(sqroot) > 1000)
  {
    strcpy(sqlstate, "38001");
    sprintf(msgtext, "SQROOT is longer than 1000 characters");
    return SQLUDR_ERROR;
  }

  qstate = SQLUDR_Q_MORE;
  char *ok = NULL;
  int len = 0;
  char *evt_time_start = NULL;
  char *evt_time_end = NULL;
  int evt_time_len = 0;
  char *evt_severity_start = NULL;
  char *evt_severity_end = NULL;
  int evt_severity_len = 0;
  char *evt_text_start = NULL;
  char *evt_text_end = NULL;
  int evt_text_len = 0;
  int blank_len = 0;
  int haveValidLine = 0;
  char line[1024];
  char name_num[3];
  int fileloop;
  int foundFile = 0;

  for(fileloop=0; fileloop<10; fileloop++) // loop over 10 log files. Absent files are OK
  {
    memset(inString, 0, 1024);
    strcat(inString, sqroot);
    strcat(inString, "/logs/events.");
    sprintf(name_num, "%d", fileloop+1);
    if (fileloop < 9)
      name_num[1] = 0;
    else
      name_num[2] = 0;
    strcat(inString, name_num);
    strcat(inString, ".log");

    if (doTrace)
      {
        printf("(%d) READFILE ind %d len %d data [%s]\n", pid,
               (int) nullInd, (int) inLen, inString);
        fflush(stdout);
      }

    /* Open the input file */
    infile = fopen(inString, "r");
    if (infile == NULL)
    {
      if ((fileloop == 9) && !foundFile)
      {
        strncpy(sqlstate, "38001", 6);
        strncpy(msgtext, "Could not open event log files", 31);

        if (doTrace)
        {
          printf("(%d) READFILE ERROR: %s\n", pid, strerror(errno));
          fflush(stdout);
        }
        // return SQLUDR_ERROR; /* this temporary, till we get the UDF to return an error without a core */
        continue; // remove this line when previous line is in effect
      }
      else
        continue;
    }

    if (doTrace)
      {
        printf("(%d) READFILE fopen\n", pid);
        fflush(stdout);
      }

    foundFile = 1;


    while (qstate == SQLUDR_Q_MORE)
      {
        evt_time_start = NULL;
        evt_time_end = NULL;
        evt_time_len = 0;
        evt_severity_start = NULL;
        evt_severity_end = NULL;
        evt_severity_len = 0;
        evt_text_start = NULL;
        evt_text_end = NULL;
        evt_text_len = 0;

        /* Fill output row with zeros */
        memset(output_row, 0, udrinfo->out_row_length);

        if (!haveValidLine) {
          /* Read the next input line */
          ok = fgets(line, 1024, infile);
        }
        if (ok || haveValidLine)
          {
            haveValidLine = 0;

            // parse the first column, event_time from line. Should be of format [05/21/2014 19:10:16]
            evt_time_start = strchr(line, '[');
            if (!evt_time_start)
              evt_time_len = -1;
            evt_time_end = strchr(line, ']');
            if (!evt_time_end)
              evt_time_len = -1;
            if (evt_time_len != -1)
              evt_time_len = (evt_time_end - evt_time_start) - 1;
            if (evt_time_len > 0) {
              memcpy(output_row + evt_time_param->data_offset,
                     evt_time_start+1, evt_time_len);
            }
            else
              *((short *)(output_row + evt_time_param->ind_offset)) = -1;

            // second column event_component is set to NULL now, as seapilot is not producing this information
            *((short *)(output_row + evt_component_param->ind_offset)) = -1;

            // parse the third column, event_severity from line. Should look something like ERROR:3, WARNING:4
            // the number after the ascii severity string is discarded for now.
            if (!evt_time_end)
              evt_time_end = &line[0];
            blank_len = strspn(evt_time_end+1, " ");
            if (blank_len == 0 || blank_len > 10)
              evt_severity_len = -1;
            evt_severity_start = evt_time_end+1+blank_len ;
            if (evt_severity_len != -1)
              evt_severity_end = strchr(evt_severity_start, ':');
            if (!evt_severity_end)
              evt_severity_len = -1;
            if (evt_severity_len != -1)
              evt_severity_len = (evt_severity_end - evt_severity_start);
            if (evt_severity_len > 0) {
              memcpy(output_row + evt_severity_param->data_offset,
                     evt_severity_start, evt_severity_len);
            }
            else
              *((short *)(output_row + evt_severity_param->ind_offset)) = -1;

            // parse the fourth column, event_text from line. It is whatever comes after the severity text till end of line.
            // If the next line does not have an event_time, it is added to this event_text as it could be query id or the
            // message for a 15001 parse error. 15001 parse error is handled slightly differently from query ids
            if (!evt_severity_end)
              evt_severity_end = evt_time_end;
            blank_len = strspn(evt_severity_end+1, "0123456789 ");
            evt_text_start = evt_severity_end+1+blank_len ;
            evt_text_end = strchr(evt_severity_start, '\n');
            if (!evt_text_end)
              evt_text_len = -1;
            if (evt_text_len != -1)
              evt_text_len = (evt_text_end - evt_text_start);
            if (evt_text_len > 0) {
              memcpy(output_row + evt_text_param->data_offset,
                     evt_text_start, evt_text_len);
              while (ok && !haveValidLine)
              {
                ok = fgets(line, 1024, infile);
                if (ok) {
                  if (strncmp(line, "SQL Query id", 12) == 0) {
                    len = strlen(line);
                    memcpy(output_row + evt_text_param->data_offset +evt_text_len,
                           " ", 1);
                    memcpy(output_row + evt_text_param->data_offset +evt_text_len +1,
                           line, len);
                    evt_text_len += len + 1;
                  }
                  else {
                    evt_time_start = strchr(line, '[');
                    evt_time_end = strchr(line, ']');
                    if (evt_time_start && evt_time_end && ((evt_time_end - evt_time_start)==20))
                      haveValidLine = 1 ; // the line we read should be used for the next row
                    else {
                      // anything other than query id or event_time. Main case is 15001 error messages
                      len = strlen(line);
                      memcpy(output_row + evt_text_param->data_offset +evt_text_len,
                             " ", 1);
                      memcpy(output_row + evt_text_param->data_offset +evt_text_len +1,
                             line, len+1);
                      evt_text_len += len + 2;
                    } // else haveValidLine
                  } // else SQL Query id
                } // if ok
                // check if we are overflowing the column
                if (evt_text_len > 1024) {
                  *((short *)(output_row + evt_text_param->ind_offset)) = -1;
                  break ;
                }
              } // while ok && !haveValidLine
              setVCLen(evt_text_param, output_row, evt_text_len);
            }
            else
              *((short *)(output_row + evt_text_param->ind_offset)) = -1;

            if (doTrace)
              {
                printf("(%d) READFILE line [%s]\n", pid, line);
                fflush(stdout);
              }

            /* Emit a row */
            emitRow(output_row, 0, &qstate);
            if (doTrace)
              {
                printf("(%d) READFILE emit\n", pid);
                fflush(stdout);
              }
          }
        else
          {
            break; // out of while loop when we have read EOF
          }

      } // while (qstate == SQLUDR_Q_MORE)
    /* Close the input file */
    if (infile)
      fclose(infile);
    if (doTrace)
      {
        printf("(%d) READFILE fclose\n", pid);
        fflush(stdout);
      }
  } // for loop

  /* Emit EOD */
  qstate = SQLUDR_Q_EOD;
  emitRow(output_row, 0, &qstate);
  if (doTrace)
  {
    printf("(%d) READFILE emit EOD\n", pid);
    fflush(stdout);
  }

  return SQLUDR_SUCCESS;
}
