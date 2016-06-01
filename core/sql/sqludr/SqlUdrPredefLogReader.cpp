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
const int MaxLineLength = 5000;
const int ParseStatusNumChars = 2;

const char TruncationError     = 'T';
const char FieldParserError    = 'E';
const char CharConversionError = 'C';

static void setParseError(char err, std::string &parseErrorField)
{
  if (err != ' ' && parseErrorField.size() < ParseStatusNumChars)
    if (parseErrorField.find_first_of(err) == std::string::npos)
      parseErrorField.append(1, err);
}

// the next two methods set output columns from string input
// and instead of raising exceptions, they set the parse error field

static void setIntOutputColumn(const UDRInvocationInfo &info,
                               int paramNum,
                               const char *src,
                               std::string &parseErrorField)
{
  char *endptr = NULL;
  int result = 0;

  long num = strtol(src, &endptr, 10);

  if (num <= std::numeric_limits<int32_t>::max() &&
      num >= std::numeric_limits<int32_t>::min())
    result = num;
  else
    {
      if (num < std::numeric_limits<int32_t>::min())
        result = std::numeric_limits<int32_t>::min();
      else
        result = std::numeric_limits<int32_t>::max();
      setParseError(TruncationError, parseErrorField);
    }

  if (endptr == NULL || *endptr != 0 || endptr == src)
    {
      // no valid number read, treat this as a NULL value
      info.out().setNull(paramNum);
      if (endptr && *endptr != 0)
        // strtol didn't consume all the characters, this is a
        // parse error
        setParseError(FieldParserError, parseErrorField);
    }
  else
    info.out().setInt(paramNum, result);
}

static void setCharOutputColumn(const UDRInvocationInfo &info,
                                int paramNum,
                                const char *src,
                                std::string &parseErrorField)
{
  int srcLen = strlen(src);
  const tmudr::TypeInfo &outType = info.out().getColumn(paramNum).getType();
  int tgtLen = outType.getByteLength();

  if (srcLen > tgtLen)
    {
      // set an error indicator and truncate the string
      setParseError(TruncationError, parseErrorField);
      srcLen = tgtLen;

      // remove any incomplete UTF-8 characters
      if ((static_cast<unsigned char>(src[srcLen]) >> 6) == 2)
        // the first character to be cut off is a continuation
        // character (starting with bits '01'), continue to remove
        // characters until the removed character is no longer a
        // continuation character (i.e. it's the first character
        // of the sequence).
        do
          {
            srcLen--;
          }
        while ((static_cast<unsigned char>(src[srcLen]) >> 6) == 2);
    }

  if (srcLen > 0 || !outType.getIsNullable())
    info.out().setString(paramNum, src, srcLen);
  else
    // treat a blank value as a NULL value
    info.out().setNull(paramNum);
}

// validate UTF-8 characters in inputLine and copy
// to inputLineValidated, replacing any invalid characters
// with the "replacement character" U+FFFD
bool validateCharsAndCopy(char *outBuf, int outBufLen,
                          const char *inBuf, int inBufLen,
                          int &resultLen)
{
  bool result = true;
  int srcPos = 0;
  int tgtPos = 0;
  unsigned char c;
  int byte = 1;
  int tgtLimit = outBufLen - 4 - 1; // leave room for larger replacement

  while (srcPos < inBufLen && tgtPos < tgtLimit)
    {
      c = inBuf[srcPos];

      if (c < 0x80)
        {
          // ASCII character
          outBuf[tgtPos++] = inBuf[srcPos++];
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

          if (numBytes > inBufLen - srcPos)
            validUTF8Char = 0; // incomplete sequence

          // make sure we have numBytes continuation bytes following
          // in the range of 0x80 ... 0xbf
          for (int p=1; p<numBytes; p++)
            {
              unsigned char s = inBuf[srcPos+p];

              if (s < 0x80 || s >= 0xc0)
                validUTF8Char = 0;
            }

          if (validUTF8Char)
            {
              for (int j=0; j<numBytes; j++)
                outBuf[tgtPos++] = inBuf[srcPos++];
            }
          else
            {
              // U+FFFD in UTF-8
              const unsigned char replacementChar[] =
                { 0xef, 0xbf, 0xbd };

              for (int k=0; k<sizeof(replacementChar); k++)
                outBuf[tgtPos++] = (char) replacementChar[k];
              srcPos += numBytes;
              result = false;
            }
        }
    }

  outBuf[tgtPos] = 0;
  resultLen = tgtPos;

  return result;
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
//  p: force parallel execution on workstation environment with
//     virtual nodes (debug build only)
//
// Returned columns:
//
// log_ts        timestamp(6),
// severity      char(10 bytes) character set utf8,
// component     varchar(50 bytes) character set utf8,
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

// compiler interface class for TRAF_CPP_EVENT_LOG_READER

class ReadCppEventsUDFInterface : public UDR
{
public:

  enum ColNum {
    LOG_TS_COLNUM = 0,
    SEVERITY_COLNUM,
    COMPONENT_COLNUM,
    NODE_NUMBER_COLNUM,
    CPU_COLNUM,
    PIN_COLNUM,
    PROCESS_NAME_COLNUM,
    SQL_CODE_COLNUM,
    QUERY_ID_COLNUM,
    MESSAGE_COLNUM,
    LOG_FILE_NODE_COLNUM, // optional columns
    LOG_FILE_NAME_COLNUM,
    LOG_FILE_LINE_COLNUM,
    PARSE_STATUS_COLNUM
  };

  ReadCppEventsUDFInterface() : UDR(), logDir_(NULL), infile_(NULL) {}

  // override any methods where the UDF author would
  // like to change the default behavior

  virtual void describeParamsAndColumns(UDRInvocationInfo &info); // Binder
  virtual void describeDesiredDegreeOfParallelism(UDRInvocationInfo &info,
                                                  UDRPlanInfo &plan);// Optimizer
  virtual void processData(UDRInvocationInfo &info,
                           UDRPlanInfo &plan);
  virtual ~ReadCppEventsUDFInterface();

private:
  bool useParallelExecForVirtualNodes_;
  DIR *logDir_;
  FILE *infile_;

};

extern "C" UDR * TRAF_CPP_EVENT_LOG_READER(
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

  if (info.par().getNumColumns() > 1)
    throw UDRException(38221,
                       "There should be no more than one input parameters to the call to %s, got %d",
                       info.getUDRName().data(),
                       info.par().getNumColumns());
  else if (info.par().getNumColumns() == 1)
    {
      if (!info.par().isAvailable(0) ||
          !info.par().getSQLTypeClass(0) == TypeInfo::CHARACTER_TYPE)
        throw UDRException(38222,
                           "Expecting a character constant as first parameter of the call to %s",
                           info.getUDRName().data());

      std::string options = info.par().getString(0);

      // add an additional formal parameter for the options value
      info.addFormalParameter(
           ColumnInfo("OPTIONS",
                      info.par().getColumn(0).getType()));

      // validate options
      for (std::string::iterator it = options.begin();
           it != options.end();
           it++)
        {
          switch (*it)
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
            case 'p':
              // debug option, use parallel execution even with virtual nodes
              useParallelExecForVirtualNodes_ = true;
              break;
#endif

            default:
              {
                throw UDRException(38223,
                                   "Option %c not supported in first parameter of the call to %s",
                                   *it,
                                   info.getUDRName().data());
              }
            }
        }
    } // got 1 input parameter

  // add the output columns
  TableInfo &outTable = info.out();

  outTable.addColumn(
       ColumnInfo("LOG_TS",
                  TypeInfo(TypeInfo::TIMESTAMP,
                           0,
                           true,
                           6)));
  outTable.addCharColumn   ("SEVERITY",    10, true);
  outTable.addVarCharColumn("COMPONENT",   50, true);
  outTable.addIntColumn    ("NODE_NUMBER",     true);
  outTable.addIntColumn    ("CPU",             true);
  outTable.addIntColumn    ("PIN",             true);
  outTable.addCharColumn   ("PROCESS_NAME",12, true);
  outTable.addIntColumn    ("SQL_CODE",        true);
  outTable.addVarCharColumn("QUERY_ID",   200, true);
  outTable.addVarCharColumn("MESSAGE",   4000, true);

  if (addFileColumns)
    {
      outTable.addIntColumn    ("LOG_FILE_NODE");
      outTable.addVarCharColumn("LOG_FILE_NAME",200);
      outTable.addIntColumn    ("LOG_FILE_LINE");
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

void ReadCppEventsUDFInterface::processData(UDRInvocationInfo &info,
                 UDRPlanInfo &plan)
{
  // input parameters
  bool addFileColumns = false;
  bool doTrace = false;
  int pid = (int) getpid();

  if (info.par().getNumColumns() >= 1)
    {
      std::string inputPar = info.par().getString(0);

      for (std::string::iterator it = inputPar.begin();
           it != inputPar.end();
           it++)
        switch (*it)
          {
          case ' ':
          case 'f': // handled below with addFileColumns
          case 'p': // handled at compile time
            break;

          case 't':
            doTrace = true;
          break;

          default:
            // rely on compiler interface to check for errors
            break;
          }
    }

  if (doTrace)
  {
    printf("(%d) EVENT_LOG_READER runtime call\n", pid);
    fflush(stdout);
  }

  if (info.out().getNumColumns() != 10)
    if (info.out().getNumColumns() == 14)
      addFileColumns = true;
    else
      throw UDRException(
           38000,
           "Expecting 10 or 14 result columns, got %d",
           info.out().getNumColumns());

  char inString[1024] = "";
  int inLen = 0;
  FILE *cFile = NULL;
  struct dirent *dirEntry = NULL;
  int lineNumber = 0;           // line number in current file
  int status = 0;
  int columnNum = 0;
  int columnSequenceError = 0;   // error that prevents us from parsing further
  char inputLine[MaxLineLength];          // space for all fields in character form
  char inputLineValidated[MaxLineLength]; // inputLine after validation
  char cFileInputLine[MaxLineLength];
  char *ok = NULL;                        // status of fgets
  int haveRowToEmit = 0;
  int appendPos = 0;
  int numLogLocations = 3 ;

  for(int logLocationIndex = 0; logLocationIndex < numLogLocations; logLocationIndex++) 
  {
    char* logrootdir = NULL;
    switch (logLocationIndex) 
    {
    case 0: // sqroot, for all logs other than dcs
      logrootdir = getenv("MY_SQROOT");
      if (strlen(logrootdir) > 1000)
	throw UDRException(38001, "SQROOT is longer than 1000 characters");
      break ;
    case 1:
      logrootdir = getenv("DCS_INSTALL_DIR");
      if (!logrootdir)
	throw UDRException(38001, "DCS_INSTALL_DIR not set");
      else if (strlen(logrootdir) > 1000)
	throw UDRException(38001, "DCS_INSTALL_DIR is longer than 1000 characters");
      break ;
    case 2:
      logrootdir = getenv("REST_INSTALL_DIR");
      if (!logrootdir)
	throw UDRException(38001, "REST_INSTALL_DIR not set");
      else if (strlen(logrootdir) > 1000)
	throw UDRException(38001, "REST_INSTALL_DIR is longer than 1000 characters");
      break ;
    default:
      throw UDRException(38001, "Internal error in determining logroot directory");
    }
      
    std::string logDirName(logrootdir);
    std::string confFileName(logrootdir);
    std::string logFileName;
    std::string eventLogFileName(logrootdir);
    
    logDirName += "/logs";
    confFileName += "/conf/log4cxx.trafodion.config";  // conf file logic not coded for dcs yet
    
    if (doTrace)
    {
      printf("(%d) EVENT_LOG_READER open log dir %s\n", pid, logDirName.data());
      fflush(stdout);
    }
    
    errno = 0;
    if (logDir_ != NULL)
    {
      // this may happen if we exited a previous call with an exception
      closedir(logDir_);
      logDir_ = NULL;
    }
    
    logDir_ = opendir(logDirName.data());
    if (logDir_ == NULL)
    {
      //rest server logs are only on master node
      //If udr runs on diff node, ignore if rest log folder does not exist
      if(logLocationIndex == 2) 
        continue;

      throw UDRException(
			 38002,
			 "Error %d on opening directory %s",
			 (int) errno, logDirName.data());
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
      fclose(cFile);
      cFile = NULL;
    }

    if (addFileColumns)
      // log_file_node is the same for every row generated by this process
      info.out().setInt(LOG_FILE_NODE_COLNUM, info.getMyInstanceNum());

    // ---------------------------------------------------------------------------
    // Loop over the files in the log directory
    // ---------------------------------------------------------------------------
    while (1)
    {
      // read the next file in the log directory
      
      errno = 0;
      struct dirent *dirEntry = readdir(logDir_);
      
      if (errno != 0)
	throw UDRException(
			   38003,
			   "Error %d on reading from directory %s",
			   (int) errno, logDirName.data());
      
      // last file seen, we are done
      if (dirEntry == NULL)
	break;
      
      if (doTrace)
      {
	printf("(%d) EVENT_LOG_READER examining log file %s\n", pid, dirEntry->d_name);
	fflush(stdout);
      }

      const char *fileName = dirEntry->d_name;
      size_t nameLen = strlen(fileName);
      const char *suffix =  NULL;
      const char *expectedSuffixPart = ".log";
      size_t expectedSuffixLenMax = strlen(expectedSuffixPart);
      if (logLocationIndex == 0)
	expectedSuffixLenMax += 2 ; // for "*log.1" rollover files
      else if (logLocationIndex == 1)
	expectedSuffixLenMax += 11 ; // for "*.log.yyyy-mm-dd" rollover files
      
      
      if (nameLen > expectedSuffixLenMax)
      suffix = &fileName[nameLen-expectedSuffixLenMax];

      // parse the file name to see whether this is a file we want to look at, 
      // allow some fixed string values as well as any name configured in 
      // the config file 
      if (suffix && strstr(suffix, expectedSuffixPart) != NULL &&
	  (strstr(fileName, "master_exec_")          == fileName ||
	   strstr(fileName, eventLogFileName.data()) == fileName ||
	   strstr(fileName, "tm_")                   == fileName ||
	   strstr(fileName, "mxlobsrvr_")            == fileName ||
	   strstr(fileName, "sscp")                  == fileName ||
	   strstr(fileName, "ssmp")                  == fileName ||
	   strstr(fileName, "mon")                   == fileName ||
	   strstr(fileName, "pstartd")               == fileName ||
	   strstr(fileName, "wdg")                   == fileName || 
	   strstr(fileName, "udr_")                  == fileName ||
	   strstr(fileName, "dcs-")                  == fileName ||
	   strstr(fileName, "rest-")                  == fileName 
	   ))
      {
        if (infile_ != NULL)
        {
	  fclose(infile_);
	  infile_ = NULL;
	}
	
        logFileName = logDirName + "/" + fileName;
	
        // Open the input file
        infile_ = fopen(logFileName.data(), "r");
        if (infile_ == NULL)
          throw UDRException(
			     38001,
			     "Error %d returned when opening log file %s",
			     status, fileName);
	
        if (doTrace)
        {
	  printf("(%d) EVENT_LOG_READER fopen\n", pid);
	  fflush(stdout);
	}

        if (addFileColumns)
          info.out().setString(LOG_FILE_NAME_COLNUM, fileName);
	
        lineNumber = 0;
        std::string messageTextField;
        std::string rowParseStatus;
	
        // ---------------------------------------------------------------------
        // Loop over the lines of the file
        // ---------------------------------------------------------------------
        while ((ok = fgets(inputLine, sizeof(inputLine), infile_)) != NULL)
        {
	  int year, month, day, hour, minute, second, fraction;
	  char fractionSeparator[2];
	  char *currField = inputLineValidated;
	  char *nextField = NULL;
	  int numChars = 0;
	  int numItems = 0;
	  int intFieldVal;
	  int lineLength = strlen(inputLine);
	  int lineLengthValidated = 0;
	  std::string lineParseError;
	  
	  lineNumber++;
	  
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
	      extraStatus = fgets(extraChars, sizeof(extraChars), infile_);
	    }
	    while (extraStatus != NULL && extraChars[strlen(extraChars)-1] != '\n');
	    
	    setParseError(TruncationError, lineParseError);
	  }
	  
	  if (! validateCharsAndCopy(inputLineValidated,
				     sizeof(inputLineValidated),
				     inputLine,
				     lineLength,
				     lineLengthValidated))
	  {
	    if (doTrace)
	    {
	      printf("(%d) EVENT_LOG_READER invalid UTF8 char line %d\n",
		     pid, lineNumber);
	      fflush(stdout);
	    }
	    setParseError(CharConversionError, lineParseError);
	  }
	  
	  
	  // try to read the timestamp at the beginning of the line. Example:
	  // 2014-10-30 20:49:53,252
	  numItems = sscanf(currField,
			    "%4d-%2d-%2d %2d:%2d:%2d %1[,.] %6d%n",
			    &year, &month, &day, &hour, &minute,
			    &second, fractionSeparator, &fraction, &numChars);
	  
	  if (numItems == 8)
	  {
	    // We were able to read a timestamp field
	    
	    // Emit previous row, we have seen the start of next row
	    if (haveRowToEmit)
	    {
	      // set final two columns, message text and parse error
	      setCharOutputColumn(info,
				  MESSAGE_COLNUM,
				  messageTextField.data(),
				  rowParseStatus);
	      if (addFileColumns)
		setCharOutputColumn(info,
				    PARSE_STATUS_COLNUM,
				    rowParseStatus.c_str(),
				    rowParseStatus);
	      emitRow(info);
	      if (doTrace)
	       {
		 printf("(%d) EVENT_LOG_READER emit\n", pid);
		 fflush(stdout);
	       }
	    }
	    
	    // we read a line that will produce an output row, initialize
	    // some fields for this output row
	    haveRowToEmit = 0;
	    appendPos = 0;
	    messageTextField.erase();
	    rowParseStatus = lineParseError;
	    
	    // When we see a comma between time and fraction, we interpret
	    // that as a fraction that is specified in milliseconds. Convert
	    // to microseconds. When it's specified with a dot, we interpret
	    // the fraction as microseconds (SQL syntax).
	    if (*fractionSeparator == ',')
	      fraction *= 1000;
	    
	    char buf[100];
	    snprintf(buf, sizeof(buf),
		     "%04d-%02d-%02d %02d:%02d:%02d.%06d",
		     year, month, day, hour, minute, second, fraction);
	    setCharOutputColumn(info, LOG_TS_COLNUM, buf, rowParseStatus);
	  }
	  else
	  {
	    if (!haveRowToEmit)
	    {
	      // no valid timestamp and we did not have a previous line
	      // with a timestamp
	      if (numItems > 6)
		numItems = 6;
	      
	      if (doTrace)
	      {
		printf("(%d) EVENT_LOG_READER Read only %d of 7 timestamp fields: %s\n",
		       pid, numItems, currField);
		fflush(stdout);
	      }
              
	      // return a NULL value if we fail to parse the timestamp
	      info.out().setNull(LOG_TS_COLNUM);
	      rowParseStatus = lineParseError;
	      setParseError(FieldParserError, rowParseStatus);
	    }
	    else
	    {
	      // no valid timestamp and we have a row to emit
	      // consider this line as a continuation of previous row
	      // (add a blank instead of a line feed, though)
	      messageTextField += " ";
	      messageTextField += currField;
	      
	      // add any parse errors from this line
	      for (std::string::iterator it = lineParseError.begin();
		   it != lineParseError.end();
		   it++)
		setParseError(*it, rowParseStatus);
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
	      setParseError(FieldParserError, rowParseStatus);
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
	      // (after currField, which points to a NUL byte)
	      // otherwise set the message text field to an empty string
	      if (messageTextField.empty())
		messageTextField =
		  (currField-inputLineValidated < lineLengthValidated) ?
		  currField+1 : currField;
	      setParseError(FieldParserError, rowParseStatus);
	    }
	    
	    // now that we have the non-blank portion of the value,
	    // copy it into the output column
	    switch (columnNum)
	    {
	    case 2:
	      setCharOutputColumn(info,
				  SEVERITY_COLNUM,
				  startOfVal,
				  rowParseStatus);
	      break;
	      
	    case 3:
	      setCharOutputColumn(info,
				  COMPONENT_COLNUM,
				  startOfVal,
				  rowParseStatus);
	      break;
	      
	    case 4:
	      setIntOutputColumn(info,
				 NODE_NUMBER_COLNUM,
				 startOfVal,
				 rowParseStatus);
	      break;
	      
	    case 5:
	      setIntOutputColumn(info,
				 CPU_COLNUM,
				 startOfVal,
				 rowParseStatus);
	      break;
	      
	    case 6:
	      setIntOutputColumn(info,
				 PIN_COLNUM,
				 startOfVal,
				 rowParseStatus);
	      break;
	      
	    case 7:
	      setCharOutputColumn(info,
				  PROCESS_NAME_COLNUM,
				  startOfVal,
				  rowParseStatus);
	      break;
	      
	    case 8:
	      setIntOutputColumn(info,
				 SQL_CODE_COLNUM,
				 startOfVal,
				 rowParseStatus);
	      break;
	      
	    case 9:
	      setCharOutputColumn(info,
				  QUERY_ID_COLNUM,
				  startOfVal,
				  rowParseStatus);
	      // we read all required fields,
	      // next field is the message text
	      if (messageTextField.empty())
		messageTextField = nextField;
	      break;
	    }
	    
	    currField = nextField;
	  } // loop over column numbers 2-9
	  
            // do some final adjustments
	  if (!haveRowToEmit)
	  {
	    int numLeadingBlanks = messageTextField.find_first_not_of(' ');
	    
	    if (numLeadingBlanks > 0 && numLeadingBlanks != std::string::npos)
	      messageTextField.erase(0, numLeadingBlanks);
	    
	    if (addFileColumns)
	      info.out().setInt(LOG_FILE_LINE_COLNUM, lineNumber);
	  }
	  
	  haveRowToEmit = 1;
	} // loop over the lines of the file
	
        if (haveRowToEmit) 
        {
	  // set final two columns, message text and parse error
	  setCharOutputColumn(info,
			      MESSAGE_COLNUM,
			      messageTextField.data(),
			      rowParseStatus);
	  if (addFileColumns)
	    setCharOutputColumn(info,
				PARSE_STATUS_COLNUM,
				rowParseStatus.c_str(),
				rowParseStatus);
	  // Emit a row
	  emitRow(info);
	  if (doTrace)
	  {
	    printf("(%d) EVENT_LOG_READER emit\n", pid);
	    fflush(stdout);
	  }
	  haveRowToEmit = 0;
            appendPos = 0;
	}
        // Close the input file
        if (infile_)
        {
	  fclose(infile_);
	  infile_ = NULL;
	}
        if (doTrace)
        {
	  printf("(%d) EVENT_LOG_READER fclose\n", pid);
	  fflush(stdout);
	}
	
      } // file name matched our pattern
    } // while (1) - list files in the directory
    
    closedir(logDir_);
    logDir_ = NULL;
  } // for numLogLocations
}

ReadCppEventsUDFInterface::~ReadCppEventsUDFInterface()
{
  if (logDir_ != NULL)
    closedir(logDir_);
  if (infile_ != NULL)
    fclose(infile_);
}
