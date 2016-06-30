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
#include "NAVersionedObject.h"
#include "ComQueue.h"

#include "NAMemory.h"
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

ExLob::ExLob() :
    storage_(Lob_Invalid_Storage),
    dir_(string()),
    lobGlobalHeap_(NULL),
    fs_(NULL),
    fdData_(NULL),
    openFlags_(0),
    lobTrace_(FALSE)
{
    lobDataFile_[0] = '\0';
    
}

ExLob::~ExLob()
{
    
    if (fdData_) {
      hdfsCloseFile(fs_, fdData_);
      fdData_ = NULL;
    }
   
}

Ex_Lob_Error ExLob::initialize(char *lobFile, Ex_Lob_Mode mode, 
                               char *dir, 
			       LobsStorage storage,
                               char *hdfsServer, Int64 hdfsPort,
                               char *lobLocation,
                               int bufferSize , short replication ,
                               int blockSize, Int64 lobMaxSize, ExLobGlobals *lobGlobals)
{
  int openFlags;
  mode_t filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  struct timespec startTime;
  struct timespec endTime;
  Int64 secs, nsecs, totalnsecs;
 
  if (dir) 
    {
      if (dir_.empty()) 
	{
	  dir_ = string(dir);
	}

      if (lobFile)
        snprintf(lobDataFile_, MAX_LOB_FILE_NAME_LEN, "%s/%s", dir_.c_str(), 
                 lobFile);
      
    } 
  else 
    { 
      if (lobFile)
        snprintf(lobDataFile_, MAX_LOB_FILE_NAME_LEN, "%s", lobFile);
      
    }

  if (storage_ != Lob_Invalid_Storage) 
    {
      return LOB_INIT_ERROR;
    } else 
    {
      storage_ = storage;
    }

  stats_.init(); 

  hdfsServer_ = hdfsServer;
  hdfsPort_ = hdfsPort;
  lobLocation_ = lobLocation;
  clock_gettime(CLOCK_MONOTONIC, &startTime);

  if (lobGlobals->getHdfsFs() == NULL)
    {
      fs_ = hdfsConnect(hdfsServer_, hdfsPort_);
      if (fs_ == NULL) 
	return LOB_HDFS_CONNECT_ERROR;
      lobGlobals->setHdfsFs(fs_);
    }        
  else 
    {
      fs_ = lobGlobals->getHdfsFs();
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
  stats_.hdfsConnectionTime += totalnsecs;
    
  if (mode == EX_LOB_CREATE) 
    { 
      // check if file is already created
      hdfsFileInfo *fInfo = hdfsGetPathInfo(fs_, lobDataFile_);
      if (fInfo != NULL) 
	{
	  hdfsFreeFileInfo(fInfo, 1);
	  return LOB_DATA_FILE_CREATE_ERROR;
	} 
      openFlags = O_WRONLY | O_CREAT;   
      fdData_ = hdfsOpenFile(fs_, lobDataFile_, openFlags, bufferSize, replication, blockSize);
      if (!fdData_) 
	{
          return LOB_DATA_FILE_CREATE_ERROR;
	}
      hdfsCloseFile(fs_, fdData_);
      fdData_ = NULL;
     
    }
  lobGlobalHeap_ = lobGlobals->getHeap();    
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
        cliErr = SQL_EXEC_LOBcliInterface(handleIn, handleLenIn, 
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
    else
      {
        if (blackBox && blackBoxLen >0 )
          {
            // we have received the external data file name from the descriptor table
            // replace the contents of the lobDataFile with this name 
            str_cpy_and_null(lobDataFile_, blackBox, blackBoxLen,'\0','0',TRUE);
       
          }
        outOffset = offset;
        outSize = size;
      }

    str_sprintf(logBuf, " Returned after ::fetchCursor %Ld,%Ld",outOffset,outSize);
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
   
    str_sprintf(logBuf,"After Cli LOB_CLI_SELECT_UNIQUE:descOffset:%Ld, descSize: %Ld",offset,size);
    lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
    return err;
}


Ex_Lob_Error ExLob::writeData(Int64 offset, char *data, Int32 size, Int64 &operLen)
{ 
    Ex_Lob_Error err;
    lobDebugInfo("In ExLob::writeData",0,__LINE__,lobTrace_);
    if (!fdData_ || (openFlags_ != (O_WRONLY | O_APPEND))) // file is not open for write
    {
      // get file info
      hdfsFileInfo *fInfo = hdfsGetPathInfo(fs_, lobDataFile_);
      if (fInfo == NULL) {
         return LOB_DATA_FILE_NOT_FOUND_ERROR;
      }
    }
     hdfsCloseFile(fs_, fdData_);
     fdData_=NULL;
     openFlags_ = O_WRONLY | O_APPEND; 
     fdData_ = hdfsOpenFile(fs_, lobDataFile_, openFlags_, 0, 0, 0);
     if (!fdData_) {
       openFlags_ = -1;
       return LOB_DATA_FILE_OPEN_ERROR;
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


    if (!fdData_ || (openFlags_ != (O_WRONLY | O_APPEND))) // file is not open for write
    {
      // get file info
      hdfsFileInfo *fInfo = hdfsGetPathInfo(fs_, lobDataFile_);
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
      fdData_ = hdfsOpenFile(fs_, lobDataFile_, openFlags_, bufferSize, replication, blockSize);
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

Ex_Lob_Error ExLob::dataModCheck2(
       char * dirPath, 
       Int64  inputModTS,
       Lng32  numOfPartLevels)
{
  if (numOfPartLevels == 0)
    return LOB_OPER_OK;

  Lng32 currNumFilesInDir = 0;
  hdfsFileInfo * fileInfos = 
    hdfsListDirectory(fs_, dirPath, &currNumFilesInDir);
  if ((currNumFilesInDir > 0) && (fileInfos == NULL))
    {
      return LOB_DATA_FILE_NOT_FOUND_ERROR;
    }

  NABoolean failed = FALSE;
  for (Lng32 i = 0; ((NOT failed) && (i < currNumFilesInDir)); i++)
    {
      hdfsFileInfo &fileInfo = fileInfos[i];
      if (fileInfo.mKind == kObjectKindDirectory)
        {
          Int64 currModTS = fileInfo.mLastMod;
          if ((inputModTS > 0) &&
              (currModTS > inputModTS))
            failed = TRUE;
        }
    }

  hdfsFreeFileInfo(fileInfos, currNumFilesInDir);
  if (failed)
    return LOB_DATA_MOD_CHECK_ERROR;

  numOfPartLevels--;
  Ex_Lob_Error err = LOB_OPER_OK;
  if (numOfPartLevels > 0)
    {
      for (Lng32 i = 0; ((NOT failed) && (i < currNumFilesInDir)); i++)
        {
          hdfsFileInfo &fileInfo = fileInfos[i];
          err = dataModCheck2(fileInfo.mName, inputModTS, numOfPartLevels);
          if (err != LOB_OPER_OK)
            return err;
        }
    }

  return LOB_OPER_OK;
}

// numOfPartLevels: 0, if not partitioned
//                  N, number of partitioning cols
Ex_Lob_Error ExLob::dataModCheck(
       char * dirPath, 
       Int64  inputModTS,
       Lng32  numOfPartLevels,
       ExLobGlobals *lobGlobals)
{
  // find mod time of root dir
  hdfsFileInfo *fileInfos = hdfsGetPathInfo(fs_, dirPath);
  if (fileInfos == NULL)
    {
      hdfsDisconnect(fs_);
      fs_ = hdfsConnect(hdfsServer_, hdfsPort_);
      if (fs_ == NULL)
        return LOB_HDFS_CONNECT_ERROR;

      fileInfos = hdfsGetPathInfo(fs_, dirPath);
      if (fileInfos == NULL)
        return LOB_DIR_NAME_ERROR;

      if (lobGlobals)
        lobGlobals->setHdfsFs(fs_);
    }

  Int64 currModTS = fileInfos[0].mLastMod;
  hdfsFreeFileInfo(fileInfos, 1);
  if ((inputModTS > 0) &&
      (currModTS > inputModTS))
    return LOB_DATA_MOD_CHECK_ERROR;

  if (numOfPartLevels > 0)
    {
      return dataModCheck2(dirPath, inputModTS, numOfPartLevels);
    }

  return LOB_OPER_OK;
}

Ex_Lob_Error ExLob::emptyDirectory(char *dirPath,
                                   ExLobGlobals *lobGlobals)
{
  int retcode = 0;

  hdfsFileInfo *fileInfos = hdfsGetPathInfo(fs_, dirPath);
  if (fileInfos == NULL)
    {
      hdfsDisconnect(fs_);
      fs_ = hdfsConnect(hdfsServer_, hdfsPort_);
      if (fs_ == NULL)
        return LOB_HDFS_CONNECT_ERROR;
      
      fileInfos = hdfsGetPathInfo(fs_, lobDataFile_);
      if (fileInfos == NULL)
        return LOB_DIR_NAME_ERROR;
      
      if (lobGlobals)
        lobGlobals->setHdfsFs(fs_);
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
   if (srcType == HDFS_FILE)
     {
       hdfsFile sourceFile = hdfsOpenFile(fs_,srcfile,O_RDONLY,0,0,0);   
       if (!sourceFile)								
	  return LOB_SOURCE_FILE_OPEN_ERROR;										 
       hdfsFileInfo *sourceFileInfo = hdfsGetPathInfo(fs_,srcfile);
       // get EOD from source hdfs file.
       if (sourceFileInfo)
	 sourceEOF = sourceFileInfo->mSize;
       else
	 return LOB_SOURCE_FILE_OPEN_ERROR;
       
       str_sprintf(logBuf,"Returning EOF of %Ld for file %s", sourceEOF,srcfile);
       lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
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
       str_sprintf(logBuf,"Returning EOF of %Ld for file %s", sourceEOF,srcfile);
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



Ex_Lob_Error ExLob::readSourceFile(char *srcfile, char *&fileData, Int32 &size, Int64 offset)
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
Ex_Lob_Error ExLob::readHdfsSourceFile(char *srcfile, char *&fileData, Int32 &size, Int64 offset)
 {
     char logBuf[4096];
     str_sprintf(logBuf,"Calling ::readHdfsSourceFile: %s Offset:%Ld, Size: %Ld",srcfile, offset,size);
     lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
  
   
     int openFlags = O_RDONLY;
     hdfsFile fdSrcFile = hdfsOpenFile(fs_,srcfile, openFlags,0,0,0);
     if (fdSrcFile == NULL) {
       return LOB_SOURCE_FILE_OPEN_ERROR;
     }

     
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
Ex_Lob_Error ExLob::readLocalSourceFile(char *srcfile, char *&fileData, Int32 &size, Int64 offset)
   {  
     char logBuf[4096];
     str_sprintf(logBuf,"Calling ::readLocalSourceFile: %s Offset:%Ld, Size: %Ld",srcfile, offset,size);
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

Ex_Lob_Error ExLob::readExternalSourceFile(char *srcfile, char *&fileData, Int32 &size,Int64 offset)
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

Ex_Lob_Error ExLob::writeDesc(Int64 &sourceLen, char *source, LobsSubOper subOper, Int64 &descNumOut, Int64 &operLen, Int64 lobMaxSize,Int64 lobMaxChunkMemSize,Int64 lobGCLimit, char * handleIn, Int32 handleInLen, char *blackBox, Int32 *blackBoxLen, char *handleOut, Int32 &handleOutLen, void *lobGlobals)
{
  Ex_Lob_Error err=LOB_OPER_OK; 
    Int64 dataOffset = 0;
    Int64 outDescPartnKey = 0;
    Int64 outDescSyskey = 0;
    Int32 clierr = 0;
    char logBuf[4096];
  
    lobDebugInfo("In ExLob::writeDesc",0,__LINE__,lobTrace_);
    // Calculate sourceLen for each subOper.
    if ((subOper == Lob_File)||(subOper == Lob_External))
      {
	err = statSourceFile(source, sourceLen); 
	if (err != LOB_OPER_OK)
	  return err;
      }
    if (sourceLen <= 0 || sourceLen > lobMaxSize)
      {
	return LOB_MAX_LIMIT_ERROR; //exceeded the size of the max lob size
        //TBD trigger compaction
      }
    if (subOper != Lob_External) 
      {
        lobDebugInfo("Calling ExLob::allocateDesc",0,__LINE__,lobTrace_);
        err = allocateDesc((unsigned int)sourceLen, descNumOut, dataOffset, lobMaxSize, lobMaxChunkMemSize,handleIn, handleInLen, lobGCLimit,lobGlobals);
      }
   
    operLen = 0; 
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
				     0,lobTrace_);
    if (clierr < 0 ) {
      str_sprintf(logBuf,"CLI LOB_CLI_INSERT returned error %d",clierr);
      lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
  
      return LOB_DESC_WRITE_ERROR;
    }
    return err;
}


Ex_Lob_Error ExLob::insertDesc(Int64 offset, Int64 size,  char *handleIn, Int32 handleInLen,  char *handleOut, Int32 &handleOutLen, char *blackBox, Int32 blackBoxLen,void *lobGlobals) 
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
   str_sprintf(logBuf,"Calling Cli LOB_CLI_INSERT: Offset:%Ld, Size: %Ld",
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
				     0,lobTrace_);
   str_sprintf(logBuf,"After LOB_CLI_INSERT: ChunkNum: OutSyskey:%Ld",
               chunkNum,outDescSyskey);
   lobDebugInfo(logBuf,0,__LINE__,lobTrace_);
   lobDebugInfo("Leaving ExLob::InsertDesc",0,__LINE__,lobTrace_);
   if (clierr < 0 ) {
     lobDebugInfo("LOB_CLI_INSERT cli call returned error :",clierr,__LINE__,TRUE);
     return LOB_DESC_WRITE_ERROR;
    }
   return LOB_OPER_OK;
}


Ex_Lob_Error ExLob::writeLobData(char *source, Int64 sourceLen, LobsSubOper subOperation, Int64 tgtOffset,Int64 &operLen, Int64 lobMaxChunkMemSize)
{
    Ex_Lob_Error err=LOB_OPER_OK; 
    char logBuf[4096];
    lobDebugInfo("In ExLob::writeLobData",0,__LINE__,lobTrace_);
    char *inputAddr = source;
    Int64 readOffset = 0;
    Int32 allocMemSize = 0;
    Int64 inputSize = sourceLen;
    Int64 writeOffset = tgtOffset;
    if (subOperation == Lob_External)
      return LOB_OPER_OK;
    while(inputSize > 0)
      {
        allocMemSize = MINOF(lobMaxChunkMemSize, inputSize);
	if (subOperation == Lob_File) 
	  {
            str_sprintf(logBuf,"reading source file %s allocMemSize : %Ld, readOffset:%Ld", source,allocMemSize,readOffset);
            lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
	    err = readSourceFile(source, inputAddr, allocMemSize, readOffset);
	    if (err != LOB_OPER_OK)
              {
                lobDebugInfo("readSouceFile returned an error",0,__LINE__,lobTrace_);
                return err;    
              } 
	  }    
	else 
	  { // in memory
	   
	  }
	err = writeData(writeOffset, inputAddr, allocMemSize, operLen);
	if (err != LOB_OPER_OK)
	  {
            str_sprintf(logBuf,"::writeData returned error .writeOffset:%Ld, allocMemSize:%Ld, operLen %Ld ", writeOffset,allocMemSize,operLen);
            lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
	    //handle errors that happen in one of the chunks.
	   return err;
	  }
	if (subOperation == Lob_File) {
	  writeOffset = writeOffset+allocMemSize;
	  readOffset = readOffset+allocMemSize;
	  inputSize = inputSize-lobMaxChunkMemSize;
	  getLobGlobalHeap()->deallocateMemory(inputAddr);
          str_sprintf(logBuf,"Bookkeeping for Lob_File source.writeOffset:%Ld, readOffset:%Ld, inputSize: %Ld ", writeOffset,readOffset, inputSize);
          lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
	}
	else
	  {
	    writeOffset = writeOffset+allocMemSize;
	    inputSize = inputSize-lobMaxChunkMemSize;
	    inputAddr = inputAddr+allocMemSize;
            str_sprintf(logBuf,"Bookkeeping for Lob_Memory source. writeOffset:%Ld,  inputSize: %Ld ", writeOffset, inputSize);
            lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
	  }
      }
    lobDebugInfo("Leaving ExLob::writeLobData",0,__LINE__,lobTrace_);	
    hdfsCloseFile(fs_, fdData_);
    fdData_=NULL;
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
   if (blackBox)
     {
       
       // we have received the external data file name from the descriptor table
       // replace the contents of the lobDataFile with this name 
       str_cpy_and_null(lobDataFile_, blackBox, blackBoxLen,'\0','0',TRUE);
       
     }
   if (blackBoxLen == -1)
     {
       lobDebugInfo("Reading multiple chunks",0,__LINE__,lobTrace_);
       sizeToRead = size;
       multipleChunks = TRUE;
     }
   str_sprintf(logBuf,"sizeToRead:%Ld, desc.size :%Ld", sizeToRead, desc.getSize());
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
        str_cpy_and_null(lobDataFile_, blackBox, blackBoxLen,'\0','0',TRUE);
       
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

Ex_Lob_Error ExLob::append(char *data, Int64 size, LobsSubOper so, Int64 headDescNum, Int64 &operLen, Int64 lobMaxSize,Int64 lobMaxChunkMemSize, Int64 lobGCLimit, char *handleIn, Int32 handleInLen,  char * handleOut, Int32 &handleOutLen,void *lobGlobals)
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

    if (so ==Lob_External)
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
    err = allocateDesc((unsigned int)sourceLen, dummyParam, dataOffset, lobMaxSize,lobMaxChunkMemSize,handleIn, handleInLen,lobGCLimit,lobGlobals);
    if (err != LOB_OPER_OK)
      return err;

    lobDebugInfo("Calling cli LOB_CLI_INSERT_APPEND",0,__LINE__,lobTrace_);
    clierr = SQL_EXEC_LOBcliInterface(handleIn, handleInLen, 
				      blackBox, &blackBoxLen,
                                      handleOut, &handleOutLen,
                                      LOB_CLI_INSERT_APPEND, LOB_CLI_ExecImmed,
                                      &dataOffset, &sourceLen,
                                      &outDescPartnKey, &outDescSyskey, 
				      0,
				      0,lobTrace_);
    
    
    if (clierr < 0 || clierr == 100) { // some error or EOD.
      str_sprintf(logBuf,"cli LOB_CLI_INSERT_APPEND returned :%d", clierr);
      lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
      return LOB_DESC_APPEND_ERROR;
    }

    char *inputAddr = data;
    if (so == Lob_Buffer)
      {
	inputAddr = (char *)(*(long *)data);
      }
     str_sprintf(logBuf,"Calling writeLobData: inputAddr: %Ld, InputSize%Ld, tgtOffset:%Ld",(long)inputAddr,sourceLen,dataOffset);
    err = writeLobData(inputAddr, sourceLen,so,dataOffset,operLen,lobMaxChunkMemSize);
    if (err != LOB_OPER_OK)
      {
        lobDebugInfo("writeLobData returned error",0,__LINE__,lobTrace_);
        return err;
      }
    return LOB_OPER_OK;
}
Ex_Lob_Error ExLob::insertData(char *data, Int64 size, LobsSubOper so,Int64 headDescNum, Int64 &operLen, Int64 lobMaxSize, Int64 lobMaxChunkMemSize,char * handleIn, Int32 handleInLen, char *blackBox, Int32 blackBoxLen, char * handleOut, Int32 &handleOutLen, void *lobGlobals)
{
   Ex_Lob_Error err=LOB_OPER_OK; 
   ExLobDesc desc;
   int clierr = 0;
   operLen = 0;
   char logBuf[4096];
   lobDebugInfo("In ExLob::InsertData",0,__LINE__,lobTrace_);
   str_sprintf(logBuf,"data:%Ld, size %Ld, lobMaxSize:%Ld, lobMaxChunkMemSize:%Ld", (long)data, size,lobMaxSize,lobMaxChunkMemSize);
  
   // get offset and input size from desc (the one that was just  
   // inserted into the descriptor handle table)
   
   err = getDesc(desc,handleIn,handleInLen,blackBox, &blackBoxLen,handleOut,handleOutLen,0);
     
    if (err !=LOB_OPER_OK) { // some error or EOD.
       lobDebugInfo("getDesc returned error",0,__LINE__,lobTrace_);
       return LOB_DESC_READ_ERROR;
    }
    
    if ((data == NULL)) { 
       return LOB_SOURCE_DATA_ERROR;
    }

    char *inputAddr = data;
    if (so == Lob_Buffer)
      {
	inputAddr = (char *)(*(long *)data);
      }
    Int64 inputSize = desc.getSize();
    Int64 tgtOffset = desc.getOffset();
    str_sprintf(logBuf,"Calling writeLobData: inputAddr: %Ld, InputSize%Ld, tgtOffset:%Ld",(long)inputAddr,inputSize,tgtOffset);

    lobDebugInfo(logBuf,0,__LINE__,lobTrace_);
    err = writeLobData(inputAddr, inputSize,so, tgtOffset, 
		       operLen,lobMaxChunkMemSize);
    if (err != LOB_OPER_OK){
      lobDebugInfo("writeLobData returned error",0,__LINE__,lobTrace_);
      return err;
    }
    return LOB_OPER_OK;
}
Ex_Lob_Error ExLob::update(char *data, Int64 size, LobsSubOper so,Int64 headDescNum, Int64 &operLen, Int64 lobMaxSize, Int64 lobMaxChunkMemSize, Int64 lobGCLimit, char *handleIn, Int32 handleInLen,  char *handleOut, Int32 &handleOutLen, void *lobGlobals)
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

    if (so == Lob_External)
      {
        blackBox = data;
        blackBoxLen = (Int32)size;
      }
    lobDebugInfo("In ExLob::update",0,__LINE__,lobTrace_);
    if ((so == Lob_File) || (so == Lob_External))
      {
        str_sprintf(logBuf,"Calling statSourceFile: source:%s, sourceLen: %Ld",
               data,sourceLen);
        lobDebugInfo(logBuf, 0,__LINE__,lobTrace_);
	err = statSourceFile(data, sourceLen); 
	if (err != LOB_OPER_OK)
	  return err;
      }
    if(so != Lob_External)
      {
        if (sourceLen <= 0 || sourceLen > lobMaxSize)
          {
            return LOB_MAX_LIMIT_ERROR; //exceeded the size of the max lob size
          }
        lobDebugInfo("Calling allocateDesc",0,__LINE__,lobTrace_);
        err = allocateDesc((unsigned int)sourceLen, dummyParam, dataOffset, lobMaxSize, lobMaxChunkMemSize, handleIn, handleInLen, lobGCLimit,lobGlobals);
        if (err != LOB_OPER_OK)
          return err;
      }
    lobDebugInfo("Calling CLI LOB_CLI_UPDATE_UNIQUE",0,__LINE__,lobTrace_);
    clierr = SQL_EXEC_LOBcliInterface(handleIn, 
                                      handleInLen, 
				      blackBox, &blackBoxLen,
                                      handleOut, &handleOutLen,
                                      LOB_CLI_UPDATE_UNIQUE, LOB_CLI_ExecImmed,
                                      &dataOffset, &sourceLen,
                                      &outDescPartnKey, &outDescSyskey, 
				      0,
				      0,lobTrace_);
    
    if (clierr < 0 || clierr == 100) { // some error or EOD.
      
       return LOB_DESC_UPDATE_ERROR;
    }
    char *inputAddr = data;
    if (so == Lob_Buffer)
      {
	inputAddr = (char *)(*(long *)data);
      }
    str_sprintf(logBuf,"Calling writeLobData.sourceLen:%Ld, dataOffset:%Ld",sourceLen,dataOffset);
    lobDebugInfo(logBuf,0,__LINE__,lobTrace_);
           
    err = writeLobData(inputAddr, sourceLen,so,dataOffset,operLen,lobMaxChunkMemSize);
    str_sprintf(logBuf,"writeLobData returned. operLen:%Ld",operLen);
    lobDebugInfo(logBuf,0,__LINE__,lobTrace_);
    if (err != LOB_OPER_OK){
       lobDebugInfo("writeLobData Failed",0,__LINE__,lobTrace_); 
       return err;
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
     if (hdfsDelete(fs_, lobDataFile_, 0) != 0)
       {
         str_sprintf(logBuf,"hdfsDelete of %s returned error",lobDataFile_);
         lobDebugInfo(lobDataFile_,0,__LINE__,lobTrace_);
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

Ex_Lob_Error ExLob::openDataCursor(char *file, LobsCursorType type, Int64 range, Int64 bufMaxSize, 
                                   Int64 maxBytes, Int64 waited, ExLobGlobals *lobGlobals)
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
    strcpy(cursor.name_, file);

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
      fdData_ = hdfsOpenFile(fs_, lobDataFile_, openFlags_, 0, 0, 0);
      if (!fdData_) {
        openFlags_ = -1;
        lobCursorLock_.unlock();
        return LOB_DATA_FILE_OPEN_ERROR;
      }
    }

    if (hdfsSeek(fs_, fdData_, (it->second).descOffset_) == -1) {
      lobCursorLock_.unlock();
      return LOB_DATA_FILE_POSITION_ERROR;
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
    str_sprintf(logBuf,"ExLob::readCursor:: cliInterface:%Ld,bytesRead_:%Ld,descOffset_:%LddescSize_:%Ld,eod_:%d,eor_:%d,eol_:%d,",(long)cursor.cliInterface_,cursor.bytesRead_,cursor.descOffset_,cursor.descSize_,cursor.eod_,cursor.eor_,cursor.eol_);
    lobDebugInfo(logBuf,0,__LINE__,lobTrace_);
    if (cursor.eod_) {
       // remove cursor from the map.
       // server has already closed the cursor. 
       closeCursor(handleIn, handleInLen); 
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




Ex_Lob_Error ExLob::closeCursor(char *handleIn, Int32 handleInLen)
{
  char logBuf[4096];
  lobCursors_it it = lobCursors_.find(string(handleIn, handleInLen));
  if (it != lobCursors_.end())
    {
      str_sprintf(logBuf,"closing cursor for handle");
      lobDebugInfo(logBuf,0,__LINE__,lobTrace_);    
      lobCursors_.erase(it);
    }
    return LOB_OPER_OK;
}


Ex_Lob_Error ExLob::doSanityChecks(char *dir, LobsStorage storage,
                                   Int32 handleInLen, Int32 handleOutLen, 
                                   Int32 blackBoxLen)
{

#ifdef SQ_USE_HDFS
    if (!fs_)
      return LOB_HDFS_CONNECT_ERROR;
#else
    if (fdData_ == -1)
      return LOB_DATA_FILE_OPEN_ERROR;
#endif

    if (dir_.compare(dir) != 0)
      return LOB_DIR_NAME_ERROR;

    if (storage_ != storage)
      return LOB_STORAGE_TYPE_ERROR;

    if (handleInLen > MAX_HANDLE_IN_LEN) {
      return LOB_HANDLE_IN_LEN_ERROR;
    }

    if (handleOutLen > MAX_HANDLE_IN_LEN) {
      return LOB_HANDLE_OUT_LEN_ERROR;
    }

    if (blackBoxLen > MAX_HANDLE_IN_LEN) {
      return LOB_BLACK_BOX_LEN_ERROR;
    }

    return LOB_OPER_OK;
}

Ex_Lob_Error ExLob::allocateDesc(ULng32 size, Int64 &descNum, Int64 &dataOffset, Int64 lobMaxSize, Int64 lobMaxChunkMemLen, char *handleIn, Int32 handleInLen, Int64 lobGCLimit, void *lobGlobals)
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
    lobDebugInfo("In ExLob::allocateDesc",0,__LINE__,lobTrace_);
    Int32 openFlags = O_RDONLY ;   
    
    hdfsFileInfo *fInfo = hdfsGetPathInfo(fs_, lobDataFile_);
    if (fInfo)
      dataOffset = fInfo->mSize;

    if (dataOffset > lobGCLimit) // 5 GB default
      {
        str_sprintf(logBuf,"Starting GC. Current Offset : %Ld",dataOffset);
        lobDebugInfo(logBuf,0,__LINE__,lobTrace_);
           
         
        Int32 rc = SQL_EXEC_LOB_GC_Interface(lobGlobals,handleIn,handleInLen,
                                             hdfsServer_,hdfsPort_,
                                             lobLocation_,
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
          hdfsFreeFileInfo(fInfo, 1);
          fInfo = hdfsGetPathInfo(fs_, lobDataFile_);
        }
        
      if (fInfo)
        dataOffset = fInfo->mSize;
      if (GCDone)
        str_sprintf(logBuf,"Done GC. Allocating new Offset %Ld in %s",
                    dataOffset,lobDataFile_);
      else
        str_sprintf(logBuf,"Allocating new Offset %Ld in %s ",
                    dataOffset,lobDataFile_);
      lobDebugInfo(logBuf,0,__LINE__,lobTrace_);
      //Find the last offset in the file
      // dataOffset = hdfsTell(fs_,fdData_);  //commenting out.hdfsTell always returns 0 !!
     
      return LOB_OPER_OK;    
}
Ex_Lob_Error ExLob::compactLobDataFile(ExLobInMemoryDescChunksEntry *dcArray,Int32 numEntries)
{
  Ex_Lob_Error rc = LOB_OPER_OK;
  char logBuf[4096];
  lobDebugInfo("In ExLob::compactLobDataFile",0,__LINE__,lobTrace_);
  Int64 maxMemChunk = 1024*1024*1024; //1GB limit for intermediate buffer for transfering data
  char * saveLobDataFile = new(getLobGlobalHeap()) char[MAX_LOB_FILE_NAME_LEN+6];
  str_sprintf(saveLobDataFile, "%s_save",lobDataFile_);
  char * tmpLobDataFile = new(getLobGlobalHeap()) char[MAX_LOB_FILE_NAME_LEN+5];
  str_sprintf(tmpLobDataFile, "%s_tmp",lobDataFile_);

  str_sprintf(logBuf,"DataFile %s, TempDataFile : %s, SaveDataFile : %s ",lobDataFile_,tmpLobDataFile, saveLobDataFile);

  lobDebugInfo(logBuf,0,__LINE__,lobTrace_);
  hdfsFS fs = hdfsConnect(hdfsServer_,hdfsPort_);
  if (fs == NULL)
    return LOB_DATA_FILE_OPEN_ERROR;
  
 
  hdfsFile  fdData = hdfsOpenFile(fs, lobDataFile_, O_RDONLY, 0, 0,0);
  if (!fdData) 
    {   
      str_sprintf(logBuf,"Could not open file:%s",lobDataFile_);
      lobDebugInfo(logBuf,0,__LINE__,lobTrace_);
      hdfsCloseFile(fs,fdData);
      fdData = NULL;
      return LOB_DATA_FILE_OPEN_ERROR;
    }
  
  hdfsFile fdTemp = hdfsOpenFile(fs, tmpLobDataFile,O_WRONLY|O_CREAT,0,0,0);
   if (!fdTemp) 
    {
      str_sprintf(logBuf,"Could not open file:%s",tmpLobDataFile);
      lobDebugInfo(logBuf,0,__LINE__,lobTrace_);
      hdfsCloseFile(fs,fdTemp);
      fdTemp = NULL;
      return LOB_DATA_FILE_OPEN_ERROR;
    }

   Int32 i = 0;
   Int64 bytesRead = 0;
   Int64 bytesWritten = 0;
   Int64 size = 0;
   Int64 chunkLen = 0;
   char * tgt = NULL;
   while (i < numEntries)
     {
       chunkLen = dcArray[i].getChunkLen();
       if (chunkLen > maxMemChunk)
         {
           tgt = (char *)(getLobGlobalHeap())->allocateMemory(maxMemChunk);
           while (chunkLen > maxMemChunk)
             {             
               bytesRead = hdfsPread(fs,fdData,dcArray[i].getCurrentOffset(),tgt,maxMemChunk);
               if (bytesRead != maxMemChunk)
                 {
                   lobDebugInfo("Problem reading from  data file",0,__LINE__,lobTrace_);
                   getLobGlobalHeap()->deallocateMemory(tgt);
                   return LOB_DATA_READ_ERROR;
                 }
               bytesWritten = hdfsWrite(fs,fdTemp, tgt,maxMemChunk);
               if (bytesWritten != size)
                 {
                   lobDebugInfo("Problem writing temp data file",0,__LINE__,lobTrace_);
                   getLobGlobalHeap()->deallocateMemory(tgt);
                   return LOB_TARGET_FILE_WRITE_ERROR;
                 }
               chunkLen -= maxMemChunk;
             }
          
         }
       else
         {
           tgt = (char *)(getLobGlobalHeap())->allocateMemory(chunkLen);
            bytesRead = hdfsPread(fs,fdData,dcArray[i].getCurrentOffset(),tgt,chunkLen);
               if (bytesRead != chunkLen)
                 {
                   lobDebugInfo("Problem reading from  data file",0,__LINE__,lobTrace_);
                   getLobGlobalHeap()->deallocateMemory(tgt);
                   return LOB_DATA_READ_ERROR;
                 }
               bytesWritten = hdfsWrite(fs,fdTemp, tgt,chunkLen);
               if (bytesWritten != chunkLen)
                 {
                   lobDebugInfo("Problem writing to temp data file",0,__LINE__,lobTrace_);
                   getLobGlobalHeap()->deallocateMemory(tgt);
                   return LOB_TARGET_FILE_WRITE_ERROR;
                 }
         }
       if (hdfsFlush(fs, fdTemp)) {
          lobDebugInfo("Problem flushing to temp data file",0,__LINE__,lobTrace_);
         return LOB_DATA_FLUSH_ERROR;
       }
       getLobGlobalHeap()->deallocateMemory(tgt);
       i++;
     }
   hdfsCloseFile(fs,fdTemp);
   hdfsCloseFile(fs,fdData);
  
   //Now save the data file and rename the tempfile to the original datafile

   Int32 rc2 = hdfsRename(fs,lobDataFile_,saveLobDataFile);
   if (rc2 == -1)
     {
       lobDebugInfo("Problem renaming datafile to save data file",0,__LINE__,lobTrace_);
       NADELETEBASIC(saveLobDataFile,getLobGlobalHeap());
       NADELETEBASIC(tmpLobDataFile,getLobGlobalHeap());
       return LOB_DATA_FILE_WRITE_ERROR;
     }
   rc2 = hdfsRename(fs,tmpLobDataFile, lobDataFile_);
   if (rc2 == -1)
     {
       lobDebugInfo("Problem renaming temp datafile to data file",0,__LINE__,lobTrace_);
       NADELETEBASIC(saveLobDataFile,getLobGlobalHeap());
       NADELETEBASIC(tmpLobDataFile,getLobGlobalHeap());
       return LOB_DATA_FILE_WRITE_ERROR;
     }
   NADELETEBASIC(saveLobDataFile,getLobGlobalHeap());
   NADELETEBASIC(tmpLobDataFile,getLobGlobalHeap());
   return LOB_OPER_OK;
}

Ex_Lob_Error ExLob::restoreLobDataFile()
{
  Ex_Lob_Error rc = LOB_OPER_OK;
  lobDebugInfo("In ExLob::restoreLobDataFile",0,__LINE__,lobTrace_);
  
  hdfsFS fs = hdfsConnect(hdfsServer_,hdfsPort_);
  if (fs == NULL)
    return LOB_DATA_FILE_OPEN_ERROR;
   char * saveLobDataFile = new(getLobGlobalHeap()) char[MAX_LOB_FILE_NAME_LEN+6];
   str_sprintf(saveLobDataFile, "%s_save",lobDataFile_);
   Int32 rc2 = hdfsDelete(fs,lobDataFile_,FALSE);//ok to ignore error.
   rc2 = hdfsRename(fs,saveLobDataFile, lobDataFile_);
   if (rc2)
     {
       lobDebugInfo("Problem renaming savedatafile to data file",0,__LINE__,lobTrace_);
       NADELETEBASIC(saveLobDataFile,getLobGlobalHeap());
       return LOB_OPER_ERROR; 
     }
   NADELETEBASIC(saveLobDataFile,getLobGlobalHeap());
   return rc;

} 

Ex_Lob_Error ExLob::purgeBackupLobDataFile()
{
  Ex_Lob_Error rc = LOB_OPER_OK;
   lobDebugInfo("In ExLob::purgeBackupLobDataFile",0,__LINE__,lobTrace_);
  hdfsFS fs = hdfsConnect(hdfsServer_,hdfsPort_);
  if (fs == NULL)
    return LOB_DATA_FILE_OPEN_ERROR;
   char * saveLobDataFile = new(getLobGlobalHeap()) char[MAX_LOB_FILE_NAME_LEN+6];
   str_sprintf(saveLobDataFile, "%s_save",lobDataFile_);
   Int32 rc2 = hdfsDelete(fs,saveLobDataFile,FALSE);//ok to ignore error.
   
   NADELETEBASIC(saveLobDataFile,getLobGlobalHeap());
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
         }
      }

      bytesAvailable = cursor.descSize_ - cursor.bytesRead_;
      bytesToCopy = min(bytesAvailable, tgtSize - operLen);
      offset = cursor.descOffset_ + cursor.bytesRead_;
      // #endif

      if (!fdData_ || (openFlags_ != O_RDONLY)) 
      {
         hdfsCloseFile(fs_, fdData_);
	 fdData_=NULL;
         openFlags_ = O_RDONLY;
         fdData_ = hdfsOpenFile(fs_, lobDataFile_, openFlags_, 0, 0, 0);
         if (!fdData_) {
            openFlags_ = -1;
            return LOB_DATA_FILE_OPEN_ERROR;
         }
      }

      clock_gettime(CLOCK_MONOTONIC, &startTime);

      bytesRead = hdfsPread(fs_, fdData_, offset, tgt, bytesToCopy);
      str_sprintf(logBuf,"After hdfsPread: BytesToCopy:%Ld, Offset:%Ld, tgt:%Ld, BytesRead :%Ld",
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
      stats_.CumulativeReadTime += totalnsecs;

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
   hdfsCloseFile(fs_, fdData_);
   fdData_ = NULL;
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
  

  if (fdData_)// we may have a stale handle. close and open to refresh 
    {
      hdfsCloseFile(fs_, fdData_);
      fdData_=NULL;
      openFlags_ = O_RDONLY;
      fdData_ = hdfsOpenFile(fs_, lobDataFile_, openFlags_, 0, 0, 0);
      if (!fdData_) {
	openFlags_ = -1;
	return LOB_DATA_FILE_OPEN_ERROR;
      }
    }
  else
    {
      fdData_ = hdfsOpenFile(fs_, lobDataFile_, openFlags_, 0, 0, 0);
      if (!fdData_) {
	openFlags_ = -1;
	return LOB_DATA_FILE_OPEN_ERROR;
      }
    }
  
  if (!multipleChunks)
    {
      lobDebugInfo("Reading in single chunk",0,__LINE__,lobTrace_);
      if ((bytesRead = hdfsPread(fs_, fdData_, offset, 
				 memAddr, size)) == -1) {
	  
	return LOB_DATA_READ_ERROR;
      } 
      str_sprintf(logBuf,"After hdfsPread: File:%s, Offset:%Ld, Size:%Ld,Target Mem Addr:%Ld",lobDataFile_,offset,size,memAddr);
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
		    handleLenIn);
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

	    if ((err != LOB_OPER_OK) || (operLen != chunkSize))
	      {
		getLobGlobalHeap()->deallocateMemory(lobData);
		return err;
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
		    handleInLen);
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
  char *lobData = 0;
  Int64 chunkSize = 0;	
  hdfsFile  fdTgtFile;
  char logBuf[4096];
  lobDebugInfo("In ExLob::readDataToHdfsFile",0,__LINE__,lobTrace_);
  // open and write to the target file
  int openFlags = O_WRONLY;
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

  if ((srcLen < lobMaxChunkMemLen) && (multipleChunks ==FALSE)) // simple single I/O case
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
  else
    {// multiple chunks to read
      lobDebugInfo("Reading in multiple chunks into local file",0,__LINE__,lobTrace_);
      err = openCursor(handleIn, 
		       handleInLen,
                       transId);
      if (err != LOB_OPER_OK)
	return err;
      while ( srcLen > 0)
	{
	  chunkSize = MINOF(srcLen, lobMaxChunkMemLen);
	  lobData = (char *) (getLobGlobalHeap())->allocateMemory(chunkSize);	      
	  if (lobData == NULL) 
	    {
	      getLobGlobalHeap()->deallocateMemory(lobData);
	      return LOB_SOURCE_DATA_ALLOC_ERROR;
	    }
	  //handle reading the multiple chunks like a cursor
	  err = readCursor(lobData,chunkSize, handleIn,
			   handleInLen, operLen, transId);

	  if ((err != LOB_OPER_OK) || (operLen != chunkSize))
	    {
	      getLobGlobalHeap()->deallocateMemory(lobData);
	      return LOB_DATA_READ_ERROR;
	    }
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
	  getLobGlobalHeap()->deallocateMemory(lobData);
	  srcLen -= chunkSize;

	}
      closeCursor(handleIn, 
		  handleInLen);	    
    }
  hdfsCloseFile(fs_, fdTgtFile);
  fdTgtFile=NULL;
  hdfsCloseFile(fs_,fdData_);
  fdData_=NULL;
  
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
    memcpy(statsBuffer, (char *)&stats_, sizeof(stats_));
    return LOB_OPER_OK;
}

Ex_Lob_Error ExLob::initStats()
{
    stats_.init();
    return LOB_OPER_OK;
}
//Main driver of any LOB related operation 

Ex_Lob_Error ExLobsOper (
			 char        *lobName,          // lob name
			 char        *handleIn,         // input handle (for cli calls)
			 Int32       handleInLen,       // input handle len
			 char        *hdfsServer,       // server where hdfs fs resides
			 Int64       hdfsPort,          // port number to access hdfs server
			 char        *handleOut,        // output handle (for cli calls)
			 Int32       &handleOutLen,     // output handle len
			 Int64       descNumIn,         // input desc Num (for flat files only)
			 Int64       &descNumOut,       // output desc Num (for flat files only)
			 Int64       &retOperLen,       // length of data involved in this operation
			 Int64       requestTagIn,      // only for checking status
			 Int64       &requestTagOut,    // returned with every request other than check status
			 Ex_Lob_Error  &requestStatus,  // returned req status
			 Int64       &cliError,         // err returned by cli call
			 char        *dir,              // directory in the storage
			 LobsStorage storage,           // storage type
			 char        *source,           // source (memory addr, filename, foreign lob etc)
			 Int64       sourceLen,         // source len (memory len, foreign desc offset etc)
			 Int64       cursorBytes,
			 char        *cursorId,
			 LobsOper    operation,         // LOB operation
			 LobsSubOper subOperation,      // LOB sub operation
			 Int64       waited,            // waited or nowaited
			 void        *&globPtr,         // ptr to the Lob objects. 
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
  char fn[MAX_LOB_FILE_NAME_LEN];
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

  char *fileName = lobName;

  if (globPtr == NULL)
    {
      if ((operation == Lob_Init) ||
          (operation == Lob_Empty_Directory) ||
          (operation == Lob_Data_Mod_Check))
	{
          
          globPtr = new ExLobGlobals();
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

  if ((globPtr != NULL) && (operation != Lob_Init))
    {
      lobGlobals = (ExLobGlobals *)globPtr;

      lobMap = lobGlobals->getLobMap();

      it = lobMap->find(string(fileName));

      if (it == lobMap->end())
	{
	  //lobPtr = new (lobGlobals->getHeap())ExLob();
	  lobPtr = new ExLob();
	  if (lobPtr == NULL) 
	    return LOB_ALLOC_ERROR;

	  err = lobPtr->initialize(fileName, (operation == Lob_Create) ? EX_LOB_CREATE : EX_LOB_RW, dir, storage, hdfsServer, hdfsPort, dir,bufferSize, replication, blockSize,lobMaxSize,lobGlobals);
	  if (err != LOB_OPER_OK)
            {
              char buf[5000];
              str_sprintf(buf,"Lob initialization failed;filename:%s;location:%s;hdfsserver:%s;hdfsPort:%d;lobMaxSize:%Ld",fileName,dir,hdfsServer,lobMaxSize);
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
      err = lobPtr->writeDesc(sourceLen, source, subOperation, descNumOut, retOperLen, lobMaxSize, lobMaxChunkMemSize,lobGCLimit,handleIn,handleInLen,(char *)blackBox, &blackBoxLen,handleOut,handleOutLen,lobGlobals);
      if (err != LOB_OPER_OK)
        {
          lobDebugInfo("writeDesc failed ",err,__LINE__,lobGlobals->lobTrace_);
        }
      break;

    case Lob_InsertData:
      err = lobPtr->insertData(source, sourceLen, subOperation, descNumIn, retOperLen, lobMaxSize,lobMaxChunkMemSize,handleIn,handleInLen,(char *)blackBox, blackBoxLen,handleOut,handleOutLen,lobGlobals);
      if (err != LOB_OPER_OK)
        {
          lobDebugInfo("insertData failed ",err,__LINE__,lobGlobals->lobTrace_);
        }
      break;

    case Lob_InsertDataSimple:
      err = lobPtr->writeDataSimple(source, sourceLen, subOperation, retOperLen,
				    bufferSize , replication , blockSize);
      break;

    case Lob_Read:
      if (storage == Lob_External_HDFS_File)   
        //Allocate storage to read the lob external file name from the 
        //descriptor tables  to get the data from.        
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

    case Lob_ReadDesc: // read desc only. Needed for pass thru.
      err = lobPtr->getDesc(desc,handleIn,handleInLen,(char *)blackBox, &blackBoxLen,handleOut,handleOutLen,transId); 
      retOperLen = 0;
      break;
    case Lob_OpenCursor:
      err = lobPtr->openCursor(handleIn, handleInLen,transId);
      break;

    case Lob_OpenDataCursorSimple:  
      if (openType == 1) { // preopen
	sprintf(fn,"%s:%Lx:%s",lobPtr->getDataFileName(), (long long unsigned int)lobName, cursorId);
	preOpenObj = new (lobGlobals->getHeap()) ExLobPreOpen(lobPtr, fn, descNumIn, sourceLen, cursorBytes, waited);
	lobGlobals->addToPreOpenList(preOpenObj);
      } else if (openType == 2) { // must open
	sprintf(fn,"%s:%Lx:%s",lobPtr->getDataFileName(), (long long unsigned int)lobName, cursorId);
	fileName = fn;
	err = lobPtr->openDataCursor(fileName, Lob_Cursor_Simple, descNumIn, sourceLen, cursorBytes, waited, lobGlobals);
      } else
	err = LOB_SUBOPER_ERROR;
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
      sprintf(fn,"%s:%Lx:%s",lobPtr->getDataFileName(), (long long unsigned int)lobName, cursorId);
      fileName = fn;       
      err = lobPtr->readDataCursorSimple(fileName, source, sourceLen, retOperLen, lobGlobals);
      break;

    case Lob_CloseFile:
      if (lobPtr->hasNoOpenCursors()) {
	lobGlobals->traceMessage("Lob_CloseFile",NULL,__LINE__);
	err = lobPtr->closeFile();
	it = lobMap->find(string(lobName));
	lobMap->erase(it);
	delete lobPtr;
	lobPtr = NULL;
      }  
      break;

    case Lob_CloseCursor:
      err = lobPtr->closeCursor(handleIn, handleInLen);
      break;

    case Lob_CloseDataCursorSimple:
      sprintf(fn,"%s:%Lx:%s",lobPtr->getDataFileName(), (long long unsigned int)lobName, cursorId);
      fileName = fn;
      err = lobPtr->closeDataCursorSimple(fileName, lobGlobals);
      break;

    case Lob_Append:
      if ((subOperation == Lob_Memory) ||(subOperation == Lob_Buffer) || (subOperation ==Lob_External))
        {
          err = lobPtr->append(source, sourceLen, subOperation, descNumIn, retOperLen,lobMaxSize, lobMaxChunkMemSize,lobGCLimit,handleIn,handleInLen,handleOut,handleOutLen,lobGlobals);
          if (err != LOB_OPER_OK)
            {
              lobDebugInfo("append(Memory,Buffer) failed ",err,__LINE__,lobGlobals->lobTrace_);
            }
        }
      else if (subOperation == Lob_File)
        {
          err = lobPtr->append(source, -1, subOperation, descNumIn, retOperLen,lobMaxSize, lobMaxChunkMemSize,lobGCLimit,handleIn,handleInLen,handleOut,handleOutLen,lobGlobals);
          if (err != LOB_OPER_OK)
            {
              lobDebugInfo("append(File) failed ",err,__LINE__,lobGlobals->lobTrace_);
            }
        }
      else  
	err = LOB_SUBOPER_ERROR;
      break;

    case Lob_Update:
      if ((subOperation == Lob_Memory)||(subOperation == Lob_Buffer)||(subOperation ==Lob_External))
        {
          err = lobPtr->update(source, sourceLen, subOperation, descNumIn, retOperLen, lobMaxSize, lobMaxChunkMemSize,lobGCLimit,handleIn,handleInLen,handleOut,handleOutLen,lobGlobals);
          if (err != LOB_OPER_OK)
            {
              lobDebugInfo("update(Memory,Buffer) failed ",err,__LINE__,lobGlobals->lobTrace_);
            }
        }
      else if (subOperation == Lob_File)
        {
          err = lobPtr->update(source, -1, subOperation,descNumIn, retOperLen,lobMaxSize, lobMaxChunkMemSize,lobGCLimit,handleIn,handleInLen,handleOut,handleOutLen,lobGlobals); 
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
      delete lobPtr;
      lobPtr = NULL;
      if (err != LOB_OPER_OK)           
        lobDebugInfo("purgeLob failed ",err,__LINE__,lobGlobals->lobTrace_);
      break;

    case Lob_Purge:
      err = lobPtr->purgeLob();
      it = lobMap->find(string(lobName));
      lobMap->erase(it);
      delete lobPtr;
      lobPtr = NULL;
      if (err != LOB_OPER_OK)           
        lobDebugInfo("purgeLob failed ",err,__LINE__,lobGlobals->lobTrace_);
      break;

    case Lob_Stats:
      err = lobPtr->readStats(source);
      lobPtr->initStats(); // because file may remain open across cursors
      break;

    case Lob_Empty_Directory:
      lobPtr->initialize(fileName, EX_LOB_RW,
			 dir, storage, hdfsServer, hdfsPort, dir, bufferSize, 
                         replication, blockSize);
      err = lobPtr->emptyDirectory(dir, lobGlobals);
      break;

    case Lob_Data_Mod_Check:
      {
        lobPtr->initialize(NULL, EX_LOB_RW,
                           NULL, storage, hdfsServer, hdfsPort, NULL, 
                           bufferSize, replication, blockSize, lobMaxSize, 
                           lobGlobals);

        Int64 inputModTS = *(Int64*)blackBox;
        Int32 inputNumOfPartLevels = 
          *(Lng32*)&((char*)blackBox)[sizeof(inputModTS)];
        err = lobPtr->dataModCheck(dir, inputModTS, inputNumOfPartLevels,
                                   lobGlobals);
      }
      break;

    case Lob_Cleanup:
        delete lobGlobals;
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
  if (lobPtr && lobPtr->getStats())
    lobPtr->getStats()->hdfsAccessLayerTime += totalnsecs; 
       
  return err;
}

void cleanupLOBDataDescFiles(const char *lobHdfsServer,int lobHdfsPort,const char *lobHdfsLoc)
{ 
  int numExistingFiles=0;
  hdfsFS fs;
  int err = 0;
  fs = hdfsConnect(lobHdfsServer, lobHdfsPort);
  if (fs == NULL)
    return;
  // Get this list of all data and desc files in the lob sotrage location
  hdfsFileInfo *fileInfos = hdfsListDirectory(fs, lobHdfsLoc, &numExistingFiles);
  if (fileInfos == NULL)
      return ;
    
  //Delete each one in a loop
  for (int i = 0; i < numExistingFiles; i++)  
    {    
      err = hdfsDelete(fs, fileInfos[i].mName, 0);
    }
    
  // *Note* : delete the memory allocated by libhdfs for the file info array  
  if (fileInfos)
    {
      hdfsFreeFileInfo(fileInfos, numExistingFiles);
    }
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

Ex_Lob_Error ExLob::readDataCursorSimple(char *file, char *tgt, Int64 tgtSize, 
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
      stats_.bytesPrefetched += bytesToCopy;
      operLen += bytesToCopy;
    } 

    // update stats
    stats_.bytesRead += operLen;
    stats_.bytesToRead += tgtSize;
    stats_.numReadReqs++;

    return LOB_OPER_OK;
}

Ex_Lob_Error ExLob::closeDataCursorSimple(char *fileName, ExLobGlobals *lobGlobals)
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
    stats_.cursorElapsedTime += totalnsecs;

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
          lobPtr->stats_.buffersUsed++;
        }
        size = min(cursor->bufMaxSize_, (cursor->maxBytes_ - cursor->bytesRead_ + (16 * 1024)));
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
	    lobPtr->deleteCursor(cursor->name_, this);
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

      stats_.numHdfsReqs++;

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
   stats_.CumulativeReadTime += totalnsecs;

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
Ex_Lob_Error ExLob::deleteCursor(char *cursorName, ExLobGlobals *lobGlobals)
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

    request_.setType(Lob_Req_Get_Desc);

    err = request_.send();

    if (err != LOB_OPER_OK) {
       return err;
    }

    err = request_.getError();

    return err;
}

///////////////////////////////////////////////////////////////////////////////
// ExLobGlobals definitions
///////////////////////////////////////////////////////////////////////////////

ExLobGlobals::ExLobGlobals() :
    lobMap_(NULL), 
    fs_(NULL),
    isCliInitialized_(FALSE),
    isHive_(FALSE),
    threadTraceFile_(NULL),
    lobTrace_(FALSE),
    heap_(NULL)
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

    preOpenListLock_.lock();
    preOpenList_.clear();
    preOpenListLock_.unlock();

    
    if (lobMap_) 
      delete lobMap_;

    for (int i=0; i<NUM_WORKER_THREADS; i++) {
      enqueueShutdownRequest();
    }

    for (int i=0; i<NUM_WORKER_THREADS; i++) {
      pthread_join(threadId_[i], NULL);
    }
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


    // start the worker threads
    startWorkerThreads();

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

ExLobHdfsRequest::ExLobHdfsRequest(LobsHdfsRequestType reqType, hdfsFS fs, 
                                   hdfsFile file, char *buffer, int size) :
   reqType_(reqType),
   fs_(fs),
   file_(file),
   buffer_(buffer),
   size_(size)
{
  lobPtr_ = 0;
  error_ = LOB_OPER_OK;
}

ExLobHdfsRequest::ExLobHdfsRequest(LobsHdfsRequestType reqType, ExLobCursor *cursor) :
   reqType_(reqType),
   cursor_(cursor)
{
  buffer_=0;
  lobPtr_=0;
  fs_=0;
  file_=0;
  size_=0;
  error_=LOB_OPER_OK;
}

ExLobHdfsRequest::ExLobHdfsRequest(LobsHdfsRequestType reqType, ExLob *lobPtr, ExLobCursor *cursor) :
   reqType_(reqType),
   lobPtr_(lobPtr),
   cursor_(cursor)
{
  buffer_=0;
  fs_=0;
  file_=0;
  size_=0;
  error_=LOB_OPER_OK;
}

ExLobHdfsRequest::ExLobHdfsRequest(LobsHdfsRequestType reqType) :
   reqType_(reqType)
{

  buffer_=0;
  cursor_=0;
  lobPtr_=0;
  fs_=0;
  file_=0;
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
{// Leaving this allocated from system heap. Since this class contains hdfsFS unable to derive from LOB heap
  ExLobHdfsRequest *request = new  ExLobHdfsRequest(Lob_Hdfs_Cursor_Prefetch, lobPtr, cursor);
   
   if (!request) {
     // return error
   }

   enqueueRequest(request);

   return LOB_OPER_OK;
}

Ex_Lob_Error ExLobGlobals::enqueueShutdownRequest()
{
 // Leaving this allocated from system heap. Since this class contains hdfsFS unable to derive from LOB heap
  ExLobHdfsRequest *request = new ExLobHdfsRequest(Lob_Hdfs_Shutdown);
   
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
         delete request;
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

        lobPtr->openDataCursor(preOpenObj->cursorName_, Lob_Cursor_Simple, preOpenObj->range_, 
                               preOpenObj->bufMaxSize_, preOpenObj->maxBytes_, 
                               preOpenObj->waited_, this);
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
//The output file will be in the masterexec.<pid> logs in the 
//$MY_SQROOT/logs directory on each node

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
