/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2015 Hewlett-Packard Development Company, L.P.
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
**********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ex_lob.C
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

#define SQ_USE_HDFS 1

#ifdef SQ_USE_HDFS
#include "hdfs.h"
#include "jni.h"
#endif 

#include "ExpLOBstats.h"
#include "ExpLOBaccess.h"
#include "ExpLOBinterface.h"

#include "NAVersionedObject.h"
#include "ComQueue.h"

#include "NAMemory.h"
#include <seabed/ms.h>
#include <../../sqf/src/seabed/src/trans.h>
#include <seabed/fserr.h>

extern int ms_transid_get(bool pv_supp,
                          bool pv_trace,
                          MS_Mon_Transid_Type *pp_transid);
extern int ms_transid_reinstate(MS_Mon_Transid_Type);

// short LobServerFNum;
SB_Phandle_Type serverPhandle;

ExLob::ExLob() :
    storage_(Lob_Invalid_Storage),
    dir_(string()),
    lobGlobalHeap_(NULL),

    // fdDesc_(-1),
    fdDesc_(NULL),
    fs_(NULL),
    fdData_(NULL),
    openFlags_(0)  
{
    lobDataFile_[0] = '\0';
    lobDescFile_[0] = '\0'; 
}

ExLob::~ExLob()
{
    
    if (fdData_) {
      hdfsCloseFile(fs_, fdData_);
      fdData_ = NULL;
    }
    if (fdDesc_) {
      hdfsCloseFile(fs_, fdDesc_);
      fdDesc_ = NULL;
    }
    
    /*   
    Commenting this out. It is causing cores during hive access.
    Note :  Not calling hdfsDisconnect this will cause a leak that needs to be 
            fixed at a different place  
    if (fs_){
     hdfsDisconnect(fs_);
    fs_=NULL;
    
    }*/
   
}

Ex_Lob_Error ExLob::initialize(char *lobFile, Ex_Lob_Mode mode, 
                               char *dir, 
			       LobsStorage storage,
                               char *hdfsServer, Int64 hdfsPort,
                               int bufferSize , short replication ,
                               int blockSize, ExLobGlobals *lobGlobals)
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

   
      snprintf(lobDataFile_, MAX_LOB_FILE_NAME_LEN, "%s/%s", dir_.c_str(), lobFile);
      snprintf(lobDescFile_, MAX_LOB_FILE_NAME_LEN, "%s/%s.desc", dir_.c_str(), lobFile);
    } 
  else 
    { 
      snprintf(lobDataFile_, MAX_LOB_FILE_NAME_LEN, "%s", lobFile);
      snprintf(lobDescFile_, MAX_LOB_FILE_NAME_LEN, "%s.desc", lobFile);
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

  clock_gettime(CLOCK_MONOTONIC, &startTime);

  if (lobGlobals->getHdfsFs() == NULL)
    {
      fs_ = hdfsConnect(hdfsServer_, hdfsPort_);
      if (fs_ == NULL) 
	{
	  return LOB_HDFS_CONNECT_ERROR;
	}
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
      if (!lobGlobals->isHive())
	{
	  //Create the desc header file that holds info about the 
	  //lob data file offsets etc.   
	  fdDesc_ = hdfsOpenFile(fs_, lobDescFile_, O_WRONLY, bufferSize, replication, blockSize);
	  if (!fdDesc_) 
	    {
	      return LOB_DESC_FILE_CREATE_ERROR;
	    }
	  //write empty header info into it. 
	  ExLobDescHeader header(LOB_DATA_FILE_SIZE_MAX);
      
	  Int64 numWritten = 0;
	  numWritten = hdfsWrite(fs_, fdDesc_, (void *)&header, sizeof(ExLobDescHeader));
	  if (numWritten <=0)
	    return LOB_DATA_WRITE_ERROR;
      
      
	  if (hdfsFlush(fs_, fdDesc_)) 
	    {
	      return LOB_DATA_FLUSH_ERROR;
	    }
	  hdfsCloseFile(fs_, fdDesc_);
	  fdDesc_ = NULL;   
	}
    }
  lobGlobalHeap_ = lobGlobals->getHeap();    
  return LOB_OPER_OK;
    
}

Ex_Lob_Error ExLob::fetchCursor() 
{
    Ex_Lob_Error err; 

    request_.setType(Lob_Req_Fetch_Cursor);

    err = request_.send();

    if (err != LOB_OPER_OK) {
       return err;
    }

    err = request_.getError();

    return err;
}

Ex_Lob_Error ExLob::delDesc(Int64 descNum) 
{
    Ex_Lob_Error err; 

    request_.setType(Lob_Req_Del_Desc);
    request_.setDescNumIn(descNum);

    err = request_.send();

    if (err != LOB_OPER_OK) {
       return err;
    }

    err = request_.getError();

    return err;
}

Ex_Lob_Error ExLob::getDesc(ExLobDesc &desc) 
{
    Ex_Lob_Error err; 

    request_.setType(Lob_Req_Get_Desc);

    err = request_.send();

    if (err != LOB_OPER_OK) {
       return err;
    }

    request_.getDescOut(desc);
    err = request_.getError();

    return err;
}

Ex_Lob_Error ExLob::putDesc(ExLobDesc &desc, Int64 descNum)
{
    Ex_Lob_Error err; 

    request_.setType(Lob_Req_Put_Desc);
    request_.setDescNumIn(descNum);
    request_.putDescIn(desc);

    err = request_.send();

    if (err != LOB_OPER_OK) {
       return err;
    }

    err = request_.getError();

    return err;
}

Ex_Lob_Error ExLob::writeData(Int64 offset, char *data, Int64 size, Int64 &operLen)
{ 
    Ex_Lob_Error err;
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

     if (hdfsWrite(fs_, fdData_, data, size) == -1) {
       return LOB_DATA_WRITE_ERROR;
     }
     if (hdfsFlush(fs_, fdData_)) {
       return LOB_DATA_FLUSH_ERROR;
     }

    operLen = size;

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
Ex_Lob_Error ExLob::emptyDirectory()
{
    Ex_Lob_Error err;

    int numExistingFiles=0;
    hdfsFileInfo *fileInfos = hdfsListDirectory(fs_, lobDataFile_, &numExistingFiles);
    if (fileInfos == NULL)
    {
       return LOB_DATA_FILE_NOT_FOUND_ERROR; //here a directory
    }

    for (int i = 0; i < numExistingFiles; i++) //do in a loop now -- maybe change it to staties latetr
    {
#ifdef USE_HADOOP_1
      int retCode = hdfsDelete(fs_, fileInfos[i].mName);
#else
      int retCode = hdfsDelete(fs_, fileInfos[i].mName, 0);
#endif
      if (retCode !=0)
      {
        //ex_assert(retCode == 0, "delete returned error");
        return LOB_DATA_FILE_DELETE_ERROR;
      }
    }
    if (fileInfos)
    {
      hdfsFreeFileInfo(fileInfos, numExistingFiles);
    }
    /* Commenting this out, it would make regression test hive/TEST003 fail
#ifdef _DEBUG
    char debugFile[1000];
    sprintf(debugFile,"%s%s",lobDataFile_,  "debugFile" );

    hdfsFile hdfsfile = hdfsOpenFile(fs_, debugFile, O_WRONLY | O_CREAT, 0, 1, 512);
    if (hdfsfile == NULL)
    {
      assert(0);
    }
    if (hdfsCloseFile(fs_, hdfsfile) !=0 )
    {
      assert(0);
    }

   // sleep(10); // sleep for 10 seconds to verify the diffrence in time stamps
#endif
    */

    return LOB_OPER_OK;
}
Ex_Lob_Error ExLob::readSourceFile(char *srcfile, char *&fileData, int &size)
{
    int openFlags = O_RDONLY;
    int fdSrcFile = open(srcfile, openFlags);
    if (fdSrcFile == -1) {
      return LOB_SOURCE_FILE_OPEN_ERROR;
    }

    if (flock(fdSrcFile, LOCK_EX) == -1) {
      return LOB_SOURCE_FILE_LOCK_ERROR;
    }

    struct stat statbuf;
    if (stat(srcfile, &statbuf) != 0) {
      return LOB_SOURCE_FILE_STAT_ERROR;
    }

    size = statbuf.st_size;

    fileData = (char *) (getLobGlobalHeap())->allocateMemory(size);
    if (fileData == (char *)-1) {
      return LOB_SOURCE_DATA_ALLOC_ERROR;
    }

    if (pread(fdSrcFile, fileData, size, 0) == -1) {
      close(fdSrcFile);
      getLobGlobalHeap()->deallocateMemory(fileData);
      fileData = NULL;
      return LOB_SOURCE_FILE_READ_ERROR;
    }

    flock(fdSrcFile, LOCK_UN);
    close(fdSrcFile);

    return LOB_OPER_OK;
}

Ex_Lob_Error ExLob::writeDesc(Int64 sourceLen, Int64 &descNumOut, Int64 &operLen)
{
    Ex_Lob_Error err; 
    Int64 dataOffset =0;

    err = allocateDesc((unsigned int)sourceLen, descNumOut, dataOffset);

    operLen = 0; 
    if (err != LOB_OPER_OK)
      return err;

    //send a message to mxlobsrvr to insert into the descriptor tables
    request_.setType(Lob_Req_Allocate_Desc);
    request_.getDesc().setSize(sourceLen);
    request_.setDescNumOut(descNumOut);
    request_.setDataOffset(dataOffset);
    err = request_.send();
    if (err != LOB_OPER_OK) {
      return err;
    }
    return err;
}

Ex_Lob_Error ExLob::writeData(char *source, Int64 sourceLen, LobsSubOper subOperation, 
                          Int64 descNumIn, Int64 &operLen)
{
    Int64 dataOffset;
    Ex_Lob_Error err; 
    ExLobDesc desc;

    operLen = 0;

    // get offset from desc row
    err = getDesc(desc);
    if (err != LOB_OPER_OK)
       return err;

    int cliErr = request_.getCliError();
    if (cliErr < 0 || cliErr == 100) { // some error or EOD.
       return LOB_DESC_READ_ERROR;
    }
    
    if ((source == NULL)) { 
       return LOB_SOURCE_DATA_ERROR;
    }

    char *memAddr =  NULL;
    int memSize = -1;

    if (subOperation == Lob_File) {
      err = readSourceFile(source, memAddr, memSize);
      if (err != LOB_OPER_OK)
          return err;
    } else { // in memory
       memAddr = source;
       memSize = sourceLen;
    }


    
    err = writeData(NULL, memAddr, memSize, operLen);
    hdfsCloseFile(fs_, fdData_);
    fdData_=NULL;


    if (subOperation == Lob_File) {
       delete memAddr;
    }

    return err;
}

Ex_Lob_Error ExLob::readToMem(char *memAddr, Int64 size, Int64 descNumIn, Int64 &operLen)
{
   Ex_Lob_Error err = LOB_OPER_OK; 

   Int64 sizeToRead = size;
   Int64 dNum = descNumIn;
   char *addr = memAddr;
   int cliErr;

   operLen = 0;


   if (!fdData_ || (openFlags_ != O_RDONLY)) 
   {
      hdfsCloseFile(fs_, fdData_);
      fdData_=NULL;
      openFlags_ = O_RDONLY;
      fdData_ = hdfsOpenFile(fs_, lobDataFile_, openFlags_, 0, 0, 0);
      if (!fdData_) {
         return LOB_DATA_FILE_OPEN_ERROR;
      }
   }
   err = readDataToMem(addr, dNum, 0 /*offset*/, sizeToRead, operLen);

   return err;
}

Ex_Lob_Error ExLob::readToFile(char *fileName, Int64 descNum, Int64 &operLen)
{
    Ex_Lob_Error err; 

    err = readDataToFile(fileName, descNum, 0 /*offset*/, operLen);
    if (err != LOB_OPER_OK)
      return err;

    return LOB_OPER_OK;
}

Ex_Lob_Error ExLob::append(char *data, Int64 size, Int64 headDescNum, Int64 &operLen)
{
    Ex_Lob_Error err = LOB_OPER_OK;
    Int64 dummyParam;
    Int64 dataOffset=0;
    err = allocateDesc((unsigned int)size, dummyParam, dataOffset);
    if (err != LOB_OPER_OK)
      return err;
    request_.setType(Lob_Req_Append);
    request_.getDesc().setSize(size);
    request_.setDataOffset(dataOffset);
    request_.send();

    err = request_.getError();

    if (err != LOB_OPER_OK) {
       return err;
    }

    int cliErr = request_.getCliError();
    if (cliErr < 0 || cliErr == 100) { // some error or EOD.
       return LOB_DESC_APPEND_ERROR;
    }

    
    // write new lob desc
    if (size != -1)
    { 
      err = writeData(NULL, data, size, operLen);
      hdfsCloseFile(fs_, fdData_);
      fdData_ = NULL;

      if (err != LOB_OPER_OK)
        return err;
    }

    operLen = size;
    return LOB_OPER_OK;
}

Ex_Lob_Error ExLob::update(char *data, Int64 size, Int64 headDescNum, Int64 &operLen)
{
    Ex_Lob_Error err = LOB_OPER_OK;
    Int64 dummyParam;
    Int64 dataOffset = 0;

    err = allocateDesc((unsigned int)size, dummyParam, dataOffset);
    if (err != LOB_OPER_OK)
      return err;
    // send a message to mxlobsrvr to do an update into descriptor tables
    request_.setType(Lob_Req_Update);
    request_.getDesc().setSize(size);
    request_.setDataOffset(dataOffset);

    request_.send();

    err = request_.getError();

    if (err != LOB_OPER_OK) {
       return err;
    }

    int cliErr = request_.getCliError();
    if (cliErr < 0 || cliErr == 100) { // some error or EOD.
       return LOB_DESC_UPDATE_ERROR;
    }

   

    // write new lob desc
    if (size != -1)
    {

     
      err = writeData(NULL, data, size, operLen);
      hdfsCloseFile(fs_, fdData_);
      fdData_ = NULL;

      if (err != LOB_OPER_OK)
        return err;
    }

    operLen = size;

    return LOB_OPER_OK;
}

Ex_Lob_Error ExLob::delDesc()
{
    Ex_Lob_Error err;
    Int64 dummyParam;

    request_.setType(Lob_Req_Del_Desc);

    request_.send();

    err = request_.getError();

    return err;
}

Ex_Lob_Error ExLob::purgeLob()
{
   
     if (hdfsDelete(fs_, lobDataFile_, 0) != 0)
       {
	 return LOB_DATA_FILE_DELETE_ERROR;
       }
     if (hdfsDelete(fs_, lobDescFile_, 0) != 0) 
       {
	 return LOB_DESC_FILE_DELETE_ERROR;
       }


    return LOB_OPER_OK;
}

Ex_Lob_Error ExLob::openCursor(char *handleIn, Int64 handleInLen)
{
    Ex_Lob_Error err;
    cursor_t cursor;

    request_.setType(Lob_Req_Select_Cursor);

    err = request_.send();

    if (err != LOB_OPER_OK) {
       return err;
    }

    err = request_.getError();

    if (err != LOB_OPER_OK) {
       return err;
    }

    cursor.bytesRead_ = -1;
    cursor.descOffset_ = -1;
    cursor.descSize_ = -1;
    cursor.cliInterface_ = NULL; // used only in lob process
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

Ex_Lob_Error ExLob::readCursor(char *tgt, Int64 tgtSize, char *handleIn, Int64 handleInLen, Int64 &operLen)
{
    int dataOffset;
    Ex_Lob_Error result;
    cursor_t cursor;

    lobCursors_it it = lobCursors_.find(string(handleIn, handleInLen));

    if (it == lobCursors_.end())
    {
       return LOB_CURSOR_NOT_OPEN;
    }
    else
    {
       cursor = it->second; 
    } 

    if (cursor.eod_) {
       // remove cursor from the map.
       // server has already closed the cursor. 
       closeCursor(handleIn, handleInLen); 
       // indicate EOD to SQL
       operLen = 0; 
       return LOB_OPER_OK;
    }

    result = readCursorData(tgt, tgtSize, cursor, operLen); // increments cursor
        
    if (result != LOB_OPER_OK)
      return result;

    it->second = cursor;

    return LOB_OPER_OK;
}

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

Ex_Lob_Error ExLob::closeCursor(char *handleIn, Int64 handleInLen)
{
    lobCursors_it it = lobCursors_.find(string(handleIn, handleInLen));
    if (it != lobCursors_.end())
    {
      lobCursors_.erase(it);
    }
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

Ex_Lob_Error ExLob::print()
{
    Ex_Lob_Error err;
    request_.setType(Lob_Req_Print);
    err = request_.send();
    return err;
}

Ex_Lob_Error ExLob::doSanityChecks(char *dir, LobsStorage storage,
                                   Int64 handleInLen, Int64 handleOutLen, 
                                   Int64 blackBoxLen)
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

/*
Ex_Lob_Error ExLob::allocateDesc(unsigned int size, Int64 &descNum, Int64 &dataOffset) 
{
  Ex_Lob_Error err;

  request_.setType(Lob_Req_Allocate_Desc);
  request_.getDesc().setSize(size);

  err = request_.send();
  if (err != LOB_OPER_OK) {
     return err;
  }

  descNum = request_.getDescNumOut();
  err = request_.getError();

  return err;
}
*/

Ex_Lob_Error ExLob::allocateDesc(ULng32 size, Int64 &descNum, Int64 &dataOffset)
{
    Ex_Lob_Error err = LOB_OPER_OK;
    Lng32 retval = 0;
    Int64 numRead = 0;
    Int64 numWritten = 0;

    // TBD need a way to lock access to this file.    
    
    Int32 openFlags = O_RDONLY ;   
    fdDesc_ = hdfsOpenFile(fs_, lobDescFile_, O_RDONLY, 0, 0,0);
      if (!fdDesc_) {
	hdfsCloseFile(fs_,fdDesc_);
	fdDesc_ = NULL;
	return LOB_DESC_FILE_OPEN_ERROR;
      }
    ExLobDescHeader header;
    numRead = hdfsPread(fs_,fdDesc_, 0, (void *)&header, sizeof(ExLobDescHeader) );
    if (numRead <=0)
      {
	return LOB_DESC_HEADER_READ_ERROR;
      }
    if (header.getAvailSize() >= size) {
      descNum = header.getFreeDesc(); 

      dataOffset = header.getDataOffset();
      header.incFreeDesc();
      header.decAvailSize(size);
      header.incDataOffset(size);

      hdfsCloseFile(fs_,fdDesc_);
      fdDesc_ = NULL;
      openFlags = O_WRONLY;
      fdDesc_ = hdfsOpenFile(fs_,lobDescFile_,openFlags,0,0,0);
      if (!fdDesc_) {
	
	return LOB_DESC_FILE_OPEN_ERROR;
      }
      numWritten = hdfsWrite(fs_,fdDesc_, (void *)&header, sizeof(ExLobDescHeader)) ;
      if (numWritten <= 0)
	{
	  return LOB_DESC_HEADER_WRITE_ERROR;
      }

      
    }
    else {
      return LOB_DATA_FILE_FULL_ERROR;
    }
    ExLobDesc desc(dataOffset, size, descNum);

      hdfsCloseFile(fs_,fdDesc_);
      fdDesc_=NULL;
      openFlags = O_WRONLY| O_APPEND;
      fdDesc_ = hdfsOpenFile(fs_,lobDescFile_,openFlags,0,0,0);
      numWritten = hdfsWrite(fs_,fdDesc_, (void *)&desc, sizeof(ExLobDesc));
      if (numWritten <= 0)
	{
	  err = LOB_DESC_WRITE_ERROR;
	}
      hdfsCloseFile(fs_,fdDesc_);
      fdDesc_=NULL;
      /* if (pwrite(fdDesc_, &desc, sizeof(ExLobDesc), 
        sizeof(ExLobDescHeader) + sizeof(ExLobDesc) * descNum) == -1) {
	return LOB_DESC_WRITE_ERROR; }*/


      /*err = unlockDesc();
	if (err != LOB_OPER_OK)
	return err; */
      // TBD need a way to unlock this hdfs file.
    return err;
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

Ex_Lob_Error ExLob::readCursorData(char *tgt, Int64 tgtSize, cursor_t &cursor, Int64 &operLen)
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

   while ( (operLen < tgtSize) && !cursor.eod_ )
   {
     /*#ifdef SQ_USE_HDFS
      if (cursor.bytesRead_ == -1) {  // starting
         cursor.bytesRead_ = 0;
      }
      offset = cursor.descOffset_ + cursor.bytesRead_;
      bytesToCopy = tgtSize - operLen;
      #else */
      if (cursor.bytesRead_ == cursor.descSize_) // time to read next chunck
      {
         err = fetchCursor();
         if (err != LOB_OPER_OK) {
            return err;
         }

         if (request_.getCliError() == 100) {
            cursor.eod_ = true; // subsequent call will return 100 and close the cursor
            continue;
         } else {
            cursor.descSize_ = request_.getDesc().getSize();
            cursor.descOffset_ = request_.getDesc().getOffset();
            cursor.bytesRead_ = 0;
         }
      }

      bytesAvailable = cursor.descSize_ - cursor.bytesRead_;
      bytesToCopy = min(bytesAvailable, tgtSize - operLen);
      offset = cursor.descOffset_ + cursor.bytesRead_;
      // #endif

#ifdef SQ_USE_HDFS
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
#else
      bytesRead = pread(fdData_, tgt, bytesToCopy, cursor.descOffset_ + cursor.bytesRead_);
      if (bytesRead == -1) {
         return LOB_DATA_READ_ERROR;
      }
#endif

      cursor.bytesRead_ += bytesRead;
      operLen += bytesRead;
      tgt += bytesRead;
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
      //bytesRead = hdfsPread(fs_, fdData_, offset, tgt, bytesToCopy);
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

Ex_Lob_Error ExLob::readDataToMem(char *memAddr, Int64 &descNum,
                                  Int64 offset, Int64 size, Int64 &operLen)

{ 
  Ex_Lob_Error err;
  operLen = 0;
  NABoolean multipleChunks = FALSE;
  ExLobDesc *arrayOfDescs=NULL;
  ExLobDesc desc;
  err = getDesc(desc);
  if (getRequest()->getBlackBoxLen() == -1)
    {
      multipleChunks = TRUE;
     
      err = openCursor(getRequest()->getHandleIn(), 
		       getRequest()->getHandleInLen());
      //now we can fetch the descriptors for each chunk
    }
  else
    if (err != LOB_OPER_OK)
      return err;
   
  int cliErr = request_.getCliError();
  if (cliErr < 0 || cliErr == 100) {
    return LOB_DESC_READ_ERROR;
  }

 
#ifdef SQ_USE_HDFS

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
     
      if (!multipleChunks)
	{
	  if (hdfsPread(fs_, fdData_, desc.getOffset() + offset, 
			memAddr, desc.getSize()) == -1) {
	  
	    return LOB_DATA_READ_ERROR;
	  }
#else
	  if (pread(fdData_, memAddr, desc.getSize(), desc.getOffset() + offset) == -1) {
	    return LOB_DATA_READ_ERROR;
	  }

#endif
      
	  operLen = desc.getSize();
	  return LOB_OPER_OK;
	}
      else
	{
	  //handle reading the multiple chunks like a cursor
	  err = readCursor(memAddr,size, getRequest()->getHandleIn(),
			   getRequest()->getHandleInLen(), operLen);
	 
	 
	  if (err==LOB_OPER_OK)
	    closeCursor(getRequest()->getHandleIn(), 
			getRequest()->getHandleInLen());
	  else
	    return err;
	}
      return LOB_OPER_OK;
}

Ex_Lob_Error ExLob::readDataFromFile(char *memAddr, Int64 len, Int64 &operLen)
{
   int bytesRead;

#ifdef SQ_USE_HDFS
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
   bytesRead = hdfsPread(fs_, fdData_, 0, memAddr, len);
#else
   bytesRead = pread(fdData_, memAddr, len, 0); 
#endif

   if (bytesRead == -1) {
      operLen = 0;
      return LOB_DATA_READ_ERROR;
   } else {
      operLen = bytesRead;
   }

   return LOB_OPER_OK;
}

#ifdef SQ_USE_HDFS
Ex_Lob_Error ExLob::readDataToFile(char *fileName, Int64 &descNum, Int64 offset, Int64 &operLen)
#else
Ex_Lob_Error ExLob::readDataToFile(char *fileName, Int64 &descNum, Int64 offset, Int64 &operLen)
#endif
{ 
    Ex_Lob_Error err;

    ExLobDesc desc;
    err = getDesc(desc);
    if (err != LOB_OPER_OK)
      return err;

    if (offset > desc.getSize())
    {
      operLen = 0;
      return LOB_OPER_OK;
    }

    char *data = (char *)memalign(512, desc.getSize());
    if (data == NULL) {
      return LOB_SOURCE_DATA_ALLOC_ERROR;
    }

#ifdef SQ_USE_HDFS
    // tbd
#else
    if (pread(fdData_, data, desc.getSize(), desc.getOffset()) == -1) {
      free(data);
      return LOB_DATA_READ_ERROR;
    }
#endif

    int openFlags = O_CREAT | O_RDWR ; // O_DIRECT needs mem alignment
    int filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

    int fdDestFile = open(fileName, openFlags, filePerms);
    if (fdDestFile == -1) {
      return LOB_DEST_FILE_OPEN_ERROR;
    }

    if (pwrite(fdDestFile, data, desc.getSize(), 0) == -1) {
      free(data);
      close(fdDestFile);
      return LOB_DATA_WRITE_ERROR;
    }

    operLen = desc.getSize();
    descNum = desc.getNextDescNum();

    free(data);
    close(fdDestFile);

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

Ex_Lob_Error ExLobDesc::print()
{
    printf("%4d %4d  %4d %4d %4d %4d %8d\n",
           dataSize_, dataState_, tail_, prev_, next_, nextFree_, dataOffset_);
    return LOB_OPER_OK;
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
    heap_(NULL)
{
  //initialize the log file
  if (getenv("TRACE_HDFS_THREAD_ACTIONS"))
    {
      char logFileName[50]= "";
      sprintf(logFileName,"trace_threads.%d",getpid());
      threadTraceFile_ = fopen(logFileName,"a");
    }
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

Ex_Lob_Error ExLobGlobals::setServerPhandle()
{
    int nid;
    
    int err = msg_mon_get_my_info(&nid, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    char server[12];
    sprintf(server, "%s%d", "$ZLOBSRV", nid);

    int oid;
    err = msg_mon_open_process(server, &serverPhandle, &oid);

    if (err != XZFIL_ERR_OK)
      return LOB_SERVER_OPEN_ERROR;

    return LOB_OPER_OK;
}

Ex_Lob_Error ExLobGlobals::resetServerPhandle()
{
   Ex_Lob_Error err;

   msg_mon_close_process(&serverPhandle);

   err = setServerPhandle();

   return err;
}

// called once per process
Ex_Lob_Error ExLobGlobals::initialize()
{
    Ex_Lob_Error err = LOB_OPER_OK;

    lobMap_ = (lobMap_t *) new (getHeap())lobMap_t; // Leaving this allocated from system heap. Since this class contains hdfsFS unable to derive from LOB heap
    if (lobMap_ == NULL)
      return LOB_INIT_ERROR;

    
    err = setServerPhandle();
    

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

       if (err == XZFIL_ERR_PATHDOWN) {
	 //lobGlobals->resetServerPhandle();
       }

    } while ( (err == XZFIL_ERR_PATHDOWN) && (retries > 0) ); // 201 if lobserver got restared

    status_ = LOB_OPER_REQ_DONE;

    if (err != XZFIL_ERR_OK)
      return LOB_SEND_MSG_ERROR;

    memcpy(this, &rep_data, rep_data_max);
  
    return LOB_OPER_OK;
}

void ExLobRequest::getDescOut(ExLobDesc &desc) 
{ 
   memcpy(&desc, &desc_, sizeof(ExLobDesc)); 
}

void ExLobRequest::putDescIn(ExLobDesc &desc) 
{ 
   memcpy(&desc_, &desc, sizeof(ExLobDesc)); 
}

///////////////////////////////////////////////////////////////////////////////
// ExLobHdfs definitions
///////////////////////////////////////////////////////////////////////////////
#ifdef SQ_USE_HDFS

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

#endif

Ex_Lob_Error ExLobsOper (
    char        *lobName,          // lob name
    char        *handleIn,         // input handle (for cli calls)
    Int64       handleInLen,       // input handle len
    char        *hdfsServer,       // server where hdfs fs resides
    Int64       hdfsPort,          // port number to access hdfs server
    char        *handleOut,        // output handle (for cli calls)
    Int64       &handleOutLen,     // output handle len
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
    Int64 cursorBytes,
    char *cursorId,
    LobsOper    operation,         // LOB operation
    LobsSubOper subOperation,      // LOB sub operation
    Int64       waited,            // waited or nowaited
    void        *&globPtr,         // ptr to the Lob objects. 
    Int64       transId,
    void        *blackBox,         // black box to be sent to cli
    Int64       blackBoxLen,       // length of black box
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

    retOperLen = 0;
    ExLobDesc desc;
    
    lobMap_t *lobMap = NULL;
    lobMap_it it;

    clock_gettime(CLOCK_MONOTONIC, &startTime);

    char *fileName = lobName;

    if (globPtr == NULL)
    {
      if (operation == Lob_Init)
      {
        globPtr = (void *) new ExLobGlobals();
        if (globPtr == NULL) 
          return LOB_INIT_ERROR;

        lobGlobals = (ExLobGlobals *)globPtr;

        err = lobGlobals->initialize(); 
        return err;
      }
      else
      {
        return LOB_GLOB_PTR_ERROR;
      }
    }
    else
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

        err = lobPtr->initialize(fileName, (operation == Lob_Create) ? EX_LOB_CREATE : EX_LOB_RW, 
                                 dir, storage, hdfsServer, hdfsPort, bufferSize, replication, blockSize,
                                 lobGlobals);
        if (err != LOB_OPER_OK)
          return err;

        lobMap->insert(pair<string, ExLob*>(string(fileName), lobPtr));
      }
      else
      {
        lobPtr = it->second;
        
#ifndef SQ_USE_HDFS
        err = lobPtr->doSanityChecks(dir, storage, handleInLen, handleOutLen, blackBoxLen);
        if (err != LOB_OPER_OK)
          return err;
#endif
      }
    }
   
	MS_Mon_Transid_Type transIdBig;
 if (!lobGlobals->isHive())
      {
	// get current transaction
   
	int transIdErr = ms_transid_get(false, false, &transIdBig);

	// set the pass thru request object values in the lob
    
	lobPtr->getRequest()->setValues(lobPtr->getDescFileName(),
                                    descNumIn, handleInLen, handleIn, storage,
                                    transId, transIdBig,
                                    (char *)blackBox, blackBoxLen);
      }
    switch(operation)
    {
      case Lob_Create:
        break;

      case Lob_InsertDesc:
        err = lobPtr->writeDesc(sourceLen, descNumOut, retOperLen);
        break;

      case Lob_InsertData:
        err = lobPtr->writeData(source, sourceLen, subOperation, descNumIn, retOperLen);
        break;

      case Lob_InsertDataSimple:
        err = lobPtr->writeDataSimple(source, sourceLen, subOperation, retOperLen,
                                        bufferSize , replication , blockSize);
        break;

      case Lob_Read:
        if (subOperation == Lob_Memory)
          err = lobPtr->readToMem(source, sourceLen, descNumIn, retOperLen);
        else if (subOperation == Lob_File)
          err = lobPtr->readToFile(source, descNumIn, retOperLen);
        else  
          err = LOB_SUBOPER_ERROR;
        break;

      case Lob_ReadDesc: // read desc only. Needed for pass thru.
        err = lobPtr->getDesc(desc);
        retOperLen = 0;
        break;

      case Lob_ReadData: // read data only. Lob data file is already opened. 
        err = lobPtr->readDataFromFile(source, sourceLen, retOperLen);
        break;

      case Lob_ReadDataSimple:
        err = lobPtr->readDataFromFile(source, sourceLen, retOperLen);
        break;

      case Lob_OpenCursor:
        err = lobPtr->openCursor(handleIn, handleInLen);
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
        if (subOperation == Lob_Memory)
          err = lobPtr->readCursor(source, sourceLen, handleIn, handleInLen, retOperLen);
        else if (subOperation == Lob_File)
          err = lobPtr->readCursor(source, -1, handleIn, handleInLen, retOperLen);
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
        if (subOperation == Lob_Memory)
          err = lobPtr->append(source, sourceLen, descNumIn, retOperLen);
        else if (subOperation == Lob_File)
          err = lobPtr->append(source, -1, descNumIn, retOperLen);
        else  
          err = LOB_SUBOPER_ERROR;
        break;

       case Lob_Update:
        if (subOperation == Lob_Memory)
          err = lobPtr->update(source, sourceLen, descNumIn, retOperLen);
        else  
          err = LOB_SUBOPER_ERROR;
        break;

      case Lob_Delete:
        err = lobPtr->delDesc();
        break;

      case Lob_Drop:
        err = lobPtr->purgeLob();
        it = lobMap->find(string(lobName));
        lobMap->erase(it);
        delete lobPtr;
        lobPtr = NULL;
        break;

      case Lob_Purge:
        err = lobPtr->purgeLob();
        it = lobMap->find(string(lobName));
        lobMap->erase(it);
        delete lobPtr;
        lobPtr = NULL;
        break;

      case Lob_Print:
        err = lobPtr->print();
        break;

      case Lob_Stats:
      err = lobPtr->readStats(source);
      lobPtr->initStats(); // because file may remain open across cursors
      break;

      case Lob_Empty_Directory:
        lobPtr->initialize(fileName, EX_LOB_RW,
                          dir, storage, hdfsServer, hdfsPort, bufferSize, replication, blockSize);
        err = lobPtr->emptyDirectory();
        break;

      case Lob_Cleanup:
        delete lobGlobals;
        break;

      default:
        err = LOB_OPER_ERROR;
        break;
    }

    if (!lobGlobals->isHive() )
      {
	if (lobPtr)
	  // set the pass thru request object values from the lob
	  lobPtr->getRequest()->getValues(descNumOut, handleOutLen, handleOut, 
					  requestStatus, cliError,
					  (char *)blackBox, blackBoxLen);    // reinstate the transaction
	if (TRANSID_IS_VALID(transIdBig)) {
	  ms_transid_reinstate(transIdBig);
	}
      }

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
  fs = hdfsConnect(lobHdfsServer, lobHdfsPort);
  if (fs == NULL)
    return;
  // Get this list of all data and desc files in the lob sotrage location
  hdfsFileInfo *fileInfos = hdfsListDirectory(fs, lobHdfsLoc, &numExistingFiles);
  if (fileInfos == NULL)
    return ;
  //Delete each one in a loop
  for (int i = 0; i < numExistingFiles; i++)      
    hdfsDelete(fs, fileInfos[i].mName, 0);
    
  // *Note* : delete the memory allocated by libhdfs for the file info array  
  if (fileInfos)
    {
      hdfsFreeFileInfo(fileInfos, numExistingFiles);
    }
}
