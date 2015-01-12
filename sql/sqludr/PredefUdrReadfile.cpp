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
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <limits>
#include "sqludr.h"

using namespace tmudr;

// -------------------------------------
// constants
// -------------------------------------

// max. line length we can parse (give or take a few),
// this should be the size of the "message" output column
// plus enough room for the other fields
const int maxLineLength = 5000;

const char TruncationError     = 'T';
const char FieldParserError    = 'E';
const char CharConversionError = 'C';

static void setParseError(char err,
                          char *parseErrorField)
{
  if (err != ' ' && parseErrorField)
    {
      // parse_error_output_field points to a 2 character field
      // that is initialized with blanks. Add the code to the end
      // if it is not yet recorded in the field
      if (parseErrorField[0] == ' ')
        parseErrorField[0] = err;
      else if (parseErrorField[1] == ' ' &&
               parseErrorField[0] != err)
        parseErrorField[1] = err;
    }
}

static void setIntOutputColumn(char *outputRow,
                               SQLUDR_PARAM *param,
                               const char *src,
                               char *parseErrorField)
{
  SQLUDR_INT32 *tgt = (SQLUDR_INT32 *) (outputRow + param->data_offset);
  SQLUDR_INT16 *indPtr = (SQLUDR_INT16 *) (outputRow + param->ind_offset);
  char *endptr = NULL;

  long num = strtol(src, &endptr, 10);

  if (num <= std::numeric_limits<int32_t>::max() &&
      num >= std::numeric_limits<int32_t>::min())
    *tgt = (SQLUDR_INT32) strtol(src, &endptr, 10);
  else
    {
      if (num < std::numeric_limits<int32_t>::min())
        *tgt = std::numeric_limits<int32_t>::min();
      else
        *tgt = std::numeric_limits<int32_t>::max();
      setParseError(TruncationError, parseErrorField);
    }

  if (endptr == NULL || *endptr != 0 || endptr == src)
    {
      // no valid number read, treat this as a NULL value
      *tgt = 0;
      *indPtr = -1;
      if (endptr && *endptr != 0)
        // strtol didn't consume all the characters, this is a
        // parse error
        setParseError(FieldParserError, parseErrorField);
    }
  else
    *indPtr = 0;
}

static void setCharOutputColumn(char *outputRow,
                                SQLUDR_PARAM *param,
                                const char *src,
                                char *parseErrorField)
{
  char *tgt = outputRow + param->data_offset;
  SQLUDR_INT16 *indPtr = (SQLUDR_INT16 *) (outputRow + param->ind_offset);

  strncpy(tgt, src, param->data_len);
  int len = strlen(src);
  if (len <= param->data_len)
    {
      if (len > 0)
        {
          // set the remainder of the field to blanks
          memset(tgt+len, ' ', param->data_len - len); 
        }
      else
        {
          // treat a blank value as a NULL value
          *indPtr = -1;
          memset(tgt, ' ', param->data_len);
        }
    }
  else
    setParseError(TruncationError, parseErrorField);
}

static void setVarCharOutputColumn(char *outputRow,
                                   SQLUDR_PARAM *param,
                                   const char *src,
                                   char *parseErrorField)
{
  int len = strlen(src);
  char *tgt = outputRow + param->data_offset;
  SQLUDR_INT16 *indPtr = (SQLUDR_INT16 *) (outputRow + param->ind_offset);

  strncpy(tgt, src, param->data_len);
  *indPtr = 0;
  if (len <= param->data_len)
    {
      // just to be clean, blank out remainder of the field
      memset(tgt+len, ' ', param->data_len - len); 

      if (len == 0)
        // treat a blank value as a NULL value
        *indPtr = -1;
    }
  else
    {
      len = param->data_len;
      setParseError(TruncationError, parseErrorField);
    }

  if (param->vc_ind_len == sizeof(SQLUDR_UINT16))
    *((SQLUDR_UINT16 *) (outputRow + param->vc_ind_offset)) = len;
  else // if (param->vc_ind_len == sizeof(SQLUDR_INT32))
    *((SQLUDR_UINT32 *) (outputRow + param->vc_ind_offset)) = len;
}

static void appendToVarCharOutputColumn(char *outputRow,
                                        SQLUDR_PARAM *param,
                                        const char *src,
                                        int& appendPos,
                                        char *parseErrorField)
{
  int len = strlen(src) + appendPos;
  char *tgt = outputRow + param->data_offset + appendPos;
  char *start = outputRow + param->data_offset;
  SQLUDR_INT16 *indPtr = (SQLUDR_INT16 *) (outputRow + param->ind_offset);

  if (appendPos < param->data_len)
    strncpy(tgt, src, (param->data_len - appendPos));
  *indPtr = 0;
    
  if (len <= param->data_len)
    {
      // just to be clean, blank out remainder of the field
      memset(start+len, ' ', param->data_len - len); 

      if (len == 0)
        // treat a blank value as a NULL value
        *indPtr = -1;
    }
  else
    {
      len = param->data_len;
      setParseError(TruncationError, parseErrorField);
    }

  if (param->vc_ind_len == sizeof(SQLUDR_UINT16))
    *((SQLUDR_UINT16 *) (outputRow + param->vc_ind_offset)) = len;
  else // if (param->vc_ind_len == sizeof(SQLUDR_INT32))
    *((SQLUDR_UINT32 *) (outputRow + param->vc_ind_offset)) = len;

  appendPos = len;
}



// -----------------------------------------------------------------
// Function to read event log files generated by Trafodion C++ code
//
// SQL Syntax to invoke this function:
//
//  select * from udf(event_log_reader( [options] ));
//
// The optional [options] argument is a character constant. The
// following options are supported:
//  f: add file name output columns (see below)
//  t: turn on tracing
//  d: loop in the runtime code, to be able to attach a debugger
//     (debug build only)
//  p: force parallel execution on workstation environment with
//     virtual nodes (debug build only)
//
// Returned columns:
//
// log_ts        timestamp(6),
// severity      char(10 bytes) character set utf8,
// component     char(24 bytes) character set utf8,
// node_number   integer,
// cpu           integer,
// pin           integer,
// process_name  char(12 bytes) character set utf8,
// sql_code      integer,
// query_id      varchar(200 bytes) character set utf8,
// message       varchar(4000 bytes) character set utf8
//
// if option "f" was specified, we have four more columns:
//
// log_file_node integer not null,
// log_file_name varchar(200 bytes) character set utf8 not null,
// log_file_line integer not null,
// parse_status  char(2 bytes) character set utf8 not null
//
// (log_file_node, log_file_name, log_file_line) form a unique key 
// for each result row. parse_status indicates whether there were
// any errors reading the information:
// '  ' (two blanks): no errors
// 'E'  (as first or second character): parse error
// 'T'  (as first or second character): truncation or over/underflow
//                                      occurred
// 'C'  (as first or second character): character conversion error
// -----------------------------------------------------------------

extern "C" SQLUDR_LIBFUNC SQLUDR_INT32 TRAF_CPP_EVENT_LOG_READER(
     SQLUDR_CHAR *input_row,
     SQLUDR_TMUDF_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  SQLUDR_Q_STATE qstate;

  // input parameters
  int addFileColumns = 0;
  int doTrace = 0;
  int startWithALoop = 0;
  int pid = (int) getpid();

  if (udrinfo->num_inputs >= 1)
    {
      char *options = input_row + udrinfo->inputs[0].data_offset;
      int optionsLen = (int) udrinfo->inputs[0].data_len;

      for (int i=0; i<optionsLen; i++)
        switch (options[i])
          {
          case ' ':
          case 'f': // handled below with addFileColumns
          case 'p': // handled at compile time
            break;

          case 't':
            doTrace = 1;
          break;

          case 'd':
            startWithALoop = 1;
          break;

          default:
            // rely on compiler interface to check for errors
            break;
          }
    }

  if (doTrace)
  {
    printf("(%d) EVENT_LOG_READER calltype %d input %p\n", pid,
           (int) calltype, input_row);
    fflush(stdout);
  }

  if (udrinfo->num_return_values != 10)
    if (udrinfo->num_return_values == 14)
      addFileColumns = 1;
    else
        {
          strcpy(sqlstate, "38000");
          snprintf(msgtext,
                   SQLUDR_MSGTEXT_SIZE,
                   "Expecting 10 or 14 result columns, got %d",
                   udrinfo->num_return_values);
          return SQLUDR_ERROR;
        }

  // the output parameters
  SQLUDR_PARAM *log_ts_param        = &(udrinfo->return_values[0]);
  SQLUDR_PARAM *severity_param      = &(udrinfo->return_values[1]);
  SQLUDR_PARAM *component_param     = &(udrinfo->return_values[2]);
  SQLUDR_PARAM *node_number_param   = &(udrinfo->return_values[3]);
  SQLUDR_PARAM *cpu_param           = &(udrinfo->return_values[4]);
  SQLUDR_PARAM *pin_param           = &(udrinfo->return_values[5]);
  SQLUDR_PARAM *process_name_param  = &(udrinfo->return_values[6]);
  SQLUDR_PARAM *sql_code_param      = &(udrinfo->return_values[7]);
  SQLUDR_PARAM *query_id_param      = &(udrinfo->return_values[8]);
  SQLUDR_PARAM *message_param       = &(udrinfo->return_values[9]);
  SQLUDR_PARAM *log_file_node_param = NULL;
  SQLUDR_PARAM *log_file_name_param = NULL;
  SQLUDR_PARAM *log_file_line_param = NULL;
  SQLUDR_PARAM *parse_status_param  = NULL;
  if (addFileColumns)
    {
      log_file_node_param = &(udrinfo->return_values[10]);
      log_file_name_param = &(udrinfo->return_values[11]);
      log_file_line_param = &(udrinfo->return_values[12]);
      parse_status_param  = &(udrinfo->return_values[13]);
    }

  char *output_row = rowDataSpace2;

  // pointers of the appropriate type for the output
  char          *log_ts_param_ptr        = (char *) &(output_row[log_ts_param->data_offset]);
  SQLUDR_INT32  *log_file_node_param_ptr = NULL;
  SQLUDR_INT32  *log_file_line_param_ptr = NULL;
  char          *parse_status_param_ptr  = NULL;
  if (addFileColumns)
    {
      log_file_node_param_ptr = (SQLUDR_INT32 *) &(output_row[log_file_node_param->data_offset]);
      log_file_line_param_ptr = (SQLUDR_INT32 *) &(output_row[log_file_line_param->data_offset]);
      parse_status_param_ptr  = &(output_row[parse_status_param->data_offset]);
    }

  // null indicators
  SQLUDR_INT16 *log_ts_param_ind        = (SQLUDR_INT16 *) &(output_row[udrinfo->return_values[0].ind_offset]);

  char inString[1024] = "";
  int inLen = 0;
  DIR *logDir = NULL;
  FILE *infile = NULL;
  FILE *cFile = NULL;
  struct dirent *dirEntry = NULL;
  int lineNumber = 0;           // line number in current file
  int status = 0;
  int columnNum = 0;
  int columnSequenceError = 0;   // error that prevents us from parsing further
  char inputLine[maxLineLength];          // space for all fields in character form
  char inputLineValidated[maxLineLength]; // inputLine after validation
  char cFileInputLine[maxLineLength];
  char *ok = NULL;                        // status of fgets
  int haveRowToEmit = 0;
  int appendPos = 0;

#ifndef NDEBUG
  if (startWithALoop)
    {
      int i=1;

      while (i < 2)
        i = 1-i;
    }
#endif

  char* sqroot = getenv("MY_SQROOT");
  if (strlen(sqroot) > 1000)
  {
    strcpy(sqlstate, "38001");
    snprintf(msgtext, SQLUDR_MSGTEXT_SIZE, "SQROOT is longer than 1000 characters");
    return SQLUDR_ERROR;
  }

  std::string logDirName(sqroot);
  std::string confFileName(sqroot);
  std::string logFileName;
  std::string eventLogFileName(sqroot);

  logDirName += "/logs";
  confFileName += "/conf/log4cpp.trafodion.config";

  if (doTrace)
    {
      printf("(%d) EVENT_LOG_READER open log dir %s\n", pid, logDirName.data());
      fflush(stdout);
    }

  errno = 0;
  logDir = opendir(logDirName.data());
  if (logDir == NULL)
    {
      status = errno;
      strcpy(sqlstate, "38002");
      snprintf(msgtext, SQLUDR_MSGTEXT_SIZE, "Error %d on opening directory %s",
               status, logDirName.data());
      return SQLUDR_ERROR;
    }

   cFile = fopen(confFileName.data(), "r");
   if (cFile)
     {
       char * name ;
       if (doTrace)
         {
           printf("(%d) EVENT_LOG_READER Conf file fopen\n", pid);
           fflush(stdout);
         }
       while ((ok = fgets(cFileInputLine, sizeof(cFileInputLine), 
                          cFile)) != NULL)
          {
            if (name = strstr(cFileInputLine, 
                              "log4j.appender.mxoAppender.fileName="))
              {
                name = name + strlen("log4j.appender.mxoAppender.fileName=");
                if (strstr(name, "${trafodion.log.dir}/"))
                  {
                    name = name + strlen("${trafodion.log.dir}/");
                    name[strlen(name) - 1] = '_' ;
                    eventLogFileName.assign(name);
                  }
                else
                  {
                  // log file directory different from expected.
                  // currently not supported.
                  }
                
                break ;
              }
            else
              memset((char *)cFileInputLine, 0, sizeof(cFileInputLine));
          }
     }
       

  // initialize output row
  memset(output_row, 0, udrinfo->out_row_length);

  if (addFileColumns)
    {
      // log_file_node is the same for every row generated by this process
      *log_file_node_param_ptr = udrinfo->instance_current;
    }

  qstate = SQLUDR_Q_MORE;

  while (1)
  {
    // read the next file in the log directory

    errno = 0;
    struct dirent *dirEntry = readdir(logDir);

    if (errno != 0)
    {
      status = errno;
      strcpy(sqlstate, "38003");
      snprintf(msgtext, SQLUDR_MSGTEXT_SIZE, "Error %d on reading from directory %s",
               status, logDirName.data());
      return SQLUDR_ERROR;
    }

    // last file seen, we are done
    if (dirEntry == NULL)
      break;

    if (doTrace)
      {
        printf("(%d) EVENT_LOG_READER examining log file %s\n", pid, dirEntry->d_name);
        fflush(stdout);
      }

    // parse the file name to see whether this is a file we want to look at
    if ((strstr(dirEntry->d_name, "mxosrvr_") == dirEntry->d_name ||
         strstr(dirEntry->d_name, "tmf") == dirEntry->d_name ||
         strstr(dirEntry->d_name, "master_exec_") == dirEntry->d_name ||
         strstr(dirEntry->d_name, eventLogFileName.data()) == 
         dirEntry->d_name ||
         strstr(dirEntry->d_name, "tm.log") == dirEntry->d_name) &&
        strcmp(strstr(dirEntry->d_name, ".log"), ".log") == 0)
      {

        logFileName = logDirName + "/" + dirEntry->d_name;

        /* Open the input file */
        infile = fopen(logFileName.data(), "r");
        if (infile == NULL)
          {
            status = errno;
            strncpy(sqlstate, "38001", 6);
            snprintf(msgtext, SQLUDR_MSGTEXT_SIZE, "Error %d returned when opening log file %s",
                     status, dirEntry->d_name);
            return SQLUDR_ERROR;
          }

        if (doTrace)
          {
            printf("(%d) EVENT_LOG_READER fopen\n", pid);
            fflush(stdout);
          }

        if (addFileColumns)
          {
            // set file name output column (same for all lines of this file)
            setVarCharOutputColumn(output_row,
                                   log_file_name_param,
                                   dirEntry->d_name,
                                   NULL); // no truncation warning for this metadata column
          }

        lineNumber = 0;

        // Loop over the lines of the file
        while ((ok = fgets(inputLine, sizeof(inputLine), infile)) != NULL)
          {
            int year, month, day, hour, minute, second, fraction;
            char fractionSeparator[2];
            char *currField = inputLineValidated;
            char *nextField = NULL;
            char *messageTextField = NULL;
            int numChars = 0;
            int numItems = 0;
            int intFieldVal;
            int lineLength = strlen(inputLine);
            int lineLengthValidated = 0;

            lineNumber++;

            if (parse_status_param_ptr)
              // initialize parse status for this row
              memset(parse_status_param_ptr, ' ', 2);

            // skip any empty lines, should not really happen
            if (lineLength < 2)
              {
                if (doTrace)
                  {
                    printf("(%d) EVENT_LOG_READER read short line %s\n", pid, inputLine);
                    fflush(stdout);
                  }

                continue;
              }

            // remove a trailing LF character
            if (inputLine[lineLength-1] == '\n')
              {
                lineLength--;
                inputLine[lineLength] = 0;
              }
            else
              {
                // skip over any text in the same line that
                // didn't get read
                char extraChars[4000];
                char *extraStatus;
                do
                  {
                    extraStatus = fgets(extraChars, sizeof(extraChars), infile);
                  }
                while (extraStatus != NULL && extraChars[strlen(extraChars)-1] != '\n');

                setParseError(TruncationError, parse_status_param_ptr);
              }

            // validate UTF-8 characters in inputLine and copy
            // to inputLineValidated, replacing any invalid characters
            // with the "replacement character" U+FFFD
            int srcPos = 0;
            int tgtPos = 0;
            unsigned char c;
            int byte = 1;
            int tgtLimit = sizeof(inputLineValidated) - 4 - 1;

            while (srcPos < lineLength && tgtPos < tgtLimit)
              {
                c = inputLine[srcPos];

                if (c < 0x80)
                  {
                    // ASCII character
                    inputLineValidated[tgtPos++] = inputLine[srcPos++];
                  }
                else
                  {
                    // non-ASCII or invalid byte sequence
                    int numBytes;
                    int validUTF8Char = 1;

                    if (c >= 0xc0 && c < 0xe0) // start of 2-byte sequence
                      numBytes = 2;
                    else if (c >= 0xe0 && c < 0xf0) // start of 3-byte sequence
                      numBytes = 3;
                    else if (c >= 0xf0 && c < 0xfc) // start of 4-byte sequence
                      numBytes = 4;
                    else
                      {
                        // invalid sequence, remove those one by one
                        numBytes = 1;
                        validUTF8Char = 0;
                      }

                    if (numBytes > lineLength - srcPos)
                      validUTF8Char = 0; // incomplete sequence

                    // make sure we have numBytes continuation bytes following
                    // in the range of 0x80 ... 0xbf
                    for (int p=1; p<numBytes; p++)
                      {
                        unsigned char s = inputLine[srcPos+p];

                        if (s < 0x80 || s >= 0xc0)
                          validUTF8Char = 0;
                      }

                    if (validUTF8Char)
                      {
                        for (int j=0; j<numBytes; j++)
                          inputLineValidated[tgtPos++] = inputLine[srcPos++];
                      }
                    else
                      {
                        // U+FFFD in UTF-8
                        const unsigned char replacementChar[] =
                          { 0xef, 0xbf, 0xbd };

                        for (int k=0; k<sizeof(replacementChar); k++)
                          inputLineValidated[tgtPos++] = (char) replacementChar[k];
                        srcPos += numBytes;

                        if (doTrace)
                          {
                            printf("(%d) EVENT_LOG_READER invalid %d byte UTF8 char %d in line %d\n",
                                   pid, numBytes, (int) inputLine[srcPos-numBytes], lineNumber);
                            fflush(stdout);
                          }
                        setParseError(CharConversionError, parse_status_param_ptr);
                      }
                  }
              }

            lineLengthValidated = tgtPos;
            inputLineValidated[lineLengthValidated] = 0;

            // read the timestamp at the beginning of the line. Example:
            // 2014-10-30 20:49:53,252
            numItems = sscanf(currField,
                              "%4d-%2d-%2d %2d:%2d:%2d %1[,.] %6d%n",
                              &year, &month, &day, &hour, &minute,
                              &second, fractionSeparator, &fraction, &numChars);

            if (numItems == 8)
              {

                 /* Emit previous row, we have seen the start of next row  */
                if (haveRowToEmit)
                  {
                    emitRow(output_row, 0, &qstate);
                    if (doTrace)
                      {
                        printf("(%d) EVENT_LOG_READER emit\n", pid);
                        fflush(stdout);
                      }
                    haveRowToEmit = 0;
                    appendPos = 0;
                  }

                // When we see a comma between time and fraction, we interpret
                // that as a fraction that is specified in milliseconds. Convert
                // to microseconds. When it's specified with a dot, we interpret
                // the fraction as microseconds (SQL syntax).
                if (*fractionSeparator == ',')
                  fraction *= 1000;
                  
                sprintf(log_ts_param_ptr,
                        "%04d-%02d-%02d %02d:%02d:%02d.%06d",
                        year, month, day, hour, minute, second, fraction);
              }
            else
              {
                if (!haveRowToEmit)
                  {
                    if (numItems > 6)
                      numItems = 6;
                    
                    if (doTrace)
                      {
                        printf("(%d) EVENT_LOG_READER Read only %d of 7 timestamp fields: %s\n",
                               pid, numItems, currField);
                        fflush(stdout);
                      }
                    
                    // return a NULL value if we fail to parse the timestamp
                    *log_ts_param_ind = -1;
                    setParseError(FieldParserError, parse_status_param_ptr);
                  }
                else
                  {
                    // no valid timestamp and we have a row to emit
                    // consdier this line as a continuation of previous row
                    messageTextField = currField;
                  }
              }

            if (!haveRowToEmit) 
              {
                // skip over the information already read
                currField = currField + numChars;
                
                // skip over the comma
                currField = strstr(currField, ",");
                if (currField)
                  currField++;
                else
                  {
                    // did not find a comma delimiter, this is a parse
                    // error, produce NULL values for remaining columns
                    // except the message
                    if (numChars > 0)
                      currField = inputLineValidated + (numChars-1);
                    else
                      currField = inputLineValidated;
                    *currField = 0;
                    messageTextField =
                      (numChars < lineLengthValidated) ? currField+1 : currField;
                    setParseError(FieldParserError, parse_status_param_ptr);
                  }
              }

            // read columns 2: SEVERITY - 9: QUERY_ID
            for (columnNum = 2; (columnNum <= 9 && !haveRowToEmit); columnNum++)
              {
                // find the next comma, the end of our field value
                char *endOfField = strstr(currField, ",");
                char *startOfVal = NULL;

                if (endOfField != NULL)
                  {
                    startOfVal = (endOfField != currField ? endOfField-1 : currField);

                    // next field starts after the comma
                    nextField = endOfField + 1;

                    // back up before the trailing comma, if the value is not empty
                    if (endOfField != currField)
                      endOfField--;

                    // remove trailing blanks
                    while (*endOfField == ' ')
                      endOfField--;

                    // place a nul-terminator at the end of the field
                    // (this overwrites the comma or a trailing blank)
                    if (endOfField != currField)
                      endOfField[1] = 0;
                    else
                      endOfField[0] = 0; // empty field

                    // from the end, go back to the preceding ":" or ","
                    // or until we reach the start of the current field
                    // This way, we skip field names like "CPU:" in CPU: 3
                    while (*startOfVal != ':' &&
                           *startOfVal != ',' &&
                           startOfVal != currField)
                      startOfVal--;

                    // skip the ":"
                    if (startOfVal != currField)
                      startOfVal++;

                    // skip leading blanks
                    while (*startOfVal == ' ')
                      startOfVal++;
                  } // found a comma delimiter
                else
                  {
                    // Did not find a comma delimiter. This could be a
                    // parse error or a missing optional column,
                    // produce NULL values for remaining columns
                    // except the message
                    if (currField != inputLineValidated)
                      // back up, since currField is now pointing
                      // at the first character of the message text
                      currField--;
                    *currField = 0;
                    startOfVal = currField;
                    nextField = currField;
                    // if there is any text left, point to it
                    // (after currField, which points to a NULL byte)
                    // otherwise set the message text field to an empty string
                    if (messageTextField == NULL)
                      messageTextField =
                        (currField-inputLineValidated < lineLengthValidated) ?
                        currField+1 : currField;
                    setParseError(FieldParserError, parse_status_param_ptr);
                  }

                // now that we have the non-blank portion of the value,
                // copy it into the output column
                switch (columnNum)
                  {
                  case 2:
                    setCharOutputColumn(output_row,
                                        severity_param,
                                        startOfVal,
                                        parse_status_param_ptr);
                    break;

                  case 3:
                    setCharOutputColumn(output_row,
                                        component_param,
                                        startOfVal,
                                        parse_status_param_ptr);
                    break;

                  case 4:
                    setIntOutputColumn(output_row,
                                       node_number_param,
                                       startOfVal,
                                       parse_status_param_ptr);
                    break;

                  case 5:
                    setIntOutputColumn(output_row,
                                       cpu_param,
                                       startOfVal,
                                       parse_status_param_ptr);
                    break;

                  case 6:
                    setIntOutputColumn(output_row,
                                       pin_param,
                                       startOfVal,
                                       parse_status_param_ptr);
                    break;

                  case 7:
                    setCharOutputColumn(output_row,
                                        process_name_param,
                                        startOfVal,
                                        parse_status_param_ptr);
                    break;

                  case 8:
                    setIntOutputColumn(output_row,
                                       sql_code_param,
                                       startOfVal,
                                       parse_status_param_ptr);
                    break;

                  case 9:
                    setVarCharOutputColumn(output_row,
                                           query_id_param,
                                           startOfVal,
                                           parse_status_param_ptr);
                    // we read all required fields,
                    // next field is the message text
                    if (messageTextField == NULL)
                      messageTextField = nextField;
                    break;
                  }

                currField = nextField;
              } // loop over column numbers 2-9

            // now we reached the last field in the file, the message text itself
            while (*messageTextField == ' ')
              messageTextField++;

            appendToVarCharOutputColumn(output_row,
                                      message_param,
                                      messageTextField,
                                      appendPos,
                                      parse_status_param_ptr);

            if (addFileColumns)
              {
                // line number column is computed, not from the log file
                *log_file_line_param_ptr = lineNumber;
              }

            haveRowToEmit = 1;
          } // loop over the lines of the file

        if (haveRowToEmit) 
          {
            /* Emit a row */
            emitRow(output_row, 0, &qstate);
            if (doTrace)
              {
                printf("(%d) EVENT_LOG_READER emit\n", pid);
                fflush(stdout);
              }
            haveRowToEmit = 0;
            appendPos = 0;
          }
        /* Close the input file */
        if (infile)
          fclose(infile);
        if (doTrace)
          {
            printf("(%d) EVENT_LOG_READER fclose\n", pid);
            fflush(stdout);
          }
      } // file name matched our pattern
  } // while (1) - list files in the directory

  /* Emit EOD */
  qstate = SQLUDR_Q_EOD;
  emitRow(output_row, 0, &qstate);
  if (doTrace)
  {
    printf("(%d) EVENT_LOG_READER emit EOD\n", pid);
    fflush(stdout);
  }

  return SQLUDR_SUCCESS;
}

// compiler interface class for TRAF_CPP_EVENT_LOG_READER

class ReadCppEventsUDFInterface : public TMUDRInterface
{
  // override any methods where the UDF author would
  // like to change the default behavior

  virtual void describeParamsAndColumns(UDRInvocationInfo &info); // Binder
  virtual void describeDesiredDegreeOfParallelism(UDRInvocationInfo &info,
                                                  UDRPlanInfo &plan);// Optimizer

private:
  bool useParallelExecForVirtualNodes_;

};

extern "C" TMUDRInterface * TRAF_CPP_EVENT_LOG_READER_CreateCompilerInterfaceObject(
     const UDRInvocationInfo *info)
{
  return new ReadCppEventsUDFInterface();
}

void ReadCppEventsUDFInterface::describeParamsAndColumns(
     UDRInvocationInfo &info)
{
  bool addFileColumns = false;

  useParallelExecForVirtualNodes_ = false;

  // This UDF is a table-valued function, no table-valued inputs
  if (info.getNumTableInputs() != 0)
    throw UDRException(38220,
                       "There should be no table-valued parameters to the call to %s, got %d",
                       info.getUDRName().data(),
                       info.getNumTableInputs());

  if (info.getNumActualParameters() > 1)
    throw UDRException(38221,
                       "There should be no more than one input parameters to the call to %s, got %d",
                       info.getUDRName().data(),
                       info.getNumActualParameters());
  else if (info.getNumActualParameters() == 1)
    {
      const tmudr::ParameterInfo &firstParam = info.getActualParameterInfo(0);

      if (firstParam.isAvailable() != tmudr::ParameterInfo::STRING_VALUE)
        throw UDRException(38222,
                           "Expecting a character constant as first parameter of the call to %s",
                           info.getUDRName().data());

      const char *options = firstParam.getStringValue();

      // add an additional formal parameter for the options value
      info.addFormalParameter(ParameterInfo("OPTIONS",
                                            firstParam.getType()));

      // validate options
      while (*options)
        {
          switch (*options)
            {
            case 'f':
              addFileColumns = true;
            break;

            case 't':
              // trace option, handled at runtime
            break;

            case ' ':
              // tolerate blanks in the options
              break;

#ifndef NDEBUG
            case 'd':
              // debug option, makes UDF loop at runtime to attach
              // debugger
              break;
            case 'p':
              // debug option, use parallel execution even with virtual nodes
              useParallelExecForVirtualNodes_ = true;
              break;
#endif

            default:
              {
                throw UDRException(38223,
                                   "Option %c not supported in first parameter of the call to %s",
                                   *options,
                                   info.getUDRName().data());
              }
            }
          options++;
        }
    } // got 1 input parameter

  // add the output columns
  TableInfo &outTable = info.getOutputTableInfo();

  outTable.addColumn(
       ColumnInfo("LOG_TS",
                  TypeInfo(TypeInfo::TIMESTAMP,
                           0,
                           true,
                           6)));
  outTable.addCharColumn   ("SEVERITY",    10, true);
  outTable.addCharColumn   ("COMPONENT",   24, true);
  outTable.addIntegerColumn("NODE_NUMBER",     true);
  outTable.addIntegerColumn("CPU",             true);
  outTable.addIntegerColumn("PIN",             true);
  outTable.addCharColumn   ("PROCESS_NAME",12, true);
  outTable.addIntegerColumn("SQL_CODE",        true);
  outTable.addVarCharColumn("QUERY_ID",   200, true);
  outTable.addVarCharColumn("MESSAGE",   4000, true);

  if (addFileColumns)
    {
      outTable.addIntegerColumn("LOG_FILE_NODE");
      outTable.addVarCharColumn("LOG_FILE_NAME",200);
      outTable.addIntegerColumn("LOG_FILE_LINE");
      outTable.addCharColumn   ("PARSE_STATUS",2);
    }
}

void ReadCppEventsUDFInterface::describeDesiredDegreeOfParallelism(
     UDRInvocationInfo &info,
     UDRPlanInfo &plan)
{
  // check for configurations with virtual nodes. Run the UDF serially
  // in those cases, since all the virtual nodes share the same node.
  int usesNoVirtualNodes = system("grep '^[ \t]*_virtualnodes ' $MY_SQROOT/sql/scripts/sqconfig >/dev/null");

  if (usesNoVirtualNodes != 0 || useParallelExecForVirtualNodes_)
    // this TMUDF needs to run once on each node, since every
    // parallel instance will be reading the local files on that node
    plan.setDesiredDegreeOfParallelism(UDRPlanInfo::ONE_INSTANCE_PER_NODE);
  else
    plan.setDesiredDegreeOfParallelism(1);
}
