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
#ifndef SORTERROR_H
#define SORTERROR_H 

#include "Platform.h"
#include "NABasicObject.h"

enum SortErrorEnum {
       EMissErrTxt              =    10001 // internal error, missing error text
      ,EFromEOF                 =    10002 // from file eof
      ,EScrEOF                  =    10003 // scratch file eof
      ,EInterRuns               =    10004 // merge is disallowed for Quick Sort
      ,EUnexpectErr             =    10005 // Unexpected error value
      ,EPrevIOFail              =    10006 // Previous IO failed
      ,EScrWrite                =    10007 // Error writing to scr file writeBlock
      ,EInvAlgo                 =    10008 // invalid sort algorithm
      ,EExceedMaxRuns           =    10009 // exceeded maximum run number 1024
      ,EInvRunNumber            =    10010 // run number is inval
      ,EScrRead                 =    10011 // Error reading scr file readBlock
      ,EInvScrBlockNum          =    10012 // invalid scratch block number
      ,EScrNoDisks              =    10013 // no disks from generateDiskTable()
      ,EScrNoMemory             =    10014 // no memory
      ,EGetPHandle              =    10015 // PROCESSHANDLE_GETMINE_ failed     
      ,EDecomposePHandle        =    10016 // PROCESSHANDLE_DECOMPOSE_ failed     
      ,EDevInfoByLDev           =    10017 // DEVICE_GETINFOBYLDEV_ failed  
      ,EFindFnameStart          =    10018 // FILENAME_FINDSTART_ failed  
      ,EFindFnameNext           =    10019 // FILENAME_FINDNEXT_ failed  
      ,EFindFnameFinish         =    10020 // FILENAME_FINDFINISH_ failed 
      ,EGetInfoLstByName        =    10021 // FILE_GETINFOLISTBYNAME_ failed 
      ,EFileCreate              =    10022 // FILE_CREATE_ failed
      ,EFileOpen                =    10023 // FILE_OPEN_ failed
      ,ESetMode                 =    10024 // SETMODE failed
      ,EFileClose               =    10025 // FILE_CLOSE_ failed
      ,EAwaitioX                =    10026 // AWAITIOX failed
      ,EGetInfoLst              =    10027 // FILE_GETINFOLIST_ failed 
      ,EPosition                =    10028 // POSITION failed 
      ,EGetInfo                 =    10029 // FILE_GETINFO_ failed

      ,EPathNameLen             =    10030 // GetTempPath failed 
      ,EGetVolInfo              =    10031 // GetVolumeInformation failed 
      ,EDskFreeSpace            =    10032 // GetDiskFreeSpace failed 
      ,EGetTmpFName             =    10033 // GetTempFileName failed 
      ,ECreateFile              =    10034 // CreateFile failed 
      ,ECreateEvent             =    10035 // CreateEvent failed 
      ,ECloseHandle             =    10036 // CloseHandle failed 
      ,ESleepEx                 =    10037 // SleepEx failed 
      ,EWaitSingleObj           =    10038 // WaitForSingleObject failed 
      ,EResetEvent              =    10039 // ResetEvent failed  
      ,ESetFilePtr              =    10040 // SetFilePointer failed  
      ,EChkDir                  =    10041 // _chdir failed  
      ,ECreateDir               =    10042 // CreateDirectory failed
      ,EWriteFail               =    10043 // WriteFileEx failed
      ,EWaitMultObj             =    10045 // WaitForMultipleObjects failed
      ,EGetOverlappedResult     =    10046 // GetOverlappedResult failed
      
      ,EIONotComplete        =    10044 // Io completion failed
      ,EWrongLengthRead         =    10047 // wrong length read
      ,EThresholdReached        =    10048 // scratch space threshold was reached
      ,EScrFileNotFound         =    10049 // scratch file was not found
      ,EProcessGetPair          =    10050 // PROCESS_GETPAIRINFO_ failed
      ,EControl               =    10051  //CONTROL call failed
      ,EUnKnownOvType           =  10052  //Overflow type is unknown, SSD or HDD?
      ,EOvTypeNotConfigured     =  10053  //Overflow type is not configured.
      ,ETestError               =    8141 // Test of clients error handling


    };
  
class SortError : public NABasicObject {

 public:
  SortError();
  ~SortError();

  void initSortError();
  void setErrorInfo(short sorterr, short syserr = 0, short syserrdetail = 0, const char *errorMsg = NULL);
  short getSortError() const;
  short getSysError() const;
  short getErrorDetail() const;
  char* getSortErrorMsg() const;   
 private:
  short sortError_;
  short sysError_;               //actual value of the system error
  short sysErrorDetail_;           //some system error has detailed error
  char sortErrorMsg_[100];        //the sort class::method that is in error
};

#endif
