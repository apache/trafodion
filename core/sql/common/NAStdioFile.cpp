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
#include <wchar.h> //for wcsncpy

#include "NAStdioFile.h"
#include "string.h"
#include "time.h"

// -------------------------------------------------------------------
CNAStdioFile::CNAStdioFile():CNADataSource()
{
    m_fileHandle = NULL;
}

CNAStdioFile::~CNAStdioFile()
{
  Close();
}

// -------------------------------------------------------------------
NABoolean CNAStdioFile::Open(const char *fileName, EOpenMode mode)
{
  m_lastError = ENOERR;
  if ( m_fileHandle != NULL )
    {
      // bulk reads should never be opening the file twice
      if ( mode == eReadBulk )
        throw -1;

      // we already have the file open so just reset file pointer back
      // to the beginning
      // fseek returns 0 if successful, -1 if not
      Int32 retcode = fseek(m_fileHandle, 0, SEEK_SET);
      if (retcode != ENOERR)
      {
        m_lastError = errno;
        return FALSE;
      }
      return TRUE;
    }

  // no file open so figure out which way to open it
  switch (mode)
    {
    case eRead:
      m_fileHandle = fopen(fileName, "r");
      break;
    case eReadBulk:
      m_fileHandle = fopen(fileName, "r");

      // if the open went OK, then set No Buffering on this file
      // if ( m_fileHandle != NULL )
      // setvbuf( m_fileHandle, NULL, IONBF, 0 );
      break;
    case eWrite:
      m_fileHandle = fopen(fileName, "w");
      break;
    case eAppend:
      m_fileHandle = fopen(fileName, "a");
      break;
    case eReadBinary:
      m_fileHandle = fopen(fileName, "rb");
      break;
    case eWriteBinary:
      m_fileHandle = fopen(fileName, "wb");
      break;
    case eReadWrite:
      m_fileHandle = fopen(fileName, "r+");
      break;
    default:
      // throw some error here since unknown mode??
      break;
    }

  if (m_fileHandle == NULL)
    m_lastError = errno;

  return (m_fileHandle != NULL);
}

// -------------------------------------------------------------------
void CNAStdioFile::Close()
{
  if (m_fileHandle != NULL)
    {
      fclose(m_fileHandle);
      m_fileHandle = NULL;
    }
}

// -------------------------------------------------------------------
// CNAStdioFile::ReadString
//
// return the length of the string read if the method processes
// the user's request successfully.
//
// returns -1 if the method encounters an error while processing
// the user's request.  For bulk read operations, the user then
// needs to invoke the method CNAStdioFile::GetLastErrorAsDword
// to get the Microsoft error code; for other kinds of operations,
// i.e., the stdio operations, the user needs to call the method
// CNAStdioFile::GetLastError to get the errno.h error number.
//
// For bulk read operation on Windows NT platforms, the user
// should always check to make sure that the return value
// is zero (0) before calling the CNAStdioFile::IsEOF method
// to find out whether an EOF condition has been encountered.
// -------------------------------------------------------------------
Int32 CNAStdioFile::ReadString(char *buffer, Lng32 bufferSize, NABoolean flipByteOrderNeeded)
{
  if (Initialize() == FALSE)
    return -1;
  ULng32 numRead = 0L;

  if ( m_bulkRead )
    {
      throw -1;
    }
  else
    {
      numRead = fread(buffer, sizeof(char), bufferSize, m_fileHandle);
      if (ferror(m_fileHandle))
        {
          m_lastError = errno;
          return -1;
        }
    }

  if ( flipByteOrderNeeded == TRUE )
     FlipByteOrder(buffer, bufferSize);

  return (Int32) numRead;
}

// -------------------------------------------------------------------
// CNAStdioFile::ReadBlock
//
// returns the length, in bytes, of the block of characters read
// if the method processes the user's request successfully. If
// the read block is empty, i.e., the end of the file has been
// reached, returns 0.
//
// returns -1 if the method encounters an error while processing
// the user's request.
//
// After the method CNAStdioFile::ReadBlock completes,
// besides checking the return value, the user can call one
// of the following methods to find out whether the method
// CNAStdioFile::ReadBlock call completed successfully or not:
//
// 1. For bulk read operations on Windows NT platforms, the users
//    should invoke the method CNAStdioFile::GetLastErrorAsDword
//    (instead of the method CNAStdioFile::GetLastError) to get
//    the Microsoft ::GetLastError error code.  Please note that
//    the error conditions ERROR_IO_PENDING and ERROR_HANDLE_EOF
//    are not considered as errors by this method; the method
//    CNAStdioFile::GetLastErrorAsDword will return the Microsoft
//    error code ERROR_SUCCESS (i.e. 0) for these cases.  Note
//    that the Microsoft error code ERROR_SUCCESS and the errno.h
//    ENOERR value (i.e. 0) can be used interchangeably because
//    they both equal to zero.  For simplicity, we can safely use
//    ENOERR in place of ERROR_SUCCESS, and it is safe to use the
//    following check for bulk read operations running on Windows NT
//    platforms: if (pStdioFile->GetLastError() != ENOERR) // error!
//
// 2. For other kinds of operations, i.e., the stdio operations,
//    the user needs to call the method CNAStdioFile::GetLastError
//    to get the errno.h error number.  If CNAStdioFile::ReadBlock
//    call was successful, the method CNAStdioFile::GetLastError
//    returns the ENOERR (i.e. 0) value.
// -------------------------------------------------------------------
Int32 CNAStdioFile::ReadBlock( char *buffer, Lng32 numBytes,
                             NABoolean flipByteOrderNeeded )
{
  if (Initialize() == FALSE)
    return( -1 );  // an error has occurred
  ULng32 numRead = 0L;

  if ( m_bulkRead )
    {
      throw -1;
    }
  else
    {
      numRead = fread(buffer, sizeof(char), numBytes, m_fileHandle);
      if (ferror(m_fileHandle))
        {
          m_lastError = errno;
          return( -1 );  // an error has occurred
        }
    }

  if ( flipByteOrderNeeded == TRUE )
     FlipByteOrder(buffer, numBytes);

  return (Int32)numRead;
}

// -------------------------------------------------------------------
void CNADataSource::FlipByteOrder(char* buffer, Lng32 bufferSize)
{
  ComASSERT (bufferSize % 2 == 0);

  // modified based on wc_swap_bytes(NAWchar *str, int length), in w:/common/wstr.h
  unsigned char* ptr;
  unsigned char temp;

  if ( buffer == 0 || bufferSize == 0 ) return;

  for (Lng32 i = 0; i < bufferSize; i += 2)
    {
      ptr = (unsigned char*)&buffer[i];
      temp = *ptr;
      *ptr = *(ptr+1);
      *(ptr+1) = temp;
    }
}

// -------------------------------------------------------------------
void CNADataSource::FlipByteOrder(char* buffer, TInt64 bufferSizeInBytes)
{
  ComASSERT (bufferSizeInBytes % 2 == 0);

  // modified based on wc_swap_bytes(NAWchar *str, int length), in w:/common/wstr.h
  unsigned char* ptr;
  unsigned char temp;

  if ( buffer == NULL || bufferSizeInBytes == 0 ) return;

  for (TInt64 i = 0; i < bufferSizeInBytes; i += 2)
    {
      ptr = (unsigned char*)&buffer[i];
      temp = *ptr;
      *ptr = *(ptr+1);
      *(ptr+1) = temp;
    }
}

// -------------------------------------------------------------------
// CNAStdioFile::CheckIOCompletion
//
// This method does nothing and returned 0 when running on NSK
// platforms.
//
// For Windows NT platforms, returns the number of characters
// read.  After this method is invoked, the user will need to
// call the method CNAStdioFile::GetLastErrorAsDword to find
// out whether an error has occurred.
// -------------------------------------------------------------------
ULng32 CNAStdioFile::CheckIOCompletion()
{
  ULng32 numRead = 0;


  return( numRead );
}

// -------------------------------------------------------------------
// CNAStdioFile::Read
//
// return the length, in bytes, of the data read if the method
// processes the user's request successfully.
//
// returns -1 if the method encounters an error while processing
// the user's request.  The user then needs to call the method
// CNAStdioFile::GetLastError to get the errno.h error number.
// -------------------------------------------------------------------
Int32 CNAStdioFile::Read(void *buffer, Lng32 bufferSize)
{
  if (Initialize() == FALSE)
    return -1;
  Int32 numRead = 0;

  numRead = (Int32) fread(buffer, sizeof(char), bufferSize, m_fileHandle);
  if (ferror(m_fileHandle))
  {
    m_lastError = errno;
    return -1;
  }

  return numRead;
}

// -------------------------------------------------------------------
NABoolean CNAStdioFile::IsEOF()
{
  if ( m_bulkRead )
  {
    throw -1;
  }
  else
  {
    if ( feof(m_fileHandle) )
    {
      return TRUE;
    }
    else
      return FALSE;
  }
}

// -------------------------------------------------------------------
// CNAStdioFile::ReadLine
//
// Returns TRUE if the method was able to read the input
// successfully.
//
// Returns FALSE if the method encountered an error while
// processing the user's request or it could not read any
// more input data due to the EOF condition.  The user
// then needs to call the CNAStdioFile::GetLastError method
// to find out whether an error has occurred or not, and
// the user needs to invoke the CNAStdioFile::IsEOF method
// to find out if an EOF condition has occurred.
// -------------------------------------------------------------------
NABoolean CNAStdioFile::ReadLine(char *buffer, Lng32 bufferSize)
{
  if (Initialize() == FALSE)
      return FALSE;
  Int32 numRead = 0;
  char *str = NULL;

  str = fgets(buffer, bufferSize, m_fileHandle);
  if (str == NULL)
  {
    if (IsEOF())
      buffer[0] = '\0';  // just to be really sure
    else
      m_lastError = errno;
    return FALSE;
  }

  // remove one trailing '\n' (newline) character if it exists
  numRead = (Int32)strlen(buffer);
  if (buffer[numRead - 1] == '\n')
    buffer[numRead - 1] = '\0';

  return TRUE;
}

// -------------------------------------------------------------------
// CNAStdioFile::WriteString
//
// return the length, in bytes, of the string written if the
// method processes the user's request successfully.
//
// returns -1 if the method encounters an error while processing
// the user's request.  The user then needs to call the method
// CNAStdioFile::GetLastError to get the error number.
// -------------------------------------------------------------------
Int32 CNAStdioFile::WriteString(const char *strLine)
{
  if (Initialize() == FALSE)
    return -1;
  Int32 strLength = (Int32) strlen(strLine);
  Int32 charSize = sizeof(char);
  Int32 numWrite = 0;

  numWrite = (Int32) fwrite(strLine, charSize, strLength, m_fileHandle);
  if (ferror(m_fileHandle))
  {
    m_lastError = errno;
    return -1;
  }

  return numWrite;
}

// -------------------------------------------------------------------
// CNAStdioFile::Write
//
// return the number of 8-bit characters written if the method
// processes the user's request successfully.
//
// returns -1 if the method encounters an error while processing
// the user's request.  The user then needs to call the method
// CNAStdioFile::GetLastError to get the error number.
// -------------------------------------------------------------------
Int32 CNAStdioFile::Write(const char *buffer, Lng32 bufferSize)
{
  if (Initialize() == FALSE)
    return -1;
  Int32 numWrite = 0;

  numWrite = (Int32)fwrite(buffer, sizeof(char), bufferSize, m_fileHandle);
  if (ferror(m_fileHandle))
  {
    m_lastError = errno;
    return -1;
  }

  return numWrite;
}

// -------------------------------------------------------------------
// CNAStdioFile::Write
//
// return the number of wide characters data written if the method
// processes the user's request successfully.
//
// returns -1 if the method encounters an error while processing
// the user's request.  The user then needs to call the method
// CNAStdioFile::GetLastError to get the error number.
//
// Note that bufferSize contains the number of wide characters
// (in parameter buffer) to be written.
// -------------------------------------------------------------------

Int32 CNAStdioFile::Write(const NAWchar *buffer, Lng32 bufferSize, NABoolean flipByteOrderNeeded)
{
  if (Initialize() == FALSE)
    return -1;
  NAWchar* buffer_write = NULL;

  if ( flipByteOrderNeeded == TRUE )
  {
    buffer_write = new NAWchar[bufferSize];
    if (na_wcsncpy (buffer_write, buffer, bufferSize) == 0)
    {
      // The copy operation failed - very unlikely
      m_lastError = EINVAL;
      delete [] buffer_write;
      return -1;
    }
    TInt64 bufferSizeInBytes = bufferSize * sizeof(NAWchar);
    FlipByteOrder((char*)buffer_write, bufferSizeInBytes);
  }
  else
    buffer_write = (NAWchar*)buffer;

  Int32 numWrite = 
    (Int32)fwrite(buffer_write, sizeof(NAWchar), bufferSize, m_fileHandle);
  if (ferror(m_fileHandle))
  {
    m_lastError = errno;
    if (buffer_write)
      delete [] buffer_write;
    return -1;
  }

  if ( flipByteOrderNeeded == TRUE )
    delete [] buffer_write;

  return numWrite;  // number of wide characters written
}

// -------------------------------------------------------------------
// CNAStdioFile::Flush
//
// returns ENOERR (0) if successful; otherwise, returns EOF (-1).
// The user can then call the method CNAStdioFile::GetLastError to
// get the error number.
// -------------------------------------------------------------------
Int32 CNAStdioFile::Flush()
{
  if (Initialize() == FALSE)
    return EOF;

  // fflush returns ENOERR if successful
  Int32 status = fflush (m_fileHandle);

  if (status != ENOERR)
    m_lastError = errno;

  return status;
}

//------------------------------------------------------------------------
CNADataSource::CNADataSource()
: m_lastError (ENOERR)
{
          m_bulkRead = FALSE;
          m_newlineStr[0] = '\n';
          m_newlineStr[1] = '\0';
          m_newlineStrWchar[0] = L'\n';
          m_newlineStrWchar[1] = L'\0';

}

//---------------------------------------------------------------------
CNADataSource::~CNADataSource()
{
}

// --------------------------------------------------------------------------
// Helper:  GetTimeString
//
// Get the current time in string format by calling ctime.
// If convertSpaceColonToDash is TRUE then all spaces and colons
// are converted to dashes.
//
// the caller needs to allocate CTIME_LENGTH character array prior to calling
// and send a pointer to this array as pTime
// --------------------------------------------------------------------------
void CNADataSource::GetTimeString( char *pTime, NABoolean convertSpaceColonToDash )
{
  // This code gets the time from the ctime OSS call.
  // ctime returns a 26 byte field in the form:
  //   Thu Feb 24 11:57:02 2005\n\0
  //   1234567890123456789012345 6
  //
  // This code will:
  //   remove the \n (newline) and replace with \0
  //   optionally replace all spaces and ':' in the string with dashes
  time_t t;
  time (&t);
  sprintf(pTime, "%s\n", ctime(&t));
  pTime[24] = '\0';

  // Replace the spaces with dashes.
  if (convertSpaceColonToDash)
  {
    Int32 pos = 0;
    while (pTime[pos] != '\0')	
    {
      if (pTime[pos] == ' ' || pTime[pos] == ':')
        pTime[pos] = '-';
      pos++;
    }
  }
}

