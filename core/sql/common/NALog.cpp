//**********************************************************************
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


#include "errno.h"
#include "string.h"
#include "NALog.h"
#include "NAStdioFile.h"
#include "CatSQLShare.h"

#define FILE_COULD_NOT_BE_ACCESSED 20356 //Error 20356 - File cannot be accessed.
#define TIME_KWORD "Time: "
#define TIME_KWORD_LENGTH 6
#define PROCESS_FILENAME_LENGTH 48 // Length of process name
#define PROCESS_KWORD "Process: "
#define PROCESS_KWORD_LENGTH 9
#define HEADER_LEN 16


// This macro is called whenever a failure occurs and we want to stop logging
#define TURNOFFLOGGING \
      m_bLoggingEnabled = FALSE; \
      m_bLogFailure = TRUE; 

// This macro is called in DEBUG code to return an error code 20356 when the log 
// cannot be accessed. It's up to the caller to handle error 20356. 
// In RELEASE code, the logging attribute it turned off
// and the operation continues.
#ifdef _DEBUG
#define THROWERROREXCP \
      if (GetLastError() != ENOERR) { \
	return FILE_COULD_NOT_BE_ACCESSED; \
      }

#endif

// -------------------------------------------------------------------------
// Constructor
// -------------------------------------------------------------------------
CNALogfile::CNALogfile()
: m_bPrintTime (TRUE),
  m_bPrintProcessID (TRUE),
  m_bLoggingEnabled (FALSE),
  m_bFlushAtLogTime (TRUE),
  m_bPrintLogHeader (TRUE),
  m_bDashToUnderscore (FALSE),
  m_eLoggingLevel (CNALogfile::eLEVEL1),
  m_bClearLogAtOpen (FALSE),
  m_bLogFailure (FALSE),
  m_pLogName (NULL) 
{
  // Check for envvar:  SQLMX_UTIL_LOGNAME
  // If the envvar defined, get the log name and log attributes
  // from the variable
  char * pLogfileInfo = getenv( "SQLMX_UTIL_LOGNAME" );

  if (pLogfileInfo != NULL)
  {
    ScanEnvVar (pLogfileInfo);

    // Set up to indicate that the envvar was processed successfully and
    // we are ready to start logging
    m_bLoggingEnabled = TRUE;
  }
}

// -------------------------------------------------------------------------
// Destructor
// -------------------------------------------------------------------------
CNALogfile::~CNALogfile()
{
  if (m_pLogName)
    delete [] m_pLogName;
}

// -------------------------------------------------------------------------
// Helper:  ScanEnvVar
//
// This method scans the contents of the environment variable:
//    SQLMX_LOGNAME  (pEnvVarInfo)
// and sets up the requested information in the CNALog class
// -------------------------------------------------------------------------
void
CNALogfile::ScanEnvVar ( char *pEnvVarInfo )
{
  Int32 length = 0;
  char *pStartOfToken = pEnvVarInfo;
  char *pNextToken = GetNextToken (pStartOfToken, length);
  while (pNextToken)
  {
    NAString token (pNextToken, length);
    token.toLower();
    if (token == "clear")
      m_bClearLogAtOpen = TRUE;
    else if (token == "1") 
      m_eLoggingLevel = CNALogfile::eLEVEL1;
    else if ( token == "2") 
      m_eLoggingLevel = CNALogfile::eLEVEL2;
    else if (token == "3")
      m_eLoggingLevel = CNALogfile::eLEVEL3;
    else 
    {
      m_pLogName = new char [token.length() + 1];
      strcpy (m_pLogName, token);
    }

    pStartOfToken = pNextToken + length; 
    pNextToken = GetNextToken (pStartOfToken, length);
  }
}

// ------------------------------------------------------------------------
// Helper: GetNextToken
//
// This method extract the next token from the string requested through
// the environment variable (SQLMX_LOGNAME)
//
// It returns a pointer to the start of the token and its length
// ------------------------------------------------------------------------
char *
CNALogfile::GetNextToken (char *pStrToScan, Int32 &length)
{
  char *pStartOfToken = pStrToScan;

  // If we are at the end of the string, return NULL.
  if (pStartOfToken[0] == '\0')
    return NULL;

  // if the first character is the comma, point past it.
  if (pStartOfToken[0] == ',')
    pStartOfToken++;

  char *pEndOfStr = pStartOfToken + strlen(pStartOfToken);

  // Take care of any leading blanks before the options start
  while(pStartOfToken < pEndOfStr && *pStartOfToken == ' ')
    pStartOfToken++;

  if (pStartOfToken >= pEndOfStr)
    return NULL;

  char *pNextComma = strchr (pStartOfToken, ',');
  if (pNextComma)
    length = pNextComma - pStartOfToken;
  else
    length = (Int32)strlen(pStartOfToken);

  return pStartOfToken;
}
  
// ----------------------------------------------------------------------
// Helper:  GetTimeToLog
//
// This method gets a time value:  Time: Thu Jun  9 08:30:49 2005
// to write to the log
// ----------------------------------------------------------------------
NAString
CNALogfile::GetTimeToLog(void)
{
  Int32 length = CTIME_LENGTH + TIME_KWORD_LENGTH + 2;
  char timeBuf[CTIME_LENGTH + TIME_KWORD_LENGTH + 2];
  char *pTimeBuf = (char *)&timeBuf;
  strcpy (pTimeBuf, TIME_KWORD);
  pTimeBuf = pTimeBuf + strlen (TIME_KWORD);
  GetTimeString(pTimeBuf, FALSE);
  strcat(pTimeBuf, " ");
  NAString timeStr (timeBuf);
  return timeStr;
}

// ----------------------------------------------------------------------
// Helper:  GetProcessToLog
//
// This method gets a process value:  Process: \SQUAW.$:1:437:130336604
// to write to the log
// ----------------------------------------------------------------------
NAString
CNALogfile::GetProcessToLog()
{
    char processIdStrBuf[101];
    size_t processIdStrLen = 0;
    Int32 rtnCode = -1;
    processIdStrBuf[0] = '\0';
    rtnCode = SqlShareLnxGetMyProcessIdString(processIdStrBuf,   // char   * outBuf
                                              100,                       // size_t   outBufMaxLen
                                              &processIdStrLen); // size_t * computedProcessIdStrLen
   // CAT_ASSERT (rtnCode == XZFIL_ERR_OK);
    processIdStrBuf[processIdStrLen] = '\0';
  NAString processStr(processIdStrBuf);


  return processStr;
}

// ----------------------------------------------------------------------
// Helper: ConvertDashToUnderscore
//
// This helper method convert any dash in log filename into underscore
// ----------------------------------------------------------------------
void 
CNALogfile::ConvertDashToUnderscore(char *pLogName)
{
  Int32 pos = 0;
  while (pLogName[pos] != '\0')
  {
    if (pLogName[pos] == '-')
      pLogName[pos] = '_';
    pos++;
  }
}

// ----------------------------------------------------------------------
// Method: generic Open
//
// If the open fails, logging is turned off
// In debug mode, an exception is thrown
// ----------------------------------------------------------------------
Int32
CNALogfile::Open( void )
{
  // If file already opened, return
  if (IsOpen())
    return 0;

  // If the logname was not specified, go generate one
  if (!m_pLogName)
  {
    m_pLogName = new char [CTIME_LENGTH + 4 + 1];
    strcpy(m_pLogName, "LOG-");
    GetTimeString(m_pLogName+4, TRUE); 
  }

  if (m_bDashToUnderscore)
  {
    ConvertDashToUnderscore(m_pLogName);
  }

  // Open the file for read access to see if it exists and if it exists
  // contains a log header
  CNAStdioFile::EOpenMode stdioMode = CNAStdioFile::eRead; 
      
  try
  {
    NAString headerStr ("*************** ");

    if (inherited::Open(m_pLogName, stdioMode))
    {
      // This is an existing file, go ahead and read the first row from the 
      // file.  It must be a log header record.  If not, then turn off logging 
      // and exit.
      // The header looks like:  
      //  *************** Time: <t> Process: <p> Log opened *************** 
      //    <t> ::= Thu Jun  9 08:30:49 2005 
      //    <p> ::= \SQUAW.$:1:437:130336604
      NAString rowRead;
      rowRead.append(' ', HEADER_LEN);
      if (ReadString((char *)rowRead.data(), HEADER_LEN))
      {
        // Found data in the file
        // But the data is not a header row, turn off logging
        if ( !isHeaderString((char *)rowRead.data()) )
        {
          TURNOFFLOGGING
          m_lastError = 2;
          inherited::Close();
#ifdef _DEBUG
          THROWERROREXCP
#endif
          return 0;
        }
      }
      inherited::Close();
    }

    // Go ahead and open the log for write/append access
    stdioMode = 
       m_bClearLogAtOpen ? CNAStdioFile::eWrite : CNAStdioFile::eAppend;
    if (!inherited::Open(m_pLogName, stdioMode))
    {
      TURNOFFLOGGING
#ifdef _DEBUG
      THROWERROREXCP
#endif
    }

    m_bLoggingEnabled = TRUE;

    // Log file is good, write the log header
    headerStr += GetTimeToLog();
    headerStr += GetProcessToLog();
    headerStr += (" Log opened ***************");

    // The header line should not have the time and process ID prepended
    // store the current values, turn off the options, log the row, then
    // reset the time and process ID values.
    NABoolean printProcessId = GetPrintProcessID();
    NABoolean printTime = GetTimePrint();
    SetTimePrint (FALSE);
    SetPrintProcessID (FALSE);
    if (m_bPrintLogHeader)
      Write (headerStr, headerStr.length());

#ifdef _DEBUG
    // Try logging different amounts of data
    char * pLogSizeLimit = getenv( "LOG_SIZE_LIMIT" );
    if (pLogSizeLimit)
    {
      NAString str
        ("asdfghjkl;qwertyuiopzxcvbnm,./asdfghjkl;qwertyuiopzxcvbnm,./");
      NAString newstr;
      Int32 upperlimit = atoi(pLogSizeLimit);
      if (upperlimit > 0)
      {
        for (Int32 i = 0; i < upperlimit; i++)
          newstr += str;
        if (newstr.length() > 0)
        {
          newstr += "\n";
          Int32 retcode = inherited::WriteString(newstr);
          NAString numWritten;
	  numWritten = LongToNAString((Lng32)retcode);
          numWritten += ": Number of bytes written \n";
          inherited::WriteString(numWritten);
        }
      }
    }
#endif

    SetTimePrint(printTime);
    SetPrintProcessID (printProcessId);
  }
  // The CNAStdioFile Open method could throw an exception.
  // Just catch it, turn off logging, and return
  catch (...) 
  { 
    TURNOFFLOGGING 
#ifdef _DEBUG
    THROWERROREXCP
#endif
    return 0; 
  } 
  return 0;
}

// ----------------------------------------------------------------------
// Method::Open
//
// This method set the log name and open mode in the class structures
// and calls the generic Open method.
// ----------------------------------------------------------------------
Int32  
CNALogfile::Open( char *pLogName, CNAStdioFile::EOpenMode mode )
{
  // If log name has already been specified, delete it
  // Caller may want to open a different file
  if (m_pLogName)
  {
    Close();
    delete [] m_pLogName;
  }

  // Save the log name
  m_pLogName = new char [strlen(pLogName) + 1];
  strcpy (m_pLogName, pLogName);

  // Set clearLogAtOpen
  m_bClearLogAtOpen = (mode == CNAStdioFile::eAppend) ? FALSE : TRUE;

  // Open file
  return Open();
}
   
Int32
CNALogfile::Open( NAString logName, CNAStdioFile::EOpenMode mode )
{
   return Open((char *) logName.data(), mode);
}


// ----------------------------------------------------------------------
// Method: Close
//
// Errors are ignored
// ----------------------------------------------------------------------
void CNALogfile::Close()
{
  if (IsOpen())
    inherited::Close();
  m_bLoggingEnabled = FALSE;
}

// ----------------------------------------------------------------------
// Method: Write
// 
// If the write was not successful, FALSE is returned and logging is
// turned off.
// FALSE is also returned if Write is called and logging is not activated
// or the log file is not opened
// ----------------------------------------------------------------------
NABoolean CNALogfile::Write(const char * pBuffer, Lng32 buflen)
{
  if (!IsLoggingEnabled())
    return FALSE;

  // If for some reason logging is enabled but the log operation failed
  // don't try to log again 
  if (GetLogFailure())
  {
    SetLoggingEnabled (FALSE);
    return FALSE;
  }

  // Construct the row to log
  NAString row;
  if (m_bPrintTime)
    row += GetTimeToLog();
  if (m_bPrintProcessID)
    row += GetProcessToLog();

  // print line
  // rsm - I am not sure if this is still required
  if (buflen > 0)
  {
    // Strings such as error msgs come here as "ERROR[nn] Msg text.\r\n".
    // The WINNT fwrite takes that final \n and replaces it with \r\n,
    // so we end up with "ERROR[nn] Msg text.\r\r\n"
    // which appears in the log as "ERROR[nn] Msg text.^M"
    // and screws up MKS diff utility (on regressions e.g.).
    //
    // And yes, it is safe to cast away const-ness to do this;
    // we are only overwriting *existing* chars in the buffer
    // *and* if this if-test is true the buffer contents must be
    // coming from a run-time rather than a static/pure/read-only/const data
    //
    if (buflen > 1 && pBuffer[buflen-1] == '\n' && pBuffer[buflen-2] == '\r')
    {
      char *bufxxx = (char *)pBuffer;
      bufxxx[buflen-2] = '\n';
      bufxxx[buflen-1] = '\0';
      buflen--;
    }
  }

  // Add the buffer;
  row += pBuffer;
  row += "\n";
  if (row.length() > 0)
  {
    Int32 retcode = inherited::WriteString(row);
    if (!retcode)
    {
      TURNOFFLOGGING;
#ifdef _DEBUG
      THROWERROREXCP
#endif
      return FALSE;
    }
  }
  return TRUE;
}

// ----------------------------------------------------------------------
// Method: Log
//
// Log a message to the current log file
// ----------------------------------------------------------------------
Int32
CNALogfile::Log (const char *pBuffer, ELoggingLevel level) 
{
  if (IsLoggingEnabled() && m_eLoggingLevel >= level)
  {
#ifndef _DEBUG
    // Only log LEVEL3 in debug mode
    if (level == CNALogfile::eLEVEL3)
      return 0;
#endif
    // If the write wrote something - flush the buffer
    if (Write (pBuffer, (Lng32)strlen(pBuffer)))
    {
      if (Flush() != ENOERR)
      {
        TURNOFFLOGGING;
#ifdef _DEBUG
        THROWERROREXCP
#endif
      }
    }
  }
  
  return 0;
}

NABoolean 
CNALogfile::isHeaderString (const char *strHeader)
{
  NAString header ("*************** ");
  NAString headerTemp = NAString(strHeader, HEADER_LEN);    
  
  if (headerTemp == header)
  {
    return TRUE;
  }
  return FALSE;
}

