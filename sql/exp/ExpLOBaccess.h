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

#include <seabed/ms.h>
#include <seabed/fs.h>

#ifdef LOB_DEBUG_STANDALONE
#define Int64 long long
#define Lng32 long
#else
#endif

#define SQ_USE_HDFS 1

#ifdef SQ_USE_HDFS

#include "hdfs.h"

#endif


using namespace std;

// 2GB max Lob data file size 
#define LOB_DATA_FILE_SIZE_MAX (1 << 31)

#define MAX_LOB_FILE_NAME_LEN 256
#define MAX_HANDLE_IN_LEN 1024
#define MAX_HANDLE_OUT_LEN 1024
#define MAX_BLACK_BOX_LEN 2048
#define LOB_DESC_HEADER_KEY 1
#define NUM_WORKER_THREADS 2 
// 2 threads at most, one to read and the other to pick up next read from preOpen

#define LOB_CURSOR_PREFETCH_BYTES_MAX (1 << 27) // 128MB

class ExLobGlobals;

///////////////////////////////////////////////////////////////////////////////
// THE SQL LOB API
///////////////////////////////////////////////////////////////////////////////

Ex_Lob_Error ExLobsOper (
    char        *lobName,          // lob name
    char        *handleIn,         // input handle (for cli calls)
    Int64       handleInLen,       // input handle len
    char       *hdfsServer,       // server where hdfs fs resides
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
    Int64       blackBoxLen,        // length of black box
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
    
    Ex_Lob_Error print(char *descFileName);

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
    ExLobDesc() { };
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

    Ex_Lob_Error print();

  private:

    int dataOffset_;
    int dataSize_;
    int dataState_;
    int tail_;
    int next_;
    int prev_;
    int nextFree_;
};

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
    void getDescOut(ExLobDesc &desc); 
    void putDescIn(ExLobDesc &desc); 
    ExLobDesc &getDesc() { return desc_; }
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
    ExLobDesc desc_;
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
                            int bufferSize = 0, short replication =0, int blocksize=0, ExLobGlobals *lobGlobals = NULL);
    Ex_Lob_Error initialize(char *lobFile);
    Ex_Lob_Error writeDesc(Int64 sourceLen, Int64 &descNumOut, Int64 &operLen);
    Ex_Lob_Error writeData(char *source, Int64 sourceLen, LobsSubOper subOperation, 
                           Int64 descNumIn, Int64 &operLen);
    Ex_Lob_Error writeDataSimple(char *data, Int64 size, LobsSubOper subOperation, Int64 &operLen,
                                 int bufferSize = 0, short replication =0, int blocksize=0);
    Ex_Lob_Error readToMem(char *memAddr, Int64 size, Int64 descNumIn, Int64 &operLen);
    Ex_Lob_Error readToFile(char *fileName, Int64 descNum, Int64 &operLen);
    Ex_Lob_Error readCursor(char *tgt, Int64 tgtSize, char *handleIn, Int64 handleInLen, Int64 &operLen);
    Ex_Lob_Error readCursorData(char *tgt, Int64 tgtSize, cursor_t &cursor, Int64 &operLen);
    Ex_Lob_Error readCursorDataSimple(char *tgt, Int64 tgtSize, cursor_t &cursor, Int64 &operLen);
    Ex_Lob_Error readDataCursorSimple(char *fileName, char *tgt, Int64 tgtSize, Int64 &operLen, ExLobGlobals *lobGlobals);
    bool hasNoOpenCursors() { return lobCursors_.empty(); }
    Ex_Lob_Error openCursor(char *handleIn, Int64 handleInLen);
    Ex_Lob_Error openDataCursor(char *fileName, LobsCursorType type, Int64 range, 
                                Int64 bytesLeft, Int64 bufMaxSize, Int64 prefetch, ExLobGlobals *lobGlobals);
    Ex_Lob_Error deleteCursor(char *cursorName, ExLobGlobals *lobGlobals);
    Ex_Lob_Error fetchCursor();
    Ex_Lob_Error selectCursorDesc(ExLobRequest *request);
    Ex_Lob_Error fetchCursorDesc(ExLobRequest *request);
    Ex_Lob_Error append(char *data, Int64 size, Int64 headDescNum, Int64 &operLen);
    Ex_Lob_Error append(ExLobRequest *request);
    Ex_Lob_Error update(char *data, Int64 size, Int64 headDescNum, Int64 &operLen);
    Ex_Lob_Error update(ExLobRequest *request);
    Ex_Lob_Error readSourceFile(char *srcfile, char *&fileData, int &size);
    Ex_Lob_Error delDesc();
    Ex_Lob_Error delDesc(ExLobRequest *request);
    Ex_Lob_Error purgeLob();
    Ex_Lob_Error closeFile();
    Ex_Lob_Error closeCursor(char *handleIn, Int64 handleInLen);
    Ex_Lob_Error closeDataCursorSimple(char *fileName, ExLobGlobals *lobGlobals);
    Ex_Lob_Error closeCursorDesc(ExLobRequest *request);
    Ex_Lob_Error doSanityChecks(char *dir, LobsStorage storage,
                                Int64 handleInLen, Int64 handleOutLen, 
                                Int64 blackBoxLen);
    Ex_Lob_Error allocateDesc(unsigned int size, Int64 &descNum, Int64 &dataOffset);
    Ex_Lob_Error readStats(char *buffer);
    Ex_Lob_Error initStats();

    Ex_Lob_Error insertDesc(ExLobRequest *request) ;

    ExLobRequest *getRequest() { return &request_; }

    Ex_Lob_Error lockDesc();
    Ex_Lob_Error unlockDesc();
    //int getfdDesc() { return fdDesc_; }
    hdfsFile *getfdDesc() { return &fdDesc_;}
    char *getDescFileName() { return lobDescFile_; }
    char *getDataFileName() { return lobDataFile_; }

    Ex_Lob_Error print();
    int getErrNo();

    Ex_Lob_Error delDesc(Int64 descNum);
    Ex_Lob_Error getDesc(ExLobDesc &desc);
    Ex_Lob_Error putDesc(ExLobDesc &desc, Int64 descNum);
    Ex_Lob_Error getDesc(ExLobRequest *request);

    Ex_Lob_Error writeData(Int64 offset, char *data, Int64 size, Int64 &operLen);
    Ex_Lob_Error readDataToMem(char *memAddr, Int64 &descNum, Int64 offset, Int64 size, Int64 &operLen);
    Ex_Lob_Error readDataToFile(char *fileName, Int64 &descNum, Int64 offset, Int64 &operLen);
    Ex_Lob_Error readDataFromFile(char *memAddr, Int64 len, Int64 &operLen);

    Ex_Lob_Error emptyDirectory();

    ExLobStats *getStats() { return &stats_; }
    NAHeap *getLobGlobalHeap() { return lobGlobalHeap_;}
  public:

    char lobDataFile_[MAX_LOB_FILE_NAME_LEN];
    char lobDescFile_[MAX_LOB_FILE_NAME_LEN];
    //int fdDesc_;
    hdfsFile fdDesc_;
    lobCursors_t lobCursors_;
    ExLobLock lobCursorLock_;
    LobsStorage storage_;
    string dir_; // lob data directory
#ifdef SQ_USE_HDFS
    char *hdfsServer_;
    Int64 hdfsPort_;
    hdfsFS fs_;
    hdfsFile fdData_;
    int openFlags_;
#endif
    ExLobRequest request_; 
    ExLobStats stats_;
    bool prefetchQueued_;
    NAHeap *lobGlobalHeap_;
};

typedef map<string, ExLob *> lobMap_t;
typedef map<string, ExLob *>::iterator lobMap_it;

///////////////////////////////////////////////////////////////////////////////
// ExLobHdfsRequest
///////////////////////////////////////////////////////////////////////////////
#ifdef SQ_USE_HDFS
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
#endif

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

typedef list<ExLobPreOpen *> preOpenList_t;
typedef list<ExLobPreOpen *>::iterator preOpenList_it;

class ExLobGlobals
{
  public :
  
    ExLobGlobals(); 
    ~ExLobGlobals();

    Ex_Lob_Error initialize();
    Ex_Lob_Error setServerPhandle();
    Ex_Lob_Error resetServerPhandle();
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
};

#endif 
