/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2003-2014 Hewlett-Packard Development Company, L.P.
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
/*
 * ExFastTransportIO.h
 *
 *  Created on: Aug 31, 2012
 */
#ifndef EXFASTTRANSPORTIO_H_
#define EXFASTTRANSPORTIO_H_
#include "ex_stdh.h"
#include "ex_error.h"
#include <stdio.h>
#include "ComSysUtils.h"
class FileReadWrite;
class ExFastExtractStats;

#ifndef UINT32_MAX
#define UINT32_MAX 4294967295UL;
#endif

void * processIO(void * param);

struct ErrorMsg
{
	char msg[1024];
	ExeErrorCode ec;
};

//SimpleQueue is implemented here to achieve the following:
//1. Make the queue thread safe.
//2. Avoid C/C++ library mallocs
//Credits to LGSM team for sharing most of this logic.
//Care should be taken to check for queue full/empty
//before insert/removehead calls are made.
class SimpleQueue
{
public:
  class Entry
  {
  public:
    void *getData() { return data_; }
    void setData(void *data) { data_ = data; }
  protected:
    void *data_;
  };

  SimpleQueue(); // Do not implement

  Entry &getTailEntry() const
  {
    return queue_[tail_ & mask_];
  }

  UInt32 getTailIndex() const
  {
    return tail_;
  }

  void insert( ErrorMsg *em )
  {
    // We assert that queue is never full when insert() is called. The
    // assert will only fail if the caller does not test isFull()
    // before calling insert().
	//update: replacing assert with a diagnostic.
    if(isFull())
    {
    	snprintf(em->msg, 1024, "SimpleQueue: insert called when queue is full");
    	em->ec = EXE_EXTRACT_ERROR_WRITING_TO_FILE;
    	return;
    }
    asm volatile("" : : : "memory");
    tail_++;
  }

  Entry &getQueueEntry(UInt32 i, ErrorMsg *em) const
  {
    if(isVacant(i))
	{
		snprintf(em->msg, 1024, "SimpleQueue: getQueueEntry called for vacant entry");
		em->ec = EXE_EXTRACT_ERROR_WRITING_TO_FILE;
	}
    return queue_[i & mask_];
  }

  Entry &getHeadEntry(ErrorMsg *em) const
  {
    if(isEmpty())
	{
		snprintf(em->msg, 1024, "SimpleQueue: getHeadEntry called for empty queue");
		em->ec = EXE_EXTRACT_ERROR_WRITING_TO_FILE;
	}
    return queue_[head_ & mask_];
  }

  UInt32 getHeadIndex() const
  {
    return head_;
  }

  bool entryExists(UInt32 i) const
  {
    return (!isVacant(i));
  }

  void removeHead(ErrorMsg *em)
  {
    // We assert that queue is never empty when removeHead() is
    // called. The assert will only fail if the caller does not test
    // isEmpty() before calling removeHead().
    if(isEmpty())
	{
		snprintf(em->msg, 1024, "SimpleQueue: removeHead called for empty queue");
		em->ec = EXE_EXTRACT_ERROR_WRITING_TO_FILE;
		return;
	}
    asm volatile("" : : : "memory");
    head_++;
  }

  UInt32 getSize() const { return size_; }
  UInt32 getLength() const { return tail_ - head_; }

  bool isFull() const { return (getLength() == mask_); }
  bool isEmpty() const { return (head_ == tail_); }

  bool isVacant(UInt32 i) const
  {
    // We have two cases to consider
    // * There is no wraparound and head_ <= tail_
    // * tail_ has wrapped around before head_
    if (head_ <= tail_)
      return !((head_ <= i) && (i < tail_));
    else
      return !((head_ <= i) || (i < tail_));
  }

  void cleanup(void)
  {
	  if (queue_)
	      heap_->deallocateMemory(queue_);
	  queue_ = NULL;
  }

  SimpleQueue(UInt32 initialSize, NAMemory *heap, ErrorMsg *em)
    : head_(0),
      tail_(0),
      size_(initialSize),
      mask_(0),
      queue_(NULL),
      heap_(heap)
  {
    // We want to raise size_ to the next highest power of 2. This
    // allows for fast modulo arithmetic when we index the circular
    // array of queue entries. Two steps are required:
    // * N = the number of significant bits in size_
    // * a bit-shift to compute (2 ** N)

	UInt32 significantBits = 1;
	UInt32 s = size_ - 1;
    while (s > 1)
    {
      significantBits++;
      s = s >> 1;
    }
    if(significantBits >= 32)
	{
		snprintf(em->msg, 1024, "SimpleQueue: Too many bits in size_ variable");
		em->ec = EXE_EXTRACT_ERROR_WRITING_TO_FILE;
	}
    size_ = (1 << significantBits);

    if(! heap_)
	{
		snprintf(em->msg, 1024, "SimpleQueue: Invalid NAMemory pointer");
		em->ec = EXE_EXTRACT_ERROR_WRITING_TO_FILE;
	}

    UInt32 numBytes = size_ * sizeof(Entry);
    queue_ = (Entry *) heap_->allocateMemory(numBytes);
    //Note, failure of allocateMemory will result in lngJmp.
    memset(queue_, 0, numBytes);

    head_ = UINT32_MAX - 1;
    tail_ = UINT32_MAX - 1;
    mask_ = size_ - 1;
  }

  ~SimpleQueue()
  {
    // If the queue is used to store pointers, this destructor will not
    // delete the objects pointed to. The user of queue is responsible
    // for deleting objects before calling this destructor.

    //a cleanup call is used to clean this object. In EID, we expect
	//the EID root to delete the heap at the higher level.
  }

private:

  UInt32 head_;
  UInt32 tail_;
  UInt32 size_;

  // Using a bit mask is faster than masking with (size_ - 1).
  UInt32 mask_;

  // Pointer to an array of queue entries
  Entry *queue_;

  NAMemory *heap_;

}; // class SimpleQueue


class IOBuffer
{

public:

  friend class ExFastExtractTcb;
  enum BufferStatus {PARTIAL = 0, FULL, EMPTY, ERR};

  IOBuffer(char *buffer, Int32 bufSize)
    : bytesLeft_(bufSize),
      bufSize_(bufSize),
      status_(EMPTY)
  {
    data_ = buffer;
    //memset(data_, '\0', bufSize);

  }

  ~IOBuffer()
  {
  }

  void setStatus(BufferStatus val)
  {
    status_ = val;
  }
  BufferStatus getStatus()
  {
    return status_ ;
  }

  char* data_;
  Int32 bytesLeft_;
  Int32 bufSize_;
  BufferStatus status_;

};

// This is the thread state, it is one way communication back to EID
// indicating its state. EID never sets the thread state unless the
// thread has exited.
enum ThreadState
  {
	  THREAD_OK =0,
	  THREAD_ERROR,
	  THREAD_EXIT,
	  THREAD_KILLED
  };

// This is a flag to indicate some action to the thread. This is again
// one way communication from EID to the thread. The thread never sets
// this flag.
  enum ThreadFlag
  {
	  FLAG_NORMAL_RUN = 0,
	  FLAG_INFORM_EXIT,
	  FLAG_KILL_THREAD
  };

struct Params
{
  // pointer to queue used to exchange IOBuffers between EID thread and 
  // IO thread
  SimpleQueue *writeQueue;
  // mutex to guard access to writeQ
  pthread_mutex_t * queueMutex;
  // condition variable to wake up IO thread when writeQ has an IOBuffer
  pthread_cond_t * queueReadyCv;

  // Information needed for IO
  FileReadWrite * writeFile;

  // status of IO thread
  ThreadState *threadState;

  // Flags to inform actions to thread from EID
  ThreadFlag  *threadFlag;

  // contains errors produced by IO thread.
  ErrorMsg *em;

  //Print diags flag
  NABoolean printDiags;

};


class FileReadWrite
{
public:

  enum {NOT_CONNECTED = 0, CONNECTED, HAS_DATA, DONE, ERROR};

  enum Direction {READ_ = 0, WRITE_, READ_WRITE_};
  enum CommType { FILE_ = 0, SOCKETS_};

  FileReadWrite(CommType comType,
             Direction dr,
             const char *outputURL,
             NABoolean isAppend,
             ExFastExtractStats* feStats);


  ~FileReadWrite();


  ssize_t writeFD(char *data, UInt32 bytes, ErrorMsg *em, NABoolean printDiags);
  void flush();
  void close();
  void openFile();

  NABoolean atStartOfFile () const
  {return (lseek( outputFD_, 0, SEEK_END ) == 0);};

  NABoolean hasValidOutputFD() const;

public:
  ExFastExtractStats *feStats_; 	//cache the pointer to stats
  Int32 outputFD_; 					// file descriptor
  CommType comType_; 				// FILE or SOCKET
  Direction direction_; 			// READ, WRITE or READ_WRITE
  NABoolean append_;
  char outputURL_[513]; 			// filename
}; // class FileReadWrite

#endif /* EXFASTTRANSPORTIO_H_ */

