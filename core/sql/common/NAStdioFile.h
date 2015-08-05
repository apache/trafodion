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
#ifndef NASTDIOFILE_H
#define NASTDIOFILE_H

#include "Platform.h"
#include "stdio.h"
#include "fcntl.h"
#include "errno.h"
#include "NABoolean.h"
#include "ComASSERT.h"


#include "NAWinNT.h"

// Forward Declaration
class CNAProcess;
class CNAStdioFile;


// The CNADataSource class calls ctime to get time information.  This
// define sets up the length returned from ctime which includes the
// null terminator
#define CTIME_LENGTH 26

// The NSK platform defines a special OSS error code ENOERR which indicates
// no error.  The WINDOWS platform does not.  For consistency, add the 
// define for the windows platform.
//
// For bulk read operations on Windows NT platforms, we call the
// Microsoft function ::GetLastError to receive the Microsoft error
// code instead of using the C runtime static errno variable.  The
// ::GetLastError function returns the Microsoft pre-defined literal
// ERROR_SUCCESS when a previous bulk read operation was successful.
// Since both ENOERR and ERROR_SUCCESS equal to zero, for simplicity,
// we can safely use ENOERR in place of ERROR_SUCCESS for both bulk
// read and other stdio operations.
#define ENOERR 0

//--------------------------------------------------------------------------------

class CNADataSource{

public:
    enum EOpenMode { eRead, eReadBulk, eWrite, eAppend, eReadBinary, eWriteBinary, eReadWrite };

public:

    CNADataSource();
    virtual ~CNADataSource();

    virtual NABoolean Open(const char *sourceName, EOpenMode mode)=0;
    virtual void Close()=0;

    virtual Int32 ReadString(char *buffer, Lng32 bufferSize, NABoolean flipByteOrder = FALSE)=0;
    virtual Int32 Read(void *buffer, Lng32 bufferSize)=0;
    virtual NABoolean ReadLine(char *buffer, Lng32 bufferSize)=0;
    virtual Int32 ReadBlock(char *buffer, Lng32 bufferSize, NABoolean flipByteOrder = FALSE)=0;

    virtual Int32 WriteString(const char *strLine)=0;
    virtual Int32 Write(const char *buffer, Lng32 bufferSize)=0;

    virtual ULng32 CheckIOCompletion()=0;
    virtual NABoolean IsEOF()=0;
    void SetFileName(const char *fileName) { m_sourceName = fileName; }

    // double-byte typed string related routines
    NAWchar* GetNewlineStringInWchar()     { return m_newlineStrWchar; }
    virtual Int32 Write(const NAWchar *buffer, Lng32 bufferSize, NABoolean flipByteOrder = FALSE)=0;
    TCHAR * GetNewlineString() { return m_newlineStr; }

    NABoolean IsBulkReadOperation() const { return m_bulkRead; }

    // m_lastError contains the file system error (or ENOERR) encountered
    // during the last I/O call to OSS. The OSS subsystem stores the
    // file system error in the errno static variable defined in the
    // errno.h include file released as part of Guardian
    // (see $system.system.errnoh for the lastest version).
    //
    // Note that we purposely use the same name as the Microsoft
    // function ::GetLastError to prevent the methods in the derived
    // classes (e.g., class CNALogfile) from inadvertently calling the
    // Microsoft function GetLastError instead of the inherited
    // GetLastError (i.e., CNAStdioFile::GetLastError) method.  If
    // those methods really want to invoke the Microsoft function
    // ::GetLastError directly, they should specify the name
    // ::GetLastError explicitly.
    Int32 GetLastError() const { return m_lastError; }
    // Since both the Microsoft pre-defined literal ERROR_SUCCESS
    // and the literal ENOERR equal to zero, for simplicity,
    // we can safely use the (...GetLastError() != ENOERR) check
    // for both bulk read and other stdio operations.  For example:
    //
    //   if (this->GetLastError() != ENOERR)  // an error has occurred
    //   {
    //     if (IsBulkReadOperation())
    //     {
    //       #ifdef NA_WINNT
    //         if (GetLastErrorAsDword() == ERROR_INVALID_HANDLE)
    //           /* handle error ERROR_INVALID_HANDLE */ ;
    //         else
    //           /* more error handling code ... */ ;
    //       #else
    //         throw -1;
    //       #endif
    //     }
    //     else
    //     {
    //       if (this->GetLastError() == EBADF)
    //         /* handle error EBADF */ ;
    //       else
    //         /* more error handling code ... */ ;
    //     }
    //   }
    //
    // Note that GetLastError calls in the code above invoke the
    // CNAStdioFile::GetLastError method.

protected:
    // flip byer order for each double byte character in the buffer. Throw
    // an exception (-1) if bufferSize is not even.
    void FlipByteOrder(char* buffer, Lng32 bufferSize);
    void FlipByteOrder(char* buffer, TInt64 bufferSize);

    // Helper methods
    void GetTimeString( char *pTime, NABoolean convertSpaceColonToDash );

protected:
    NABoolean             m_bulkRead;
    TCHAR            m_newlineStr[3];
    NAWchar          m_newlineStrWchar[3];
    const char  *m_sourceName;  // Process Name or File Name
    Int32              m_lastError;

};

//--------------------------------------------------------------------------------
class CNAProcess : public CNADataSource{

    typedef CNADataSource inherited;

public:

        CNAProcess();
        ~CNAProcess();

        NABoolean Open(const char *sourceName, EOpenMode mode);
        void Close();

        Int32 ReadString(char *buffer, Lng32 bufferSize, NABoolean flipByteOrder = FALSE);
        Int32 Read(void *buffer, Lng32 bufferSize);
        NABoolean ReadLine(char *buffer, Lng32 bufferSize);
        Int32 ReadBlock(char *buffer, Lng32 bufferSize, NABoolean flipByteOrder = FALSE);

        Int32 WriteString(const char *strLine);
        Int32 Write(const char *buffer, Lng32 bufferSize);

        ULng32 CheckIOCompletion();
        NABoolean IsEOF();

        // double-byte typed string related routines
        Int32 Write(const NAWchar *buffer, Lng32 bufferSize, NABoolean flipByteOrder = FALSE);

protected:
        short                   m_fileHandle;

};

// Class CNAPopenProcess is only available for NSK...

//----------------------------------------------------------------------------------
class CNAStdioFile : public CNADataSource{

    typedef CNADataSource inherited;

public: // functions

    // constructor and destructor
    CNAStdioFile();
    ~CNAStdioFile();

    NABoolean Open(const char *sourceName, EOpenMode mode);
    void Close();

    Int32 ReadString(char *buffer, Lng32 bufferSize, NABoolean flipByteOrder = FALSE);
    Int32 Read(void *buffer, Lng32 bufferSize);
    Int32 ReadBlock(char *buffer, Lng32 bufferSize, NABoolean flipByteOrder = FALSE);
    NABoolean ReadLine(char *buffer, Lng32 bufferSize);

    Int32 WriteString(const char *strLine);
    Int32 Write(const char *buffer, Lng32 bufferSize);
    Int32 Flush ();

    ULng32 CheckIOCompletion();
    NABoolean IsEOF();
    NABoolean IsOpen();

    // double-byte typed string related routines
    Int32 Write(const NAWchar *buffer, Lng32 bufferSize, NABoolean flipByteOrder = FALSE);
    FILE *GetFileHandle() { return m_fileHandle; }

    NABoolean Initialize();
protected:
    FILE            *m_fileHandle;
 
};

//---------------------------------------------------------------------------------
inline NABoolean CNADataSource::Open(const char *fileName, CNADataSource::EOpenMode mode)
{
    // Added to take care of unresolved externals in MXCMP
    ComASSERT(FALSE);
    return 0;
}

inline Int32 CNADataSource::ReadBlock(char *buffer, Lng32 bufferSize, NABoolean flipByteOrder)
{
    // Added to take care of unresolved externals in MXCMP
    ComASSERT(FALSE);
    return 0;
}

inline ULng32 CNADataSource::CheckIOCompletion()
{
    // Added to take care of unresolved externals in MXCMP
    ComASSERT(FALSE);
    return 0;
}

inline void CNADataSource::Close()
{
    // Added to take care of unresolved externals in MXCMP
    ComASSERT(FALSE);
}

inline NABoolean CNADataSource::IsEOF()
{
    // Added to take care of unresolved externals in MXCMP
    ComASSERT(FALSE);
    return 0;
}

inline NABoolean CNADataSource::ReadLine(char *buffer, Lng32 bufferSize)
{
    // Added to take care of unresolved externals in MXCMP
    ComASSERT(FALSE);
    return 0;
}

inline Int32 CNADataSource::ReadString(char *buffer, Lng32 bufferSize, NABoolean flipByteOrder)
{
    // Added to take care of unresolved externals in MXCMP
    ComASSERT(FALSE);
    return 0;
}

inline Int32 CNADataSource::Read(void *buffer, Lng32 bufferSize)
{
    // Added to take care of unresolved externals in MXCMP
    ComASSERT(FALSE);
    return 0;
}

inline Int32 CNADataSource::WriteString(const char *strLine)
{
    // Added to take care of unresolved externals in MXCMP
    ComASSERT(FALSE);
    return 0;
}

inline Int32 CNADataSource::Write(const char *buffer, Lng32 bufferSize)
{
    // Added to take care of unresolved externals in MXCMP
    ComASSERT(FALSE);
    return 0;
}

inline Int32 CNADataSource::Write(const NAWchar *buffer, Lng32 bufferSize, NABoolean flipByteOrder)
{
    // Added to take care of unresolved externals in MXCMP
    ComASSERT(FALSE);
    return 0;
}

inline NABoolean CNAStdioFile::IsOpen( void )
{
  return (m_fileHandle) ? TRUE : FALSE;
}

inline NABoolean CNAStdioFile::Initialize( void )
{
  if (!m_fileHandle)
  {
    // Note that both the NSK literal ENOERR and the
    // Microsoft literal ERROR_SUCCESS equal to zero.
    if (m_lastError == ENOERR)
    {
      if (IsBulkReadOperation())
      {
        m_lastError = (Int32)ERROR_INVALID_HANDLE;
      }
      else
        m_lastError = EBADF;
    }
    return FALSE;
  }
  m_lastError = ENOERR;
  return TRUE;
}

#endif
