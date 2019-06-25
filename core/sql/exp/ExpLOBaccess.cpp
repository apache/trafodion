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
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         EXLOBaccess.cpp
 * Description:  class to store and retrieve LOB data.
 *               
 *               
 * Created:      10/29/2012
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <string>
#include <errno.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>




#include "hdfs.h"
#include "jni.h"


#include "ExpLOBstats.h"
#include "ExpLOBaccess.h"
#include "ExpLOBinterface.h"
#include "ExpLOBexternal.h"
#include "ExpLOB.h"
#include "NAVersionedObject.h"
#include "ComQueue.h"
#include "QRLogger.h"
#include "NAMemory.h"
#include "HdfsClient_JNI.h"
#include <seabed/ms.h>
#include <seabed/fserr.h>
#include <curl/curl.h>
#include <../../sqf/src/seabed/src/trans.h>
extern int ms_transid_get(bool pv_supp,
                          bool pv_trace,
                          MS_Mon_Transid_Type *pp_transid,
                          MS_Mon_Transseq_Type *pp_startid);
extern int ms_transid_reinstate(MS_Mon_Transid_Type, MS_Mon_Transseq_Type);

// short LobServerFNum;
SB_Phandle_Type serverPhandle;

ExLob::ExLob(NAHeap * heap, ExHdfsScanStats *hdfsAccessStats) :
    lobDataFile_(heap),
    storage_(Lob_Invalid_Storage),
    lobStorageLocation_(string()),
    lobGlobalHeap_(NULL),
    fs_(NULL),
    fdData_(NULL),
    openFlags_(0),
    stats_(hdfsAccessStats),
    lobTrace_(FALSE),
    useLibHdfs_(FALSE),
    hdfsClient_(NULL)    
{
  // nothing else to do
}

ExLob::~ExLob()
{
    
    if (fdData_) {
      hdfsCloseFile(fs_, fdData_);
      fdData_ = NULL;
    }
    if (hdfsClient_ != NULL) {
       HdfsClient::deleteInstance(hdfsClient_);
       hdfsClient_ = NULL;
    }
   
}

Ex_Lob_Error ExLob::initialize(const char *lobFile, Ex_Lob_Mode mode, 
                               char *lobStorageLocation, 
			       LobsStorage storage,
                               char *hdfsServer, Int64 hdfsPort,
                               char *lobLocation,
                               int bufferSize , short replication ,
                               int blockSize, Int64 lobMaxSize, ExLobGlobals *lobGlobals)
{
  int openFlags;
  struct timespec startTime;
  struct timespec endTime;
  Int64 secs, nsecs, totalnsecs;
 
  useLibHdfs_ = lobGlobals->useLibHdfs_;
  if (lobStorageLocation) 
    {
      if (lobStorageLocation_.empty()) 
	{
	  lobStorageLocation_ = string(lobStorageLocation);
	}

      if (lobFile)
        {
          lobDataFile_ = lobStorageLocation; 
          lobDataFile_ += "/";
          lobDataFile_ += lobFile;
        }      
    } 
  else 
    { 
      if (lobFile)
        lobDataFile_ = lobFile;
      
    }

  if (storage_ != Lob_Invalid_Storage) 
    {
      return LOB_INIT_ERROR;
    } else 
    {
      storage_ = storage;
    }

  hdfsServer_ = hdfsServer;
  hdfsPort_ = hdfsPort;
  // lobLocation_ = lobLocation;
  clock_gettime(CLOCK_MONOTONIC, &startTime);
  lobGlobalHeap_ = lobGlobals->getHeap();    
  HDFS_Client_RetCode hdfsClientRetcode;
  if (useLibHdfs_) { 
     if (lobGlobals->getHdfsFs() == NULL)     
        return LOB_HDFS_CONNECT_ERROR;
     else 
        fs_ = lobGlobals->getHdfsFs();
     hdfsClient_ = NULL;
  }
  else {
     hdfsClient_ = HdfsClient::newInstance(lobGlobalHeap_, stats_, hdfsClientRetcode);
     fs_ = NULL;
     if (hdfsClient_ == NULL)
        return LOB_HDFS_CONNECT_ERROR;
  }

  clock_gettime(CLOCK_MONOTONIC, &endTime);

  secs = endTime.tv_sec - startTime.tv_sec;
  nsecs = endTime.tv_nsec - startTime.tv_nsec;
  if (nsecs < 0) 
    {
      secs--;
      nsecs += NUM_NSECS_IN_SEC;
    }
  totalnsecs = (secs * NUM_NSECS_IN_SEC) + nsecs;
   
  if (! useLibHdfs_) {
     if (mode == EX_LOB_CREATE) {
        hdfsClientRetcode = hdfsClient_->hdfsCreate(lobDataFile_.data(), FALSE, TRUE, FALSE); 
        if (hdfsClientRetcode != HDFS_CLIENT_OK)
            return LOB_DATA_FILE_CREATE_ERROR;
     }
     hdfsClientRetcode = hdfsClient_->hdfsOpen(lobDataFile_.data(), FALSE);
     if (hdfsClientRetcode != HDFS_CLIENT_OK)
        return LOB_DATA_FILE_OPEN_ERROR;
     fdData_ = NULL;
  }
  else
  {
  if (mode == EX_LOB_CREATE) 
    { 
      // check if file is already created
      hdfsFileInfo *fInfo = hdfsGetPathInfo(fs_, lobDataFile_.data());
      if (fInfo != NULL) 
	{
	  hdfsFreeFileInfo(fInfo, 1);
	  return LOB_DATA_FILE_CREATE_ERROR;
	} 
      openFlags = O_WRONLY | O_CREAT;   
      fdData_ = hdfsOpenFile(fs_, lobDataFile_.data(), openFlags, bufferSize, replication, blockSize);
      if (!fdData_) 
	{
          return LOB_DATA_FILE_CREATE_ERROR;
	}
      hdfsCloseFile(fs_, fdData_);
      fdData_ = NULL;
    }
  }
  return LOB_OPER_OK;
    
}

Ex_Lob_Error ExLob::fetchCursor(char *handleIn, Int32 handleLenIn, Int64 &outOffset, Int64 &outSize,NABoolean &isEOD, Int64 transId) 
{
  Ex_Lob_Error err = LOB_OPER_OK;
  Int64 dummyParam;
  int cliErr=0;
  Int64 offset = 0;
  Int64 size = 0;
  lobCursors_it it = lobCursors_.find(string(handleIn, handleLenIn));
  char logBuf[4096];
  lobDebugInfo("In ExLob::fetchCursor",0,__LINE__,lobTrace_);
  char *blackBox = new(getLobGlobalHeap()) char[MAX_LOB_FILE_NAME_LEN+6];
  Int32 blackBoxLen = 0;

   if (it == lobCursors_.end())
   {
      return LOB_CURSOR_NOT_OPEN;                         
   }

   void *cliInterface = it->second.cliInterface_;
   
   
    cliErr = SQL_EXEC_LOBcliInterface(handleIn, handleLenIn, 
                                      blackBox, &blackBoxLen,		       
                                      (char *)&dummyParam, (Lng32 *)&dummyParam,
                                     LOB_CLI_SELECT_FETCH, LOB_CLI_ExecImmed,
                                     &offset, &size,
                                     &dummyParam, &dummyParam, 
				     &cliInterface,
                                      transId,lobTrace_);
    if (cliErr <0 ) 
      {
        str_sprintf(logBuf, "LOB_CLI_SELECT_FETCH Returned cli error  %d",cliErr);
        lobDebugInfo(logBuf,0,__LINE__,lobTrace_);

        err = LOB_DESC_READ_ERROR;
        return err;
      }
    if (cliErr == 100 )
      {
        isEOD= TRUE;
        
      }
    else
      {
        if (blackBox && blackBoxLen >0 )
          {
            // we have received the external data file name from the descriptor table
            // replace the contents of the lobDataFile with this name
            char temp[blackBoxLen+1];
            str_cpy_and_null(temp, blackBox, blackBoxLen, '\0', '0', TRUE);
            lobDataFile_ = temp;
            outOffset = offset;
            err=statSourceFile(temp,outSize);
            if (err != LOB_OPER_OK)
              return err;
          }
        else
          {
            outOffset = offset;
            outSize = size;
          }
        
      }

    str_sprintf(logBuf, " Returned after ::fetchCursor %ld,%ld",outOffset,outSize);
    lobDebugInfo(logBuf,0,__LINE__,lobTrace_);

    return err;
}



Ex_Lob_Error ExLob::getDesc(ExLobDesc &desc,char * handleIn, Int32 handleInLen, char *blackBox, Int32 *blackBoxLen, char *handleOut, Int32 &handleOutLen, Int64 transId) 
{
    Ex_Lob_Error err = LOB_OPER_OK; 
    NABoolean multipleChunks = FALSE;
    Int32 clierr = 0;
    Int64 size,offset,dummyParam = 0;
    char logBuf[4096];
    lobDebugInfo("In ExLob::getDesc",0,__LINE__,lobTrace_);
 

     clierr = SQL_EXEC_LOBcliInterface(handleIn, 
                                       handleInLen, 
                                       blackBox, blackBoxLen,
                                       handleOut, &handleOutLen,
                                       LOB_CLI_SELECT_UNIQUE, LOB_CLI_ExecImmed,
                                       &offset, &size,
                                       &dummyParam, &dummyParam, 
                                       0,
                                       transId,lobTrace_);
     
     if (clierr < 0) 
       return LOB_DESC_READ_ERROR;
    desc.setOffset(offset);
    desc.setSize(size);
   
    str_sprintf(logBuf,"After Cli LOB_CLI_SELECT_UNIQUE:descOffset:%ld, descSize: %ld",offset,size);
    lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
    return err;
}


Ex_Lob_Error ExLob::writeData(Int64 &offset, char *data, Int32 size, Int64 &operLen)
{ 
    Ex_Lob_Error err;
    HDFS_Client_RetCode hdfsClientRetcode = HDFS_CLIENT_OK;
    //Int64 writeOffset;

    if (! useLibHdfs_ ) {
      offset = hdfsClient_->hdfsWriteImmediate(data, size, hdfsClientRetcode,0,TRUE); 
       if (hdfsClientRetcode != HDFS_CLIENT_OK)
          return LOB_DATA_WRITE_ERROR;
       operLen = size;
       return LOB_OPER_OK;
    }
    lobDebugInfo("In ExLob::writeData",0,__LINE__,lobTrace_);
    if (!fdData_ || (openFlags_ != (O_WRONLY | O_APPEND))) // file is not open for write
    {
      // get file info
      hdfsFileInfo *fInfo = hdfsGetPathInfo(fs_, lobDataFile_.data());
      if (fInfo == NULL) {
         return LOB_DATA_FILE_NOT_FOUND_ERROR;
      }
      if (fdData_)
      {
        hdfsCloseFile(fs_, fdData_);
        fdData_=NULL;
      }
      openFlags_ =  O_WRONLY | O_APPEND; 
      fdData_ = hdfsOpenFile(fs_, lobDataFile_.data(), openFlags_, 0, 0, 0);
    }

     if ((operLen = hdfsWrite(fs_, fdData_, data, size)) == -1) {
       return LOB_DATA_WRITE_ERROR;
     }
     if (hdfsFlush(fs_, fdData_)) {
       return LOB_DATA_FLUSH_ERROR;
     }

    
    return LOB_OPER_OK;
}

Ex_Lob_Error ExLob::writeDataSimple(char *data, Int64 size, LobsSubOper subOperation, Int64 &operLen,
                                    int bufferSize , short replication , int blockSize)
{ 
    Ex_Lob_Error err;
    Int64 writeOffset = 0;
    if (! useLibHdfs_)
       return writeData(writeOffset,data, size, operLen);

    if (!fdData_ || (openFlags_ != (O_WRONLY | O_APPEND))) // file is not open for write
    {
      // get file info
      hdfsFileInfo *fInfo = hdfsGetPathInfo(fs_, lobDataFile_.data());
      if (fInfo == NULL) {
         return LOB_DATA_FILE_NOT_FOUND_ERROR;
      } else { 
         // file exists, check the size
         if (fInfo->mSize != 0) {
            hdfsFreeFileInfo(fInfo, 1);
            return LOB_DATA_FILE_NOT_EMPTY_ERROR;
         }
      }
      hdfsCloseFile(fs_, fdData_);
      fdData_=NULL;
      openFlags_ = O_WRONLY | O_APPEND ; 
      fdData_ = hdfsOpenFile(fs_, lobDataFile_.data(), openFlags_, bufferSize, replication, blockSize);
      if (!fdData_) {
         openFlags_ = -1;
         return LOB_DATA_FILE_OPEN_ERROR;
      }
    }
    if (hdfsWrite(fs_, fdData_, data, size) == -1) {
      return LOB_DATA_WRITE_ERROR;
    }
    if (hdfsFlush(fs_, fdData_)) {
      return LOB_DATA_FLUSH_ERROR;
    }


    operLen = size;

    return LOB_OPER_OK;
}

// numOfPartLevels: 0, if not partitioned
//                  N, number of partitioning cols
// failedModTS: timestamp value that caused the mismatch
Ex_Lob_Error ExLob::dataModCheck(
       char * dirPath, 
       Int64  inputModTS,
       Lng32  numOfPartLevels,
       ExLobGlobals *lobGlobals,
       Int64 &failedModTS,
       char  *failedLocBuf,
       Int32 *failedLocBufLen)
{
  if (inputModTS <= 0)
    return LOB_OPER_OK;

  Ex_Lob_Error result = LOB_OPER_OK;
  HDFS_Client_RetCode rc;
  Int64 currModTS;


  failedModTS = -1;

  // libhdfs returns a second-resolution timestamp,
  // get a millisecond-resolution timestamp via JNI
  rc = HdfsClient::getHiveTableMaxModificationTs(currModTS,
                                                dirPath,
                                                numOfPartLevels);
  // check for errors and timestamp mismatches
  if (rc != HDFS_CLIENT_OK || currModTS <= 0)
    {
      result = LOB_DATA_READ_ERROR;
    }
  else if (currModTS > inputModTS)
    {
      result = LOB_DATA_MOD_CHECK_ERROR;
      failedModTS = currModTS;
    }

  if (result != LOB_OPER_OK && failedLocBuf && failedLocBufLen)
    {
      // sorry, we lost the exact location for partitioned
      // files, user needs to search for him/herself
      Lng32 failedFileLen = strlen(dirPath);
      Lng32 copyLen = (failedFileLen > (*failedLocBufLen-1) 
                       ? (*failedLocBufLen-1) : failedFileLen);

      str_cpy_and_null(failedLocBuf, dirPath, copyLen,
                       '\0', ' ', TRUE);
      *failedLocBufLen = copyLen;
    }

  return result;
}

Ex_Lob_Error ExLob::emptyDirectory(char *dirPath,
                                   ExLobGlobals *lobGlobals)
{
  int retcode = 0;
  HDFS_Client_RetCode hdfsClientRetcode;
  if (! useLibHdfs_) {
     hdfsClientRetcode = HdfsClient::hdfsDeletePath(dirPath);
     if (hdfsClientRetcode != HDFS_CLIENT_OK)
        return LOB_DATA_FILE_DELETE_ERROR;
     return LOB_OPER_OK;
  }
      
  hdfsFileInfo *fileInfos = hdfsGetPathInfo(fs_, dirPath);
  if (fileInfos == NULL)
    {
      return LOB_DIR_NAME_ERROR;
    }
  
  Lng32 currNumFilesInDir = 0;
  fileInfos = hdfsListDirectory(fs_, dirPath, &currNumFilesInDir);
  if ((currNumFilesInDir > 0) && (fileInfos == NULL))
    {
      return LOB_DATA_FILE_NOT_FOUND_ERROR;
    }

  if ((currNumFilesInDir == 0) && (fileInfos == NULL)) // empty directory
    {
      return LOB_OPER_OK;
    }

  // delete all files in this directory
  NABoolean error = FALSE;
  for (Lng32 i = 0; i < currNumFilesInDir; i++)
    {
      hdfsFileInfo &fileInfo = fileInfos[i];
      if (fileInfo.mKind == kObjectKindFile)
        {
          retcode = hdfsDelete(fs_, fileInfo.mName, 0);
          if (retcode != 0)
            error = TRUE;
        }
    } // for

  // recursively delete all files in sub-dirs
  for (Lng32 i = 0; i < currNumFilesInDir; i++)
    {
      hdfsFileInfo &fileInfo = fileInfos[i];
      if (fileInfo.mKind == kObjectKindDirectory)
        {
          retcode = emptyDirectory(fileInfo.mName, lobGlobals);
          if (retcode != LOB_OPER_OK)
            error = TRUE;
        }
    } // for

  if (fileInfos)
    {
      hdfsFreeFileInfo(fileInfos, currNumFilesInDir);
    }

  if (error)
    return LOB_DATA_FILE_DELETE_ERROR;
  
  return LOB_OPER_OK;
}

struct MemoryStruct {
  char *memory;
  size_t size;
  NAHeap *heap;
};

// callback for writing from http file to memory while dynamically growing the size.
static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  
  mem->memory =  (char *)(mem->heap)->allocateMemory(mem->size + realsize + 1 );

  if(mem->memory == NULL) {
    /* out of memory! */
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}
//Call back for retrieving http file header info
static size_t header_throw_away(void *ptr, size_t size, size_t nmemb, void *data)
{
  /* we are not interested in the headers itself,
     so we only return the size we would have saved ... */
  return (size_t)(size * nmemb);
}


Ex_Lob_Error ExLob::statSourceFile(char *srcfile, Int64 &sourceEOF)
{
   char logBuf[4096];
   lobDebugInfo("In ExLob::statSourceFile",0,__LINE__,lobTrace_);
   // check if the source file is a hdfs file or from local file system.
  LobInputOutputFileType srcType = fileType(srcfile);
   HDFS_Client_RetCode hdfsClientRetcode;
   if (srcType == HDFS_FILE)
     {
       if (! useLibHdfs_) {
          sourceEOF = HdfsClient::hdfsSize(srcfile, hdfsClientRetcode);
          if (hdfsClientRetcode != HDFS_CLIENT_OK)
             return LOB_SOURCE_FILE_OPEN_ERROR;
          ex_assert(sourceEOF >= 0, "Offset is -1 possibly due to path being directory");
       }
       else {
       hdfsFile sourceFile = hdfsOpenFile(fs_,srcfile,O_RDONLY,0,0,0);   
       if (!sourceFile)	           
         return LOB_SOURCE_FILE_OPEN_ERROR;
         										 
       hdfsFileInfo *sourceFileInfo = hdfsGetPathInfo(fs_,srcfile);
       // get EOD from source hdfs file.
       if (sourceFileInfo)
	 sourceEOF = sourceFileInfo->mSize;
       else
	 return LOB_SOURCE_FILE_OPEN_ERROR;
       
       str_sprintf(logBuf,"Returning EOF of %ld for file %s", sourceEOF,srcfile);
       lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
       }
     }
   else if (srcType == LOCAL_FILE)
     {
       int openFlags = O_RDONLY;
       int fdSrcFile = open(srcfile, openFlags);
       if (fdSrcFile < 0) {
	 return LOB_SOURCE_FILE_OPEN_ERROR;
       }

       if (flock(fdSrcFile, LOCK_EX) == -1) {
	 return LOB_SOURCE_FILE_LOCK_ERROR;
       }

       struct stat statbuf;
       if (stat(srcfile, &statbuf) != 0) {
	 return LOB_SOURCE_FILE_STAT_ERROR;
       }
       sourceEOF = statbuf.st_size;
       flock(fdSrcFile, LOCK_UN);
       close(fdSrcFile);
       str_sprintf(logBuf,"Returning EOF of %ld for file %s", sourceEOF,srcfile);
       lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
       
     }
   else if (srcType == CURL_FILE)
     {
       // This is an http/ftp file. Use curl interface to determine size
       CURL *curl;
       CURLcode res;
       const time_t filetime = 0;
        double filesize = 0;
       curl = curl_easy_init();
       if(curl) {
	 curl_easy_setopt(curl, CURLOPT_URL, srcfile);

	 /* find file size from header */
	 /* No download if the file */
	 curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
	 /* Ask for filetime */
	 curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	 curl_easy_setopt(curl, CURLOPT_FILETIME, 1L);
	 /* No header output: TODO 14.1 http-style HEAD output for ftp */
	 curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION,header_throw_away);
	 curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
	 res = curl_easy_perform(curl);
	 if(CURLE_OK == res) {
           res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &filesize);
	   if (res == CURLE_OK)
	     {
	       Int64 temp_fs = 0;
	      
               temp_fs = filesize;
	       
	      sourceEOF = temp_fs;
	     }
	   else
	      return LOB_SOURCE_FILE_STAT_ERROR;
	 }
	 curl_easy_cleanup(curl);
       }

     }

   
   return LOB_OPER_OK;
}



Ex_Lob_Error ExLob::readSourceFile(char *srcfile, char *&fileData, Int64 &size, Int64 offset)
 {
   Ex_Lob_Error lobErr = LOB_OPER_OK;
   // check if the source file is a hdfs file or from local file system.
 
   LobInputOutputFileType srcType = fileType(srcfile);
   if (srcType == HDFS_FILE)
     {
       lobErr = readHdfsSourceFile(srcfile, fileData, size, offset);
     }
   else if (srcType == LOCAL_FILE)
     {
       lobErr = readLocalSourceFile(srcfile, fileData, size, offset);
     }
   else if(srcType == CURL_FILE)
     {
       lobErr = readExternalSourceFile((char *)srcfile, fileData, size, offset);
     }
   else
     return LOB_SOURCE_FILE_OPEN_ERROR;
  
  return lobErr;
 }
Ex_Lob_Error ExLob::readHdfsSourceFile(char *srcfile, char *&fileData, Int64 &size, Int64 offset)
 {
     char logBuf[4096];
     str_sprintf(logBuf,"Calling ::readHdfsSourceFile: %s Offset:%ld, Size: %ld",srcfile, offset,size);
     lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
     HDFS_Client_RetCode hdfsClientRetcode; 
     Int64 bytesRead;
     if (!useLibHdfs_) {
        HdfsClient *srcHdfsClient = HdfsClient::newInstance(getLobGlobalHeap(), NULL, hdfsClientRetcode);
        ex_assert(hdfsClientRetcode == HDFS_CLIENT_OK, "Internal error: HdfsClient::newInstance returned an error");
        hdfsClientRetcode  = srcHdfsClient->hdfsOpen(srcfile, FALSE);
        if (hdfsClientRetcode != HDFS_CLIENT_OK) {
           HdfsClient::deleteInstance(srcHdfsClient);
           return LOB_SOURCE_FILE_OPEN_ERROR;
        }
        fileData = (char *) (getLobGlobalHeap())->allocateMemory(size);
        if (fileData == (char *)-1) {
           HdfsClient::deleteInstance(srcHdfsClient);
           return LOB_SOURCE_DATA_ALLOC_ERROR;
        }
        bytesRead = srcHdfsClient->hdfsRead(offset, fileData, size, hdfsClientRetcode);
        if (hdfsClientRetcode != HDFS_CLIENT_OK) {
           HdfsClient::deleteInstance(srcHdfsClient);
           getLobGlobalHeap()->deallocateMemory(fileData);
           return LOB_SOURCE_FILE_READ_ERROR;
        }  
        size = bytesRead;
        // Memory growth/leak
        HdfsClient::deleteInstance(srcHdfsClient);
        return LOB_OPER_OK;
     }
     int openFlags = O_RDONLY;
     hdfsFile fdSrcFile = hdfsOpenFile(fs_,srcfile, openFlags,0,0,0);
     if (fdSrcFile == NULL) 
       return LOB_SOURCE_FILE_OPEN_ERROR;
         
     fileData = (char *) (getLobGlobalHeap())->allocateMemory(size);
     if (fileData == (char *)-1) {
       return LOB_SOURCE_DATA_ALLOC_ERROR;
     }

     if (hdfsPread(fs_,fdSrcFile, offset,fileData, size) == -1) {
       hdfsCloseFile(fs_,fdSrcFile);
       getLobGlobalHeap()->deallocateMemory(fileData);
       fileData = NULL;
       return LOB_SOURCE_FILE_READ_ERROR;
     }

     
     hdfsCloseFile(fs_,fdSrcFile);
     
     return LOB_OPER_OK;
 }

Ex_Lob_Error ExLob::readLocalSourceFile(char *srcfile, char *&fileData, Int64 &size, Int64 offset)
   {  
     char logBuf[4096];
     str_sprintf(logBuf,"Calling ::readLocalSourceFile: %s Offset:%ld, Size: %ld",srcfile, offset,size);
     lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
  
     int openFlags = O_RDONLY;
     int fdSrcFile = open(srcfile, openFlags);
     if (fdSrcFile < 0 ) {
       return LOB_SOURCE_FILE_OPEN_ERROR;
     }

     if (flock(fdSrcFile, LOCK_EX) == -1) {
       return LOB_SOURCE_FILE_LOCK_ERROR;
     }

     struct stat statbuf;
     if (stat(srcfile, &statbuf) != 0) {
       return LOB_SOURCE_FILE_STAT_ERROR;
     }

     fileData = (char *) (getLobGlobalHeap())->allocateMemory(size);
     if (fileData == (char *)-1) {
       return LOB_SOURCE_DATA_ALLOC_ERROR;
     }

     if (pread(fdSrcFile, fileData, size, offset) == -1) {
       close(fdSrcFile);
       getLobGlobalHeap()->deallocateMemory(fileData);
       fileData = NULL;
       return LOB_SOURCE_FILE_READ_ERROR;
     }

     flock(fdSrcFile, LOCK_UN);
     close(fdSrcFile);
     
     return LOB_OPER_OK ;
   }

Ex_Lob_Error ExLob::readExternalSourceFile(char *srcfile, char *&fileData, Int64 &size,Int64 offset)
{
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    chunk.memory = (char *) (getLobGlobalHeap())->allocateMemory(size);
    chunk.size = 0;    /* no data at this point */
    chunk.heap = getLobGlobalHeap();

   curl = curl_easy_init();
   if(curl) {
     
          curl_easy_setopt(curl, CURLOPT_URL, srcfile);

          /* send all data to this function  */
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
          /* we pass our 'chunk' struct to the callback function */
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
          res = curl_easy_perform(curl);
          curl_easy_cleanup(curl);
	  fileData = chunk.memory;
          
     }

  return LOB_OPER_OK;
}

Ex_Lob_Error ExLob::getLength(char *handleIn, Int32 handleInLen,Int64 &outLobLen,LobsSubOper so, Int64 transId)
{
  char logBuf[4096];
  Int32 cliErr = 0;
  Ex_Lob_Error err=LOB_OPER_OK; 
  char *blackBox = new(getLobGlobalHeap()) char[MAX_LOB_FILE_NAME_LEN+6];
  Int32 blackBoxLen = 0;
  Int64 dummy = 0;
  Int32 dummy2 = 0;
  if (so != Lob_External_File)
    {
      
      cliErr = SQL_EXEC_LOBcliInterface(handleIn, handleInLen,NULL,NULL,NULL,NULL,LOB_CLI_SELECT_LOBLENGTH,LOB_CLI_ExecImmed, 0,&outLobLen, 0, 0,0,transId,lobTrace_);
    
      if (cliErr < 0 ) {
        str_sprintf(logBuf,"CLI SELECT_LOBLENGTH returned error %d",cliErr);
        lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
  
        return LOB_DESC_READ_ERROR;
      }
    }
    else
      {
        //Get the lob external filename from the descriptor file and get the length of the file
        cliErr = SQL_EXEC_LOBcliInterface(handleIn, 
                                          handleInLen, 
                                          blackBox, &blackBoxLen,
                                          NULL, 0,
                                          LOB_CLI_SELECT_UNIQUE, LOB_CLI_ExecImmed,
                                          &dummy, &dummy,
                                          &dummy, &dummy, 
                                          0,
                                          transId,lobTrace_);
        if (cliErr < 0 ) {
          str_sprintf(logBuf,"CLI SELECT_LOBLENGTH returned error %d",cliErr);
          lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
  
          return LOB_DESC_READ_ERROR;
        }
        if (blackBox && blackBoxLen >0 )
          {
            // we have received the external data file name from the 
            // descriptor table
            
            char temp[blackBoxLen+1];
            str_cpy_and_null(temp, blackBox, blackBoxLen, '\0', '0', TRUE);
            
            
            err=statSourceFile(temp,outLobLen);
            if (err != LOB_OPER_OK)
              return err;
          }
       
      }
  return err;
}

Ex_Lob_Error ExLob::getOffset(char *handleIn, Int32 handleInLen,Int64 &outLobOffset,LobsSubOper so, Int64 transId)
{
  char logBuf[4096];
  Int32 cliErr = 0;
  Ex_Lob_Error err=LOB_OPER_OK; 

  Int64 dummy = 0;
  Int32 dummy2 = 0;
  if (so != Lob_External_File)
    {
      
      cliErr = SQL_EXEC_LOBcliInterface(handleIn, handleInLen,NULL,NULL,NULL,NULL,LOB_CLI_SELECT_LOBOFFSET,LOB_CLI_ExecImmed,&outLobOffset,0, 0, 0,0,transId,lobTrace_);
    
      if (cliErr < 0 ) {
        str_sprintf(logBuf,"CLI SELECT_LOBOFFSET returned error %d",cliErr);
        lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
  
        return LOB_DESC_READ_ERROR;
      }
    }
 
  return err;
}

Ex_Lob_Error ExLob::getFileName(char *handleIn, Int32 handleInLen, char *outFileName, Int32 &outFileLen , LobsSubOper so, Int64 transId)
{
  char logBuf[4096];
  Int32 cliErr = 0;
  Ex_Lob_Error err=LOB_OPER_OK; 
  Int64 dummy = 0;
  Int32 dummy2 = 0;
  if (so != Lob_External_File)
    {
      //Derive the filename from the LOB handle and return
      str_cpy_all(outFileName, (char *)lobDataFile_.data(),lobDataFile_.length());
    }
    else
      {
        //Get the lob external filename from the descriptor file 
        cliErr = SQL_EXEC_LOBcliInterface(handleIn, 
                                          handleInLen, 
                                          (char *)outFileName, &outFileLen,
                                          NULL, 0,
                                          LOB_CLI_SELECT_UNIQUE, LOB_CLI_ExecImmed,
                                          &dummy, &dummy,
                                          &dummy, &dummy, 
                                          0,
                                          transId,lobTrace_);
        if (cliErr < 0 ) {
          str_sprintf(logBuf,"CLI SELECT_FILENAME returned error %d",cliErr);
          lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
  
          return LOB_DESC_READ_ERROR;
        }
             
      }
  return err;
}


Ex_Lob_Error ExLob::writeDesc(Int64 &sourceLen, char *source, LobsSubOper subOper, Int64 &descNumOut, Int64 &dataOffset, Int64 lobMaxSize, char * handleIn, Int32 handleInLen, char *blackBox, Int32 *blackBoxLen, char *handleOut, Int32 &handleOutLen, Int64 xnId, void *lobGlobals)
{
  Ex_Lob_Error err=LOB_OPER_OK; 
    
    Int64 outDescPartnKey = 0;
    Int64 outDescSyskey = 0;
    Int32 clierr = 0;
    char logBuf[4096];
  
    lobDebugInfo("In ExLob::writeDesc",0,__LINE__,lobTrace_);
    //if external lob input, make sure it resides in hdfs
    if (subOper == Lob_External_File)
      {
       LobInputOutputFileType srcFileType = fileType(source);
       if (srcFileType != HDFS_FILE)
         return  LOB_SOURCE_FILE_READ_ERROR;
       //Check if external file exists
       Int64 sourceEOD = 0;
       if (statSourceFile(source, sourceEOD) != LOB_OPER_OK)
         return  LOB_SOURCE_FILE_READ_ERROR;
      }
   
    if (sourceLen < 0 || sourceLen > lobMaxSize)
      return LOB_MAX_LIMIT_ERROR; //exceeded the size of the max lob size
        
   
    
    if (err != LOB_OPER_OK)
      return err;
    lobDebugInfo("Calling cli LOB_CLI_INSERT",0,__LINE__,lobTrace_);
   
    clierr = SQL_EXEC_LOBcliInterface(handleIn, 
                                     handleInLen, 
				     blackBox,
                                     blackBoxLen,
                                     handleOut, &handleOutLen,
                                     LOB_CLI_INSERT, LOB_CLI_ExecImmed,
                                     &dataOffset, &sourceLen,
                                     &outDescPartnKey, &outDescSyskey, 
				     0,
				     xnId,lobTrace_);
    if (clierr < 0 ) {
      str_sprintf(logBuf,"CLI LOB_CLI_INSERT returned error %d",clierr);
      lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
  
      return LOB_DESC_WRITE_ERROR;
    }
    return err;
}

#if 0
Ex_Lob_Error ExLob::insertDesc(Int64 offset, Int64 size,  char *handleIn, Int32 handleInLen,  char *handleOut, Int32 &handleOutLen, char *blackBox, Int32 blackBoxLen,Int64 xnId, void *lobGlobals) 
{
  
   Lng32 clierr;
   Int64 dummyParam;
   Int64 outDescSyskey = 0;
   Int64 outDescPartnKey = 0;
   handleOutLen = 0;
   Int32 chunkNum = 1;
 
   NABoolean foundUnused = FALSE;
   char logBuf[4096];
   lobDebugInfo("In ExLob::InsertDesc",0,__LINE__,lobTrace_);
   str_sprintf(logBuf,"Calling Cli LOB_CLI_INSERT: Offset:%ld, Size: %ld",
               offset,size);
   lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
   clierr = SQL_EXEC_LOBcliInterface(handleIn, 
                                     handleInLen, 
				     NULL, &chunkNum,
                                     handleOut, &handleOutLen,
                                     LOB_CLI_INSERT, LOB_CLI_ExecImmed,
                                     &offset, &size,
                                     &outDescPartnKey, &outDescSyskey, 
				     0,
				     xnId,lobTrace_);
   str_sprintf(logBuf,"After LOB_CLI_INSERT: ChunkNum:%d OutSyskey:%ld",
               chunkNum,outDescSyskey);
   lobDebugInfo(logBuf,0,__LINE__,lobTrace_);
   lobDebugInfo("Leaving ExLob::InsertDesc",0,__LINE__,lobTrace_);
   if (clierr < 0 ) {
     lobDebugInfo("LOB_CLI_INSERT cli call returned error :",clierr,__LINE__,TRUE);
     return LOB_DESC_WRITE_ERROR;
    }
   return LOB_OPER_OK;
}
#endif

Ex_Lob_Error ExLob::writeLobData(char *source, Int64 sourceLen, LobsSubOper subOperation, Int64 &tgtOffset,Int64 &operLen, Int64 lobMaxChunkMemSize)
{
  Ex_Lob_Error err=LOB_OPER_OK; 
  char logBuf[4096];
  lobDebugInfo("In ExLob::writeLobData",0,__LINE__,lobTrace_);
  HDFS_Client_RetCode hdfsClientRetcode = HDFS_CLIENT_OK;
    

  char *inputAddr = source;
  Int64 readOffset = 0;
  Int64 inputSize = sourceLen;
   
  if (subOperation == Lob_External_File)
    return LOB_OPER_OK;
    
    
  err = writeData(tgtOffset, inputAddr, inputSize, operLen);
  if (err != LOB_OPER_OK)
    {
      str_sprintf(logBuf,"::writeData returned error .writeOffset:%ld, operLen %ld ", tgtOffset,operLen);
      lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
      //handle errors that happen in one of the chunks.
      return err;
    }
  if (subOperation == Lob_File) 
    getLobGlobalHeap()->deallocateMemory(inputAddr);
               
  lobDebugInfo("Leaving ExLob::writeLobData",0,__LINE__,lobTrace_);	
  if (useLibHdfs_) {
    hdfsCloseFile(fs_, fdData_);
    fdData_=NULL;
  }
  return err;
}

Ex_Lob_Error ExLob::readToMem(char *memAddr, Int64 size,  Int64 &operLen,char * handleIn, Int32 handleInLen, char *blackBox, Int32 blackBoxLen, char * handleOut, Int32 &handleOutLen, Int64 transId)
{
   Ex_Lob_Error err = LOB_OPER_OK; 
   NABoolean multipleChunks = FALSE;
  
   int cliErr;

   operLen = 0;
   ExLobDesc desc;
   Int64 sizeToRead = 0;
   char logBuf[4096];
   lobDebugInfo("In ExLob::readToMem",0,__LINE__,lobTrace_);
   
   err = getDesc(desc,handleIn,handleInLen,blackBox, &blackBoxLen,handleOut,handleOutLen,transId);
   if (err != LOB_OPER_OK)
     {	   
       return err;
     }
   sizeToRead = MINOF(size,desc.getSize());
   if (blackBox && blackBoxLen >0 )
     {
       
       // we have received the external data file name from the descriptor table
       // replace the contents of the lobDataFile with this name 
       char temp[blackBoxLen+1];
       str_cpy_and_null(temp, blackBox, blackBoxLen, '\0', '0', TRUE);
       lobDataFile_ = temp;
     }
   if (blackBoxLen == -1)
     {
       lobDebugInfo("Reading multiple chunks",0,__LINE__,lobTrace_);
       sizeToRead = size;
       multipleChunks = TRUE;
     }
   str_sprintf(logBuf,"sizeToRead:%ld, desc.size :%d", sizeToRead, desc.getSize());
   lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
   err = readDataToMem(memAddr, desc.getOffset(),sizeToRead, operLen, handleIn,handleInLen, multipleChunks,transId);

   return err;
}
LobInputOutputFileType ExLob::fileType(char *ioFileName)
{
  std::string fileTgt(ioFileName);
  std:string hdfsDirStr("hdfs://");
  std::string httpStr("http://");
  std:: string fileDirStr("file://");
  short found = 0;
  LobInputOutputFileType  filetype;
  bool isHdfs = FALSE;
  bool isLocal = FALSE;
  bool isExternal = FALSE;
  bool isHdfsDir = FALSE;
  bool isFileDir = FALSE;
  if (((found = fileTgt.find(hdfsDirStr)) != std::string::npos) && (found == 0))
    {
      return HDFS_FILE;
      
    }      
    else if (((found = fileTgt.find(fileDirStr)) != std::string::npos) &&(found == 0))
      return LOCAL_FILE;
      
     
    else if (((found = fileTgt.find(httpStr)) != std::string::npos) && (found == 0))
      return CURL_FILE;
    
    else
      return LOCAL_FILE;
}
Ex_Lob_Error ExLob::readToFile(char *tgtFileName, Int64 tgtLength, Int64 &operLen, Int64 lobMaxChunkMemLen, Int32 fileflags,char *handleIn,Int32 handleInLen, char *blackBox, Int32 blackBoxLen, char * handleOut, Int32 &handleOutLen, Int64 transId)
{
  char logBuf[4096];
  lobDebugInfo("In ExLob::readToFile",0,__LINE__,lobTrace_);
  Ex_Lob_Error err = LOB_OPER_OK; 
  Int64 srcOffset = 0;
  Int64 srcLength = 0;
  LobInputOutputFileType tgtType = fileType(tgtFileName);
  ExLobDesc desc;
  NABoolean multipleChunks = FALSE;
  err = getDesc(desc,handleIn,handleInLen,blackBox, &blackBoxLen,handleOut,handleOutLen,transId);
  if (err != LOB_OPER_OK)
    return err;
  if (blackBoxLen == -1)  // mxlobsrvr returned -1 indicating multiple chunks for this particular lob handle
    {
      lobDebugInfo("Reading multiple chunks",0,__LINE__,lobTrace_);
      multipleChunks = TRUE;
      //the data retrieval in chunks is handled in readDataToMem.
    }
  else if (tgtLength <=0 )
    {
      return LOB_SOURCE_FILE_READ_ERROR;
    }
  else
    {
      srcOffset = desc.getOffset();
      
    }
  if (blackBox)
    {
      // we have received the external data file name from the descriptor table
      // replace the contents of the lobDataFile with this name 
      char temp[blackBoxLen+1];
      str_cpy_and_null(temp, blackBox, blackBoxLen, '\0', '0', TRUE);
      lobDataFile_ = temp;
    }
  if (tgtType == HDFS_FILE)
    {
      err = readDataToHdfsFile(tgtFileName,  srcOffset , tgtLength,operLen, lobMaxChunkMemLen, fileflags,handleIn,handleInLen,multipleChunks,transId);
      if (err != LOB_OPER_OK)
	return err;
    }
  else if(tgtType == CURL_FILE)
    {
      err = readDataToExternalFile(tgtFileName, srcOffset, tgtLength, operLen, lobMaxChunkMemLen, fileflags,handleIn, handleInLen,multipleChunks,transId);
      if (err != LOB_OPER_OK)
	return err;
    }
  else if (tgtType == LOCAL_FILE)
    { 
      err = readDataToLocalFile(tgtFileName,srcOffset, tgtLength,operLen, lobMaxChunkMemLen, fileflags,handleIn,handleInLen,multipleChunks,transId);
      if (err != LOB_OPER_OK)
	return err;
    }
  else
    return LOB_TARGET_FILE_OPEN_ERROR; //unknown format

  return LOB_OPER_OK;
}


Ex_Lob_Error ExLob::insertSelect(ExLob *srcLobPtr,
                                 char *handleIn,Int32 handleInLen,char *source,
                                 Int64 sourceLen, Int64 &operLen,
                                 Int64 lobMaxSize, Int64 lobMaxChunkMemLen,
                                 Int64 lobGCLimit, char *blackBox, 
                                 Int32 blackBoxLen,char *handleOut, 
                                 Int32 &handleOutLen,LobsSubOper so, Int64 xnId,void *lobGlobals)
{
  Ex_Lob_Error err = LOB_OPER_OK;
  Int32 cliRC;
  Int16 flags;
  Lng32  lobNum;
  Int64 descNumOut = 0;
  Int64 descNumIn = 0;
  Int64 dataOffset = 0;
  Int64 descSyskey = 0;
  Int32 lobType = 0;
  Int64 uid, inDescSyskey, descPartnKey;
  short schNameLen;
  char schName[1024];
  Int64 inputLobDataLen = 0;
  Int64 retOperLen = 0; 
  Int64 dummy = 0;
  char logBuf[4096];
  char sourceHandle[LOB_HANDLE_LEN]  = {};
 
  str_cpy_all(sourceHandle, source, sourceLen);

  if (so == Lob_External_Lob)
    {
      ExLobDesc desc;
      char extFileName[MAX_LOB_FILE_NAME_LEN+6];
      Int64 extFileNameLen = 0;
      // retrieve the external file name from the source log descriptor
      err = getDesc(desc,sourceHandle,sourceLen,blackBox,&blackBoxLen,handleOut,handleOutLen,xnId);
      if (err != LOB_OPER_OK)
        return LOB_DESC_READ_ERROR;
      // we have received the external data file name from the descriptor table
      // replace the contents of the lobDataFile with this name      
      str_cpy_all(extFileName, blackBox, blackBoxLen);
      extFileName[blackBoxLen]= '\0';

      extFileNameLen = blackBoxLen;
      // Now insert this into the target lob descriptor
      
      err = writeDesc(extFileNameLen, extFileName, Lob_External_File, descNumOut, 
                      retOperLen, lobMaxSize, 
                      handleIn,handleInLen,(char *)blackBox, &blackBoxLen,
                      handleOut,handleOutLen,xnId,lobGlobals);
 
      if (err != LOB_OPER_OK)
        return err;
      return err;
    }
  // First retrieve length of the lob pointed to by source (input handle) 
  cliRC = SQL_EXEC_LOBcliInterface(sourceHandle, sourceLen,
                                   NULL,NULL,NULL,NULL,
                                   LOB_CLI_SELECT_LOBLENGTH,LOB_CLI_ExecImmed,
                                   0,&inputLobDataLen, &dummy, &dummy,0,0,FALSE);
  if (cliRC < 0)
    {
      str_sprintf(logBuf,"cli LOB_CLI_SElECT_LOBLENGTH returned :%d", cliRC);
      lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);   
      return LOB_DESC_READ_ERROR;
    }
      		   
  // Allocate memory to hold the lob data from the input lob handle
  char *inputLobData = (char *)(getLobGlobalHeap()->allocateMemory(inputLobDataLen));
  // retrieve the section/sections from the lob pointed to by the input handle
  // into memory
  err = srcLobPtr->readToMem(inputLobData, inputLobDataLen,retOperLen, 
                             sourceHandle,sourceLen,
                             blackBox, blackBoxLen,
                             handleOut, handleOutLen,xnId);
  if (err != LOB_OPER_OK)
    return err;

  
  // write the lob data into the target lob 
   err =  insertData(inputLobData, inputLobDataLen, Lob_Memory, dataOffset, retOperLen, lobMaxSize,lobMaxChunkMemLen,lobGCLimit,handleIn,handleInLen,(char *)blackBox, blackBoxLen,handleOut,handleOutLen,lobGlobals);
  
  if (err != LOB_OPER_OK)
    return err;

  err = writeDesc(inputLobDataLen, inputLobData, Lob_Memory, descNumOut, dataOffset, lobMaxSize,handleIn,handleInLen,(char *)blackBox, &blackBoxLen,handleOut,handleOutLen,xnId,lobGlobals);
 
  if (err != LOB_OPER_OK)
    return err;
  if (handleOutLen > 0)
    {
      ExpLOBoper::extractFromLOBhandle(NULL, &lobType, NULL, NULL, &descSyskey,
				       NULL, NULL, NULL, handleOut);
      
      ExpLOBoper::updLOBhandle(descSyskey, 0, handleIn); 
    }

 
  return LOB_OPER_OK;
}

Ex_Lob_Error ExLob::append(char *data, Int64 size, LobsSubOper so, Int64 headDescNum, Int64 &operLen, Int64 lobMaxSize,Int64 lobMaxChunkMemSize, Int64 lobGCLimit, char *handleIn, Int32 handleInLen,  char * handleOut, Int32 &handleOutLen,Int64 xnId,void *lobGlobals)
{
    Ex_Lob_Error err = LOB_OPER_OK;
    Int64 dummyParam;
    Int64 dataOffset=0;
    Int64 sourceLen = size;
    Int32 clierr = 0;
    Int32 chunkNum = 0;
    Int64 outDescPartnKey, outDescSyskey = 0;
    char logBuf[4096];
    char *blackBox = NULL;
    Int32 blackBoxLen = 0;
    Int64 chunkMemSize = 0;
    char *source_filename = NULL;
    Int64 sourceFileReadOffset = 0;
    char *retBuf = 0;
    Int64 retReadLen = 0;
    Int64 inputSize = 0;
    if (so ==Lob_External_File)
      {
        blackBox = data;
        blackBoxLen = (Int32)size;
      }
    lobDebugInfo("In ExLob::append",0,__LINE__,lobTrace_);

    if ((so == Lob_File))
      {
	err = statSourceFile(data, sourceLen); 
	if (err != LOB_OPER_OK)
	  return err;
      }
   
      
    if (sourceLen <= 0 || sourceLen > lobMaxSize)
      {
        return LOB_MAX_LIMIT_ERROR; //exceeded the size of the max lob size
      }

    if (lobGCLimit !=-1)
     err = checkAndDoGC(size,dataOffset,lobMaxSize,lobMaxChunkMemSize,handleIn,
                      handleInLen,lobGCLimit,lobGlobals);
   if (err != LOB_OPER_OK){
     lobDebugInfo("checkAndDoGC returned error",0,__LINE__,lobTrace_);
     return err;
   }
   char *inputAddr = data;
   if (so == Lob_File)
     {
       source_filename = data;
     }
   
   inputSize = sourceLen;
   while (inputSize >0)
     {
       chunkMemSize = MINOF(lobMaxChunkMemSize, inputSize);
       if (so == Lob_File)
         {
           //read a chunk of the file input data
           err = readSourceFile(source_filename,retBuf,chunkMemSize, sourceFileReadOffset);
             
           if (err != LOB_OPER_OK)              
             return err;
                
           inputAddr = retBuf;
         }
       str_sprintf(logBuf,"Calling writeLobData: inputAddr: %ld, InputSize%ld, tgtOffset:%ld",(long)inputAddr,sourceLen,dataOffset);
       err = writeLobData(inputAddr, chunkMemSize,so,dataOffset,operLen,lobMaxChunkMemSize);
       if (err != LOB_OPER_OK)
         {
           lobDebugInfo("writeLobData returned error",0,__LINE__,lobTrace_);
           return err;
         }
       lobDebugInfo("Calling cli LOB_CLI_INSERT_APPEND",0,__LINE__,lobTrace_);
       clierr = SQL_EXEC_LOBcliInterface(handleIn, handleInLen, 
                                         blackBox, &blackBoxLen,
                                         handleOut, &handleOutLen,
                                         LOB_CLI_INSERT_APPEND, LOB_CLI_ExecImmed,
                                         &dataOffset, &chunkMemSize,
                                         &outDescPartnKey, &outDescSyskey, 
                                         0,
                                         xnId,lobTrace_);
    
    
       if (clierr < 0 || clierr == 100) 
         { // some error or EOD.
           str_sprintf(logBuf,"cli LOB_CLI_INSERT_APPEND returned :%d", clierr);
           lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
           return LOB_DESC_APPEND_ERROR;
         }
       inputSize -= chunkMemSize;
       if (so ==Lob_File)
         sourceFileReadOffset +=chunkMemSize;       
       inputAddr += chunkMemSize;
     }

   
    return LOB_OPER_OK;
}
Ex_Lob_Error ExLob::insertData(char *data, Int64 size, LobsSubOper so,Int64 &tgtOffset, Int64 &operLen, Int64 lobMaxSize, Int64 lobMaxChunkMemSize, Int64 lobGCLimit,char * handleIn, Int32 handleInLen, char *blackBox, Int32 blackBoxLen, char * handleOut, Int32 &handleOutLen, void *lobGlobals)
{
  Ex_Lob_Error err=LOB_OPER_OK; 
  ExLobDesc desc;
  int clierr = 0;
  operLen = 0;
  char logBuf[4096];
  lobDebugInfo("In ExLob::InsertData",0,__LINE__,lobTrace_);
  str_sprintf(logBuf,"data:%ld, size %ld, lobMaxSize:%ld, lobMaxChunkMemSize:%ld", (long)data, size,lobMaxSize,lobMaxChunkMemSize);
  
  if (lobGCLimit !=-1)
    err = checkAndDoGC(size,tgtOffset,lobMaxSize,lobMaxChunkMemSize,handleIn,
                       handleInLen,lobGCLimit,lobGlobals);
  if (err != LOB_OPER_OK)
    {
      lobDebugInfo("checkAndDoGC returned error",0,__LINE__,lobTrace_);
      return err;
      if ((data == NULL))  
        return LOB_SOURCE_DATA_ERROR;
    }

  char *inputAddr = data;
   
    
  str_sprintf(logBuf,"Calling writeLobData: inputAddr: %ld, InputSize%ld, tgtOffset:%ld",(long)inputAddr,size,tgtOffset);

  lobDebugInfo(logBuf,0,__LINE__,lobTrace_);
  err = writeLobData(inputAddr, size,so, tgtOffset, 
                     operLen,lobMaxChunkMemSize);
  if (err != LOB_OPER_OK){
    lobDebugInfo("writeLobData returned error",0,__LINE__,lobTrace_);
    return err;
  }
  return LOB_OPER_OK;
}
Ex_Lob_Error ExLob::update(char *data, Int64 size, LobsSubOper so,Int64 headDescNum, Int64 &operLen, Int64 lobMaxSize, Int64 lobMaxChunkMemSize, Int64 lobGCLimit, char *handleIn, Int32 handleInLen,  char *handleOut, Int32 &handleOutLen, Int64 xnId,void *lobGlobals)
{
  Ex_Lob_Error err = LOB_OPER_OK;
  Int64 dummyParam;
  Int64 dataOffset = 0;
  Int64 sourceLen = size;
  Int32 clierr = 0;
  Int64 outDescPartnKey,outDescSyskey = 0;
  Int32 chunkNum = 0;
  char logBuf[4096];
  char *blackBox = NULL;
  Int32 blackBoxLen = 0;
  Int64 chunkMemSize = 0;
  char *source_filename = NULL;
  Int64 sourceFileReadOffset = 0;
  char *retBuf = 0;
  Int64 retReadLen = 0;
  Int64 inputSize = 0;
  if (so == Lob_External_File)
    {
      blackBox = data;
      blackBoxLen = (Int32)size;
    }
  lobDebugInfo("In ExLob::update",0,__LINE__,lobTrace_);
  if ((so == Lob_File) || (so == Lob_External_File))
    {
      str_sprintf(logBuf,"Calling statSourceFile: source:%s, sourceLen: %ld",
                  data,sourceLen);
      lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
      err = statSourceFile(data, sourceLen); 
      if (err != LOB_OPER_OK)
        return err;
    }
  if(so != Lob_External_File)
    {
      if (sourceLen < 0 || sourceLen > lobMaxSize)
        {
          return LOB_MAX_LIMIT_ERROR; //exceeded the size of the max lob size
        }
        
      if (lobGCLimit !=-1)
        err = checkAndDoGC(size,dataOffset,lobMaxSize,lobMaxChunkMemSize,handleIn,
                           handleInLen,lobGCLimit,lobGlobals);
      if (err != LOB_OPER_OK)
        {
          lobDebugInfo("checkAndDoGC returned error",0,__LINE__,lobTrace_);
          return err;
        }
    }
      char *inputAddr = data;
      if (so == Lob_File)
        {
          source_filename = data;
        }
     
      inputSize = sourceLen;
      if(inputSize == 0) //empty_blob
        {
          // Update the current contents with an empty lob descriptor
          dataOffset=0; chunkMemSize=0;
          clierr = SQL_EXEC_LOBcliInterface(handleIn, 
                                            handleInLen, 
                                            blackBox, &blackBoxLen,
                                            handleOut, &handleOutLen,
                                            LOB_CLI_UPDATE_UNIQUE, LOB_CLI_ExecImmed,
                                            &dataOffset, &chunkMemSize,
                                            &outDescPartnKey, &outDescSyskey, 
                                            0,
                                            xnId,lobTrace_);
    
          if (clierr < 0 || clierr == 100)       
            return LOB_DESC_UPDATE_ERROR;
        }
     
      chunkMemSize = MINOF(lobMaxChunkMemSize, inputSize);
      if (so == Lob_File)
        {
          //read a chunk of the file input data
          err = readSourceFile(source_filename,retBuf,chunkMemSize, sourceFileReadOffset);
             
          if (err != LOB_OPER_OK)              
            return err;
                
          inputAddr = retBuf;
        }
      str_sprintf(logBuf,"Calling writeLobData.sourceLen:%ld, dataOffset:%ld",sourceLen,dataOffset);
      lobDebugInfo(logBuf,0,__LINE__,lobTrace_);
      if(sourceLen !=0)  
        {     
          err = writeLobData(inputAddr, chunkMemSize,so,dataOffset,operLen,lobMaxChunkMemSize);
          str_sprintf(logBuf,"writeLobData returned. operLen:%ld",operLen);
          lobDebugInfo(logBuf,0,__LINE__,lobTrace_);
          if (err != LOB_OPER_OK)
            {
              lobDebugInfo("writeLobData Failed",0,__LINE__,lobTrace_); 
              return err;
            }
        }
      lobDebugInfo("Calling CLI LOB_CLI_UPDATE_UNIQUE",0,__LINE__,lobTrace_);
      clierr = SQL_EXEC_LOBcliInterface(handleIn, 
                                        handleInLen, 
                                        blackBox, &blackBoxLen,
                                        handleOut, &handleOutLen,
                                        LOB_CLI_UPDATE_UNIQUE, LOB_CLI_ExecImmed,
                                        &dataOffset, &chunkMemSize,
                                        &outDescPartnKey, &outDescSyskey, 
                                        0,
                                        xnId,lobTrace_);
    
      if (clierr < 0 || clierr == 100)       
        return LOB_DESC_UPDATE_ERROR;
      inputSize -= chunkMemSize;
      if (so ==Lob_File)
        sourceFileReadOffset +=chunkMemSize;       
      inputAddr += chunkMemSize;
      //Insert rest of the chunks   
      while (inputSize >0)
        {
          chunkMemSize = MINOF(lobMaxChunkMemSize, inputSize);
          if (so == Lob_File)
            {
              //read a chunk of the file input data
              err = readSourceFile(source_filename,retBuf,chunkMemSize, sourceFileReadOffset);
             
              if (err != LOB_OPER_OK)              
                return err;
                
              inputAddr = retBuf;
            }
          str_sprintf(logBuf,"Calling writeLobData: inputAddr: %ld, InputSize%ld, tgtOffset:%ld",(long)inputAddr,sourceLen,dataOffset);
          err = writeLobData(inputAddr, chunkMemSize,so,dataOffset,operLen,lobMaxChunkMemSize);
          if (err != LOB_OPER_OK)
            {
              lobDebugInfo("writeLobData returned error",0,__LINE__,lobTrace_);
              return err;
            }
          lobDebugInfo("Calling cli LOB_CLI_INSERT_APPEND",0,__LINE__,lobTrace_);
          clierr = SQL_EXEC_LOBcliInterface(handleIn, handleInLen, 
                                            blackBox, &blackBoxLen,
                                            handleOut, &handleOutLen,
                                            LOB_CLI_INSERT_APPEND, LOB_CLI_ExecImmed,
                                            &dataOffset, &chunkMemSize,
                                            &outDescPartnKey, &outDescSyskey, 
                                            0,
                                            xnId,lobTrace_);
    
    
          if (clierr < 0 || clierr == 100) 
            { // some error or EOD.
              str_sprintf(logBuf,"cli LOB_CLI_INSERT_APPEND returned :%d", clierr);
              lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
              return LOB_DESC_APPEND_ERROR;
            }
          inputSize -= chunkMemSize;
          if (so ==Lob_File)
            sourceFileReadOffset +=chunkMemSize;       
          inputAddr += chunkMemSize;
        }
      return LOB_OPER_OK;
    }

Ex_Lob_Error ExLob::delDesc(char *handleIn, Int32 handleInLen, Int64 transId)
{
    Ex_Lob_Error err;   
    Int64 offset=0;
    Int64 dummyParam=0;  
    Lng32 clierr=0;

    clierr = SQL_EXEC_LOBcliInterface(handleIn, handleInLen, 
				      0, 0,
                                      (char *)&dummyParam, (Lng32 *)&dummyParam,
                                      LOB_CLI_DELETE, LOB_CLI_ExecImmed,
                                      &dummyParam, &dummyParam,
                                      &dummyParam, &dummyParam, 
				      0,
				      transId,lobTrace_);

    if (clierr < 0)
      return LOB_DESC_FILE_DELETE_ERROR;
    
    return LOB_OPER_OK;
 
}

Ex_Lob_Error ExLob::purgeLob()
{
    char logBuf[4096];
    if (! useLibHdfs_) {
       HDFS_Client_RetCode hdfsClientRetcode;
       hdfsClientRetcode = HdfsClient::hdfsDeletePath(lobDataFile_.data());
       if (hdfsClientRetcode != HDFS_CLIENT_OK)
	  return LOB_DATA_FILE_DELETE_ERROR;
       return LOB_OPER_OK;
    }
     if (hdfsDelete(fs_, lobDataFile_.data(), 0) != 0)
       {
         // extract a substring small enough to fit into logBuf
         size_t len = MINOF(lobDataFile_.length(),sizeof(logBuf)-40); 
         char lobDataFileSubstr[len+1];  // +1 for trailing null
         strncpy(lobDataFileSubstr,lobDataFile_.data(),len);
         lobDataFileSubstr[len] = '\0';
  
         str_sprintf(logBuf,"hdfsDelete of %s returned error",lobDataFileSubstr);
         lobDebugInfo("In ExLob::purgeLob",0,__LINE__,lobTrace_);
	 return LOB_DATA_FILE_DELETE_ERROR;
       }
    
    return LOB_OPER_OK;
}

Ex_Lob_Error ExLob::openCursor(char *handleIn, Int32 handleInLen,Int64 transId)
{
    Ex_Lob_Error err;
    cursor_t cursor;
    Int32 clierr;
    Int64 dummyParam = 0;
    void *cliInterface = NULL;
    char logBuf[4096];
   
    lobDebugInfo("In ExLob::openCursor",0,__LINE__,lobTrace_);
    clierr = SQL_EXEC_LOBcliInterface(handleIn, 
                                      handleInLen,
				      0,0,
                                      (char *)&dummyParam, (Lng32 *)&dummyParam,
                                      LOB_CLI_SELECT_CURSOR, LOB_CLI_ExecImmed,
                                      &dummyParam, &dummyParam,
                                      &dummyParam, &dummyParam, 
                                      &cliInterface,
                                      transId,lobTrace_);

    if (clierr <0 ) {
      str_sprintf(logBuf,"openCursor returned cliErr %d",clierr);
      return LOB_DESC_READ_ERROR;
    }

  
    cursor.bytesRead_ = -1;
    cursor.descOffset_ = -1;
    cursor.descSize_ = -1;
    cursor.cliInterface_ = cliInterface; // used only in lob process
    cursor.eod_ = false;
    cursor.eor_ = false; 
    cursor.eol_ = false;

    
    lobCursors_it it = lobCursors_.find(string(handleIn, handleInLen));

    if (it == lobCursors_.end())
    {
      lobCursors_.insert(pair<string, cursor_t>
                        (string(handleIn, handleInLen), cursor));
    }
    else
    {
       it->second = cursor;
    } 
    return LOB_OPER_OK;
}

Ex_Lob_Error ExLob::openDataCursor(const char *file, LobsCursorType type, 
                                   Int64 range, Int64 bufMaxSize, 
                                   Int64 maxBytes, Int64 waited, 
                                   ExLobGlobals *lobGlobals,
                                   Int32 *hdfsDetailError)
{
    Ex_Lob_Error err;
    cursor_t cursor;

    clock_gettime(CLOCK_MONOTONIC, &cursor.openTime_);
      
    // check to see if cursor is already open. 
    // occurs for pre-open cases
    lobCursorLock_.lock();

    lobCursors_it it = lobCursors_.find(string(file, strlen(file)));
    if (it != lobCursors_.end()) {
      clock_gettime(CLOCK_MONOTONIC, &cursor.openTime_);
      lobCursorLock_.unlock();
      return LOB_OPER_OK;
    }
    
    union ranges_t {
      Int64 range64;
      struct {
        Lng32 beginRange;
        Lng32 numRanges;
      }r;
    } ranges;
        
    cursor.bytesRead_ = -1;
    cursor.descOffset_ = -1;
    cursor.descSize_ = -1;
    cursor.cliInterface_ = NULL; // used only in lob process
    cursor.eod_ = false;
    cursor.eor_ = false;
    cursor.eol_ = false;
    cursor.type_ = type;
    cursor.bufMaxSize_ = bufMaxSize;
    cursor.maxBytes_ = maxBytes;  
    cursor.prefetch_ = !waited;
    cursor.bufferHits_ = 0;
    cursor.bufferMisses_ = 0;
    cursor.name_ = file;

    cursor.currentRange_ = -1;
    cursor.endRange_ = -1;
    cursor.currentStartOffset_ = -1;
    cursor.descOffset_ = range;
    
    cursor.currentFd_ = NULL;
    cursor.currentBytesToRead_ = -1;
    cursor.currentBytesRead_ = 0;
    cursor.currentEod_ = false;

    lobCursors_.insert(pair<string, cursor_t> 
                     (string(file, strlen(file)), cursor));

    it = lobCursors_.find(string(file, strlen(file))); // to get the actual cursor object in the map

    if (!fdData_ || (openFlags_ != O_RDONLY)) 
      {
        hdfsCloseFile(fs_, fdData_);
        fdData_ = NULL;
        openFlags_ = O_RDONLY;
        fdData_ = hdfsOpenFile(fs_, lobDataFile_.data(), openFlags_, 0, 0, 0);
       
        if (!fdData_)
          {
            openFlags_ = -1;
            if (hdfsDetailError)
              *hdfsDetailError = errno;
            lobCursorLock_.unlock();
            return LOB_DATA_FILE_OPEN_ERROR;
          }                
                 
        if (hdfsSeek(fs_, fdData_, (it->second).descOffset_) == -1) 
          {
            lobCursorLock_.unlock();
            return LOB_DATA_FILE_POSITION_ERROR;
          }
      }

    // start reading in a worker thread
    lobGlobals->enqueuePrefetchRequest(this, &(it->second));

    lobCursorLock_.unlock();
    
    return LOB_OPER_OK;
}

Ex_Lob_Error ExLob::readCursor(char *tgt, Int64 tgtSize, char *handleIn, Int32 handleInLen, Int64 &operLen,Int64 transId)
{
    int dataOffset;
    Ex_Lob_Error result;
    cursor_t cursor;
    char logBuf[4096];
    lobCursors_it it = lobCursors_.find(string(handleIn, handleInLen));
    lobDebugInfo("In ExLob::readCursor",0,__LINE__,lobTrace_);
    if (it == lobCursors_.end())
    {
       return LOB_CURSOR_NOT_OPEN;
    }
    else
    {
       cursor = it->second; 
    } 
    str_sprintf(logBuf,"ExLob::readCursor:: cliInterface:%ld,bytesRead_:%ld,descOffset_:%lddescSize_:%ld,eod_:%d,eor_:%d,eol_:%d,",(long)cursor.cliInterface_,cursor.bytesRead_,cursor.descOffset_,cursor.descSize_,cursor.eod_,cursor.eor_,cursor.eol_);
    lobDebugInfo(logBuf,0,__LINE__,lobTrace_);
    if (cursor.eod_) {
       // remove cursor from the map.
       // server has already closed the cursor. 
      closeCursor(handleIn, handleInLen,transId); 
       // indicate EOD to SQL
       operLen = 0; 
       return LOB_OPER_OK;
    }
    
    result = readCursorData(tgt, tgtSize, cursor, operLen, handleIn,handleInLen,transId); // increments cursor
        
    if (result != LOB_OPER_OK)
      return result;

    it->second = cursor;

    return LOB_OPER_OK;
}




Ex_Lob_Error ExLob::closeCursor(char *handleIn, Int32 handleInLen, Int64 transId)
{
  char logBuf[4096];
  Int64 dummyParam = 0;
  Int32 cliErr = 0;
  Ex_Lob_Error err = LOB_OPER_OK;
  lobCursors_it it = lobCursors_.find(string(handleIn, handleInLen));
  if (it == lobCursors_.end())
    {
      // cursor already closed
      return LOB_OPER_OK;                         
    }

  void *cliInterface = it->second.cliInterface_;
  if (cliInterface)
    {
      cliErr = SQL_EXEC_LOBcliInterface(handleIn, handleInLen, 
                                        NULL, NULL,
                                        (char *)&dummyParam, (Lng32 *)&dummyParam,
                                        LOB_CLI_SELECT_CLOSE, LOB_CLI_ExecImmed,
                                        &dummyParam, &dummyParam,
                                        &dummyParam, &dummyParam, 
                                        &cliInterface,
                                        transId,lobTrace_);
      if (cliErr <0 ) 
        {
          str_sprintf(logBuf, "LOB_CLI_SELECT_CLOSE Returned cli error  %d",cliErr);
          lobDebugInfo(logBuf,0,__LINE__,lobTrace_);
          err = LOB_DESC_READ_ERROR;
          return err;
        }
    }
  if (it != lobCursors_.end())
    {
      str_sprintf(logBuf,"closing cursor for handle");
      lobDebugInfo(logBuf,0,__LINE__,lobTrace_);    
      lobCursors_.erase(it);
    }
  
 
      
  return LOB_OPER_OK;
}

Ex_Lob_Error ExLob::checkAndDoGC(ULng32 size, Int64 &dataOffset, Int64 lobMaxSize, Int64 lobMaxChunkMemLen, char *handleIn, Int32 handleInLen, Int64 lobGCLimit, void *lobGlobals)
{
  NABoolean GCDone = FALSE;
    Ex_Lob_Error err = LOB_OPER_OK;
    Lng32 retval = 0;
    Int64 numRead = 0;
    Int64 numWritten = 0;
    dataOffset = 0;
    Int64 dummyParam = 0;
    if (size > lobMaxSize)
      return LOB_MAX_LIMIT_ERROR;
    char logBuf[4096];
    lobDebugInfo("In ExLob::checkAndDoGC",0,__LINE__,lobTrace_);
    Int32 openFlags = O_RDONLY ;   
    HDFS_Client_RetCode hdfsClientRetcode;
    hdfsFileInfo *fInfo = NULL;

    if (! useLibHdfs_) {
     
         dataOffset = hdfsClient_->hdfsSize(hdfsClientRetcode); 
         if (hdfsClientRetcode != HDFS_CLIENT_OK)
            return LOB_DATA_FILE_WRITE_ERROR;
         ex_assert(dataOffset >= 0, "Offset is -1 possibly due to path being directory");
         return  LOB_OPER_OK;
       
    }
    else
      {
        fInfo = hdfsGetPathInfo(fs_, lobDataFile_.data());
        if (fInfo)
          dataOffset = fInfo->mSize;
      }
    // if -1, don't do GC or if reached the limit do GC
    if ((lobGCLimit != -1) && (dataOffset > lobGCLimit)) 
      {
        str_sprintf(logBuf,"Starting GC. Current Offset : %ld",dataOffset);
        lobDebugInfo(logBuf,0,__LINE__,lobTrace_);
           
         
        Int32 rc = SQL_EXEC_LOB_GC_Interface(lobGlobals,handleIn,handleInLen,
                                             hdfsServer_,hdfsPort_,
                                             (char *)lobStorageLocation_.c_str(),
                                             lobMaxChunkMemLen,lobTrace_);
       
        if (rc<0)
          {
            lobDebugInfo("GC failed",0,__LINE__,lobTrace_); 
            GCDone = FALSE;
          }
        else
          GCDone = TRUE;
           
           
      }
      if (GCDone) // recalculate the new offset  
        {  
           if (! useLibHdfs_) 
             {
                dataOffset = hdfsClient_->hdfsSize(hdfsClientRetcode); 
                if (hdfsClientRetcode != HDFS_CLIENT_OK)
                  return LOB_DATA_FILE_WRITE_ERROR;
                ex_assert(dataOffset >= 0, "Offset is -1 possibly due to path being directory");
             }
           else
             {
               hdfsFreeFileInfo(fInfo, 1);
               fInfo = hdfsGetPathInfo(fs_, lobDataFile_.data());
               if (fInfo)
                 dataOffset = fInfo->mSize;

             }
        }
        
     
      // extract a substring small enough to fit into logBuf
      size_t len = MINOF(lobDataFile_.length(),sizeof(logBuf)-70); 
      char lobDataFileSubstr[len+1];  // +1 for trailing null
      strncpy(lobDataFileSubstr,lobDataFile_.data(),len);
      lobDataFileSubstr[len] = '\0';

      if (GCDone)
        str_sprintf(logBuf,"Done GC. Allocating new Offset %ld in %s",
                    dataOffset,lobDataFileSubstr);
      else
        str_sprintf(logBuf,"Allocating new Offset %ld in %s ",
                    dataOffset,lobDataFileSubstr);
      lobDebugInfo(logBuf,0,__LINE__,lobTrace_);
     
      return LOB_OPER_OK;    
}

Ex_Lob_Error ExLob::compactLobDataFile(ExLobInMemoryDescChunksEntry *dcArray,Int32 numEntries)
{
  Ex_Lob_Error rc = LOB_OPER_OK;
  char logBuf[4096];
  lobDebugInfo("In ExLob::compactLobDataFile",0,__LINE__,lobTrace_);
  Int64 maxMemChunk = 64*1024*1024; //64 MB limit for intermediate buffer for transfering data

  // make some temporary file names
  size_t len = lobDataFile_.length();
  char saveLobDataFile[len + sizeof("_save")]; // sizeof includes room for null terminator
  strcpy(saveLobDataFile,lobDataFile_.data());
  strcpy(saveLobDataFile+len,"_save");
  char tmpLobDataFile[len + sizeof("_tmp")]; // sizeof includes room for null terminator
  strcpy(tmpLobDataFile,lobDataFile_.data());
  strcpy(tmpLobDataFile+len,"_tmp");

  // extract small enough bits of these file names to fit in logBuf
  len = MINOF(lobDataFile_.length(),sizeof(logBuf)/3 - 20); 
  char lobDataFileSubstr[len + 1];
  strncpy(lobDataFileSubstr,lobDataFile_.data(),len);
  lobDataFileSubstr[len] = '\0';
  len = MINOF(sizeof(tmpLobDataFile),sizeof(logBuf)/3 - 20); 
  char tmpLobDataFileSubstr[len + 1];
  strncpy(tmpLobDataFileSubstr,tmpLobDataFile,len);
  tmpLobDataFileSubstr[len] = '\0';
  len = MINOF(sizeof(saveLobDataFile),sizeof(logBuf)/3 - 20);
  char saveLobDataFileSubstr[len + 1];
  strncpy(saveLobDataFileSubstr,saveLobDataFile,len);
  saveLobDataFileSubstr[len] = '\0';

  str_sprintf(logBuf,"DataFile %s, TempDataFile : %s, SaveDataFile : %s ",
              lobDataFileSubstr,tmpLobDataFileSubstr, saveLobDataFileSubstr);

  lobDebugInfo(logBuf,0,__LINE__,lobTrace_);

  HDFS_Client_RetCode hdfsClientRetcode = HDFS_CLIENT_OK;
  HdfsClient *srcHdfsClient = HdfsClient::newInstance(getLobGlobalHeap(), NULL, hdfsClientRetcode);
  ex_assert(hdfsClientRetcode == HDFS_CLIENT_OK, "Internal error: HdfsClient::newInstance returned an error");
  HdfsClient *dstHdfsClient = HdfsClient::newInstance(getLobGlobalHeap(), NULL, hdfsClientRetcode);
  ex_assert(hdfsClientRetcode == HDFS_CLIENT_OK, "Internal error: HdfsClient::newInstance returned an error");
  
  hdfsClientRetcode  = srcHdfsClient->hdfsOpen(lobDataFile_.data(), FALSE); 
  if (hdfsClientRetcode != HDFS_CLIENT_OK)
    {
      // extract substring small enough to fit in logBuf
      len = MINOF(lobDataFile_.length(),sizeof(logBuf) - 40); 
      char lobDataFileSubstr2[len + 1];
      strncpy(lobDataFileSubstr2,lobDataFile_.data(),len);
      lobDataFileSubstr2[len] = '\0';

      str_sprintf(logBuf,"Could not open file:%s",lobDataFileSubstr2);
      lobDebugInfo(logBuf,0,__LINE__,lobTrace_);
      return LOB_DATA_FILE_OPEN_ERROR;
    }
        
  hdfsClientRetcode  = dstHdfsClient->hdfsCreate(tmpLobDataFile, TRUE, FALSE, FALSE); 
  if (hdfsClientRetcode != HDFS_CLIENT_OK)
    {
      // extract substring small enough to fit in logBuf
      len = MINOF(sizeof(tmpLobDataFile),sizeof(logBuf)/3 - 20); 
      char tmpLobDataFileSubstr2[len + 1];
      strncpy(tmpLobDataFileSubstr2,tmpLobDataFile,len);
      tmpLobDataFileSubstr2[len] = '\0';

      str_sprintf(logBuf,"Could not open file:%s",tmpLobDataFileSubstr2);
      lobDebugInfo(logBuf,0,__LINE__,lobTrace_);
      srcHdfsClient->hdfsClose();
      HdfsClient::deleteInstance(srcHdfsClient);
      return LOB_DATA_FILE_OPEN_ERROR;
    }

   Int32 i = 0;
   Int64 bytesRead = 0;
   Int64 bytesWritten = 0;
   Int64 size = 0;
   Int64 chunkLen = 0;
   Int64 readLen = 0;
   Int64 offset;
   char * tgt = NULL;
   Ex_Lob_Error  saveError = LOB_OPER_OK;
   tgt = (char *)(getLobGlobalHeap())->allocateMemory(maxMemChunk);
   while ((i < numEntries) && (saveError == LOB_OPER_OK)) 
   {
       readLen = dcArray[i].getChunkLen();
       offset = dcArray[i].getCurrentOffset(); 
       while (readLen > 0) 
       {             
          if (readLen > maxMemChunk)
             chunkLen = maxMemChunk;
          else
             chunkLen = readLen;  
          bytesRead = srcHdfsClient->hdfsRead(offset, tgt, chunkLen, hdfsClientRetcode);
          if (hdfsClientRetcode != HDFS_CLIENT_OK) {
             lobDebugInfo("Problem reading from  data file",0,__LINE__,lobTrace_);
             saveError = LOB_SOURCE_FILE_READ_ERROR;
             break;
          }
          bytesWritten = dstHdfsClient->hdfsWrite(tgt, chunkLen, hdfsClientRetcode);
          if (hdfsClientRetcode != HDFS_CLIENT_OK || bytesWritten != chunkLen) {
             lobDebugInfo("Problem writing temp data file",0,__LINE__,lobTrace_);
             saveError = LOB_DATA_FILE_WRITE_ERROR;
             break;
          }
          readLen -= chunkLen;
          offset += chunkLen;
       }
       i++;
   }
   getLobGlobalHeap()->deallocateMemory(tgt);
   srcHdfsClient->hdfsClose(); 
   dstHdfsClient->hdfsClose();  
   HdfsClient::deleteInstance(srcHdfsClient);
   HdfsClient::deleteInstance(dstHdfsClient);
   if (saveError != LOB_OPER_OK)
      return saveError;
   //Now save the data file and rename the tempfile to the original datafile

   hdfsClientRetcode = HdfsClient::hdfsRename(lobDataFile_.data(),saveLobDataFile);
   if (hdfsClientRetcode != HDFS_CLIENT_OK) {
       lobDebugInfo("Problem renaming datafile to save data file",0,__LINE__,lobTrace_);
       return LOB_DATA_FILE_WRITE_ERROR;
   }
   hdfsClientRetcode = HdfsClient::hdfsRename(tmpLobDataFile, lobDataFile_.data());
   if (hdfsClientRetcode != HDFS_CLIENT_OK) {
       lobDebugInfo("Problem renaming temp datafile to data file",0,__LINE__,lobTrace_);
       return LOB_DATA_FILE_WRITE_ERROR;
   }
   return LOB_OPER_OK;
}

Ex_Lob_Error ExLob::restoreLobDataFile()
{
  lobDebugInfo("In ExLob::restoreLobDataFile",0,__LINE__,lobTrace_);
  HDFS_Client_RetCode hdfsClientRetcode; 
  char saveLobDataFile[lobDataFile_.length() + sizeof("_save")]; // sizeof includes room for null terminator
  strcpy(saveLobDataFile,lobDataFile_.data());
  strcpy(saveLobDataFile+lobDataFile_.length(),"_save");
  hdfsClientRetcode = HdfsClient::hdfsDeletePath(lobDataFile_.data());
  hdfsClientRetcode = HdfsClient::hdfsRename(saveLobDataFile, lobDataFile_.data());
  if (hdfsClientRetcode != HDFS_CLIENT_OK)
     {
       lobDebugInfo("Problem renaming savedatafile to data file",0,__LINE__,lobTrace_);
       return LOB_OPER_ERROR; 
     }
  return LOB_OPER_OK;

} 

Ex_Lob_Error ExLob::purgeBackupLobDataFile()
{
  Ex_Lob_Error rc = LOB_OPER_OK;
  lobDebugInfo("In ExLob::purgeBackupLobDataFile",0,__LINE__,lobTrace_);
  char saveLobDataFile[lobDataFile_.length() + sizeof("_save")]; // sizeof includes room for null terminator
  strcpy(saveLobDataFile,lobDataFile_.data());
  strcpy(saveLobDataFile+lobDataFile_.length(),"_save");
  HDFS_Client_RetCode hdfsClientRetcode  = HdfsClient::hdfsDeletePath(saveLobDataFile);//ok to ignore error.
  return rc;
}
///////////////////////////////////////////////////////////////////////////////
// ExLobDescHeader definitions
///////////////////////////////////////////////////////////////////////////////

ExLobDescHeader::ExLobDescHeader(unsigned int size) :
    freeDesc_(0),
    dataOffset_(0),
    availSize_(size)
{
}

ExLobDescHeader::~ExLobDescHeader()
{
}

///////////////////////////////////////////////////////////////////////////////
// ExLobDesc definitions
///////////////////////////////////////////////////////////////////////////////

ExLobDesc::ExLobDesc(int offset, int size, int tail) :
    dataOffset_(offset),
    dataSize_(size),
    dataState_(EX_LOB_DATA_INITIALIZING),
    tail_(tail),
    next_(-1),
    prev_(-1),
    nextFree_(-1)
{
}

ExLobDesc::~ExLobDesc()
{
}

Ex_Lob_Error ExLob::readCursorData(char *tgt, Int64 tgtSize, cursor_t &cursor, Int64 &operLen, char *handleIn, Int32 handleLenIn, Int64 transId)
{
   ExLobDesc desc;
   Ex_Lob_Error err;
   Int64 bytesAvailable = 0;
   Int64 bytesToCopy = 0;
   Int64 bytesRead = 0;
   operLen = 0;
   tOffset offset; 
   struct timespec startTime; 
   struct timespec endTime;
   NABoolean isEOD=FALSE;
   Int64 outOffset = 0;
   Int64 outSize = 0;
   char logBuf[4096];
   lobDebugInfo("In ExLob::readCursorData",0,__LINE__,lobTrace_);

   while ( (operLen < tgtSize) && !cursor.eod_ )
   {
    
      if (cursor.bytesRead_ == cursor.descSize_) // time to read next chunck
      {
        err = fetchCursor(handleIn, handleLenIn,outOffset, outSize,isEOD,transId);
         if (err != LOB_OPER_OK) {
            return err;
         }

         if (isEOD) {
            cursor.eod_ = true; // subsequent call will return 100 and close the cursor
            continue;
         } else {
            cursor.descSize_ = outSize;
            cursor.descOffset_ = outOffset;
            cursor.bytesRead_ = 0;
            if (outSize == 0) // this is an empty lob entry
              continue;
         }
      }
      
      bytesAvailable = cursor.descSize_ - cursor.bytesRead_;
      bytesToCopy = min(bytesAvailable, tgtSize - operLen);
      offset = cursor.descOffset_ + cursor.bytesRead_;
      if (!useLibHdfs_) {
         HDFS_Client_RetCode hdfsClientRetcode;
         if (storage_ == Lob_External_HDFS_File) {
            HdfsClient *srcHdfsClient = HdfsClient::newInstance(getLobGlobalHeap(), NULL, hdfsClientRetcode);
            ex_assert(hdfsClientRetcode == HDFS_CLIENT_OK, "Internal error: HdfsClient::newInstance returned an error");
            hdfsClientRetcode  = srcHdfsClient->hdfsOpen(lobDataFile_.data(), FALSE);
            if (hdfsClientRetcode != HDFS_CLIENT_OK)
               return LOB_SOURCE_FILE_OPEN_ERROR;
            bytesRead = srcHdfsClient->hdfsRead(offset, tgt, bytesToCopy, hdfsClientRetcode);
            if (hdfsClientRetcode != HDFS_CLIENT_OK) {
               HdfsClient::deleteInstance(srcHdfsClient);
               return LOB_SOURCE_FILE_READ_ERROR;
            } 
            HdfsClient::deleteInstance(srcHdfsClient);
          }
          else {
               bytesRead = hdfsClient_->hdfsRead(offset, tgt, bytesToCopy, hdfsClientRetcode);
               if (hdfsClientRetcode != HDFS_CLIENT_OK)
                  return LOB_DATA_READ_ERROR;
          }
      }
      else {

      // #endif

      if (!fdData_ || (openFlags_ != O_RDONLY)) 
        {
          hdfsCloseFile(fs_, fdData_);
          fdData_=NULL;
          openFlags_ = O_RDONLY;
          fdData_ = hdfsOpenFile(fs_, lobDataFile_.data(), openFlags_, 0, 0, 0);
        
          if (!fdData_)
            {
              openFlags_ = -1;
              return LOB_DATA_FILE_OPEN_ERROR;                 
            }               
             
           
        }

      clock_gettime(CLOCK_MONOTONIC, &startTime);

      bytesRead = hdfsPread(fs_, fdData_, offset, tgt, bytesToCopy);
      str_sprintf(logBuf,"After hdfsPread: BytesToCopy:%ld, Offset:%ld, tgt:%ld, BytesRead :%ld",
                  bytesToCopy,offset,(long)tgt,bytesRead);
      lobDebugInfo(logBuf,0,__LINE__,lobTrace_);
      clock_gettime(CLOCK_MONOTONIC, &endTime);

      Int64 secs = endTime.tv_sec - startTime.tv_sec;
      Int64 nsecs = endTime.tv_nsec - startTime.tv_nsec;
      if (nsecs < 0) {
        secs--;
        nsecs += NUM_NSECS_IN_SEC;
      }
      Int64 totalnsecs = (secs * NUM_NSECS_IN_SEC) + nsecs;
      } // useLibHdfs
      if (bytesRead == -1) {
         return LOB_DATA_READ_ERROR;
      } else if (bytesRead == 0) {
         cursor.eod_ = true;         
         continue;
      }


      cursor.bytesRead_ += bytesRead;
      operLen += bytesRead;
      tgt += bytesRead;
   }
   if (useLibHdfs_) {
      hdfsCloseFile(fs_, fdData_);
      fdData_ = NULL;
   }
   return LOB_OPER_OK;
}



Ex_Lob_Error ExLob::readDataToMem(char *memAddr,
                                  Int64 offset, Int64 size, Int64 &operLen,
                                  char *handleIn, Int32 handleLenIn, 
                                  NABoolean multipleChunks, Int64 transId)

{ 
  Ex_Lob_Error err = LOB_OPER_OK;
  operLen = 0;
  Int64 bytesRead = 0;
  char logBuf[4096];
  lobDebugInfo("In ExLob::readToMem",0,__LINE__,lobTrace_);
  if (multipleChunks) 
    {
      lobDebugInfo("Reading in multiple chunks",0,__LINE__,lobTrace_);
      err = openCursor(handleIn, 
		       handleLenIn,transId);
      //now we can fetch the descriptors for each chunk
    }
   
  if (err != LOB_OPER_OK)
    return err;
  

  if (useLibHdfs_) 
  {
  if (fdData_)// we may have a stale handle. close and open to refresh 
    {
      hdfsCloseFile(fs_, fdData_);
      fdData_=NULL;
      openFlags_ = O_RDONLY;
      fdData_ = hdfsOpenFile(fs_, lobDataFile_.data(), openFlags_, 0, 0, 0);
    
      if (!fdData_)
        {
          openFlags_ = -1;
          return LOB_DATA_FILE_OPEN_ERROR;
        }
                                           
        
    }
  else
    {
      fdData_ = hdfsOpenFile(fs_, lobDataFile_.data(), openFlags_, 0, 0, 0);
     
      if (!fdData_)
        {
          openFlags_ = -1;
          return LOB_DATA_FILE_OPEN_ERROR;
        }                                
        
          
    }
	
  } // useLibHdfs_
     
  if (!multipleChunks)
    {
      if (! useLibHdfs_) {
         HDFS_Client_RetCode hdfsClientRetcode;
         Int32 readLen;
         if (storage_ == Lob_External_HDFS_File) {
            HdfsClient *srcHdfsClient = HdfsClient::newInstance(getLobGlobalHeap(), NULL, hdfsClientRetcode);
            ex_assert(hdfsClientRetcode == HDFS_CLIENT_OK, "Internal error: HdfsClient::newInstance returned an error");
            hdfsClientRetcode  = srcHdfsClient->hdfsOpen(lobDataFile_.data(), FALSE);
            if (hdfsClientRetcode != HDFS_CLIENT_OK) {
               HdfsClient::deleteInstance(srcHdfsClient);
               return LOB_SOURCE_FILE_OPEN_ERROR;
            }
            readLen = srcHdfsClient->hdfsRead(offset, memAddr, size, hdfsClientRetcode);
            if (hdfsClientRetcode != HDFS_CLIENT_OK) {
               HdfsClient::deleteInstance(srcHdfsClient);
               return LOB_SOURCE_FILE_READ_ERROR;
            }  
            HdfsClient::deleteInstance(srcHdfsClient);
            operLen = readLen;
         } 
         else {
            readLen = hdfsClient_->hdfsRead(offset, memAddr, size, hdfsClientRetcode);
            if (hdfsClientRetcode != HDFS_CLIENT_OK)
               return LOB_DATA_READ_ERROR;
            operLen = readLen;
         }
         return LOB_OPER_OK;
      }
      lobDebugInfo("Reading in single chunk",0,__LINE__,lobTrace_);
      if ((bytesRead = hdfsPread(fs_, fdData_, offset, 
				 memAddr, size)) == -1) {
	  
	return LOB_DATA_READ_ERROR;
      } 
      
      // extract a substring small enough to fit into logBuf
      size_t len = MINOF(lobDataFile_.length(),sizeof(logBuf)-100); 
      char lobDataFileSubstr[len+1];  // +1 for trailing null
      strncpy(lobDataFileSubstr,lobDataFile_.data(),len);
      lobDataFileSubstr[len] = '\0';

      //str_sprintf(logBuf,"After hdfsPread: File:%s, Offset:%ld, Size:%ld,Target Mem Addr:%d",
       //           lobDataFileSubstr,offset,size,memAddr);
      str_sprintf(logBuf,"After hdfsPread: File:%s, Offset:%ld, Size:%ld",
                  lobDataFileSubstr,offset,size);
      lobDebugInfo(logBuf,0,__LINE__,lobTrace_);
      operLen = bytesRead;
      return LOB_OPER_OK;
    }
  else
    {
      //handle reading the multiple chunks like a cursor
      err = readCursor(memAddr,size, handleIn,
		       handleLenIn, operLen, transId);
	 	 
      if (err==LOB_OPER_OK)
	closeCursor(handleIn, 
		    handleLenIn,transId);
      else
	return err;
    }
  return LOB_OPER_OK;
}

 

Ex_Lob_Error ExLob::readDataToLocalFile(char *fileName,  Int64 offset, Int64 size, Int64 &writeOperLen, Int64 lobMaxChunkMemSize, Int32 fileflags,char *handleIn,Int32 handleInLen, NABoolean multipleChunks,Int64 transId)
{ 
    Ex_Lob_Error err;
    Int64 operLen = 0;
   
    Int64 srcLen = size;
    Int64 srcOffset = offset;
    Int64 tgtOffset = 0;
    char *lobData = 0;
    Int64 chunkSize = 0;
    char logBuf[4096];
    lobDebugInfo("In ExLob::readDataToLocalFile",0,__LINE__,lobTrace_);
    if (srcLen <=0)
       return LOB_SOURCE_DATA_ALLOC_ERROR;
    // open the target file for writing 
    int filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    int openFlags = O_RDWR ; // O_DIRECT needs mem alignment
    if (((LobTgtFileFlags)fileflags == Lob_Append_Or_Error ) ||
	((LobTgtFileFlags)fileflags == Lob_Error_Or_Create ) ||
	((LobTgtFileFlags)fileflags == Lob_Append_Or_Create))
      openFlags |= O_APPEND;
    else
      openFlags |= O_TRUNC;
    int fdDestFile = open(fileName, openFlags, filePerms);
	  
    if (fdDestFile >=0 )
      {
	if ((LobTgtFileFlags)fileflags == Lob_Error_Or_Create)
	    return LOB_TARGET_FILE_EXISTS_ERROR;	  
      }
    if (fdDestFile == -1) 
      {
	if (((LobTgtFileFlags)fileflags == Lob_Append_Or_Error) ||
	    ((LobTgtFileFlags)fileflags == Lob_Truncate_Or_Error))
	  return LOB_TARGET_FILE_OPEN_ERROR;	  
	else
	  {
	    openFlags = O_CREAT | O_RDWR ;
	    fdDestFile = open(fileName, openFlags, filePerms);
	    if (fdDestFile == -1)	  
	      return LOB_TARGET_FILE_OPEN_ERROR; 
	  }
      }
    if ((srcLen < lobMaxChunkMemSize) && (multipleChunks ==FALSE)) // simple single I/O case
      {
        lobDebugInfo("Reading in single chunk",0,__LINE__,lobTrace_);
	lobData = (char *) (getLobGlobalHeap())->allocateMemory(srcLen);

	if (lobData == NULL) 
	  {
	    return LOB_SOURCE_DATA_ALLOC_ERROR;
	  }
	err = readDataToMem(lobData, srcOffset,srcLen,operLen, handleIn,handleInLen, multipleChunks,transId);
	if (err != LOB_OPER_OK)
	  {
	    getLobGlobalHeap()->deallocateMemory(lobData);
	    return err;
	  }
       
	writeOperLen += pwrite(fdDestFile, lobData, srcLen, tgtOffset) ; 
	if (writeOperLen <= 0)
	  {
	    getLobGlobalHeap()->deallocateMemory(lobData);
	    return LOB_TARGET_FILE_WRITE_ERROR;
	  }
	getLobGlobalHeap()->deallocateMemory(lobData);
      }
    else // multiple chunks to read
      {
        lobDebugInfo("Reading in multiple chunks into local file",0,__LINE__,lobTrace_);
	err = openCursor(handleIn, 
                         handleInLen,transId);
	if (err != LOB_OPER_OK)
	  return err;
	while ( srcLen > 0)
	  {
	    chunkSize = MINOF(srcLen, lobMaxChunkMemSize);
	    lobData = (char *) (getLobGlobalHeap())->allocateMemory(chunkSize);

	    if (lobData == NULL) 
	      {
		getLobGlobalHeap()->deallocateMemory(lobData);
		return LOB_SOURCE_DATA_ALLOC_ERROR;
	      }
	    //handle reading the multiple chunks like a cursor
	    err = readCursor(lobData,chunkSize, handleIn,
			     handleInLen, operLen, transId);
            if ((operLen == 0) && (err == LOB_OPER_OK)) //this may be an empty lob section
              continue;
	    if ((err != LOB_OPER_OK) || (operLen != chunkSize))
	      {
		getLobGlobalHeap()->deallocateMemory(lobData);
		return LOB_DATA_READ_ERROR;
	      }
       
	    writeOperLen += pwrite(fdDestFile, lobData, chunkSize, tgtOffset) ; 
	    if (writeOperLen <= 0)
	      {
		getLobGlobalHeap()->deallocateMemory(lobData);
		return LOB_TARGET_FILE_WRITE_ERROR;
	      }     
	    getLobGlobalHeap()->deallocateMemory(lobData);
	    srcLen -= chunkSize;
	    tgtOffset += chunkSize;     
	  }
	closeCursor(handleIn, 
		    handleInLen,transId);
      }
    close(fdDestFile);
    return LOB_OPER_OK;
}


Ex_Lob_Error ExLob::readDataToHdfsFile(char *tgtFileName,  Int64 offset, Int64 size, Int64 &writeOperLen, Int64 lobMaxChunkMemLen, Int32 fileflags,char *handleIn, Int32 handleInLen, NABoolean multipleChunks,Int64 transId)
{ 
  Ex_Lob_Error err;
  Int64 operLen = 0;
 
  Int64 srcLen = size;
  Int64 srcOffset = offset;
  Int64 tgtOffset = 0;
  char *lobData = NULL;
  Int64 chunkSize = 0;	
  hdfsFile  fdTgtFile;
  char logBuf[4096];
  writeOperLen = 0;
  HdfsClient *tgtHdfsClient;
  HDFS_Client_RetCode hdfsClientRetcode;
  NABoolean overwrite = FALSE;
  NABoolean append = FALSE;
  Int64 remainLen = size;
  Int64 pos = offset;
  Int64 readLen;
  lobDebugInfo("In ExLob::readDataToHdfsFile",0,__LINE__,lobTrace_);
  // open and write to the target file
  int openFlags = O_WRONLY;
  if (! useLibHdfs_) {
     if (((LobTgtFileFlags)fileflags == Lob_Truncate_Or_Error) ||
         ((LobTgtFileFlags)fileflags == Lob_Truncate_Or_Create))
        overwrite = TRUE; 
     if (((LobTgtFileFlags)fileflags == Lob_Append_Or_Error) ||
	 ((LobTgtFileFlags)fileflags == Lob_Append_Or_Create))
        append = TRUE; 
     tgtHdfsClient = HdfsClient::newInstance(getLobGlobalHeap(), NULL, hdfsClientRetcode);
     ex_assert(hdfsClientRetcode == HDFS_CLIENT_OK, "Internal error: HdfsClient::newInstance returned an error");
     if (tgtHdfsClient->hdfsCreate(tgtFileName, overwrite, append, FALSE) !=  HDFS_CLIENT_OK)
        return LOB_TARGET_FILE_OPEN_ERROR; 
  } else {
  if ((LobTgtFileFlags)fileflags == Lob_Append_Or_Error )
    openFlags |= O_APPEND;
  //hdfsFile fdTgtFile = hdfsOpenFile(fs_,tgtFileName, openFlags, 0,0,0);
  if (hdfsExists(fs_,tgtFileName) == 0)
    {
      if ((LobTgtFileFlags)fileflags == Lob_Error_Or_Create)
	return LOB_TARGET_FILE_EXISTS_ERROR;
      else
	{
	  
	  openFlags =  O_WRONLY ;
	  if ((LobTgtFileFlags)fileflags == Lob_Append_Or_Error )
	    openFlags |= O_APPEND;
	  fdTgtFile = hdfsOpenFile(fs_, tgtFileName, openFlags, 0,0,0);
	  if (fdTgtFile == NULL)
	    return LOB_TARGET_FILE_OPEN_ERROR;
	}
    }
  else
    { 
      if (((LobTgtFileFlags)fileflags == Lob_Append_Or_Error) ||
	  ((LobTgtFileFlags)fileflags == Lob_Truncate_Or_Error))
	return LOB_TARGET_FILE_OPEN_ERROR;	  
      else
	{ 
	  openFlags =  O_WRONLY ;
	  fdTgtFile = hdfsOpenFile(fs_, tgtFileName, openFlags, 0,0,0);
	  if (fdTgtFile == NULL)
	    return LOB_TARGET_FILE_OPEN_ERROR;
	}
    } 
  }
  if (!multipleChunks) {
   if (! useLibHdfs_) {
     Int32 bytesRead; 
     Int32 bytesWritten;
     while (remainLen > 0) 
     {
        if (remainLen > lobMaxChunkMemLen)
           readLen = lobMaxChunkMemLen;
        else
           readLen = remainLen;
        if (lobData == NULL)
           lobData = new (lobGlobalHeap_) char[readLen];
        bytesRead = hdfsClient_->hdfsRead(pos, lobData, readLen, hdfsClientRetcode);
        if (hdfsClientRetcode == HDFS_CLIENT_OK) 
            bytesWritten = tgtHdfsClient->hdfsWrite(lobData, bytesRead, hdfsClientRetcode, lobMaxChunkMemLen);
        if (hdfsClientRetcode == HDFS_CLIENT_OK) {
           pos += bytesRead;
           remainLen -= bytesRead;
           writeOperLen += bytesWritten;
        } else {
            NADELETEBASIC(lobData, lobGlobalHeap_);
            HdfsClient::deleteInstance(tgtHdfsClient);
            return LOB_DATA_READ_ERROR;
        }   
     }
     HdfsClient::deleteInstance(tgtHdfsClient);
     return LOB_OPER_OK; 
    } // !multipleChunk && !useLibHdfs
    else { 
      if ((srcLen < lobMaxChunkMemLen))
      {
         lobDebugInfo("Reading in single chunk",0,__LINE__,lobTrace_);
         lobData = (char *) (getLobGlobalHeap())->allocateMemory(srcLen);

         if (lobData == NULL) 
	{
	  return LOB_SOURCE_DATA_ALLOC_ERROR;
	}
      err = readDataToMem(lobData, srcOffset,srcLen,operLen,handleIn,handleInLen, multipleChunks,transId);
      if (err != LOB_OPER_OK)
	{
	  getLobGlobalHeap()->deallocateMemory(lobData);
	  return err;
	}
     
      writeOperLen += hdfsWrite(fs_,fdTgtFile,lobData, srcLen);
      if (writeOperLen <= 0)
	{
	  getLobGlobalHeap()->deallocateMemory(lobData);  
	  return LOB_TARGET_FILE_WRITE_ERROR;
	}
      if (hdfsFlush(fs_, fdTgtFile)) 
	{
	  getLobGlobalHeap()->deallocateMemory(lobData);
	  return LOB_DATA_FLUSH_ERROR;
	} 
      getLobGlobalHeap()->deallocateMemory(lobData);
    }
   } // !multipleChunk && useLibHdfs
 } // !multipleChunk
 else {// multiple chunks to read
      lobDebugInfo("Reading in multiple chunks into local file",0,__LINE__,lobTrace_);
      err = openCursor(handleIn, 
		       handleInLen,
                       transId);
      if (err != LOB_OPER_OK)
	return err;
      chunkSize = MINOF(srcLen, lobMaxChunkMemLen);
      lobData = (char *) (getLobGlobalHeap())->allocateMemory(chunkSize);	      
      if (lobData == NULL) 
         return LOB_SOURCE_DATA_ALLOC_ERROR;
      while ( srcLen > 0)
	{
	  chunkSize = MINOF(srcLen, lobMaxChunkMemLen);
	  //handle reading the multiple chunks like a cursor
	  err = readCursor(lobData,chunkSize, handleIn,
			   handleInLen, operLen, transId);
          if ((operLen == 0) && (err == LOB_OPER_OK)) //this may be an empty lob section
              continue;
	  if ((err != LOB_OPER_OK) || (operLen != chunkSize))
	    {
	      getLobGlobalHeap()->deallocateMemory(lobData);
	      return LOB_DATA_READ_ERROR;
	    }
          if (!useLibHdfs_) {
              writeOperLen += tgtHdfsClient->hdfsWrite(lobData, chunkSize, hdfsClientRetcode, lobMaxChunkMemLen);
              if (hdfsClientRetcode != HDFS_CLIENT_OK) {
                  NADELETEBASIC(lobData, lobGlobalHeap_);
                  HdfsClient::deleteInstance(tgtHdfsClient);
                  return LOB_TARGET_FILE_WRITE_ERROR;
              }
          }
          else {
	  writeOperLen += hdfsWrite(fs_,fdTgtFile,lobData, chunkSize);
	  if (writeOperLen <= 0)
	    {
	      getLobGlobalHeap()->deallocateMemory(lobData);  
	      return LOB_TARGET_FILE_WRITE_ERROR;
	    }
	  if (hdfsFlush(fs_, fdTgtFile)) 
	    {
	      getLobGlobalHeap()->deallocateMemory(lobData);
	      return LOB_DATA_FLUSH_ERROR;
	    } 
          }
	  srcLen -= chunkSize;
	}
      closeCursor(handleIn, 
		  handleInLen,transId);	    
  }
  getLobGlobalHeap()->deallocateMemory(lobData);
  if (! useLibHdfs_) {
     HdfsClient::deleteInstance(tgtHdfsClient);
  } else {
     hdfsCloseFile(fs_, fdTgtFile);
     fdTgtFile=NULL;
     hdfsCloseFile(fs_,fdData_);
     fdData_=NULL;
  }
  return LOB_OPER_OK;
}




Ex_Lob_Error ExLob::readDataToExternalFile(char *tgtFileName,  Int64 offset, Int64 size, Int64 &operLen,Int64 lobMaxChunkMemLen,Int32 fileflags,char *handleIn,Int32 handleInLen, NABoolean multipleChunks,Int64 transId)
{ 
  //TBD
  return LOB_OPER_OK;
}
Ex_Lob_Error ExLob::closeFile()
{
    if (fdData_) 
    {
      hdfsCloseFile(fs_, fdData_);
      fdData_ = NULL;
    }

    return LOB_OPER_OK;
}



Ex_Lob_Error ExLob::readStats(char *statsBuffer)
{
    stats_ = (ExHdfsScanStats *)statsBuffer;
    return LOB_OPER_OK;
}

Ex_Lob_Error ExLob::initStats()
{
    return LOB_OPER_OK;
}
//Main driver of any LOB related operation 

Ex_Lob_Error ExLobsOper (
			 char        *lobName,          // lob name
                         ExHdfsScanStats *hdfsAccessStats,
			 char        *handleIn,         // input handle (for cli calls)
			 Int32       handleInLen,       // input handle len

			 char        *hdfsServer,       // server where hdfs fs resides
			 Int64       hdfsPort,          // port number to access hdfs server

			 char        *handleOut,        // output handle (for cli calls)
			 Int32       &handleOutLen,     // output handle len

			 Int64       descNumIn,         // input desc Num (for flat files only)
			 Int64       &descNumOut,       // output desc Num (for flat files only)

			 Int64       &retOperLen,       // length of data involved in this operation

			 Int64       &hdfsDataOffset,  // returned by ::insertData    
			 Int64       &unused_data_member,    // unused

			 Ex_Lob_Error  &requestStatus,  // returned req status
			 Int64       &cliError,         // err returned by cli call

			 char        *lobStorageLocation,              // directory in the storage
			 LobsStorage storage,           // storage type

			 char        *source,           // source (memory addr, filename, foreign lob etc)
			 Int64       sourceLen,         // source len (memory len, foreign desc offset etc)

			 Int64       cursorBytes,
			 char        *cursorId,
			 LobsOper    operation,         // LOB operation
			 LobsSubOper subOperation,      // LOB sub operation
			 Int64       waited,            // waited or nowaited
			 ExLobGlobals        *&globPtr,         // ptr to the Lob objects. 
			 Int64       transId,
			 void        *blackBox,         // black box to be sent to cli
			 Int32       blackBoxLen,       // length of black box
			 Int64       lobMaxSize,
			 Int64       lobMaxChunkMemSize,
                         Int64       lobGCLimit,
			 int         bufferSize ,
			 short       replication ,
			 int         blockSize,
			 Lng32       openType)
{ 
  Ex_Lob_Error err = LOB_OPER_OK;
  ExLob *lobPtr = NULL;
  struct timespec startTime;
  struct timespec endTime;
  Int64 secs, nsecs, totalnsecs;
  ExLobPreOpen *preOpenObj;
  ExLobGlobals *lobGlobals = NULL;
  transId = 0;
  retOperLen = 0;
  ExLobDesc desc;
    
  lobMap_t *lobMap = NULL;
  lobMap_it it;

  clock_gettime(CLOCK_MONOTONIC, &startTime);

  const char *fileName = lobName;

  if (globPtr == NULL)
    {
      if ((operation == Lob_Init))
	{
          NAHeap *lobHeap = (NAHeap *)blackBox;
          globPtr = new (lobHeap) ExLobGlobals(lobHeap);
	  if (globPtr == NULL) 
	    return LOB_INIT_ERROR;

	  lobGlobals = (ExLobGlobals *)globPtr;

	  err = lobGlobals->initialize(); 
          if (err != LOB_OPER_OK)
            return err;
	}
      else
	{
	  return LOB_GLOB_PTR_ERROR;
	}
    }
  if (globPtr != NULL)
  {
    lobGlobals = (ExLobGlobals *)globPtr;
    if ((operation != Lob_Init) && (operation != Lob_Cleanup))
    {
      lobMap = lobGlobals->getLobMap();

      it = lobMap->find(string(fileName));

      if (it == lobMap->end())
	{
	  lobPtr = new (lobGlobals->getHeap())ExLob(lobGlobals->getHeap(), hdfsAccessStats);
	  if (lobPtr == NULL) 
	    return LOB_ALLOC_ERROR;

	  err = lobPtr->initialize(fileName, (operation == Lob_Create) ? EX_LOB_CREATE : EX_LOB_RW, lobStorageLocation, storage, hdfsServer, hdfsPort, lobStorageLocation,bufferSize, replication, blockSize,lobMaxSize,lobGlobals);
	  if (err != LOB_OPER_OK)
            {
              char buf[5000];
              str_sprintf(buf,"Lob initialization failed;filename:%s;location:%s;hdfsserver:%s;lobMaxSize:%ld",fileName,lobStorageLocation,hdfsServer,lobMaxSize);
              lobDebugInfo(buf,err,__LINE__,lobGlobals->lobTrace_);
              return err;
            }

	  lobMap->insert(pair<string, ExLob*>(string(fileName), lobPtr));
	}
      else
	{
	  lobPtr = it->second;
        
	}
      lobPtr->lobTrace_ = lobGlobals->lobTrace_;
    }
  }
  /* 
// **Note** This is code that needs to get called before sneding a request to the 
//mxlobsrvr process. It's inactive code currently   
  MS_Mon_Transid_Type transIdBig;
  MS_Mon_Transseq_Type transStartId;
  if (!lobGlobals->isHive())
    {
      // get current transaction
   
      int transIdErr = ms_transid_get(false, false, &transIdBig, &transStartId);
      // set the pass thru request object values in the lob
    
      lobPtr->getRequest()->setValues(lobPtr->getDescFileName(),
                                     descNumIn, handleInLen, handleIn, storage,
                                     transId, transIdBig, transStartId,
                                     (char *)blackBox, blackBoxLen);
    }
  */
  switch(operation)
    {
    case Lob_Init:
    case Lob_Create:
      break;

    case Lob_InsertDesc:
      
      err = lobPtr->writeDesc(sourceLen, source, subOperation, descNumOut, hdfsDataOffset, lobMaxSize, handleIn,handleInLen,(char *)blackBox, &blackBoxLen,handleOut,handleOutLen,transId,lobGlobals);
      if (err != LOB_OPER_OK)
        {
          lobDebugInfo("writeDesc failed ",err,__LINE__,lobGlobals->lobTrace_);
        }
        
      break;

    case Lob_InsertData:
      err = lobPtr->insertData(source, sourceLen, subOperation, hdfsDataOffset, retOperLen, lobMaxSize,lobMaxChunkMemSize,lobGCLimit,handleIn,handleInLen,(char *)blackBox, blackBoxLen,handleOut,handleOutLen,lobGlobals);
      if (err != LOB_OPER_OK)
        {
          lobDebugInfo("insertData failed ",err,__LINE__,lobGlobals->lobTrace_);
        }
      break;

    case Lob_InsertDataSimple:
      err = lobPtr->writeDataSimple(source, sourceLen, subOperation, retOperLen,
				    bufferSize , replication , blockSize);
      break;
    case Lob_InsSel:
      {
        
        ExLob *srcLobPtr;
        Int16 flags;
        Lng32  lobNum;
        Int32 lobType;
        Int64 uid, inDescSyskey, descPartnKey;
        short schNameLen;
        char schName[ComAnsiNamePart::MAX_IDENTIFIER_EXT_LEN+1];
        char sourceHandle[LOB_HANDLE_LEN]  = {};
        str_cpy_all(sourceHandle, source, sourceLen);
        ExpLOBoper::extractFromLOBhandle(&flags, &lobType, &lobNum, &uid,  
                                   &inDescSyskey, &descPartnKey,
                                   &schNameLen, schName,
                                   sourceHandle); 
        char srcLobNameBuf[LOB_NAME_LEN];
        char * srcLobName = 
          ExpLOBoper::ExpGetLOBname(uid, lobNum, srcLobNameBuf, LOB_NAME_LEN);
        lobMap_it it2;
        it2 = lobMap->find(string(srcLobName));

        if (it2 == lobMap->end())
	{
	  srcLobPtr = new (lobGlobals->getHeap())ExLob(lobGlobals->getHeap(), hdfsAccessStats);
	  if (srcLobPtr == NULL) 
	    return LOB_ALLOC_ERROR;

	  err = srcLobPtr->initialize(srcLobName, EX_LOB_RW, lobStorageLocation, storage, hdfsServer, hdfsPort, lobStorageLocation,bufferSize, replication, blockSize,lobMaxSize,lobGlobals);
	  if (err != LOB_OPER_OK)
            {
              char buf[5000];
              str_sprintf(buf,"Lob initialization failed;filename:%s;location:%s;hdfsserver:%s;lobMaxSize:%ld",srcLobName,lobStorageLocation,hdfsServer,lobMaxSize);
              lobDebugInfo(buf,err,__LINE__,lobGlobals->lobTrace_);
              return err;
            }

	  lobMap->insert(pair<string, ExLob*>(string(srcLobName), srcLobPtr));
	}
      else	
        srcLobPtr = it2->second;

        err = lobPtr->insertSelect(srcLobPtr,handleIn, handleInLen, source, sourceLen, retOperLen, lobMaxSize, lobMaxChunkMemSize,lobGCLimit,(char *)blackBox, blackBoxLen,handleOut,handleOutLen,subOperation,transId, lobGlobals);
        if (err != LOB_OPER_OK)
          {
            lobDebugInfo("insertSelect failed ",err,__LINE__,lobGlobals->lobTrace_);
          }
      }
      break;
    case Lob_Read:
      if (storage == Lob_External_HDFS_File)   
        //Allocate storage to read the lob external file name from the 
        //descriptor tables  to get the data from.
        // TODO: do we ever take this code path for Hive files?       
        blackBox = new(lobGlobals->getHeap()) char[MAX_LOB_FILE_NAME_LEN+6];
          
      if (subOperation == Lob_Memory)
        {
          err = lobPtr->readToMem(source,sourceLen,retOperLen,handleIn,handleInLen,(char *)blackBox, blackBoxLen,handleOut,handleOutLen,transId);
          if (err != LOB_OPER_OK)
            {
              lobDebugInfo("readToMem failed ",err,__LINE__,lobGlobals->lobTrace_);
            }
        }
      else if (subOperation == Lob_File)
        {
          err = lobPtr->readToFile(source, sourceLen, retOperLen, lobMaxChunkMemSize,  openType,handleIn,handleInLen,(char *)blackBox, blackBoxLen,handleOut,handleOutLen,transId);
          if (err != LOB_OPER_OK)
            {
              lobDebugInfo("readToFile failed ",err,__LINE__,lobGlobals->lobTrace_);
            }
        }
      else  
	err = LOB_SUBOPER_ERROR;
      if (blackBox)
        (lobGlobals->getHeap())->deallocateMemory((char*) blackBox);
      break;
    break;
    case Lob_GetFileSize:
      {
        err = lobPtr->statSourceFile(lobName,retOperLen);  
      }
      break;
    case Lob_ReadSourceFile:
      {
        char *bufAddr = (char *)unused_data_member; 
        err = lobPtr->readSourceFile(lobName,bufAddr,descNumOut,descNumIn); 
        unused_data_member = (Int64)bufAddr;
      }
      break;
    case Lob_GetLength:
      {
        err = lobPtr->getLength(handleIn, handleInLen,retOperLen,subOperation,transId);  
      }
      break;
  case Lob_GetOffset:
      {
        err = lobPtr->getOffset(handleIn, handleInLen,retOperLen,subOperation,transId);  
      }
      break;
    case Lob_GetFileName:
      {
        err = lobPtr->getFileName(handleIn, handleInLen, (char *)blackBox, blackBoxLen,  subOperation, transId);
      }
      break;
    case Lob_ReadDesc: // read desc only. Needed for pass thru.
      err = lobPtr->getDesc(desc,handleIn,handleInLen,(char *)blackBox, &blackBoxLen,handleOut,handleOutLen,transId); 
      retOperLen = 0;
      break;
    case Lob_OpenCursor:
      err = lobPtr->openCursor(handleIn, handleInLen,transId);
      break;

    case Lob_OpenDataCursorSimple:
      {
        size_t dataFileNameLen = strlen(lobPtr->getDataFileName());
        size_t cursorIdLen = strlen(cursorId); 
        if (openType == 1) { // preopen
          char temp1[30];  // big enough for :%Lx:
          sprintf(temp1, ":%Lx:",(long long unsigned int)lobName);
          char fn[dataFileNameLen + sizeof(temp1) + cursorIdLen + 1];
          strcpy(fn,lobPtr->getDataFileName());
          strcpy(fn + dataFileNameLen, temp1);
          strcpy(fn + dataFileNameLen + strlen(temp1), cursorId);
          preOpenObj = new (lobGlobals->getHeap()) ExLobPreOpen(lobPtr, fn, descNumIn, sourceLen, 
                                                                cursorBytes, waited, lobGlobals->getHeap());
          lobGlobals->addToPreOpenList(preOpenObj);
        } else if (openType == 2) { // must open
          char temp2[30];  // big enough for :%Lx:
          sprintf(temp2, ":%Lx:",(long long unsigned int)lobName);
          char fn[dataFileNameLen + sizeof(temp2) + cursorIdLen + 1];
          strcpy(fn,lobPtr->getDataFileName());
          strcpy(fn + dataFileNameLen, temp2);
          strcpy(fn + dataFileNameLen + strlen(temp2), cursorId);
          err = lobPtr->openDataCursor(fn, Lob_Cursor_Simple, descNumIn, sourceLen, cursorBytes, waited, lobGlobals, (Int32 *)blackBox);
        } else
          err = LOB_SUBOPER_ERROR;
      }
      break;

    case Lob_ReadCursor:
      if ((subOperation == Lob_Memory) || (subOperation == Lob_Buffer))
	err = lobPtr->readCursor(source, sourceLen, handleIn, handleInLen, retOperLen,transId);
      else if (subOperation == Lob_File)
	err = lobPtr->readCursor(source, -1, handleIn, handleInLen, retOperLen,transId);
      else  
	err = LOB_SUBOPER_ERROR;
      break;

    case Lob_ReadDataCursorSimple:
      {
        char temp3[30];  // big enough for :%Lx:
        sprintf(temp3, ":%Lx:",(long long unsigned int)lobName);
        size_t dataFileNameLen = strlen(lobPtr->getDataFileName());
        size_t cursorIdLen = strlen(cursorId);
        char fn[dataFileNameLen + sizeof(temp3) + cursorIdLen + 1];
        strcpy(fn,lobPtr->getDataFileName());
        strcpy(fn + dataFileNameLen, temp3);
        strcpy(fn + dataFileNameLen + strlen(temp3), cursorId);       
        err = lobPtr->readDataCursorSimple(fn, source, sourceLen, retOperLen, lobGlobals);
      }
      break;

    case Lob_CloseFile:
      if (lobPtr->hasNoOpenCursors()) {
	lobGlobals->traceMessage("Lob_CloseFile",NULL,__LINE__);
	err = lobPtr->closeFile();
      }  
      break;

    case Lob_CloseCursor:
      err = lobPtr->closeCursor(handleIn, handleInLen,transId);
      break;

    case Lob_CloseDataCursorSimple:
      {
        char temp4[30];  // big enough for :%Lx:
        sprintf(temp4, ":%Lx:",(long long unsigned int)lobName);
        size_t dataFileNameLen = strlen(lobPtr->getDataFileName());
        size_t cursorIdLen = strlen(cursorId);
        char fn[dataFileNameLen + sizeof(temp4) + cursorIdLen + 1];
        strcpy(fn,lobPtr->getDataFileName());
        strcpy(fn + dataFileNameLen, temp4);
        strcpy(fn + dataFileNameLen + strlen(temp4), cursorId); 
        err = lobPtr->closeDataCursorSimple(fn, lobGlobals);
      }
      break;

    case Lob_Append:
      if ((subOperation == Lob_Memory) ||(subOperation == Lob_Buffer) || (subOperation ==Lob_External_File))
        {
          err = lobPtr->append(source, sourceLen, subOperation, descNumIn, retOperLen,lobMaxSize, lobMaxChunkMemSize,lobGCLimit,handleIn,handleInLen,handleOut,handleOutLen,transId, lobGlobals);
          if (err != LOB_OPER_OK)
            {
              lobDebugInfo("append(Memory,Buffer) failed ",err,__LINE__,lobGlobals->lobTrace_);
            }
        }
      else if (subOperation == Lob_File)
        {
          err = lobPtr->append(source, -1, subOperation, descNumIn, retOperLen,lobMaxSize, lobMaxChunkMemSize,lobGCLimit,handleIn,handleInLen,handleOut,handleOutLen,transId,lobGlobals);
          if (err != LOB_OPER_OK)
            {
              lobDebugInfo("append(File) failed ",err,__LINE__,lobGlobals->lobTrace_);
            }
        }
      else  
	err = LOB_SUBOPER_ERROR;
      break;

    case Lob_Update:
      if ((subOperation == Lob_Memory)||(subOperation == Lob_Buffer)||(subOperation ==Lob_External_File))
        {
          err = lobPtr->update(source, sourceLen, subOperation, descNumIn, retOperLen, lobMaxSize, lobMaxChunkMemSize,lobGCLimit,handleIn,handleInLen,handleOut,handleOutLen,transId, lobGlobals);
          if (err != LOB_OPER_OK)
            {
              lobDebugInfo("update(Memory,Buffer) failed ",err,__LINE__,lobGlobals->lobTrace_);
            }
        }
      else if (subOperation == Lob_File)
        {
          err = lobPtr->update(source, -1, subOperation,descNumIn, retOperLen,lobMaxSize, lobMaxChunkMemSize,lobGCLimit,handleIn,handleInLen,handleOut,handleOutLen,transId,lobGlobals); 
          if (err != LOB_OPER_OK)
            {
              lobDebugInfo("update(Memory,Buffer) failed ",err,__LINE__,lobGlobals->lobTrace_);
            }
        }
      else
	err = LOB_SUBOPER_ERROR;
      break;

    case Lob_Delete:
      err = lobPtr->delDesc(handleIn, handleInLen,transId);
      break;

    case Lob_Drop:
      err = lobPtr->purgeLob();
      it = lobMap->find(string(lobName));
      lobMap->erase(it);
      NADELETE(lobPtr, ExLob,lobGlobals->getHeap()) ;
      lobPtr = NULL;
      if (err != LOB_OPER_OK)           
        lobDebugInfo("purgeLob failed ",err,__LINE__,lobGlobals->lobTrace_);
      break;

    case Lob_Purge:
      err = lobPtr->purgeLob();
      it = lobMap->find(string(lobName));
      lobMap->erase(it);
      NADELETE(lobPtr, ExLob,lobGlobals->getHeap()) ;
      lobPtr = NULL;
      if (err != LOB_OPER_OK)           
        lobDebugInfo("purgeLob failed ",err,__LINE__,lobGlobals->lobTrace_);
      break;

    case Lob_Empty_Directory:    
      err = lobPtr->emptyDirectory(lobStorageLocation, lobGlobals);

      break;

    case Lob_Data_Mod_Check:
      {       
        Int64 inputModTS = *(Int64*)blackBox;
        Int32 inputNumOfPartLevels = 
          *(Lng32*)&((char*)blackBox)[sizeof(inputModTS)];
        Int32 * failedLocBufLen = 
          (Int32*)&((char*)blackBox)[sizeof(inputModTS)+
                                     sizeof(inputNumOfPartLevels)];
        char * failedLocBuf = &((char*)blackBox)[sizeof(inputModTS)+
                                                 sizeof(inputNumOfPartLevels)+
                                                 sizeof(*failedLocBufLen)];
        Int64 failedModTS = -1;
        err = 
          lobPtr->dataModCheck(lobStorageLocation, 
                               inputModTS, inputNumOfPartLevels,
                               lobGlobals, failedModTS, 
                               failedLocBuf, failedLocBufLen);
        descNumOut = failedModTS;
      }
      break;

    case Lob_Cleanup:
        NADELETE(lobGlobals,ExLobGlobals, lobGlobals->getHeap());
        break;
     
    case Lob_PerformGC:
      err = lobPtr->compactLobDataFile((ExLobInMemoryDescChunksEntry *)source,sourceLen);
      if (err != LOB_OPER_OK)           
        lobDebugInfo("compactLobDataFile failed ",err,__LINE__,lobGlobals->lobTrace_);
      break;
    case Lob_RestoreLobDataFile:
      err = lobPtr->restoreLobDataFile();
      if (err != LOB_OPER_OK)           
        lobDebugInfo("restoreLobDataFile failed ",err,__LINE__,lobGlobals->lobTrace_);
      break;
    case Lob_PurgeBackupLobDataFile:
      err = lobPtr->purgeBackupLobDataFile();
      if (err != LOB_OPER_OK)           
        lobDebugInfo("purgeBackupLobDataFile failed ",err,__LINE__,lobGlobals->lobTrace_);
      break;
    default:
      err = LOB_OPER_ERROR;
      break;
    }
  /*
//**Note ** This code is needed to reinstate the master transaction after 
// returning from the mxlobsrvr process. This is inactive code for now
if (!lobGlobals->isHive() )
    {
      if (lobPtr)
       // set the pass thru request object values from the lob
       lobPtr->getRequest()->getValues(descNumOut, handleOutLen, handleOut, 
                                       requestStatus, cliError,
                                       (char *)blackBox, blackBoxLen);    // reinstate the transaction
      if (TRANSID_IS_VALID(transIdBig)) {
       ms_transid_reinstate(transIdBig, transStartId);
      }
    }

  */
  clock_gettime(CLOCK_MONOTONIC, &endTime);
  secs = endTime.tv_sec - startTime.tv_sec;
  nsecs = endTime.tv_nsec - startTime.tv_nsec;
  if (nsecs < 0) {
    secs--;
    nsecs += NUM_NSECS_IN_SEC;
  }
  totalnsecs = (secs * NUM_NSECS_IN_SEC) + nsecs;
/*
  if (lobPtr && lobPtr->getStats())
    lobPtr->getStats()->hdfsAccessLayerTime += totalnsecs; 
*/
  return err;
}

void cleanupLOBDataDescFiles(const char *lobHdfsServer,int lobHdfsPort,const char *lobHdfsLoc)
{ 
  HDFS_Client_RetCode hdfsClientRetcode  = HdfsClient::hdfsDeletePath(lobHdfsLoc);//ok to ignore error.
  return;
}


// The following methods are used for hive access
/* 
Main thread issues an open to open a range of 128 MB and wakes up a 
worker thread. It doesn’t wait.It calls pre open on the next range. This is 
done in method ::readDataCursorSimple.

The worker threads do their work in ::doWorkInThread and ::performRequests, ::readCursorDataSimple.(note the  diff from the method the mainthread calls above) 

Main thread then issues a read. Since worker thread had already begun fetching 
16KB buffers in (1), the main thread most likely will not need to wait and the 
data will be ready. It keeps consuming the buffers, recycling them back into 
postFetchBufList. 
When done, the main thread closes the cursor(::closeDataCursorSimple). This is determined by whether we 
have reached the end of range or the end of data for that file.
The worker threads on the other hand  read 16KB of data and buffers them in a 
prefetchBufList. It continues doing this until end of range is reached or the 
buffer limit (128MB) has been reached. 
*/

Ex_Lob_Error ExLob::readDataCursorSimple(const char *file, char *tgt, Int64 tgtSize, 
                                         Int64 &operLen, ExLobGlobals *lobGlobals)
{
    int dataOffset;
    Ex_Lob_Error result = LOB_OPER_OK;
    cursor_t *cursor;
    ExLobCursor::bufferList_t::iterator c_it;
    ExLobCursorBuffer *buf = NULL;
    Int64 bytesToCopy = 0;
    operLen = 0;
    Int64 len;
    char *target = tgt;
    bool done = false;

    struct timespec startTime;
    struct timespec endTime;

    lobCursorLock_.lock();

    lobCursors_it it = lobCursors_.find(string(file, strlen(file)));

    if (it == lobCursors_.end())
    {
       lobCursorLock_.unlock();
       return LOB_CURSOR_NOT_OPEN;
    }
    else
    {
       cursor = &(it->second); 
    } 

    lobCursorLock_.unlock();

    while ((operLen < tgtSize) && !done && !cursor->eol_)
    {
      lobGlobals->traceMessage("locking cursor",cursor,__LINE__);
      cursor->lock_.lock();

      // if no buffers to read and is eor or eod, we are done.
      // else wait for prefetch thread to wake us up. 
      if (cursor->prefetchBufList_.size() == 0) {
        if (cursor->eor_ || cursor->eod_) {
          done = true;
        } else {
          cursor->bufferMisses_++;
	  lobGlobals->traceMessage("wait on condition cursor",cursor,__LINE__);
          cursor->lock_.wait();
        }
	lobGlobals->traceMessage("unlocking cursor",cursor,__LINE__);
        cursor->lock_.unlock();
        continue;
      } 

      // a buffer is available
      c_it = cursor->prefetchBufList_.begin();
      buf = *c_it;
      lobGlobals->traceMessage("unlocking cursor",cursor,__LINE__);
      cursor->lock_.unlock();

      bytesToCopy = min(buf->bytesRemaining_, tgtSize - operLen);
      memcpy(target, buf->data_ + buf->bytesUsed_, bytesToCopy);
      target += bytesToCopy;
      if (bytesToCopy == buf->bytesRemaining_) { // buffer is now empty
        buf->bytesRemaining_ = -1;
        buf->bytesUsed_ = -1;
        lobGlobals->postfetchBufListLock_.lock();
        lobGlobals->postfetchBufList_.push_back(buf);
        lobGlobals->postfetchBufListLock_.unlock();
	lobGlobals->traceMessage("locking cursor",cursor,__LINE__);
        cursor->lock_.lock();
        c_it = cursor->prefetchBufList_.erase(c_it);
	lobGlobals->traceMessage("signal condition cursor",cursor,__LINE__);
        cursor->lock_.wakeOne(); // wake up prefetch thread if it was waiting for an empty buffer.
	lobGlobals->traceMessage("unlocking cursor",cursor,__LINE__);
        cursor->lock_.unlock();
      } else {
        buf->bytesUsed_ += bytesToCopy;
        buf->bytesRemaining_ -= bytesToCopy;
      }
      //stats_.bytesPrefetched += bytesToCopy;
      operLen += bytesToCopy;
    } 
/*
    // update stats
    stats_.bytesRead += operLen;
    stats_.bytesToRead += tgtSize;
    stats_.numReadReqs++;
*/
    return LOB_OPER_OK;
}

Ex_Lob_Error ExLob::closeDataCursorSimple(const char *fileName, ExLobGlobals *lobGlobals)
{
    cursor_t *cursor = NULL;
    Int64 secs = 0;
    Int64 nsecs = 0;

    lobCursorLock_.lock();

    lobCursors_it it = lobCursors_.find(string(fileName, strlen(fileName)));
    if (it != lobCursors_.end())
    {
      cursor = &(it->second);
      lobGlobals->traceMessage("locking cursor",cursor,__LINE__);
      cursor->lock_.lock();

      clock_gettime(CLOCK_MONOTONIC, &cursor->closeTime_);
      secs = cursor->closeTime_.tv_sec - cursor->openTime_.tv_sec;
      nsecs = cursor->closeTime_.tv_nsec - cursor->openTime_.tv_nsec;

      if (cursor->eod_ || cursor->eor_) { // prefetch thread already done,
        cursor->emptyPrefetchList(lobGlobals);
	lobGlobals->traceMessage("unlocking cursor",cursor,__LINE__);
	cursor->lock_.unlock();
        lobCursors_.erase(it);            // so erase it here. 
        // no need to unlock as cursor object is gone.
      } else {
        cursor->eol_ = true;     // prefetch thread will do the eol rituals
	lobGlobals->traceMessage("signal condition cursor",cursor,__LINE__);
        cursor->lock_.wakeOne(); // wakeup prefetch thread
	lobGlobals->traceMessage("unlocking cursor",cursor,__LINE__);
        cursor->lock_.unlock();
      }
    }

    lobCursorLock_.unlock();

    if (nsecs < 0) {
      secs--;
      nsecs += NUM_NSECS_IN_SEC;
    }
    Int64 totalnsecs = (secs * NUM_NSECS_IN_SEC) + nsecs;
    //stats_.cursorElapsedTime += totalnsecs;

    return LOB_OPER_OK;
}


Ex_Lob_Error ExLobGlobals::performRequest(ExLobHdfsRequest *request)
{
  Ex_Lob_Error err = LOB_OPER_OK;
  ExLob *lobPtr;
  ExLobCursorBuffer *buf;
  ExLobCursor *cursor;
  Int64 size;
  NABoolean seenEOR = false;
  NABoolean seenEOD = false;
  ExLobCursor::bufferList_t::iterator c_it;
  Int64 totalBufSize;

  switch (request->reqType_) 
  {
    case Lob_Hdfs_Cursor_Prefetch :
      lobPtr = request->lobPtr_;
      cursor = request->cursor_;
      traceMessage("locking cursor",cursor,__LINE__);
      cursor->lock_.lock();
      while (!cursor->eod_ && !cursor->eor_ && !cursor->eol_) 
      {
        postfetchBufListLock_.lock();
        c_it = postfetchBufList_.begin();
        if (c_it != postfetchBufList_.end()) {
          buf = *c_it;
          postfetchBufList_.erase(c_it);
          postfetchBufListLock_.unlock();
	  traceMessage("unlocking cursor",cursor,__LINE__);
          cursor->lock_.unlock();
        } else { 
          postfetchBufListLock_.unlock();
          // there are no empty buffers. 
          // if prefetch list already has the max, wait for one to free up.
          totalBufSize =  cursor->prefetchBufList_.size() * cursor->bufMaxSize_;
          if (totalBufSize > LOB_CURSOR_PREFETCH_BYTES_MAX) {
	    traceMessage("wait on condition cursor",cursor,__LINE__);
            cursor->lock_.wait();
            char buffer2[2048];
            sprintf(buffer2, "cursor->eod_ %d cursor->eor_ %d "
                             "cursor->eol_ %d", cursor->eod_,
                              cursor->eor_, cursor->eol_);
            traceMessage(buffer2, cursor, __LINE__);
            continue;
          }
          // create a new buffer
	  traceMessage("unlocking cursor",cursor,__LINE__);
          cursor->lock_.unlock();
          buf = new (getHeap()) ExLobCursorBuffer();
          buf->data_ = (char *) (getHeap())->allocateMemory( cursor->bufMaxSize_);
          //lobPtr->stats_.buffersUsed++;
        }
        size = min(cursor->bufMaxSize_, (cursor->maxBytes_ - cursor->bytesRead_));
        if (buf->data_) {
          lobPtr->readCursorDataSimple(buf->data_, size, *cursor, buf->bytesRemaining_);
          buf->bytesUsed_ = 0;
	  traceMessage("locking cursor",cursor,__LINE__);
          cursor->lock_.lock();
          if (size < (cursor->bufMaxSize_)) {
            cursor->eor_ = true;
	    seenEOR = true;
          }
          if (buf->bytesRemaining_) {
            cursor->prefetchBufList_.push_back(buf);
	    traceMessage("signal condition cursor",cursor,__LINE__);
            cursor->lock_.wakeOne();
	    traceMessage("unlocking cursor",cursor,__LINE__);
            cursor->lock_.unlock();
          } else {
            cursor->eod_ = true;
            seenEOD = true;
	    traceMessage("signal condition cursor",cursor,__LINE__);
            cursor->lock_.wakeOne();
	    traceMessage("unlocking cursor",cursor,__LINE__);
            cursor->lock_.unlock();
            postfetchBufListLock_.lock();
            postfetchBufList_.push_back(buf);
            postfetchBufListLock_.unlock();
          }
        } else {
          assert("data_ is null"); 
        }
	// Important! Break and do not access cursor object if we have reached
	// end of data or range.
	// The main thread could have destroyed the cursor 
	// in ::closeDataCursorSimple
	if (seenEOD || seenEOR)
        {
          char buffer2[2048];
          sprintf(buffer2, "seenEOD %d seenEOR %d",
                               seenEOD, seenEOR);
          traceMessage(buffer2, cursor, __LINE__);
          break;
        }
	traceMessage("locking cursor",cursor,__LINE__);
	cursor->lock_.lock();
      } // while

      if (!seenEOD && !seenEOR)
	{
          traceMessage("locking cursor",cursor,__LINE__);
	  cursor->lock_.unlock();
	  if (cursor->eol_) { // never reaches here ??  
	    lobPtr->deleteCursor(cursor->name_.c_str(), this);
	  }
	}
      processPreOpens();
      break;

    default:
      request->error_ = LOB_HDFS_REQUEST_UNKNOWN;
  }

  return LOB_OPER_OK;
}



Ex_Lob_Error ExLob::readCursorDataSimple(char *tgt, Int64 tgtSize, cursor_t &cursor, Int64 &operLen)
{
   ExLobDesc desc;
   Ex_Lob_Error err;
   Int64 bytesAvailable = 0;
   Int64 bytesToCopy = 0;
   Int64 bytesRead = 0;
   operLen = 0;
   tOffset offset; 
   struct timespec startTime; 
   struct timespec endTime;
   bool done = false;

   if (!fdData_) {
      return LOB_CURSOR_NOT_OPEN_ERROR;
   }

   if (cursor.bytesRead_ == -1) {  // starting
      cursor.bytesRead_ = 0;
   }

   clock_gettime(CLOCK_MONOTONIC, &startTime);
   
   while ( (operLen < tgtSize) && !done )
   {
      //offset = cursor.descOffset_ + cursor.bytesRead_;
      bytesToCopy = tgtSize - operLen;
      offset = cursor.descOffset_ + cursor.bytesRead_;

      // gets chunks of 64KB. Uses readDirect internally.
      // bytesRead = hdfsPread(fs_, fdData_, offset, tgt, bytesToCopy);
      bytesRead = hdfsRead(fs_, fdData_, tgt, bytesToCopy);

      //stats_.numHdfsReqs++;

      if (bytesRead == -1) {
         return LOB_DATA_READ_ERROR;
      } else if (bytesRead == 0) {
         done = true; 
      }

      cursor.bytesRead_ += bytesRead;
      operLen += bytesRead;
      tgt += bytesRead;
   }

   clock_gettime(CLOCK_MONOTONIC, &endTime);

   Int64 secs = endTime.tv_sec - startTime.tv_sec;
   Int64 nsecs = endTime.tv_nsec - startTime.tv_nsec;
   if (nsecs < 0) {
    secs--; 
    nsecs += NUM_NSECS_IN_SEC;
   }

   Int64 totalnsecs = (secs * NUM_NSECS_IN_SEC) + nsecs;
   //stats_.CumulativeReadTime += totalnsecs;

   return LOB_OPER_OK;
}

void ExLobCursor::emptyPrefetchList(ExLobGlobals *lobGlobals)
{
    ExLobCursor::bufferList_t::iterator c_it;
    ExLobCursorBuffer *buf = NULL;

    c_it = prefetchBufList_.begin();
    while (c_it != prefetchBufList_.end())
    {
      buf = *c_it;
      lobGlobals->postfetchBufListLock_.lock();
      lobGlobals->postfetchBufList_.push_back(buf);
      lobGlobals->postfetchBufListLock_.unlock();
      c_it = prefetchBufList_.erase(c_it);
    }
}

// Seems like this is currently unused. 
// closeDataCusrorSimple takes care of destroying the cursor.But addign code
// similar to closeDataCursorSimple for correctness in case it is used in future
Ex_Lob_Error ExLob::deleteCursor(const char *cursorName, ExLobGlobals *lobGlobals)
{
    cursor_t *cursor = NULL;

    lobCursorLock_.lock();

    lobCursors_it it = lobCursors_.find(string(cursorName, strlen(cursorName)));
    if (it != lobCursors_.end())
    {
      cursor = &(it->second);
      lobGlobals->traceMessage("locking cursor",cursor,__LINE__);
      cursor->lock_.lock();
      cursor->emptyPrefetchList(lobGlobals);
      lobGlobals->traceMessage("unlocking cursor",cursor,__LINE__);
      cursor->lock_.unlock();
      lobCursors_.erase(it);
    }

    lobCursorLock_.unlock();

    return LOB_OPER_OK;
}

//*** Note - sample code to send and receive  
Ex_Lob_Error ExLob::sendReqToLobServer() 
{
  Ex_Lob_Error err; 
  
  return err;
}

///////////////////////////////////////////////////////////////////////////////
// ExLobGlobals definitions
///////////////////////////////////////////////////////////////////////////////

ExLobGlobals::ExLobGlobals(NAHeap *lobHeap) :
    lobMap_(NULL), 
    fs_(NULL),
    isCliInitialized_(FALSE),
    threadTraceFile_(NULL),
    lobTrace_(FALSE),
    numWorkerThreads_(0),
    heap_(lobHeap),
    useLibHdfs_(FALSE)
{
  //initialize the log file
  if (getenv("TRACE_HDFS_THREAD_ACTIONS"))
    {
      char logFileName[50]= "";
      sprintf(logFileName,"trace_threads.%d",getpid());
      threadTraceFile_ = fopen(logFileName,"a");
    }
  if(getenv("TRACE_LOB_ACTIONS"))
    lobTrace_ = TRUE;
}

ExLobGlobals::~ExLobGlobals()
{
    ExLobCursor::bufferList_t::iterator c_it;
    ExLobCursorBuffer *buf = NULL;

   

    if (numWorkerThreads_ > 0) { 
       for (int i=0; numWorkerThreads_-i > 0 && i < NUM_WORKER_THREADS; i++) {
           QRLogger::log(CAT_SQL_EXE, LL_DEBUG, 0, NULL,  
           "Worker Thread Shutdown Requested %ld ", 
           threadId_[i]);
           enqueueShutdownRequest();
       }
     
       for (int i=0; numWorkerThreads_ > 0 && i < NUM_WORKER_THREADS; i++) {
           pthread_join(threadId_[i], NULL);
           QRLogger::log(CAT_SQL_EXE, LL_DEBUG, 0, NULL,  
           "Worker Thread Completed %ld ", 
           threadId_[i]);
           numWorkerThreads_--;
       }
    }

    //Free the preOpenList AFTER the worker threads have left to avoid the 
    //case where a slow worker thread is still processing a preOpen and 
    //may access the preOpenList.
  
   
    preOpenListLock_.lock();
    ExLobPreOpen *po = NULL;
    preOpenList_t::iterator p_it;
    p_it = preOpenList_.begin();
    while (p_it != preOpenList_.end()) 
      {
        po = *p_it;
        NADELETE(po,ExLobPreOpen,heap_);
        p_it = preOpenList_.erase(p_it);
      }
       
        
    preOpenListLock_.unlock();
        
      
    //Free the request list 
    ExLobHdfsRequest *request;
    reqList_t::iterator it;
   
    reqQueueLock_.lock();
    it = reqQueue_.begin();
    while (it != reqQueue_.end())
      {
        request = *it;
        NADELETE(request,ExLobHdfsRequest,heap_);
        it = reqQueue_.erase(it);
      }       
    reqQueueLock_.unlock();
      
    // Free the post fetch bugf list AFTER the worker threads have left to 
    // avoid slow worker thread being stuck and master deallocating these 
    // buffers and not consuming the buffers which could cause a  lock.
 
    postfetchBufListLock_.lock();
    c_it = postfetchBufList_.begin();
    while (c_it != postfetchBufList_.end()) {
      buf = *c_it;
      if (buf->data_) {
        heap_->deallocateMemory( buf->data_);
      }
      c_it = postfetchBufList_.erase(c_it);
    }
    postfetchBufListLock_.unlock();

    //delete the lobMap AFTER the worker threads have finished their pending 
    //work since they may still be using an objetc that was fetched off the lobMap_
    if (lobMap_) 
    {
        lobMap_it it2; 
        for (it2 = lobMap_->begin(); it2 != lobMap_->end() ; ++it2)
        {
           ExLob *lobPtr = it2->second; 
           NADELETE(lobPtr, ExLob, heap_);
        } 
        lobMap_->clear();
        NADELETE(lobMap_,lobMap_t,heap_);
        lobMap_ = NULL;
    }
    
    //msg_mon_close_process(&serverPhandle);
    if (threadTraceFile_)
      fclose(threadTraceFile_);
    threadTraceFile_ = NULL;
}



// called once per process
Ex_Lob_Error ExLobGlobals::initialize()
{
    Ex_Lob_Error err = LOB_OPER_OK;

    lobMap_ = (lobMap_t *) new (getHeap())lobMap_t;  
    if (lobMap_ == NULL)
      return LOB_INIT_ERROR;

    return err;
}

static void *workerThreadMain(void *arg)
{
   // parameter passed to the thread is an instance of the ExLobHdfs object
   ExLobGlobals *glob = (ExLobGlobals *)arg;

   glob->doWorkInThread();

   return NULL;
}

Ex_Lob_Error ExLobGlobals::startWorkerThreads()
{
   int rc;
   for (int i=0; i<NUM_WORKER_THREADS; i++) {
     rc = pthread_create(&threadId_[i], NULL, workerThreadMain, this);
     if (rc != 0)
      return LOB_HDFS_THREAD_CREATE_ERROR;
      QRLogger::log(CAT_SQL_EXE, LL_DEBUG, 0, NULL,  
           "Worker Thread Created %ld ",
           threadId_[i]);
     numWorkerThreads_++;
   }
   
   return LOB_OPER_OK;
}



///////////////////////////////////////////////////////////////////////////////
// ExLobHdfs definitions
///////////////////////////////////////////////////////////////////////////////

ExLobLock::ExLobLock()
    : bellRang_(false),
      waiters_(0)
{
   pthread_mutexattr_t mutexAttr;
   pthread_mutexattr_init( &mutexAttr );
   pthread_mutex_init( &mutex_, &mutexAttr );
   pthread_cond_init( &workBell_, NULL );
}

ExLobLock::~ExLobLock()
{
   pthread_mutex_unlock( &mutex_ );
   pthread_mutex_destroy(&mutex_);
   pthread_cond_destroy(&workBell_);
}

void ExLobLock::lock()
{
   pthread_mutex_lock( &mutex_ );
}

void ExLobLock::unlock()
{
   pthread_mutex_unlock( &mutex_ );
}

void ExLobLock::wakeOne()
{
   pthread_cond_signal(&workBell_);
}

void ExLobLock::wakeAll()
{
   pthread_cond_broadcast(&workBell_);
}

void ExLobLock::wait()
{
    waiters_++;
    pthread_cond_wait(&workBell_, &mutex_);
    waiters_--;
}

ExLobHdfsRequest::ExLobHdfsRequest(LobsHdfsRequestType reqType, ExLobCursor *cursor) :
   reqType_(reqType),
   cursor_(cursor)
{
  buffer_=0;
  lobPtr_=0;
  size_=0;
  error_=LOB_OPER_OK;
}

ExLobHdfsRequest::ExLobHdfsRequest(LobsHdfsRequestType reqType, ExLob *lobPtr, ExLobCursor *cursor) :
   reqType_(reqType),
   lobPtr_(lobPtr),
   cursor_(cursor)
{
  buffer_=0;
  size_=0;
  error_=LOB_OPER_OK;
}

ExLobHdfsRequest::ExLobHdfsRequest(LobsHdfsRequestType reqType) :
   reqType_(reqType)
{

  buffer_=0;
  cursor_=0;
  lobPtr_=0;
  size_=0;
  error_=LOB_OPER_OK;
}

ExLobHdfsRequest::~ExLobHdfsRequest()
{
}

Ex_Lob_Error ExLobGlobals::enqueueRequest(ExLobHdfsRequest *request)
{
   char buffer2[2048];
   sprintf(buffer2, "enqueue request %d", request->reqType_);
   traceMessage(buffer2, NULL, __LINE__);
   reqQueueLock_.lock();
   reqQueue_.push_back(request);
   reqQueueLock_.wakeOne();
   reqQueueLock_.unlock();

   return LOB_OPER_OK;
}

Ex_Lob_Error ExLobGlobals::enqueuePrefetchRequest(ExLob *lobPtr, ExLobCursor *cursor)
{
  ExLobHdfsRequest *request = new  (heap_) ExLobHdfsRequest(Lob_Hdfs_Cursor_Prefetch, lobPtr, cursor);
   
   if (!request) {
     // return error
   }

   enqueueRequest(request);

   return LOB_OPER_OK;
}

Ex_Lob_Error ExLobGlobals::enqueueShutdownRequest()
{
  ExLobHdfsRequest *request = new (heap_) ExLobHdfsRequest(Lob_Hdfs_Shutdown);
   
   if (!request) {
     // return error
   }

   enqueueRequest(request);

   return LOB_OPER_OK;
}


ExLobHdfsRequest* ExLobGlobals::getHdfsRequest()
{
   ExLobHdfsRequest *request;
   reqList_t::iterator it;

   reqQueueLock_.lock();
   it = reqQueue_.begin();

   request = NULL;

   while(request == NULL) 
   {
      if (it != reqQueue_.end()) 
      {
         request = *it;
         it = reqQueue_.erase(it);
      } else {
         reqQueueLock_.wait();
         it = reqQueue_.begin();
      }
   }

   reqQueueLock_.unlock();
   char buffer2[2048];
   sprintf(buffer2, "got request %d", request->reqType_);
   traceMessage(buffer2, NULL, __LINE__);
   return request;
}

void ExLobGlobals::doWorkInThread()
{
   ExLobHdfsRequest *request;

   // mask all signals
   struct sigaction act;
   sigemptyset(&act.sa_mask);

   sigset_t mask;
   sigfillset(&mask);
   int rc = pthread_sigmask(SIG_SETMASK, &mask, NULL);
   if (rc != 0) {
      return;
   }

   // enter processing zone
   for (;;) 
   {
      request = getHdfsRequest(); // will wait until new req arrives

      if (request->isShutDown()) { 
	//we are asked to shutdown
	//wake up next worker before going away
	reqQueueLock_.lock();
	reqQueueLock_.wakeOne();
	reqQueueLock_.unlock();
	break; 

      }
      else {
         performRequest(request);
         NADELETE(request, ExLobHdfsRequest, heap_);
      }
   }

   pthread_exit(0);
}

Ex_Lob_Error ExLobGlobals::addToPreOpenList(ExLobPreOpen *preOpenObj)
{
    preOpenListLock_.lock();
    preOpenList_.push_back(preOpenObj);
    preOpenListLock_.unlock();
    return LOB_OPER_OK;
}

Ex_Lob_Error ExLobGlobals::processPreOpens() 
{
    ExLobPreOpen *preOpenObj = NULL; 
    preOpenList_t::iterator p_it;

    preOpenListLock_.lock();
    if (!preOpenList_.empty()) 
    {
        p_it = preOpenList_.begin();
        preOpenObj = *p_it;
        preOpenList_.erase(p_it);
    }
    preOpenListLock_.unlock();

    if (preOpenObj != NULL) 
    {
        ExLob *lobPtr = preOpenObj->lobPtr_;

        lobPtr->openDataCursor(preOpenObj->cursorName_.data(), Lob_Cursor_Simple, preOpenObj->range_, 
                               preOpenObj->bufMaxSize_, preOpenObj->maxBytes_, 
                               preOpenObj->waited_, this,0);
    }

    return LOB_OPER_OK;
}

//Enable envvar TRACE_HDFS_THREAD_ACTIONS to enable tracing. 
//The output file will be named trace_threads.<pid> on ech node

void ExLobGlobals::traceMessage(const char *logMessage, ExLobCursor *cursor,
                                int line)
{
  if ( threadTraceFile_ && logMessage)
  {
    fprintf(threadTraceFile_, 
    "Thread: 0x%lx Line:  %d %s 0x%lx\n" ,
       (unsigned long)pthread_self(), line, logMessage, 
       (unsigned long) cursor);
    fflush(threadTraceFile_);
  }
    
}

//Enable envvar TRACE_LOB_ACTIONS to enable tracing. 
//The output file will be in 
//$TRAF_LOG directory on each node

void lobDebugInfo(const char *logMessage,Int32 errorcode,
                         Int32 line, NABoolean lobTrace)
{
  if ( lobTrace) 
    {
      NAString logString("LOB : ");
      logString += logMessage;
      
      SQLMXLoggingArea::logSQLMXDebugEvent(logString.data(),(short)errorcode,line);
    }
  
    
}

// ExLobRequest definitions
///////////////////////////////////////////////////////////////////////////////

ExLobRequest::ExLobRequest() :
    reqNum_(0),
    descNumIn_(-1),
    descNumOut_(-1),
    handleInLen_(-1),
    handleOutLen_(-1),
    dataOffset_(-1),
    type_(Lob_Req_Invalid),
    storage_(Lob_Invalid_Storage),
    operLen_(-1),
    error_(LOB_INVALID_ERROR_VAL),
    cliError_(-1),
    status_(LOB_INVALID_ERROR_VAL),
    transId_(0)
{
   TRANSID_SET_NULL(transIdBig_);
}
void ExLobRequest::setValues(char *descFileName, Int64 descNumIn, Int64 handleInLen, 
                             char *handleIn, LobsStorage storage, Int64 transId,
                             SB_Transid_Type transIdBig,
                             SB_Transseq_Type transStartId,
                             char *blackBox, Int64 blackBoxLen)
{
  
    descNumIn_ = descNumIn;
    handleInLen_ = handleInLen;
    storage_ = storage;
    strcpy(descFileName_, descFileName);
    if (handleIn != NULL && handleInLen > 0) {
       memcpy(handleIn_, handleIn, handleInLen);
    }
    cliError_ = -1;
    error_ = LOB_INVALID_ERROR_VAL;
    status_ = LOB_INVALID_ERROR_VAL;

    transId_ = transId;
    transIdBig_ = transIdBig;
    transStartId_ = transStartId;
    blackBoxLen_ = blackBoxLen;
    if (blackBox != NULL && blackBoxLen > 0) {
       memcpy(blackBox_, blackBox, blackBoxLen);
    }
    
}

void ExLobRequest::getValues(Int64 &descNumOut, Int64 &handleOutLen, 
                             char *handleOut, Ex_Lob_Error &requestStatus, 
                             Int64 &cliError,
                             char *blackBox, Int64 &blackBoxLen)
{
  
    descNumOut = descNumOut_;
    handleOutLen = handleOutLen_;
    requestStatus = error_;
    cliError = cliError_;
    if (handleOut != NULL && handleOutLen_ > 0) {
       memcpy(handleOut, handleOut_, handleOutLen_);
    }
    blackBoxLen = blackBoxLen_;
    if (blackBox != NULL && blackBoxLen_ > 0) {
       memcpy(blackBox, blackBox_, blackBoxLen_);
    }
    // #endif
}

ExLobRequest::~ExLobRequest()
{
}

Ex_Lob_Error ExLobRequest::send()
{
 

    int msgid; 
    int oid;
    MS_Result_Type result;
    short req_ctrl[BUFSIZ];
    short rep_ctrl[BUFSIZ];
    char *req_data = (char *)this;
    ExLobRequest rep_data;
    short req_data_len = sizeof(ExLobRequest);
    short rep_data_max = sizeof(ExLobRequest);
    int err=0;
    int inx=0;
    int retries = 3;

    incrReqNum();

    status_ = LOB_OPER_REQ_IN_PROGRESS;

    do 
    {
       err = BMSG_LINK_(&serverPhandle,
                        &msgid, 
                        req_ctrl,
                        (ushort) (inx &1),
                        rep_ctrl,
                        1,
                        req_data,
                        req_data_len,
                        (char *)&rep_data, 
                        rep_data_max,
                        0,0,0,0); 
       retries--;

       err = BMSG_BREAK_(msgid, (short *) &result, &serverPhandle);

       if (err == -XZFIL_ERR_PATHDOWN) {
        //lobGlobals>resetServerPhandle();
       }

    } while ( (err == XZFIL_ERR_PATHDOWN) && (retries > 0) ); // 201 if lobserver got restared

    status_ = LOB_OPER_REQ_DONE;

    if (err != XZFIL_ERR_OK)
      return LOB_SEND_MSG_ERROR;

    memcpy(this, &rep_data, rep_data_max);
  
    return LOB_OPER_OK;
}
