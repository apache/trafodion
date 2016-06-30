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
 * File:         ex_lob.h
 * Description:  header file for LOB storage and retrieval
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
#ifndef EXP_LOB_ACCESS_H
#define EXP_LOB_ACCESS_H

#include <string>
#include <map>
#include <list>

#include "ExpLOBenums.h"
#include "ExpLOBstats.h"
#include "ExpLOBinterface.h"
#include "ComSmallDefs.h"
#include "Globals.h"
#include <seabed/ms.h>
#include <seabed/fs.h>

#ifdef LOB_DEBUG_STANDALONE
#define Int64 long long
#define Lng32 long
#else
#endif

#define SQ_USE_HDFS 1



#include "hdfs.h"




using namespace std;


#define MAX_LOB_FILE_NAME_LEN 256
#define MAX_HANDLE_IN_LEN 1024
#define MAX_HANDLE_OUT_LEN 1024
#define MAX_BLACK_BOX_LEN 2048
#define LOB_DESC_HEADER_KEY 1
#define NUM_WORKER_THREADS 2 
// 2 threads at most, one to read and the other to pick up next read from preOpen

#define LOB_CURSOR_PREFETCH_BYTES_MAX (1 << 27) // 128MB

class ExLobGlobals;
// This class defines the request used to construct the message to send over 
// to the mxlobsrvr process. It's currently not used. All lob functionailty is 
// now in te master process. We are retaining it here for furture use.
class ExLobRequest
{
  public:
    ExLobRequest();
    ~ExLobRequest();
    Ex_Lob_Error send();

    void setValues(char *descFileName, Int64 descNumIn, Int64 handleInLen, 
                   char *handleIn, LobsStorage storage, Int64 transId,
                   SB_Transid_Type transIdBig,
                   SB_Transseq_Type transStartId,
                   char *blackBox, Int64 blackBoxLen);
    void getValues(Int64 &descNumOut, Int64 &handleOutLen, 
                   char *handleOut, Ex_Lob_Error &requestStatus,
                   Int64 &cliError,
                   char *blackBox, Int64 &blackBoxLen);

    Int64 getDescNumIn() { return descNumIn_; }
    void setDescNumIn(Int64 descNum) { descNumIn_ = descNum; }
    Int64 getDescNumOut() { return descNumOut_; }
    void setDescNumOut(Int64 descNum) { descNumOut_ = descNum; }
    Int64 getDataOffset() { return dataOffset_; }
    void setDataOffset(int dataOffset) { dataOffset_ = dataOffset; }
    LobsRequest getType() { return type_; }
    void setType(LobsRequest type) { type_ = type; }
    char *getDescFileName() { return descFileName_; }
    Int64 getOperLen() { return operLen_; }
    void setOperLen(Int64 len) { operLen_ = len; }
    void log();   
    void setError(Ex_Lob_Error err) { error_ = err; }
    Ex_Lob_Error getError() { return error_; }
    Ex_Lob_Error getStatus() { return status_; }
    char *getHandleIn() { return handleIn_; }
    Int64 getHandleInLen() { return handleInLen_; }
    char *getHandleOut() { return handleOut_; }
    void setHandleOutLen(Lng32 len) { handleOutLen_ = len; }
    void setCliError(int cliErr) { cliError_ = cliErr; }
    int getCliError() { return (int)cliError_; }
    Int64 getTransId() { return transId_; }
    SB_Transid_Type getTransIdBig() { return transIdBig_; }
    SB_Transseq_Type getTransStartId() { return transStartId_; }
    Int64 getBlackBoxLen() { return blackBoxLen_; };
    void setBlackBoxLen(Int64 len) { blackBoxLen_ = len; }
    char *getBlackBox() { return blackBox_; }
    
    void incrReqNum() { reqNum_++; }
    Int64 getReqNum() { return reqNum_; }

  private:
    Int64 reqNum_;
    Int64 descNumIn_;
    Int64 descNumOut_;
    char handleIn_[MAX_HANDLE_IN_LEN];
    Int64 handleInLen_;
    char handleOut_[MAX_HANDLE_OUT_LEN];
    Int64 handleOutLen_;
    Int64 dataOffset_;
    LobsRequest type_;
    LobsStorage storage_;
    Int64 operLen_;
    Ex_Lob_Error error_;
    Int64 cliError_;
    Ex_Lob_Error status_;
    Int64 transId_;
    SB_Transid_Type transIdBig_;
    SB_Transseq_Type transStartId_;
    char descFileName_[MAX_LOB_FILE_NAME_LEN];
    char blackBox_[MAX_BLACK_BOX_LEN];
    Int64 blackBoxLen_;
};

///////////////////////////////////////////////////////////////////////////////
// THE SQL LOB API
///////////////////////////////////////////////////////////////////////////////

Ex_Lob_Error ExLobsOper (
    char        *lobName,          // lob name
    char        *handleIn,         // input handle (for cli calls)
    Int32       handleInLen,       // input handle len
    char       *hdfsServer,       // server where hdfs fs resides
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
    Int64 cursorBytes,
    char *cursorId,
    LobsOper    operation,         // LOB operation
    LobsSubOper subOperation,      // LOB sub operation
    Int64       waited,            // waited or nowaited
    void        *&globPtr,         // ptr to the Lob objects. 
    Int64       transId,
    void        *blackBox,         // black box to be sent to cli
    Int32       blackBoxLen,        // length of black box
    Int64       lobMaxSize = 0,        // max size of lob.
    Int64       lobMaxChunkMemSize = 0 ,//max length of intermediate mem buffer used to do i/o.
    Int64 lobGCLimit = 0, // size at which GC must be triggered
    int         bufferSize =0,
    short       replication =0,
    int         blocksize=0,
    Lng32       openType=0

);

class ExLobLock
{
  public:
    ExLobLock();
    ~ExLobLock();

    void lock();
    void unlock();
    void wakeOne();
    void wakeAll();
    void wait();
    Int64 waiters() { return waiters_; }

  private:
    pthread_mutex_t mutex_;
    pthread_cond_t workBell_;
    Int64 waiters_;
    bool bellRang_;
};

///////////////////////////////////////////////////////////////////////////////
// ExLobDescHeader
///////////////////////////////////////////////////////////////////////////////

class ExLobDescHeader
{
  public:
    
    ExLobDescHeader(unsigned int size);
    ExLobDescHeader() { };
    ~ExLobDescHeader();

    int getFreeDesc() { return freeDesc_; }
    void incFreeDesc() { freeDesc_++; }
    unsigned int getAvailSize() { return availSize_; }
    void decAvailSize(int size) { availSize_ -= size; }
    int getDataOffset() { return dataOffset_; }
    void incDataOffset(int size) { dataOffset_ += size; }
    
  

  private:

    int freeDesc_; 
    int delDesc_;
    int dataOffset_;
    unsigned int availSize_;
};

///////////////////////////////////////////////////////////////////////////////
// ExLobDesc
///////////////////////////////////////////////////////////////////////////////

class ExLobDesc
{
  public:
    ExLobDesc(int offset, int size, int tail);
  ExLobDesc() :
    dataOffset_(0),
      dataSize_(0),
      dataState_(0),
      tail_(0),
      next_(0),
      prev_(0),
      nextFree_(0){};
    ~ExLobDesc();

    int getTailDescNum() { return tail_; }  
    void setTailDescNum(int tail) { tail_ = tail; }
    int getNextDescNum() { return next_; }
    void setNextDescNum(int next) { next_ = next; }
    void setPrevDescNum(int prev) { prev_ = prev; }
    int getPrevDescNum() { return prev_; }
    int getSize() { return dataSize_; }
    void setSize(int size) { dataSize_ = size; }
    int getOffset() { return dataOffset_; }
    void setOffset(int offset) { dataOffset_ = offset; }
    void setDataState(int dataState) { dataState_ = dataState; }


  private:

    int dataOffset_;
    int dataSize_;
    int dataState_;
    int tail_;
    int next_;
    int prev_;
    int nextFree_;
};

class ExLobInMemoryDescChunksEntry
{
public:
  ExLobInMemoryDescChunksEntry():
       currentOffset_ (-1),
       newOffset_( -1),
       descPartnKey_(-1),
       descSyskey_(-1),
       chunkLen_(-1),
       chunkNum_(-1)
  {}
    ExLobInMemoryDescChunksEntry(Int64 co,Int64 partnKey,Int64 syskey,Int64 chunkLen,Int32 chunkNum)
  {
    currentOffset_ = co; newOffset_ = -1;descPartnKey_ = partnKey; descSyskey_ = syskey; chunkLen_ = chunkLen; chunkNum_ =chunkNum;
  }

  void setCurrentOffset(Int64 co) { currentOffset_ = co;}
  Int64 getCurrentOffset(){return currentOffset_;}
  
  void setNewOffset(Int64 no) { newOffset_ = no;}
  Int64 getNewOffset(){return newOffset_;}

  void setDescPartnKey(Int64 dk) { descPartnKey_ = dk;}
  Int64 getDescPartnKey(){return descPartnKey_;}

  void setSyskey(Int64 sk) { descSyskey_ = sk;}
  Int64 getSyskey(){return descSyskey_;}

  void setChunkLen(Int64 cl) { chunkLen_ = cl;}
  Int64 getChunkLen(){return chunkLen_;}

  void setChunkNum(Int32 cn) { chunkNum_ = cn;}
  Int32 getChunkNum(){return chunkNum_;}
private:
  Int64 currentOffset_;
  Int64 newOffset_;
  Int64 descPartnKey_;
  Int64 descSyskey_;
  Int64 chunkLen_;
  Int32 chunkNum_;
};

class ExLobCursorBuffer
{
  public:
    ExLobCursorBuffer() :
      data_(NULL),
      bytesRemaining_(-1),
      bytesUsed_(-1)
    { 
    }

    ~ExLobCursorBuffer() 
    { 
      // do nothing here as this object is copied between lists. 
    }

  public:
    char *data_;
    Int64 bytesRemaining_;
    Int64 bytesUsed_; 
};

class ExLobCursor
{
public:

   ExLobCursor() { } 
   ~ExLobCursor(){ };
   void emptyPrefetchList(ExLobGlobals *lobGlobals);

public: 
   void *cliInterface_; 	// passed back and forth for cursors.
   Int64 bytesRead_;    	// bytes already read
   Int64 descOffset_;   	// from this desc offset
   Int64 descSize_;     	// which is of this max size.
   bool eod_;           	// end of file, error 100 from cli.
   bool eor_;                   // end of range
   bool eol_;                   // end of life                         
   LobsCursorType type_;	// simple or multiple
   Int64 bufMaxSize_;           // max size of buffer
   Int64 maxBytes_;             // bytesLeft to prefetch
   Int64 prefetch_;             // prefetch or not to prefetch
   char name_[MAX_LOB_FILE_NAME_LEN]; 
                               
   Lng32 currentRange_;		// current index of file for multi cursor
   Lng32 endRange_;		    // end index of file for multi cursor
   struct hdfsFile_internal* currentFd_; // file pointed by currentRange_ 
   bool currentEod_; 		  // eod of currentFd_
   Int64 currentStartOffset_;	// start offset in currentFd_
   Int64 currentBytesToRead_;	// bytes to read in currentFd_
   Int64 currentBytesRead_;	  // bytes read in currentFd_
                            
   ExLobLock lock_;         // lock for this cursor
   typedef list<ExLobCursorBuffer *> bufferList_t;
   bufferList_t prefetchBufList_;
   
   struct timespec openTime_;
   struct timespec closeTime_; 

   Int64 bufferHits_;
   Int64 bufferMisses_;
};

typedef class ExLobCursor cursor_t;

typedef map<string, cursor_t> lobCursors_t; // handle, offset
typedef map<string, cursor_t>::iterator lobCursors_it;
void cleanupLOBDataDescFiles(const char *hdfsServer, int hdfsPort, const char *hdfsLoc);

///////////////////////////////////////////////////////////////////////////////
// ExLob 
///////////////////////////////////////////////////////////////////////////////

class ExLob
{
  public:
    
    ExLob();  // default constructor
    ~ExLob(); // default desctructor

    Ex_Lob_Error initialize(char *lobFile, Ex_Lob_Mode mode, char *dir, 
                            LobsStorage storage, char *hdfsServer, Int64 hdfsPort,
                            char *lobLocation,
                            int bufferSize = 0, short replication =0, 
                            int blocksize=0, Int64 lobMaxSize = 0, 
                            ExLobGlobals *lobGlobals = NULL);
    Ex_Lob_Error initialize(char *lobFile);
  Ex_Lob_Error writeDesc(Int64 &sourceLen, char *source, LobsSubOper subOperation, Int64 &descNumOut, Int64 &operLen, Int64 lobMaxSize, Int64 lobMaxChunkMemSize,Int64 lobGCLimit, char * handleIn, Int32 handleInLen, char *blackBox, Int32 *blackBoxLen, char * handleOut, Int32 &handleOutLen, void *lobGlobals);
    Ex_Lob_Error writeLobData(char *source, Int64 sourceLen, 
			      LobsSubOper subOperation, 
			      Int64 tgtOffset,Int64 &operLen, 
			      Int64 lobMaxMemChunkLen);
    Ex_Lob_Error writeDataSimple(char *data, Int64 size, LobsSubOper subOperation, Int64 &operLen,
                                 int bufferSize = 0, short replication =0, int blocksize=0);
    Ex_Lob_Error readToMem(char *memAddr, Int64 size,  Int64 &operLen,char * handleIn, Int32 handleInLen, char *blackBox, Int32 blackBoxLen, char * handleOut, Int32 &handleOutLen, Int64 transId);
  Ex_Lob_Error readToFile(char *fileName, Int64 tgtLen,Int64 &operLen,Int64 lobMaxChunkMemLen, Int32 fileflags, char *handleIn,Int32 handleInLen, char *blackBox, Int32 blackBoxLen, char * handleOut, Int32 &handleOutLen, Int64 transId);
  Ex_Lob_Error readCursor(char *tgt, Int64 tgtSize, char *handleIn, Int32 handleInLen, Int64 &operLen,Int64 transId);
  Ex_Lob_Error readCursorData(char *tgt, Int64 tgtSize, cursor_t &cursor, Int64 &operLen,char *handleIn, Int32 handeLenIn,Int64 transId);
    Ex_Lob_Error readCursorDataSimple(char *tgt, Int64 tgtSize, cursor_t &cursor, Int64 &operLen);
    Ex_Lob_Error readDataCursorSimple(char *fileName, char *tgt, Int64 tgtSize, Int64 &operLen, ExLobGlobals *lobGlobals);
    bool hasNoOpenCursors() { return lobCursors_.empty(); }
  Ex_Lob_Error openCursor(char *handleIn, Int32 handleInLen,Int64 transId);
    Ex_Lob_Error openDataCursor(char *fileName, LobsCursorType type, Int64 range, 
                                Int64 bytesLeft, Int64 bufMaxSize, Int64 prefetch, ExLobGlobals *lobGlobals);
    Ex_Lob_Error deleteCursor(char *cursorName, ExLobGlobals *lobGlobals);
  Ex_Lob_Error fetchCursor(char *handleIn, Int32 handleLenIn, Int64 &outOffset, Int64 &outSize,NABoolean &isEOD,Int64 transId);
  Ex_Lob_Error insertData(char *data, Int64 size, LobsSubOper so,Int64 headDescNum, Int64 &operLen, Int64 lobMaxSize, Int64 lobMaxChunkMemSize,char *handleIn,Int32 handleInLen, char *blackBox, Int32 blackBoxLen, char * handleOut, Int32 &handleOutLen, void *lobGlobals);
  Ex_Lob_Error append(char *data, Int64 size, LobsSubOper so, Int64 headDescNum, Int64 &operLen, Int64 lobMaxSize, Int64 lobMaxChunkMemLen,Int64 lobGCLimit, char *handleIn,Int32 handleInLen, char * handleOut, Int32 &handleOutLen, void *lobGlobals);
  Ex_Lob_Error update(char *data, Int64 size, LobsSubOper so,Int64 headDescNum, Int64 &operLen, Int64 lobMaxSize,Int64 lobMaxChunkMemLen,Int64 lobGCLimit,char *handleIn,Int32 handleInLen, char * handleOut, Int32 &handleOutLen, void *lobGlobals);
  Ex_Lob_Error readSourceFile(char *srcfile, char *&fileData, Int32 &size, Int64 offset);
  Ex_Lob_Error readHdfsSourceFile(char *srcfile, char *&fileData, Int32 &size, Int64 offset);
  Ex_Lob_Error readLocalSourceFile(char *srcfile, char *&fileData, Int32 &size, Int64 offset);
  Ex_Lob_Error readExternalSourceFile(char *srcfile, char *&fileData, Int32 &size, Int64 offset);
  Ex_Lob_Error statSourceFile(char *srcfile, Int64 &sourceEOF);
  Ex_Lob_Error delDesc(char *handleIn, Int32 handleInLen, Int64 transId);
  Ex_Lob_Error purgeLob();
  Ex_Lob_Error closeFile();
  LobInputOutputFileType fileType(char *ioFileName);
  Ex_Lob_Error closeCursor(char *handleIn, Int32 handleInLen);
  Ex_Lob_Error closeDataCursorSimple(char *fileName, ExLobGlobals *lobGlobals);
  
  Ex_Lob_Error doSanityChecks(char *dir, LobsStorage storage,
                              Int32 handleInLen, Int32 handleOutLen, 
                              Int32 blackBoxLen);
  Ex_Lob_Error allocateDesc(unsigned int size, Int64 &descNum, Int64 &dataOffset,Int64 lobMaxSize,Int64 lobMaxChunkMemSize, char *handleIn, Int32 handleInLen,Int64 lobGCLimit, void *lobGlobals);
  Ex_Lob_Error readStats(char *buffer);
  Ex_Lob_Error initStats();
  
  Ex_Lob_Error insertDesc(Int64 offset, Int64 size,  char *handleIn, Int32 handleInLen,  char *handleOut, Int32 &handleOutLen, char *blackBox, Int32 blackBoxLen,void *lobGlobals) ;
  
  Ex_Lob_Error lockDesc();
  Ex_Lob_Error unlockDesc();
  char *getDataFileName() { return lobDataFile_; }
  
  int getErrNo();
  
  
  Ex_Lob_Error getDesc(ExLobDesc &desc,char * handleIn, Int32 handleInLen, char *blackBox, Int32 *blackBoxLen, char * handleOut, Int32 &handleOutLen, Int64 transId);
  
  Ex_Lob_Error writeData(Int64 offset, char *data, Int32 size, Int64 &operLen);
  Ex_Lob_Error readDataToMem(char *memAddr, Int64 offset, Int64 size,
                             Int64 &operLen,char *handleIn, Int32 handleLenIn, 
                             NABoolean multipleChunks, Int64 transId);
  
  Ex_Lob_Error readDataToLocalFile(char *fileName, Int64 offset, Int64 size,Int64 &operLen,Int64 lobMaxChunkMemLen ,Int32 fileFlags,char *handleIn,Int32 handleInLen, NABoolean multipleChunks,Int64 transId);
  Ex_Lob_Error readDataToHdfsFile(char *fileName, Int64 offset, Int64 size, Int64 &operLen,Int64 lobMaxChunkMemLen, Int32 fileflags,char *handleIn,Int32 handleInLen, NABoolean multipleChunks,Int64 transId);
  Ex_Lob_Error readDataToExternalFile(char *tgtFileName,  Int64 offset, Int64 size, Int64 &operLen, Int64 lobMaxChunkMemLen, Int32 fileflags,char *handleIn,Int32 handleInLen, NABoolean multipleChunks,Int64 transId);
  Ex_Lob_Error readDataFromFile(char *memAddr, Int64 len, Int64 &operLen);
  Ex_Lob_Error compactLobDataFile(ExLobInMemoryDescChunksEntry *dcArray,Int32 numEntries);
  Ex_Lob_Error  restoreLobDataFile();
  Ex_Lob_Error purgeBackupLobDataFile();

  // dirPath: path to needed directory (includes directory name)
  // modTS is the latest timestamp on any file/dir under dirPath.
  // This method validates that current modTS is not greater then input modTS.
  // Return: LOB_OPER_OK, if passes. LOB_DATA_MOD_CHECK_ERROR, if fails.
  Ex_Lob_Error dataModCheck(
       char * dirPath, 
       Int64  modTS,
       Lng32  numOfPartLevels,
       ExLobGlobals *lobGlobals);

  Ex_Lob_Error dataModCheck2(
       char * dirPath, 
       Int64  modTS,
       Lng32  numOfPartLevels);

  Ex_Lob_Error emptyDirectory(char* dirPath, ExLobGlobals* lobGlobals);

  ExLobStats *getStats() { return &stats_; }
  NAHeap *getLobGlobalHeap() { return lobGlobalHeap_;}
  ExLobRequest *getRequest() { return &request_; }
  
  //The next 2 functions are not active at this point. They serve as an example
  //on how to send requests across to the mxlobsrvr process from the master 
  //process
  Ex_Lob_Error getDesc(ExLobRequest *request);
  Ex_Lob_Error sendReqToLobServer() ;
  public:

    char lobDataFile_[MAX_LOB_FILE_NAME_LEN];
    lobCursors_t lobCursors_;
    ExLobLock lobCursorLock_;
    LobsStorage storage_;
    string dir_; // lob data directory
    char *hdfsServer_;
    Int64 hdfsPort_;
    char *lobLocation_;
    hdfsFS fs_;
    hdfsFile fdData_;
    int openFlags_;
    ExLobStats stats_;
    bool prefetchQueued_;
    NAHeap *lobGlobalHeap_;
    ExLobRequest request_;
    NABoolean lobTrace_;
};

typedef map<string, ExLob *> lobMap_t;
typedef map<string, ExLob *>::iterator lobMap_it;

///////////////////////////////////////////////////////////////////////////////
// ExLobHdfsRequest
///////////////////////////////////////////////////////////////////////////////

class ExLobHdfsRequest
{
  public:

    ExLobHdfsRequest(LobsHdfsRequestType reqType, hdfsFS fs, hdfsFile file, char *buffer, int size);
    ExLobHdfsRequest(LobsHdfsRequestType reqType, ExLobCursor *cursor);
    ExLobHdfsRequest(LobsHdfsRequestType reqType, ExLob *lobPtr, ExLobCursor *cursor);
    ExLobHdfsRequest(LobsHdfsRequestType reqType);
    ~ExLobHdfsRequest();

    bool isShutDown() { return reqType_ == Lob_Hdfs_Shutdown; }

  public :
    LobsHdfsRequestType reqType_;
    ExLob *lobPtr_;
    ExLobCursor *cursor_;
    hdfsFS fs_;
    hdfsFile file_; 
    char *buffer_;
    int size_;
    Ex_Lob_Error error_;
};


class ExLobPreOpen
{
  public :

    ExLobPreOpen(ExLob *lobPtr, char *cursorName, Int64 range, Int64 bufMaxSize, Int64 maxBytes, Int64 waited) 
      : lobPtr_(lobPtr),
        range_(range),
        bufMaxSize_(bufMaxSize),
        maxBytes_(maxBytes),
        waited_(waited)
    {
      strcpy(cursorName_, cursorName);
    }

    ~ExLobPreOpen();

  public :
    ExLob *lobPtr_; 
    char cursorName_[MAX_LOB_FILE_NAME_LEN + 16];
    Int64 range_;
    Int64 bufMaxSize_;
    Int64 maxBytes_;
    Int64 waited_;
};
void lobDebugInfo(const char *logMessage,Int32 errorcode,
                         Int32 line, NABoolean lobTrace);
typedef list<ExLobPreOpen *> preOpenList_t;
typedef list<ExLobPreOpen *>::iterator preOpenList_it;

class ExLobGlobals
{
  public :
  
    ExLobGlobals(); 
    ~ExLobGlobals();

    Ex_Lob_Error initialize();
    lobMap_t * getLobMap() { return lobMap_; }
    Ex_Lob_Error getLobPtr(char *lobName, ExLob *& lobPtr);
    Ex_Lob_Error delLobPtr(char *lobName);
    hdfsFS getHdfsFs() { return fs_; }
    void setHdfsFs(hdfsFS fs) { fs_ = fs; }
    Ex_Lob_Error startWorkerThreads();
    void doWorkInThread();
    ExLobHdfsRequest* getHdfsRequest();
    Ex_Lob_Error enqueueRequest(ExLobHdfsRequest *request);
    Ex_Lob_Error enqueuePrefetchRequest(ExLob *lobPtr, ExLobCursor *cursor);
    Ex_Lob_Error enqueueShutdownRequest();
    Ex_Lob_Error performRequest(ExLobHdfsRequest *request);
    Ex_Lob_Error addToPreOpenList(ExLobPreOpen *preOpenObj);
    Ex_Lob_Error processPreOpens();
    NABoolean isCliInitialized()
    {
      return isCliInitialized_;
    }
 
    void setCliInitialized()
    {
      isCliInitialized_ = TRUE;
    }
     NABoolean isHive()
    {
      return isHive_;
    }
 
    void setIsHive(NABoolean TorF)
    {
      isHive_ = TorF;
    }
    void setHeap(void * heap)
    {
      heap_ = (NAHeap *) heap;
    }
    NAHeap * getHeap()
    {
      return heap_;
    }
  void traceMessage(const char *logMessage, ExLobCursor *c, int line);
  
  public :
    lobMap_t *lobMap_;
    hdfsFS fs_;
    pthread_t threadId_[NUM_WORKER_THREADS];
    typedef list<ExLobHdfsRequest *> reqList_t;
    reqList_t reqQueue_;
    ExLobLock reqQueueLock_;
    preOpenList_t preOpenList_;
    ExLobLock preOpenListLock_;
    typedef list<ExLobCursorBuffer *> bufferList_t;
    bufferList_t postfetchBufList_;
    ExLobLock postfetchBufListLock_;
    NABoolean isCliInitialized_;
    NABoolean isHive_;
    FILE *threadTraceFile_;
    NAHeap *heap_;
    NABoolean lobTrace_;
};



#endif 
