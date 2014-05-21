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
 * ExFastTransportIO.cpp
 *
 *  Created on: Aug 31, 2012
 */
#include "ExFastTransportIO.h"

#include "ex_stdh.h"
#include "ex_error.h"
#include "ExCextdecs.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "ExStats.h"
#include "ComSysUtils.h"
#include "ExFastTransport.h"

timespec timespecdiff(timespec start, timespec end)
{
	timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

FileReadWrite::FileReadWrite(CommType comType,
             Direction dr,
             const char *outputURL,
             NABoolean isAppend,
             ExFastExtractStats *feStats
             )
    :
      outputFD_(-1),
      comType_(comType),
      direction_(dr),
      append_(isAppend),
      feStats_(feStats)
{

  if (!outputURL || comType != FILE_)
    return ; // caller does error handling

  strncpy(&(outputURL_[0]), outputURL, 512);
}

FileReadWrite::~FileReadWrite()
{
  if (outputFD_ != -1)
  {
    ::close(outputFD_);
    outputFD_ = -1;
  }
}
void FileReadWrite::openFile()
{
	if (comType_ == FILE_)
	{
		if (append_)
			outputFD_ = open(outputURL_, O_CREAT | O_WRONLY | O_APPEND | O_LARGEFILE, 0644);
		else
			outputFD_ = open(outputURL_, O_CREAT | O_WRONLY | O_TRUNC | O_LARGEFILE, 0644);
	}
}

void FileReadWrite::close()
{
  if (outputFD_ != -1)
  {
    ::close(outputFD_);
    outputFD_ = -1;
  }
}

ssize_t FileReadWrite::writeFD(char *data, UInt32 bytes, ErrorMsg *em, NABoolean printDiags)
{
  ssize_t bytesWritten = -1;
  ssize_t totalBytesWritten = 0;
  ssize_t bytesToWrite = (ssize_t)bytes;
  // we don't use a while loop here to avoid any possibility of an indefinite
  // loop. Simply error out if a few attempts do not succeed in writing out 
  // the whole buffer.
  timespec tstart, tfinish;
  if(printDiags)
  {
	  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tstart);
  }

  for (int i = 0; i < 10; i++)
  {
    bytesWritten = write(outputFD_, data, bytesToWrite);
    if ((bytesWritten == -1)&&(errno != EINTR))
      break;
    if ((errno == EINTR) && (bytesWritten == -1))
      bytesWritten = 0;
 
    totalBytesWritten += bytesWritten;
    if (totalBytesWritten >= bytes)
      break;
    data += bytesWritten;
    bytesToWrite -= bytesWritten;
  }

  if(printDiags)
  {
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tfinish);
	timespec diff = timespecdiff(tstart, tfinish);
	printf("[FASTEXTRACT] Time taken for Write= %lu secs %lu nanosecs \n", diff.tv_sec, diff.tv_nsec);
	fflush(NULL);
  }

  if ((bytesWritten == -1)||(totalBytesWritten != bytes))
  {
    em->ec = EXE_EXTRACT_ERROR_WRITING_TO_FILE;

    if (bytesWritten == -1)
    {
      int currentMsgLen = 0;
      char * errDesc = NULL;
      currentMsgLen = snprintf(em->msg, 1024, "%s. File System Error:  %d. Error detail: ", outputURL_, errno);
      if ((currentMsgLen > 0)&&(currentMsgLen < 1024))
      {
        errDesc = strerror_r(errno, em->msg +currentMsgLen,1024-currentMsgLen);
        if ((errDesc != NULL)&&((errno != EINVAL)||(errno != ERANGE)))
          strncpy(em->msg + currentMsgLen, errDesc, 1024-currentMsgLen);
      }
    }
    else
    {
      snprintf(em->msg, 1024, "%s. Repeated attempts failed to write out a buffer completely",outputURL_);
    }
  }

  if (feStats_)
  {
    feStats_->incSentBuffersCount();
    feStats_->incSentBytes(bytesWritten);
  }
  return totalBytesWritten;
}
void FileReadWrite::flush()
{
  fsync(outputFD_);
}

NABoolean FileReadWrite::hasValidOutputFD() const
{
  if ((outputFD_ == 0)||(outputFD_ == -1))
    return FALSE;
  else
    return TRUE;
}

void * processIO(void * param)
{
  struct Params *p = (struct Params *) param;
  IOBuffer *buf = NULL;

  //This thread is sent cancel enabled so that we can
  //cancel this thread as part of cleanup , generally
  //during cleanup.
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

  //set thread cancel type to asynchronous in the case of
  //planned take over cleanup.
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

  //perform file open
  p->writeFile->openFile();
  if(! p->writeFile->hasValidOutputFD())
  {

	  int currentMsgLen = 0;
	  char * errDesc = NULL;
	  currentMsgLen = snprintf(p->em->msg, 1024, "%s. File System Error:  %d. Error detail: ", p->writeFile->outputURL_, errno);
	  if ((currentMsgLen > 0)&&(currentMsgLen < 1024))
	  {
	    errDesc = strerror_r(errno, p->em->msg +currentMsgLen,1024-currentMsgLen);
	        if ((errDesc != NULL)&&((errno != EINVAL)||(errno != ERANGE)))
	          strncpy(p->em->msg + currentMsgLen , errDesc, 1024-currentMsgLen);
	  }
	  p->em->ec = EXE_EXTRACT_ERROR_CREATING_FILE;

	  *p->threadState = THREAD_ERROR;
	  pthread_exit(NULL);
	  return NULL;
  }

  NABoolean threadError = FALSE;
  do
  {
    pthread_mutex_lock(p->queueMutex);
    if(p->writeQueue->isEmpty())
    {
      //now check to see if EID indicated to exit.
      if(*(p->threadFlag) == FLAG_INFORM_EXIT)
      {
    	//we will set the thread state at the end after
    	//closing the file.

        //need to unlock the mutex and exit.
        pthread_mutex_unlock(p->queueMutex);
        break; //breaks out of do loop.
      }
      //This thread automatically release mutex and goes to wait mode.
      //When this thread wakes up, the mutex is automatically locked for this thread.
      pthread_cond_wait(p->queueReadyCv, p->queueMutex);
    }

    if(!p->writeQueue->isEmpty())
    {
      SimpleQueue::Entry & ent=	p->writeQueue->getHeadEntry(p->em);
      if(p->em->ec != EXE_OK)
      {
    	  threadError = TRUE;
    	  pthread_mutex_unlock(p->queueMutex);
    	  break; //breaks out of do loop.
      }
      buf = (IOBuffer*) ent.getData();
      p->writeQueue->removeHead(p->em);
      if(p->em->ec != EXE_OK)
	  {
		threadError = TRUE;
		pthread_mutex_unlock(p->queueMutex);
		break; //breaks out of do loop.
	  }
    }
    //do unlock irrespective of if conditions we came out from above.
    pthread_mutex_unlock(p->queueMutex);
    //now work on the buffer if it available.
    if(buf)
    {
      ssize_t bytesToWrite = buf->bufSize_ - buf->bytesLeft_;
      if(p->writeFile->writeFD(buf->data_,
                            bytesToWrite,
                            p->em,
                            p->printDiags) != bytesToWrite)
      {
    	//We are setting the buffer status to empty outside
		//of a mutex lock and at the same time expect EID
		//thread to pick the buffer as soon as it is empty.
		//To avoid any race conditions, we use this asm call
		//to inform the compiler that the following set of
		//instructions are not reordered.
		//In the current scenario, there is no additional
		//instructions other than setting the buffer empty, but
		//this call is added for safety.
		asm volatile("" : : : "memory");

		buf->status_ = IOBuffer::ERR;
        threadError = TRUE;
        break; //breaks out of do loop.
      }
      else
      {
    	//We are setting the buffer status to empty outside
    	//of a mutex lock and at the same time expect EID
    	//thread to pick the buffer as soon as it is empty.
    	//To avoid any race conditions, we use this asm call
    	//to inform the compiler that the following set of
    	//instructions are not reordered.
    	//In the current scenario, there is no additional
    	//instructions other than setting the buffer empty, but
    	//this call is added for safety.
    	asm volatile("" : : : "memory");

    	buf->status_ = IOBuffer::EMPTY;
        buf = NULL;
      }
    }
  }while(1);

  //Flush the file before close
  timespec tstart, tfinish;
  if(p->printDiags)
  {
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tstart);
  }
  p->writeFile->flush();
  if(p->printDiags)
  {
  	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tfinish);
  	timespec diff = timespecdiff(tstart, tfinish);
  	printf("[FASTEXTRACT] Time taken for flush= %lu secs %lu nanosecs \n", diff.tv_sec, diff.tv_nsec);
  	fflush(NULL);
  }
  //Lets close the file before exit.
  p->writeFile->close();

  if(threadError)
  {
	//We are setting the thread state outside
	//of a mutex lock and at the same time expect EID
	//thread to pick the state as soon as it is set.
	//To avoid any race conditions, we use this asm call
	//to inform the compiler that the following set of
	//instructions are not reordered.
	//In the current scenario, there is no additional
	//instructions other than setting the thread state, but
	//this call is added for safety.
	asm volatile("" : : : "memory");
	*p->threadState = THREAD_ERROR;
  }
  else
  {
	//We are setting the thread state outside
	//of a mutex lock and at the same time expect EID
	//thread to pick the state as soon as it is set.
	//To avoid any race conditions, we use this asm call
	//to inform the compiler that the following set of
	//instructions are not reordered.
	//In the current scenario, there is no additional
	//instructions other than setting the thread state, but
	//this call is added for safety.
	asm volatile("" : : : "memory");
	*p->threadState = THREAD_EXIT;

  }
  pthread_exit(NULL);

  return NULL;
}

